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
    //
    // std::string cmd, key, value;
    // while (true) {
    //     std::cout << "Enter command (put/get/exit): ";
    //     std::cin >> cmd;
    //
    //     if (cmd == "put") {
    //         std::cout << "Key: ";
    //         std::cin >> key;
    //         std::cout << "Value: ";
    //         std::cin >> value;
    //         kv.put(key, value);
    //         std::cout << "Stored.\n";
    //     } else if (cmd == "get") {
    //         std::cout << "Key: ";
    //         std::cin >> key;
    //         auto val = kv.get(key);
    //         if (val) {
    //             std::cout << "Value: " << *val << "\n";
    //         } else {
    //             std::cout << "Key not found.\n";
    //         }
    //     } else if (cmd == "exit") {
    //         break;
    //     } else {
    //         std::cout << "Unknown command.\n";
    //     }
    // }
    // const int nodeCount = 3;
    // std::vector<std::shared_ptr<RaftNode>> nodes;
    //
    // // 模拟 RPC：收到投票请求就转发调用 voteRequest
    // auto sendVote = [&](int from, int to) {
    //     // 模拟投票请求
    //     int term = nodes[from]->getCurrentTerm();
    //     nodes[to]->receiveVoteRequest(term, from);
    //     // 模拟收到投票回复
    //     nodes[from]->receiveVoteResponse(term, true);
    // };
    //
    // for (int i = 0; i < nodeCount; ++i) {
    //     nodes.push_back(std::make_shared<RaftNode>(i, nodeCount, sendVote));
    // }
    //
    // // 每隔10ms tick 一下所有节点
    // for (int round = 0; round < 1000; ++round) {
    //     for (auto& node : nodes) {
    //         node->tick();
    //     }
    //     std::this_thread::sleep_for(std::chrono::milliseconds(10));
    // }
    if (argc < 2) {
        std::cerr << "Usage: ./raft_kv <node_id>\n";
        return 1;
    }

    int nodeId = std::stoi(argv[1]);
    int basePort = 50000;

    // 3个节点：0->5000, 1->5001, 2->5002
    std::vector<std::pair<int, int>> peers = {
        {0, basePort}, {1, basePort+1}, {2, basePort+2}
    };

    boost::asio::io_context io_context;
    RaftServer server(io_context, nodeId, basePort + nodeId, peers);
    server.run();
     return 0;
}
