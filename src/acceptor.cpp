#include "socket.h"
#include <netinet/in.h>
#include <iostream>
#include "channel.h"
#include <functional>
#include "inetaddress.h"
#include "eventloop.h"
#include "acceptor.h"
#include "logger.h"

namespace adachi::network {
    Acceptor::Acceptor(adachi::tool::EventLoop* loop, const INetAddress &listenaddr) 
        : socket_(adachi::network::Socket::CreateNonBlockSocket())
        , accept_channel_(loop, socket_.Fd())
        , owner_(loop)
    {
        if (!socket_.BindAddress(listenaddr)) {
            ADACHI_LOG_ERROR << "class Acceptor: BindAddress error\n";
            return;
        }
    }

    bool Acceptor::Listen(const int& backlog) {
        if (socket_.Listen(backlog)) {
            listen_check_ = true;
            accept_channel_.SetActive(adachi::io::Channel::kRead);
            return true;
        }
        return false;
    }

    bool Acceptor::IsListening() {
        return listen_check_;
    }

    void Acceptor::SetNewconnectionCallback(std::function<void(int, INetAddress&, int)> callback) {
        accept_channel_.SetReadCallback([this, callback](){
            while (true) {
                INetAddress newlink_addr;
                int fd = Accept(newlink_addr);
                if (fd >= 0) {
                    callback(fd, newlink_addr, 0);
                    continue;
                }

                int saveerrno = errno;
                if (saveerrno == EAGAIN || saveerrno == EWOULDBLOCK) {
                    break;
                }
                if (saveerrno == EINTR) {
                    continue;
                }

                callback(fd, newlink_addr, saveerrno);
                break;
            }
        });
    }
}
