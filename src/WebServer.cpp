#include "WebServer.hpp"
#include "Logger.hpp"
#include "utils.hpp"
#include <cstdlib>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <dirent.h> //opendir
#include <fstream> //ifstream

static bool validateTransferEncoding(Request& request);
static bool isTargetDir(Request& request);
static void fillLocationName(Connection& connection);
static void fillLocalPathname(Request& request, Location& location);
static Location& getLocation(VirtualServer* vServer,
							 std::string locationName);
static std::string baseDirectoryListing(void);
static std::string getDirName(Request& request);
static void fillConnectionHeader(Connection& connection);

bool WebServer::_running = true;

WebServer::WebServer(const std::string& configFile)
    : _config(configFile), _logger(DEBUG2)
{
    _implementedMethods.insert("GET");
    _implementedMethods.insert("POST");
    _implementedMethods.insert("DELETE");

    _unimplementedMethods.insert("HEAD");
    _unimplementedMethods.insert("PUT");
    _unimplementedMethods.insert("CONNECT");
    _unimplementedMethods.insert("OPTIONS");
    _unimplementedMethods.insert("TRACE");

    // hard coded ports
    // _ports.insert(8081);
    // _ports.insert(8082);
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

void WebServer::signalHandler(int signum){
    static Logger logger;
    if (signum == SIGINT || signum == SIGTERM)
    {
        logger.log(INFO, "Received shutdown signal. Initiating graceful shutdown...");
        _running = false;
    }
    else if (signum == SIGPIPE)
    {
        logger.log(DEBUG, "Received SIGPIPE signal - ignoring");
    }
}

void WebServer::init(void)
{
    _virtualServersLookup = _config.getVirtualServers();
    _virtualServersDefault = _config.getDefaultsVirtualServers();

	//select unique ports values for socket creating and binding

    _epollFd = epoll_create(1);
    if (_epollFd == -1)
    {
		_logger.log(ERROR, "Server error: Could not create epoll instance.");
        throw std::exception();
    }

    setUpSockets();
    bindSocket();
    startListening();

    addSocketsToEpoll();
}

void WebServer::addSocketsToEpoll(void)
{
    std::map<int, std::pair<uint32_t, uint16_t> >::iterator it = _socketsToPairs.begin();
    std::map<int, std::pair<uint32_t, uint16_t> >::iterator ite = _socketsToPairs.end();

    while (it != ite)
    {
        int sockFd = it->first;
        struct epoll_event target_event;
        std::memset(&target_event, 0, sizeof(target_event));
        target_event.events = EPOLLIN;
        target_event.data.fd = sockFd;
        if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, sockFd, &target_event) == -1)
        {
			_logger.log(ERROR, "Could not add fd to epoll instance");
            throw std::exception();
        }
		std::string msg = "Added fd " + itoa(sockFd) + " to epoll instance";
		_logger.log(DEBUG, msg);

        ++it;
    }
}

void WebServer::setUpSockets(void)
{
	std::map<std::pair<uint32_t, uint16_t>,
		std::map<std::string, VirtualServer> >::iterator it = _virtualServersLookup.begin();
	std::map<std::pair<uint32_t, uint16_t>,
		std::map<std::string, VirtualServer> >::iterator ite = _virtualServersLookup.end();

    while (it != ite)
    {
        int sockFd;
        sockFd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
        if (sockFd == -1)
        {
			_logger.log(ERROR, "Could not create socket");
            throw std::exception();
        }
        int opt = 1;
        if (setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) ==
            -1)
        {
			_logger.log(ERROR, "Could not set socket option");
            throw std::exception();
        };

        _socketsToPairs[sockFd] = it->first;
		std::string msg = "Created new listening socket. Fd: " + itoa(sockFd);
		_logger.log(DEBUG, msg);
        ++it;
    }
}

void WebServer::bindSocket(void)
{
    std::map<int, std::pair<uint32_t, uint16_t> >::iterator it = _socketsToPairs.begin();
    std::map<int, std::pair<uint32_t, uint16_t> >::iterator ite = _socketsToPairs.end();

    while (it != ite)
    {
        int sockFd = it->first;
        struct sockaddr_in server_address;
        std::memset(&server_address, 0, sizeof(sockaddr_in));
        // this will change later
		// TODO change next like to allow for differente IPs
        server_address.sin_addr.s_addr = htonl(INADDR_ANY);
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(it->second.second);

        if (bind(sockFd, (struct sockaddr*)&server_address,
                 sizeof(sockaddr_in)) == -1)
        {
            close(sockFd);
			_logger.log(ERROR, "Could not bind socket");
            throw std::exception();
        };

		std::string msg = "Binded socket " + itoa(sockFd) + " to IP bla and"
			"port " + itoa(it->second.second);
		_logger.log(DEBUG, msg);
        it++;
    }
}

void WebServer::startListening(void)
{
    std::map<int, std::pair<uint32_t, uint16_t> >::iterator it = _socketsToPairs.begin();
    std::map<int, std::pair<uint32_t, uint16_t> >::iterator ite = _socketsToPairs.end();

    while (it != ite)
    {
        int sockFd = it->first;
        if (listen(sockFd, 10) == -1)
        {
            close(sockFd);
			_logger.log(ERROR, "Could not activate listening");
            throw std::exception();
        };
        it++;
    }
}

void WebServer::checkTimeouts(void)
{
    time_t now = time(NULL);
    std::map<int, Connection>::iterator it = _connectionsMap.begin();
    std::map<int, Connection>::iterator ite = _connectionsMap.end();
    while (it != ite)
    {
        if (now - it->second.lastActivity > TIMEOUT)
        {
            std::map<int, Connection>::iterator temp;
            it->second.buffer.clear();
            epoll_ctl(_epollFd, EPOLL_CTL_DEL, it->second.connectionFd, NULL);
            close(it->second.connectionFd);
            _logger.log(INFO, "Connection Timeout. Fd: " +
                                  itoa(it->second.connectionFd));
            temp = it;
            ++it;
            _connectionsMap.erase(temp);
            continue;
        }
        ++it;
    }
}

void WebServer::run(void)
{
    //set signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGPIPE, signalHandler);

    int fdsReady;
    struct epoll_event _eventsList[MAX_EVENTS];

    _logger.log(INFO, "webserv ready to receive connections");
    while (_running)
    {
        fdsReady = epoll_wait(_epollFd, _eventsList, MAX_EVENTS, 1000);
        if (fdsReady == -1)
        {
            // TODO: deal with EINTR. (when signal is received during wait)
            if(_running == false)
            {
                break;
            }
            std::cerr << std::strerror(errno) << std::endl;
            throw std::runtime_error("Server Error: could not create socket");
        }

        for (int i = 0; i < fdsReady; i++)
        {
            int eventFd = _eventsList[i].data.fd;
            if (_socketsToPairs.count(eventFd) == 1)
            {
                int connectionFd = acceptConnection(_epollFd, eventFd);
                if (connectionFd == -1)
                {
                    // TODO: print to user log
                    std::cerr << "Accept connection failed" << std::endl;
                }
                Connection connection(connectionFd);
                _logger.log(INFO, "Connection established. fd: " +
                                      itoa(connection.connectionFd));
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
				if (_connectionsMap.find(eventFd) == _connectionsMap.end())
				{
					continue;
				}
                if (connection.request.continueParsing == false)
                {
                    modifyEventInterest(_epollFd, eventFd, EPOLLOUT);
                    _logger.log(DEBUG,
                                "Fd " + itoa(eventFd) + " now on EPOLLOUT");
                }
            }
            else if ((_eventsList[i].events & EPOLLOUT) == EPOLLOUT)
            {
                Connection& connection = _connectionsMap[eventFd];
                if (connection.response.isReady == false)
                {
                    fillResponse(connection);
                    buildResponseBuffer(connection);
                    connection.response.isReady = true;
                }

                int bytesSent;
                std::string& buf = connection.responseBuffer;
                bytesSent = send(eventFd, buf.c_str(), buf.size(), 0);
                if (bytesSent == -1)
                {
                    _logger.log(ERROR, "Connection closed due to send error");
                    epoll_ctl(_epollFd, EPOLL_CTL_DEL, eventFd, NULL);
                    _connectionsMap.erase(eventFd);
                    _logger.log(DEBUG,
                                "Connection removed from connectionsMap. Fd: " +
                                    itoa(eventFd));
                    close(eventFd);
                    continue;
                }
                if (bytesSent < static_cast<int>(buf.size()))
                {
                    buf = buf.substr(bytesSent);
                    _logger.log(DEBUG, "Partial response sent. Sent: " +
                                           itoa(bytesSent) +
                                           ". Remaining: " + itoa(buf.size()));
                    continue;
                }
                buf = buf.substr(bytesSent);
                _logger.log(DEBUG, "Final part of response sent. Sent: " +
                                       itoa(bytesSent) +
                                       ". Remaining: " + itoa(buf.size()));
                _logger.log(INFO, "Response sent. Fd: " +
                                      itoa(connection.connectionFd));

				//connection management
				if (connection.response.closeAfterSend == true)
				{
					epoll_ctl(_epollFd, EPOLL_CTL_DEL, eventFd, NULL);
					_logger.log(DEBUG, "Fd " + itoa(connection.connectionFd) +
										   " deleted from epoll instance");
					_connectionsMap.erase(eventFd);
					close(eventFd);
				}
				else
				{
					modifyEventInterest(_epollFd, eventFd, EPOLLIN);
					_logger.log(DEBUG, "Fd " + itoa(connection.connectionFd) +
						" event of interest changed to EPOLLIN");
					connection.virtualServer = NULL;
					connection.request = Request();
					connection.response = Response();
				}
            }
        }

        checkTimeouts();
    }

    cleanup();
}

void WebServer::cleanup(void) {
    _logger.log(DEBUG, "cleaning up...");
    std::map<int, Connection>::iterator itConn = _connectionsMap.begin();
    std::map<int, Connection>::iterator iteConn = _connectionsMap.end();

    while (itConn != iteConn)
    {
        close(itConn->second.connectionFd);
        itConn++;
    }

    std::map<int, std::pair<uint32_t, uint16_t> >::iterator itSock = _socketsToPairs.begin();
    std::map<int, std::pair<uint32_t, uint16_t> >::iterator iteSock = _socketsToPairs.end();

    while (itSock != iteSock)
    {
        close(itSock->first);
        itSock++;
    }
    if (_epollFd > 0)
    {
        close(_epollFd);
    }
    _logger.log(INFO, "Server shutdown complete");
}

void WebServer::modifyEventInterest(int epollFd, int eventFd, uint32_t event)
{
    struct epoll_event target_event;
    std::memset(&target_event, 0, sizeof(target_event));
    target_event.events = event;
    target_event.data.fd = eventFd;
    epoll_ctl(epollFd, EPOLL_CTL_MOD, eventFd, &target_event);
}

static void fillLocationName(Connection& connection)
{
	Request& request = connection.request;
	VirtualServer* vServer = connection.virtualServer;

	std::string& target = request.target;
	if (vServer->_locations.find(target) != vServer->_locations.end())
	{
		request.locationName = vServer->_locations.find(target)->first;
		return;
	}

	size_t lastSlashPos = target.rfind("/");
	if (lastSlashPos != 0)
	{
		request.locationName = target.substr(0, lastSlashPos);
		return;
	}

	request.locationName = "/";
	return;
}

static void fillLocalPathname(Request& request, Location& location)
{
	if (isTargetDir(request) == true)
	{
		request.isDir = true;
	}

	if (request.isDir == true && location.isAutoIndex() == false &&
		request.method == "GET")
	{
		request.localPathname = "." + location.getRoot();
		request.localPathname += request.target;
		request.localPathname += location.getIndex();
		request.isDir = false;
		return;
	}

	request.localPathname = "." + location.getRoot();
	request.localPathname += request.target;
}

void WebServer::fillResponse(Connection& connection)
{
    Request& request = connection.request;
    Response& response = connection.response;

    if (request.badRequest == true)
    {
        response.statusCode = "400";
        response.reasonPhrase = "Bad Request";
		response.closeAfterSend = true;
		response.headerFields["connection"] = "close";
    }
    else if (validateTransferEncoding(request) == false)
    {
        response.statusCode = "501";
        response.reasonPhrase = "Not Implemented";
		response.closeAfterSend = true;
		response.headerFields["connection"] = "close";
    }
    else if (request.bodyTooLarge == true)
    {
        response.statusCode = "413";
        response.reasonPhrase = "Content Too Large";
		response.headerFields["content-length"] = "0";
		response.closeAfterSend = true;
		response.headerFields["connection"] = "close";
    }
	else if (_unimplementedMethods.find(request.method)
		!= _unimplementedMethods.end())
	{
        response.statusCode = "501";
        response.reasonPhrase = "Not Implemented";
		response.closeAfterSend = true;
		response.headerFields["connection"] = "close";
	}
    else
    {
		fillLocationName(connection);

		Location& location = getLocation(connection.virtualServer,
								   request.locationName);
        // identifyLocation
        // if (location doesnt exist in virtual server)
        // {
        //     response.statusCode = "404";
        //     response.reasonPhrase = "Not Found";
        // }
        if (isCgiRequest(connection, location))
        {
            _logger.log(INFO, "Handling CGI Request"); 
            Cgi cgiHandler(connection, location);
            // cgiHandler.execute();
            response.statusCode = "200";
            response.reasonPhrase = "OK";
            std::cout << "server name: " << connection.virtualServer->getServerName() << std::endl;
            std::pair<std::string, std::string> pair(
                "origin", connection.virtualServer->getServerName());
            response.headerFields.insert(pair);
            response.body = request.body;
        }
        else
        {
            response.statusCode = "200";
            response.reasonPhrase = "OK";
            std::pair<std::string, std::string> pair(
                "origin", connection.virtualServer->getServerName());
            response.headerFields.insert(pair);
            response.body = request.body;
        }
		if (location.getRedirect().empty() == false)
		{
			std::string msg = "Location redirects to ";
			msg += location.getRedirect();
			_logger.log(DEBUG, msg);

			response.statusCode = "301";
			response.reasonPhrase = "Moved Permanently";
			response.headerFields["location"] = location.getRedirect();
			return;
		}

		if (location.isAllowedMethod(request.method) == false)
		{
			response.statusCode = "405";
			response.reasonPhrase = "Method Not Allowed";
			response.headerFields["allow"] = location.getAllowedMethods();
			response.closeAfterSend = true;
			response.headerFields["connection"] = "close";
			return;
		}

		fillLocalPathname(request, location);

		//404
		if (access(request.localPathname.c_str(), F_OK) != 0)
		{
			std::string msg = "File " + request.localPathname + " does not exist.";
			_logger.log(DEBUG, msg);
			response.statusCode = "404";
			response.reasonPhrase = "Not Found";
			response.closeAfterSend = true;
			response.headerFields["connection"] = "close";

			VirtualServer& vServer = *connection.virtualServer;
			std::string errorFilePath = vServer.getErrorPage("404");
			std::ifstream errorFile(errorFilePath.c_str());
			if (errorFile.is_open() == false)
			{
				std::string msg = "WebServ could not open error page"
					"for some reason.";
				_logger.log(DEBUG, msg);
				response.statusCode = "500";
				response.reasonPhrase = "Internal Server Error";
				response.closeAfterSend = true;
				response.headerFields["connection"] = "close";
				return;
			}

			std::istreambuf_iterator<char> it(errorFile);
			std::istreambuf_iterator<char> ite;
			std::string fileContent(it, ite);
			errorFile.close();
			response.body = fileContent;
			response.headerFields["content-length"] = itoa(static_cast<int>(fileContent.length()));
			return;
		}

		//403
		if (request.isDir == true &&
			access(request.localPathname.c_str(), X_OK) != 0)
		{
			std::string msg = "WebServ does not have access rights for " + request.localPathname + " directory.";
			_logger.log(DEBUG, msg);
			response.statusCode = "403";
			response.reasonPhrase = "Forbidden";
			response.closeAfterSend = true;
			response.headerFields["connection"] = "close";
			return;
		}
		if (request.isDir == false &&
			access(request.localPathname.c_str(), R_OK) != 0)
		{
			std::string msg = "WebServ does not have access rights for " + request.localPathname + ".";
			_logger.log(DEBUG, msg);
			response.statusCode = "403";
			response.reasonPhrase = "Forbidden";
			response.closeAfterSend = true;
			response.headerFields["connection"] = "close";
			return;
		}

		fillConnectionHeader(connection);

		if (request.method == "GET")
		{
			handleGET(connection);
		}
		else if (request.method == "POST")
		{
			handlePOST(connection);
		}
		else if (request.method == "DELETE")
		{
			handleDELETE(connection);
		}
    }
}

static void fillConnectionHeader(Connection& connection)
{
	Request& request = connection.request;
	Response& response = connection.response;

	if (request.headerFields.count("connection") == 0)
	{
		return;
	}
	if (request.headerFields["connection"] == "close")
	{
		response.closeAfterSend = true;
		response.headerFields["connection"] = "close";
		return;
	}
}

void WebServer::buildResponseBuffer(Connection& connection)
{
	Response& response = connection.response;
	std::string& buffer = connection.responseBuffer;

	//status line
	buffer = "HTTP/1.1 ";
	buffer += response.statusCode;
	buffer += " ";
	buffer += response.reasonPhrase;
	buffer += "\r\n";

	//headers
	std::map<std::string, std::string>::iterator it = response.headerFields.begin();
	std::map<std::string, std::string>::iterator ite = response.headerFields.end();

	while (it != ite)
	{
		buffer += it->first;
		buffer += ": ";
		buffer += it->second;
		buffer += "\r\n";
		++it;
	}
	buffer += "\r\n";

	//body
    buffer += connection.response.body;
}

// WIP
void WebServer::identifyVirtualServer(Connection& connection)
{
	//based on connection info host:port
    std::pair<uint32_t, uint16_t> key(connection.host, connection.port);
    std::map<std::string, VirtualServer>& vServersFromHostPort =
        _virtualServersLookup[key];
    std::string serverName = connection.request.headerFields["host"];
    std::map<std::string, VirtualServer>::iterator it =
        vServersFromHostPort.find(serverName);

    if (it == vServersFromHostPort.end())
    {
        // set default
		_logger.log(INFO, "Could not match virtualServer by name, using default.");
        connection.virtualServer = _virtualServersDefault[key];
        return;
    }
    connection.virtualServer = &(it->second);
    std::cout << "server name: " << connection.virtualServer->getServerName() << std::endl;
}

void WebServer::parseRequest(Connection& connection)
{
    Request& request = connection.request;
    if (consumeNetworkBuffer(connection.connectionFd, connection.buffer) == 1)
    {
		return;
    }
    connection.lastActivity = time(NULL);
    if (request.parsedRequestLine == false)
    {
        parseRequestLine(connection.buffer, request);
    }
    if (request.parsedRequestLine == true && request.parsedHeader == false &&
        request.continueParsing == true)
    {
        parseHeader(connection.buffer, request);
    }
    if (request.parsedHeader == true && request.continueParsing == true &&
        request.validatedHeader == false)
    {
        validateHeader(connection);
    }
    if (request.parsedHeader == true)
    {
        // std::map<std::string, std::string>::iterator it, ite;
        // it = request.headerFields.begin();
        // ite = request.headerFields.end();
        // while (it != ite)
        // {
        //     std::cout << "key: " << it->first << " | value: " << it->second
        //     << std::endl;
        //     ++it;
        // }
        // std::cout << "---------------------------------" << std::endl;
        // line below for test
        // request.continueParsing = false;
    }
    if (request.validatedHeader == true)
    {
        parseBody(connection);
    }
}

void WebServer::parseBody(Connection& connection)
{
	std::string& connectionBuffer = connection.buffer;
	Request& request = connection.request;

    if (request.isChunked == true &&
        connectionBuffer.find("0\r\n\r\n") != std::string::npos)
    {
		size_t maxBodySize = connection.virtualServer->getBodySize() * 1024;
        while (true)
        {
            std::string hexSize = getNextLineRN(connectionBuffer);
            hexSize = removeCRLF(hexSize);

            if (hexSize.find('-') != std::string::npos || hexSize.length() > 16)
            {
                _logger.log(DEBUG, "Invalid chunk size");
                request.badRequest = true;
                request.continueParsing = false;
                break;
            }

            unsigned long decSize = 0;
            std::istringstream iss(hexSize);
            if (iss >> std::hex >> decSize && iss.eof() != false)
            {
                request.contentLength += static_cast<size_t>(decSize);
                if (request.contentLength > maxBodySize)
                {
                    _logger.log(DEBUG, "Request body too large");
                    request.bodyTooLarge = true;
                    request.continueParsing = false;
                    break;
                }
            }
            else
            {
                _logger.log(DEBUG, "Invalid chunk size format");
                request.badRequest = true;
                request.continueParsing = false;
                break;
            }

            std::string chunk = getNextLineRN(connectionBuffer);
            chunk = removeCRLF(chunk);
            if (chunk.size() != static_cast<size_t>(decSize))
            {
                _logger.log(
                    DEBUG,
                    "Reported chunk size does not match actual chunk size");
                request.badRequest = true;
                request.continueParsing = false;
                break;
            }
            request.body.append(chunk);

            if (decSize == 0)
            {
                request.continueParsing = false;
                request.parsedBody = true;
                _logger.log(DEBUG2, "Parsed body: " + request.body);
                break;
            }
        }
    }
    else if (connectionBuffer.size() >= request.contentLength &&
             request.isChunked == false)
    {
        request.body.append(connectionBuffer, 0, request.contentLength);
        connectionBuffer = connectionBuffer.substr(request.contentLength);
        _logger.log(DEBUG2, "Parsed body: " + request.body);
        request.parsedBody = true;
        request.continueParsing = false;
    }
}

static bool findRN(const std::string& line)
{
    bool found = false;
    if (line.find('\r') != std::string::npos ||
        line.find('\n') != std::string::npos)
        found = true;
    return found;
}

void WebServer::parseRequestLine(std::string& connectionBuffer,
                                 Request& request)
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
    // std::cout << "request line: " << requestLine << std::endl;
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
    std::string str("HTTP/1.1");
    _logger.log(DEBUG, "Parsed HTTP Version: " + str);
}

void WebServer::parseTarget(std::string& requestLine, Request& request)
{
    std::string requestTarget = requestLine.substr(0, requestLine.find(" "));
    if (requestTarget.size() == 0 || findRN(requestTarget) == true)
    {
        request.badRequest = true;
        request.continueParsing = false;
    }
    parseQueryString(requestTarget, request);
    request.target = requestTarget;
    _logger.log(DEBUG, "Parsed target: " + requestTarget);
    requestLine = requestLine.substr(requestTarget.size() + 1, std::string::npos);
    // std::cout << "Remainder of request line: " << "'" << requestLine << "'"
    // << std::endl;
}

void WebServer::parseMethod(std::string& requestLine, Request& request)
{
    std::string method;
    method = requestLine.substr(0, requestLine.find(" "));
    if (_implementedMethods.count(method) == 1 ||
        _unimplementedMethods.count(method) == 1)
    {
        request.method = method;
        _logger.log(DEBUG, "Parsed method: " + method);
        request.parsedMethod = true;
    }
    else
    {
        request.badRequest = true;
        request.continueParsing = false;
    }

    requestLine = requestLine.substr(method.size() + 1, std::string::npos);
    // std::cout << "Remainder of request line: " << "'" << requestLine << "'"
    // << std::endl; next method will need to verify if number of whitespaces
    // are adequate
}

void WebServer::parseHeader(std::string& connectionBuffer, Request& request)
{
    // std::cout << "inside parseHeader" << std::endl;
    // std::cout << "buffer: " << connectionBuffer << std::endl;
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

        // std::cout << "Field-name: " << fieldName << std::endl;
        // std::cout << "Field-value: " << fieldValues << std::endl;
        _logger.log(DEBUG,
                    "Parsed header line -> " + fieldName + ": " + fieldValues);

        tolower(fieldName);
        std::pair<std::string, std::string> tmp(fieldName, fieldValues);
        std::pair<std::map<std::string, std::string>::iterator, bool>
            insertCheck;
        insertCheck = request.headerFields.insert(tmp);
        if (insertCheck.second == false)
        {
            // quick fix double field name TE CL
            if (fieldName == "transfer-encoding" ||
                fieldName == "content-length")
            {
                request.badRequest = true;
            }
            request.headerFields[fieldName] =
                request.headerFields[fieldName] + ", " + fieldValues;
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
    // std::cout << "FieldLine Tail: " << fieldLineTail << std::endl;

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
        // std::cout << "Pre trim tmp: " << tmp << std::endl;
        tmp = trim(tmp, " \t") + ", ";
        // std::cout << "Post trim tmp: " << tmp << std::endl;
        fieldValues += tmp;
        fieldLineTail = fieldLineTail.substr(commaPos + 1, std::string::npos);
        // std::cout << "fieldLineTail: " << fieldLineTail << std::endl;
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

    // checking if we receive only digits
    for (std::string::const_iterator it = fieldValue.begin();
         it != fieldValue.end(); ++it)
    {
        if (std::isdigit(*it) == 0)
        {
            return false;
        }
    }
    // capture value in numeric format
    unsigned long long value = 0;
    for (std::string::const_iterator it = fieldValue.begin();
         it != fieldValue.end(); ++it)
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
    if (fieldValue != "chunked")
    {
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
    // TODO: Validate server_name max size
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

static bool validateContentLengthMaxSize(Request& request)
{
	if (request.contentLength > MAX_BODY_SIZE)
	{
		return false;
	}
	return true;
}

static bool validateContentLengthSize(Connection& connection)
{
	Request& request = connection.request;
    if (request.headerFields.count("content-length") == 0)
    {
        return true;
    }
    else
    {
		size_t maxBodySize = connection.virtualServer->getBodySize() * 1024;
        if (request.contentLength > maxBodySize)
        {
            return false;
        }
        else
        {
            return true;
        }
    }
}

void WebServer::validateHeader(Connection& connection)
{
	Request& request = connection.request;

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
    // need to see more about extras RN
    if (findExtraRN(request) == true)
    {
        request.badRequest = true;
        request.continueParsing = false;
        return;
    }
	identifyVirtualServer(connection);
    if (validateContentLengthMaxSize(request) == false)
    {
        _logger.log(DEBUG, "Content-Length greater than global limit. Webserv will not read body.");
        request.bodyTooLarge = true;
        request.continueParsing = false;
		return;
    }
    if (validateContentLengthSize(connection) == false)
    {
        _logger.log(DEBUG, "Request body too large. Webserver will read and discard body");
        request.bodyTooLarge = true;
    }
    request.validatedHeader = true;
}

int WebServer::consumeNetworkBuffer(int connectionFd,
                                    std::string& connectionBuffer)
{
    char tempBuffer[5];
    ssize_t bytesRead = recv(connectionFd, tempBuffer, sizeof(tempBuffer), 0);

    if (bytesRead > 0)
    {
        connectionBuffer.append(tempBuffer, bytesRead);
        return 0;
    }
	else if (bytesRead == 0)
	{
		return 1;
	}
    else
    {
        _logger.log(INFO, "Fd " + itoa(connectionFd) +
                              ". Connection closed by client.");
        connectionBuffer.clear();
        epoll_ctl(_epollFd, EPOLL_CTL_DEL, connectionFd, NULL);
        _logger.log(DEBUG, "Fd " + itoa(connectionFd) +
                               " deleted from epoll instance");
        _connectionsMap.erase(connectionFd);
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
        _logger.log(DEBUG,
                    "Fd " + itoa(newFd) + " added to epoll instance - EPOLLIN");
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
        throw std::runtime_error(
            "Server Error: Could not set fd to NonBlocking");
    }
}

static bool isTargetDir(Request& request)
{
	std::string& target = request.target;
	size_t endPos = target.length() - 1;

	if (target[endPos] == '/')
	{
		return true;
	}
	return false;
}

static Location& getLocation(VirtualServer* vServer, std::string locationName)
{
	std::map<std::string, Location>::iterator it = vServer->_locations.begin();
	std::map<std::string, Location>::iterator ite = vServer->_locations.end();

	while (it != ite)
	{
		if (it->first == locationName)
		{
			return it->second;
		}
		++it;
	}

	size_t lastSlashPos = locationName.rfind("/");
	if (lastSlashPos == 0)
	{
		return getLocation(vServer, "/");
	}

	std::string parentLocation = locationName.substr(0, locationName.rfind("/"));
	return getLocation(vServer, parentLocation);
}

void WebServer::handleGET(Connection& connection)
{
	Request& request = connection.request;
	Response& response = connection.response;
	Location& location = getLocation(connection.virtualServer, request.locationName);

	if (request.isDir == true)
	{
		_logger.log(DEBUG, "Target resource is a directory");
		if (location.isAutoIndex() == true)
		{
			//build directorylisting
			std::string msg = "Directory listing if on";
			_logger.log(DEBUG, msg);

			std::string dirList = baseDirectoryListing();
			dirList += "<h1> Contents of " + request.target;
			dirList += "</h1>\n<ul>\n";

			std::string localDirName = "." + location.getRoot() + request.target;
			DIR* dir = opendir(localDirName.c_str());
			struct dirent* dirContent = readdir(dir);
			while (dirContent != NULL)
			{
				dirList += "<li>";
				dirList += dirContent->d_name;
				if (dirContent->d_type == DT_DIR)
				{
					dirList += "/";
				}
				dirList += "</li>\n";
				dirContent = readdir(dir);
			}
			dirList += "</ul>\n</body>\n</html>";

			closedir(dir);
			response.statusCode = "200";
			response.reasonPhrase = "OK";
			response.headerFields["content-length"] = itoa(static_cast<int>(dirList.size()));
			response.body = dirList;
			return;
		}
	}

	//open file
	std::ifstream file(request.localPathname.c_str());
	if (file.is_open() == false)
	{
		std::string msg = "WebServ could not open" + request.localPathname + "for some reason.";
		_logger.log(DEBUG, msg);
		response.statusCode = "500";
		response.reasonPhrase = "Internal Server Error";
		response.closeAfterSend = true;
		response.headerFields["connection"] = "close";
		//add body?
		return;
	}

	std::string msg = "Opened " + request.localPathname;
	_logger.log(DEBUG, msg);

	//read content of file using special std::string constructor
	std::istreambuf_iterator<char> it(file);
	std::istreambuf_iterator<char> ite;
	std::string fileContent(it, ite);

	file.close();

	msg = "Filling response object with data from " + request.localPathname;
	_logger.log(DEBUG, msg);
	response.body = fileContent;
	response.headerFields["content-length"] = itoa(static_cast<int>(fileContent.length()));
	response.statusCode = "200";
	response.reasonPhrase = "OK";
}

static std::string baseDirectoryListing(void)
{
	std::string content;
	content = "<!DOCTYPE html>\n";
	content+="<html lang=\"en\">\n";
	content+="<head>\n";
	content+="<meta charset=\"UTF-8\">\n";
	content+="<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
	content+="<title>Unordered List Example</title>\n";
	content+="</head>\n";
	content+="<body>\n";

	return content;
}

void WebServer::handlePOST(Connection& connection)
{
	Request& request = connection.request;
	Response& response = connection.response;
	// Location& location = getLocation(connection.virtualServer, request.locationName);

	//415
	if (request.headerFields.count("content-type") == 0)
	{
		_logger.log(DEBUG, "No content-type header in POST request");
		response.statusCode = "415";
		response.reasonPhrase = "Unsupported Media Type";
		response.headerFields["accept-post"] = "multipart/form-data";
		response.headerFields["content-length"] = "0";
		response.closeAfterSend = true;
		response.headerFields["connection"] = "close";
		return;
	}

	std::map<std::string, std::string>::iterator it =
		request.headerFields.find("content-type");
	std::string headerValue = it->second;
	std::string contentType = headerValue.substr(0, headerValue.find(";"));
	if (contentType != "multipart/form-data")
	{
		_logger.log(DEBUG, "content-type header in POST request is not accepted");
		response.statusCode = "415";
		response.reasonPhrase = "Unsupported Media Type";
		response.headerFields["accept-post"] = "multipart/form-data";
		response.headerFields["content-length"] = "0";
		response.closeAfterSend = true;
		response.headerFields["connection"] = "close";
		return;
	}

	std::string delim = headerValue.substr(headerValue.find("=") + 1,
										headerValue.find("\r\n"));
	std::string msg = "Delimiter is " + delim;
	_logger.log(DEBUG, msg);

	//capture body
	std::stringstream bodyStream(request.body);
	std::string firstLine;
	std::getline(bodyStream, firstLine);
	if (firstLine == "--" + delim + "\r")
	{
		_logger.log(DEBUG, "Body first line matches with delim");
		std::string secondLine;
		std::getline(bodyStream, secondLine);
		std::string fileName = secondLine.substr(secondLine.find("filename=") + 10);
		fileName = fileName.substr(0, fileName.find("\"\r"));
		_logger.log(DEBUG, "Captured filename.");

		//try and create file
		std::string localFileName = request.localPathname + "/" + fileName;
		if (access(localFileName.c_str(), F_OK) == 0)
		{
			_logger.log(DEBUG, "A file with the same name already exists in webserv filesystem");
			response.statusCode = "409";
			response.reasonPhrase = "Conflict";
			response.body = "File already exists in webserv filesystem";
			response.headerFields["content-length"] = itoa(static_cast<int>(response.body.length()));
			response.closeAfterSend = true;
			response.headerFields["connection"] = "close";
			return;
		}

		//substring to capture begining of content
		std::string content;
		content = request.body.substr(request.body.find("\r\n\r\n") + 4);
		//read body until next boundary, effectively ignoring multiple fields forms

		// std::string closeDelim = "\r\n--" + delim + "--";
		std::string closeDelim = "\r\n--" + delim;
		size_t closeDelimPos = content.find(closeDelim);
		if (closeDelimPos == std::string::npos)
		{
			response.statusCode = "400";
			response.reasonPhrase = "Bad Request";
			response.closeAfterSend = true;
			response.headerFields["connection"] = "close";
			return;
		}
		content = content.substr(0, closeDelimPos);

		std::ofstream out(localFileName.c_str());
		if (out.is_open() == false)
		{
			response.statusCode = "500";
			response.reasonPhrase = "Internal Server Error";
			response.closeAfterSend = true;
			response.headerFields["connection"] = "close";
			return;
		}

		out << content;
		out.close();
		_logger.log(DEBUG, "File sucessfully uploaded");

		response.statusCode = "201";
		response.reasonPhrase = "Created";
		response.body = "File uploaded correctly.";
		response.headerFields["content-length"] = itoa(static_cast<int>(response.body.length()));
		response.headerFields["location"] = request.locationName + "/" + fileName;
		return;
	}

	response.statusCode = "400";
	response.reasonPhrase = "Bad Request";
	response.closeAfterSend = true;
	response.headerFields["connection"] = "close";
	return;
}

//./content/index.html
//./content/filesDir/file1
//substr(0, rfind())...

static std::string getDirName(Request& request)
{
	std::string localPathname = request.localPathname;
	std::string dir;
	dir = request.localPathname.substr(0, localPathname.rfind("/"));
	return dir;
}

void WebServer::handleDELETE(Connection& connection)
{
	Request& request = connection.request;
	Response& response = connection.response;

	if (request.isDir == true)
	{
			_logger.log(DEBUG, "Webserv does not allow DELETE request to directories");
			response.statusCode = "403";
			response.reasonPhrase = "Forbidden";
			response.closeAfterSend = true;
			response.headerFields["connection"] = "close";
			return;
	}

	std::string localDir = getDirName(request);

	if (access(localDir.c_str(), R_OK | W_OK | X_OK) == 0)
	{
		if (remove(request.localPathname.c_str()) != 0)
		{
			std::string msg = "WebServ could not delete" + request.localPathname + "for some reason.";
			_logger.log(DEBUG, msg);
			response.statusCode = "500";
			response.reasonPhrase = "Internal Server Error";
			response.closeAfterSend = true;
			response.headerFields["connection"] = "close";
			return;
		}

		std::string msg = "Deleting file " + request.localPathname;
		_logger.log(DEBUG, msg);
		response.statusCode = "204";
		response.reasonPhrase = "No Content";
		return;
	}
	else
	{

		_logger.log(DEBUG, "Webserv does not have rights to delete file");
		response.statusCode = "403";
		response.reasonPhrase = "Forbidden";
		response.closeAfterSend = true;
		response.headerFields["connection"] = "close";
		return;
	}
}
