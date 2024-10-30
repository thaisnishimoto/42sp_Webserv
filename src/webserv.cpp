#include <sys/socket.h> //socket
#include <sys/types.h> //socket
#include <cerrno> //perror
#include <cstdio> //perror
#include <netdb.h> //bind
#include <cstring> //memset

#define PORT 8081
#define MAX_QUEUE 1 

int	main(void)
{
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

	while(42)
	{
		int	new_fd = accept(sock_fd, NULL, NULL);
		if (new_fd == -1)
		{
			std::perror("Accept failed");
			return 1;
		}
	}

	return 0;
}
