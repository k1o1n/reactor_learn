#ifndef TCPCONNECTION_H
#define TCPCONNECTION_H
#include "noncopyable.h"
#include "socket.h"
namespace adachi::network {
    class TcpConnection : adachi::tool::NonCopyAble {
    public:
        TcpConnetion() {
            
        }
    private:
        Socket socket_;
    };
}
#endif // TCPCONNECTION