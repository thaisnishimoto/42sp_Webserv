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
	std::set<uint16_t> _ports;
	std::map<uint16_t, int> _portsToSockets;
	std::vector<VirtualServer> _virtualServers;
	std::map<int, std::string> _connectionBuffers;
	std::map<int, Request> _requestMap;
	std::map<int, Response> _requestResponse;
	std::map<int, VirtualServer&> _targetVirtualServers;

	//provisory
	std::set<std::string> _implementedMethods;
	std::set<std::string> _unimplementedMethods;


public:
	WebServer(void);
	~WebServer(void);
	void init(void);
	void run(void);

	void setUpSockets(std::set<uint16_t> ports);
	void bindSocket(void);
	void startListening(void);
};

#endif //_WERBSERVER_HPP_
