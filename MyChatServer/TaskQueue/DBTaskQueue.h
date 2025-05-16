#ifndef DBTASKQUEUE_H
#define DBTASKQUEUE_H

#include <queue>
#include <mutex>
#include <vector>
#include <string>

class DBTaskQueue {
public:
    void push(const std::string& cmd) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(cmd);
        }
        
        cv.notify_one();
    }

    std::vector<std::string> popBatch(size_t max_batch) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv.wait_for(lock, std::chrono::milliseconds(10), [&] {
            return !queue_.empty();
            });

        std::vector<std::string> batch;
        while (!queue_.empty() && batch.size() < max_batch) {
            batch.push_back(queue_.front());
            queue_.pop();
        }
        return batch;
    }

    size_t size() {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

private:
    std::queue<std::string> queue_;
    std::mutex mutex_;

    std::condition_variable cv;
};


#endif