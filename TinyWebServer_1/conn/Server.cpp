#include"Server.h"

// 添加或更新映射

void Server::addUserSocket(const string& userId, int socketFd) {
	userSocketMap[userId] = socketFd;
}

// 查找 userId 对应的 socket
int Server::getSocketByUserId(const string& userId) {
	if (userSocketMap.find(userId) != userSocketMap.end()) {
		return userSocketMap[userId];
	}
	return -1; // 返回 -1 表示没有找到对应的 socket
}

// 转发信息到指定 userId 对应的客户端
void Server::forwardMessage(const int socketFd, const string& message) {
	if (socketFd != -1) {
		send(socketFd, message.c_str(), message.size(), 0);
		cout << "Message forwarded to userId: "  << endl;
	}
	else {
		cout << "UserId "  << " not connected." << endl;
	}
}

void Server::removeUserSocket(const string& userId)
{
	if (userSocketMap.find(userId) != userSocketMap.end()) {
		userSocketMap[userId] = -1;
	}
	return;
}