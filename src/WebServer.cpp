#include "WebServer.hpp"

WebServer::WebServer(void)
{
	init();
}

void WebServer::init(void)
{
  _epollFd = epoll_create(1);
  
	VirtualServer server1(8081), server2(8082);

	_virtualServers.push_back(server1);
	_virtualServers.push_back(server2);

	std::pair<int, VirtualServer*> pair1(server1.getServerFd(), &_virtualServers[0]);
	std::pair<int, VirtualServer*> pair2(server2.getServerFd(), &_virtualServers[1]);

	_listeners.insert(pair1);
	_listeners.insert(pair2);

  struct epoll_event target_event;
  target_event.events = EPOLLIN;
  target_event.data.fd = server1.getServerFd();
  epoll_ctl(_epollFd, EPOLL_CTL_ADD, server1.getServerFd(), &target_event);
  target_event.data.fd = server2.getServerFd();
  epoll_ctl(_epollFd, EPOLL_CTL_ADD, server2.getServerFd(), &target_event);
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
