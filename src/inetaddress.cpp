#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "inetaddress.h"
class Socket;
namespace adachi::network {
    INetAddress::INetAddress() {
        SetFamily(INetAddress::IPV4);
        len_ = sizeof(sockaddr_storage);
    }
    bool INetAddress::SetFamily(sa_family_t family) {
        if (family == AF_INET6) {
            addr_.ss_family = family;
            family_ = family;
            return true;
        }
        else if (family == AF_INET) {
            addr_.ss_family = AF_INET;
            family_ = family;
            return true;
        }
        return false;
    }
    bool INetAddress::SetIP(const std::string& ip) {
        if (addr_.ss_family == AF_INET6) {
            return inet_pton(addr_.ss_family, ip.c_str(), &((sockaddr_in6*)&addr_)->sin6_addr) == 1;
        }
        else {
            return inet_pton(addr_.ss_family, ip.c_str(), &((sockaddr_in*)&addr_)->sin_addr) == 1;
        }
    }
    void INetAddress::SetPort(const in_port_t& port) {
        if (addr_.ss_family == AF_INET6) {
            ((sockaddr_in6*)&addr_)->sin6_port = htons(port);
        }
        else {
            ((sockaddr_in*)&addr_)->sin_port = htons(port);
        }
    }

    const in_port_t INetAddress::Port() {
        if (addr_.ss_family == AF_INET6) {
            return ntohs(((sockaddr_in6*)&addr_)->sin6_port);
        }
        else {
            return ntohs(((sockaddr_in*)&addr_)->sin_port);
        }
    }
    const std::string INetAddress::Ip() {
        if (addr_.ss_family == AF_INET6) {
            char buf[INET6_ADDRSTRLEN]{};
            const char* p = inet_ntop(AF_INET6, &((sockaddr_in6*)&addr_)->sin6_addr, buf, sizeof(buf));
            return p ? std::string(p) : std::string();
        }
        else {
            char buf[INET_ADDRSTRLEN]{};
            const char* p = inet_ntop(AF_INET, &((sockaddr_in*)&addr_)->sin_addr, buf, sizeof(buf));
            return p ? std::string(p) : std::string();
        }
    }
    const sockaddr* INetAddress::GetCore() const {
        return (const sockaddr*)&addr_;
    }
    sa_family_t INetAddress::Family() {return family_;}
    socklen_t INetAddress::Length() {return len_;}
}
