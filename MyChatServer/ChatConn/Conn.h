#ifndef CONNBUFFER_H
#define CONNBUFFER_H
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


class ConnBuffer 
{
public:
	ConnBuffer(int m_sockfd_) : m_sockfd(m_sockfd_), recv_state(READ_LENGTH), head_len(4), m_read_idx(0), m_write_idx(0), bytes_to_send(0), bytes_have_send(0) {
		memset(m_read_buf, '\0', READ_BUFFER_SIZE);		//存储读取的请求报文数据
		memset(m_write_buf, '\0', WRITE_BUFFER_SIZE);	//存储发出的响应报文数据
	}


private:
	int m_sockfd;

	// 读缓冲区
	size_t head_len;
	char m_read_buf[READ_BUFFER_SIZE];
	int m_read_idx;

	// 写缓冲区
	locker write_lock;
	char m_write_buf[WRITE_BUFFER_SIZE];
	int m_write_idx;
	int bytes_to_send;		//剩余发送字节数
	int bytes_have_send;	//已发送字节数

	struct iovec m_iv[1];	//io向量机制iovec
	int m_iv_count;


	queue<Json::Value> jsonTaskQueue;
	Json::Value jsonData;
	

};



#endif