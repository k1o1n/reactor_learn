#include "logger.h"
#include <cstdio>
#include <stdexcept>
#include <chrono>
#include <unistd.h>

namespace adachi::tool {
    AsyncLogging::AsyncLogging(const std::string& logpath) 
        : running_(true)
        , current_buffer_(nullptr)
        , next_buffer_(nullptr)
        , len_(0)
        , logpath_(logpath)
        , backendthread_(&AsyncLogging::BackendThreadFunc, this)
    {
    }
    AsyncLogging::AsyncLogging(const char* logpath) 
        : running_(true)
        , current_buffer_(nullptr)
        , next_buffer_(nullptr)
        , len_(0)
        , logpath_(logpath)
        , backendthread_(&AsyncLogging::BackendThreadFunc, this)
    {
    }
    AsyncLogging::~AsyncLogging() {
        running_.store(false);
        cv_.notify_one();
        if (backendthread_.joinable()) backendthread_.join();

        FILE* fp = std::fopen(logpath_.c_str(), "a");
        if (fp) {
            if (!work_list_.empty()) {
                for (auto& it : work_list_) {
                    // ::fwrite_unlocked 性能最高，如果没有这个平台函数，用 std::fwrite
                    std::fwrite(it.first, 1, it.second, fp);
                }
                std::fflush(fp); // 强制刷入操作系统 page cache
            }

            if (len_ > 0 && current_buffer_ != nullptr) {
                std::fwrite(current_buffer_, 1, len_, fp);
                std::fflush(fp);
            }
            std::fclose(fp);
        }
        if (current_buffer_ != nullptr) delete[] current_buffer_;
        if (next_buffer_ != nullptr) delete[] next_buffer_;
    }

    void AsyncLogging::BackendThreadFunc() {
        std::vector<char*> ptr;

        FILE* fp = std::fopen(logpath_.c_str(), "a");

        while (running_.load()) {
            std::vector<std::pair<char*, unsigned int>> temp_work_list;

            {
                std::unique_lock<std::mutex> lock(mtx_);
                cv_.wait_for(lock, std::chrono::seconds(1), [this]() {return !work_list_.empty() || !running_.load();});
                temp_work_list.swap(work_list_);

                // 定时把未写满的当前缓冲也刷到待写队列，避免长时间运行时日志文件看起来为空。
                if (current_buffer_ != nullptr && len_ > 0) {
                    temp_work_list.emplace_back(current_buffer_, len_);
                    current_buffer_ = nullptr;
                    len_ = 0;
                    std::swap(current_buffer_, next_buffer_);
                }
            } 
            
            // 写盘，不要用锁
            if (fp) {
                for (auto& it : temp_work_list) {
                    // ::fwrite_unlocked 性能最高，如果没有这个平台函数，用 std::fwrite
                    std::fwrite(it.first, 1, it.second, fp);
                }
                std::fflush(fp); // 强制刷入操作系统 page cache
            }

            while (ptr.size() < 2 && !temp_work_list.empty()) {
                ptr.push_back(temp_work_list.back().first);
                temp_work_list.pop_back();
            }
            while (!temp_work_list.empty()) {
                delete[] temp_work_list.back().first;
                temp_work_list.pop_back();
            }

            std::lock_guard<std::mutex> lock(mtx_);
            if (current_buffer_ == nullptr && !ptr.empty()) {
                current_buffer_ = ptr.back();
                ptr.pop_back();
            }
            if (next_buffer_ == nullptr && !ptr.empty()) {
                next_buffer_ = ptr.back();
                ptr.pop_back();
            }
        }

        if (fp) std::fclose(fp);
        
        while (!ptr.empty()) {
            delete[] ptr.back();
            ptr.pop_back();
        }
    }

    AsyncLogging& AsyncLogging::Instance() {
        static AsyncLogging instance("pid" + std::to_string(getpid()) + ".log");
        return instance;
    }

    void AsyncLogging::Append(const char* buf, unsigned int len) {
        bool check = false;
        {
            std::lock_guard<std::mutex> lock(mtx_);
            if (current_buffer_ == nullptr) std::swap(current_buffer_, next_buffer_);

            if (current_buffer_ == nullptr) current_buffer_ = new char[kBackendbuffersize];

            if (kBackendbuffersize - len_ < len) {
                work_list_.emplace_back(current_buffer_, len_);
                check = true;
                current_buffer_ = nullptr;
                len_ = 0;
                std::swap(current_buffer_, next_buffer_);
                if (current_buffer_ == nullptr) {
                    current_buffer_ = new char[kBackendbuffersize];
                }
            }

            memcpy(current_buffer_ + len_, buf, len);
            len_ += len;
        }

        if (check) cv_.notify_one();
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