#ifndef LOCKER_H
#define LOCKER_H

#include<pthread.h>
#include<semaphore.h>
#include<exception>
#include <ostream>
#include <iostream>

class sem
{
public:
	sem()
	{
		if (sem_init(&m_sem, 0, 0) != 0)
		{
			throw std::exception();
		}
	}
	sem(int num)
	{
		if (sem_init(&m_sem, 0, num) != 0)
		{
			throw std::exception();
		}
	}
	~sem()
	{
		sem_destroy(&m_sem);
	}
	bool wait()
	{
		return sem_wait(&m_sem) == 0;
	}
	bool post()
	{
		return sem_post(&m_sem) == 0;
	}

private:
	sem_t m_sem;
};
class locker
{
public:
	locker()
	{
		if (pthread_mutex_init(&m_mutex, NULL) != 0)
		{
			std::cerr << "[locker] mutex 初始化失败，错误码：" << errno << std::endl;
			abort();  // 直接退出，防止继续使用坏的锁
			throw std::exception();
		}
	}
	~locker()
	{
		pthread_mutex_destroy(&m_mutex);
	}
	bool lock()
	{
		return pthread_mutex_lock(&m_mutex) == 0;
	}
	bool unlock()
	{
		return pthread_mutex_unlock(&m_mutex) == 0;
	}
	pthread_mutex_t *get()
	{
		return &m_mutex;
	}

private:
	pthread_mutex_t m_mutex;
};
class cond
{
public:
	cond()
	{
		if (pthread_cond_init(&m_cond, NULL) != 0) {
			throw std::exception();
		}
	}
	~cond()
	{
		pthread_cond_destroy(&m_cond);
	}
	bool wait(pthread_mutex_t *m_mutex)	//成功，返回0
	{
		int ret = 0;
		ret = pthread_cond_wait(&m_cond, m_mutex);
		return ret == 0;
	}
	//有一个超时的限制
	bool timewait(pthread_mutex_t *m_mutex, struct timespec t)
	{
		int ret = pthread_cond_timedwait(&m_cond, m_mutex, &t);
		return ret == 0;
	}
	bool signal()
	{
		return pthread_cond_signal(&m_cond) == 0;
	}
	//唤醒所有正在等待某个特定条件变量上的线程。
	bool broadcast()
	{
		return pthread_cond_broadcast(&m_cond) == 0;
	}
	pthread_cond_t *get()
	{
		return &m_cond;
	}
private:
	pthread_cond_t m_cond;
};
#endif

