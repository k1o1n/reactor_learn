#include <iostream>
#include <cstring>
#include <map>
#include <set>
#include "adachi_network.h"
#include <memory>
//#include <mutex>

int main() {
    adachi::network::INetAddress listenaddr;
    listenaddr.SetIp("127.0.0.1");
    listenaddr.SetPort(12345);
    adachi::network::TcpServer server(listenaddr, {60, {1, 0}});
    server.SetSubThreadNum(10);
    server.Start();
    std::cin.get();
    return 0;
}