//
// Created by 乔益金 on 25-6-27.
//

#ifndef RAFTLOG_H
#define RAFTLOG_H

#include <vector>
#include "../rpc/Message.h"


class RaftLog {
public:
    void Append(const LogEntry& entry);
    const std::vector<LogEntry>& GetLogEntries() const;
    void AppendAll(std::function<void(const std::string&,const std::string&)> applyFunc);
private:
    std::vector<LogEntry> _logs;
};



#endif //RAFTLOG_H
