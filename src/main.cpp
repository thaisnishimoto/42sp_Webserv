#include "WebServer.hpp"
#include <exception>
#include <iostream>

int main(void)
{
    try
    {
        WebServer webServer;
        webServer.init();
        webServer.run();
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "General error" << std::endl;
        return 1;
    }
    return 0;
}
