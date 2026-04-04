#ifndef LOGGER_H
#define LOGGER_H
#include <thread>
#include <string>
#include <mutex>
#include <condition_variable>
#include <memory>
#include "noncopyable.h"
#include <vector>
#include <cstring>
#include <atomic>
#include <charconv>

namespace adachi::tool {
    class AsyncLogging;
    /// 一个日志类，传入日志等级以及配对的后端处理器
    /// 当析构被触发，日志类会向对应的后端处理器写入信息
    /// 日志栈空间大小为4096bytes（创建时会写入默认信息，同样占用空间），空间不足后续写入会发生截断
    class Logger {
    public:
        Logger(std::string_view level, const char* file, int line, const char* func, AsyncLogging* backendobj);
        ~Logger();

        /// 已经使用了多少字节空间
        unsigned int Size() const;
        /// 剩余多少字节空间
        unsigned int LessSize() const;

        template<typename T>
        Logger& Format(T value) {
            auto [ptr, ec] = std::to_chars(buf_ + used_, buf_ + kLoggerbuffersize, value);
        
            if (ec == std::errc()) {
                // 写入成功，to_chars 写了多少，游标就前移多少
                used_ = ptr - buf_;
            }
            // 否则 (比如 ec == std::errc::value_too_large)
            // 意味着剩余空间真的连这个数字都装不下了，静默丢弃，完美截断！
            return *this;
        }

        /// string_view表示仅可读
        Logger& operator<<(std::string_view);
        Logger& operator<<(const char*);
        Logger& operator<<(unsigned short);
        Logger& operator<<(short);
        Logger& operator<<(unsigned int);
        Logger& operator<<(int);
        Logger& operator<<(unsigned long long);
        Logger& operator<<(long long);
        Logger& operator<<(float);
        Logger& operator<<(double);
        Logger& operator<<(long double);
        Logger& operator<<(ssize_t);
        Logger& operator<<(bool);

        static constexpr int kLoggerbuffersize = 4096;

    private:
        char buf_[kLoggerbuffersize];
        unsigned int used_;
        AsyncLogging* backendobj_;
    };
    // class Logger;
    /// Logger的后端处理器，初始化传入日志需要保存的路径
    /// 默认全局实例为终端路径下"pid" + std::to_string(getpid()) + ".log"的日志
    /// 日志存在处理上限，如果后端处理速度过慢，会出现部分日志丢失的情况
    class AsyncLogging : NonCopyAble {
    public:
        AsyncLogging(const std::string& logpath);
        AsyncLogging(const char* logpath);
        ~AsyncLogging();
        static AsyncLogging& Instance();
        void Append(const char* buf, unsigned int len);
        void BackendThreadFunc();    

        /// 每一个buffer的大小
        static constexpr unsigned int kBackendbuffersize = Logger::kLoggerbuffersize * 1024;
        /// buffer数量的上限，超过这个上限就会发生截断
        static constexpr unsigned int kBackendbufferlimit = 25;
    private:
        struct PendingBuffer {
            std::unique_ptr<char[]> data;
            unsigned int len = 0;
        };

        void StartBackendThread();
        std::unique_ptr<char[]> TakeBufferLocked();
        void RecycleBufferLocked(std::unique_ptr<char[]> buffer);

        std::atomic<bool> running_;
        std::recursive_mutex mtx_;

        std::mutex wake_mtx_;
        std::condition_variable wake_cv_;
        std::atomic<bool> wake_requested_{false};

        std::unique_ptr<char[]> current_buffer_;
        unsigned int current_buffer_len_;
        std::unique_ptr<char[]> next_buffer_;

        std::vector<PendingBuffer> work_list_;
        std::vector<std::unique_ptr<char[]>> empty_buffers_;
        
        std::string logpath_;

        std::thread backendthread_;
    };
}

#define ADACHI_LOG(type) adachi::tool::Logger(#type, __FILE__, __LINE__, __FUNCTION__, &adachi::tool::AsyncLogging::Instance())
#define ADACHI_LOG_INFO ADACHI_LOG(INFO)
#define ADACHI_LOG_ERROR ADACHI_LOG(ERROR)
#define ADACHI_LOG_FATAL ADACHI_LOG(FATAL)
#define ADACHI_LOG_WARNING ADACHI_LOG(WARNING)
#define ADACHI_LOG_DEBUG ADACHI_LOG(DEBUG)

#endif