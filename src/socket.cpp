#include <unistd.h>
#include "noncopyable.h"
#include <netinet/in.h>
#include "inetaddress.h"
#include <sys/socket.h>
#include <fcntl.h>
#include "socket.h"
#include <iostream>

namespace adachi::network {
    Socket::Socket(int fd) 
        : fd_(fd)
    {
        fd_ = fd;
    }
    Socket::~Socket() {
        if (fd_ >= 0) close(fd_);
    }

    void Socket::Close() {
        if (fd_ >= 0) close(fd_);
        fd_ = -1;
    }

    Socket::Socket(Socket&& newsocket) {
        fd_ = newsocket.fd_;
        newsocket.fd_ = -1;
    }

    bool Socket::BindAddress(const INetAddress& addr) {
        return bind(fd_, addr.GetCore(), addr.Length()) == 0;
    }
    bool Socket::Listen(const int& backlog) {
        return listen(fd_, backlog) == 0;
    }
    int Socket::Accept(INetAddress& addr) {
        socklen_t len = sizeof(addr.addr_);
        int connfd = accept4(fd_, (sockaddr*)&(addr.addr_), &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
        addr.len_ = len;
        return connfd;
    }

    Socket Socket::CreateNonBlockSocket() {
        int fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0); 

        if (fd < 0) {
            std::cout << "[Error] Socket::CreateNonBlockSocket failed: fd < 0" << std::endl;
        }

        int opt = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
        Socket newsocket(fd);
        return newsocket;
    }

    int Socket::Fd() const {
        return fd_;
    }
}
