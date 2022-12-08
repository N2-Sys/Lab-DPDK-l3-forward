#include <string>
#include <cassert>

#include "common.hpp"
#include "gtest/gtest.h"

#include <fstream>

static uint32_t N;

#define MAXN 32

class NIC{
public:
    NIC() {
        idx = this - NICs;
    }

    void init() {
        assert(0 && "Not imp");
    }

    int idx;
} NICs[MAXN];

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
        argc --;
        ::testing::InitGoogleTest(&argc, argv + 1);
        router.init();
        
        /** Read settings **/
        std::ifstream fin;
        fin.open("../test_setting", std::ios_base::in);
        assert(fin.is_open() && "Failed to open test_setting");
        {
            std::string opt;
            while (fin >> opt) {
                if (opt == "N")
                    assert ((fin >> N) && "Failed to read N");
                else
                    assert (0 && "Unknown test_settings option");
            }
        }
        fin.close();
        
        /** validate options **/
        assert(N <= MAXN && "N should <= MAXN (32)");
        
        /** Init local NICs **/
        for (int i = 0; i < N; i ++) {
            assert(0 && "Not imp");
        }

        /** Send settings to peer **/
        for (int i = 0; i < N; i ++) {
            router.addLPM(inet_addr(("10.1.1." + std::to_string(i + 1)).c_str()), 32, i);
        }

        /** Load remote settings **/
        for (int i = 0; i < N; i ++) {
            assert(0 && "Not imp");
        }

        return RUN_ALL_TESTS();
    } else if (option == "interactive") {
        router.init();
        assert(0 && "Interactive mode is working in progress");
        return 0;
    } else {
        assert(0 && "Unknown option");
    }
    
    return 0;
}
