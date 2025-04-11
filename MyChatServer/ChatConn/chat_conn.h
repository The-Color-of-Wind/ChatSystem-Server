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

#include "user.h"

using namespace std;

class chat_conn
{
public:	// �ֽ�����
	bool parse_messages();
	enum ReadState {	// ��ȡ��Ϣ״̬��
		READ_LENGTH,
		READ_BODY
	};
	ReadState recv_state;	// ��ʼ��ȡ��Ϣͷ
	size_t head_len = 4;
	queue<Json::Value> jsonTaskQueue;
	Json::Value jsonData;


public:

	//���ö�ȡ�ļ�������m_real_file��С
	static const int FILENAME_LEN = 200;
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
	CHAT_CODE process_read();
	bool process_write(CHAT_CODE ret);
	bool read_once();
	bool parseJsonData();
	bool write();
	bool serverSendMessage();

	void wirteWriteBuf(string str);


	string getMysql_UserResult(const string& type, const string& sqlStatement);
	string setMysql_UserResult(const string& type, const string& sqlStatement);
	string getMysql_FriendsResult(const string& type, const string& sqlStatement);
	string getMysql_ChatsResult(const string& type, const string& sqlStatement);
	string sendMessage_Mysql(const string& type, string& sqlStatement);
	string updateChat_Mysql();
	string getFriendsUnreadMessages_Mysql(const string& type, const string& sqlStatement);
	string searchUserInformation_Mysql(const string& type, const string& sqlStatement);
	string modifyFriend_Mysql(const string& type, const string& sqlStatement);


	sockaddr_in* get_address()
	{
		return &m_address;
	}

	void setConnPool(connection_pool* connPool) {
		this->connPool = connPool;
	}

	bool getSendMessage() { return this->sendMessage; }
public:
	//ÿһ�û����ӹ��е�
	static int m_epollfd;
	static int m_user_count;
	MYSQL* mysql;

private:
	connection_pool* connPool;
	User* user;

	int m_sockfd;
	sockaddr_in m_address;

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
	int sendSocketFd;
};

#endif