#include "threadpool.h"
#include <thread>
#include <algorithm>
namespace adachi::tool {
    ThreadPool::ThreadPool(unsigned int siz) 
        : stop_flag_(false)
    {
        siz = max(siz, 1u);
        for (unsigned int idx = 0; idx < siz; ++idx) {
            work_threads_.emplace(&Loop, this);
        }
    }
    ThreadPool::~ThreadPool() {
        stop_flag_.store(true);

        cv_.notify_all();
        for (auto it : work_threads_) {
            if (it.joinable()) it.join();
        }
    }

    ThreadPool& ThreadPool::GetInstance() {
        static ThreadPool instance;
        return instance;
    }

    void Loop() {
        while (true) {
            std::unique_lock<std::mutex> lock(mtx_);
            cv_.wait(lock, [](){
                return !mission_.empty() || stop_flag_.load();
            }); 

            if (stop_flag_.load()) return;
            auto task = std::move(mission_.front());
            mission_.pop();
            lock.unlock();

            task();
        }
    }
}