#include "eventloopthreadpool.h"
#include "eventloopthread.h"
#include <algorithm>

namespace adachi::tool {
    EventLoopThreadPool::EventLoopThreadPool(std::function<void(EventLoopThread*)> prework, int maxevents) 
        : maxevents_(maxevents)
        , prework_(prework)
    {

    }
    unsigned int EventLoopThreadPool::Size() const {
        return num_;
    }
    unsigned int EventLoopThreadPool::MaxSize() const {
        return maxnum_;
    }
    EventLoop* EventLoopThreadPool::GetOneThread() {
        if (!running_) return nullptr;
        pos_ %= num_;
        EventLoop* ret = oper_threads_[pos_];
        pos_ = (pos_ + 1) % num_;
        return ret;
    }
    std::vector<EventLoop*> EventLoopThreadPool::GetAllThread() {
        return oper_threads_;
    }
    void EventLoopThreadPool::Start() {
        maxnum_ = num_;
        threads_.resize(num_);
        oper_threads_.resize(num_);
        for (unsigned int idx = 0; idx < num_; ++idx) {
            threads_[idx] = std::make_unique<EventLoopThread>(prework_, maxevents_);
            oper_threads_[idx] = threads_[idx]->Start();
        }
        running_ = true;
    }
    bool EventLoopThreadPool::IsRunning() {
        return running_;
    }
    void EventLoopThreadPool::SetSize(unsigned int num) {
        if (running_) {
            num_ = std::max(std::min(num, maxnum_), 1u);
        }
        else {
            num_ = std::max(1u, num);
        }
    }
}