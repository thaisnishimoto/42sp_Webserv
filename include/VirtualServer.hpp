#ifndef _VIRTUALSERVER_HPP_
#define _VIRTUALSERVER_HPP_

#include <iostream>
#include <netdb.h>
#include <vector>
#include <map>

class Location
{
  public:
    std::vector<std::string> allowedMethods;
    std::string root;
    std::string redirect;
    bool autoindex;
    bool cgi;
};

class VirtualServer
{
  public:
    VirtualServer(void);
    VirtualServer(int port, std::string name);

    uint32_t host;
    uint16_t port;
    std::string name;
    size_t clientMaxBodySize;
    std::map<std::string, std::string> errorPages;
    std::map<std::string, Location> locations;
    bool defaultVirtualServer;
};

#endif //_VIRTUALSERVER_HPP_
