#include <unistd.h>
#include "noncopyable.h"
#include <netinet/in.h>
#include "inetaddress.h"
#include <sys/socket.h>
#include <fcntl.h>
#include "socket.h"
namespace adachi::network {
    Socket::Socket(int fd) 
        : fd_(fd)
    {
        fd_ = fd;
    }
    Socket::~Socket() {
        if (fd_ >= 0) close(fd_);
    }

    Socket::Socket(Socket&& newsocket) {
        fd_ = newsocket.fd_;
        newsocket.fd_ = -1;
    }

    bool Socket::BindAddress(const INetAddress& addr) {
        if (addr.Family() != INetAddress::IPV6 && addr.Family() != INetAddress::IPV4) {
            return false;
        }
        return bind(fd_, addr.GetCore(), addr.Length()) == 0;
    }
    bool Socket::Listen(const int& backlog) {
        return listen(fd_, backlog) == 0;
    }
    int Socket::Accept(INetAddress& addr) {
        return accept(fd_, (sockaddr*)&(addr.addr_), &addr.len_);
    }

    Socket Socket::CreateNonBlockSocket(int family = AF_INET) {
        int fd = socket(family, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0); 
        Socket newsocket(fd);
        return newsocket;
    }
}
