#include "logger.h"
#include <cstdio>
#include <stdexcept>
#include <chrono>
#include <unistd.h>

namespace adachi::tool {
    namespace {
        thread_local bool g_async_logging_appending = false;
    }

    AsyncLogging::AsyncLogging(const std::string& logpath) 
        : running_(true)
        , current_buffer_(std::make_unique<char[]>(kBackendbuffersize))
        , current_buffer_len_(0)
        , next_buffer_(std::make_unique<char[]>(kBackendbuffersize))
        , logpath_(logpath)
        , backendthread_()
    {
    }
    AsyncLogging::AsyncLogging(const char* logpath) 
        : running_(true)
        , current_buffer_(std::make_unique<char[]>(kBackendbuffersize))
        , current_buffer_len_(0)
        , next_buffer_(std::make_unique<char[]>(kBackendbuffersize))
        , logpath_(logpath)
        , backendthread_()
    {
    }

    void AsyncLogging::StartBackendThread() {
        backendthread_ = std::thread(&AsyncLogging::BackendThreadFunc, this);
    }

    std::unique_ptr<char[]> AsyncLogging::TakeBufferLocked() {
        if (!empty_buffers_.empty()) {
            std::unique_ptr<char[]> buffer = std::move(empty_buffers_.back());
            empty_buffers_.pop_back();
            return buffer;
        }
        return std::make_unique<char[]>(kBackendbuffersize);
    }

    void AsyncLogging::RecycleBufferLocked(std::unique_ptr<char[]> buffer) {
        if (!buffer) {
            return;
        }
        if (empty_buffers_.size() < 2) {
            empty_buffers_.push_back(std::move(buffer));
        }
    }

    AsyncLogging::~AsyncLogging() {
        std::vector<PendingBuffer> remaining_work_list;
        std::unique_ptr<char[]> remaining_current_buffer;
        unsigned int remaining_current_len = 0;

        {
            std::lock_guard<std::recursive_mutex> lock(mtx_);
            running_.store(false);
        }
        {
            wake_requested_.store(true, std::memory_order_release);
        }
        wake_cv_.notify_one();
        if (backendthread_.joinable()) backendthread_.join();

        {
            std::lock_guard<std::recursive_mutex> lock(mtx_);
            remaining_work_list.swap(work_list_);
            remaining_current_buffer = std::move(current_buffer_);
            remaining_current_len = current_buffer_len_;
            current_buffer_len_ = 0;
            next_buffer_.reset();
            empty_buffers_.clear();
        }

        FILE* fp = std::fopen(logpath_.c_str(), "a");
        if (fp) {
            for (auto& it : remaining_work_list) {
                std::fwrite(it.data.get(), 1, it.len, fp);
            }

            if (remaining_current_buffer && remaining_current_len > 0) {
                std::fwrite(remaining_current_buffer.get(), 1, remaining_current_len, fp);
                std::fflush(fp);
            }
            std::fclose(fp);
        }
    }

    void AsyncLogging::BackendThreadFunc() {
        FILE* fp = std::fopen(logpath_.c_str(), "a");

        while (true) {
            std::vector<PendingBuffer> temp_work_list;
            std::vector<std::unique_ptr<char[]>> reusable_buffers;

            {
                std::unique_lock<std::mutex> wake_lock(wake_mtx_);
                wake_cv_.wait_for(wake_lock, std::chrono::seconds(1), [this]() {
                    return wake_requested_.load(std::memory_order_acquire) || !running_.load();
                });
                wake_requested_.store(false, std::memory_order_release);
            }

            {
                std::lock_guard<std::recursive_mutex> lock(mtx_);

                if (current_buffer_ && current_buffer_len_ > 0) {
                    work_list_.push_back(PendingBuffer{std::move(current_buffer_), current_buffer_len_});
                    current_buffer_len_ = 0;
                    if (next_buffer_) {
                        current_buffer_ = std::move(next_buffer_);
                    }
                    else {
                        current_buffer_ = TakeBufferLocked();
                    }
                }

                temp_work_list.swap(work_list_);

                if (!running_.load() && temp_work_list.empty()) {
                    break;
                }
            } 
            
            // 写盘，不要用锁
            if (fp) {
                for (auto& it : temp_work_list) {
                    std::fwrite(it.data.get(), 1, it.len, fp);
                }
                std::fflush(fp); // 强制刷入操作系统 page cache
            }

            while (!temp_work_list.empty()) {
                if (reusable_buffers.size() < 2 && temp_work_list.back().data) {
                    reusable_buffers.push_back(std::move(temp_work_list.back().data));
                }
                temp_work_list.pop_back();
            }
            {
                std::lock_guard<std::recursive_mutex> lock(mtx_);
                while (!reusable_buffers.empty()) {
                    RecycleBufferLocked(std::move(reusable_buffers.back()));
                    reusable_buffers.pop_back();
                }

                if (!next_buffer_) {
                    next_buffer_ = TakeBufferLocked();
                }
            }
        }

        if (fp) std::fclose(fp);
    }

    AsyncLogging& AsyncLogging::Instance() {
        static AsyncLogging instance("pid" + std::to_string(getpid()) + ".log");
        static std::once_flag start_once;
        std::call_once(start_once, &AsyncLogging::StartBackendThread, std::ref(instance));
        return instance;
    }

    void AsyncLogging::Append(const char* buf, unsigned int len) {
        if (!running_.load()) {
            return;
        }

        if (g_async_logging_appending) {
            ssize_t written = ::write(STDERR_FILENO, buf, len);
            (void)written;
            return;
        }

        struct AppendGuard {
            AppendGuard() {
                g_async_logging_appending = true;
            }

            ~AppendGuard() {
                g_async_logging_appending = false;
            }
        } append_guard;

        bool check = false;
        bool should_drop = false;
        {
            std::lock_guard<std::recursive_mutex> lock(mtx_);

            if (!running_.load()) {
                return;
            }

            if (!current_buffer_) {
                if (next_buffer_) {
                    current_buffer_ = std::move(next_buffer_);
                }
                else {
                    current_buffer_ = TakeBufferLocked();
                }
            }

            if (kBackendbuffersize - current_buffer_len_ < len) {
                if (work_list_.size() >= kBackendbufferlimit) {
                    // 队列已满时只丢当前新日志，保留 current_buffer_ 中已经积累的旧日志。
                    should_drop = true;
                    check = true;
                }
                else {
                    work_list_.push_back(PendingBuffer{std::move(current_buffer_), current_buffer_len_});
                    current_buffer_len_ = 0;
                    check = true;
                    if (next_buffer_) {
                        current_buffer_ = std::move(next_buffer_);
                    }
                    else {
                        current_buffer_ = TakeBufferLocked();
                    }
                }
            }

            if (!should_drop) {
                memcpy(current_buffer_.get() + current_buffer_len_, buf, len);
                current_buffer_len_ += len;
            }

            if (!next_buffer_) {
                next_buffer_ = TakeBufferLocked();
            }
        }

        if (check) {
            wake_requested_.store(true, std::memory_order_release);
            wake_cv_.notify_one();
        }
    }

    Logger::Logger(std::string_view level, const char* file, int line, const char* func, AsyncLogging* backendobj) 
        : used_(0)
        , backendobj_(backendobj)
    {
        (*this) << "[" << level << "]" 
        << "[" << (__DATE__) << "]" 
        << "[" << (__TIME__) << "]" 
        << "[" << file << "]" 
        << "[" << line << "]" 
        << "[" << func << "]";
    }

    Logger::~Logger() {
        if (LessSize() > 0) {
            buf_[used_++] = '\n';
        }
        backendobj_->Append(buf_, used_);
    }

    unsigned int Logger::Size() const {
        return used_;
    }
    unsigned int Logger::LessSize() const {
        return kLoggerbuffersize - used_;
    }

    Logger& Logger::operator<<(std::string_view msg) {
        if (size_t res = LessSize()) {
            unsigned int loglen = static_cast<unsigned int>(std::min(msg.size(), res));
            memcpy(buf_ + used_, msg.data(), loglen);
            used_ += loglen;
        }
        return *this;
    }
    Logger& Logger::operator<<(const char* msg) {
        if (msg == nullptr) {
            return operator<<("(null)"); // 安全兜底
        }
        // 非空指针，直接转交给 string_view 处理，零成本抽象
        return operator<<(std::string_view(msg)); 
    }
    Logger& Logger::operator<<(unsigned short msg) { return Format(msg);}
    Logger& Logger::operator<<(short msg) { return Format(msg);}
    Logger& Logger::operator<<(unsigned int msg) { return Format(msg);}
    Logger& Logger::operator<<(int msg) { return Format(msg);}
    Logger& Logger::operator<<(unsigned long long msg) { return Format(msg);}
    Logger& Logger::operator<<(long long msg) { return Format(msg);}
    Logger& Logger::operator<<(float msg) { return Format(msg);}
    Logger& Logger::operator<<(double msg) { return Format(msg);}
    Logger& Logger::operator<<(long double msg) { return Format(msg);}
    Logger& Logger::operator<<(ssize_t msg) { return Format(msg);}
    Logger& Logger::operator<<(bool msg) { return msg ? operator<<("true") : operator<<("false");}
}