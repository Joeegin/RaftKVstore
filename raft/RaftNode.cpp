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
//Leader添加日志，并发送给Follower
void RaftNode::appendNewCommand(const std::string &command) {
    _logs.push_back({_currentTerm, command});
    for (int peerId = 0; peerId < _totalNodes; ++peerId) {
        if (peerId == _id) continue;
        sendAppendEntries(peerId);
    }
}
//Leader发送AppendEntries
void RaftNode::sendAppendEntries(int peerId) {
    int nextIndex = _nextIndex[peerId];
    int prevLogIndex = nextIndex-1;
    int prevLogTerm = (prevLogIndex>=0 && prevLogIndex<_logs.size()) ? _logs[prevLogIndex].term : -1;

    std::vector<LogEntry> entries;
    if (nextIndex<_logs.size()) {
        entries.assign(_logs.begin()+nextIndex,_logs.end());
    }

    AppendEntries msg{
        _currentTerm,
        _id,
        prevLogIndex,
        prevLogTerm,
        entries,
        _commitIndex,
    };
    //通过网络层发送，待实现
    sendAppendEntriesRPC(peerId, msg);
}
//Follower接收AppendEntries并返回结果
AppendResponse RaftNode::handleAppendEntries(const AppendEntries &req) {
    if (req.term <_currentTerm) {
        return {_currentTerm, false};
    }

    if (req.prevLogIndex>=_logs.size() || _logs[req.prevLogIndex].term !=req.term) {
        return {_currentTerm, false};
    }

    _logs.resize(req.prevLogIndex+1);
    _logs.insert(_logs.end(),req.entries.begin(),req.entries.end());

    // 更新 commitIndex 并执行日志
    if (req.leaderCommit > _commitIndex) {
        _commitIndex = std::min(req.leaderCommit, (int)_logs.size() - 1);
        applyLog();
    }

    return {_currentTerm, true};
}
//Leader收到成功的响应后，推进commitIndex
void RaftNode::receiveAppendResponse(int peerId, const AppendResponse &resp) {
    if (resp.term > _currentTerm) {
        becomeFollower(resp.term);
        return;
    }
    if (resp.success) {
        _matchIndex[peerId]=_logs.size()-1;
        _nextIndex[peerId]=_matchIndex[peerId]+1;
    }
}


