#ifndef CHATSERVER_H
#define CHATSERVER_H

#include "../lock/locker.h"

#include "../log/log.h"
#include "../timer/lst_timer.h"

#include "../IniConfig/IniConfig.h"
#include "../MysqlConnectPool/sql_connect_pool.h"
#include "../ThreadPool/LockFreeThreadPool.h"
#include "../ThreadPool/threadpool.h"
#include "../ChatMapping/ChatMapping.h"
#include "../ChatConn/chat_conn.h"

#include <string>
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

using namespace std;

#define MAX_EVENT_NUMBER 100000 //最大事件数
#define MAX_FD 65536           //最大文件描述符


//这三个函数在chat_conn.cpp中定义，改变链接属性
extern int addfd(int epollfd, int fd, bool one_shot);
extern int remove(int epollfd, int fd);
extern int setnonblocking(int fd);

static int epollfd = 0;		//epoll模板
static int pipefd[2];		//信号通信管道
static sort_timer_lst timer_lst;	//定时器容器链表


class ChatServer
{
public:
	ChatServer() : chatMapping(ChatMapping::getInstance()) {  // 在构造函数初始化列表中初始化引用
	}
	void readConfigFile();	// 读取配置文件
	void initConnPool();		// 初始化数据库连接池
	void initThreadPool();	// 初始化线程池
	void initUsers();
	void initEpoll();
	void initlistensocket(int efd, short port);	// 初始化Lfd

	void init();	// 创建epoll
	void startServer();
	void stopServer();

private:
	
	connection_pool* connPool;
	//LockFreeThreadPool<chat_conn>* pool;
	threadpool<chat_conn>* pool;
	ChatMapping& chatMapping;

private:
	//shared_ptr<chat_conn> users;
	chat_conn* users;

	struct epoll_event events[MAX_EVENT_NUMBER];
	
	int listenfd;

	bool stop_server;
	struct sockaddr_in address;


};

#endif
