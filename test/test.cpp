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
    server.server_.Start();
    std::cin.get();
    return 0;
}