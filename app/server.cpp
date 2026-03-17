#include "echo_server.h"

int main(int argc, char const *argv[])
{
    SetLogLevel(INFO);
    EchoServer server(8085, std::thread::hardware_concurrency());
    server.Run();
    return 0;
}
