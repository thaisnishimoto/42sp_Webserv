#ifndef _VIRTUALSERVER_HPP_
#define _VIRTUALSERVER_HPP_

#include <iostream>
#include <netdb.h>

class VirtualServer
{
public:
	VirtualServer(int port, std::string name);

	uint16_t _port;
	std::string _name;
};

#endif //_VIRTUALSERVER_HPP_
