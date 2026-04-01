#ifndef TIMER_H
#define TIMER_H
#include <unordered_set>
#include <memory>
#include <vector>

namespace adachi::io {
    class Channel;
}

namespace adachi::tool {
    class EventLoop;
    class HeartBeatObj;
}

namespace adachi::network {
    class TcpConnection;
}

namespace adachi::io {
    /// 计时器，在EventLoop上进行挂载，实现时间轮心跳机制
    class Timer {
    public:
        /// owner：事件循环归属
        /// heartbeat_num：时间轮的总片数
        /// timeslice：时间片的单位时间，以及首次检测时间
        Timer(adachi::tool::EventLoop* owner, int heartbeat_num, timespec timeslice);
        ~Timer();
        /// 严格执行eventloop的非跨线程思想，插入将会提交到对应的线程进行处理
        void Insert(std::weak_ptr<adachi::tool::HeartBeatObj>);
    private:
        void Tick();
        void InsertInThread(std::shared_ptr<adachi::tool::HeartBeatObj>);
        std::shared_ptr<Channel> timer_channel_;
        std::vector<std::vector<std::shared_ptr<adachi::tool::HeartBeatObj>>> timerwheel_st_; 
        int ptr_;
    };

    class TimerOpt {
    public:
        TimerOpt() = default;
        ~TimerOpt() = default;
        TimerOpt(int heartbeat_num, timespec timeslice);
        int heartbeat_num_;
        timespec timeslice_;
    };
}

#endif 