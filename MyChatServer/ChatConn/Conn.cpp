
// ======= ģ�������� =======
class ChatConn : public std::enable_shared_from_this<ChatConn> {
public:
    ChatConn() : is_processing(false) {}

    // ����Ϣ��Ӳ������ύ��������
    void push_message(const std::string& msg, ThreadPool& pool) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            msg_queue.push(msg);
            if (is_processing) return;  // ��ǰ���������ڴ���ֱ�ӷ���
            is_processing = true;       // ���ô����־λ
        }

        submit_next(pool);  // �ύ��������
    }

private:
    std::mutex mtx;
    std::queue<std::string> msg_queue; // ��Ϣ����
    bool is_processing;                // �Ƿ����ڴ�����

    // �ύ��һ����������ֻ�����̳߳��̵߳��ã�
    void submit_next(ThreadPool& pool) {
        auto self = shared_from_this();  // ���� this ��ǰ����

        pool.submit([self, &pool]() {
            std::string msg;

            {
                std::lock_guard<std::mutex> lock(self->mtx);
                if (self->msg_queue.empty()) {
                    self->is_processing = false;  // ����Ϣ���������״̬
                    return;
                }
                msg = std::move(self->msg_queue.front());
                self->msg_queue.pop();
            }

            self->handle_message(msg);       // ������Ϣ
            self->submit_next(pool);         // ����������һ��
            });
    }

    // ʵ�ʴ�����Ϣ�߼���ʾ����
    void handle_message(const std::string& msg) {
        std::cout << "������Ϣ: " << msg << "���߳�ID: "
            << std::this_thread::get_id() << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));  // ģ�⴦���ʱ
    }
};



#endif