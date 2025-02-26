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
			std::cerr <<"Usage: ./webserv config_file.conf" << std::endl;
			return 1;
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
