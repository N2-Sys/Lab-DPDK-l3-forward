#pragma once

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include <cstdint>
#include <cassert>

enum class MessageType {
    AddRule = 0,
    DelRule = 1
};

enum class MessageFillResult {
    Success = 0,
    NoMessage = 1,
    ControllerClosed = 2
};

enum class MessageSendResult {
    Success = 0,
    RouterClosed = 1
};

#pragma pack(1)
struct Message {
    uint32_t length;
    MessageType type;

    uint32_t addr;
    uint8_t cidr;
    uint8_t dst_port;

    MessageFillResult fillFrom(int fd);
    MessageSendResult sendTo(int fd);
};
#pragma pack()
