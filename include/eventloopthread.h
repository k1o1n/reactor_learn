#ifndef EVENTLOOPTHREAD_H
#define EVENTLOOPTHREAD_H
#include "noncopyable.h"
#include <thread>
#include <mutex>
#include <functional>
#include <condition_variable>
namespace adachi::tool {
    class EventLoop;
}
namespace adachi::tool {
    class EventLoopThread : NonCopyAble {
    public:
        EventLoopThread(std::function<void(EventLoopThread*)> prework = [](EventLoopThread*){}, int maxevents = 1024);
        EventLoop* Start();
        ~EventLoopThread();
    private:
        std::function<void(EventLoopThread*)> prework_;
        std::function<void()> start_;
        EventLoop* loop_;
        std::thread thread_;
        std::mutex mtx_;
        std::condition_variable cv_;
        bool exiting_ = false;
    };
}
#endif 