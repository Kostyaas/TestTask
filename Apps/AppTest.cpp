#include "AppTest.h"

App::App(std::shared_ptr<Ilogger> logger_)
    : logger_(logger_),
      running_(true),
      worker_thread_(&App::ProcessQueue, this) {}

App::~App() {
    running_ = false;
    cv_.notify_all();
    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }
}

void App::Run() {
    std::string input;
    while (running_) {
        std::cout << "Жду сообщений от" << logger_->getClassName() << " : ";
        std::getline(std::cin, input);
        std::cout << std::endl;
        if (input.empty()) {
            continue;
        }

        if (input == "exit") {
            running_ = false;
            break;
        }

        ProcessConsoleInput(input);
    }
    cv_.notify_one();
}

void App::SetLogger(std::shared_ptr<Ilogger> logger) {
    std::lock_guard<std::mutex> lock(logger_mutex_);
    logger_ = std::move(logger);
}

void App::ProcessConsoleInput(const std::string &input) {
    size_t last_space = input.rfind(' ');
    if (last_space == std::string::npos) {
        AddToQueue(input, Priority::easy, false);
        return;
    }

    std::string potential_priority = input.substr(last_space + 1);
    Priority priority;
    bool has_priority = false;
    
    if (potential_priority == "easy") {
        priority = Priority::easy;
        has_priority = true;
    }
    else if (potential_priority == "medium") {
        priority = Priority::medium;
        has_priority = true;
    }
    else if (potential_priority == "hard") {
        priority = Priority::hard;
        has_priority = true;
    }

    if (has_priority) {
        std::string message = input.substr(0, last_space);
        AddToQueue(message, priority, true);
    }
    else {
        AddToQueue(input, Priority::easy, false);
    }
}

void App::AddToQueue(const std::string &message, Priority priority, bool has_priority) {
    std::string trimmed = message;
    trimmed.erase(0, trimmed.find_first_not_of(" \t"));
    trimmed.erase(trimmed.find_last_not_of(" \t") + 1);

    if (trimmed.empty()) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        queue_.push({trimmed, priority, has_priority});
    }
    cv_.notify_one();
}

void App::ProcessQueue() {
    while (running_ || !queue_.empty()) {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        cv_.wait(lock, [this]() { return !queue_.empty() || !running_; });

        if (!queue_.empty()) {
            SMS sms = queue_.front();
            queue_.pop();
            lock.unlock();

            std::lock_guard<std::mutex> logger_lock(logger_mutex_);
            if (sms.has_priority) {
                logger_->InputMessage(sms.message, sms.priority);
            }
            else {
                logger_->InputMessage(sms.message);
            }
        }
    }
}