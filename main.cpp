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
        {0, 50000}, {1, 50001}, {2, 50002}
    };

    boost::asio::io_context io_context;
    RaftServer server(io_context, nodeId, basePort + nodeId, peers);

    server.run();



     return 0;
}
