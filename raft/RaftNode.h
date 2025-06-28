//
// Created by 乔益金 on 25-6-26.
//

#ifndef RAFTNODE_H
#define RAFTNODE_H

#include <functional>
#include "../rpc/Message.h"
#include "../kvstore/KVStore.h"

enum class RaftRole {
    FOLLOWER,
    CANDIDATE,
    LEADER,
};
class RaftNode {
public:
    RaftNode(int id,int totalNodes,std::function<void(int,int)>);

    void tick();//模拟时钟，用于超时检测和心跳检测
    void receiveVoteRequest(int term,int candidateId);//处理收到的投票请求
    void receiveVoteResponse(int term,bool voteGranted);//处理收到的投票响应

    int getId() const{ return _id;};
    RaftRole getRole() const{ return _role;};
    int getCurrentTerm() const { return _currentTerm; };

    void receiveAppendEntries(const AppendEntries& ae, std::function<void(const AppendResponse&)> reply);
    std::vector<LogEntry> getUncommittedEntries() const;
    void commitTo(int index, KVStore& store);

    //Leader广播AppendEntries
    void appendEntries(const std::string& op,const std::string& key,const std::string& value);
    void setAppendEntriesCallback(std::function<void(int,const AppendEntries&)> cb){ _appendEntriesCallback = cb;};

    void sendAppendEntriesTo(int toNodeId);
    std::function<void(int toNodeId, const AppendEntries&)> _sendAppendEntries;
    //待实现日志响应
    void receiveAppendResponse(int fromNodeId,int term,bool success);
private:
    //选举状态转换
    void becomeFollower(int term);
    void becomeCandidate();
    void becomeLeader();

    int _id;//节点id
    int _totalNodes;//集群节点数量
    int _currentTerm=0;//当前任期
    int _votedFor=-1;//当前任期投票给哪个节点，-1代表未投票

    int _votesReceived=0;//当前任期收到的投票数

    int _electionTimeout;//选举的时间上限
    int _electionElapsed;//当前累计时间，如果超过选举时间上限并且没有收到Leader心跳，就触发一次选举

    RaftRole _role=RaftRole::FOLLOWER;

    std::function<void (int from,int to)> _sendVoteRequest;//发送投票请求的回调函数

    int _commitIndex=-1;//已经提交的日志的索引
    int _lastCommitIndex=-1;

    std::vector<int> _nextIndex;//Leader给每个Follower将要发送的日志索引
    std::vector<int> _matchIndex;//每个Follower记录的已经确认复制的日志索引

    std::vector<LogEntry> _logs;

    std::function<void(int,const AppendEntries&)> _appendEntriesCallback;

    //KVStore _kvstore;
};



#endif //RAFTNODE_H
