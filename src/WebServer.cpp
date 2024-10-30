#include "WebServer.hpp"
#include <fcntl.h>

WebServer::WebServer(void)
{
}

WebServer::~WebServer(void)
{
	for (size_t i = 0; i < _virtualServers.size(); ++i)
	{
		delete _virtualServers[i];
	}
	if (_epollFd > 2)
	{
		close(_epollFd);
	}
}

void WebServer::init(void)
{
	_epollFd = epoll_create(1);
	if (_epollFd == -1)
	{
		throw std::runtime_error("Server error: Could not create epoll instance");
	}

	_virtualServers.push_back(new VirtualServer(8081));
	_virtualServers.push_back(new VirtualServer(8082));

	for (size_t i = 0; i < _virtualServers.size(); ++i)
	{
		int serverFd = _virtualServers[i]->getServerFd();
		std::pair<int, VirtualServer*> pair(serverFd, _virtualServers[i]);
		_listeners.insert(pair);
		struct epoll_event target_event;
		std::memset(&target_event, 0, sizeof(target_event));
		target_event.events = EPOLLIN;
		target_event.data.fd = serverFd;
		if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, serverFd, &target_event) == -1)
		{
			throw std::runtime_error("Server Error: Could not add fd to epoll instance");
		}
	}
}

void WebServer::run(void)
{
	int fdsReady;
	struct epoll_event _eventsList[MAX_EVENTS];

	std::cout << "Main loop initiating..." << std::endl;
	while (true)
	{
		fdsReady = epoll_wait(_epollFd, _eventsList, MAX_EVENTS, -1);
		if (fdsReady == -1)
		{
			//TODO: deal with EINTR. (when signal is received during wait)
			std::cerr << std::strerror(errno) << std::endl;
			throw std::runtime_error("Server Error: could not create socket");
		}

		for (int i = 0; i < fdsReady; i++)
		{
			int	eventFd = _eventsList[i].data.fd;
			if (_listeners.count(eventFd) == 1)
			{
				VirtualServer*	ptr = _listeners[eventFd];
				int	connectionFd = ptr->acceptConnection(_epollFd);
				if (connectionFd == -1)
				{
				    //TODO: print to user log
				    std::cerr << "Accept connection failed" << std::endl;
				}
				std::pair<int, VirtualServer*>	pair(connectionFd, ptr);
				_connections.insert(pair);
			}
			else if (_connections.count(eventFd) == 1 &&
				(_eventsList[i].events & EPOLLIN) == EPOLLIN)
			{
				VirtualServer* ptr = _connections[eventFd];
				ptr->processRequest(eventFd);
			}
		}
	}
}
