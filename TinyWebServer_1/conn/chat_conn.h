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
#include "json.hpp"
using json = nlohmann::json;
#include "../lock/locker.h"
#include "../CGImysql/sql_connect_pool.h"

#include "user.h"
#include "Server.h"

using namespace std;

class chat_conn
{
public:

	//设置读取文件的名称m_real_file大小
	static const int FILENAME_LEN = 200;
	//设置读缓冲区m_read_buf大小
	static const int READ_BUFFER_SIZE = 2048;
	//设置写缓冲区m_write_buf大小
	static const int WRITE_BUFFER_SIZE = 2048;
	
public:
	//可以添加功能
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
	void init(int sockfd, const sockaddr_in &addr);
	void init();

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
	


	string getMysql_UserResult(const string &type, const string &sqlStatement);
	string setMysql_UserResult(const string &type, const string &sqlStatement);
	string getMysql_FriendsResult(const string &type, const string &sqlStatement);
	string getMysql_ChatsResult(const string &type, const string &sqlStatement);
	string sendMessage_Mysql(const string &type, string &sqlStatement);
	string updateChat_Mysql();
	string getFriendsUnreadMessages_Mysql(const string &type, const string &sqlStatement);
	string searchUserInformation_Mysql(const string &type, const string &sqlStatement);
	string modifyFriend_Mysql(const string &type, const string &sqlStatement);


	sockaddr_in *get_address()
	{
		return &m_address;
	}

	void setConnPool(connection_pool *connPool) {
		this->connPool = connPool;
	}

	bool getSendMessage() { return this->sendMessage; }
public:
	//每一用户连接共有的
	static int m_epollfd;
	static int m_user_count;
	MYSQL *mysql;

private:
	connection_pool *connPool;
	User *user;

	int m_sockfd;
	sockaddr_in m_address;

	//存储读取的请求数据
	char m_read_buf[READ_BUFFER_SIZE];
	//缓冲区中m_read_buf中数据的最后一个字节的下一个位置
	int m_read_idx;
	//m_read_buf读取的位置
	int m_checked_idx;
	//m_read_buf中已经解析的字符个数
	int m_start_line;

	//存储发出的响应数据
	char m_write_buf[WRITE_BUFFER_SIZE];
	//指示buffer中的长度
	int m_write_idx;

	struct iovec m_iv[1];	//io向量机制iovec
	int m_iv_count;
	int bytes_to_send;		//剩余发送字节数
	int bytes_have_send;	//已发送字节数

	Json::Value jsonData;
	//json receiveResponseJsonArray;
	//vector<Json::Value> receiveResponseJsonVector;
	//json sendResponseJsonArray;
	//string jsonStr;

	//判断是发信息还是回复
	bool sendMessage = false;
	int sendSocketFd;
};

#endif