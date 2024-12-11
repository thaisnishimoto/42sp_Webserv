#include "Connection.hpp"
#include <cerrno>
#include <cstring>
#include <sys/socket.h>

Connection::Connection(void)
{
}

Connection::Connection(int fd)
{
	struct sockaddr_in	connectionInfo;
	std::memset(&connectionInfo, 0, sizeof(sockaddr_in));
	socklen_t addrLen = sizeof(connectionInfo);

	if(getsockname(fd, (struct sockaddr *) &connectionInfo, &addrLen) == -1)
	{
		std::cerr << std::strerror(errno) << std::endl;
	}

	connectionFd = fd;
	port = connectionInfo.sin_port;
	host = connectionInfo.sin_addr.s_addr;
	virtualServer = NULL;
}
