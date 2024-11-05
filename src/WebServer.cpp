#include "WebServer.hpp"

WebServer::WebServer(void)
{
	init();
}

void WebServer::init(void)
{
	VirtualServer server1(8081), server2(8082);

	std::pair<int, VirtualServer&> pair1(server1.getSockFd(), server1);
	std::pair<int, VirtualServer&> pair2(server2.getSockFd(), server2);

	_listeners.insert(pair1);
	_listeners.insert(pair2);
}
