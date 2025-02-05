#ifndef _WEBSERVER_HPP_
#define _WEBSERVER_HPP_

#define MAX_EVENTS 10
#define TIMEOUT 60

#include "Config.hpp"
#include "Connection.hpp"
#include "Logger.hpp"
#include "VirtualServer.hpp"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <map>
#include <netinet/in.h>
#include <set>
#include <sstream>
#include <stdexcept>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

class WebServer
{
  private:
    int _epollFd;
    Config _config;
    std::set<uint16_t> _ports;
    std::map<int, std::pair<uint32_t, uint16_t> > _socketsToPairs;
	//delete line below?
    std::vector<VirtualServer> _virtualServers;
    std::map<std::pair<uint32_t, uint16_t>,
             std::map<std::string, VirtualServer> >
        _virtualServersLookup;
    std::map<std::pair<uint32_t, uint16_t>, VirtualServer*>
        _virtualServersDefault;

    std::map<int, Connection> _connectionsMap;

    // provisory
    std::set<std::string> _implementedMethods;
    std::set<std::string> _unimplementedMethods;

    Logger _logger;

  public:
    WebServer(const std::string& configFile);
    ~WebServer(void);
    void init(void);
    void run(void);

    void setUpSockets(void);
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
    void validateHeader(Connection& connection);

	void parseBody(Connection& connection);

    void buildResponseBuffer(Connection& connection);
    void fillResponse(Connection& connection);
	void handleGET(Connection& connection);
	void handlePOST(Connection& connection);

    int consumeNetworkBuffer(int connectionFd, std::string& connectionBuffer);
};

#endif //_WERBSERVER_HPP_
