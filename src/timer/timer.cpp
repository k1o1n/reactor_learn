#include "timer/timer.h"
#include "timer/heartbeatobj.h"
#include "tcpconnection.h"
#include "channel.h"
#include "eventloop.h"
#include <sys/timerfd.h>
#include <stdexcept>
#include <cerrno>
#include <string>

namespace adachi::io {
    Timer::Timer(adachi::tool::EventLoop* owner, int heartbeat_num, timespec timeslice) 
        : ptr_(0)
    {
        timerwheel_st_.resize(heartbeat_num);

        int timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
        if (timerfd < 0) {
            throw std::runtime_error("Timer::Timer failed: timerfd is a negative number");
        }

        itimerspec _ = {timeslice, timeslice};
        timerfd_settime(timerfd, 0, &_, nullptr);
        timer_channel_ = std::make_shared<Channel>(owner, timerfd);
        
        timer_channel_->SetReadCallback([timerfd, this]() {
            uint64_t times;
            int n = read(timerfd, &times, sizeof(times));

            if (n < 0) {
                int saveerrno = errno;
                if (saveerrno == EAGAIN || saveerrno == EWOULDBLOCK || saveerrno == EINTR) return;
                throw std::runtime_error("Timer ReadCallback error: " + std::string(strerror(saveerrno)));
            }

            if (n != 8) {
                throw std::runtime_error("Timer ReadCallback error: expected 8 but found " + std::to_string(n));
            }
            
            for (uint64_t idx = 0; idx < times; ++idx) this->Tick();
        });
        timer_channel_->SetActive(adachi::io::Channel::kRead);
    }

    Timer::~Timer() {
        if (timer_channel_->Fd()) {
            timer_channel_->RemoveFromLoop();
            close(timer_channel_->Fd());
        }
    }

    void Timer::Tick() {
        timerwheel_st_[ptr_].clear();
        ptr_ = (ptr_ + 1) % timerwheel_st_.size();
    } 

    void Timer::Insert(std::weak_ptr<adachi::tool::HeartBeatObj> tcp_weak_ptr) {
        if (auto ptr = tcp_weak_ptr.lock()) {
            if (timer_channel_->owner_->IsInThread()) {
                InsertInThread(ptr);
            }
            else {
                timer_channel_->owner_->Submit([tcp_weak_ptr, this]() {
                    if (auto ptr = tcp_weak_ptr.lock()) {
                        InsertInThread(ptr);
                    }
                });
            } 
        }   
    }

    void Timer::InsertInThread(std::shared_ptr<adachi::tool::HeartBeatObj> ptr) {
        int insert_pos = (ptr_ - 1 + timerwheel_st_.size()) % timerwheel_st_.size();
        if (ptr->last_insert_pos_ != insert_pos) {
            ptr->last_insert_pos_ = insert_pos;
            timerwheel_st_[insert_pos].emplace_back(ptr);
        }
    }

    TimerOpt::TimerOpt(int heartbeat_num, timespec timeslice)
        : heartbeat_num_(heartbeat_num)
        , timeslice_(timeslice)
    {} 

}