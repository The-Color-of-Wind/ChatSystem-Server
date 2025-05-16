#ifndef CHATMAPPING_H
#define CHATMAPPING_H

#include <iostream>
#include <unordered_map>
#include <string>
#include <sys/socket.h>
#include <shared_mutex>
#include <optional>

#include "../lock/locker.h"
#include "../ChatConn/chat_conn.h"
using namespace std;

class chat_conn;

class ChatMapping {
public:
	static ChatMapping& getInstance() {
		static ChatMapping instance;
		return instance;
	}

	void addUserSocket(const string& userId, int subReactorId, chat_conn* conn);
	void removeUserSocket(const string& userId);
	optional<pair<int, chat_conn*>> getSocketByUserId(const string& userId);
	//void forwardMessage(const int socketFd, const string& message);
	//void setUsers(chat_conn* users) { this->users = users; }
	//chat_conn* getUsers() { return this->users; }
	vector<chat_conn*> getEventConn() { return this->evnetConn; }
	void addEventConn(chat_conn* conn) {
		this->evnetConn.push_back(conn);
		return;
	}
private:
	ChatMapping(){ }
	ChatMapping(const ChatMapping&) = delete;
	ChatMapping& operator=(const ChatMapping&) = delete;
private:

	std::unordered_map<string, pair<int, chat_conn*>> userSocketMap;

	//chat_conn* users;

	shared_mutex map_rw_mutex;
	std::vector<chat_conn*> evnetConn;

};

#endif