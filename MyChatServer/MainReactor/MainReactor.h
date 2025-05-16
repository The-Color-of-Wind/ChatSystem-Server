#ifndef MAINREACTOR_H
#define MAINREACTOR_H

#include "../lock/locker.h"

#include "../log/log.h"
#include "../timer/lst_timer.h"

#include "../IniConfig/IniConfig.h"
#include "../MysqlConnectPool/sql_connect_pool.h"
#include "../ThreadPool/ThreadPool.h"
#include "../ChatMapping/ChatMapping.h"
#include "../ChatConn/chat_conn.h"
#include "../SubReactor/SubReactor.h"

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
#include <vector>
#include <unordered_map>
#include <sys/eventfd.h>



#define MAX_EVENT_NUMBER 100000 //����¼���
//���ö�������m_read_buf��С
static const int READ_BUFFER_SIZE = 2048;
//����д������m_write_buf��С
static const int WRITE_BUFFER_SIZE = 2048;

class MainReactor
{
public:
	MainReactor(int port, int num_reactors);
	~MainReactor();
	void run();

private:

	void create_listener();
	void assign_connection(int client_fd);

	static void* startServer(void* arg);


private:
	int listen_fd;
	int main_epoll_fd;
	std::vector<SubReactor*> sub_reactors;
	int num_reactors;
	int port;

	std::atomic<int> next_reactor{ 0 };	// ȷ��next_reactor++Ϊԭ�Ӳ���
	bool stop_server;

};

#endif
