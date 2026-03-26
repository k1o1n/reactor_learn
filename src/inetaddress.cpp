#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "inetaddress.h"
class Socket;
namespace adachi::network {
    INetAddress::INetAddress() {
        len_ = sizeof(sockaddr_in);
        addr_.sin_family = AF_INET;
    }
    INetAddress::INetAddress(const INetAddress& obj) {
        SetIp(obj.Ip());
        SetPort(obj.Port());
        len_ = sizeof(sockaddr_in);
        addr_.sin_family = AF_INET;
    }
    INetAddress::INetAddress(INetAddress&& obj) {
        SetIp(obj.Ip());
        SetPort(obj.Port());
        len_ = sizeof(sockaddr_in);
        addr_.sin_family = AF_INET;
    }
    bool INetAddress::SetIp(const std::string& ip) {
        return inet_pton(addr_.sin_family, ip.c_str(), &addr_.sin_addr) == 1;
    }
    void INetAddress::SetPort(const in_port_t& port) {
        addr_.sin_port = htons(port);
    }

    in_port_t INetAddress::Port() const {
        return ntohs(addr_.sin_port);
    }
    std::string INetAddress::Ip() const {
        char buf[INET_ADDRSTRLEN]{};
        const char* p = inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
        return p ? std::string(p) : std::string();
    }
    const sockaddr* INetAddress::GetCore() const {
        return reinterpret_cast<const sockaddr*>(&addr_);
    }
    socklen_t INetAddress::Length() const {return len_;}
}
