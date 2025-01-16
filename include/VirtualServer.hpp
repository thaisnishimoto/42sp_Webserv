#ifndef _VIRTUALSERVER_HPP_
#define _VIRTUALSERVER_HPP_

#include <iostream>
#include <netdb.h>

class VirtualServer
{
  public:
    VirtualServer(int port, std::string name);

    uint16_t port;
    std::string name;
};

#endif //_VIRTUALSERVER_HPP_
