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

#define MAX_EVENT_NUMBER 100000 //����¼���
#define MAX_FD 65536           //����ļ�������


//������������chat_conn.cpp�ж��壬�ı���������
extern int addfd(int epollfd, int fd, bool one_shot);
extern int remove(int epollfd, int fd);
extern int setnonblocking(int fd);

static int epollfd = 0;		//epollģ��
static int pipefd[2];		//�ź�ͨ�Źܵ�
static sort_timer_lst timer_lst;	//��ʱ����������


class ChatServer
{
public:
	ChatServer() : chatMapping(ChatMapping::getInstance()) {  // �ڹ��캯����ʼ���б��г�ʼ������
	}
	void readConfigFile();	// ��ȡ�����ļ�
	void initConnPool();		// ��ʼ�����ݿ����ӳ�
	void initThreadPool();	// ��ʼ���̳߳�
	void initUsers();
	void initEpoll();
	void initlistensocket(int efd, short port);	// ��ʼ��Lfd

	void init();	// ����epoll
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
