#ifndef HEARTBEATOBJ_H
#define HEARTBEATOBJ_H
#include <memory>

namespace adachi::network {
    class TcpConnection;
}

namespace adachi::tool {
    /// 装载TcpConnection类的weak指针，析构时根据weak指针状态判断是否关闭Tcp连接
    class HeartBeatObj {
    public:
        HeartBeatObj(std::weak_ptr<adachi::network::TcpConnection>);
        ~HeartBeatObj();
        int last_insert_pos_;
    private:
        std::weak_ptr<adachi::network::TcpConnection> tcp_weakptr_;
    };
}

#endif