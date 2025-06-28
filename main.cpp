#include <iostream>
#include <vector>
#include <memory>
#include <thread>
#include <chrono>
#include <boost/asio.hpp>


#include "kvstore/KVStore.h"
#include "raft/RaftNode.h"
#include "raft/RaftServer.h"

int main(int argc, char* argv[]) {
    // KVStore kv("test.txt");

    if (argc < 2) {
        std::cerr << "Usage: ./raft_kv <node_id>\n";
        return 1;
    }

    int nodeId = std::stoi(argv[1]);
    int basePort = 50000;

    // 3个节点：0->50000, 1->50001, 2->50002
    std::vector<std::pair<int, int>> peers = {
        {0, basePort}, {1, basePort+1}, {2, basePort+2}
    };

    boost::asio::io_context io_context;
    RaftServer server(io_context, nodeId, basePort + nodeId, peers);

    std::thread io_thread([&]() {
        io_context.run();
    });

    server.run();

    std::string cmd;
    while (true) {
        std::cout << "[Node " << nodeId << "] Command (put key value / exit): ";
        std::cin >> cmd;
        if (cmd == "exit") break;
        else if (cmd == "put") {
            std::string key, value;
            std::cin >> key >> value;
            server.appendToLog("put", key, value);
        }
    }


     return 0;
}
