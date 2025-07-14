#ifndef CHATCONNECTION_H
#define CHATCONNECTION_H
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include<string>
#include <sys/wait.h>
#include <sys/uio.h>
#include<mysql/mysql.h>
#include <iostream>

#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <string>
#include <sstream>
#include <json/json.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <vector>
#include <queue>
#include "json.hpp"
using json = nlohmann::json;
#include "../lock/locker.h"
#include "../MysqlConnectPool/sql_connect_pool.h"
#include "../log/log.h"
#include "../ChatMapping/ChatMapping.h"
#include "../ThreadPool/ThreadPool.h"
#include "../TaskQueue/DBTaskQueue.h"

#include "user.h"

using namespace std;

class chat_conn : public std::enable_shared_from_this<chat_conn>
{
public:	// �ֽ�����

	chat_conn(int m_epollfd_, int m_sockfd_, int m_eventfd_, ThreadPool<chat_conn>* pool_, DBTaskQueue* dbTaskQueue_=nullptr) : m_epollfd(m_epollfd_), m_sockfd(m_sockfd_), m_eventfd(m_eventfd_), pool(pool_), dbTaskQueue(dbTaskQueue_), head_len(4) {
		user = new User();

		init();

	}

public:

	//���ö�������m_read_buf��С
	static const int READ_BUFFER_SIZE = 2048;
	//����д������m_write_buf��С
	static const int WRITE_BUFFER_SIZE = 2048;

public:
	//������ӹ���
	enum CHAT_CODE
	{
		LOGIN,
		REGISTER,
		FRIENDS,
		CHATTING,
		SEND_MESSAGE,
		RECEIVE_MESSAGE,
		UPDATE_CHAT,
		GET_UNREAD_MESSAGES,
		SEARCH_USER,
		ADD_FRIEND,
		ACCEPT_FRIEND,
		DELETE_FRIEND,
		GROUP_TEXT,
		NOCODE
	};

public:
	void init(int sockfd, const sockaddr_in& addr);
	void init();
	void readInit();
	void writeInit();
	void removeUser();

	void init_login(string id, string password);

	void init_register(string phone, string name, string password);

	void process();
	CHAT_CODE process_read(Json::Value jsonData);
	bool process_write(CHAT_CODE ret, Json::Value jsonData);

	bool read_once();
	bool parseJsonData();
	bool write();
	bool receive();
	bool serverSendMessage();

	void wirteWriteBuf(string str);


	string getMysql_UserResult(const string& type, const string& sqlStatement, Json::Value jsonData);
	string setMysql_UserResult(const string& type, const string& sqlStatement, Json::Value jsonData);
	string getMysql_FriendsResult(const string& type, const string& sqlStatement, Json::Value jsonData);
	string getMysql_ChatsResult(const string& type, const string& sqlStatement, Json::Value jsonData);
	string sendMessage_Mysql(const string& type, string& sqlStatement, Json::Value jsonData);
	string updateChat_Mysql(Json::Value jsonData);
	string getFriendsUnreadMessages_Mysql(const string& type, const string& sqlStatement, Json::Value jsonData);
	string searchUserInformation_Mysql(const string& type, const string& sqlStatement, Json::Value jsonData);
	string modifyFriend_Mysql(const string& type, const string& sqlStatement, Json::Value jsonData);


	void setConnPool(connection_pool* connPool) {
		this->connPool = connPool;
	}

	bool getSendMessage() { return this->sendMessage; }

	bool parse_messages();
	// �ύ��һ����������ֻ�����̳߳��̵߳��ã�
	void submit_next(ThreadPool<chat_conn>& pool);

	// ʵ�ʴ�����Ϣ�߼���ʾ����
	void handle_message(const std::string& msg);

	// ����Ϣ��Ӳ������ύ��������
	void push_message(const std::string& msg, ThreadPool<chat_conn>& pool);
public:
	//ÿһ�û����ӹ��е�
	int m_epollfd;
	MYSQL* mysql;
	int m_sockfd;
	int m_eventfd;

	connection_pool* connPool;
	User* user;

	

	//�洢��ȡ����������
	char m_read_buf[READ_BUFFER_SIZE];
	//��������m_read_buf�����ݵ����һ���ֽڵ���һ��λ��
	int m_read_idx;
	//m_read_buf��ȡ��λ��
	int m_checked_idx;
	//m_read_buf���Ѿ��������ַ�����
	int m_start_line;

	//�洢��������Ӧ����
	char m_write_buf[WRITE_BUFFER_SIZE];
	locker write_lock;
	//ָʾbuffer�еĳ���
	int m_write_idx;

	struct iovec m_iv[1];	//io��������iovec
	int m_iv_count;
	int bytes_to_send;		//ʣ�෢���ֽ���
	int bytes_have_send;	//�ѷ����ֽ���

	//Json::Value jsonData;

	//�ж��Ƿ���Ϣ���ǻظ�
	bool sendMessage = false;

	size_t head_len = 4;

	ThreadPool<chat_conn>* pool;
	DBTaskQueue* dbTaskQueue;



public:
	std::mutex mtx;
	std::queue<std::string> msg_queue; // ��Ϣ����
	bool is_processing;                // �Ƿ����ڴ�����



};

#endif