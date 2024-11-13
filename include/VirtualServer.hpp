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
private:
	int _port; //maybe change to unsigned short int?
	int _serverFd;
	std::map<int, std::string> _clientBuffers;

public:
	VirtualServer(int port);
	~VirtualServer(void);
	int acceptConnection(int epollFd);
	void processRequest(int connectionFd);
	int getServerFd(void);
	void setUpSocket(void);
	void bindSocket(void);
	void startListening(void);
	void setNonBlocking(int fd);
	// Response handleRequest(Request& request);

};

#endif //_VIRTUALSERVER_HPP_
