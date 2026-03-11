#include "noncopyable.h"
#include "socket.h"
#include "tcpconnection.h"
#include <string>
namespace adachi::network {
    TcpConnection::TcpConnection(int fd, unsigned int read_buffer_size, unsigned int write_buffer_size) 
        : socket_(std::make_unique<Socket>(fd))
        , read_buffer_(read_buffer_size)
        , write_buffer_(write_buffer_size)
        , channel_(std::make_unique<adachi::io::Channel>(fd))
    {

    }
    int TcpConnection::Read() {
        return -1;
    }
    int TcpConnection::Write(const std::string& message) {
        return -1;
    }
    int TcpConnection::Send() {
        return -1;
    }
    void TcpConnection::Close() {
        
    }
    TcpConnection::~TcpConnection() {
        
    }
}
