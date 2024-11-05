#include "WebServer.hpp"

WebServer::WebServer(void)
{
	init();
}

void WebServer::init(void)
{
  _epollFd = epoll_create(1);
  
	VirtualServer server1(8081), server2(8082);

	std::pair<int, VirtualServer&> pair1(server1.getSockFd(), server1);
	std::pair<int, VirtualServer&> pair2(server2.getSockFd(), server2);

	_listeners.insert(pair1);
	_listeners.insert(pair2);

  struct epoll_event target_event;
  target_event.events = EPOLLIN;
  target_event.data.fd = server1.getSockFd();
  epoll_ctl(_epollFd, EPOLL_CTL_ADD, server1.getSockFd(), &target_event);
  target_event.data.fd = server2.getSockFd();
  epoll_ctl(_epollFd, EPOLL_CTL_ADD, server2.getSockFd(), &target_event);
}


void WebServer::run(void) {
  int fdsReady;
  while (true)
  {
    std::cout << "Start run WebSever" << std::endl;
    fdsReady = epoll_wait(_epollFd, _eventsList, MAX_EVENTS, -1);
  }
}
