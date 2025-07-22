#include <iostream>
#include <vector>
#include <string>
#include <FileLogger.h>
#include "AppTest.cpp"
#include "SocketApp.cpp"
#include <thread>
#include <chrono>

void runServer()
{
    try
    {
        LogServer server(8080, 5, 100);
        server.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Ошибка сервера: " << e.what() << "\n";
    }
}

void runClient()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    try
    {
        auto logger = std::make_shared<SocketLogger>(Priority::easy, "127.0.0.1", 8080);
        App app(logger);
        app.Run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Ошибка клиента: " << e.what() << "\n";
    }
}

int main()
{
    std::thread serverThread(runServer);
    std::thread clientThread(runClient);

    serverThread.join();
    clientThread.join();
    

    return 0;
}