#ifndef EVENTLOOPTHREADPOOL_H
#define EVENTLOOPTHREADPOOL_H
#include <functional>
#include "noncopyable.h"
#include <vector>
#include <memory>
namespace adachi::tool {
    class EventLoopThread;
    class EventLoop;
}
namespace adachi::tool {
    /// 初始化传入参数中的prework为EventLoopThread启动前会先做的操作，可以选择不做任何事情
    class EventLoopThreadPool : NonCopyAble {
    public:
        EventLoopThreadPool(std::function<void(EventLoopThread*)> prework = [](EventLoopThread*){}, int maxevents = 1024);
        unsigned int Size() const;
        unsigned int MaxSize() const;
        EventLoop* GetOneThread();
        std::vector<EventLoop*> GetAllThread();
        void Start();
        bool IsRunning();
        void SetSize(unsigned int num);
    private:
        int pos_ = 0;
        int maxevents_;
        bool running_ = false;
        std::vector<std::unique_ptr<EventLoopThread>> threads_;
        std::vector<EventLoop*> oper_threads_;
        unsigned int num_ = 1;
        unsigned int maxnum_ = 1;
        std::function<void(EventLoopThread*)> prework_;
    };
}
#endif // EVENTLOOPTHREADPOOL_H