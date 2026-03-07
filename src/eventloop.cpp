#include "functional"
#include "eventloop.h"
namespace adachi::tool {
    EventLoop::EventLoop() 
        : loop_([](){})
    {
        
    }

    void EventLoop::SetEventLoop(std::function<void()> loop) {
        loop_ = loop;
    }
}
