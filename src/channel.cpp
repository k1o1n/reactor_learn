#include "channel.h"
#include <functional>
#include "eventloop.h"
#include <iostream>
namespace adachi::io {
    Channel::Channel(adachi::tool::EventLoop* loop, int fd) 
        : fd_(fd)
        , events_(0)
        , active_events_(0)
        , owner_(nullptr)
        , read_callback_([](){})
        , write_callback_([](){})
        , error_callback_([](){})
        , close_callback_([](){})
    {
        if (!loop->AddChannel(this)) {
            std::cout << "[info] Channel constrution failed" << std::endl;
            return;
        }
        owner_ = loop;
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

    const int Channel::Fd() {
        return fd_;
    }

    const int Channel::Events() {
        return events_;
    }

    Channel* Channel::SetActive(const int& status) {
        events_ = status;
        if (!owner_) {
            std::cout << "[info] SetActive failed: events set successfully but owner eventloop not found" << std::endl;
            return this;
        }
        if (!owner_->UpdateChannel(this)) {
            std::cout << "[info] SetActive failed" << std::endl;
        }
        return this;
    }

    Channel* Channel::SetActiveEvents(const int& status) {
        active_events_ = status;
        return this;
    }

    void Channel::Handle() {
        std::shared_ptr<void> guard; // 指向父类延长生命周期
        if (tied_) {
            guard = save_life_ptr_.lock();
            if (!guard) return;
        }

        if (active_events_ & Channel::kRead) {
            read_callback_();
        }
        if (active_events_ & Channel::kWrite) {
            write_callback_();
        }
        if (active_events_ & Channel::kError) {
            error_callback_();
        }
        if (active_events_ & Channel::kClose) {
            close_callback_();
        }
    }

    void Channel::Tie(const std::shared_ptr<void>& obj) {
        save_life_ptr_ = obj;
        tied_ = true;
    }

    void Channel::RemoveFromLoop() {
        if (!owner_) owner_->DeleteChannel(this);
    }

    const adachi::tool::EventLoop* Channel::GetOwner() const {
        return static_cast<const adachi::tool::EventLoop*>(owner_);
    }
}
