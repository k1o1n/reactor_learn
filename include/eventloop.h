#ifndef EVENTLOOP_H
#define EVENTLOOP_H
#include <functional>
#include "noncopyable.h"
#include "epoll.h"
#include "memory"
#include <atomic>
#include <mutex>
#include <vector>
#include <thread>
namespace adachi::io {
    class Channel;
}
namespace adachi::io {
    class Channel;
}
namespace adachi::tool {
    class EventLoop : adachi::tool::NonCopyAble {
    public:
        EventLoop(int maxevents = 1024);
        ~EventLoop();

        void StopLoop();
        void Loop();

        bool Status() {
            return quit_.load();
        }
        bool AddChannel(adachi::io::Channel* channel);
        bool UpdateChannel(adachi::io::Channel* channel);
        bool DeleteChannel(adachi::io::Channel* channel);
        void WakeUp();

        void DoCrossThreadMission();

        void Submit(const std::function<void()>& cb);

        bool IsInThread();
    private:
        adachi::io::Epoll epoll_;
        std::atomic<bool> quit_;    
        std::atomic<bool> looping_; 
        std::mutex mtx_;
        adachi::io::Channel wakeupchannel_;
        std::vector<std::function<void()>> missions_;
        std::thread::id tid_;
    };
}
#endif // EVENTLOOP_H