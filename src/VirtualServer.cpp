#include "VirtualServer.hpp"

VirtualServer::VirtualServer(int port): _port(port)
{
	_serverFd = socket(AF_INET, SOCK_STREAM, 0);
	//TODO - check for error and how to deal with it.

	struct sockaddr_in	server_address;
	std::memset(&server_address, 0, sizeof(sockaddr_in));
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(_port);

	bind(_serverFd, (struct sockaddr *)&server_address, sizeof(sockaddr_in));
	listen(_serverFd, 10);
}

int VirtualServer::getServerFd(void)
{
	return _serverFd;
}

int VirtualServer::acceptConnection(int epollFd)
{
	struct epoll_event target_event;
	int newFd; 
	std::string	buffer;
	
	newFd = accept(_serverFd, NULL, NULL);
	
	std::pair<int, std::string>	pair(newFd, buffer);
	_clientBuffers.insert(pair);
	target_event.events = EPOLLIN;
	target_event.data.fd = newFd;
	epoll_ctl(epollFd, EPOLL_CTL_ADD, newFd, &target_event);
	
	return newFd;
}
