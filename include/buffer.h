#ifndef BUFFER_H
#define BUFFER_H
#include "noncopyable.h"
#include <vector>
#include <string>
namespace adachi::io {
    class Buffer : adachi::tool::NonCopyAble {
    public:
        Buffer(int size = 1024);
        int ReadFd(int fd);
        int SendFd(int fd);
        int WriteBuffer(const std::string& message);
        int ReadBuffer(std::string* message, unsigned int size);
        
    private:
        void DefaultProtocol() {
            
        }

        std::vector<char> buffer_;
        int headptr_;
        int readptr_;
        int writeptr_;
    };
}
#endif // BUFFER_H