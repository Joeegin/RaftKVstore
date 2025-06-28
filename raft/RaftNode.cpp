//
// Created by 乔益金 on 25-6-26.
//

#include "RaftNode.h"
#include <iostream>
#include <regex>

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
    if (_role == RaftRole::LEADER) {
        // 发送心跳
        for (int i = 0; i < _totalNodes; ++i) {
            if (i == _id) continue;
            sendAppendEntriesTo(i);  // empty entries 即心跳
        }
    } else if (_electionElapsed >= _electionTimeout) {
        becomeCandidate();  // 发起选举
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
    _nextIndex = std::vector<int>(_totalNodes, _logs.size());
    _matchIndex = std::vector<int>(_totalNodes, -1);

    // 初始发送空的 AppendEntries 心跳
    for (int i = 0; i < _totalNodes; ++i) {
        if (i == _id) continue;
        sendAppendEntriesTo(i);
    }
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

void RaftNode::receiveAppendEntries(const AppendEntries &ae, std::function<void(const AppendResponse &)> reply) {
    if (ae.term<_currentTerm) {
        reply({_currentTerm,false});
        return;
    }

    becomeFollower(ae.term);
    _electionElapsed=0;

    //验证前一条日志
    if (ae.prevLogIndex>=0 && (ae.prevLogIndex>=_logs.size() || _logs[ae.prevLogIndex].term !=ae.term)) {
        reply({_currentTerm,false});
        return;
    }

    //删除冲突日志
    for (int i=0;i<ae.entries.size();++i) {
        int logIndex=ae.prevLogIndex+1+i;
        if (logIndex>=_logs.size()) break;

        if (_logs[logIndex].term!=ae.term) {
            _logs.resize(logIndex);
            break;
        }
    }

    //追加新日志
    for (int i; i < ae.entries.size(); ++i) {
        _logs.push_back(ae.entries[i]);
    }

    //更新提交日志的索引
    if (ae.leaderCommit > _commitIndex) {
        _commitIndex = std::min((int)_logs.size() - 1, ae.leaderCommit);
        //触发状态机应用日志applyLogs();
    }
    //持久化日志，待实现

    reply({ _currentTerm, true });
}

std::vector<LogEntry> RaftNode::getUncommittedEntries() const {
    return std::vector<LogEntry>(_logs.begin() + _lastCommitIndex + 1, _logs.begin() + _commitIndex + 1);
}

void RaftNode::commitTo(int index, KVStore &store) {
    while (_lastCommitIndex < index && _lastCommitIndex + 1 < _logs.size()) {
        ++_lastCommitIndex;
        const auto& entry = _logs[_lastCommitIndex];
        store.put(entry.key, entry.value);
    }

}

void RaftNode::appendEntries(const std::string& op,const std::string &key, const std::string &value) {
    if (_role!=RaftRole::LEADER) {return;}

    LogEntry entry = { _currentTerm, op,key, value };
    _logs.push_back(entry);
    _matchIndex[_id]=_logs.size()-1;

    for (int i = 0; i < _totalNodes; ++i) {
        if (i == _id) continue;
        sendAppendEntriesTo(i);
    }
}

void RaftNode::receiveAppendResponse(int fromNodeId, int term, bool success) {
    if (term>_currentTerm) {
        becomeFollower(term);
        return;
    }

    if (_role!=RaftRole::LEADER) {return;}

    if (success) {
        _matchIndex[fromNodeId]=_nextIndex[fromNodeId]-1;
        _nextIndex[fromNodeId]=_nextIndex[fromNodeId]+1;

        // 检查是否可以提交（多数节点已复制）
        for (int N=_logs.size()-1;N>_commitIndex;--N) {
            int count =1;
            for (int i=0;i<_totalNodes;++i) {
                if (i==_id){ continue;}
                if (_matchIndex[i]>=N) {
                    ++count;
                }
            }

            if (count>_totalNodes/2 && _logs[N].term== _currentTerm) {
                _commitIndex = N;
                break;
            }
        }

    }else {
        _nextIndex[fromNodeId]=std::max(1, _nextIndex[fromNodeId] - 1);
    }

    // 无论成功与否，都重新尝试发送 AppendEntries
    sendAppendEntriesTo(fromNodeId);
}

void RaftNode::sendAppendEntriesTo(int toNodeId) {
    int nextIdx = _nextIndex[toNodeId];
    int prevLogIdx = nextIdx - 1;
    int prevLogTerm = (prevLogIdx >= 0 && prevLogIdx < _logs.size()) ? _logs[prevLogIdx].term : 0;

    std::vector<LogEntry> entries;
    if (nextIdx < _logs.size()) {
        entries.insert(entries.end(), _logs.begin() + nextIdx, _logs.end());
    }

    AppendEntries ae{
        _currentTerm,
        _id,
        prevLogIdx,
        prevLogTerm,
        entries,
        _commitIndex
    };

    if (_sendAppendEntries) {
        _sendAppendEntries(toNodeId, ae);
    }
}

