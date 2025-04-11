#include "ChatServer.h"


void ChatServer::readConfigFile()
{
	IniConfig::getInstance().iniConfigData();
}

void ChatServer::initConnPool()
{
	connPool = connection_pool::GetInstance();

	connPool->init(IniConfig::getInstance().db_host, IniConfig::getInstance().db_user, IniConfig::getInstance().db_pwd, IniConfig::getInstance().db_name, IniConfig::getInstance().db_port, IniConfig::getInstance().db_conn_num);

}

void ChatServer::initThreadPool()
{
	//创建线程池
	pool = NULL;
	try
	{
		pool = new threadpool<chat_conn>(connPool, 16, IniConfig::getInstance().getThreadNum());
	}
	catch (...)
	{
		return;
	}
}
void ChatServer::initUsers()
{
	//创建映射
	chatMapping.setUsers(new chat_conn[MAX_FD]);
	users = ChatMapping::getInstance().getUsers();
	//users = new chat_conn[MAX_FD];
	assert(users);
	for (int i = 0; i < MAX_FD; i++) {
		users[i].setConnPool(connPool);
	}
}
void ChatServer::initEpoll()
{
	epollfd = epoll_create(5);
	assert(epollfd != -1);
}

//初始化lfd
void ChatServer::initlistensocket(int efd, short port)
{
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	assert(listenfd >= 0);

	//	fcntl(listenfd, F_SETFL, O_NONBLOCK);	//将socket设为非阻塞

	bzero(&address, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	address.sin_port = htons(port);

	//设置端口复用
	int flag = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

	int ret = bind(listenfd, (struct sockaddr*)&address, sizeof(address));
	assert(ret >= 0);
	ret = listen(listenfd, 5);
	assert(ret >= 0);

	addfd(epollfd, listenfd, false);
	chat_conn::m_epollfd = epollfd;

	return;
}

void ChatServer::init()
{
	initConnPool();
	initThreadPool();
	initUsers();

}


void ChatServer::startServer()
{
	//创建epoll模型
	//struct epoll_event events[IniConfig::getInstance().getThreadNum()];
	
	
	initEpoll();
	
	initlistensocket(epollfd, IniConfig::getInstance().port);

	stop_server = false;	//循环条件

	while (!stop_server)
	{
		//cout << "IniConfig::getInstance().getThreadNum():" << IniConfig::getInstance().getThreadNum() << endl;
		int number = epoll_wait(epollfd, events, IniConfig::getInstance().getThreadNum(), -1);
		if (number < 0 && errno != EINTR)
		{
			cout << "errno:" << endl;
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
				if (!IniConfig::getInstance().getListenEt()) {
					int connfd = accept(listenfd, (struct sockaddr*)&client_address, &client_addrlength);
					if (connfd < 0)
					{
						printf("%s:errno is:%d", "accept error", errno);
						continue;
					}
					if (chat_conn::m_user_count >= MAX_FD)
					{
						cout << connfd << "Internal server busy" << endl;
						continue;
					}

					//初始化用户,init里面有加入红黑树
					users[connfd].init(connfd, client_address);
				}
				else {
					while (1)
					{
						int connfd = accept(listenfd, (struct sockaddr*)&client_address, &client_addrlength);
						if (connfd < 0)
						{
							printf("%s:errno is:%d", "accept error", errno);
							continue;
						}
						if (chat_conn::m_user_count >= MAX_FD)
						{
							cout << connfd << "Internal server busy" << endl;
							continue;
						}

						//初始化用户,init里面有加入红黑树
						users[connfd].init(connfd, client_address);
					}
					continue;
				}
			}

			//客户端关闭
			else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
			{
				users[sockfd].removeUser();
			}
			/*
			//处理信号（管道读端发送读事件）
			else if ((sockfd == pipefd[0]) && (events[i].events & EPOLLIN))
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
			*/
			//处理客户连接上的读信号
			else if (events[i].events & EPOLLIN)
			{

				if (users[sockfd].read_once())	//读出数据
				{
					pool->append(users + sockfd);	//将任务加入队列

				}

			}

			//处理写事件
			else if (events[i].events & EPOLLOUT)
			{

				if (users[sockfd].getSendMessage()) {
					users[sockfd].serverSendMessage();
				}
				else {
					if (users[sockfd].write())
					{
						//printf("write success\n");
					}
					else
					{
						cerr << "write error" << endl;
					}
				}


			}
		}
	}

}

void ChatServer::stopServer()
{
	close(epollfd);
	close(listenfd);

	delete[] users;
	delete pool;
}