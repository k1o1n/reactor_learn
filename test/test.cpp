#include <iostream>
#include <cstring>
#include <map>
#include <set>
#include "adachi_network.h"
#include <memory>

class Ipv4TcpServer {
public:
    Ipv4TcpServer(const adachi::network::INetAddress listenaddr)
        : server_(listenaddr)
    {

    }
    adachi::network::TcpServer server_;
    std::set<std::shared_ptr<adachi::network::TcpConnection>> link_;
};

int main() {
    adachi::network::INetAddress listenaddr;
    listenaddr.SetIp("127.0.0.1");
    listenaddr.SetPort(12345);
    Ipv4TcpServer server(listenaddr);
    server.server_.SetSubThreadNum(10);
    server.server_.SetNewconnectionCallback([&server](int fd, adachi::network::INetAddress& addr, int saveerrno) {
        if (fd >= 0) {
            // std::cout << "[info] receive a connection from " << addr.Ip() << " " << std::endl;
            std::shared_ptr<adachi::network::TcpConnection> linkptr = std::make_shared<adachi::network::TcpConnection>(server.server_.baseloop_, fd);
            linkptr->SaveLifeMechanism();
            auto it = server.link_.insert(linkptr);

            linkptr->channel_->SetReadCallback([ptr = *it.first]() {
                int saveerrno;
                ptr->Read(saveerrno);
            });
            linkptr->SetCloseCallback([&server](const std::shared_ptr<adachi::network::TcpConnection> ptr) {
                server.link_.erase(ptr);
            });
            linkptr->channel_->SetActive(adachi::io::Channel::kRead);
        }
        else {
            std::cout << "[Error] connection failed: " << strerror(saveerrno) << std::endl;
        }
    });
    server.server_.Start();
    std::cin.get();
    return 0;
}