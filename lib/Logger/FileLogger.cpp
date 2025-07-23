#include "FileLogger.h"
std::string GetPriority(Priority priority)
{
    switch (priority)
    {
    case Priority::easy:
        return "EASY";
    case Priority::medium:
        return "MEDIUM";
    case Priority::hard:
        return "HARD";
    default:
        return "?";
    }
}
std::string getCurrentDateTime()
{
    auto now = std::chrono::system_clock::now();

    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_tm = *std::localtime(&now_time_t);

    std::ostringstream oss;
    oss << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S");

    return oss.str();
}

void FileLogger::InputMessage(const std::string &sms, Priority priority)
{
    if (priority_ > priority)
        return;
    StreamOut_ << "[" << getCurrentDateTime() << "] " << GetPriority(priority) << " " << sms << std::endl;
}

SocketLogger::SocketLogger(Priority priority, const std::string &ip, uint16_t port) : BaseLoger(priority, -1)
{
    StreamOut_ = socket(AF_INET, SOCK_STREAM, 0);
    if (StreamOut_ < 0)
    {
        throw std::runtime_error("Failed to create socket");
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr) <= 0)
    {
        close(StreamOut_);
        throw std::runtime_error("Invalid IP address");
    }

    if (connect(StreamOut_, (sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        close(StreamOut_);
        throw std::runtime_error("Connection failed");
    }
}

void SocketLogger::InputMessage(const std::string &sms, Priority priority)
{
    if (priority_ > priority)
        return;
    std::string meesage = "[" + getCurrentDateTime() + "] " + GetPriority(priority) + " " + sms;
    if (send(StreamOut_, meesage.c_str(), meesage.size(), 0) < 0)
    {
        throw std::runtime_error("Failed to send message");
    }
}
