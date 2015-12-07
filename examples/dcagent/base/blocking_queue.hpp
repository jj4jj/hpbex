#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
template <class T>
class blocking_queue { //
	std::queue<T>				queue;
	std::mutex					mtx;
	std::condition_variable		cond;
public:
	void	push(const T & t){
		std::lock_guard<std::mutex>		lock(mtx);
		queue.push(t);
		cond.notify_one();
	}
	void	pop(T & t){
		std::unique_lock<std::mutex>		lock(mtx);
		cond.wait(lock, [&]{return !queue.empty(); });
		t = queue.front();
		queue.pop();
	}
	bool	pop(T & t, int timeout_ms){	//return true: timeout
		std::unique_lock<std::mutex>		lock(mtx);
		cond.wait_for(lock,
			std::chrono::milliseconds(timeout_ms), [&]{return !queue.empty(); });
		if (queue.empty()){
			return true;
		}
		t = queue.front();
		queue.pop();
		return false;
	}
	bool	empty() const { return queue.empty(); }
	size_t  size() const { return queue.size(); }
};