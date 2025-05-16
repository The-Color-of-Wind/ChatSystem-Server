#ifndef THREADPOOL_H
#define THREADPOOL_H
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <functional>
#include <memory>
#include <atomic>


// ======= �̳߳�ʵ�� =======
template<typename T>
class ThreadPool {
public:
    ThreadPool(connection_pool* connPool, size_t thread_count) : m_connPool(connPool), stop(false) {
        for (size_t i = 0; i < thread_count; ++i) {
            workers.emplace_back([this]() {
                while (true) {
                    std::function<void()> task;

                    {
                        std::unique_lock<std::mutex> lock(this->mtx);
                        this->cv.wait(lock, [this]() {
                            return this->stop || !this->tasks.empty();
                            });

                        if (this->stop && this->tasks.empty())
                            return;

                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }

                    task();  // ִ������
                }
                });
        }
    }

    ~ThreadPool() {
        {
            std::lock_guard<std::mutex> lock(mtx);
            stop = true;
        }
        cv.notify_all();
        for (std::thread& worker : workers)
            worker.join();
    }

    // �ύ���������̳߳�
    void submit(std::function<void()> task) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            tasks.push(std::move(task));
        }
        cv.notify_one();  // ����һ���߳�
    }

private:
    std::vector<std::thread> workers;                 // �߳�����
    std::queue<std::function<void()>> tasks;          // �����������
    std::mutex mtx;
    std::condition_variable cv;
    bool stop;

    connection_pool* m_connPool;	//���ݿ��
};



#endif