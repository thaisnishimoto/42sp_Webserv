#ifndef _VIRTUALSERVER_HPP_
#define _VIRTUALSERVER_HPP_

#include <iostream>
#include <map>
#include <sys/socket.h>
#include <netdb.h>
#include <cstring>

class VirtualServer
{
private:
	int _port; //maybe change to unsigned short int?
	int _sockFd;
	std::map<int, std::string&> _clientBuffers;

public:
	VirtualServer(int port);
	void acceptConncetion(int sockFd, int epollFd);
	void processRequest(int connectionFd);
	int getSockFd(void);
	// Response handleRequest(Request& request);

};

#endif //_VIRTUALSERVER_HPP_
