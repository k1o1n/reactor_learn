#ifndef KVREQUEST_H
#define KVREQUEST_H
#include <iostream>
#include <string>

namespace adachi::network::kv::protocol {
    struct KVRequest {
        /// 请求类型
        uint16_t opcode_;
        std::string key_;
        std::string value_;
        /// body字段可以在提供完相关需求信息之后额外一些无名字段信息
        std::string extrainfo_;
    };
}

#endif