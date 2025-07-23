#pragma once
#include <queue>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>
#include <iostream>
#include "FileLogger.h"



struct SMS {
    std::string message;
    Priority priority;
    bool has_priority;
};

class App {
public:
    explicit App(std::shared_ptr<Ilogger> logger_);
    ~App();

    void Run();
    void SetLogger(std::shared_ptr<Ilogger> logger);

private:
    void ProcessConsoleInput(const std::string &input);
    void AddToQueue(const std::string &message, Priority priority, bool has_priority);
    void ProcessQueue();

    std::shared_ptr<Ilogger> logger_;
    std::mutex logger_mutex_;
    std::queue<SMS> queue_;
    std::mutex queue_mutex_;
    std::condition_variable cv_;
    std::atomic<bool> running_;
    std::thread worker_thread_;
};