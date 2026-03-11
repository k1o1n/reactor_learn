#ifndef TCPCONNECTION_H
#define TCPCONNECTION_H
#include "noncopyable.h"
#include "socket.h"
#include <string>
#include "channel.h"
#include "buffer.h"
namespace adachi::network {
    class TcpConnection : adachi::tool::NonCopyAble {
    public:
        TcpConnection(int fd, unsigned int read_buffer_size = 1024, unsigned int write_buffer_size = 1024);
        int Read();
        int Write(const std::string& message);
        int Send();
        void Close();
        ~TcpConnection();
        adachi::io::Channel channel_;
    private:
        Socket socket_;
        adachi::io::Buffer read_buffer_;
        adachi::io::Buffer write_buffer_;
    };
}
#endif // TCPCONNECTION