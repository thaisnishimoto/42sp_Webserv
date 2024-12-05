#include "WebServer.hpp"
#include "utils.hpp"
#include <fcntl.h>
#include <sys/socket.h>

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
	// for (size_t i = 0; i < _virtualServers.size(); ++i)
	// {
	// 	delete _virtualServers[i];
	// }
	// if (_epollFd > 2)
	// {
	// 	close(_epollFd);
	// }
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
	std::map<int, uint16_t>::iterator it = _socketsToPorts.begin();
	std::map<int, uint16_t>::iterator ite = _socketsToPorts.end();

	while (it != ite)
	{
		int sockFd = it->first;
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
			else if ((_eventsList[i].events & EPOLLIN) == EPOLLIN)
			{
				Request& request = _requestMap[eventFd];
				initialParsing(eventFd, _connectionBuffers[eventFd], request);
				if (request.continueParsing == false)
				{
					modifyEventInterest(_epollFd, eventFd, EPOLLOUT);
				}
			}
			else if ((_eventsList[i].events & EPOLLOUT) == EPOLLOUT)
			{
				if (_responseMap.count(eventFd) == 0)
				{
					Response& response = _responseMap[eventFd];
					fillResponse(response, _requestMap[eventFd]);
					if (response.isReady == true)
					{
						std::string tmp;
						tmp = "HTTP/1.1 " + response.statusCode + " " + response.reasonPhrase + "\r\n\r\n";

						int bytesSent;
						std::cout << "Will call send now" << std::endl;
						bytesSent = send(eventFd, tmp.c_str(), tmp.size(), 0);
						std::cout << "Sent " << bytesSent << "bytes" << std::endl;
						// shutdown(eventFd, SHUT_RDWR);
						// std::cout << errno << std::endl;
						// send(eventFd, "HTTP/1.1 400 Bad Request\r\n\r\n", 30, 0);

						epoll_ctl(_epollFd, EPOLL_CTL_DEL, eventFd, NULL); 
						_requestMap.erase(eventFd);
						_responseMap.erase(eventFd);
						_connectionBuffers[eventFd].clear();
						_connectionBuffers.erase(eventFd);
						close(eventFd);
					}
				}
				//codigo para response
			}
		}
	}
}

void WebServer::modifyEventInterest(int epollFd, int eventFd, uint32_t event)
{
	struct epoll_event target_event;
	std::memset(&target_event, 0, sizeof(target_event));
	target_event.events = event;
	target_event.data.fd = eventFd;
	epoll_ctl(epollFd, EPOLL_CTL_MOD, eventFd, &target_event);
}

void WebServer::fillResponse(Response& response, Request& request)
{
	if (request.badRequest == true)
	{
		response.statusCode = "400";
		response.reasonPhrase = "Bad Request";
		response.isReady = true;
	}
}

void WebServer::initialParsing(int connectionFd, std::string& connectionBuffer, Request& request)
{
	consumeNetworkBuffer(connectionFd, connectionBuffer);

	if (request.parsedRequestLine == false)
	{
		parseRequestLine(connectionBuffer, request);
	}
	// else if (request.parsedHeader == false && request.continueParsing == true)
	// {
	// 	parseHeader(connectionBuffer, request);
	// }
}

void WebServer::parseRequestLine(std::string& connectionBuffer, Request& request)
{
	std::string requestLine;
    requestLine = getNextLineRN(connectionBuffer);

    if (requestLine.size() == 2)
    {
        requestLine = getNextLineRN(connectionBuffer);
    }
	if (requestLine.empty() == true)
	{
		return;
	}
    std::cout << "request line: " << requestLine << std::endl;
    std::string requestLineCpy = requestLine;
    parseMethod(requestLineCpy, request);
    if (request.continueParsing == false)
        return;
    parseTarget(requestLineCpy, request);
    if (request.continueParsing == false)
        return;
    parseVersion(requestLineCpy, request);

	//updatebuffer
	connectionBuffer = connectionBuffer.substr(requestLine.size() + 1);
}

void WebServer::parseVersion(std::string& requestLine, Request& request)
{
    if (requestLine != "HTTP/1.1\r\n")
    {
        request.badRequest = true;
		request.continueParsing = true;
    }
}

void WebServer::parseTarget(std::string& requestLine, Request& request)
{
	std::string requestTarget = requestLine.substr(0, requestLine.find(" "));
    std::cout << "request target: " << requestTarget << std::endl;
    if (requestTarget.size() == 0)
    {
        request.badRequest = true;
		request.continueParsing = false;
    }
	request.target = requestTarget;
    requestLine = requestLine.substr(requestTarget.size() + 1, std::string::npos);
    std::cout << "Remainder of request line: " << "'" << requestLine << "'" << std::endl;
}

void WebServer::parseMethod(std::string& requestLine, Request& request)
{
	std::string method;
    method = requestLine.substr(0, requestLine.find(" "));
    std::cout << "method: " << method << std::endl;
    if (_implementedMethods.count(method) == 1 ||
		_unimplementedMethods.count(method) == 1)
    {
		request.method = method;
        request.parsedMethod = true;
    }
    else
    {
        request.badRequest = true;
		request.continueParsing = false;
    }

    requestLine = requestLine.substr(method.size() + 1, std::string::npos);
    std::cout << "Remainder of request line: " << "'" << requestLine << "'" << std::endl;
    //next method will need to verify if number of whitespaces are adequate
}

int WebServer::consumeNetworkBuffer(int connectionFd, std::string& connectionBuffer)
{
	char tempBuffer[5];

	ssize_t bytesRead = recv(connectionFd, tempBuffer, sizeof(tempBuffer), 0);

	if (bytesRead > 0)
	{
		connectionBuffer.append(tempBuffer, bytesRead);
		return 0;
	}
	else
	{
		std::cout << "Connection closed by the client" << std::endl;
		connectionBuffer.clear();
		_connectionBuffers.erase(connectionFd);
    	epoll_ctl(_epollFd, EPOLL_CTL_DEL, connectionFd, NULL);
		close(connectionFd);
		return 1;
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
