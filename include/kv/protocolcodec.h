#ifndef PROTOCOLCODEC_H
#define PROTOCOLCODEC_H
#include <cstdint>
#include <string>
#include "packet.h"
#include "kvrequest.h"
#include "kvresponse.h"
#include <cstring>

namespace adachi::network::kv::protocol {
    /// 负责解析协议
    /// 共三层转换：字节流<->packet<->kvrequest/kvresponse
    static constexpr u_int32_t kDefaultMagic = 0x06161027;
    static constexpr u_int16_t kDefaultVersion = 1;
    static constexpr u_int16_t kRequest = 1;
    static constexpr u_int16_t kResponse = 2;
    static constexpr u_int16_t kPing = 1;
    static constexpr u_int16_t kPut = 2;
    static constexpr u_int16_t kGet = 3;
    static constexpr u_int16_t kDel = 4;
    static constexpr u_int16_t kOk = 0;
    static constexpr u_int16_t kNotFound = 1;
    static constexpr u_int16_t kBadRequest = 2;
    static constexpr u_int16_t kInternalError = 3;
    static constexpr u_int16_t kUnsupportedOpcode = 4;
    static constexpr u_int16_t kBodyTooLarge = 5;
    static constexpr u_int16_t KeyTooLarge = 6;
    static constexpr u_int16_t kValueTooLarge = 7;

    static constexpr uint32_t kMaxKeyLength = 256;
    static constexpr uint32_t kMaxValueLength = 1024 * 1024;
    static constexpr uint32_t kMaxBodyLength = 2 * 1024 * 1024;

    struct ProtocolCodec { 
        static bool IsBigEnd();
        static void NToH(uint64_t&);
        static void NToH(uint32_t&);
        static void NToH(uint16_t&);
        static void NToH(uint8_t&);
        static void HToN(uint64_t&);
        static void HToN(uint32_t&);
        static void HToN(uint16_t&);
        static void HToN(uint8_t&);

        template<typename T>
        /// 将字节流内容视为网络字节序写到本地类中并且转换为主机序
        static bool WriteFlowToHost(const char* src, uint64_t len, uint64_t& offset, T* store) {
            if (src == nullptr) return false;
            if (store == nullptr) return false;
            if (offset + sizeof(T) > len) return false;
            memmove(store, src + offset, sizeof(T));
            offset += sizeof(T);
            NToH(*store);
            return true;
        }

        template<typename T>
        /// 将内容视为主机序写到字节流中并转换为网络字节序
        static bool WriteHostToFlow(T src, uint64_t& offset, const char* store, uint64_t len) {
            if (store == nullptr) return false;
            if (sizeof(T) + offset > len) return false;
            HToN(src);
            memmove(store + offset, src, sizeof(T));
            offset += sizeof(T);
            HToN(src);
        }

        /// 执行完毕之后必须判断opcode是否合法，会影响到后续的解析 
        static bool DecodeFlowToPacket(std::string_view, Packet*);
        /// 执行完毕之后必须判断opcode是否合法，会影响到后续的解析
        static bool DecodeFlowToPacket(const char*, uint64_t, Packet*);
        static bool EncodePacketToFlow(const Packet*, char*, uint64_t);
        static bool EncodePacketToFlow(const Packet*, std::string&);
        static bool EncodeKVRequestToPacket(const KVRequest*, Packet*);
        /// 如果返回false，代表传入了空指针。status字段为非kOk时也会返回true（当不存在空指针传入时）
        static bool DecodePacketToKVResponse(const Packet*, KVResponse*);
    };

}

#endif