#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>  

extern int errno;
int connectTCP(const char *hostname, const char *service, const char *transport)
{
	struct sockaddr_in client;
	struct hostent *host;
	struct servent *serv;
	int sockfd = 0, n = 0,port=0, optval = 1;

	memset(&client, 0, sizeof(client));
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("client: socket error: %s\n", strerror(errno));
		exit(1);
	}

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
        {
                printf("client: error setting SO_REUSEADDR socket option: %s\n", strerror(errno));
                exit(1);
        }

	client.sin_family = PF_INET;

	if(host = gethostbyname(hostname))
		memcpy(&client.sin_addr, host->h_addr, host->h_length);
	else if(inet_pton(AF_INET, hostname, &client.sin_addr) <= 0)
	{
		printf("client: Invalid address: %s\n", hostname);
		exit(1);
	}

	if(serv = getservbyname(service, "tcp"))
		client.sin_port = serv->s_port;
	else
	{
		printf("client: getservbyname couldn't find %s\n", service);
		exit(1);
	}

	if(connect(sockfd, (struct sockaddr *)&client, sizeof(client)) < 0)
	{
		printf("client: connect error: %s\n", strerror(errno));
		exit(1);
	}
	return sockfd;                  
}
