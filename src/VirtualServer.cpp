#include "VirtualServer.hpp"

VirtualServer::VirtualServer(int port): _port(port)
{
	_sockFd = socket(AF_INET, SOCK_STREAM, 0);
	//TODO - check for error and how to deal with it.

	struct sockaddr_in	server_address;
	std::memset(&server_address, 0, sizeof(sockaddr_in));
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(_port);

	bind(_sockFd, (struct sockaddr *)&server_address, sizeof(sockaddr_in));
	listen(_sockFd, 10);
}

int VirtualServer::getSockFd(void)
{
	return _sockFd;
}
