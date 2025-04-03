#include"Server.h"

// ��ӻ����ӳ��

void Server::addUserSocket(const string& userId, int socketFd) {
	userSocketMap[userId] = socketFd;
}

// ���� userId ��Ӧ�� socket
int Server::getSocketByUserId(const string& userId) {
	if (userSocketMap.find(userId) != userSocketMap.end()) {
		return userSocketMap[userId];
	}
	return -1; // ���� -1 ��ʾû���ҵ���Ӧ�� socket
}

// ת����Ϣ��ָ�� userId ��Ӧ�Ŀͻ���
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