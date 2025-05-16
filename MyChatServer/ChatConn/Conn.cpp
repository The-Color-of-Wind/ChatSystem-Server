
// ======= 模拟连接类 =======
class ChatConn : public std::enable_shared_from_this<ChatConn> {
public:
    ChatConn() : is_processing(false) {}

    // 新消息入队并尝试提交处理任务
    void push_message(const std::string& msg, ThreadPool& pool) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            msg_queue.push(msg);
            if (is_processing) return;  // 当前已有任务在处理，直接返回
            is_processing = true;       // 设置处理标志位
        }

        submit_next(pool);  // 提交处理任务
    }

private:
    std::mutex mtx;
    std::queue<std::string> msg_queue; // 消息队列
    bool is_processing;                // 是否正在处理中

    // 提交下一个处理任务（只能由线程池线程调用）
    void submit_next(ThreadPool& pool) {
        auto self = shared_from_this();  // 避免 this 提前析构

        pool.submit([self, &pool]() {
            std::string msg;

            {
                std::lock_guard<std::mutex> lock(self->mtx);
                if (self->msg_queue.empty()) {
                    self->is_processing = false;  // 无消息则结束处理状态
                    return;
                }
                msg = std::move(self->msg_queue.front());
                self->msg_queue.pop();
            }

            self->handle_message(msg);       // 处理消息
            self->submit_next(pool);         // 继续处理下一条
            });
    }

    // 实际处理消息逻辑（示例）
    void handle_message(const std::string& msg) {
        std::cout << "处理消息: " << msg << "，线程ID: "
            << std::this_thread::get_id() << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));  // 模拟处理耗时
    }
};



#endif