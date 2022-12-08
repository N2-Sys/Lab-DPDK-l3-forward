#include <string>
#include <cassert>

#include "common.hpp"

MessageSendResult Message::sendTo(int fd) {
    
}

int main(int argc, char** argv) {
    if (argc != 2) {
        assert(0 && "./test <testcase/interactive>");
    }
    std::string option = std::string(argv[1]);
    if (option == "testcase") {

    } else if (option == "interactive") {

    } else {
        assert(0 && "Unknown option");
    }
    
    return 0;
}
