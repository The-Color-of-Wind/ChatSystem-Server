#include "ChatMapping.h"

// 添加或更新映射

void ChatMapping::addUserSocket(const string& userId, int socketFd) {

	mapLocker.lock();

	userSocketMap[userId] = socketFd;
	mapLocker.unlock();

}


// 查找 userId 对应的 socket
int ChatMapping::getSocketByUserId(const string& userId) {
	mapLocker.lock();
	if (userSocketMap.find(userId) != userSocketMap.end()) {
		int userSocket = userSocketMap[userId];
		mapLocker.unlock();
		return userSocket;
	}
	mapLocker.unlock();
	return -1; // 返回 -1 表示没有找到对应的 socket
}

void ChatMapping::removeUserSocket(const string& userId)
{
	mapLocker.lock();
	if (userSocketMap.find(userId) != userSocketMap.end()) {
		userSocketMap[userId] = -1;
	}
	mapLocker.unlock();
	return;
}