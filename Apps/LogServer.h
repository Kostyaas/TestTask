#pragma once

#include "LogStatistics.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

class LogServer
{
public:
    LogServer(uint16_t port, int stats_interval, int stats_timeout);
    ~LogServer();
    void run();

private:
    void handleClient(int client_socket);
    void statsWorker();

    uint16_t port_;
    int stats_interval_;
    int stats_timeout_;
    LogStatistics statistics_;
    std::thread stats_thread_;
    std::atomic<bool> stop_{false}; 
    int server_fd_{-1};            
};