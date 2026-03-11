#ifndef INETADDRESS_H
#define INETADDRESS_H
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
namespace adachi::network {
    class Socket;
    class INetAddress {
    public:
        static const sa_family_t IPV4 = AF_INET;
        static const sa_family_t IPV6 = AF_INET6;

        INetAddress();
        bool SetFamily(sa_family_t family);
        bool SetIP(const std::string& ip);
        void SetPort(const in_port_t& port);

        const in_port_t Port();
        const std::string Ip();
        const sockaddr* GetCore() const;
        const sa_family_t Family() const;
        const socklen_t Length() const;
    private:
        friend class Socket;
        sockaddr_storage addr_{0};
        socklen_t len_;
        sa_family_t family_;
    };
}
#endif // INETADDRESS_H