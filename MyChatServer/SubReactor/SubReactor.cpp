#include "SubReactor.h"

void SubReactor::init()
{
	conn_count = 0;

	dbTaskQueue = new DBTaskQueue();

	initConnPool();
	initThreadPool();
	/*
	for (int i = 0; i < 6; ++i) {
		dbThreads.emplace_back([this]() {
			this->dbWriterLoop();
			});
	}
	*/
	
}


void SubReactor::dbWriterLoop() {
	DBTaskQueue* queue = this->dbTaskQueue;

	size_t max_batch = 1000;
	int wait_time = 10;
	int n = 1;
	while (this->running_) {
		auto batch = queue->popBatch(max_batch);

		if (batch.empty()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(wait_time));
			wait_time += 10*(n++);
			continue;
		}
		wait_time = 10;
		n = 1;
		MYSQL* mysql = NULL;
		connectionRAII mysqlcon(&mysql, this->connPool);


		for (const auto& sql : batch) {
			if (mysql_query(mysql, sql.c_str()) != 0) {
				//failLogger.log(sql); // 持久化失败SQL
				LOG_ERROR("%s:errno is:%d", "mysql error", mysql_error(mysql));
				this->dbTaskQueue->push(sql);
			}
		}
	}
	return;
}
void SubReactor::initConnPool()
{
	connPool = connection_pool::GetInstance();

	connPool->init(IniConfig::getInstance().db_host, IniConfig::getInstance().db_user, IniConfig::getInstance().db_pwd, IniConfig::getInstance().db_name, IniConfig::getInstance().db_port, IniConfig::getInstance().db_conn_num);

}

void SubReactor::initThreadPool()
{
	//创建线程池
	pool = NULL;
	try
	{
		pool = new ThreadPool<chat_conn>(connPool, 150);
		// cout << "ThreadPool init" << endl;
	}
	catch (...)
	{
		return;
	}
	return;
}

void SubReactor::addMap(string userid, int socketfd) {
	unique_lock<shared_mutex> lock(map_rw_mutex);
	map_id_socket.insert(make_pair(userid, socketfd));
	return;
}
int SubReactor::getSocket(string userid) {
	shared_lock<shared_mutex> lock(map_rw_mutex);
	auto it = map_id_socket.find(userid);
	if (it != map_id_socket.end()) {

		return it->second;
	}
	return -1; // 返回 -1 表示没有找到对应的 socket
}

void SubReactor::removeMap(string userid) {
	unique_lock<shared_mutex> lock(map_rw_mutex);
	map_id_socket.erase(userid);
	return;
}


bool SubReactor::receive_data(chat_conn* conn)
{
	return(conn->receive());
}


bool SubReactor::send_data(chat_conn* conn)
{
	return(conn->write());

}

void SubReactor::startServer(int epollFd)
{
	// cout << "SubReactor startServer" << endl;
	epoll_fd = epollFd;
	// cout << "SubReactor epoll_fd" << epoll_fd << endl;

	stop_server = false;	//循环条件
	struct epoll_event events[MAX_EVENT_NUMBER];

	while (!stop_server)
	{
		//// cout << "IniConfig::getInstance().getThreadNum():" << IniConfig::getInstance().getThreadNum() << endl;
		int number = epoll_wait(epoll_fd, events, IniConfig::getInstance().getThreadNum(), -1);
		// cout << "SubReactor number" << number << endl;
		
		if (number < 0 && errno != EINTR)
		{
			// cout << "errno:" << endl;
			break;
		}
		for (int i = 0; i < number; i++)
		{
			auto* conn = static_cast<chat_conn*>(events[i].data.ptr);
			// cout << "SubReactor epoll_fd" << epoll_fd << endl;
			// cout << "SubReactor sockfd" << conn->m_sockfd << endl;
			if (conn->m_sockfd < 0)
			{
				LOG_ERROR("%s:errno is:%d", "accept error", errno);
				continue;
			}
			//客户端关闭
			if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
			{
				//cout << "conn->removeUser() action" << endl;
				conn->removeUser();
			}

			//处理客户连接上的读信号
			else if (events[i].events & EPOLLIN)
			{

				// cout << "receive_data action" << endl;
				if (receive_data(conn)) {
					//cout << "receive_data success" << endl;
				}
				else {
					LOG_ERROR("%s:errno is:%d", "receive_data error", errno);
				}

			}
			//处理写事件
			else if (events[i].events & EPOLLOUT)
			{
				// cout << "send_data action" << endl;
				if (!send_data(conn))
				{
					LOG_ERROR("%s:errno is:%d", "send_data error", errno);
				}
				else {
					//cerr << "send_data success" << endl;
				}

			}
		}
	}


	stopServer();	//结束释放
}

void SubReactor::stopServer()
{

	close(epoll_fd);

	delete pool;
}
