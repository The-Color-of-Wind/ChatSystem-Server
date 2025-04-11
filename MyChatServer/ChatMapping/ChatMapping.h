#ifndef CHATMAPPING_H
#define CHATMAPPING_H

#include <iostream>
#include <unordered_map>
#include <string>
#include <sys/socket.h>

#include "../lock/locker.h"
using namespace std;

class chat_conn;

class ChatMapping {
public:
	static ChatMapping& getInstance() {
		static ChatMapping instance;
		return instance;
	}

	void addUserSocket(const string& userId, int socketFd);
	void removeUserSocket(const string& userId);

	int getSocketByUserId(const string& userId);
	//void forwardMessage(const int socketFd, const string& message);
	void setUsers(chat_conn* users) { this->users = users; }
	chat_conn* getUsers() { return this->users; }
private:
	ChatMapping(){ }
	ChatMapping(const ChatMapping&) = delete;
	ChatMapping& operator=(const ChatMapping&) = delete;
private:
	// 用于存储 userId 与 socket 的映射关系
	unordered_map<string, int> userSocketMap;
	chat_conn* users;
	locker mapLocker;

};

#endif