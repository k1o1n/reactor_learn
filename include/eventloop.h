#ifndef EVENTLOOP_H
#define EVENTLOOP_H
#include "functional"
namespace adachi::tool {
    class EventLoop {
    public:
        EventLoop();

        void SetEventLoop(std::function<void()> loop);
        std::function<void()> loop_;
    private:
    };
}
#endif // EVENTLOOP_H