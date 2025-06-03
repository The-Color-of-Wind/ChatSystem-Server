#include <iostream>
#include <unordered_map>
#include <string>
#include <sys/socket.h>

using namespace std;


class Server {
public:
	static Server* instance() {
		static Server server;
		return &server;
	}

	void addUserSocket(const string& userId, int socketFd);
	void removeUserSocket(const string& userId);

	int getSocketByUserId(const string& userId);
	void forwardMessage(const int socketFd, const string& message);


private:
	Server() {}
	Server(const Server&) = delete;
	Server& operator=(const Server&) = delete;
private:
	// ���ڴ洢 userId �� socket ��ӳ���ϵ
	unordered_map<string, int> userSocketMap;

};