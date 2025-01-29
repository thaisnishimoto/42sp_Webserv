#ifndef _CONNECTION_HPP_
#define _CONNECTION_HPP_

#include <cerrno>
#include <cstring>
#include <ctime>   //time_t
#include <netdb.h> //uint16_t
#include <sys/socket.h>

#include "Request.hpp"
#include "Response.hpp"
#include "VirtualServer.hpp"

class Connection
{
  public:
    int connectionFd;
    uint16_t port;
    uint32_t host;
    // TODO
    // rename buffer to requestBuffer
    std::string buffer;
    std::string responseBuffer;
    Request request;
    Response response;
    VirtualServer* virtualServer;
    Location* location;

    bool error;
    time_t lastActivity;

    Connection(void);
    Connection(int fd);
};

#endif //_CONNECTION_HPP_
