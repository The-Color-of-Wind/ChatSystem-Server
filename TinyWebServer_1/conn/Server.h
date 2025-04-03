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
	// 用于存储 userId 与 socket 的映射关系
	unordered_map<string, int> userSocketMap;

};