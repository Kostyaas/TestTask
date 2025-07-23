#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <deque>
#include <algorithm>
#include <atomic>
#include <thread>
#include <mutex>
#include<numeric>
#include <condition_variable>

class LogStatistics {
public:
    struct LogEntry {
        std::chrono::system_clock::time_point timestamp;
        std::string level;
        std::string message;
        size_t length;
    };

    void processMessage(const std::string& raw_message);
    void printStatistics(int nth_message);
    bool hasNewMessages() const;
    void resetNewMessages();
    void waitForTimeout(int timeout_sec);

private:
    std::chrono::system_clock::time_point parseDateTime(const std::string& datetime);

    std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic<bool> new_message_{false};

    int total_messages_ = 0;
    std::unordered_map<std::string, int> level_counts_;
    std::vector<size_t> message_lengths_;
    std::deque<LogEntry> last_hour_messages_;
};