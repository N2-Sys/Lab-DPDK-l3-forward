#include "shared.h"
#include "common.hpp"
#include <errno.h>

MessageFillResult Message::fillFrom(int fd) {
    auto ret = recv(fd, &this->length, sizeof(this->length), MSG_DONTWAIT);
    if (ret == 0)
        return MessageFillResult::ControllerClosed;
    if (ret == -1) {
        if (errno == EWOULDBLOCK || errno == EAGAIN)
            return MessageFillResult::NoMessage;
        return MessageFillResult::ControllerClosed;
    }
    if (ret > 0) {
        auto acc = ret;
        while (acc < sizeof(this->length)) {
            auto tmp = recv(fd,
                reinterpret_cast<char*>(&this->length) + acc,
                sizeof(this->length) - acc, 0);
            if (tmp <= 0)
                return MessageFillResult::ControllerClosed;
            acc += tmp;
        }
        this->length = ntohl(this->length);
        while (acc < this->length) {
            auto tmp = recv(fd,
                reinterpret_cast<char*>(&this->length) + acc,
                this->length - acc, 0);
            if (tmp <= 0)
                return MessageFillResult::ControllerClosed;
            acc += tmp;
        }
        return MessageFillResult::Success;
    }
    assert(0 && "Unkonwn message error");
    return MessageFillResult::Success;
}

class {
public:
    void init() {
        fd = socket(AF_INET, SOCK_STREAM, 0);
        assert(fd > 0 && "Allocate socket to controller failed");

        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(23233);
        addr.sin_addr.s_addr = inet_addr("10.1.0.1");

        assert(connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(sockaddr)) == 0 && "Failed to connect to controller");
    }

    bool handleRPC() {
        Message msg;
        switch (msg.fillFrom(fd)) {
            case MessageFillResult::Success:
                switch (msg.type) {
                    case MessageType::AddRule:
                        userAddLPMRule(msg.addr, msg.cidr, msg.dst_port);
                        break;
                    case MessageType::DelRule:
                        userDelLPMRule(msg.addr, msg.cidr);
                        break;
                }
                break;
            case MessageFillResult::NoMessage:
                return true;
            case MessageFillResult::ControllerClosed:
                return false;
        }
        assert(0 && "Unkonwn error");
        return false;
    }

private:
    int fd = 0;
} controller;

int main(int argc, char **argv) {
    /** Detect the controller interface **/
    controller.init();
    
    /** Main Logic **/
    userInit(argc, argv);
    while (controller.handleRPC())
        userLoop();

    return 0;
}