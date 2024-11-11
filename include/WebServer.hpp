#ifndef _WEBSERVER_HPP_
#define _WEBSERVER_HPP_

#define MAX_EVENTS 10
#include "VirtualServer.hpp"
#include <stdexcept>
#include <vector>
#include <sys/epoll.h>
#include <map>


class WebServer
{
private:
	std::vector<VirtualServer> _virtualServers;
	int _epollFd;
	std::map<int, VirtualServer*> _listeners;
	std::map<int, VirtualServer*> _connections;

public:
	WebServer(void);
	~WebServer(void);
	void run(void);
	void init(void);
};

#endif //_WERBSERVER_HPP_
