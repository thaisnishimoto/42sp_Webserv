#include "VirtualServer.hpp"

//provisorio
VirtualServer::VirtualServer(void)
{
	defaultVirtualServer = false;
}

VirtualServer::VirtualServer(int port, std::string name)
    : port(port), name(name)
{
}
