#include <iostream>
#include <vector>
#include <string>
#include <FileLogger.h>
#include "AppTest.cpp"
#include "LogServer.cpp"
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

void runFileLogger()
{
    auto logger = std::make_shared<FileLogger>(Priority::easy, "log.txt", std::ios::app);
    App app(logger);
    app.Run();
}
void Test1(){
    runFileLogger();
}
void Test2(){
    std::thread serverThread(runServer);
    std::thread clientThread(runClient);
    serverThread.join();
    clientThread.join();
}
int main()
{   
    // Test1();
    Test2();
    return 0;
}