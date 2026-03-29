#ifndef BUFFER_H
#define BUFFER_H
#include "noncopyable.h"
#include <vector>
#include <string>
#include <memory>
#include <cstring>
#include <cerrno>
namespace adachi::io {
    class Buffer : adachi::tool::NonCopyAble {
    public:
        Buffer(int size = 1024);
        int ReadFd(int fd, int& saveerrno);
        int WriteFd(int fd, int& saveerrno);
        void WriteBuffer(const std::string& message);
        void WriteBuffer(const char* message, unsigned int len);
        void ReadBuffer(std::string& message);
        void ReadBuffer(std::string& message, size_t len);
        void Expand(unsigned int size);
        bool Empty();

        /// 单位为sizeof(char)，表示目前最大容量
        size_t Capacity();
        /// 单位为sizeof(char)，表示目前存储内容大小
        size_t Size();
        /// 读取目前存取的前32位
        unsigned int PeekUnsignedInt();
    private:
        void MovePtr();
        std::vector<char> buffer_;
        int readptr_;
        int writeptr_;
    };
}
#endif // BUFFER_H