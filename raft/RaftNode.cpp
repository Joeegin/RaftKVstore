//
// Created by 乔益金 on 25-6-26.
//

#include "RaftNode.h"
#include <iostream>
#include <regex>

static int randomTimeout() {
    return 1500+std::rand()%1000;  // 1.5-2.5秒的随机超时
}

RaftNode::RaftNode(int id, int totalNodes, std::function<void(int, int,int)> callback)
    :_id(id),_totalNodes(totalNodes),_sendVoteRequest(callback),_kvstore(filename) {
    _electionTimeout =randomTimeout();
    _electionElapsed=0;

}

void RaftNode::tick() {
    _electionElapsed+=10;

    if (_role!=RaftRole::LEADER && _electionElapsed>=_electionTimeout) {
        becomeCandidate();
    }
    // 新增：如果自己是 LEADER，定期发送心跳包
    if (_role == RaftRole::LEADER) {
        _heartbeatElapsed += 10;
        if (_heartbeatElapsed >= _heartbeatInterval) {
            for (int i = 0; i < _totalNodes; ++i) {
                if (i == _id) continue;
                sendAppendEntriesTo(i);
            }
            _heartbeatElapsed = 0;
        }
    }
}

void RaftNode::becomeFollower(int term) {
    _currentTerm=term;
    _role=RaftRole::FOLLOWER;
    _votedFor=-1;
    _votesReceived=0;
    _electionElapsed=0;//重制定时器
    _electionTimeout=randomTimeout();//重新生成一个时间上限

    std::cout << "[Node " << _id << "] Becomes FOLLOWER for term " << _currentTerm << ", leaderId reset to -1" << std::endl;
}

void RaftNode::becomeCandidate() {
    _currentTerm++;//任期加一
    _role=RaftRole::CANDIDATE;
    _votedFor=_id;//给自己投票
    _votesReceived=1;
    _electionElapsed=0;//重制定时器
    _electionTimeout=randomTimeout();
    std::cout << "[Node " << _id << "] Becomes CANDIDATE for term " << _currentTerm << std::endl;

    //向所有节点拉票
    for (int i = 0; i < _totalNodes; ++i) {
        if (i != _id) {
            _sendVoteRequest(_id, i,_currentTerm);
        }
    }
}

void RaftNode::becomeLeader() {
    _role=RaftRole::LEADER;
    _leaderId=_id;//更新leaderid

    _nextIndex = std::vector<int>(_totalNodes, _logs.size());
    _matchIndex = std::vector<int>(_totalNodes, _logs.size()-1);

    // 初始发送空的 AppendEntries 心跳
    for (int i = 0; i < _totalNodes; ++i) {
        if (i == _id) continue;
        sendAppendEntriesTo(i);//发送心跳包
    }
    std::cout << "[Node " << _id << "] Becomes LEADER for term " << _currentTerm << std::endl;
}

void RaftNode::receiveVoteRequest(int term, int candidateId) {
    //收到选举者的拉票请求
    if (term>_currentTerm) {
        becomeFollower(term);//重新变成跟随者
        _votedFor=candidateId;//投给选举者

        std::cout << "[Node " << _id << "] votes for Node " << candidateId << " in term " << term << std::endl;

    }
}

void RaftNode::receiveVoteResponse(int term, bool voteGranted) {
    if (_role!=RaftRole::CANDIDATE || term!=_currentTerm) {
        std::cout << "[Node " << _id << "] Ignoring vote response: role=" << (int)_role << ", term=" << term << ", currentTerm=" << _currentTerm << std::endl;
        return ;
    }
    if (voteGranted) {

        _votesReceived++;//票数加一
        std::cout << "[Node " << _id << "] Received vote, total votes: " << _votesReceived << "/" << (_totalNodes/2 + 1) << std::endl;
        //获得大多数人的票，成为leader
        if (_votesReceived > _totalNodes / 2) {
            becomeLeader();
        }
    } else {
        std::cout << "[Node " << _id << "] Vote denied" << std::endl;
    }
}

void RaftNode::receiveAppendEntries(const AppendEntries &ae, std::function<void(const AppendResponse &)> reply) {
    std::cout << "[Node " << _id << "] Handling AppendEntries from Node " << ae.leaderId
              << " (term=" << ae.term << "), currentTerm=" << _currentTerm << std::endl;

    if (ae.term<_currentTerm) {
        std::cout << "[Node " << _id << "] Rejecting AppendEntries from Node " << ae.leaderId
                      << " due to stale term " << ae.term << " < " << _currentTerm << std::endl;
        reply({_currentTerm, false,ae.leaderId});
        return;
    }

    becomeFollower(ae.term);
    _leaderId=ae.leaderId;
    std::cout << "[Node " << _id << "] Updated leaderId to " << _leaderId << " from AppendEntries" << std::endl;
    _electionElapsed=0;

    //验证前一条日志
    if (ae.prevLogIndex>=0 && (ae.prevLogIndex>=_logs.size() || _logs[ae.prevLogIndex].term !=ae.prevLogTerm)) {
        reply({_currentTerm,false});
        std::cout << "[Node " << _id << "] Received AppendEntries: term=" << ae.term
          << ", leaderId=" << ae.leaderId
          << ", prevLogIndex=" << ae.prevLogIndex
          << ", logs.size=" << _logs.size() << std::endl;

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
    for (int i = 0; i < ae.entries.size(); ++i) {
        _logs.push_back(ae.entries[i]);
    }

    //更新提交日志的索引
    if (ae.leaderCommit > _commitIndex) {
        _commitIndex = std::min((int)_logs.size() - 1, ae.leaderCommit);
        //触发状态机应用日志applyLogs();
    }
    //持久化日志，待实现

    reply({ _currentTerm, true ,ae.leaderId});
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
        _leaderId=fromNodeId;
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

//心跳包以及日志同步
// 在 RaftNode.cpp 中改进 sendAppendEntriesTo
void RaftNode::sendAppendEntriesTo(int toNodeId) {
    int nextIdx = _nextIndex[toNodeId];
    int prevLogIdx = nextIdx - 1;
    int prevLogTerm = (prevLogIdx >= 0 && prevLogIdx < _logs.size()) ? _logs[prevLogIdx].term : 0;

    std::vector<LogEntry> entries;
    if (nextIdx < _logs.size()) {
        entries.insert(entries.end(), _logs.begin() + nextIdx, _logs.end());
    }

    // 如果是心跳包(空日志)，确保prevLogIdx和prevLogTerm正确
    if (entries.empty() && !_logs.empty()) {
        prevLogIdx = _logs.size() - 1;
        prevLogTerm = _logs.back().term;
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

void RaftNode::appendNewCommand(int currentTerm,std::string& op,std::string& key,std::string &value) {
    _kvstore.put(key, value);
    _logs.push_back({currentTerm,op,key,value});
}

std::optional<std::string> RaftNode::getValue(const std::string &key) {
    std::optional<std::string> value=_kvstore.get(key);
    return value;
}
