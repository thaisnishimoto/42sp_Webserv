#ifndef _WEBSERVER_HPP_
#define _WEBSERVER_HPP_

#define MAX_EVENTS 10
#define TIMEOUT 60
#define CLIENT_MAX_BODY_SIZE 1048576

#include "Connection.hpp"
#include "Logger.hpp"

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
	std::map<uint16_t, VirtualServer*> _vServersDefault;

	std::map<int, Connection> _connectionsMap;

	//provisory
	std::set<std::string> _implementedMethods;
	std::set<std::string> _unimplementedMethods;

	Logger _logger;


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
	void checkTimeouts(void);
	void setNonBlocking(int fd);

	void parseRequest(Connection& connection);
	void parseRequestLine(std::string& connectionBuffer, Request& request);
	void parseMethod(std::string& requestLine, Request& request);
	void parseTarget(std::string& requestLine, Request& request);
	void parseVersion(std::string& requestLine, Request& request);

	void parseHeader(std::string& connectionBuffer, Request& request);
	void identifyVirtualServer(Connection& connection);
	std::string captureFieldName(std::string& fieldLine);
	std::string captureFieldValues(std::string& fieldLine);
	void validateHeader(Request& request);

	void parseBody(std::string& connectionBuffer, Request& request);

	void buildResponseBuffer(Connection& connection);
	void fillResponse(Connection& connection);

	int consumeNetworkBuffer(int connectionFd, std::string& connectionBuffer);

};

#endif //_WERBSERVER_HPP_
