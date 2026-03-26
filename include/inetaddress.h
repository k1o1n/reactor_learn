#ifndef INETADDRESS_H
#define INETADDRESS_H
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
namespace adachi::network {
    class Socket;
    /// 仅支持ipv4
    class INetAddress {
    public:
        INetAddress();
        INetAddress(const INetAddress&);
        INetAddress(INetAddress&&);
        bool SetIp(const std::string& ip);
        void SetPort(const in_port_t& port);

        in_port_t Port() const;
        std::string Ip() const;
        const sockaddr* GetCore() const;
        socklen_t Length() const;
    private:
        friend class Socket;
        sockaddr_in addr_{};
        socklen_t len_;
    };
}
#endif // INETADDRESS_H