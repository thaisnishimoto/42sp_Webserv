#include "VirtualServer.hpp"
#include "WebServer.hpp"
#include <fcntl.h>
#include <sys/socket.h>

VirtualServer::VirtualServer(int port, std::string name): _port(port), _name(name)
{
}

VirtualServer::~VirtualServer(void)
{
	std::cout << "Virtual Server destructor called" << std::endl;
	// close(_serverFd);
}

//will be deleted
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
