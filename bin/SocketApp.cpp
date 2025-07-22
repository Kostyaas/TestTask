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
#include <condition_variable>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <numeric>

class LogStatistics
{
public:
    struct LogEntry
    {
        std::chrono::system_clock::time_point timestamp;
        std::string level;
        std::string message;
        size_t length;
    };

    void processMessage(const std::string &raw_message)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        LogEntry entry;
        entry.length = raw_message.length();

        size_t date_end = raw_message.find(']');
        if (date_end != std::string::npos && raw_message[0] == '[')
        {
            std::string datetime = raw_message.substr(1, date_end - 1);
            entry.timestamp = parseDateTime(datetime);

            size_t level_start = date_end + 2; // Пропускаем "] "
            size_t level_end = raw_message.find(' ', level_start);
            if (level_end != std::string::npos)
            {
                entry.level = raw_message.substr(level_start, level_end - level_start);
                entry.message = raw_message.substr(level_end + 1);
            }
        }

        total_messages_++;
        level_counts_[entry.level]++;
        message_lengths_.push_back(entry.length);
        last_hour_messages_.push_back(entry);

        auto one_hour_ago = std::chrono::system_clock::now() - std::chrono::hours(1);
        while (!last_hour_messages_.empty() && last_hour_messages_.front().timestamp < one_hour_ago)
        {
            last_hour_messages_.pop_front();
        }

        new_message_ = true;
    }

    void printStatistics(int nth_message)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        std::cout << "\n=== Statistics after " << nth_message << " messages ===" << std::endl;
        std::cout << "Total messages: " << total_messages_ << std::endl;

        // Статистика по уровням
        std::cout << "\nMessages by level:" << std::endl;
        for (const auto &[level, count] : level_counts_)
        {
            std::cout << level << ": " << count << std::endl;
        }

        std::cout << "\nMessages in last hour: " << last_hour_messages_.size() << std::endl;

        if (!message_lengths_.empty())
        {
            auto [min_it, max_it] = std::minmax_element(message_lengths_.begin(), message_lengths_.end());
            double avg_length = std::accumulate(message_lengths_.begin(), message_lengths_.end(), 0.0) / message_lengths_.size();

            std::cout << "\nMessage lengths:" << std::endl;
            std::cout << "Min: " << *min_it << " chars" << std::endl;
            std::cout << "Max: " << *max_it << " chars" << std::endl;
            std::cout << "Avg: " << avg_length << " chars" << std::endl;
        }

        std::cout << "========================\n"
                  << std::endl;
    }

    bool hasNewMessages() const { return new_message_; }
    void resetNewMessages() { new_message_ = false; }

    void waitForTimeout(int timeout_sec)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait_for(lock, std::chrono::seconds(timeout_sec), [this]
                     { return new_message_.load(); });
    }

private:
    std::chrono::system_clock::time_point parseDateTime(const std::string &datetime)
    {
        std::tm tm = {};
        std::istringstream ss(datetime);
        ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
        if (ss.fail())
        {
            return std::chrono::system_clock::now();
        }
        return std::chrono::system_clock::from_time_t(std::mktime(&tm));
    }

    std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic<bool> new_message_{false};

    int total_messages_ = 0;
    std::unordered_map<std::string, int> level_counts_;
    std::vector<size_t> message_lengths_;
    std::deque<LogEntry> last_hour_messages_;
};

class LogServer
{
public:
    LogServer(uint16_t port, int stats_interval, int stats_timeout)
        : port_(port), stats_interval_(stats_interval), stats_timeout_(stats_timeout) {}

    void run()
    {
        int server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0)
        {
            throw std::runtime_error("Failed to create socket");
        }

        int opt = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port_);

        if (bind(server_fd, (sockaddr *)&address, sizeof(address)) < 0)
        {
            close(server_fd);
            throw std::runtime_error("Bind failed");
        }

        if (listen(server_fd, 3) < 0)
        {
            close(server_fd);
            throw std::runtime_error("Listen failed");
        }

        std::cout << "Server listening on port " << port_ << std::endl;

        std::thread stats_thread([this]
                                 { statsWorker(); });

        while (true)
        {
            sockaddr_in client_addr{};
            socklen_t addr_len = sizeof(client_addr);
            int client_socket = accept(server_fd, (sockaddr *)&client_addr, &addr_len);

            if (client_socket < 0)
            {
                std::cerr << "Accept failed" << std::endl;
                continue;
            }

            handleClient(client_socket);
            close(client_socket);
        }

        stats_thread.join();
        close(server_fd);
    }

private:
    void handleClient(int client_socket)
    {
        char buffer[4096];
        int message_count = 0;

        while (true)
        {
            memset(buffer, 0, sizeof(buffer));
            int bytes_read = recv(client_socket, buffer, sizeof(buffer), 0);

            if (bytes_read <= 0)
            {
                break;
            }

            std::string message(buffer, bytes_read);
            statistics_.processMessage(message);

            std::cout << "Received: " << message << std::endl;

            if (((message_count++) % stats_interval_) == 0)
            {
                statistics_.printStatistics(message_count);
            }
        }
    }

    void statsWorker()
    {
        while (true)
        {
            statistics_.waitForTimeout(stats_timeout_);
            if (statistics_.hasNewMessages())
            {
                statistics_.printStatistics(-1);
                statistics_.resetNewMessages();
            }
        }
    }

    uint16_t port_;
    int stats_interval_;
    int stats_timeout_;
    LogStatistics statistics_;
};
