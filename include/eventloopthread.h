#ifndef EVENTLOOPTHREAD_H
#define EVENTLOOPTHREAD_H
#include "noncopyable.h"
#include <thread>
#include <mutex>
#include <functional>
#include <condition_variable>
#include "timer/timer.h"

namespace adachi::tool {
    class EventLoop;
}

namespace adachi::io {
    class TimerOpt;
}

namespace adachi::tool {
    /// 其中timeropt参数为心跳机制，{时间轮片数，{每tick秒，每tick纳秒}}，将时间轮片数设定为0表示关闭心跳机制
    class EventLoopThread : NonCopyAble {
    public:
        EventLoopThread(const adachi::io::TimerOpt& timeropt = {0, {0, 0}}
        , std::function<void(EventLoopThread*)> prework = [](EventLoopThread*){}
        , int maxevents = 1024);
        EventLoop* Start();
        ~EventLoopThread();
        void Join();
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