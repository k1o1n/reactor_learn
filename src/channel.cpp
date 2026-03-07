#include "channel.h"
#include <functional>
class EventLoop;
namespace adachi::io {
    Channel::Channel(int fd) 
        : fd_(fd)
        , events_(0)
        , read_callback_([](){})
        , write_callback_([](){})
        , error_callback_([](){})
        , close_callback_([](){})
    {

    }

    void Channel::SetReadCallback(const callback& cb) {
        read_callback_ = cb;
    }

    void Channel::SetWriteCallback(const callback& cb) {
        write_callback_ = cb;
    }

    void Channel::SetErrorCallback(const callback& cb) {
        error_callback_ = cb;
    }

    void Channel::SetCloseCallback(const callback& cb) {
        close_callback_ = cb;
    }

    void Channel::SetActive(const int status) {
        events_ = status;
    }

    const int Channel::Fd() {
        return fd_;
    }

    const int Channel::Events() {
        return events_;
    }

    Channel* Channel::SetActiveEvents(int active_events) {
        active_events_ = active_events;
        return this;
    }
}
