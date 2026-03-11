#include "noncopyable.h"
#include "socket.h"
#include "tcpconnection.h"
#include <string>
namespace adachi::network {
    TcpConnection::TcpConnection(int fd, unsigned int read_buffer_size, unsigned int write_buffer_size) 
        : socket_(fd)
        , read_buffer_(read_buffer_size)
        , write_buffer_(write_buffer_size)
    {

    }
    int TcpConnection::Read();
    int TcpConnection::Write(const std::string& message) {

    }
    int TcpConnection::Send() {

    }
    void TcpConnection::Close() {
        
    }
    TcpConnection::~TcpConnection();
}
