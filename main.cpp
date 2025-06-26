#include <iostream>
#include "kvstore/KVStore.h"

int main() {
    KVStore kv("test.txt");

    std::string cmd, key, value;
    while (true) {
        std::cout << "Enter command (put/get/exit): ";
        std::cin >> cmd;

        if (cmd == "put") {
            std::cout << "Key: ";
            std::cin >> key;
            std::cout << "Value: ";
            std::cin >> value;
            kv.put(key, value);
            std::cout << "Stored.\n";
        } else if (cmd == "get") {
            std::cout << "Key: ";
            std::cin >> key;
            auto val = kv.get(key);
            if (val) {
                std::cout << "Value: " << *val << "\n";
            } else {
                std::cout << "Key not found.\n";
            }
        } else if (cmd == "exit") {
            break;
        } else {
            std::cout << "Unknown command.\n";
        }
    }

    return 0;
}