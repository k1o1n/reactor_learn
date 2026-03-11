#ifndef BUFFER_H
#define BUFFER_H
#include "noncopyable.h"
#include <vector>
#include <string>
#include <memory>
#include <cstring>
namespace adachi::io {
    class Buffer : adachi::tool::NonCopyAble {
    public:
        Buffer(int size = 1024);
        int ReadFd(int fd);
        int WriteFd(int fd);
        void WriteBuffer(const std::string& message);
        void ReadBuffer(std::string& message);
        void Expand(unsigned int size);
    private:
        void MovePtr();
        std::vector<char> buffer_;
        int readptr_;
        int writeptr_;
    };
}
#endif // BUFFER_H