#ifndef SUBREACTOR_H
#define SUBREACTOR_H

#include "../lock/locker.h"

#include "../log/log.h"
#include "../timer/lst_timer.h"

#include "../IniConfig/IniConfig.h"
#include "../MysqlConnectPool/sql_connect_pool.h"
#include "../ThreadPool/ThreadPool.h"
#include "../ChatMapping/ChatMapping.h"
#include "../ChatConn/chat_conn.h"
#include "../fun/fun.h"
#include "../TaskQueue/DBTaskQueue.h"


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
#include <unordered_map>
#include <shared_mutex>

using namespace std;

#define MAX_EVENT_NUMBER 100000 //����¼���
#define MAX_FD 65536           //����ļ�������


class SubReactor
{
public:

	void init();

	void initConnPool();		// ��ʼ�����ݿ����ӳ�
	void initThreadPool();	// ��ʼ���̳߳�

	void startServer(int epollFd);
	void stopServer();


	bool receive_data(chat_conn* conn);

	bool send_data(chat_conn* conn);

	void addMap(string userid, int socketfd);
	int getSocket(string userid);

	void removeMap(string userid);

	void dbWriterLoop();

public:
	connection_pool* connPool;
	ThreadPool<chat_conn>* pool;

	int epoll_fd;
	int event_fd;
	bool stop_server;

	pthread_t thread; // ���ڱ����߳̾��
	std::atomic<bool> running_ = true;
	std::vector<std::thread> dbThreads;

	std::shared_mutex map_rw_mutex;
	std::unordered_map<std::string, int> map_id_socket;

	int conn_count;

	DBTaskQueue* dbTaskQueue;
};

#endif
