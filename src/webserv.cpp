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
  
  int fds_ready = 0;

	char buf[4096];

	const char *response =
				"HTTP/1.1 200 OK\r\n"
				"Content-Type: text/html\r\n"
				"Content-Length: 13\r\n"
				"\r\n"
				"<h1>OIE</h1>";

  struct epoll_event events_list[MAX_EVENTS];
	while(42)
	{
		std::cout << "Calling epoll_wait. fds ready: " << fds_ready << std::endl;
		fds_ready = epoll_wait(epoll_fd, events_list, MAX_EVENTS, -1);  

		for(int i = 0; i < fds_ready; i++)
		{
			if (events_list[i].data.fd == sock_fd && events_list[i].events == EPOLLIN)
			{
				int	new_fd = accept(sock_fd, NULL, NULL);
				if (new_fd == -1)
				{
					std::perror("Accept failed");
					return 1;
				}
				std::cout << "Connection Accept on: " << new_fd << std::endl;

				target_event.events = EPOLLIN | EPOLLOUT;
  				target_event.data.fd = new_fd;
				epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_fd, &target_event);
			}
			else if ((events_list[i].events & EPOLLIN) == EPOLLIN)
			{
				std::cout << "Reading from fd " <<	events_list[i].data.fd << std::endl;
				recv(events_list[i].data.fd, buf, 4096, 0);
				std::cout << buf << std::endl;
			}
			else if ((events_list[i].events & EPOLLOUT) == EPOLLOUT)
			{
				std::cout << "Sending response" << std::endl;
				send(events_list[i].data.fd, response, strlen(response), 0);
				close(events_list[i].data.fd);
			}
			else 
			{
				std::cout << "Fora dos ifs" << std::endl;
			}
		}
	}
  
	return 0;
}
