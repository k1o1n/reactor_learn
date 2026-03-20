#include "eventloopthread.h"
#include "eventloop.h"
namespace adachi::tool {
    EventLoopThread::EventLoopThread(std::function<void(EventLoopThread*)> prework, int maxevents) 
        : prework_(prework)
        , loop_(nullptr)
        , start_([this, maxevents](){
            EventLoop loop(maxevents);
            prework_(this);

            {
                std::lock_guard<std::mutex> lock(this->mtx_);
                this->loop_ = &loop;
                this->cv_.notify_one();
            }

            loop.Loop();
            std::lock_guard<std::mutex> lock(this->mtx_);
            this->loop_ = nullptr;
        })
    {

    }
    EventLoopThread::~EventLoopThread() {
        exiting_ = false;
        if (loop_ != nullptr) {
            loop_->StopLoop();
        }
        
        if (thread_.joinable()) thread_.join();
    }
    EventLoop* EventLoopThread::Start() {
        thread_ = std::thread(start_);
        EventLoop* loop = nullptr;
        {
            std::unique_lock<std::mutex> lock(mtx_);
            cv_.wait(lock, [this](){return this->loop_ != nullptr;});
            loop = this->loop_;
        }
        return loop;
    }
}