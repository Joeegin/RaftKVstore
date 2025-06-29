//
// Created by 乔益金 on 25-6-26.
//

#ifndef RAFTSERVER_H
#define RAFTSERVER_H

#include <boost/asio.hpp>
#include <thread>
#include <memory>
#include <unordered_map>
#include "../raft/RaftNode.h"
#include "../rpc/Message.h"


class RaftServer {
public:
    RaftServer(boost::asio::io_context& io_context,int id,int port, const std::vector<std::pair<int,int>>& peers);
    void run();
    void appendToLog(const std::string& op,const std::string& kay,const std::string& value);
    bool isLeader() const {
        return _node->getRole() == RaftRole::LEADER;
    };
    int getLeaderId() const {
        return _node->getLeaderId();  // 你需要在 RaftNode 中记录 leaderId
    }
private:
    void StartAcceptor();
    void handleConnection(std::shared_ptr<boost::asio::ip::tcp::socket> socket);
    void handleMessage(const std::string& msg, std::shared_ptr<boost::asio::ip::tcp::socket> socket);
    void sendVoteRequest(int from,int peerId,int term);
    void sendAppendEntries(int toNodeId, const AppendEntries& ae);

    int _id;
    int _port;
    std::unordered_map<int,int> _peerPorts;

    boost::asio::io_context& _io_context;
    boost::asio::ip::tcp::acceptor _acceptor;

    std::shared_ptr<RaftNode> _node;
};



#endif //RAFTSERVER_H
