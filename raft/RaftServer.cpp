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
    _acceptor(io_context,tcp::endpoint(boost::asio::ip::make_address("127.0.0.1"),port)){

    //保存其他节点id和端口
    for(const auto & [peerid,peerport] :peers) {
        if (peerid != id) {
            _peerPorts[peerid]=peerport;
        }
    }
    // 创建 RaftNode，传入发送投票的函数
    _node = std::make_shared<RaftNode>(id, peers.size(), [this](int from, int to,int term) {
        sendVoteRequest(from,to,term);
    });
    
    // 设置发送AppendEntries的回调函数
    _node->setAppendEntriesCallback([this](int toNodeId, const AppendEntries& ae) {
        sendAppendEntries(toNodeId, ae);
    });

}

void RaftServer::run() {
    std::cout << "[Node " << _id << "] listening on port " << _port << std::endl;
    StartAcceptor();

    // 模拟 tick（心跳+选举）
    std::thread([this]() {
        while (true) {
            _node->tick();
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
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
                std::cout << "[handleConnection] received message: " << line << std::endl;
                handleMessage(line, socket);
                //std::cout <<"连接成功"<<std::endl;
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
            
            // 根据Raft协议决定是否投票
            bool voteGranted = false;
            if (req.term >= _node->getCurrentTerm() && _node->getRole() != RaftRole::LEADER) {
                voteGranted = true;
            }

            json respMsg={
                {"type","VoteResponse"},
                {"term",req.term},
                {"voteGranted",voteGranted}
            };
            std::string response=respMsg.dump()+"\n";//序列化响应并且在结尾加上换行
            boost::asio::async_write(*socket,boost::asio::buffer(response),
                [this,socket](boost::system::error_code ec, size_t) {
                handleConnection(socket);
            });
        }
        if (message["type"] == "VoteResponse") {
            int term = message["term"];
            bool voteGranted = message["voteGranted"];
            _node->receiveVoteResponse(term, voteGranted);
            handleConnection(socket);
        }

        if (message["type"] == "AppendEntries") {
            auto ae = message["data"].get<AppendEntries>();
            std::cout << "[Node " << _id << "] received AppendEntries from Node "
                      << ae.leaderId << " term " << ae.term
                      << " entries count: " << ae.entries.size() << std::endl;

            _node->receiveAppendEntries(ae, [socket](const AppendResponse& aresp) {
                json respMsg = {
                    {"type", "AppendEntriesResponse"},
                    {"term", aresp.term},
                    {"success", aresp.success},
                    {"from", aresp.from},
                    {"matchIndex",aresp.matchIndex}
                };
                std::string response = respMsg.dump() + "\n";
                boost::asio::async_write(*socket, boost::asio::buffer(response),
                    [](boost::system::error_code ec, size_t) {});
            });
        }
        if (message["type"] == "AppendResponse") {
            auto resp = message["data"].get<AppendResponse>();

            _node->receiveAppendResponse(resp.from, resp.term, resp.success);
        }
        if (message["type"] == "ClientPut") {
            std::string key = message["key"];
            std::string value = message["value"];

            if (_node->getRole()==RaftRole::LEADER) {
                std::string op="Put";
                _node->appendEntries(op,key,value);
                json respMsg = {
                    {"type", "ClientResponse"},
                    {"success", true},
                    {"leaderId", _id}
                };
                std::string response=respMsg.dump()+"\n";
                boost::asio::async_write(*socket,boost::asio::buffer(response),
                    [](boost::system::error_code ec,size_t) {

                });
            }else {
                int currentLeaderId = _node->getLeaderId();
                std::cout << "[Node " << _id << "] Not leader, current leaderId: " << currentLeaderId << std::endl;
                json respMsg = {
                    {"type", "ClientResponse"},
                    {"success", false},
                    {"leaderId", currentLeaderId},
                    {"error", "Not leader"}
                };
                std::string response=respMsg.dump()+"\n";
                boost::asio::async_write(*socket,boost::asio::buffer(response),
                    [](boost::system::error_code ec,size_t) {});
            }
        }
        if (message["type"] == "ClientGet") {
            //消息体最好发送操作类型，方便日志保存，待实现

            std::string key = message["key"];
            if (_node->getRole()==RaftRole::LEADER) {
                auto value = _node->getValue(key);
                json respMsg={
                    {"type", "ClientResponse"},
                    {"success", true},
                    {"leaderId", _id},
                    {"value", value.value_or("NOT_FOUND")}
                };
                std::string response=respMsg.dump()+"\n";
                boost::asio::async_write(*socket,boost::asio::buffer(response),
                        [](boost::system::error_code ec, size_t) {

                    });
            } else {
                int currentLeaderId = _node->getLeaderId();
                std::cout << "[Node " << _id << "] Not leader, current leaderId: " << currentLeaderId << std::endl;
                json respMsg = {
                    {"type", "ClientResponse"},
                    {"success", false},
                    {"leaderId", currentLeaderId},
                    {"error", "Not leader"}
                };
                std::string response=respMsg.dump()+"\n";
                boost::asio::async_write(*socket,boost::asio::buffer(response),
                    [](boost::system::error_code ec,size_t) {});
            }
        }
    }catch (std::exception &e) {
        std::cerr << "[Node " << _id << "] Error parsing message: " << e.what() << std::endl;
    }
}

void RaftServer::sendVoteRequest(int fromId,int peerId,int term) {
    auto it = _peerPorts.find(peerId);
    if (it == _peerPorts.end()) return;

    int peerport = it->second;
    auto socket = std::make_shared<tcp::socket>(_io_context);
    auto endpoint = tcp::endpoint(boost::asio::ip::make_address("127.0.0.1"), peerport);

    socket->async_connect(endpoint, [this, socket, fromId, peerId, term](boost::system::error_code ec) {
        if (ec) {
            std::cerr << "Failed to connect to Node " << peerId
                      << " on port " << _peerPorts[peerId]
                      << ": " << ec.message() << " (code: " << ec.value() << ")" << std::endl;
            std::cerr << "[Vote] Connect failed: " << ec.message() << std::endl;
            return;
        }
        std::cout << "[Node " << fromId << "] Connected to Node " << peerId << " for vote request" << std::endl;

        RequestVote req{term, fromId};
        json reqMsg = {
            {"type", "RequestVote"},
            {"data", req}
        };
        std::string message = reqMsg.dump() + "\n";

        auto buffer = std::make_shared<boost::asio::streambuf>();
        boost::asio::async_write(*socket, boost::asio::buffer(message),
            [this, socket, buffer](boost::system::error_code ec, std::size_t) {
                if (!ec) {
                    boost::asio::async_read_until(*socket, *buffer, '\n',
                        [this, socket, buffer](boost::system::error_code ec, std::size_t) {
                            if (!ec) {
                                std::istream is(buffer.get());
                                std::string line;
                                std::getline(is, line);
                                if (!line.empty()) {
                                    std::cout << "[Vote] Got VoteResponse: " << line << std::endl;
                                    handleMessage(line, socket);
                                } else {
                                    std::cerr << "[Vote] empty line from peer\n";
                                }
                            } else {
                                std::cerr << "[Vote] Read error: " << ec.message() << std::endl;
                            }
                        });
                } else {
                    std::cerr << "[Vote] Write error: " << ec.message() << std::endl;
                }
            });

        std::cout << "[Vote] Sending RequestVote to Node " << peerId << std::endl;
    });
}

void RaftServer::appendToLog(const std::string &op, const std::string &key, const std::string &value) {
    if (_node->getRole() == RaftRole::LEADER) {
        _node->appendEntries(op, key, value);
    } else {
        std::cout << "Not leader, cannot append" << std::endl;
    }
}

void RaftServer::sendAppendEntries(int toNodeId, const AppendEntries& ae) {
    auto it = _peerPorts.find(toNodeId);
    if (it == _peerPorts.end()) {
        std::cerr << "[AppendEntries] peerId " << toNodeId << " not found in _peerPorts" << std::endl;
        return;
    }

    int peerport = it->second;
    std::cout << "[AppendEntries] Try connect to Node " << toNodeId << " on port " << peerport << std::endl;
    auto socket = std::make_shared<tcp::socket>(_io_context);
    auto endpoint = tcp::endpoint(boost::asio::ip::make_address("127.0.0.1"), peerport);

    socket->async_connect(endpoint, [this, socket, toNodeId, ae](boost::system::error_code ec) {
        if (ec) {
            std::cerr << "[AppendEntries] Connect failed to Node " << toNodeId
                      << ": " << ec.message() << std::endl;
            return;
        }

        std::cout << "[Node " << _id << "] Connected to Node " << toNodeId << " for append entries" << std::endl;

        json reqMsg = {
            {"type", "AppendEntries"},
            {"data", ae}
        };
        std::string message = reqMsg.dump() + "\n";

        boost::asio::async_write(*socket, boost::asio::buffer(message),
            [this, socket, toNodeId](boost::system::error_code ec, std::size_t) {
                if (ec) {
                    std::cerr << "[AppendEntries] Write error to Node " << toNodeId << ": " << ec.message() << std::endl;
                    return;
                }

                std::cout << "[AppendEntries] Sent to Node " << toNodeId << ", waiting for response..." << std::endl;

                auto buffer = std::make_shared<boost::asio::streambuf>();
                boost::asio::async_read_until(*socket, *buffer, '\n',
                    [this, socket, buffer, toNodeId](boost::system::error_code ec, std::size_t) {
                        if (ec) {
                            std::cerr << "[AppendEntries] Read error from Node " << toNodeId << ": " << ec.message() << std::endl;
                            return;
                        }

                        std::istream is(buffer.get());
                        std::string line;
                        std::getline(is, line);
                        if (!line.empty()) {
                            std::cout << "[AppendEntries] Got response from Node " << toNodeId << ": " << line << std::endl;
                            handleMessage(line, socket);
                        } else {
                            std::cerr << "[AppendEntries] Empty response from Node " << toNodeId << std::endl;
                        }
                    });
            });
    });

    std::cout << "[Node " << _id << "] sendAppendEntriesTo(" << toNodeId
              << ") entries=" << ae.entries.size() << " term=" << ae.term << std::endl;
}


