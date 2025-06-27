//
// Created by 乔益金 on 25-6-26.
//

#ifndef MESSAGE_H
#define MESSAGE_H

#include <string>
#include <nlohmann/json.hpp>

struct RequestVote {
    int term;
    int candidateId;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(RequestVote, term, candidateId)
};

struct VoteResponse {
    int term;
    bool voteGranted;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(VoteResponse, term, voteGranted)
};

//添加raft同步日志
struct LogEntry {
    int term;
    std::string command;//客户端请求，例如put x 5

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(LogEntry, term, command)
};

struct AppendEntries {
    int term;
    int leaderId;
    int prevLogIndex;//此次复制的日志的前一条日志索引
    int prevLogTerm;//上一条日志的Term
    std::vector<LogEntry> entries;
    int leaderCommit;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(AppendEntries, term, leaderId, prevLogIndex,prevLogTerm,entries,leaderCommit)
};

struct AppendResponse {
    int term;
    bool success;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(AppendResponse, term, success)
};
#endif //MESSAGE_H
