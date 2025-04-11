#include "ChatMapping.h"

// ��ӻ����ӳ��

void ChatMapping::addUserSocket(const string& userId, int socketFd) {

	mapLocker.lock();

	userSocketMap[userId] = socketFd;
	mapLocker.unlock();

}


// ���� userId ��Ӧ�� socket
int ChatMapping::getSocketByUserId(const string& userId) {
	mapLocker.lock();
	if (userSocketMap.find(userId) != userSocketMap.end()) {
		int userSocket = userSocketMap[userId];
		mapLocker.unlock();
		return userSocket;
	}
	mapLocker.unlock();
	return -1; // ���� -1 ��ʾû���ҵ���Ӧ�� socket
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