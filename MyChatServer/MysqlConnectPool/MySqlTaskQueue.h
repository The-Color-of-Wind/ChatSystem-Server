#ifndef MYSQLTASKQUEUE_H
#define MYSQLTASKQUEUE_H


#include <vector>
#include <chrono>
#include <mutex>
#include <thread>

class MySqlTaskQueue
{
public:
    MySqlTaskQueue(size_t maxQueueSize, std::chrono::milliseconds interval)
        : maxQueueSize_(maxQueueSize), interval_(interval), stopFlag_(false)
    {
        
    }



    ~MySqlTaskQueue()
    {
        stopFlag_ = true;
        if (workerThread_.joinable())
        {
            workerThread_.join();
        }
    }


    void addTask(std::string& task)
    {
        mutex_.lock();
        taskQueue_.push_back(task);
        mutex_.unlock();

        if (taskQueue_.size() >= maxQueueSize_) {
            workerThread_ = std::thread(&MySqlTaskQueue::processTasks, this);
        }
    }

    void processTasks()
    {
        if (!stopFlag_) {
            lock_guard<std::mutex> lock(mutex_);
            if (taskQueue_.empty()) {
                return;
            }
            executeDatabaseTask();

            taskQueue_.clear();
        }
        return;
        
    }

    void start();

private:
    void executeDatabaseTask(const string& task)
    {

        MYSQL* mysql = NULL;
        connectionRAII mysqlcon(&mysql, connPool);
        while (taskQueue_.empty()) {
            sqlStatement = taskQueue_.back();
            // ÷¥––sql√¸¡Ó
            if (mysql_query(mysql, sqlStatement.c_str())) {
                cerr << "mysql_query() failed: " << mysql_error(mysql) << endl;

                return false;
            }
            taskQueue_.pop_back();
        }
        return true;
    }

private:
    size_t maxQueueSize_;
    std::chrono::milliseconds interval_;
    bool stopFlag_;
    std::vector<std::string> taskQueue_;
    std::mutex mutex_;
    std::thread workerThread_;
    connection_pool* connPool;

};



#endif