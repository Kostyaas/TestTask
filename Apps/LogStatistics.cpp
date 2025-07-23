#include "LogStatistics.h"

void LogStatistics::processMessage(const std::string& raw_message) {
    std::lock_guard<std::mutex> lock(mutex_);
    LogEntry entry;
    entry.length = raw_message.length();

    size_t date_end = raw_message.find(']');
    if (date_end != std::string::npos && raw_message[0] == '[') {
        std::string datetime = raw_message.substr(1, date_end - 1);
        entry.timestamp = parseDateTime(datetime);

        size_t level_start = date_end + 2;
        size_t level_end = raw_message.find(' ', level_start);
        if (level_end != std::string::npos) {
            entry.level = raw_message.substr(level_start, level_end - level_start);
            entry.message = raw_message.substr(level_end + 1);
        }
    }

    total_messages_++;
    level_counts_[entry.level]++;
    message_lengths_.push_back(entry.length);
    last_hour_messages_.push_back(entry);

    auto one_hour_ago = std::chrono::system_clock::now() - std::chrono::hours(1);
    while (!last_hour_messages_.empty() && last_hour_messages_.front().timestamp < one_hour_ago) {
        last_hour_messages_.pop_front();
    }

    new_message_ = true;
}

void LogStatistics::printStatistics(int nth_message) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::cout << "\n=== Statistics after " << nth_message << " messages ===" << std::endl;
    std::cout << "Total messages: " << total_messages_ << std::endl;

    std::cout << "\nMessages by level:" << std::endl;
    for (const auto& [level, count] : level_counts_) {
        std::cout << level << ": " << count << std::endl;
    }

    std::cout << "\nMessages in last hour: " << last_hour_messages_.size() << std::endl;

    if (!message_lengths_.empty()) {
        auto [min_it, max_it] = std::minmax_element(message_lengths_.begin(), message_lengths_.end());
        double avg_length = std::accumulate(message_lengths_.begin(), message_lengths_.end(), 0.0) / message_lengths_.size();

        std::cout << "\nMessage lengths:" << std::endl;
        std::cout << "Min: " << *min_it << " chars" << std::endl;
        std::cout << "Max: " << *max_it << " chars" << std::endl;
        std::cout << "Avg: " << avg_length << " chars" << std::endl;
    }

    std::cout << "========================\n" << std::endl;
}

bool LogStatistics::hasNewMessages() const {
    return new_message_;
}

void LogStatistics::resetNewMessages() {
    new_message_ = false;
}

void LogStatistics::waitForTimeout(int timeout_sec) {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait_for(lock, std::chrono::seconds(timeout_sec), [this] { 
        return new_message_.load(); 
    });
}

std::chrono::system_clock::time_point LogStatistics::parseDateTime(const std::string& datetime) {
    std::tm tm = {};
    std::istringstream ss(datetime);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    if (ss.fail()) {
        return std::chrono::system_clock::now();
    }
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}
