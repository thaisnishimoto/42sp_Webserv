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
				//instanciar Connection e adicionar ao map
				// Connection connection = _connectionsMap[connectionFd];
				Connection connection(connectionFd);
				std::pair<int, Connection> pair(connectionFd, connection);
				_connectionsMap.insert(pair);
			}
			else if ((_eventsList[i].events & EPOLLIN) == EPOLLIN)
			{
				Connection& connection = _connectionsMap[eventFd];
				initialParsing(eventFd, connection.buffer, connection.request);
				if (connection.request.continueParsing == false)
				{
					modifyEventInterest(_epollFd, eventFd, EPOLLOUT);
				}
			}
			else if ((_eventsList[i].events & EPOLLOUT) == EPOLLOUT)
			{
				Connection& connection = _connectionsMap[eventFd];
				fillResponse(connection.response, connection.request);
				if (connection.response.isReady == true)
				{
					std::string tmp;
					tmp = "HTTP/1.1 " + connection.response.statusCode + " " + connection.response.reasonPhrase + "\r\n\r\n";

					int bytesSent;
					std::cout << "Will call send now" << std::endl;
					bytesSent = send(eventFd, tmp.c_str(), tmp.size(), 0);
					std::cout << "Sent " << bytesSent << "bytes" << std::endl;

					epoll_ctl(_epollFd, EPOLL_CTL_DEL, eventFd, NULL); 
					close(eventFd);
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
	if (request.parsedRequestLine == true && request.parsedHeader == false && request.continueParsing == true)
	{
		parseHeader(connectionBuffer, request);
	}
	if (request.parsedHeader == true && request.continueParsing == true)
	{
		validateHeader(request);
	}
	if (request.parsedHeader == true)
	{
		 std::map<std::string, std::string>::iterator it, ite;
		 it = request.headerFields.begin();
		 ite = request.headerFields.end();
		 while (it != ite)
		 {
			 std::cout << "key: " << it->first << " | value: " << it->second << std::endl;
			 ++it;
		 }
		 std::cout << "---------------------------------" << std::endl;
	}
	std::cout << "bad request= " << request.badRequest << std::endl;
}

static bool findExtraRN(const std::string& line)
{
	bool found = false;
	if (line.find('\r') != std::string::npos || line.find('\n') != std::string::npos)
		found = true;
	return found;
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
	if (request.continueParsing == false)
		return;
	request.parsedRequestLine = true;
}

void WebServer::parseVersion(std::string& requestLine, Request& request)
{
    if (requestLine != "HTTP/1.1\r\n")
    {
        request.badRequest = true;
		request.continueParsing = false;
    }
}

void WebServer::parseTarget(std::string& requestLine, Request& request)
{
	std::string requestTarget = requestLine.substr(0, requestLine.find(" "));
    std::cout << "request target: " << requestTarget << std::endl;
    if (requestTarget.size() == 0 || findExtraRN(requestTarget) == true)
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

void WebServer::parseHeader(std::string& connectionBuffer, Request& request)
{
    std::cout << "inside parseHeader" << std::endl;
    std::cout << "buffer: " << connectionBuffer << std::endl;
    std::string fieldLine = getNextLineRN(connectionBuffer);

    while (fieldLine.empty() == false && fieldLine != "\r\n")
    {
        std::string fieldName = captureFieldName(fieldLine);
        if (fieldName.empty() == true)
        {
			request.badRequest = true;
        	request.continueParsing = false;
            return;
        }

        std::string fieldValues = captureFieldValues(fieldLine);

        std::cout << "Field-name: " << fieldName << std::endl;
        std::cout << "Field-value: " << fieldValues << std::endl;

		tolower(fieldName);
        std::pair<std::string, std::string> tmp(fieldName, fieldValues);
        std::pair<std::map<std::string, std::string>::iterator, bool> insertCheck;
        insertCheck = request.headerFields.insert(tmp);
        if (insertCheck.second == false)
        {
            request.headerFields[fieldName] = request.headerFields[fieldName] + ", " + fieldValues;
        }
        fieldLine = getNextLineRN(connectionBuffer);
    }
	if (fieldLine == "\r\n")
	{
		request.parsedHeader = true;
	}
}

std::string WebServer::captureFieldName(std::string& fieldLine)
{
    size_t colonPos = fieldLine.find(":");

    if (colonPos == std::string::npos)
    {
        return "";
    }

    std::string fieldName;
    fieldName = fieldLine.substr(0, colonPos);
    if (fieldName.find(" ") != std::string::npos)
    {
        return "";
    }

    return fieldName;
}
std::string WebServer::captureFieldValues(std::string& fieldLine)
{
    size_t colonPos = fieldLine.find(":");

    std::string fieldLineTail;
    fieldLineTail = fieldLine.substr(colonPos + 1, std::string::npos);
    std::cout << "FieldLine Tail: " << fieldLineTail << std::endl;

    std::string fieldValues;
    while (true)
    {
        size_t commaPos = fieldLineTail.find(",");
        if (commaPos == std::string::npos)
        {
            // NOTE: removing \r\n from field line
            fieldLineTail = removeCRLF(fieldLineTail);
            fieldValues += trim(fieldLineTail, " \t");
            break;
        }
        std::string tmp;
        tmp = fieldLineTail.substr(0, commaPos);
        std::cout << "Pre trim tmp: " << tmp << std::endl;
        tmp = trim(tmp, " \t") + ", ";
        std::cout << "Post trim tmp: " << tmp << std::endl;
        fieldValues += tmp;
        fieldLineTail = fieldLineTail.substr(commaPos + 1, std::string::npos);
        std::cout << "fieldLineTail: " << fieldLineTail << std::endl;
    }
	return fieldValues;
}

static bool validateContentLength(Request& request)
{
     if (request.headerFields.count("content-length") == 0)
	 {
     	return true;
     }
     if (request.headerFields["content-length"].find(",") != std::string::npos ||
         request.headerFields["content-length"].find(" ") != std::string::npos)
     {
     	return  false;
     }
     std::istringstream ss(request.headerFields["content-length"]);
     long long value;
     ss >> value;
     if (ss.fail() || !ss.eof() || ss.bad() || value < 0)
     {
       return false;
     }
     return true;
}

static bool validateHost(Request& request)
{
    if (request.headerFields.count("host") != 1)
    {
        return false;
    }
    if (request.headerFields["host"].find(",") != std::string::npos ||
         request.headerFields["host"].find(" ") != std::string::npos)
    {
        return false;
    }
    //TODO: Validate server_name max size
    return true;
}

static bool findRNFields(Request& request)
{
    std::map<std::string, std::string>::iterator it, ite;
    it = request.headerFields.begin();
    ite = request.headerFields.end();
    while (it != ite)
    {
        if (findExtraRN(it->first) || findExtraRN(it->second))
        {
            return true;
        }
        ++it;
    }
    return false;
}

void WebServer::validateHeader(Request& request)
{
	if (validateContentLength(request) == false)
    {
		request.badRequest = true;
		request.continueParsing = false;
        return;
    }
    if (validateHost(request) == false)
    {
    	request.badRequest = true;
		request.continueParsing = false;
        return;
    }
	//need to see more about extras RN
    if (findRNFields(request) == true)
    {
    	request.badRequest = true;
		request.continueParsing = false;
        return;
    }
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
		// _connectionBuffers.erase(connectionFd);
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
