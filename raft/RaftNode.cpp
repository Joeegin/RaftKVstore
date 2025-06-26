//
// Created by 乔益金 on 25-6-26.
//

#include "RaftNode.h"
#include <iostream>

static int randomTimeout() {
    return 150+std::rand()%150;
}

RaftNode::RaftNode(int id, int totalNodes, std::function<void(int, int)> callback)
    :_id(id),_totalNodes(totalNodes),_sendVoteRequest(callback){
    _electionTimeout =randomTimeout();
    _electionElapsed=0;
}

void RaftNode::tick() {
    _electionElapsed+=10;
    if (_role!=RaftRole::LEADER && _electionElapsed>=_electionTimeout) {
        becomeCandidate();
    }
}

void RaftNode::becomeFollower(int term) {
    _currentTerm=term;
    _role=RaftRole::FOLLOWER;
    _votedFor=-1;
    _electionElapsed=0;
    _electionTimeout=randomTimeout();
}

void RaftNode::becomeCandidate() {
    _currentTerm++;
    _role=RaftRole::CANDIDATE;
    _votedFor=1;
    _electionElapsed=0;
    _electionTimeout=randomTimeout();
    std::cout << "[Node " << _id << "] Becomes CANDIDATE for term " << _currentTerm << std::endl;
    for (int i = 0; i < _totalNodes; ++i) {
        if (i != _id) {
            _sendVoteRequest(_id, i);
        }
    }

}

void RaftNode::becomeLeader() {
    _role=RaftRole::LEADER;
    std::cout << "[Node " << _id << "] Becomes LEADER for term " << _currentTerm << std::endl;
}

void RaftNode::receiveVoteRequest(int term, int candidateId) {
    if (term>_currentTerm) {
        becomeFollower(term);
        _votedFor=candidateId;
        std::cout << "[Node " << _id << "] votes for Node " << candidateId << " in term " << term << std::endl;
        _sendVoteRequest(_id, candidateId);
    }
}

void RaftNode::receiveVoteResponse(int term, bool voteGranted) {
    if (_role!=RaftRole::CANDIDATE || term!=_currentTerm) {
        return ;
    }
    if (voteGranted) {
        _votesReceived++;
        if (_votesReceived > _totalNodes / 2) {
            becomeLeader();
        }
    }
}
