#ifndef _WEBSERVER_HPP_
#define _WEBSERVER_HPP_

#define MAX_EVENTS 10

#include "VirtualServer.hpp"
#include "Request.hpp"
#include "Response.hpp"

#include <stdexcept>
#include <vector>
#include <sys/epoll.h>
#include <map>
#include <cerrno>
#include <cstdio>
#include <stdexcept>
#include <fcntl.h>

class WebServer
{
private:
	int _epollFd;
	//std::set<uint16_t> _ports;
	std::vector<VirtualServer> _virtualServers;
	std::map<int, std::string> _connectionBuffers;
	std::map<int, Request> _requestMap;
	std::map<int, Response> _requestResponse;
	std::map<int, VirtualServer&> _targetVirtualServers;

	//provisory
	static std::set<std::string> _otherMethods;
	static std::set<std::string> _implementedMethods;

public:
	WebServer(void);
	~WebServer(void);
	void run(void);
	void init(void);
};

#endif //_WERBSERVER_HPP_
