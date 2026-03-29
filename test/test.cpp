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
    adachi::network::TcpServer server(listenaddr);
    server.SetNewconnectionCallback([](std::shared_ptr<adachi::network::TcpConnection> ptr) {
        ptr->SetOnMessage([](std::shared_ptr<adachi::network::TcpConnection> conn_ptr, adachi::io::Buffer& buffer) {
            if (buffer.Size() >= sizeof(unsigned int)) {
                unsigned int len = buffer.PeekUnsignedInt();
                // unsigned int len = buffer.PeekUnsignedInt();
                if (len > 100000) {
                    std::cout << "[Info] buffer size exceeded 100000 * sizeof(char).TcpConnection will be closed soonly" << std::endl;
                    conn_ptr->Close();
                }
                else {
                    if (buffer.Size() >= len + sizeof(unsigned int)) {
                        std::string header;
                        std::string message;
                        buffer.ReadBuffer(header, sizeof(unsigned int));
                        buffer.ReadBuffer(message, len);

                        std::cout << "receieve message: " << message << std::endl;

                    }
                }
            }
        });
    });
    server.SetSubThreadNum(10);
    server.Start();
    std::cin.get();
    return 0;
}