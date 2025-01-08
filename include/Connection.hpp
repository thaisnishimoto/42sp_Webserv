#ifndef _CONNECTION_HPP_
#define _CONNECTION_HPP_

#include <netdb.h> //uint16_t
#include <ctime> //time_t
#include <iostream> //std::string

#include "Request.hpp"
#include "Response.hpp"
#include "VirtualServer.hpp"

class Connection
{
public:
	int connectionFd;
	uint16_t port;
	uint32_t host;
	std::string buffer;
	Request request;
	Response response;
	VirtualServer* virtualServer;

	bool error;
	time_t lastActivity;

	Connection(void);
	Connection(int fd);
};

#endif //_CONNECTION_HPP_
