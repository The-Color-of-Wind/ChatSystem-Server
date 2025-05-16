#include"chat_conn.h"

#include <dirent.h>
#include <ctype.h>

#include<map>
#include <mysql/mysql.h>
#include <fstream>

#include<iostream>
using namespace std;

#define connfdET	//��Ե����������
//#define connfdLT	//ˮƽ��������

//#define listenfdET //��Ե����������
#define listenfdLT //ˮƽ��������

//���ļ����������÷�����
int setnonblocking(int fd)
{
	int old_option = fcntl(fd, F_GETFL);
	int new_option = old_option | O_NONBLOCK;
	fcntl(fd, F_SETFL, new_option);

	return old_option;
}

//��epollʵ����ɾ���¼�
void removefd(int epollfd, int fd)
{
	epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
	close(fd);
}


//�޸��¼�
void modfd(int epollfd, int fd, int ev, void* ptr)
{

	epoll_event event;
	event.data.ptr = ptr;


#ifdef connfdET
	event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
#endif

#ifdef connfdLT
	event.events = ev | EPOLLONESHOT | EPOLLRDHUP;
#endif

	epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
	return;
}


//��ʼ���½��ܵ�����
//check_stateĬ��Ϊ����������״̬
void chat_conn::readInit()
{
	m_read_idx = 0;		//��������m_read_buf�����ݵ����һ���ֽڵ���һ��λ��
	m_checked_idx = 0;	//m_read_buf��ȡ��λ��
	memset(m_read_buf, '\0', READ_BUFFER_SIZE);		//�洢��ȡ������������
}
void chat_conn::writeInit()
{
	write_lock.lock();
	m_write_idx = 0;	//ָʾbuffer�еĳ���
	memset(m_write_buf, '\0', WRITE_BUFFER_SIZE);	//�洢��������Ӧ��������
	bytes_to_send = 0;		//ʣ�෢���ֽ���
	bytes_have_send = 0;	//�ѷ����ֽ���
	write_lock.unlock();
	this->sendMessage = false;

}
void chat_conn::init()
{

	m_read_idx = 0;		//��������m_read_buf�����ݵ����һ���ֽڵ���һ��λ��
	m_checked_idx = 0;	//m_read_buf��ȡ��λ��
	memset(m_read_buf, '\0', READ_BUFFER_SIZE);		//�洢��ȡ������������


	write_lock.lock();
	m_write_idx = 0;	//ָʾbuffer�еĳ���
	memset(m_write_buf, '\0', WRITE_BUFFER_SIZE);	//�洢��������Ӧ��������
	bytes_to_send = 0;		//ʣ�෢���ֽ���
	bytes_have_send = 0;	//�ѷ����ֽ���
	write_lock.unlock();
}


bool chat_conn::parse_messages()
{
	//cout << "parse_messages action " << endl;
	while (m_read_idx > 0) {
		////cout << "m_read_idx "<< m_read_idx << endl;
		if (m_read_idx < head_len)
			return false;

		uint32_t net_len;
		memcpy(&net_len, m_read_buf, 4);
		uint32_t expected_len = 0;	// ����ͷ
		expected_len = ntohl(net_len);
		////cout << "m_read_idx " << m_read_idx << endl;
		if (m_read_idx < expected_len) {
			cerr << "data no enough��continue" << endl;
			return false;	// ���ݲ������������ȴ�
		}
		////cout << "m_read_buf " << m_read_buf << endl;
		// ɾ��ǰ�ĸ��ֽ�
		memmove(m_read_buf, m_read_buf + head_len, m_read_idx - head_len);
		m_read_idx -= head_len;

		string msg = string(m_read_buf, expected_len);
		push_message(msg, *(this->pool));
		////cout << "push_message " << endl;

		
		// ���������ȫ��ȣ��򷵻�true
		if (m_read_idx == expected_len) {
			return true;
		}

		// ��������Ѷ����ݣ�����ѭ����ȡ
		memmove(m_read_buf, m_read_buf + expected_len, m_read_idx - expected_len);
		m_read_idx -= expected_len;

	}
	return true;

}

bool chat_conn::receive()
{
	if (m_read_idx >= READ_BUFFER_SIZE)
	{
		return false;
	}
	int bytes_read = 0;

	while (true) {
		//cout << "receive recv action " << endl;
		ssize_t n = recv(m_sockfd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);
		////cout << "n" << n<< endl;
		if (n > 0) {
			m_read_idx += n;
		}
		else if (n == 0) {	//�Ͽ�����
			return false;
		}
		else {
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				break;
			perror("recv");
			return false;

		}
	}
	if (parse_messages()) {
		m_read_idx = 0;
		m_checked_idx = 0;
		memset(m_read_buf, '\0', READ_BUFFER_SIZE);
	}
	//////cout << "m_read_idx:::" << m_read_idx << endl;

#ifdef connfdLT
	ssize_t n = recv(m_sockfd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);
	m_read_idx += n;

	if (n <= 0)
	{
		return false;
	}
#endif

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
	//////cout << "write..m_write_buf:::";
	//////cout << m_write_buf << "+" << m_write_idx << endl;

	if (bytes_to_send == 0)
	{
		//////cout << "error: send message data = 0" << endl;

		writeInit();
		return true;
	}

	write_lock.lock();
	while (bytes_sent < m_write_idx) {

		temp = send(m_sockfd, m_write_buf + bytes_sent, m_write_idx - bytes_sent, 0);

		if (temp == -1) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {

				//////cout << "suf ����������������Ϊ��д���Ժ�����" << endl;
				modfd(m_epollfd, m_sockfd, EPOLLOUT, this);
				return true;
			}
			else if (errno == ECONNRESET || errno == EPIPE) {
				//////cout << "���ӱ����û�ܵ��رգ��������ӶϿ�" << endl;

				perror("error: Connection reset or closed");
				return false;
			}
			else {
				//////cout << "other error , exit." << endl;
				perror("send failed");
				return false;
			}
		}
		else {
			bytes_sent += temp;
		}
	}
	write_lock.unlock();
	//////cout << "if success��set EPOLLIN" << endl;


	// ������ݷ�����ϣ����Իָ����� EPOLLIN
	writeInit();
	modfd(m_epollfd, m_sockfd, EPOLLIN, this);


	/*
	while (1)
	{
		temp = writev(m_sockfd, m_iv, 1);
		if (temp > 0)
		{
			//�����ѷ����ֽ�
			bytes_have_send += temp;
			bytes_to_send -= temp;
			//ƫ���ļ�iovec��ָ��
			newadd = bytes_have_send - m_write_idx;


		}
		if (temp < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
			{
				//////cout << "�����������Ժ�����" << endl;
				modfd(m_epollfd, m_sockfd, EPOLLOUT);
				return true;
			}
			else if (errno == EINTR)
			{

				//////cout << "д�������ź��жϣ�����" << endl;

				continue;
			}
			else
			{
				//////cout << "��������������ȡ��ӳ�䲢����ʧ��" << endl;
				return false;
			}
		}

		if (bytes_to_send <= 0)
		{
			//////cout << "�������" << endl;
			init();
			modfd(m_epollfd, m_sockfd, EPOLLIN);
		}

	}
	*/
	return true;
}


void chat_conn::push_message(const std::string& msg, ThreadPool<chat_conn>& pool) {
	{
		//cout << "push_message action" << endl;
		std::lock_guard<std::mutex> lock(mtx);
		msg_queue.push(msg);
		if (is_processing) return;  // ��ǰ���������ڴ���ֱ�ӷ���
		is_processing = true;       // ���ô����־λ
	}

	submit_next(pool);  // �ύ��������
}

void chat_conn::submit_next(ThreadPool<chat_conn>& pool) {
	//cout << "submit_next action" << endl;
	//auto self = shared_from_this();  // ���� this ��ǰ����
	////cout << "submit_next self" << endl;
	chat_conn* self = this;
	pool.submit([self, &pool]() {
		std::string msg;

		{
			std::lock_guard<std::mutex> lock(self->mtx);
			if (self->msg_queue.empty()) {
				self->is_processing = false;  // ����Ϣ���������״̬
				return;
			}
			msg = std::move(self->msg_queue.front());
			self->msg_queue.pop();
		}

		self->handle_message(msg);       // ������Ϣ
		self->submit_next(pool);         // ����������һ��
		});
}
//���������
void chat_conn::handle_message(const string& msg)
{
	// ��ȡ��ǰ����
	Json::CharReaderBuilder reader;
	string errs;
	istringstream ss(msg);
	Json::Value jsonData = Json::Value();
	if (!Json::parseFromStream(reader, ss, &jsonData, &errs)) {
		cerr << "Failed to parse JSON: " << errs << endl;
		cerr << "Buffer content: " << m_read_buf << endl;  // ��¼ buffer ����
		return;
	}

	CHAT_CODE read_ret = process_read(jsonData);

	if (read_ret == NOCODE)
	{
		modfd(m_epollfd, m_sockfd, EPOLLIN, this);
		return;
	}

	bool write_ret = process_write(read_ret, jsonData);	//����ͷд��ȥ
	if (!write_ret)
	{
		//////cout << "process_write error!" << endl;
	}


	modfd(m_epollfd, m_sockfd, EPOLLOUT, this);	//�޸ĳ�д����

	return;
}

chat_conn::CHAT_CODE chat_conn::process_read(Json::Value jsonData)
{
	//////cout << "process_read():::";
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
bool chat_conn::process_write(chat_conn::CHAT_CODE ret, Json::Value jsonData)
{
	string jsonStr;
	int sendFd = m_sockfd;
	//////cout << "ret::" << ret << endl;
	switch (ret)
	{
	case LOGIN: {
		//////cout << "LOGIN" << endl;
		if (jsonData.isMember("user_id") && jsonData.isMember("user_password")) {
			string userid = jsonData["user_id"].asString();
			string userpassword = jsonData["user_password"].asString();

			string sqlStatement = "SELECT * FROM m_user WHERE user_id = '" + userid + "' AND user_password = '" + userpassword + "';";
			jsonStr += getMysql_UserResult("login", sqlStatement, jsonData);
		}
		else {
			////cout << "user_id   user_password" << endl;
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
			jsonStr += setMysql_UserResult("register", sqlStatement, jsonData);
		}
		else
			return false;
		break;
	}
	case FRIENDS: {
		if (jsonData.isMember("user_id")) {
			string user_id = jsonData["user_id"].asString();
			//////cout << "user_id::" << user_id << endl;

			string sqlStatement = "SELECT m_user.*, m_friends_relation.friend_relation, m_friends_relation.created_relation FROM m_user "
				"JOIN m_friends_relation ON m_user.user_id = m_friends_relation.friend_id "
				"WHERE m_friends_relation.user_id = '" + user_id + "'; ";
			//////cout << "sqlStatement" << sqlStatement << endl;
			jsonStr += getMysql_FriendsResult("friends", sqlStatement, jsonData);
			//////cout << "jsonStr = " << jsonStr << endl;
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

			//////cout << "sqlStatement" << sqlStatement << endl;
			jsonStr += getMysql_ChatsResult("chatting", sqlStatement, jsonData);
			//////cout << "jsonStr = " << jsonStr << endl;
		}
		else
			return false;
		break;
	}
	case SEND_MESSAGE: {
		string sqlStatement;
		jsonStr += sendMessage_Mysql("send_message", sqlStatement, jsonData);
		/*
		string send_id = jsonData["send_id"].asString();
		string receiver_id = jsonData["receiver_id"].asString();
		string chat_type = jsonData["chat_type"].asString();
		string message_body = jsonData["message_body"].asString();
		string message_type = jsonData["message_type"].asString();
		string file_path = jsonData["file_path"].asString();
		string message_timestamp = jsonData["message_timestamp"].asString();
		sqlStatement = "CALL AddFriendMessage('" + send_id + "', '" + receiver_id + "', '" + chat_type + "', '"
			+ message_body + "', '" + message_type + "', '" + file_path + "', 'send', '" + message_timestamp + "', @new_message_id);";
		// �������ݿ�Ĳ�����
		*/
		break;
	}
	case RECEIVE_MESSAGE: {
		/*
		if (jsonData.isMember("socket")) {
			string socketFd = jsonData["socketFd"].asString();
			jsonStr = jsonData["message"].asString();
			//////cout << "jsonStr = " << jsonStr << endl;

			conn->write_lock.lock();
			copy(responseJsonStr.begin(), responseJsonStr.begin() + responseJsonStr.size(), conn->m_write_buf);
			conn->m_write_buf[jsonStr.size()] = '\0';
			conn->m_write_idx += jsonStr.size();
			conn->write_lock.unlock();

			modfd(conn->m_epollfd, conn->m_sockfd, EPOLLOUT);	//�޸ĳ�д����

		}
		*/
		return true;
		break;
	}

	case UPDATE_CHAT: {
		if (jsonData.isMember("user_id") && jsonData.isMember("comm_id")) {

			jsonStr += updateChat_Mysql(jsonData);
			//////cout << "jsonStr = " << jsonStr << endl;
		}
		else
			return false;
		break;
	}
	case GET_UNREAD_MESSAGES: {
		//////cout << "GET_UNREAD_MESSAGES " << endl;
		if (jsonData.isMember("user_id")) {
			//////cout << "jsonData.isMember  " << endl;
			string user_id = jsonData["user_id"].asString();
			string sqlStatement = "SELECT * FROM m_friends_message WHERE(receiver_id = '" + user_id + "' AND message_status = 'received'); ";
			//////cout << "sqlStatement" << sqlStatement << endl;
			jsonStr += getFriendsUnreadMessages_Mysql("get_unread_messages", sqlStatement, jsonData);
			//////cout << "jsonStr = " << jsonStr << endl;
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
			//////cout << "sqlStatement" << sqlStatement << endl;
			jsonStr += searchUserInformation_Mysql("search_user", sqlStatement, jsonData);
			//////cout << "jsonStr = " << jsonStr << endl;
		}
		else
			return false;
		break;
	}
	case ADD_FRIEND: {
		//////cout << "ADD_FRIEND " << endl;
		if (jsonData.isMember("user_id")) {
			string user_id = jsonData["user_id"].asString();
			string friend_id = jsonData["friend_id"].asString();
			string sqlStatement = "CALL AddFriend ('" + user_id + "', '" + friend_id + "');";
			//////cout << "sqlStatement" << sqlStatement << endl;
			jsonStr += modifyFriend_Mysql("get_unread_messages", sqlStatement, jsonData);
			//////cout << "jsonStr = " << jsonStr << endl;
		}
		else
			return false;
		break;
	}
	case ACCEPT_FRIEND: {
		//////cout << "ACCEPT_FRIEND " << endl;
		if (jsonData.isMember("user_id")) {
			string user_id = jsonData["user_id"].asString();
			string friend_id = jsonData["friend_id"].asString();
			string sqlStatement = "CALL UpdateFriend ('" + user_id + "', '" + friend_id + "' , 'accepted');";
			//////cout << "sqlStatement" << sqlStatement << endl;
			jsonStr += modifyFriend_Mysql("accept_friend", sqlStatement, jsonData);
			//////cout << "jsonStr = " << jsonStr << endl;
		}
		else
			return false;
		break;
	}
	case DELETE_FRIEND: {
		//////cout << "DELETE_FRIEND " << endl;
		if (jsonData.isMember("user_id")) {
			string user_id = jsonData["user_id"].asString();
			string friend_id = jsonData["friend_id"].asString();
			string sqlStatement = "CALL DeleteFriend ('" + user_id + "', '" + friend_id + "');";
			//////cout << "sqlStatement" << sqlStatement << endl;
			jsonStr += modifyFriend_Mysql("delete_friend", sqlStatement, jsonData);
			//////cout << "jsonStr = " << jsonStr << endl;
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
	if (! this->sendMessage) {
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


//string chat_conn::getMysqlResult(const string &type, const string &sqlStatement)

string chat_conn::getMysql_UserResult(const string& type, const string& sqlStatement, Json::Value jsonData)
{
	mysql = NULL;
	connectionRAII mysqlcon(&mysql, connPool);
	if (mysql_query(mysql, sqlStatement.c_str())) {
		LOG_ERROR("SELECT error:%s\n", mysql_error(mysql));
		return "error";
	}
	MYSQL_RES* result = mysql_store_result(mysql);	//ȡ�������
	if (!result) {
		//////cout << "Failed to store result" << endl;
		return "error";
	}
	int num_fields = mysql_num_fields(result);
	//////cout << "num_fields:::" << num_fields << endl;
	MYSQL_FIELD* fields = mysql_fetch_fields(result);//���������ֶνṹ�����飨��һ��ͨ����Ӱ��������̣���������Ҫ�ֶ���Ϣ��

	string responseJsonStr;
	json responseJson;

	if (mysql_num_rows(result) == 0) {
		//////cout << "No data rows in the result set" << endl;
		responseJson["type"] = type;
		responseJson["status"] = "error";
		responseJson["message"] = "name or code error";
		
	}
	else {
		//////cout << "mysql_num_rows != 0" << endl;
		while (MYSQL_ROW row = mysql_fetch_row(result)) {
			//////cout << "row             aaa" << endl;
			if (row == NULL) {
				//////cout << "Failed to fetch row" << endl;
				continue;  // ����������һ��
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
			ChatMapping::getInstance().addUserSocket(row[0], m_eventfd, this);
			//sub_reactors[idx]->map_id_socket.addMap(userid, client_fd);
			//////cout << "userID = " << row[0] << endl;
			//////cout << "m_sockfd = " << m_sockfd << endl;
			this->user->setUserId(row[0]);
			this->user->setUserPassword(row[1]);
			this->user->setUserPhone(row[2]);
			this->user->setUserName(row[3]);
			this->user->setUserEmail(row[4]);
			this->user->setUserAvatar(row[5]);
			this->user->setUserStatus(row[6]);
			this->user->setUserCreated(row[7]);
			
			//string changeStatus = "UPDATE m_user SET user_status = 'active' WHERE user_id = '" + this->user->getUserId() + "';";
			//mysql_query(mysql, changeStatus.c_str());
			this->user->setUserId("active");
		}
	}
	responseJsonStr = responseJson.dump(); responseJsonStr += "\n";
	//////cout << "responseJsonStr" << responseJsonStr << endl;
	return responseJsonStr;
}

string chat_conn::setMysql_UserResult(const string& type, const string& sqlStatement, Json::Value jsonData)
{
	mysql = NULL;
	connectionRAII mysqlcon(&mysql, connPool);
	if (mysql_query(mysql, sqlStatement.c_str())) {
		////cout << "SELECT error:%s\n" << mysql_error(mysql) << endl;
		return "error";
	}
	if (mysql_query(mysql, "SELECT @new_user_id;")) {
		////cout << "mysql_query() failed: " << mysql_error(mysql) << endl;
		return "error";
	}

	string responseJsonStr;
	json responseJson;
	

	MYSQL_RES* res = mysql_store_result(mysql);
	if (res == NULL) {
		//////cout << "mysql_store_result() failed: " << mysql_error(mysql) << endl;
	}
	else {
		MYSQL_ROW row = mysql_fetch_row(res);
		if (row) {
			//////cout << "New user ID: " << row[0] << endl;
			responseJson["type"] = type;
			responseJson["status"] = "success";
			responseJson["user_id"] = row[0];
		}
		else {
			//////cout << "No result returned." << endl;
		}
	}

	
	responseJsonStr = responseJson.dump(); responseJsonStr += "\n";
	return responseJsonStr;
}

string chat_conn::getMysql_FriendsResult(const string& type, const string& sqlStatement, Json::Value jsonData)
{
	mysql = NULL;
	connectionRAII mysqlcon(&mysql, connPool);

	if (mysql_query(mysql, sqlStatement.c_str())) {
		LOG_ERROR("SELECT error:%s\n", mysql_error(mysql));
		return "SELECT error";
	}
	MYSQL_RES* result = mysql_store_result(mysql);	//ȡ�������
	if (!result) {
		////cout << "Failed to store result" << endl;
		return "error";
	}
	int num_fields = mysql_num_fields(result);
	//////cout << "num_fields:::" << num_fields << endl;
	MYSQL_FIELD* fields = mysql_fetch_fields(result);//���������ֶνṹ�����飨��һ��ͨ����Ӱ��������̣���������Ҫ�ֶ���Ϣ��

	string responseJsonStr;
	json responseJson;

	if (mysql_num_rows(result) == 0) {
		//////cout << "No data rows in the result set" << endl;
		responseJson["type"] = type;
		responseJson["status"] = "null";
		responseJson["message"] = "have not friends";
	}
	else {
		while (MYSQL_ROW row = mysql_fetch_row(result)) {
			if (row == NULL) {
				//////cout << "Failed to fetch row" << endl;
				continue;  // ����������һ��
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

		}
	}

	responseJsonStr = responseJson.dump(); responseJsonStr += "\n";
	return responseJsonStr;

}

string chat_conn::getMysql_ChatsResult(const string& type, const string& sqlStatement, Json::Value jsonData)
{
	mysql = NULL;
	connectionRAII mysqlcon(&mysql, connPool);

	if (mysql_query(mysql, sqlStatement.c_str())) {
		LOG_ERROR("SELECT error:%s\n", mysql_error(mysql));
		return "SELECT error";
	}
	MYSQL_RES* result = mysql_store_result(mysql);	//ȡ�������
	if (!result) {
		//////cout << "Failed to store result" << endl;
		return "error";
	}
	int num_fields = mysql_num_fields(result);
	MYSQL_FIELD* fields = mysql_fetch_fields(result);//���������ֶνṹ�����飨��һ��ͨ����Ӱ��������̣���������Ҫ�ֶ���Ϣ��

	string responseJsonStr;
	json responseJson;
	

	if (mysql_num_rows(result) == 0) {
		//////cout << "No data rows in the result set" << endl;
		responseJson["type"] = type;
		responseJson["status"] = "null";
		responseJson["message"] = "have not chatting";
		
	}
	else {
		while (MYSQL_ROW row = mysql_fetch_row(result)) {
			if (row == NULL) {
				//////cout << "Failed to fetch row" << endl;
				continue;  // ����������һ��
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
			//////cout << "row[7]" << row[7] << endl;
			responseJson["user_name"] = row[8];
			responseJson["user_avatar"] = row[9];


			
		}
	}
	responseJsonStr = responseJson.dump(); responseJsonStr += "\n";
	return responseJsonStr;

}

string chat_conn::sendMessage_Mysql(const string& type, string& sqlStatement, Json::Value jsonData)
{

	string send_id = jsonData["send_id"].asString();
	string receiver_id = jsonData["receiver_id"].asString();
	string chat_type = jsonData["chat_type"].asString();
	string message_body = jsonData["message_body"].asString();
	string message_type = jsonData["message_type"].asString();
	string file_path = jsonData["file_path"].asString();
	string message_timestamp = jsonData["message_timestamp"].asString();

	string responseJsonStr;
	json responseJson;


	auto mapping = ChatMapping::getInstance().getSocketByUserId(receiver_id);
	if (!mapping.has_value()) {
		sqlStatement = "CALL AddFriendMessage('" + send_id + "', '" + receiver_id + "', '" + chat_type + "', '"
			+ message_body + "', '" + message_type + "', '" + file_path + "', 'send', '" + message_timestamp + "', @new_message_id);";
	}
	else {
		int eventdFd = mapping->first;	// ��i��subReactor
		chat_conn* conn = mapping->second;
		this->sendMessage = true;

		responseJson["type"] = "receive_message";
		responseJson["status"] = "success";
		responseJson["send_id"] = send_id;
		responseJson["receiver_id"] = receiver_id;
		responseJson["chat_type"] = chat_type;
		responseJson["message_body"] = message_body;
		responseJson["message_type"] = message_type;
		responseJson["file_path"] = file_path;
		responseJson["message_timestamp"] = message_timestamp;
		responseJsonStr = responseJson.dump();
		responseJsonStr += "\n";

		conn->write_lock.lock();
		copy(responseJsonStr.begin(), responseJsonStr.begin() + responseJsonStr.size(), conn->m_write_buf);
		conn->m_write_buf[responseJsonStr.size()] = '\0';
		conn->m_write_idx += responseJsonStr.size();
		conn->write_lock.unlock();
		modfd(conn->m_epollfd, conn->m_sockfd, EPOLLOUT, conn);	//�޸ĳ�д����

		sqlStatement = "CALL AddFriendMessage('" + send_id + "', '" + receiver_id + "', '" + chat_type + "', '"
			+ message_body + "', '" + message_type + "', '" + file_path + "', 'received', '" + message_timestamp + "', @new_message_id);";
	}

	mysql = NULL;
	connectionRAII mysqlcon(&mysql, connPool);
	if (mysql_query(mysql, sqlStatement.c_str())) {
		printf("SELECT error:%s\n", mysql_error(mysql));
		responseJson["type"] = type;
		responseJson["status"] = "error";
		responseJson["message"] = "send error";
		responseJsonStr = responseJson.dump(); responseJsonStr += "\n";
		return responseJsonStr;
	}


	responseJson["type"] = type;
	responseJson["status"] = "success";
	responseJsonStr = responseJson.dump(); responseJsonStr += "\n";
	return responseJsonStr;
}

/*
string chat_conn::sendMessage_Mysql(const string& type, string& sqlStatement, Json::Value jsonData)
{

	string send_id = jsonData["send_id"].asString();
	string receiver_id = jsonData["receiver_id"].asString();
	string chat_type = jsonData["chat_type"].asString();
	string message_body = jsonData["message_body"].asString();
	string message_type = jsonData["message_type"].asString();
	string file_path = jsonData["file_path"].asString();
	string message_timestamp = jsonData["message_timestamp"].asString();
	
	string responseJsonStr;
	json responseJson;

	auto mapping = ChatMapping::getInstance().getSocketByUserId(receiver_id);
	if (!mapping.has_value()) {
		sqlStatement = "CALL AddFriendMessage('" + send_id + "', '" + receiver_id + "', '" + chat_type + "', '"
			+ message_body + "', '" + message_type + "', '" + file_path + "', 'send', '" + message_timestamp + "', @new_message_id);";
		////cout << "sqlStatement" << sqlStatement << endl;
		dbTaskQueue->push(sqlStatement);

	}
	else {
		int eventdFd = mapping->first;	// ��i��subReactor
		chat_conn* conn = mapping->second;
		this->sendMessage = true;

		responseJson["type"] = "receive_message";
		responseJson["status"] = "success";
		responseJson["send_id"] = send_id;
		responseJson["receiver_id"] = receiver_id;
		responseJson["chat_type"] = chat_type;
		responseJson["message_body"] = message_body;
		responseJson["message_type"] = message_type;
		responseJson["file_path"] = file_path;
		responseJson["message_timestamp"] = message_timestamp;
		responseJsonStr = responseJson.dump();
		responseJsonStr += "\n";

		conn->write_lock.lock();
		copy(responseJsonStr.begin(), responseJsonStr.begin() + responseJsonStr.size(), conn->m_write_buf);
		conn->m_write_buf[responseJsonStr.size()] = '\0';
		conn->m_write_idx += responseJsonStr.size();
		conn->write_lock.unlock();
		modfd(conn->m_epollfd, conn->m_sockfd, EPOLLOUT, conn);	//�޸ĳ�д����
		
		sqlStatement = "CALL AddFriendMessage('" + send_id + "', '" + receiver_id + "', '" + chat_type + "', '"
			+ message_body + "', '" + message_type + "', '" + file_path + "', 'received', '" + message_timestamp + "', @new_message_id);";
		dbTaskQueue->push(sqlStatement);

	}
	
	responseJson["type"] = type;
	responseJson["status"] = "success";
	responseJsonStr = responseJson.dump(); responseJsonStr += "\n";
	return responseJsonStr;
}
*/
string chat_conn::updateChat_Mysql(Json::Value jsonData)
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

	//////cout << "sqlStatement" << sqlStatement << endl;

	mysql = NULL;
	connectionRAII mysqlcon(&mysql, connPool);

	string responseJsonStr;
	json responseJson;
	

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
		//////cout << "No rows updated or query failed." << endl;
	}

	sqlStatement = "UPDATE m_friends_message SET message_status = 'read' WHERE(m_friends_message.sender_id = '" + comm_id + "' AND m_friends_message.receiver_id = '" + user_id + "'); ";
	//////cout << "sqlStatement" << sqlStatement << endl;
	if (mysql_query(mysql, sqlStatement.c_str())) {
		cerr << "mysql error:%s\n" << endl;
		return "mysql error";
	}

	

	responseJsonStr = responseJson.dump(); responseJsonStr += "\n";
	return responseJsonStr;
}

string chat_conn::getFriendsUnreadMessages_Mysql(const string& type, const string& sqlStatement, Json::Value jsonData)
{
	mysql = NULL;
	connectionRAII mysqlcon(&mysql, connPool);
	if (mysql_query(mysql, sqlStatement.c_str())) {
		LOG_ERROR("SELECT error:%s\n", mysql_error(mysql));
		return "SELECT error";
	}
	MYSQL_RES* result = mysql_store_result(mysql);	//ȡ�������
	if (!result) {
		cerr << "Failed to store result" << endl;
		return "error";
	}
	int num_fields = mysql_num_fields(result);
	//////cout << "num_fields:::" << num_fields << endl;
	MYSQL_FIELD* fields = mysql_fetch_fields(result);//���������ֶνṹ�����飨��һ��ͨ����Ӱ��������̣���������Ҫ�ֶ���Ϣ��

	string responseJsonStr;
	json responseJson;
	
	//////cout << "mysql_num_rows(result) = " << mysql_num_rows(result) << endl;
	if (mysql_num_rows(result) == 0) {

		//////cout << "No data rows in the result set" << endl;
		responseJson["type"] = type;
		responseJson["status"] = "null";
		responseJson["message"] = "don't have messages";

		
	}
	else {
		while (MYSQL_ROW row = mysql_fetch_row(result)) {
			if (row == NULL) {
				//////cout << "Failed to fetch row" << endl;
				continue;  // ����������һ��
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

			
		}
	}
	responseJsonStr = responseJson.dump(); responseJsonStr += "\n";
	return responseJsonStr;

}

string chat_conn::searchUserInformation_Mysql(const string& type, const string& sqlStatement, Json::Value jsonData)
{
	mysql = NULL;
	connectionRAII mysqlcon(&mysql, connPool);
	if (mysql_query(mysql, sqlStatement.c_str())) {
		LOG_ERROR("SELECT error:%s\n", mysql_error(mysql));
		return "SELECT error";
	}
	MYSQL_RES* result = mysql_store_result(mysql);	//ȡ�������
	if (!result) {
		cerr << "Failed to store result" << endl;
		return "error";
	}
	int num_fields = mysql_num_fields(result);

	string responseJsonStr;
	json responseJson;
	
	//////cout << "mysql_num_rows(result) = " << mysql_num_rows(result) << endl;
	if (mysql_num_rows(result) == 0) {

		//////cout << "No data rows in the result set" << endl;
		responseJson["type"] = type;
		responseJson["status"] = "null";
		responseJson["message"] = "don't have messages";

		
	}
	else {
		while (MYSQL_ROW row = mysql_fetch_row(result)) {
			if (row == NULL) {
				//////cout << "Failed to fetch row" << endl;
				continue;  // ����������һ��
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

			
		}
	}

	responseJsonStr = responseJson.dump(); responseJsonStr += "\n";
	return responseJsonStr;
}

string chat_conn::modifyFriend_Mysql(const string& type, const string& sqlStatement, Json::Value jsonData)
{
	mysql = NULL;
	connectionRAII mysqlcon(&mysql, connPool);

	string responseJsonStr;
	json responseJson;
	

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
		cerr << "No rows updated or query failed." << endl;
	}

	

	responseJsonStr = responseJson.dump(); responseJsonStr += "\n";
	return responseJsonStr;
}

void chat_conn::removeUser()
{
	ChatMapping::getInstance().removeUserSocket(this->user->getUserId());
	epoll_ctl(m_epollfd, EPOLL_CTL_DEL, m_sockfd, 0);
	close(m_sockfd);
}
