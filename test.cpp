#include <string>
#include <cassert>

#include "common.hpp"

class {
public:
    void init() {
        listen_fd = socket(AF_INET, SOCK_STREAM, 0);
        assert(listen_fd > 0 && "Failed to allocate listen fd");

        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(23233);
        addr.sin_addr.s_addr = inet_addr("10.1.0.1");

        assert(bind(listen_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(sockaddr)) == 0 && "Bind failed");
        assert(listen(fd, 128) == 0 && "Listen failed");

        fd = accept(listen_fd, nullptr, nullptr);
        assert(fd > 0 && "Failed to wait client connect");
    }

    void addLPM(uint32_t ip, uint8_t cidr, uint8_t port) {
        Message msg;
        msg.addr = ip;
        msg.cidr = cidr;
        msg.dst_port = port;
        msg.type = MessageType::AddRule;
        assert(msg.sendTo(fd) == MessageSendResult::Success && "Router closed");
    }

    void delLPM(uint32_t ip, uint8_t cidr) {
        Message msg;
        msg.addr = ip;
        msg.cidr = cidr;
        msg.type = MessageType::DelRule;
        assert(msg.sendTo(fd) == MessageSendResult::Success && "Router closed");
    }

private:
    int listen_fd;
    int fd;
} router;

MessageSendResult Message::sendTo(int fd) {
    this->length = htonl(static_cast<uint32_t>(sizeof(Message)));
    auto all = sizeof(Message);
    uint32_t sent = 0;
    while (sent < all) {
        auto tmp = send(fd,
            reinterpret_cast<char*>(this) + sent,
            all - sent, 0);
        if (tmp <= 0)
            return MessageSendResult::RouterClosed;
        sent += tmp;
    }
    this->length = sizeof(Message);
    return MessageSendResult::Success;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        assert(0 && "./test <testcase/interactive>");
    }
    std::string option = std::string(argv[1]);
    if (option == "testcase") {
        router.init();
    } else if (option == "interactive") {
        router.init();
    } else {
        assert(0 && "Unknown option");
    }
    
    return 0;
}
