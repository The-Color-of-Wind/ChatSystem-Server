
#include <iostream>

#include "./ChatServer/ChatServer.h"

using namespace std;

int main()
{


	ChatServer *chatServer = new ChatServer();

	chatServer->readConfigFile();	// 读取配置文件

	chatServer->init();	// 初始化线程池、数据库池、用户、lfd、epoll

	chatServer->startServer();	// 开始循环监听

	chatServer->stopServer();	//结束释放


	return 0;
}


