#include "WebServer.hpp"
#include <fcntl.h>

WebServer::WebServer(void)
{
    _implementedMethods.insert("GET");
    _implementedMethods.insert("POST");
    _implementedMethods.insert("DELETE");

    _unimplementedMethods.insert("HEAD");
    _unimplementedMethods.insert("PUT");
    _unimplementedMethods.insert("CONNECT");
    _unimplementedMethods.insert("OPTIONS");
    _unimplementedMethods.insert("TRACE");

	//hard coded ports
	_ports.insert(8081);
	_ports.insert(8082);
}

WebServer::~WebServer(void)
{
	for (size_t i = 0; i < _virtualServers.size(); ++i)
	{
		delete _virtualServers[i];
	}
	if (_epollFd > 2)
	{
		close(_epollFd);
	}
}

void WebServer::init(void)
{
	_epollFd = epoll_create(1);
	if (_epollFd == -1)
	{
		throw std::runtime_error("Server error: Could not create epoll instance");
	}

	setUpSockets(_ports);
	bindSocket();
	startListening();

	addSocketsToEpoll();

	//hard coded as fuck
	_virtualServers.push_back(VirtualServer(8081, "Server1"));
	_virtualServers.push_back(VirtualServer(8081, "Server2"));
	_virtualServers.push_back(VirtualServer(8082, "Server3"));
}

void WebServer::addSocketsToEpoll(void)
{
	std::map<uint16_t, int>::iterator it = _portsToSockets.begin();
	std::map<uint16_t, int>::iterator ite = _portsToSockets.end();

	while (it != ite)
	{
		int sockFd = it->second;
		struct epoll_event target_event;
		std::memset(&target_event, 0, sizeof(target_event));
		target_event.events = EPOLLIN;
		target_event.data.fd = sockFd;
		if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, sockFd, &target_event) == -1)
		{
			throw std::runtime_error("Server Error: Could not add fd to epoll instance");
		}

		it++;
	}
}

void WebServer::setUpSockets(std::set<uint16_t> ports)
{
	std::set<uint16_t>::iterator it = ports.begin();
	std::set<uint16_t>::iterator ite = ports.end();
	
	while (it != ite)
	{
		int sockFd;
		sockFd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
		if (sockFd == -1)
		{
			std::cerr << std::strerror(errno) << std::endl;
			throw std::runtime_error("Virtual Server Error: could not create socket");
		}
		int opt = 1;
    	if (setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
		{
			std::cerr << std::strerror(errno) << std::endl;
			throw std::runtime_error("Virtual Server Error: Could not set socket option");
		};
		_socketsToPorts[sockFd] = *it;
		it++;
	}
}

void WebServer::bindSocket(void)
{
	std::map<int, uint16_t>::iterator it = _socketsToPorts.begin();
	std::map<int, uint16_t>::iterator ite = _socketsToPorts.end();
	
	while (it != ite)
	{
		int sockFd = it->first;
		struct sockaddr_in	server_address;
		std::memset(&server_address, 0, sizeof(sockaddr_in));
		server_address.sin_addr.s_addr = INADDR_ANY;
		server_address.sin_family = AF_INET;
		server_address.sin_port = htons(it->second);

		if (bind(sockFd, (struct sockaddr *)&server_address, sizeof(sockaddr_in)) == -1)
		{
			close(sockFd);
			std::cerr << std::strerror(errno) << std::endl;
			throw std::runtime_error("Virtual Server Error: could not bind socket");
		};
		it++;
	}
}

void WebServer::startListening(void)
{
	std::map<int, uint16_t>::iterator it = _socketsToPorts.begin();
	std::map<int, uint16_t>::iterator ite = _socketsToPorts.end();
	
	while (it != ite)
	{
		int sockFd = it->first; 
		if (listen(sockFd, 10) == -1)
		{
			close(sockFd);
			std::cerr << std::strerror(errno) << std::endl;
			throw std::runtime_error("Virtual Server Error: could not activate listening");
		};
		it++;
	}
}

void WebServer::run(void)
{
	int fdsReady;
	struct epoll_event _eventsList[MAX_EVENTS];

	std::cout << "Main loop initiating..." << std::endl;
	while (true)
	{
		fdsReady = epoll_wait(_epollFd, _eventsList, MAX_EVENTS, -1);
		if (fdsReady == -1)
		{
			//TODO: deal with EINTR. (when signal is received during wait)
			std::cerr << std::strerror(errno) << std::endl;
			throw std::runtime_error("Server Error: could not create socket");
		}

		for (int i = 0; i < fdsReady; i++)
		{
			int	eventFd = _eventsList[i].data.fd;
			if (_socketsToPorts.count(eventFd) == 1)
			{
				int	connectionFd = acceptConnection(_epollFd, eventFd);
				if (connectionFd == -1)
				{
				    //TODO: print to user log
				    std::cerr << "Accept connection failed" << std::endl;
				}
			}
			else if (_connections.count(eventFd) == 1 &&
				(_eventsList[i].events & EPOLLIN) == EPOLLIN)
			{
				VirtualServer* ptr = _connections[eventFd];
				ptr->processRequest(eventFd);
			}
		}
	}
}

int WebServer::acceptConnection(int epollFd, int eventFd)
{
	struct epoll_event target_event;
	int newFd;
	std::string	buffer;

	newFd = accept(eventFd, NULL, NULL);
	if (newFd != -1)
	{
	    setNonBlocking(newFd);
    	std::pair<int, std::string>	pair(newFd, buffer);
    	_connectionBuffers.insert(pair);
    	target_event.events = EPOLLIN;
    	target_event.data.fd = newFd;
    	epoll_ctl(epollFd, EPOLL_CTL_ADD, newFd, &target_event);
	}
	return newFd;
}

void WebServer::setNonBlocking(int fd)
{
   int flag = fcntl(fd, F_GETFL);
   if (flag < 0)
   {
       std::cerr << std::strerror(errno) << std::endl;
       throw std::runtime_error("Server Error: Could not recover fd flags");
   }
   if (fcntl(fd, F_SETFL, flag | O_NONBLOCK) < 0)
   {
       std::cerr << std::strerror(errno) << std::endl;
       throw std::runtime_error("Server Error: Could not set fd to NonBlocking");
   }
}
