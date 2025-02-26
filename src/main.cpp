#include "WebServer.hpp"
#include <exception>
#include <iostream>
#include <stdexcept>

int main(int argc, char** argv)
{
    try
    {
        if (argc != 2)
        {
            throw std::runtime_error("Wrong number of argument");
        }
        WebServer webServer(argv[1]);
        webServer.init();
        webServer.run();
    }
    catch (...)
    {
        return 1;
    }
    return 0;
}
