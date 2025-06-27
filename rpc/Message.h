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

#endif //MESSAGE_H
