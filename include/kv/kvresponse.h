#ifndef KVRESPONSE_H
#define KVRESPONSE_H
#include <iostream>
#include <string>

namespace adachi::network::kv::protocol {
    struct KVResponse {
        /// 响应结果
        uint16_t status_;
        std::string key_;
        std::string value_;
        /// body字段可以在提供完相关需求信息之后额外一些无名字段信息
        std::string extrainfo_;
    };
}

#endif