#ifndef SOCKET_H
#define SOCKET_H
#include <unistd.h>
#include "noncopyable.h"
#include <netinet/in.h>
#include "inetaddress.h"
#include <sys/socket.h>
#include <fcntl.h>
namespace adachi::network {
    class Socket : adachi::tool::NonCopyAble {
    public:
        Socket(int fd);
        ~Socket();

        Socket(Socket&& newsocket);

        bool BindAddress(const INetAddress& addr);
        bool Listen(const int& backlog);
        int Accept(INetAddress& addr);

        static Socket CreateNonBlockSocket();

        int Fd() const;

        void Close();
    private:
        int fd_;
    };
}
#endif // SOCKET_H