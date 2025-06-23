#include "obs_websocket.h"
#include <iostream>

int main() {
    auto client = CreateOBSWebSocketClient("ws://localhost:45600");

    client->set_password("hrf123456");
    client->connect(std::function<void(bool)>([](bool r) {
        std::cout << "send connect: " << r << std::endl;
    }));
    std::cin.get();
    client->disconnect([](bool r) {
        std::cout << "send disconnect: " << r << std::endl;
    });
    std::cin.get();
    client->connect([](bool r) {
        std::cout << "send connect: " << r << std::endl;
    });
    std::cin.get();
    std::cout << "Press Enter to start streaming..." << std::endl;
    std::cin.get();

    client->startStreaming([](int code,bool successed) {
        std::cout << "startStreaming: code " << code << " result: " << successed << std::endl;
    });

    std::cout << "Press Enter to stop streaming..." << std::endl;
    std::cin.get();
    client->stopStreaming([](int code, bool successed) {
        std::cout << "stopStreaming: code " << code << " result: " << successed << std::endl;
    });
    std::cin.get();
    client->disconnect([](bool r) {
        std::cout << "send disconnect: " << r << std::endl;
    });
    return 0;
}