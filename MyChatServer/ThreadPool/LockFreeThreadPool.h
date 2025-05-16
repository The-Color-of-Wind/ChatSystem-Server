#ifndef LOCKFREE_LockFreeThreadPool_H
#define LOCKFREE_LockFreeThreadPool_H

#include <cstdio>
#include <pthread.h>
#include <exception>
#include <atomic>
#include <memory>
#include <cassert>
#include <functional>
#include <vector>
#include "../lock/LockFreeQueue.h"
#include "../MysqlConnectPool/sql_connect_pool.h"

using namespace std;

template<typename T>
class LockFreeThreadPool
{
public:
	LockFreeThreadPool(connection_pool* connPool, int thread_number = 8, int max_request = 10000);
	~LockFreeThreadPool();
	//����������к���
	bool append(T* request);
	//bool append(shared_ptr<T> task);
private:
	/*
	pthread_create����
	����ԭ���еĵ�����������Ϊ����ָ�룬ָ�����̺߳����ĵ�ַ���ú�����Ҫ��Ϊ��̬��������������̺߳���Ϊ���Ա����ʱ����Ҫ��������Ϊ��̬��Ա������
	*/
	//����������
	static void* worker(void* arg);

private:
	int m_thread_number;	//�̳߳��е��߳���
	pthread_t* m_threads;	//�����̳߳����̵߳����飬��СΪm_thread_number
	int m_max_requests;
	atomic<bool> m_stop;			//�Ƿ�����߳�

	LockFreeQueue<T> task_queue;
	connection_pool* m_connPool;	//���ݿ��
	//�ӹ����߳�

};


template<typename T>
LockFreeThreadPool<T>::LockFreeThreadPool(connection_pool* connPool, int thread_number, int max_requests) :m_thread_number(thread_number), m_max_requests(max_requests), m_stop(false), m_threads(NULL), m_connPool(connPool)
{//���ݿ�
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
LockFreeThreadPool<T>::~LockFreeThreadPool()
{
	m_stop = true;
	delete[] m_threads;
	
}

template<typename T>
bool LockFreeThreadPool<T>::append(T* task)
{
	//return task_queue.enqueue(move(task));
	return task_queue.enqueue(task);

}
template<typename T>
void* LockFreeThreadPool<T>::worker(void* arg)
{
	LockFreeThreadPool* pool = static_cast<LockFreeThreadPool*> (arg);
	int idle_count = 0;
	while (!pool->m_stop.load())
	{
		auto task = pool->task_queue.dequeue();
		if (!task) {
			if (++idle_count >= 1000) {
				usleep(1000);
			}
			else {
				sched_yield();
			}
			continue;
		}
		idle_count = 0;
		connectionRAII mysqlcon(&task->mysql, pool->m_connPool);
		task->process();
	}

	return pool;
}

#endif