//
// Created by 乔益金 on 25-6-26.
//

#ifndef RAFTNODE_H
#define RAFTNODE_H

#include <functional>
#include "../rpc/Message.h"
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

    void appendNewCommand(const std::string& command);
    void sendAppendEntries(int peerId);
    AppendResponse handleAppendEntries(const AppendEntries& req);
    void receiveAppendResponse(int peerId, const AppendResponse& resp);
    void applyLog();

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

    std::vector<LogEntry> _logs;
    int _commitIndex=0;//当前节点认为提交日志的最大索引
    int _lastApplied=0;//最后一个被应用的日志索引

    std::vector<int> _nextIndex;//Leader给每个Follower将要发送的日志索引
    std::vector<int> _matchIndex;//每个Follower记录的已经确认复制的日志索引
};



#endif //RAFTNODE_H
