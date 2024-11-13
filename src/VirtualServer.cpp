#include "VirtualServer.hpp"
#include "WebServer.hpp"
#include <fcntl.h>
#include <sys/socket.h>

VirtualServer::VirtualServer(int port): _port(port)
{
	setUpSocket();
	bindSocket();
	startListening();
}

VirtualServer::~VirtualServer(void)
{
	std::cout << "Virtual Server destructor called" << std::endl;
	close(_serverFd);
}

int VirtualServer::getServerFd(void)
{
	return _serverFd;
}

void VirtualServer::setUpSocket(void)
{
	_serverFd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (_serverFd == -1)
	{
		std::cerr << std::strerror(errno) << std::endl;
		throw std::runtime_error("Virtual Server Error: could not create socket");
	}
	//TODO - check for error and how to deal with it.
	int opt = 1;
    if (setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
	{
		std::cerr << std::strerror(errno) << std::endl;
		throw std::runtime_error("Virtual Server Error: Could not set socket option");
	};
}

void VirtualServer::bindSocket(void)
{
	struct sockaddr_in	server_address;
	std::memset(&server_address, 0, sizeof(sockaddr_in));
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(_port);

	if (bind(_serverFd, (struct sockaddr *)&server_address, sizeof(sockaddr_in)) == -1)
	{
		close(_serverFd);
		std::cerr << std::strerror(errno) << std::endl;
		throw std::runtime_error("Virtual Server Error: could not bind socket");
	};
}

void VirtualServer::startListening(void)
{
	if (listen(_serverFd, 10) == -1)
	{
		close(_serverFd);
		std::cerr << std::strerror(errno) << std::endl;
		throw std::runtime_error("Virtual Server Error: could not activate listening");
	};
}

int VirtualServer::acceptConnection(int epollFd)
{
	struct epoll_event target_event;
	int newFd;
	std::string	buffer;

	newFd = accept(_serverFd, NULL, NULL);
	if (newFd != -1)
	{
	    setNonBlocking(newFd);
    	std::pair<int, std::string>	pair(newFd, buffer);
    	_clientBuffers.insert(pair);
    	target_event.events = EPOLLIN;
    	target_event.data.fd = newFd;
    	epoll_ctl(epollFd, EPOLL_CTL_ADD, newFd, &target_event);
	}
	return newFd;
}

void VirtualServer::processRequest(int connectionFd)
{
	std::string& buffer = _clientBuffers[connectionFd];

	char tempBuffer[5];

	ssize_t bytesRead = recv(connectionFd, tempBuffer, sizeof(tempBuffer), 0);

	if (bytesRead > 0)
	{
		buffer.append(tempBuffer, bytesRead);

		size_t header_end = buffer.find("\r\n\r\n");

		if (header_end != std::string::npos)
		{
			// std::cout << buffer << std::endl;
			std::string completeMsg = buffer.substr(0, header_end + 4);

			Request request;
			request.parseRequest(completeMsg);

			//process request to generate response
			
			// close(connectionFd);
			buffer.clear(); //TODO: deal with entangled request messages
		}
	}
	else
	{
		std::cout << "Connection closed by the client" << std::endl;
		buffer.clear();
		_clientBuffers.erase(connectionFd);
		close(connectionFd);
	}

}

void VirtualServer::setNonBlocking(int fd)
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
