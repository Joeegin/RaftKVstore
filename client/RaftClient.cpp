//
// Created by 乔益金 on 25-6-29.
//

#include <iostream>
#include <boost/asio.hpp>
#include <nlohmann/json.hpp>
#include "./Message.h"

using boost::asio::ip::tcp;
using json = nlohmann::json;

void sendPut(const std::string& host,int port,const std::string& key,const std::string& value) {
    boost::asio::io_context io_context;
    tcp::socket socket(io_context);

    tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve(host, std::to_string(port));
    boost::asio::connect(socket, endpoints);

    json msg={
        {"type","ClientPut"},
        {"key",key},
        {"value",value}
    };
    std::string data = msg.dump() + "\n";
    boost::asio::write(socket, boost::asio::buffer(data));
    std::cout << "[Client] Sent PUT " << key << " " << value << std::endl;

    boost::asio::streambuf response;
    boost::asio::read_until(socket, response, '\n');
    std::istream is(&response);
    std::string line;
    std::getline(is, line);

    json reply = json::parse(line);
    if (reply["success"].get<bool>()) {
        std::cout << "[Client] PUT successful" << std::endl;
        return;
    } else if (reply.contains("leaderId") && reply["leaderId"].get<int>() != -1) {
        int leaderPort = 50000 + reply["leaderId"].get<int>();
        std::cout << "Redirecting to leader at port " << leaderPort << std::endl;
        sendPut(host, leaderPort, key, value);
        return;
    } else {
        std::cout << "[Client] PUT failed: " << reply["error"].get<std::string>() << std::endl;
    }
}

void sendGet(const std::string& host,int port,const std::string& key) {
    boost::asio::io_context io_context;
    tcp::socket socket(io_context);

    tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve(host, std::to_string(port));
    boost::asio::connect(socket, endpoints);

    json msg={
        {"type","ClientGet"},
        {"key",key}
    };
    std::string data = msg.dump() + "\n";
    boost::asio::write(socket, boost::asio::buffer(data));

    boost::asio::streambuf response;
    boost::asio::read_until(socket, response, '\n');
    std::istream is(&response);
    std::string line;
    std::getline(is, line);

    json reply = json::parse(line);
    if (reply["success"].get<bool>()) {
        std::cout << "[Client] GET result: " << reply["value"] << std::endl;
        return;
    } else if (reply.contains("leaderId") && reply["leaderId"].get<int>() != -1) {
        int leaderPort = 50000 + reply["leaderId"].get<int>();
        std::cout << "Redirecting to leader at port " << leaderPort << std::endl;
        sendGet(host, leaderPort, key);
        return;
    } else {
        std::cout << "[Client] GET failed: " << reply["error"].get<std::string>() << std::endl;
    }
}

int main() {
    std::string cmd;
    std::vector<int> Ports={50000,50001,50002};
    while (true) {
        std::cout << "Enter command (put/get key value or exit): "<< std::endl;
        std::cin >> cmd;
        if (cmd == "exit") break;
        if (cmd == "put") {
            std::string key,value;
            std::cout << "Enter key: ";
            std::cin >> key;
            std::cout << "Enter value: ";
            std::cin >> value;
            sendPut("127.0.0.1",50000,key,value);

        }


    if (cmd == "get") {
        std::string key;
        std::cout << "Enter key: ";
        std::cin >> key;
        sendGet("127.0.0.1",50000,key);
    }
}
}
