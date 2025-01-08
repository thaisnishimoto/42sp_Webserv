#include "WebServer.hpp"
#include "utils.hpp"
#include <cstdlib>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>

static bool validateTransferEncoding(Request& request);

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
    //hard coded as fuck
    std::map<std::string, VirtualServer*> map1, map2;
    _virtualServers.push_back(VirtualServer(8081, "Server1"));
    _virtualServers.push_back(VirtualServer(8081, "Server2"));
    _virtualServers.push_back(VirtualServer(8082, "Server3"));
    map1.insert(std::pair<std::string, VirtualServer*>(_virtualServers[0].name, &_virtualServers[0]));
    map1.insert(std::pair<std::string, VirtualServer*>(_virtualServers[1].name, &_virtualServers[1]));
    map2.insert(std::pair<std::string, VirtualServer*>(_virtualServers[2].name, &_virtualServers[2]));

    std::pair<uint32_t, uint16_t> pair(0x7f000001, 8081);
    _vServersLookup.insert(std::pair<std::pair<uint32_t, uint16_t>, std::map<std::string, VirtualServer*> >(pair, map1));
    std::pair<uint32_t, uint16_t> pair2(0x7f000001, 8082);
    _vServersLookup.
        insert(std::pair<std::pair<uint32_t, uint16_t>, std::map<std::string, VirtualServer*> >(pair2, map2));

    //set default servers
    std::pair<uint16_t, VirtualServer*> default1(8081, &_virtualServers[1]);
    _vServersDefault.insert(default1);
    std::pair<uint16_t, VirtualServer*> default2(8082, &_virtualServers[2]);
    _vServersDefault.insert(default2);

    _epollFd = epoll_create(1);
    if (_epollFd == -1)
    {
        throw std::runtime_error("Server error: Could not create epoll instance");
    }

    setUpSockets(_ports);
    bindSocket();
    startListening();

    addSocketsToEpoll();
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
        struct sockaddr_in server_address;
        std::memset(&server_address, 0, sizeof(sockaddr_in));
        //this will change later
        server_address.sin_addr.s_addr = htonl(INADDR_ANY);
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(it->second);

        if (bind(sockFd, (struct sockaddr*)&server_address, sizeof(sockaddr_in)) == -1)
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
        fdsReady = epoll_wait(_epollFd, _eventsList, MAX_EVENTS, 1000);
        if (fdsReady == -1)
        {
            //TODO: deal with EINTR. (when signal is received during wait)
            std::cerr << std::strerror(errno) << std::endl;
            throw std::runtime_error("Server Error: could not create socket");
        }

	time_t now = time(NULL);
	std::map<int, Connection>::iterator it = _connectionsMap.begin(); 
	std::map<int, Connection>::iterator ite = _connectionsMap.end(); 
	while (it != ite)
	{
		if (now - it->second.lastActivity > 5)
		{
			std::map<int, Connection>::iterator temp; 
			std::cout << "Connection timed out. Fd: " << it->second.connectionFd << std::endl;
			it->second.buffer.clear();
			epoll_ctl(_epollFd, EPOLL_CTL_DEL, it->second.connectionFd, NULL);
			close(it->second.connectionFd);
			temp = it;
			++it;
			_connectionsMap.erase(temp);
			continue;
		}
		++it;
	}

        for (int i = 0; i < fdsReady; i++)
        {
            int eventFd = _eventsList[i].data.fd;
            if (_socketsToPorts.count(eventFd) == 1)
            {
                int connectionFd = acceptConnection(_epollFd, eventFd);
                if (connectionFd == -1)
                {
                    //TODO: print to user log
                    std::cerr << "Accept connection failed" << std::endl;
                }
                Connection connection(connectionFd);
                if (connection.error == true)
                {
                    epoll_ctl(_epollFd, EPOLL_CTL_DEL, connectionFd, NULL);
                }
                else
                {
                    std::pair<int, Connection> pair(connectionFd, connection);
                    _connectionsMap.insert(pair);
                }
            }
            else if ((_eventsList[i].events & EPOLLIN) == EPOLLIN)
            {
                Connection& connection = _connectionsMap[eventFd];
                parseRequest(connection);
                if (connection.request.continueParsing == false)
                {
                    modifyEventInterest(_epollFd, eventFd, EPOLLOUT);
                }
            }
            else if ((_eventsList[i].events & EPOLLOUT) == EPOLLOUT)
            {
                Connection& connection = _connectionsMap[eventFd];
                fillResponse(connection);
                if (connection.response.isReady == true)
                {
                    std::string tmp;
                    tmp = "HTTP/1.1 " + connection.response.statusCode + " " + connection.response.reasonPhrase +
                        "\r\n";
					tmp += "content-length: 10\r\n";
                    tmp += "origin: " + connection.response.headerFields["origin"] + "\r\n\r\n";
					tmp += connection.response.body;
                    int bytesSent;
                    std::cout << "Will call send now" << std::endl;
                    bytesSent = send(eventFd, tmp.c_str(), tmp.size(), 0);
                    std::cout << "Sent " << bytesSent << "bytes" << std::endl;

                    epoll_ctl(_epollFd, EPOLL_CTL_DEL, eventFd, NULL);
                    _connectionsMap.erase(eventFd);
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

void WebServer::fillResponse(Connection& connection)
{
    Request& request = connection.request;
    Response& response = connection.response;
    if (request.badRequest == true)
    {
        response.statusCode = "400";
        response.reasonPhrase = "Bad Request";
    }
    else if (validateTransferEncoding(request) == false)
    {
        response.statusCode = "501";
        response.reasonPhrase = "Not Implemented";
    }
    else
    {
        identifyVirtualServer(connection);
        response.statusCode = "200";
        response.reasonPhrase = "OK";
        std::pair<std::string, std::string> pair("origin", connection.virtualServer->name);
        response.headerFields.insert(pair);
		response.body = request.body;
    }
    response.isReady = true;
}

// WIP
void WebServer::identifyVirtualServer(Connection& connection)
{
    std::pair<uint32_t, uint16_t> key(connection.host, connection.port);
    std::map<std::string, VirtualServer*> vServersFromHostPort = _vServersLookup[key];
    std::string serverName = connection.request.headerFields["host"];
    std::map<std::string, VirtualServer*>::iterator it = vServersFromHostPort.find(serverName);
    if (it == vServersFromHostPort.end())
    {
        //set default
        std::cout << "Not found vserver" << std::endl;
        connection.virtualServer = _vServersDefault[connection.port];
        return;
    }
    connection.virtualServer = it->second;
}

void WebServer::parseRequest(Connection& connection)
{
    Request& request = connection.request;
    if (consumeNetworkBuffer(connection.connectionFd, connection.buffer) == 0)
    {
	connection.lastActivity = time(NULL);
    }

    if (request.parsedRequestLine == false)
    {
        parseRequestLine(connection.buffer, request);
    }
    if (request.parsedRequestLine == true && request.parsedHeader == false
        && request.continueParsing == true)
    {
        parseHeader(connection.buffer, request);
    }
    if (request.parsedHeader == true && request.continueParsing == true
        && request.validatedHeader == false)
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
        //line below for test
        // request.continueParsing = false;
    }
    if (request.validatedHeader == true)
    {
    	parseBody(connection.buffer, request);
    }
}

void WebServer::parseBody(std::string& connectionBuffer, Request& request)
{
	if (request.isChunked == true &&
		connectionBuffer.find("0\r\n\r\n") != std::string::npos)
	{
		while(true)
		{
			//TODO
			//Make sure that the sum of sizes of chunk
			//does not exceed body limite size.
			std::cout << "Request is chunked" << std::endl;

			std::string hexSize = getNextLineRN(connectionBuffer);
			hexSize = removeCRLF(hexSize);

			long decSize;
			std::istringstream iss(hexSize);
			if (iss >> std::hex >> decSize && iss.eof() != false)
			{
				std::cout << "Chunk size: "<< decSize << std::endl;
			}
			else
			{
				std::cout << "Invalid chunk size" << std::endl;
				request.badRequest = true;
				request.continueParsing = false;
			    break;
			}

			std::string chunk = getNextLineRN(connectionBuffer);
			chunk = removeCRLF(chunk);
			if (chunk.size() != static_cast<size_t>(decSize))
			{
				std::cout << "Sizes don't match!" << std::endl;
				request.badRequest = true;
				request.continueParsing = false;
			    break;
			}
			request.body.append(chunk);

			if (decSize == 0)
			{
				request.continueParsing = false;
				request.parsedBody = true;
				break;
			}
		}
	}
	else if (connectionBuffer.size() >= request.contentLength &&
			request.isChunked == false)
    {
        request.body.append(connectionBuffer, 0, request.contentLength);
        connectionBuffer = connectionBuffer.substr(request.contentLength);
        request.parsedBody = true;
        request.continueParsing = false;
    }
}

static bool findRN(const std::string& line)
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
    if (requestTarget.size() == 0 || findRN(requestTarget) == true)
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
            // quick fix double field name TE CL
            if (fieldName == "transfer-encoding" || fieldName == "content-length") {
                request.badRequest = true;
            }
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

	std::string fieldValue = request.headerFields["content-length"];
	if (fieldValue.empty() == true)
	{
		return false;
	}
    if (fieldValue.find(",") != std::string::npos ||
        fieldValue.find(" ") != std::string::npos)
    {
        return false;
    }
	
	//checking if we receive only digits
	for (std::string::const_iterator it = fieldValue.begin();
		it != fieldValue.end(); ++it)
	{
		if (std::isdigit(*it) == 0)
		{
			return false;
		}
	}
	//capture value in numeric format
	unsigned long long value = 0;
    for (std::string::const_iterator it = fieldValue.begin(); it != fieldValue.end(); ++it)
	{
        value = value * 10 + (*it - '0');
    }
    request.contentLength = value;
    return true;
}

static bool validateTransferEncoding(Request& request)
{
    if (request.headerFields.count("transfer-encoding") == 0)
    {
        return true;
    }
    std::string fieldValue = request.headerFields["transfer-encoding"];
    if (fieldValue != "chunked") {
        return false;
    }
	request.isChunked = true;
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

static bool findExtraRN(Request& request)
{
    std::map<std::string, std::string>::iterator it, ite;
    it = request.headerFields.begin();
    ite = request.headerFields.end();
    while (it != ite)
    {
        if (findRN(it->first) || findRN(it->second))
        {
            return true;
        }
        ++it;
    }
    return false;
}

static bool hasCLAndTEHeaders(Request& request)
{
    bool result = false;
    int cl = request.headerFields.count("content-length");
    int te = request.headerFields.count("transfer-encoding");
    if (cl == 1 && te == 1)
    {
        result = true;
    }
    return result;
}

void WebServer::validateHeader(Request& request)
{
    if (validateContentLength(request) == false)
    {
        request.badRequest = true;
        request.continueParsing = false;
        return;
    }
    if (validateTransferEncoding(request) == false)
    {
        request.continueParsing = false;
        return;
    }
    if (hasCLAndTEHeaders(request) == true)
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
    if (findExtraRN(request) == true)
    {
        request.badRequest = true;
        request.continueParsing = false;
        return;
    }

    request.validatedHeader = true;
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
    std::string buffer;

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
