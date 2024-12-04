#ifndef _VIRTUALSERVER_HPP_
#define _VIRTUALSERVER_HPP_

#include <iostream>
#include <map>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netdb.h>
#include <cstring>
#include <unistd.h>
#include <errno.h>

class VirtualServer
{
public:
	VirtualServer(int port, std::string name);
	~VirtualServer(void);

	uint16_t _port;
	std::string _name;

// public:
// void processRequest(int connectionFd);
// Response handleRequest(Request& request);
};

#endif //_VIRTUALSERVER_HPP_
