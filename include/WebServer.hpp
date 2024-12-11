#ifndef _WEBSERVER_HPP_
#define _WEBSERVER_HPP_

#define MAX_EVENTS 10

#include "Connection.hpp"

#include <stdexcept>
#include <vector>
#include <sys/epoll.h>
#include <map>
#include <set>
#include <sstream>
#include <cerrno>
#include <cstdio>
#include <stdexcept>
#include <fcntl.h>
#include <cstring>
#include <unistd.h>
#include <errno.h>

class WebServer
{
private:
	int _epollFd;
	std::set<uint16_t> _ports;
	std::map<int, uint16_t> _socketsToPorts;
	std::vector<VirtualServer> _virtualServers;
	std::map<std::pair<uint32_t, uint16_t>, std::map<std::string, VirtualServer*> > _vServersLookup;

	std::map<int, Connection> _connectionsMap;

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

	void addSocketsToEpoll(void);
	void modifyEventInterest(int epollFd, int eventFd, uint32_t event);

	int acceptConnection(int epollFd, int eventFd);
	void setNonBlocking(int fd);

	void parseRequest(Connection& connection);
	void parseRequestLine(std::string& connectionBuffer, Request& request);
	void parseMethod(std::string& requestLine, Request& request);
	void parseTarget(std::string& requestLine, Request& request);
	void parseVersion(std::string& requestLine, Request& request);

	void parseHeader(std::string& connectionBuffer, Request& request);
	std::string captureFieldName(std::string& fieldLine);
	std::string captureFieldValues(std::string& fieldLine);
	void validateHeader(Request& request);

	void fillResponse(Response& response, Request& request);

	int consumeNetworkBuffer(int connectionFd, std::string& connectionBuffer);
};

#endif //_WERBSERVER_HPP_
