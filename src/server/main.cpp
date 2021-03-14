#include <asio.hpp>
#include <iostream>
#include "server.h"

int main(int argc, char* argv[])
{
    try
    {
        // Test on commandline with:
        // nc -u localhost 4444
        server serv(4444);
        while(serv.get_running())
        {
            // Busy loop
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    return 0;
}
