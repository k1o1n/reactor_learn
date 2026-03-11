#ifndef EVENTLOOP_H
#define EVENTLOOP_H
#include "functional"
#include "noncopyable.h"
#include "epoll.h"
#include "memory"
#include <atomic>
#include <mutex>
// namespace adachi::io {
//     class Epoll;
// }
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
    private:
        adachi::io::Epoll epoll_;
        std::atomic<bool> quit_;    // 后续改成自旋锁结构
        std::atomic<bool> looping_; // 后续改成自旋锁
        std::mutex mtx_;
    };
}
#endif // EVENTLOOP_H