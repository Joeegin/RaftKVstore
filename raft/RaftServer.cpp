//
// Created by 乔益金 on 25-6-26.
//

#include "RaftServer.h"

#include <iostream>
#include <boost/asio.hpp>
#include <nlohmann/json.hpp>
#include <thread>
#include <memory>

using boost::asio::ip::tcp;
using json = nlohmann::json;


RaftServer::RaftServer(boost::asio::io_context &io_context, int id, int port, const std::vector<std::pair<int, int> > &peers)
    :_io_context(io_context),_id(id),_port(port),
    _acceptor(io_context,tcp::endpoint(tcp::v4(),port)){

    //保存其他节点id和端口
    for(const auto & [peerid,peerport] :peers) {
        if (peerid != id) {
            _peerPorts[peerid]=peerport;
        }
    }
    // 创建 RaftNode，传入发送投票的函数
    _node = std::make_shared<RaftNode>(id, peers.size(), [this](int from, int to) {
        sendVoteRequest(to);
    });
}

void RaftServer::run() {
    std::cout << "[Node " << _id << "] listening on port " << _port << std::endl;
    StartAcceptor();

    // 模拟 tick（心跳+选举）
    std::thread([this]() {
        while (true) {
            _node->tick();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }).detach();

    _io_context.run(); // Boost Asio事件循环
}

void RaftServer::StartAcceptor() {
    auto socket = std::make_shared<tcp::socket>(_io_context);
    _acceptor.async_accept(*socket,[this,socket](boost::system::error_code ec) {
        if (!ec) {
            handleConnection(socket);
        }
        StartAcceptor(); // 再次监听下一个连接
    });
}

void RaftServer::handleConnection(std::shared_ptr<tcp::socket> socket) {
    auto buffer=std::make_shared<boost::asio::streambuf>();
    boost::asio::async_read_until(*socket,*buffer,'\n',
        [this,socket,buffer](boost::system::error_code ec, std::size_t ) {
            if (!ec) {
                std::istream is(buffer.get());
                std::string line;
                std::getline(is, line);
                handleMessage(line, socket);
            }
        });
}

void RaftServer::handleMessage(const std::string &msg, std::shared_ptr<tcp::socket> socket) {
    try {
        auto message = json::parse(msg);//反序列化
        if (message.contains("type") && message["type"]=="RequestVote") {
            RequestVote req=message["data"].get<RequestVote>();
            std::cout << "[Node " << _id << "] received vote request from Node " << req.candidateId << std::endl;
            _node->receiveVoteRequest(req.term,req.candidateId);
            VoteResponse resp{req.term,true};
            json respMsg={
                {"type","VoteResponse"},
                    {"data",resp}
            };
             std::string response=respMsg.dump()+"\n";//序列化响应并且在结尾加上换行
            boost::asio::async_write(*socket,boost::asio::buffer(response),
                [](boost::system::error_code ec, size_t) {

            });
        }
        if (message["type"] == "VoteResponse") {
            VoteResponse resp = message["data"].get<VoteResponse>();
            _node->receiveVoteResponse(resp.term, resp.voteGranted);
        }
        if (message["type"]=="AppendEntries") {
            auto ae = message["data"].get<AppendEntries>();
            _node->receiveAppendEntries(ae,[socket](const AppendResponse& aresp) {
                json respMsg={
                    {"type", "AppendEntriesResponse"},
                    {"data", {
                            {"term", aresp.term},
                            {"success", aresp.success}}
                    }
                };
                std::string response=respMsg.dump()+"\n";
                boost::asio::async_write(*socket,boost::asio::buffer(response),
                    [](boost::system::error_code ec, size_t) {

                });
            });
        }
        if (message["type"] == "AppendResponse") {
            auto arsp = message["data"].get<AppendResponse>();
            //待实现日志响应
        }
    }catch (std::exception &e) {
        std::cerr << "[Node " << _id << "] Error parsing message: " << e.what() << std::endl;
    }
}

void RaftServer::sendVoteRequest(int peerId) {
    auto it = _peerPorts.find(peerId);
    if (it == _peerPorts.end()) return;

    auto peerport = it->second;
    auto socket = std::make_shared<tcp::socket>(_io_context);
    auto endpoint = tcp::endpoint(tcp::v4(), peerport);

    socket->async_connect(endpoint, [this, socket, peerId](boost::system::error_code ec) {
        if (!ec) {
            RequestVote req{_node->getCurrentTerm(), _id};
            json reqMsg = {
                {"type", "RequestVote"},
                {"data", req}
            };
            std::string message = reqMsg.dump() + "\n";

            auto buffer = std::make_shared<boost::asio::streambuf>();
            boost::asio::async_write(*socket, boost::asio::buffer(message),
                [this, socket, buffer](boost::system::error_code ec, std::size_t) {
                    if (!ec) {
                        // 异步读取 VoteResponse
                        boost::asio::async_read_until(*socket, *buffer, '\n',
                            [this, socket, buffer](boost::system::error_code ec, std::size_t) {
                                if (!ec) {
                                    std::istream is(buffer.get());
                                    std::string line;
                                    std::getline(is, line);
                                    if (!line.empty()) {
                                        std::cout << "[Node " << _id << "] Got VoteResponse: " << line << std::endl;
                                        handleMessage(line, socket);
                                    } else {
                                        //std::cerr << "[Node " << _id << "] received empty line (peer closed connection)\n";
                                    }
                                } else {
                                    //std::cerr << "[Node " << _id << "] Read error: " << ec.message() << std::endl;
                                }
                            });
                    } else {
                        //std::cerr << "[Node " << _id << "] Write error: " << ec.message() << std::endl;
                    }
                });

            //std::cout << "[Node " << _id << "] Sending vote request to Node " << peerId << std::endl;
        }
    });
}

void RaftServer::appendToLog(const std::string &op, const std::string &kay, const std::string &value) {

}

