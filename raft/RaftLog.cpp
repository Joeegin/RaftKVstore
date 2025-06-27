//
// Created by 乔益金 on 25-6-27.
//

#include "RaftLog.h"

void RaftLog::Append(const LogEntry &entry) {
    _logs.push_back(entry);
}

const std::vector<LogEntry> &RaftLog::GetLogEntries() const {
    return _logs;
}

void RaftLog::AppendAll(std::function<void(const std::string &, const std::string &)> applyFunc) {
    for (const LogEntry &entry : _logs) {

    }
}

