#include "WebServer.hpp"
#include "VirtualServer.hpp"
#include <cerrno>
#include <cstdio>
#include <stdexcept>

WebServer::WebServer(void)
{
}


WebServer::~WebServer(void)
{
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

	VirtualServer server1(8081), server2(8082);
	_virtualServers.push_back(server1);
	_virtualServers.push_back(server2);

	for (size_t i = 0; i < _virtualServers.size(); ++i)
	{
	   int serverFd =  _virtualServers[i].getServerFd();
	   std::pair<int, VirtualServer*> pair(serverFd, &_virtualServers[i]);
		_listeners.insert(pair);
		struct epoll_event target_event;
		target_event.events = EPOLLIN;
        target_event.data.fd = serverFd;
       if(epoll_ctl(_epollFd, EPOLL_CTL_ADD, serverFd,&target_event) == -1)
       {
           std::cout << errno << std::endl;
           throw std::runtime_error("Server error: Could not add fd to epoll instance");
       }
	}
}


void WebServer::run(void)
{
  int fdsReady;
  struct epoll_event _eventsList[MAX_EVENTS];

  while (true)
  {
	std::cout << "Start run WebSever" << std::endl;
	fdsReady = epoll_wait(_epollFd, _eventsList, MAX_EVENTS, -1);
	for (int i = 0; i < fdsReady; i++)
	{
		int	eventFd = _eventsList[i].data.fd;
		if (_listeners.count(eventFd) == 1)
		{
			VirtualServer*	ptr = _listeners[eventFd];
			int	connectionFd = ptr->acceptConnection(_epollFd);
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
