# KV Binary Protocol v1

## 1. Overview

本协议用于 `reactor_learn` 中 KV 服务的请求与响应通信，采用**固定长度头部 + 变长 body** 的二进制协议格式。

设计目标：

- 支持 TCP 字节流下的可靠拆包与组包
- 支持请求/响应关联
- 支持后续协议扩展
- 支持基础 KV 操作：`PING` / `PUT` / `GET` / `DEL`

---

## 2. Header Layout

Header 固定长度为 **24 bytes**。

| Field | Type | Description |
|:---:|:---:|:---|
| magic | uint32_t | 协议魔数，用于校验协议合法性 |
| version | uint16_t | 协议版本 |
| msg_type | uint16_t | 消息类型：请求或响应 |
| opcode | uint16_t | 操作类型 |
| status | uint16_t | 响应状态码，请求时固定为 0 |
| body_length | uint32_t | body 长度（单位：字节，不包含 header） |
| request_id | uint64_t | 请求 ID，用于将响应关联到对应请求 |

> 所有整数字段统一采用 **网络字节序（big-endian）** 编码。

---

## 3. Field Values

### 3.1 magic

固定值：

```text
0x06161027
```

---

### 3.2 version

当前版本：

```text
1
```

---

### 3.3 msg_type

| Type | Name | Value |
|:---:|:---:|:---:|
| Request | `kRequest` | 1 |
| Response | `kResponse` | 2 |

---

### 3.4 opcode

| Operation | Name | Value |
|:---:|:---:|:---:|
| PING | `kPing` | 1 |
| PUT | `kPut` | 2 |
| GET | `kGet` | 3 |
| DEL | `kDel` | 4 |

---

### 3.5 status

| Status | Name | Value | Description |
|:---:|:---:|:---:|:---|
| OK | `kOk` | 0 | 请求处理成功 |
| NOT_FOUND | `kNotFound` | 1 | key 不存在 |
| BAD_REQUEST | `kBadRequest` | 2 | 请求格式非法 |
| INTERNAL_ERROR | `kInternalError` | 3 | 服务器内部错误 |
| UNSUPPORTED_OPCODE | `kUnsupportedOpcode` | 4 | 不支持的操作类型 |
| BODY_TOO_LARGE | `kBodyTooLarge` | 5 | body 超出限制 |
| KEY_TOO_LARGE | `kKeyTooLarge` | 6 | key 超出限制 |
| VALUE_TOO_LARGE | `kValueTooLarge` | 7 | value 超出限制 |

---

## 4. Body Format

### 4.1 PING Request

无 body。

---

### 4.2 PUT Request

| Field | Type | Description |
|:---:|:---:|:---|
| key_length | uint32_t | key 长度 |
| value_length | uint32_t | value 长度 |
| key | bytes | key 内容 |
| value | bytes | value 内容 |

编码顺序：

```text
[key_length][value_length][key][value]
```

---

### 4.3 GET Request

| Field | Type | Description |
|:---:|:---:|:---|
| key_length | uint32_t | key 长度 |
| key | bytes | key 内容 |

编码顺序：

```text
[key_length][key]
```

---

### 4.4 DEL Request

| Field | Type | Description |
|:---:|:---:|:---|
| key_length | uint32_t | key 长度 |
| key | bytes | key 内容 |

编码顺序：

```text
[key_length][key]
```

---

## 5. Response Body Rules

### 5.1 PING / PUT / DEL Response

默认无 body，处理结果通过 header 中的 `status` 表示。

---

### 5.2 GET Response

当 `status == kOk` 时，body 格式如下：

| Field | Type | Description |
|:---:|:---:|:---|
| value_length | uint32_t | value 长度 |
| value | bytes | value 内容 |

编码顺序：

```text
[value_length][value]
```

当 `status != kOk` 时，body 为空。

---

## 6. Constraints

- `max_key_length = 256 bytes`
- `max_value_length = 1 MiB`
- `max_body_length = 2 MiB`
- `key_length` 不能为空且必须与实际 key 字节数一致
- 所有非法包必须触发协议错误处理逻辑

---

## 7. Error Handling Policy

服务端在解析请求时应至少校验以下内容：

1. `magic` 是否正确
2. `version` 是否支持
3. `msg_type` 是否合法
4. `opcode` 是否支持
5. `body_length` 是否超限
6. `key_length` / `value_length` 是否与 body 实际长度匹配

对于无法恢复同步的非法字节流，服务端应**直接关闭连接**，避免协议流失步导致后续请求持续异常。

对于已成功完成 framing 且仅语义非法的请求（如不支持的 opcode），服务端可返回错误响应，而不必关闭连接。