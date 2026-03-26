#include "socket.h"
#include <netinet/in.h>
#include <iostream>
#include "channel.h"
#include <functional>
#include "inetaddress.h"
#include "eventloop.h"
#include "acceptor.h"
namespace adachi::network {
    Acceptor::Acceptor(adachi::tool::EventLoop* loop, const INetAddress &listenaddr) 
        : socket_(adachi::network::Socket::CreateNonBlockSocket())
        , accept_channel_(loop, socket_.Fd())
        , owner_(loop)
    {
        if (!socket_.BindAddress(listenaddr)) {
            std::cout << "[error] class Acceptor: BindAddress error" << std::endl;
            return;
        }
        accept_channel_.SetActive(adachi::io::Channel::kRead);
    }

    bool Acceptor::Listen(const int& backlog) {
        if (socket_.Listen(backlog)) {
            listen_check_ = true;
            return true;
        }
        return false;
    }

    bool Acceptor::IsListening() {
        return listen_check_;
    }

    void Acceptor::SetNewconnectionCallback(std::function<void(int, INetAddress&, int)> callback) {
        accept_channel_.SetReadCallback([this, callback](){
            INetAddress newlink_addr;
            int fd = Accept(newlink_addr);
            int saveerrno = 0;
            if (fd < 0) saveerrno = errno;
            callback(fd, newlink_addr, saveerrno);
        });
    }
}
