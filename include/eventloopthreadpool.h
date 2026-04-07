#ifndef EVENTLOOPTHREADPOOL_H
#define EVENTLOOPTHREADPOOL_H
#include <functional>
#include "noncopyable.h"
#include <vector>
#include <memory>
#include "timer/timer.h"
#include <mutex>

namespace adachi::tool {
    class EventLoopThread;
    class EventLoop;
}

namespace adachi::tool {
    /// 初始化传入参数中的prework为EventLoopThread启动前会先做的操作，可以选择不做任何事情
    /// 其中timeropt参数为心跳机制，{时间轮片数，{每tick秒，每tick纳秒}}，将时间轮片数设定为0表示关闭心跳机制
    class EventLoopThreadPool : NonCopyAble {
    public:
        EventLoopThreadPool(const adachi::io::TimerOpt& timeropt = {0, {0, 0}}
        , std::function<void(EventLoopThread*)> prework = [](EventLoopThread*){}
        , int maxevents = 1024);
        unsigned int Size() const;
        unsigned int MaxSize() const;
        EventLoop* GetOneThread();
        std::vector<EventLoop*> GetAllThread();
        void Start();
        bool IsRunning();
        void SetSize(unsigned int num);
        void Close();
    private:
        int pos_ = 0;
        int maxevents_;
        bool running_ = false;
        std::vector<std::unique_ptr<EventLoopThread>> threads_;
        std::vector<EventLoop*> oper_threads_;
        unsigned int num_ = 1;
        unsigned int maxnum_ = 1;
        std::function<void(EventLoopThread*)> prework_;

        adachi::io::TimerOpt timeropt_;

        std::mutex mtx_;
    };
}
#endif // EVENTLOOPTHREADPOOL_H