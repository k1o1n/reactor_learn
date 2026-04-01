#ifndef EVENTLOOP_H
#define EVENTLOOP_H
#include <functional>
#include "noncopyable.h"
#include "epoll.h"
#include <memory>
#include <atomic>
#include <mutex>
#include <vector>
#include <thread>
#include "timer/timer.h"

namespace adachi::io {
    class Channel;
}

namespace adachi::io {
    class Channel;
    class TimerOpt;
}

namespace adachi::tool {
    /// 其中timeropt参数为心跳机制，{时间轮片数，{每tick秒，每tick纳秒}}，将时间轮片数设定为0表示关闭心跳机制
    class EventLoop : adachi::tool::NonCopyAble {
    public:
        EventLoop(const adachi::io::TimerOpt& timeropt = {0, {0, 0}}, int maxevents = 1024);
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

        bool IsInThread() const;

        /// 是否开启心跳机制
        bool IsHeartBeat() const;
    private:
        adachi::io::Epoll epoll_;
        std::atomic<bool> quit_;    
        std::atomic<bool> looping_; 
        std::mutex mtx_;
        adachi::io::Channel wakeupchannel_;
        std::vector<std::function<void()>> missions_;
        std::thread::id tid_;
    public:
        std::unique_ptr<adachi::io::Timer> timerptr_;
    };
}
#endif // EVENTLOOP_H