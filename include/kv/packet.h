#ifndef PACKET_H
#define PACKET_H
#include <iostream> // typedef 各种类型的名字
#include <string>

namespace adachi::network::kv::protocol {
    struct PacketHead {
        /// 协议魔数，用于校验协议合法性
        uint32_t magic_;
        /// 协议版本
        uint16_t version_;
        /// 消息类型：请求或响应
        uint16_t msg_type_;
        /// 操作类型
        uint16_t opcode_;
        /// 响应状态码，请求时固定为 0
        uint16_t status_;
        /// 长度（单位：字节，不包含 header）
        uint32_t body_length_;
        /// 请求 ID，用于将响应关联到对应请求
        uint64_t request_id_;
    };

    struct Packet {
        PacketHead head_;
        std::string body_;
    };
}

#endif