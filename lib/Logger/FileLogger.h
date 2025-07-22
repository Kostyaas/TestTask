#pragma once
#include <string>
#include <utility>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

enum class Priority
{
    easy,
    medium,
    hard
};

class Ilogger
{
public:
    Ilogger() = default;

    virtual void InputMessage(const std::string &sms, Priority priority) = 0;
    virtual void InputMessage(const std::string &sms) = 0;
    virtual ~Ilogger() = default;
};

template <class TypeOut_>
class BaseLoger : public Ilogger
{
public:
    template <class... Args>
    BaseLoger(Priority priority, Args... Parametr) : StreamOut_(std::forward<Args>(Parametr)...), priority_(priority) {}
    void InputMessage(const std::string &sms, Priority priority) override = 0;
    void InputMessage(const std::string &sms) override
    {
        InputMessage(sms, priority_);
    }
    void UpdatePriority(Priority priority_)
    {
        this->priority_ = priority_;
    }

protected:
    TypeOut_ StreamOut_;
    Priority priority_;
};

class FileLogger : public BaseLoger<std::ofstream>
{
public:
    template <class... Args>
    FileLogger(Priority priority, Args... Parametr) : BaseLoger(priority, std::forward<Args>(Parametr)...) {}
    void InputMessage(const std::string &sms, Priority priority) override;
    void InputMessage(const std::string &sms) override
    {
        BaseLoger::InputMessage(sms);
    }
};

class SocketLogger : public BaseLoger<int>
{
public:
    SocketLogger(Priority priority, const std::string &ip, uint16_t port);
    void InputMessage(const std::string &sms, Priority priority) override;
    void InputMessage(const std::string &sms) override
    {
        BaseLoger::InputMessage(sms);
    }
    ~SocketLogger() override
    {
        if (StreamOut_ >= 0)
        {
            close(StreamOut_);
        }
    }
};