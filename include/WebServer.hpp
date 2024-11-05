#ifndef _WEBSERVER_HPP_
#define _WEBSERVER_HPP_

#define MAX_EVENTS 10
#include "VirtualServer.hpp"

#include <vector>
#include <sys/epoll.h>
#include <map>


class WebServer
{
private:
	// std::vector<VirtualServer> _virtualServers;
  int _epollFd;
	std::map<int, VirtualServer> _listeners;
	std::map<int, VirtualServer*> _connections;
	void init(void);

public:
	WebServer(void);
	void run(void);
};

#endif //_WERBSERVER_HPP_
