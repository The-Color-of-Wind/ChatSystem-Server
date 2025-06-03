#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <cassert>
#include <sys/epoll.h>

#include "./lock/locker.h"
#include "./threadpool/threadpool.h"
#include "conn/chat_conn.h"
#include "./log/log.h"
#include "./timer/lst_timer.h"
#include "./CGImysql/sql_connect_pool.h"
//#include "./conn/Server.h"

using namespace std;

#define MAX_FD 65536           //最大文件描述符
#define MAX_EVENT_NUMBER 10000 //最大事件数
#define TIMESLOT 5			//最小超时单位

//#define SYNLOG	//同步写日志
#define ASYNLOG	//异步写日志

//#define listenfdET //边缘触发非阻塞
#define listenfdLT //水平触发阻塞

//这三个函数在http_conn.cpp中定义，改变链接属性
extern int addfd(int epollfd, int fd, bool one_shot);
extern int remove(int epollfd, int fd);
extern int setnonblocking(int fd);

//静态参数
static int epollfd = 0;		//epoll模板
static int pipefd[2];		//信号通信管道
static sort_timer_lst timer_lst;	//定时器容器链表


//信号处理函数
void sig_handler(int sig)
{
	//为保证函数的可重入性，保留原来的errno
	//可重入性表示中断后再次进入该函数，环境变量与之前相同，不会丢失数据
	int save_errno = errno;
	int msg = sig;
	//将信号值从管道写端写入，传输字符类型，而非整型
	send(pipefd[1], (char *)&msg, 1, 0);

	errno = save_errno;
}

//设置信号函数
void addsig(int sig, void(handler)(int), bool restart = true)
{
	//创建sigaction结构体变量
	struct sigaction sa;
	memset(&sa, '\0', sizeof(sa));
	sa.sa_handler = handler;
	if (restart)
		sa.sa_flags |= SA_RESTART;
	sigfillset(&sa.sa_mask);
	//执行sigaction函数
	assert(sigaction(sig, &sa, NULL) != -1);
}

//定时处理任务，重新定时以不断触发SIGALRM信号
void timer_handler()
{
	timer_lst.tick();	//定时任务处理函数
	alarm(TIMESLOT);
}

//定时器回调函数，删除非活动连接在socket上的注册事件，并关闭
void cb_func(client_data *user_data)
{
	//删除事件
	epoll_ctl(epollfd, EPOLL_CTL_DEL, user_data->sockfd, 0);
	assert(user_data);
	close(user_data->sockfd);
	chat_conn::m_user_count--;
}
void show_error(int connfd, const char *info)
{
	printf("%s", info);
	send(connfd, info, strlen(info), 0);
	close(connfd);
}

//初始化lfd
int initlistensocket(int efd, short port)
{
	int listenfd = socket(AF_INET, SOCK_STREAM, 0);
	assert(listenfd >= 0);

	//	fcntl(listenfd, F_SETFL, O_NONBLOCK);	//将socket设为非阻塞

	struct sockaddr_in address;
	bzero(&address, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	address.sin_port = htons(port);

	//设置端口复用
	int flag = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

	int ret = bind(listenfd, (struct sockaddr *)&address, sizeof(address));
	assert(ret >= 0);
	ret = listen(listenfd, 5);
	assert(ret >= 0);

	addfd(epollfd, listenfd, false);
	chat_conn::m_epollfd = epollfd;

	return listenfd;
}


// 断开连接的函数
void disconnect(int cfd, int epfd)
{
	int ret = epoll_ctl(epfd, EPOLL_CTL_DEL, cfd, NULL);
	if (ret == -1) {
		perror("epoll_ctl del cfd error");
		exit(1);
	}
	close(cfd);
}

int main(int argc, char *argv[])
{
#ifdef ASYNLOG
	Log::get_instance()->init("ServerLog", 200, 800000, 8);	//异步日志模型
#endif

#ifdef SYNLOG
	Log::get_instance()->init("ServerLog", 200, 800000, 0);	//同步日志模型
#endif

	//printf("action\n");

	//读取输入
	if (argc < 2) {
		//printf("please input : ./a port path\n");
		printf("please input : port\n");
	}
	int port = atoi(argv[1]);
	int ret;


	
	//创建数据库连接池
	connection_pool *connPool = connection_pool::GetInstance();
	connPool->init("localhost", "root", "666666", "animalchat", 3306, 50);


	//创建线程池
	threadpool<chat_conn> *pool = NULL;
	try
	{
		pool = new threadpool<chat_conn>(connPool, 200, MAX_EVENT_NUMBER);
	}
	catch (...)
	{
		return 1;
	}
	//创建映射
	Server::instance();

	//创建chat对象
	chat_conn *users = new chat_conn[MAX_FD];
	assert(users);
	for (int i = 0; i < MAX_FD; i++) {
		users[i].setConnPool(connPool);
	}

	/*
	//初始化数据库读取表,将用户名密码放入map中
	users->initmysql_result(connPool);
	users->init_login();
	*/

	//创建epoll模型
	struct epoll_event events[MAX_EVENT_NUMBER];
	epollfd = epoll_create(5);
	assert(epollfd != -1);


	//初始化listenlfd
	int listenfd = initlistensocket(epollfd, port);


	//设置定时信号

	addsig(SIGPIPE, SIG_IGN);
	//创建管道套接字
	ret = socketpair(PF_UNIX, SOCK_STREAM, 0, pipefd);
	assert(ret != -1);
	//设置管道写端为非阻塞，防止阻塞，导致信号调用函数的执行时间增加
	setnonblocking(pipefd[1]);
	//设置管道读端为ET非阻塞
	addfd(epollfd, pipefd[0], false);
	//传递给主循环的信号值，这里只关注SIGALRM和SIGTERM
	addsig(SIGALRM, sig_handler, false);
	addsig(SIGTERM, sig_handler, false);

	//创建连接资源数组
	client_data *users_timer = new client_data[MAX_FD];

	//循环条件
	bool stop_server = false;
	//超时标志
	bool timeout = false;
	//每隔TIMESLOT时间触发SIGALRM信号
	//alarm(TIMESLOT);

	while (!stop_server)
	{
		int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
		if (number < 0 && errno != EINTR)
		{
			break;
		}
		for (int i = 0; i < number; i++)
		{
			int sockfd = events[i].data.fd;
			if (sockfd == listenfd)
			{
				//cout << "监听连接触发，创建连接！" << endl;
				struct sockaddr_in client_address;
				socklen_t client_addrlength = sizeof(client_address);
#ifdef listenfdLT
				int connfd = accept(listenfd, (struct sockaddr *)&client_address, &client_addrlength);
				if (connfd < 0)
				{
					LOG_ERROR("%s:errno is:%d", "accept error", errno);
					continue;
				}
				if (chat_conn::m_user_count >= MAX_FD)
				{
					show_error(connfd, "Internal server busy");
					LOG_ERROR("%s", "Internal server busy");
					continue;
				}

				//初始化用户,init里面有加入红黑树
				users[connfd].init(connfd, client_address);

				//初始化该链接对应的连接资源
				users_timer[connfd].address = client_address;
				users_timer[connfd].sockfd = connfd;

				util_timer *timer = new util_timer;
				timer->user_data = &users_timer[connfd];
				timer->cb_func = cb_func;
				//设置绝对超时时间
				time_t cur = time(NULL);
				timer->expire = cur + 3 * TIMESLOT;

				//创建该连接对应的定时器，初始化为前述临时变量
				users_timer[connfd].timer = timer;
				timer_lst.add_timer(timer);

#endif
#ifdef listenfdET
				while (1)
				{
					int connfd = accept(listenfd, (struct sockaddr *)&client_address, &client_addrlength);
					if (connfd < 0)
					{
						LOG_ERROR("%s:errno is:%d", "accept error", errno);
						continue;
					}
					if (chat_conn::m_user_count >= MAX_FD)
					{
						show_error(connfd, "Internal server busy");
						LOG_ERROR("%s", "Internal server busy");
						continue;
					}

					//初始化用户,init里面有加入红黑树
					users[connfd].init(connfd, client_address);

					//初始化该链接对应的连接资源
					users_timer[connfd].address = client_address;
					users_timer[connfd].sockfd = connfd;

					util_timer *timer = new util_timer;
					timer->user_data = &users_timer[connfd];
					timer->cb_func = cb_func;
					//设置绝对超时时间
					time_t cur = time(NULL);
					timer->expire = cur + 3 * TIMESLOT;

					//创建该连接对应的定时器，初始化为前述临时变量
					users_timer[connfd].timer = timer;
					timer_lst.add_timer(timer);

				}
				continue;
#endif
			}

			//客户端关闭
			else if (events[i].events &(EPOLLRDHUP | EPOLLHUP | EPOLLERR))
			{
				//cout << "客户端关闭" << endl;
				//服务器端关闭连接，移除对应的定时器
				util_timer *timer = users_timer[sockfd].timer;
				timer->cb_func(&users_timer[sockfd]);
				if (timer) {
					timer_lst.del_timer(timer);
				}
				users[sockfd].removeUser();
				//Server::instance()->removeUserSocket(sockfd);
			}

			//处理信号（管道读端发送读事件）
			else if ((sockfd == pipefd[0]) && (events[i].events &EPOLLIN))
			{
				int sig;
				char signals[1024];

				ret = recv(pipefd[0], signals, sizeof(signals), 0);
				if (ret == -1) {
					printf("handle the error\n");
					continue;
				}
				else if (ret == 0) {
					continue;
				}
				else {
					//处理信号值对应的逻辑
					for (int i = 0; i < ret; ++i) {
						switch (signals[i])
						{
						case SIGALRM:
						{
							//timeout = true;
							//break;
						}
						case SIGTERM:
						{
							//stop_server = true;
						}
						}

					}
				}

			}

			//处理客户连接上的读信号
			else if (events[i].events & EPOLLIN)
			{

				//printf("EPOLLIN success\n");
				util_timer *timer = users_timer[sockfd].timer;
				if (users[sockfd].read_once())	//读出数据
				{
					LOG_INFO("deal with the client(%s)", inet_ntoa(users[sockfd].get_address()->sin_addr));
					Log::get_instance()->flush();
					pool->append(users + sockfd);	//将任务加入队列
					//若有数据传输，则将定时器往后延迟3个单位
					//并对新的定时器在链表上的位置进行调整
					if (timer) {
						time_t cur = time(NULL);
						timer->expire = cur + 3 * TIMESLOT;
						LOG_INFO("%s", "adjust timer once");
						Log::get_instance()->flush();
						timer_lst.adjust_timer(timer);
					}

				}
				else
				{
					timer->cb_func(&users_timer[sockfd]);
					if (timer) {
						timer_lst.del_timer(timer);
					}
				}

			}

			//处理写事件
			else if (events[i].events &EPOLLOUT)
			{
				//printf("EPOLLOUT success\n");
				util_timer *timer = users_timer[sockfd].timer;
				if (users[sockfd].getSendMessage()) {
					users[sockfd].serverSendMessage();
				}
				else {
					if (users[sockfd].write())
					{
						LOG_INFO("send data to the client(%s)", inet_ntoa(users[sockfd].get_address()->sin_addr));
						Log::get_instance()->flush();

						//printf("write success\n");

						if (timer) {
							time_t cur = time(NULL);
							timer->expire = cur + 3 * TIMESLOT;
							LOG_INFO("%s", "adjust timer once");
							Log::get_instance()->flush();
							timer_lst.adjust_timer(timer);
						}
					}
					else
					{
						timer->cb_func(&users_timer[sockfd]);
						if (timer) {
							timer_lst.del_timer(timer);
						}
					}
				}
				

			}
		}
		if (timeout)
		{
			//timer_handler();
			timeout = false;
		}
	}

	close(epollfd);
	close(listenfd);
	close(pipefd[1]);
	close(pipefd[0]);

	delete[] users;
	delete[] users_timer;
	delete pool;

	return 0;
}
