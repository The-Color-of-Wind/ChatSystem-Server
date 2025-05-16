#ifndef LOCKFREEQUEUE_H
#define LOCKFREEQUEUE_H

#include <atomic>
#include <memory>
#include <cassert>

using namespace std;

template<typename T, size_t Capacity = 20000>
class LockFreeQueue {
public:
	LockFreeQueue() :head(0), tail(0){
		static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of 2");		// 容量位2的幂次，后续可以用 & 代替取模
		buffer = new T*[Capacity];
	}
	~LockFreeQueue() {
		delete[] buffer;
	}

	bool enqueue(T* task) {
		size_t current_tail = tail.load(memory_order_relaxed);	// 原子的读，放宽执行顺序
		size_t next_tail = (current_tail + 1) & (Capacity - 1);

		if (next_tail == head.load(memory_order_acquire)) {
			cout << "lock free queue fill" << endl;
			return false;
		}

		if (tail.compare_exchange_strong(current_tail, next_tail)) {
			buffer[current_tail] = move(task);
			return true;
		}
		return false;	// 竞争失败，重新竞争
	}

	T* dequeue() {
		size_t current_head = head.load(memory_order_relaxed);

		if (current_head == tail.load(memory_order_acquire)) {
			//cout << "queue empty" << endl;
			return nullptr;
		}

		T* task = buffer[current_head];
		if (head.compare_exchange_strong(current_head, (current_head + 1) & (Capacity - 1))) {
			return task;
		}
		return nullptr;
	}

private:
	alignas(64) atomic<size_t> head;
	alignas(64) atomic<size_t> tail;
	T** buffer;
};



#endif