#include <sys/socket.h> //socket
#include <sys/types.h> //socket
#include <cerrno> //perror
#include <cstdio> //perror
#include <netdb.h> //bind
#include <cstring> //memset
#include <iostream> //cout
#include <unistd.h> //close
#include <sys/epoll.h>

#define PORT 8081
#define MAX_QUEUE 100
#define MAX_EVENTS 256

int	main(void)
{
  int epoll_fd = epoll_create(1);
	if (epoll_fd == -1)
	{
		std::perror("Epoll create failed");
		return 1;
	}
	int	sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_fd == -1)
	{
		std::perror("Socket creation failed");
		return 1;
	}

	struct sockaddr_in	server_address;
	std::memset(&server_address, 0, sizeof(sockaddr_in));
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(PORT);

	int	bind_status = bind(sock_fd, (struct sockaddr *)&server_address, sizeof(sockaddr_in));
	if (bind_status == -1)
	{
		std::perror("Binding of address failed");
		return 1;
	}

	int	listen_status = listen(sock_fd, MAX_QUEUE);
	if (listen_status == -1)
	{
		std::perror("Listen failed");
		return 1;
	}

  struct epoll_event target_event;
  target_event.events = EPOLLIN;
  target_event.data.fd = sock_fd;
  

  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_fd, &target_event) == -1)
  {
    perror("Epoll control failed");
    return 1;
  }
  
  int fds_ready;

  struct epoll_event events_list[MAX_EVENTS];
  int it = 0;
	while(42)
	{
    fds_ready = epoll_wait(epoll_fd, events_list, MAX_EVENTS, -1);  
		int	new_fd = accept(sock_fd, NULL, NULL);
		if (new_fd == -1)
		{
			std::perror("Accept failed");
			return 1;
		}
    it++;
    std::cout << "Connection Accept on: " << new_fd << " iteraitor " << it << std::endl;
	}
  
	return 0;
}
