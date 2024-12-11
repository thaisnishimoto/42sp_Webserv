#ifndef _CONNECTION_HPP_
#define _CONNECTION_HPP_

#include <netdb.h> //uint16_t
#include <iostream> //std::string

#include "Request.hpp"
#include "Response.hpp"
#include "VirtualServer.hpp"

class Connection
{
public:
	int connectionFd;
	uint16_t port;
	std::string buffer;
	Request request;
	Response response;
	VirtualServer* virtualServer;
};

#endif //_CONNECTION_HPP_
