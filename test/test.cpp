#include <iostream>
#include <cstring>
#include <map>
#include <set>
#include "adachi_network.h"
#include <memory>
//#include <mutex>

class Ipv4TcpServer {
public:
    Ipv4TcpServer(const adachi::network::INetAddress listenaddr)
        : server_(listenaddr)
    {

    }
    adachi::network::TcpServer server_;
    std::set<std::shared_ptr<adachi::network::TcpConnection>> link_;
    //std::mutex mtx_;
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
            std::shared_ptr<adachi::network::TcpConnection> linkptr = std::make_shared<adachi::network::TcpConnection>(server.server_.pool_->GetOneThread(), fd);
            linkptr->SaveLifeMechanism();
            
            server.link_.insert(linkptr);

            linkptr->channel_->SetReadCallback([ptr = linkptr]() {
                ptr->Read();
            });
            linkptr->SetCloseCallback([&server](const std::shared_ptr<adachi::network::TcpConnection> ptr) {
                //std::lock_guard<std::mutex> lock(server.mtx_);
                server.server_.baseloop_->Submit([&server, ptr]() {
                    server.link_.erase(ptr);
                }); /// 多线程操纵红黑树有危险，需要交由一个线程统一管理
            });
            linkptr->channel_->SetActive(adachi::io::Channel::kRead | adachi::io::Channel::kClose);
        }
        else {
            std::cout << "[Error] connection failed: " << strerror(saveerrno) << std::endl;
        }
    });
    server.server_.Start();
    std::cin.get();
    return 0;
}