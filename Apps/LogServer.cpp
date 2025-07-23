#include "LogServer.h"

LogServer::LogServer(uint16_t port, int stats_interval, int stats_timeout)
    : port_(port), stats_interval_(stats_interval), stats_timeout_(stats_timeout) {}

void LogServer::run()
{
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0)
    {
        throw std::runtime_error("Failed to create socket");
    }

    int opt = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);

    if (bind(server_fd_, (sockaddr *)&address, sizeof(address)) < 0)
    {
        close(server_fd_);
        throw std::runtime_error("Bind failed");
    }

    if (listen(server_fd_, 3) < 0)
    {
        close(server_fd_);
        throw std::runtime_error("Listen failed");
    }

    std::cout << "Server listening on port " << port_ << std::endl;

    stats_thread_ = std::thread([this]
                                { statsWorker(); });

    while (!stop_)
    {
        sockaddr_in client_addr{};
        socklen_t addr_len = sizeof(client_addr);
        int client_socket = accept(server_fd_, (sockaddr *)&client_addr, &addr_len);

        if (client_socket < 0)
        {
            std::cerr << "Accept failed" << std::endl;
            continue;
        }

        handleClient(client_socket);
        close(client_socket);
    }
}

void LogServer::handleClient(int client_socket)
{
    char buffer[4096];
    int message_count = 0;

    while (!stop_)
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

void LogServer::statsWorker()
{
    while (!stop_)
    {
        statistics_.waitForTimeout(stats_timeout_);
        if (statistics_.hasNewMessages())
        {
            statistics_.printStatistics(-1);
            statistics_.resetNewMessages();
        }
    }
}

LogServer::~LogServer()
{
    stop_ = true;

    if (stats_thread_.joinable())
    {
        stats_thread_.join();
    }

    if (server_fd_ != -1)
    {
        shutdown(server_fd_, SHUT_RDWR);
        close(server_fd_);
    }
}