#include "MainReactor.h"
#include <unistd.h>
#include <fcntl.h>

void print_fd_info(int fd) {
	char buf[1024] = {};
	snprintf(buf, sizeof(buf), "/proc/self/fd/%d", fd);
	char target[1024] = {};
	ssize_t len = readlink(buf, target, sizeof(target) - 1);
	if (len != -1) {
		target[len] = '\0';
		// cout << "fd " << fd << " -> " << target << std::endl;
	}
	else {
		perror("readlink");
	}
}

MainReactor::MainReactor(int port, int num_reactors) :port(port), num_reactors(num_reactors)
{
	main_epoll_fd = epoll_create1(0);

	create_listener();

	for (int i = 0; i < num_reactors; i++) {
		SubReactor* subReactor = new SubReactor();
		subReactor->init();
		subReactor->epoll_fd = epoll_create1(0);

		subReactor->event_fd = eventfd(0, EFD_NONBLOCK);
		
		chat_conn* conn = new chat_conn(subReactor->epoll_fd, i, -1, subReactor->pool);
		
		ChatMapping::getInstance().addEventConn(conn);

		add_fd(subReactor->epoll_fd, subReactor->event_fd, true, conn);

		pthread_create(&subReactor->thread, nullptr, startServer, subReactor);
		sub_reactors.push_back(subReactor);

		// 插入函数（估计是没保存）
	}

}
void* MainReactor::startServer(void* arg) {
	// cout << "MainReactor startServer" << endl;
	SubReactor* self = static_cast<SubReactor*>(arg);
	self->startServer(self->epoll_fd);
	
	return nullptr;
}
MainReactor::~MainReactor()
{
	for (auto reactor : sub_reactors) {
		delete reactor;
	}
	sub_reactors.clear();
}



void MainReactor::create_listener()
{
	listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	assert(listen_fd >= 0);
	
	std::cout << "[debug] socket created listen_fd = " << listen_fd << std::endl;

	int flag = 1;
	setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

#ifdef SO_REUSEPORT
	// 多线程共享端口，提高多核并发能力（需内核支持）
	setsockopt(listen_fd, SOL_SOCKET, SO_REUSEPORT, &flag, sizeof(flag));
#endif

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(port);


	int ret = bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
	assert(ret >= 0);

	//set_nonblocking(listen_fd);

	ret = listen(listen_fd, SOMAXCONN);
	assert(ret >= 0);


	chat_conn* conn = new chat_conn(main_epoll_fd, listen_fd, -1, nullptr);
	add_fd(main_epoll_fd, listen_fd, false, conn);

	return;
}

void MainReactor::run()
{

	struct epoll_event events[MAX_EVENT_NUMBER];
	for (int i = 0; i < MAX_EVENT_NUMBER; i++) {
		events[i].data.ptr = nullptr;
	}
	stop_server = false;
	while (!stop_server)
	{
		int number = epoll_wait(main_epoll_fd, events, IniConfig::getInstance().getThreadNum(), -1);
		//cout << "number" << number<< endl;
		if (number < 0 && errno != EINTR)
		{
			perror ("errno:");
			break;
		}
		for (int i = 0; i < number; i++)
		{
			auto* conn = static_cast<chat_conn*>(events[i].data.ptr);
			//cout << "listen sockfd" << conn->m_sockfd << endl;
			//cout << "listen fd" << listen_fd << endl;

			if (conn->m_sockfd == listen_fd)
			{
				struct sockaddr_in client_address;
				socklen_t client_addrlength = sizeof(client_address);
				if (!IniConfig::getInstance().getListenEt()) {
					int client_fd = accept(listen_fd, (struct sockaddr*)&client_address, &client_addrlength);
					//cout << "client_fd" << client_fd << endl;
					if (client_fd < 0)
					{
						printf("%s:errno is:%d", "accept error", errno);
						continue;
					}

					assign_connection(client_fd);
				}
				else {
					while (1)
					{
						cout << "while (1)" << endl;
						int client_fd = accept(listen_fd, (struct sockaddr*)&client_address, &client_addrlength);
						if (client_fd < 0)
						{
							printf("%s:errno is:%d", "accept error", errno);
							continue;
						}

						//初始化用户,init里面有加入红黑树
						assign_connection(client_fd);
					}
					continue;
				}
			}

		}
	}


}



void MainReactor::assign_connection(int client_fd)
{
	int min_index = 0;
	int min_conn = sub_reactors[0]->conn_count;

	for (int i = 1; i < num_reactors; ++i)
	{
		if (sub_reactors[i]->conn_count < min_conn)
		{
			min_conn = sub_reactors[i]->conn_count;
			min_index = i;
		}
	}

	chat_conn* conn = new chat_conn(sub_reactors[min_index]->epoll_fd, client_fd, sub_reactors[min_index]->event_fd, sub_reactors[min_index]->pool, sub_reactors[min_index]->dbTaskQueue);
	conn->connPool = sub_reactors[min_index]->connPool;

	add_fd(sub_reactors[min_index]->epoll_fd, client_fd, true, conn);

	// 新连接增加计数
	++sub_reactors[min_index]->conn_count;
}
