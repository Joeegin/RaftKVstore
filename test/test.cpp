//
// Created by 乔益金 on 25-7-1.
//

#include "Message.h"
#include <iostream>
#include <boost/asio.hpp>
#include <chrono>

int sendGet(const std::string& host,int port,const std::string& key) {
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::socket socket(io_context);
    boost::asio::ip::tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve(host, std::to_string(port));
    boost::asio::connect(socket, endpoints);
    nlohmann::json msg={
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
    nlohmann::json reply = nlohmann::json::parse(line);
    if (reply["success"].get<bool>()) {
        return 0;
    } else{
        return -1;
    }
}

int main() {
    const std::string host = "127.0.0.1";
    const int port = 50000; // 请根据实际端口修改
    const std::string key = "x";
    int total = 100000;
    int success = 0;
    auto start = std::chrono::high_resolution_clock::now();
    for (int i=0;i<total;++i) {
        if (sendGet(host, port, key) == 0) {
            ++success;
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;
    double qps = total / duration.count();
    std::cout << "总耗时: " << duration.count() << " 秒\n";
    std::cout << "成功次数: " << success << "/" << total << "\n";
    std::cout << "QPS: " << qps << std::endl;
    return 0;
}