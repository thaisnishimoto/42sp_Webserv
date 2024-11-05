#ifndef _WEBSERVER_HPP_
#define _WEBSERVER_HPP_

#include "VirtualServer.hpp"

#include <vector>
#include <map>

class WebServer
{
private:
	// std::vector<VirtualServer> _virtualServers;
	std::map<int, VirtualServer> _listeners;
	std::map<int, VirtualServer*> _connections;
	void init(void);

public:
	WebServer(void);
	void run(void);
};

#endif //_WERBSERVER_HPP_
