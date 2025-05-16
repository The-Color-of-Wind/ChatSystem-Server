/*
#ifndef THREADPOOL1_H
#define THREADPOOL1_H


#include <list>
#include <cstdio>
#include <pthread.h>
#include <exception>
#include "../lock/locker.h"
#include "../MysqlConnectPool/sql_connect_pool.h"


template<typename T>
class threadpool
{
public:
	threadpool(connection_pool *connPool, int thread_number = 8, int max_request = 10000);
	~threadpool();
	//加入任务队列函数
	bool append(T *request);

private:
	/*
	pthread_create陷阱
	函数原型中的第三个参数，为函数指针，指向处理线程函数的地址。该函数，要求为静态函数。如果处理线程函数为类成员函数时，需要将其设置为静态成员函数。
	*/
	//工作任务函数
	static void *worker(void *arg);
	void run();

private:
	int m_thread_number;	//线程池中的线程数
	pthread_t *m_threads;	//描述线程池中线程的数组，大小为m_thread_number
	std::list<T *> m_workqueue;	//请求队列
	int m_max_requests;		//请求队列的最大请求数
	locker m_queuelocker;	//请求队列的互斥锁
	sem m_queuestat;		//请求队列的sem信号量  
	bool m_stop;			//是否结束线程

	connection_pool *m_connPool;	//数据库池
	//加管理线程

};


template<typename T>
threadpool<T>::threadpool(connection_pool *connPool, int thread_number, int max_requests) :m_thread_number(thread_number), m_max_requests(max_requests), m_stop(false), m_threads(NULL), m_connPool(connPool)
{//数据库
	if (thread_number <= 0 || max_requests <= 0)
		throw std::exception();
	m_threads = new pthread_t[m_thread_number];
	if (!m_threads)
		throw std::exception();
	for (int i = 0; i < thread_number; ++i)
	{
		//printf("create the %dth thread\n",i);
		if (pthread_create(m_threads + i, NULL, worker, this) != 0)
		{
			delete[] m_threads;
			throw std::exception();
		}
		if (pthread_detach(m_threads[i]))
		{
			delete[] m_threads;
			throw std::exception();
		}
	}
}

template<typename T>
threadpool<T>::~threadpool()
{

	delete[] m_threads;
	m_stop = true;
}

template<typename T>
bool threadpool<T>::append(T *request)
{
	m_queuelocker.lock();
	if (m_workqueue.size() > m_max_requests)
	{
		m_queuelocker.unlock();
		return false;
	}
	m_workqueue.push_back(request);
	m_queuelocker.unlock();
	m_queuestat.post();
	return true;
}
template<typename T>
void * threadpool<T>::worker(void *arg)
{
	threadpool *pool = (threadpool *)arg;
	pool->run();
	return pool;
}
template<typename T>
void threadpool<T>::run()
{
	int i = 0;
	while (!m_stop)
	{
		//printf("thread: pid = %d, tid = %lu\n", getpid(), pthread_self());
		m_queuestat.wait();
		//printf("thread: pid = %d, tid = %lu\n", getpid(), pthread_self());
		m_queuelocker.lock();
		if (m_workqueue.empty())
		{
			m_queuelocker.unlock();
			printf("m_workqueue.empty");
			continue;
		}
		T *request = m_workqueue.front();
		m_workqueue.pop_front();
		m_queuelocker.unlock();

		if (!request)
			continue;
		//数据库

		connectionRAII mysqlcon(&request->mysql, m_connPool);
		request->process();

		//printf("\n\n\n");
	}

	return;
}


#endif

*/