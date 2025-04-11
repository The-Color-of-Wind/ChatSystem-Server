#include"chat_conn.h"

#include <dirent.h>
#include <ctype.h>

#include<map>
#include <mysql/mysql.h>
#include <fstream>

#include<iostream>
using namespace std;

#define connfdET	//边缘触发非阻塞
//#define connfdLT	//水平触发阻塞

//#define listenfdET //边缘触发非阻塞
#define listenfdLT //水平触发阻塞

//对文件描述符设置非阻塞
int setnonblocking(int fd)
{
	int old_option = fcntl(fd, F_GETFL);
	int new_option = old_option | O_NONBLOCK;
	fcntl(fd, F_SETFL, new_option);

	return old_option;
}
//向epoll实例中注册读事件，选择是否开启EPOLLONESHOT
void addfd(int epollfd, int fd, bool one_shot)
{
	epoll_event event;
	event.data.fd = fd;

#ifdef connfdET
	event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
#endif

#ifdef connfdLT
	event.events = EPOLLIN | EPOLLRDHUP;
#endif

#ifdef listenfdET
	event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
#endif

#ifdef listenfdLT
	event.events = EPOLLIN | EPOLLRDHUP;
#endif

	if (one_shot)
		event.events |= EPOLLONESHOT;
	epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
	setnonblocking(fd);
}
//从epoll实例中删除事件
void removefd(int epollfd, int fd)
{
	epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
	close(fd);
}


//修改事件
void modfd(int epollfd, int fd, int ev)
{

	epoll_event event;
	event.data.fd = fd;


#ifdef connfdET
	event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
#endif

#ifdef connfdLT
	event.events = ev | EPOLLONESHOT | EPOLLRDHUP;
#endif

	epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
	return;
}

void printJsonData(const Json::Value& jsonData) {
	Json::StreamWriterBuilder writer;
	std::string jsonString = Json::writeString(writer, jsonData);
	//std:://cout << jsonString << std::endl;
}


//静态变量要在类外初始化
int chat_conn::m_user_count = 0;
int chat_conn::m_epollfd = -1;

//初始化套接字地址，插入红黑树，函数内部会调用私有方法init（变量赋初值）
void chat_conn::init(int sockfd, const sockaddr_in& addr)
{
	m_sockfd = sockfd;
	m_address = addr;
	this->sendMessage = false;
	addfd(m_epollfd, sockfd, true);
	m_user_count++;
	user = new User();
	init();
}

//初始化新接受的连接
//check_state默认为分析请求行状态
void chat_conn::readInit()
{
	m_read_idx = 0;		//缓冲区中m_read_buf中数据的最后一个字节的下一个位置
	m_checked_idx = 0;	//m_read_buf读取的位置
	memset(m_read_buf, '\0', READ_BUFFER_SIZE);		//存储读取的请求报文数据
}
void chat_conn::writeInit()
{
	write_lock.lock();
	m_write_idx = 0;	//指示buffer中的长度
	memset(m_write_buf, '\0', WRITE_BUFFER_SIZE);	//存储发出的响应报文数据
	bytes_to_send = 0;		//剩余发送字节数
	bytes_have_send = 0;	//已发送字节数
	write_lock.unlock();

	jsonData = Json::Value(Json::objectValue);

}
void chat_conn::init()
{

	m_read_idx = 0;		//缓冲区中m_read_buf中数据的最后一个字节的下一个位置
	m_checked_idx = 0;	//m_read_buf读取的位置
	memset(m_read_buf, '\0', READ_BUFFER_SIZE);		//存储读取的请求报文数据


	write_lock.lock();
	m_write_idx = 0;	//指示buffer中的长度
	memset(m_write_buf, '\0', WRITE_BUFFER_SIZE);	//存储发出的响应报文数据
	bytes_to_send = 0;		//剩余发送字节数
	bytes_have_send = 0;	//已发送字节数
	write_lock.unlock();


	jsonData = Json::Value(Json::objectValue);
}

bool chat_conn::read_once()
{
	//cout << "m_read_idx:::" << m_read_idx << endl;
	if (m_read_idx >= READ_BUFFER_SIZE)
	{
		//cout << "data big error" << endl;	//记录日志
		return false;
	}
	int bytes_read = 0;

#ifdef connfdET
	while (true) {
		ssize_t n = recv(m_sockfd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);
		if (n > 0) {
			m_read_idx += n;
		}
		else if (n == 0) {	//断开连接
			return false;
		}
		else {
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				break;
			perror("recv");
			return false;

		}
	}
	//cout << "m_read_idx:::" << m_read_idx << endl;
#endif
#ifdef connfdLT
	bytes_read = recv(m_sockfd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);
	m_read_idx += bytes_read;

	if (bytes_read <= 0)
	{
		return false;
	}
#endif
	//cout << "read_once  m_read_buf: ";
	/*
	for (int i = 0; i < m_read_idx; ++i) {
		printf("%02x ", (unsigned char)m_read_buf[i]);
	}*/
	return true;
}


bool chat_conn::serverSendMessage()
{
	this->write();

	string jsonStr;
	json responseJson;
	json responseJsonArray;
	responseJson["type"] = "send_message";
	responseJson["status"] = "success";
	responseJsonArray.push_back(responseJson);
	jsonStr = responseJsonArray.dump();
	jsonStr += "\n";

	copy(jsonStr.begin(), jsonStr.begin() + jsonStr.size(), m_write_buf);
	//cout << "write_login()   m_write_buf::::: ";
	//cout << "serverSendMessage"<<m_write_buf << "+" << m_write_idx << endl;
	m_write_buf[jsonStr.size()] = '\0';
	m_write_idx = jsonStr.size();

	this->sendMessage = false;
	modfd(m_epollfd, m_sockfd, EPOLLOUT);
	return true;
}

bool chat_conn::write()
{
	

	int temp = 0;
	int newadd = 0;

	write_lock.lock();
	m_iv[0].iov_base = m_write_buf;
	m_iv[0].iov_len = m_write_idx;
	bytes_to_send = m_write_idx;
	write_lock.unlock();

	ssize_t bytes_sent = 0;
	//cout << "write..m_write_buf:::";
	//cout << m_write_buf << "+" << m_write_idx << endl;

	if (bytes_to_send == 0)
	{
		//cout << "error: send message data = 0" << endl;

		writeInit();
		return true;
	}

	write_lock.lock();
	while (bytes_sent < m_write_idx) {
		if (sendMessage) {
			//cout << "sendMessage = " << sendMessage << endl;
			//cout << "sendSocketFd = " << sendSocketFd << endl;
			temp = send(sendSocketFd, m_write_buf + bytes_sent, m_write_idx - bytes_sent, 0);
		}
		else {
			temp = send(m_sockfd, m_write_buf + bytes_sent, m_write_idx - bytes_sent, 0);
		}

		if (temp == -1) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {

				//cout << "suf 缓冲区已满，设置为可写，稍后再试" << endl;
				modfd(m_epollfd, m_sockfd, EPOLLOUT);
				return true;
			}
			else if (errno == ECONNRESET || errno == EPIPE) {
				//cout << "连接被重置或管道关闭，处理连接断开" << endl;

				perror("error: Connection reset or closed");
				return false;
			}
			else {
				//cout << "other error , exit." << endl;
				perror("send failed");
				return false;
			}
		}
		else {
			bytes_sent += temp;
		}
	}
	write_lock.unlock();
	//cout << "if success，set EPOLLIN" << endl;
	

	// 如果数据发送完毕，可以恢复监听 EPOLLIN
	writeInit();
	modfd(m_epollfd, m_sockfd, EPOLLIN);


	/*
	while (1)
	{
		temp = writev(m_sockfd, m_iv, 1);
		if (temp > 0)
		{
			//更新已发送字节
			bytes_have_send += temp;
			bytes_to_send -= temp;
			//偏移文件iovec的指针
			newadd = bytes_have_send - m_write_idx;


		}
		if (temp < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
			{
				//cout << "缓冲区满，稍后再试" << endl;
				modfd(m_epollfd, m_sockfd, EPOLLOUT);
				return true;
			}
			else if (errno == EINTR)
			{

				//cout << "写操作被信号中断，重试" << endl;

				continue;
			}
			else
			{
				//cout << "发生了其他错误，取消映射并返回失败" << endl;
				return false;
			}
		}

		if (bytes_to_send <= 0)
		{
			//cout << "发送完成" << endl;
			init();
			modfd(m_epollfd, m_sockfd, EPOLLIN);
		}

	}
	*/
	return true;
}

bool chat_conn::parse_messages()	// 一定要判断m_read_idx 和 expected_len
{
	while (true) {	// 如果沾包多条信息就要循环处理

		uint32_t expected_len = 0; // JSON 数据长度
		if (recv_state == ReadState::READ_LENGTH) {
			if (m_read_idx < head_len)
				return false;
			uint32_t net_len;
			memcpy(&net_len, m_read_buf, 4);
			expected_len = ntohl(net_len);

			
			//printf("net_len: 0x%08x, expected_len: %u\n", net_len, expected_len);
			//cout << "m_read_idx:" << m_read_idx << endl;
			if (m_read_idx < expected_len) {
				cerr << "data no enough，continue" << endl;	//记录日志
				return false;
			}
			//cout << "Buffer : " << m_read_buf << endl;  // 记录 buffer 内容
			// 删除前四个字节
			memmove(m_read_buf, m_read_buf + head_len, m_read_idx - head_len);
			//cout << " memmove Buffer : " << m_read_buf << endl;  // 记录 buffer 内容
			//cout << "m_read_idx1:" << m_read_idx << endl;
			m_read_idx -= head_len;
			//cout << "m_read_idx2:" << m_read_idx << endl;
			recv_state = ReadState::READ_BODY;

		}
		if (recv_state == ReadState::READ_BODY) {
			//cout << "m_read_idx1:" << m_read_idx << endl;
			//cout << "expected_len2:" << expected_len << endl;

			Json::CharReaderBuilder reader;
			string errs;
			istringstream ss(string(m_read_buf, expected_len));
			Json::Value jsonDataTemp = Json::Value();
			if (!Json::parseFromStream(reader, ss, &jsonDataTemp, &errs)) {
				cerr << "Failed to parse JSON: " << errs << endl;
				cerr << "Buffer content: " << m_read_buf << endl;  // 记录 buffer 内容
				return false;
			}
			//cout << "read_once():" << endl;
			jsonTaskQueue.push(jsonDataTemp);
			recv_state = ReadState::READ_LENGTH;

			// 保存数据
			if (m_read_idx == expected_len) {
				return true;
			}

			// 清除该数据
			memmove(m_read_buf, m_read_buf + expected_len, m_read_idx - expected_len);
			m_read_idx -= expected_len;	//4字节，不太清楚减多少
			//cout << "m_read_idx7:" << m_read_idx << endl;
			//cout << "expected_len8:" << expected_len << endl;
			//cout << "循环分析信息！" << endl;
		}

	}
}

//主任务操作
void chat_conn::process()
{
	if (!parse_messages()) {
		modfd(m_epollfd, m_sockfd, EPOLLIN);
		return;
	}
	readInit();
	//cout << "process ing" << endl;
	
	while (!jsonTaskQueue.empty()) {
		//cout << "jsonTaskQueue::" << jsonTaskQueue.size() << endl;
		jsonData = jsonTaskQueue.front();
		CHAT_CODE read_ret = process_read();
		//cout << "CHAT_CODE = " << read_ret << endl;
		if (read_ret == NOCODE)
		{
			modfd(m_epollfd, m_sockfd, EPOLLIN);
			return;
		}

		bool write_ret = process_write(read_ret);	//报文头写进去
		if (!write_ret)
		{
			//cout << "process_write error!" << endl;
		}
		jsonTaskQueue.pop();
	}


	modfd(m_epollfd, m_sockfd, EPOLLOUT);	//修改成写监听

	//cout << "set EPOLLOUT success" << endl;
	return;
}

chat_conn::CHAT_CODE chat_conn::process_read()
{
	//cout << "process_read():::";
	if (jsonData["type"].asString() == "login")
		return LOGIN;
	else if (jsonData["type"].asString() == "register")
		return REGISTER;
	else if (jsonData["type"].asString() == "friends")
		return FRIENDS;
	else if (jsonData["type"].asString() == "chatting")
		return CHATTING;
	else if (jsonData["type"].asString() == "send_message")
		return SEND_MESSAGE;
	else if (jsonData["type"].asString() == "receive_message")
		return RECEIVE_MESSAGE;
	else if (jsonData["type"].asString() == "update_chat")
		return UPDATE_CHAT;
	else if (jsonData["type"].asString() == "get_unread_messages")
		return GET_UNREAD_MESSAGES;
	else if (jsonData["type"].asString() == "search_user")
		return SEARCH_USER;
	else if (jsonData["type"].asString() == "add_friend")
		return ADD_FRIEND;
	else if (jsonData["type"].asString() == "accept_friend")
		return ACCEPT_FRIEND;
	else if (jsonData["type"].asString() == "delete_friend")
		return DELETE_FRIEND;
	else if (jsonData["type"].asString() == "group_text")
		return NOCODE;
	else
		return NOCODE;
}

bool chat_conn::process_write(chat_conn::CHAT_CODE ret)
{
	string jsonStr;
	//cout << "ret::" << ret << endl;
	switch (ret)
	{
	case LOGIN: {
		//cout << "LOGIN" << endl;
		if (jsonData.isMember("user_id") && jsonData.isMember("user_password")) {
			string userid = jsonData["user_id"].asString();
			string userpassword = jsonData["user_password"].asString();

			string sqlStatement = "SELECT * FROM m_user WHERE user_id = '" + userid + "' AND user_password = '" + userpassword + "';";
			jsonStr += getMysql_UserResult("login", sqlStatement);
		}
		else {
			//cout << "user_id   user_password" << endl;
			return false;
		}
			
		break;
	}
	case REGISTER: {
		if (jsonData.isMember("user_phone") && jsonData.isMember("user_name") && jsonData.isMember("user_password")) {
			string user_phone = jsonData["user_phone"].asString();
			string user_name = jsonData["user_name"].asString();
			string user_password = jsonData["user_password"].asString();

			string sqlStatement = "CALL Register(\"" + user_phone + "\", \"" + user_name + "\", \"" + user_password + "\", @new_user_id);";
			jsonStr += setMysql_UserResult("register", sqlStatement);
		}
		else
			return false;
		break;
	}
	case FRIENDS: {
		if (jsonData.isMember("user_id")) {
			string user_id = jsonData["user_id"].asString();
			//cout << "user_id::" << user_id << endl;

			string sqlStatement = "SELECT m_user.*, m_friends_relation.friend_relation, m_friends_relation.created_relation FROM m_user "
				"JOIN m_friends_relation ON m_user.user_id = m_friends_relation.friend_id "
				"WHERE m_friends_relation.user_id = '" + user_id + "'; ";
			//cout << "sqlStatement" << sqlStatement << endl;
			jsonStr += getMysql_FriendsResult("friends", sqlStatement);
			//cout << "jsonStr = " << jsonStr << endl;
		}
		else
			return false;
		break;
	}
	case CHATTING: {
		if (jsonData.isMember("user_id")) {
			string user_id = jsonData["user_id"].asString();

			string sqlStatement = "SELECT m_chatlist.*, m_user.user_name, m_user.user_avatar FROM m_user "
				"JOIN m_chatlist ON m_chatlist.comm_id = m_user.user_id "
				"WHERE m_chatlist.user_id = '" + user_id + "'; ";

			//cout << "sqlStatement" << sqlStatement << endl;
			jsonStr += getMysql_ChatsResult("chatting", sqlStatement);
			//cout << "jsonStr = " << jsonStr << endl;
		}
		else
			return false;
		break;
	}
	case SEND_MESSAGE: {
		string sqlStatement = " ";
		jsonStr += sendMessage_Mysql("send_message", sqlStatement);
		//cout << "jsonStr = " << jsonStr << endl;
		break;
	}
	case RECEIVE_MESSAGE: {
		break;
	}

	case UPDATE_CHAT: {
		if (jsonData.isMember("user_id") && jsonData.isMember("comm_id")) {

			jsonStr += updateChat_Mysql();
			//cout << "jsonStr = " << jsonStr << endl;
		}
		else
			return false;
		break;
	}
	case GET_UNREAD_MESSAGES: {
		//cout << "GET_UNREAD_MESSAGES " << endl;
		if (jsonData.isMember("user_id")) {
			//cout << "jsonData.isMember  " << endl;
			string user_id = jsonData["user_id"].asString();
			string sqlStatement = "SELECT * FROM m_friends_message WHERE(receiver_id = '" + user_id + "' AND message_status = 'received'); ";
			//cout << "sqlStatement" << sqlStatement << endl;
			jsonStr += getFriendsUnreadMessages_Mysql("get_unread_messages", sqlStatement);
			//cout << "jsonStr = " << jsonStr << endl;
		}
		else
			return false;
		break;
	}
	case SEARCH_USER: {
		if (jsonData.isMember("user_id")) {
			string user_id = jsonData["user_id"].asString();
			string search_id = jsonData["search_id"].asString();
			string sqlStatement = "SELECT * FROM m_user WHERE user_id = '" + search_id + "';";
			//cout << "sqlStatement" << sqlStatement << endl;
			jsonStr += searchUserInformation_Mysql("search_user", sqlStatement);
			//cout << "jsonStr = " << jsonStr << endl;
		}
		else
			return false;
		break;
	}
	case ADD_FRIEND: {
		//cout << "ADD_FRIEND " << endl;
		if (jsonData.isMember("user_id")) {
			string user_id = jsonData["user_id"].asString();
			string friend_id = jsonData["friend_id"].asString();
			string sqlStatement = "CALL AddFriend ('" + user_id + "', '" + friend_id + "');";
			//cout << "sqlStatement" << sqlStatement << endl;
			jsonStr += modifyFriend_Mysql("get_unread_messages", sqlStatement);
			//cout << "jsonStr = " << jsonStr << endl;
		}
		else
			return false;
		break;
	}
	case ACCEPT_FRIEND: {
		//cout << "ACCEPT_FRIEND " << endl;
		if (jsonData.isMember("user_id")) {
			string user_id = jsonData["user_id"].asString();
			string friend_id = jsonData["friend_id"].asString();
			string sqlStatement = "CALL UpdateFriend ('" + user_id + "', '" + friend_id + "' , 'accepted');";
			//cout << "sqlStatement" << sqlStatement << endl;
			jsonStr += modifyFriend_Mysql("accept_friend", sqlStatement);
			//cout << "jsonStr = " << jsonStr << endl;
		}
		else
			return false;
		break;
	}
	case DELETE_FRIEND: {
		//cout << "DELETE_FRIEND " << endl;
		if (jsonData.isMember("user_id")) {
			string user_id = jsonData["user_id"].asString();
			string friend_id = jsonData["friend_id"].asString();
			string sqlStatement = "CALL DeleteFriend ('" + user_id + "', '" + friend_id + "');";
			//cout << "sqlStatement" << sqlStatement << endl;
			jsonStr += modifyFriend_Mysql("delete_friend", sqlStatement);
			//cout << "jsonStr = " << jsonStr << endl;
		}
		else
			return false;
		break;
	}
	default:
		return false;
	}

	if (jsonStr.size() > WRITE_BUFFER_SIZE - 1) {
		cerr << "Data exceeds buffer size!" << endl;
		return false;
	}
	if (this->sendMessage) {
		ChatMapping::getInstance().getUsers()[sendSocketFd].write_lock.lock();
		copy(jsonStr.begin(), jsonStr.begin() + jsonStr.size(), ChatMapping::getInstance().getUsers()[sendSocketFd].m_write_buf);
		ChatMapping::getInstance().getUsers()[sendSocketFd].m_write_buf[jsonStr.size()] = '\0';
		ChatMapping::getInstance().getUsers()[sendSocketFd].m_write_idx = jsonStr.size();
		ChatMapping::getInstance().getUsers()[sendSocketFd].write_lock.unlock();

		modfd(m_epollfd, sendSocketFd, EPOLLOUT);	//修改成写监听

	}
	else {
		write_lock.lock();
		uint32_t jsonLen = jsonStr.size();
		uint32_t netLen = htonl(jsonLen);
		memcpy(m_write_buf, &netLen, 4);

		copy(jsonStr.begin(), jsonStr.begin() + jsonStr.size(), m_write_buf + 4);
		m_write_buf[jsonStr.size()] = '\0';
		m_write_idx = 4 + jsonLen;
		write_lock.unlock();
	}

	return true;
}

void chat_conn::wirteWriteBuf(string str)
{
	
}

//string chat_conn::getMysqlResult(const string &type, const string &sqlStatement)

string chat_conn::getMysql_UserResult(const string& type, const string& sqlStatement)
{
	mysql = NULL;
	connectionRAII mysqlcon(&mysql, connPool);
	if (mysql_query(mysql, sqlStatement.c_str())) {
		LOG_ERROR("SELECT error:%s\n", mysql_error(mysql));
		return "error";
	}
	MYSQL_RES* result = mysql_store_result(mysql);	//取出结果集
	if (!result) {
		//cout << "Failed to store result" << endl;
		return "error";
	}
	int num_fields = mysql_num_fields(result);
	//cout << "num_fields:::" << num_fields << endl;
	MYSQL_FIELD* fields = mysql_fetch_fields(result);//返回所有字段结构的数组（这一步通常不影响程序流程，除非你需要字段信息）

	string responseJsonStr;
	json responseJson;
	json responseJsonArray;
	//cout << "responseJsonArray:::" << endl;
	if (mysql_num_rows(result) == 0) {
		//cout << "No data rows in the result set" << endl;
		responseJson["type"] = type;
		responseJson["status"] = "error";
		responseJson["message"] = "name or code error";
		responseJsonArray.push_back(responseJson);
	}
	else {
		//cout << "mysql_num_rows != 0" << endl;
		while (MYSQL_ROW row = mysql_fetch_row(result)) {
			//cout << "row             aaa" << endl;
			if (row == NULL) {
				//cout << "Failed to fetch row" << endl;
				continue;  // 继续处理下一行
			}
			responseJson["type"] = type;
			responseJson["status"] = "success";
			responseJson["user_id"] = row[0];
			responseJson["user_password"] = row[1];
			responseJson["user_phone"] = row[2];
			responseJson["user_name"] = row[3];
			responseJson["user_email"] = row[4];
			responseJson["user_avatar"] = row[5];
			responseJson["user_status"] = row[6];
			responseJson["user_created"] = row[7];
			ChatMapping::getInstance().addUserSocket(row[0], m_sockfd);
			//cout << "userID = " << row[0] << endl;
			//cout << "m_sockfd = " << m_sockfd << endl;
			this->user->setUserId(row[0]);
			this->user->setUserPassword(row[1]);
			this->user->setUserPhone(row[2]);
			this->user->setUserName(row[3]);
			this->user->setUserEmail(row[4]);
			this->user->setUserAvatar(row[5]);
			this->user->setUserStatus(row[6]);
			this->user->setUserCreated(row[7]);
			responseJsonArray.push_back(responseJson);
			//string changeStatus = "UPDATE m_user SET user_status = 'active' WHERE user_id = '" + this->user->getUserId() + "';";
			//mysql_query(mysql, changeStatus.c_str());
			this->user->setUserId("active");
		}
	}
	responseJsonStr = responseJsonArray.dump(); responseJsonStr += "\n";
	//cout << "responseJsonStr" << responseJsonStr << endl;
	return responseJsonStr;
}

string chat_conn::setMysql_UserResult(const string& type, const string& sqlStatement)
{
	mysql = NULL;
	connectionRAII mysqlcon(&mysql, connPool);
	if (mysql_query(mysql, sqlStatement.c_str())) {
		//cout << "SELECT error:%s\n" << mysql_error(mysql) << endl;
		return "error";
	}
	if (mysql_query(mysql, "SELECT @new_user_id;")) {
		//cout << "mysql_query() failed: " << mysql_error(mysql) << endl;
		return "error";
	}

	string responseJsonStr;
	json responseJson;
	json responseJsonArray;

	MYSQL_RES* res = mysql_store_result(mysql);
	if (res == NULL) {
		//cout << "mysql_store_result() failed: " << mysql_error(mysql) << endl;
	}
	else {
		MYSQL_ROW row = mysql_fetch_row(res);
		if (row) {
			//cout << "New user ID: " << row[0] << endl;
			responseJson["type"] = type;
			responseJson["status"] = "success";
			responseJson["user_id"] = row[0];
		}
		else {
			//cout << "No result returned." << endl;
		}
	}

	responseJsonArray.push_back(responseJson);
	responseJsonStr = responseJsonArray.dump(); responseJsonStr += "\n";
	return responseJsonStr;
}

string chat_conn::getMysql_FriendsResult(const string& type, const string& sqlStatement)
{
	mysql = NULL;
	connectionRAII mysqlcon(&mysql, connPool);

	if (mysql_query(mysql, sqlStatement.c_str())) {
		LOG_ERROR("SELECT error:%s\n", mysql_error(mysql));
		return "SELECT error";
	}
	MYSQL_RES* result = mysql_store_result(mysql);	//取出结果集
	if (!result) {
		//cout << "Failed to store result" << endl;
		return "error";
	}
	int num_fields = mysql_num_fields(result);
	//cout << "num_fields:::" << num_fields << endl;
	MYSQL_FIELD* fields = mysql_fetch_fields(result);//返回所有字段结构的数组（这一步通常不影响程序流程，除非你需要字段信息）

	string responseJsonStr;
	json responseJson;
	json responseJsonArray;

	if (mysql_num_rows(result) == 0) {
		//cout << "No data rows in the result set" << endl;
		responseJson["type"] = type;
		responseJson["status"] = "null";
		responseJson["message"] = "have not friends";
		responseJsonArray.push_back(responseJson);
	}
	else {
		while (MYSQL_ROW row = mysql_fetch_row(result)) {
			if (row == NULL) {
				//cout << "Failed to fetch row" << endl;
				continue;  // 继续处理下一行
			}
			responseJson["type"] = type;
			responseJson["status"] = "success";
			responseJson["user_id"] = row[0];
			responseJson["user_phone"] = row[2];
			responseJson["user_name"] = row[3];
			responseJson["user_email"] = row[4];
			responseJson["user_avatar"] = row[5];
			responseJson["user_status"] = row[6];
			responseJson["user_created"] = row[7];
			responseJson["friend_relation"] = row[8];
			responseJson["created_relation"] = row[9];

			responseJsonArray.push_back(responseJson);
		}
	}

	responseJsonStr = responseJsonArray.dump(); responseJsonStr += "\n";
	return responseJsonStr;

}

string chat_conn::getMysql_ChatsResult(const string& type, const string& sqlStatement)
{
	mysql = NULL;
	connectionRAII mysqlcon(&mysql, connPool);

	if (mysql_query(mysql, sqlStatement.c_str())) {
		LOG_ERROR("SELECT error:%s\n", mysql_error(mysql));
		return "SELECT error";
	}
	MYSQL_RES* result = mysql_store_result(mysql);	//取出结果集
	if (!result) {
		//cout << "Failed to store result" << endl;
		return "error";
	}
	int num_fields = mysql_num_fields(result);
	MYSQL_FIELD* fields = mysql_fetch_fields(result);//返回所有字段结构的数组（这一步通常不影响程序流程，除非你需要字段信息）

	string responseJsonStr;
	json responseJson;
	json responseJsonArray;

	if (mysql_num_rows(result) == 0) {
		//cout << "No data rows in the result set" << endl;
		responseJson["type"] = type;
		responseJson["status"] = "null";
		responseJson["message"] = "have not chatting";
		responseJsonArray.push_back(responseJson);
	}
	else {
		while (MYSQL_ROW row = mysql_fetch_row(result)) {
			if (row == NULL) {
				//cout << "Failed to fetch row" << endl;
				continue;  // 继续处理下一行
			}
			responseJson["type"] = type;
			responseJson["status"] = "success";
			responseJson["comm_id"] = row[1];
			responseJson["chat_type"] = row[2];
			responseJson["last_message"] = row[3];
			responseJson["last_message_type"] = row[4];
			responseJson["last_time"] = row[5];
			responseJson["unread_count"] = row[6];
			responseJson["chat_status"] = row[7];
			//cout << "row[7]" << row[7] << endl;
			responseJson["user_name"] = row[8];
			responseJson["user_avatar"] = row[9];


			responseJsonArray.push_back(responseJson);
		}
	}
	responseJsonStr = responseJsonArray.dump(); responseJsonStr += "\n";
	return responseJsonStr;

}

string chat_conn::sendMessage_Mysql(const string& type, string& sqlStatement)
{

	string send_id = jsonData["send_id"].asString();
	string receiver_id = jsonData["receiver_id"].asString();
	string chat_type = jsonData["chat_type"].asString();
	string message_body = jsonData["message_body"].asString();
	string message_type = jsonData["message_type"].asString();
	string file_path = jsonData["file_path"].asString();
	string message_timestamp = jsonData["message_timestamp"].asString();
	sqlStatement = "CALL AddFriendMessage('" + send_id + "', '" + receiver_id + "', '" + chat_type + "', '"
		+ message_body + "', '" + message_type + "', '" + file_path + "', 'send', '" + message_timestamp + "', @new_message_id);";
	//cout << "sqlStatement" << sqlStatement << endl;

	mysql = NULL;
	connectionRAII mysqlcon(&mysql, connPool);

	string responseJsonStr;
	json responseJson;
	json responseJsonArray;

	if (mysql_query(mysql, sqlStatement.c_str())) {
		printf("SELECT error:%s\n", mysql_error(mysql));
		//cout << "sqlStatement error" << endl;
		responseJson["type"] = type;
		responseJson["status"] = "error";
		responseJson["message"] = "send error";

		responseJsonArray.push_back(responseJson);
		responseJsonStr = responseJsonArray.dump(); responseJsonStr += "\n";
		return responseJsonStr;
	}

	string friends_message_id;
	if (mysql_query(mysql, "SELECT @new_message_id;")) {
		//cout << "mysql_query() failed: " << mysql_error(mysql) << endl;
		return "error";
	}
	MYSQL_RES* res = mysql_store_result(mysql);
	if (res == NULL) {
		//cout << "mysql_store_result() failed: " << mysql_error(mysql) << endl;
		return "无 friends_message_id";
	}
	else {
		MYSQL_ROW row = mysql_fetch_row(res);
		if (row) {
			//cout << "friends_message_id: " << row[0] << endl;
			friends_message_id = row[0];
		}
		else {
			//cout << "No result returned." << endl;
		}
	}
	//cout << "receiver_id = " << receiver_id << endl;
	int socketFd = ChatMapping::getInstance().getSocketByUserId(receiver_id);
	//cout << "sockFd = " << socketFd << endl;
	if (socketFd == -1) {
		responseJson["type"] = type;
		responseJson["status"] = "success";
		responseJsonArray.push_back(responseJson);
		responseJsonStr = responseJsonArray.dump(); responseJsonStr += "\n";
		return responseJsonStr;
	}
	else {
		//cout << "sendMessage = true" << endl;
		this->sendMessage = true;
		sendSocketFd = socketFd;
		responseJson["type"] = "receive_message";
		responseJson["status"] = "success";
		responseJson["message_id"] = friends_message_id;
		responseJson["send_id"] = send_id;
		responseJson["receiver_id"] = receiver_id;
		responseJson["chat_type"] = chat_type;
		responseJson["message_body"] = message_body;
		responseJson["message_type"] = message_type;
		responseJson["file_path"] = file_path;
		responseJson["message_timestamp"] = message_timestamp;
	}

	responseJsonArray.push_back(responseJson);
	sqlStatement = "UPDATE m_friends_message SET message_status = 'received' WHERE m_friends_message.message_id = '" + friends_message_id + "' ;";
	//cout << "sqlStatement" << sqlStatement << endl;
	if (mysql_query(mysql, sqlStatement.c_str())) {
		//cout << "mysql error:%s\n" << endl;
		return "mysql error";
	}
	sqlStatement = "UPDATE m_friends_message SET message_status = 'read' WHERE(m_friends_message.sender_id = '" + receiver_id + "' AND m_friends_message.receiver_id = '" + send_id + "'); ";
	//cout << "sqlStatement" << sqlStatement << endl;
	if (mysql_query(mysql, sqlStatement.c_str())) {
		//cout << "mysql error:%s\n" << endl;
		return "mysql error";
	}
	responseJsonStr = responseJsonArray.dump(); responseJsonStr += "\n";
	return responseJsonStr;
}

string chat_conn::updateChat_Mysql()
{
	string type = jsonData["type"].asString();
	string user_id = jsonData["user_id"].asString();
	string comm_id = jsonData["comm_id"].asString();
	string chat_type = jsonData["chat_type"].asString();
	string last_message = jsonData["last_message"].asString();
	string last_message_type = jsonData["last_message_type"].asString();
	string last_time = jsonData["last_time"].asString();
	string unread_count = jsonData["unread_count"].asString();
	string chat_status = jsonData["chat_status"].asString();

	string sqlStatement = "UPDATE m_chatlist SET last_message ='" + last_message + "', last_message_type = '"
		+ last_message_type + "', last_time = '" + last_time + "', unread_count = '" + unread_count + "', chat_status = '" + chat_status + "' "
		"WHERE(m_chatlist.user_id = '" + user_id + "' AND m_chatlist.comm_id = '" + comm_id + "'); ";

	//cout << "sqlStatement" << sqlStatement << endl;

	mysql = NULL;
	connectionRAII mysqlcon(&mysql, connPool);

	string responseJsonStr;
	json responseJson;
	json responseJsonArray;

	if (mysql_query(mysql, sqlStatement.c_str())) {
		LOG_ERROR("mysql error:%s\n", mysql_error(mysql));
		return "mysql error";
	}
	unsigned long affectedRows = mysql_affected_rows(mysql);

	if (affectedRows > 0) {
		responseJson["type"] = type;
		responseJson["status"] = "success";
	}
	else {
		responseJson["type"] = type;
		responseJson["status"] = "null";
		responseJson["message"] = "error";
		//cout << "No rows updated or query failed." << endl;
	}

	sqlStatement = "UPDATE m_friends_message SET message_status = 'read' WHERE(m_friends_message.sender_id = '" + comm_id + "' AND m_friends_message.receiver_id = '" + user_id + "'); ";
	//cout << "sqlStatement" << sqlStatement << endl;
	if (mysql_query(mysql, sqlStatement.c_str())) {
		//cout << "mysql error:%s\n" << endl;
		return "mysql error";
	}

	responseJsonArray.push_back(responseJson);

	responseJsonStr = responseJsonArray.dump(); responseJsonStr += "\n";
	return responseJsonStr;
}

string chat_conn::getFriendsUnreadMessages_Mysql(const string& type, const string& sqlStatement)
{
	mysql = NULL;
	connectionRAII mysqlcon(&mysql, connPool);
	if (mysql_query(mysql, sqlStatement.c_str())) {
		LOG_ERROR("SELECT error:%s\n", mysql_error(mysql));
		return "SELECT error";
	}
	MYSQL_RES* result = mysql_store_result(mysql);	//取出结果集
	if (!result) {
		//cout << "Failed to store result" << endl;
		return "error";
	}
	int num_fields = mysql_num_fields(result);
	//cout << "num_fields:::" << num_fields << endl;
	MYSQL_FIELD* fields = mysql_fetch_fields(result);//返回所有字段结构的数组（这一步通常不影响程序流程，除非你需要字段信息）

	string responseJsonStr;
	json responseJson;
	json responseJsonArray;
	//cout << "mysql_num_rows(result) = " << mysql_num_rows(result) << endl;
	if (mysql_num_rows(result) == 0) {

		//cout << "No data rows in the result set" << endl;
		responseJson["type"] = type;
		responseJson["status"] = "null";
		responseJson["message"] = "don't have messages";

		responseJsonArray.push_back(responseJson);
	}
	else {
		while (MYSQL_ROW row = mysql_fetch_row(result)) {
			if (row == NULL) {
				//cout << "Failed to fetch row" << endl;
				continue;  // 继续处理下一行
			}
			responseJson["type"] = type;
			responseJson["status"] = "success";
			responseJson["send_id"] = row[1];
			responseJson["receiver_id"] = row[2];
			responseJson["chat_type"] = row[3];
			responseJson["message_body"] = row[4];
			responseJson["message_type"] = row[5];
			responseJson["file_path"] = row[6];
			responseJson["message_timestamp"] = row[8];

			responseJsonArray.push_back(responseJson);
		}
	}
	responseJsonStr = responseJsonArray.dump(); responseJsonStr += "\n";
	return responseJsonStr;

}

string chat_conn::searchUserInformation_Mysql(const string& type, const string& sqlStatement)
{
	mysql = NULL;
	connectionRAII mysqlcon(&mysql, connPool);
	if (mysql_query(mysql, sqlStatement.c_str())) {
		LOG_ERROR("SELECT error:%s\n", mysql_error(mysql));
		return "SELECT error";
	}
	MYSQL_RES* result = mysql_store_result(mysql);	//取出结果集
	if (!result) {
		//cout << "Failed to store result" << endl;
		return "error";
	}
	int num_fields = mysql_num_fields(result);

	string responseJsonStr;
	json responseJson;
	json responseJsonArray;
	//cout << "mysql_num_rows(result) = " << mysql_num_rows(result) << endl;
	if (mysql_num_rows(result) == 0) {

		//cout << "No data rows in the result set" << endl;
		responseJson["type"] = type;
		responseJson["status"] = "null";
		responseJson["message"] = "don't have messages";

		responseJsonArray.push_back(responseJson);
	}
	else {
		while (MYSQL_ROW row = mysql_fetch_row(result)) {
			if (row == NULL) {
				//cout << "Failed to fetch row" << endl;
				continue;  // 继续处理下一行
			}
			responseJson["type"] = type;
			responseJson["status"] = "success";
			responseJson["user_id"] = row[0];
			responseJson["user_password"] = row[1];
			responseJson["user_phone"] = row[2];
			responseJson["user_name"] = row[3];
			responseJson["user_email"] = row[4];
			responseJson["user_avatar"] = row[5];
			responseJson["user_status"] = row[6];
			responseJson["user_created"] = row[7];

			responseJsonArray.push_back(responseJson);
		}
	}

	responseJsonStr = responseJsonArray.dump(); responseJsonStr += "\n";
	return responseJsonStr;
}

string chat_conn::modifyFriend_Mysql(const string& type, const string& sqlStatement)
{
	mysql = NULL;
	connectionRAII mysqlcon(&mysql, connPool);

	string responseJsonStr;
	json responseJson;
	json responseJsonArray;

	if (mysql_query(mysql, sqlStatement.c_str())) {
		LOG_ERROR("mysql error:%s\n", mysql_error(mysql));
		return "mysql error";
	}
	unsigned long affectedRows = mysql_affected_rows(mysql);

	if (affectedRows > 0) {
		string user_id = jsonData["user_id"].asString();
		string friend_id = jsonData["friend_id"].asString();
		responseJson["type"] = type;
		responseJson["status"] = "success";
		responseJson["user_id"] = user_id;
		responseJson["friend_id"] = friend_id;
	}
	else {
		responseJson["type"] = type;
		responseJson["status"] = "null";
		responseJson["message"] = "error";
		//cout << "No rows updated or query failed." << endl;
	}

	responseJsonArray.push_back(responseJson);

	responseJsonStr = responseJsonArray.dump(); responseJsonStr += "\n";
	return responseJsonStr;
}

void chat_conn::removeUser()
{
	ChatMapping::getInstance().removeUserSocket(this->user->getUserId());
	epoll_ctl(m_epollfd, EPOLL_CTL_DEL, m_sockfd, 0);
	close(m_sockfd);
}