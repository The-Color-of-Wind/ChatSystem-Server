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
				//failLogger.log(sql); // �־û�ʧ��SQL
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
	//�����̳߳�
	pool = NULL;
	try
	{
		pool = new ThreadPool<chat_conn>(connPool, 150);
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
	return -1; // ���� -1 ��ʾû���ҵ���Ӧ�� socket
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
	epoll_fd = epollFd;

	stop_server = false;	//ѭ������
	struct epoll_event events[MAX_EVENT_NUMBER];

	while (!stop_server)
	{
		int number = epoll_wait(epoll_fd, events, IniConfig::getInstance().getThreadNum(), -1);
		
		if (number < 0 && errno != EINTR)
		{
			break;
		}
		for (int i = 0; i < number; i++)
		{
			auto* conn = static_cast<chat_conn*>(events[i].data.ptr);
			if (conn->m_sockfd < 0)
			{
				LOG_ERROR("%s:errno is:%d", "accept error", errno);
				continue;
			}
			//�ͻ��˹ر�
			if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
			{
				conn->removeUser();
			}

			//����ͻ������ϵĶ��ź�
			else if (events[i].events & EPOLLIN)
			{

				if (receive_data(conn)) {
					//cout << "receive_data success" << endl;
				}
				else {
					LOG_ERROR("%s:errno is:%d", "receive_data error", errno);
				}

			}
			//����д�¼�
			else if (events[i].events & EPOLLOUT)
			{
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


	stopServer();	//�����ͷ�
}

void SubReactor::stopServer()
{

	close(epoll_fd);

	delete pool;
}
