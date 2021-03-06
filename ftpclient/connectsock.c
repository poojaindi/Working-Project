#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>  

#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif /* INADDR_NONE */


extern int errno;
int connectsock(const char *hostname, const char *service, const char *transport)
{
	struct sockaddr_in client;
	struct hostent *host;
	struct servent *serv;
	int sockfd = 0, n = 0,port=0;

	memset(&client, '0', sizeof(client));
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("\nFailed to creates socket \n");
		exit(1);
	}
	client.sin_family = AF_INET;
	if(serv=getservbyname(service, "tcp"))
		client.sin_port = htons(serv->s_port);
	else
	{
		printf("NO service \n");
		exit(1);
	}
	if(host=gethostbyname(hostname))
		memcpy(&client.sin_addr,host->h_addr,host->h_length);
	else

		if(inet_pton(AF_INET, hostname, &client.sin_addr)<=0)
		{
			printf("\n Invalid address\n");
			exit( 1);
		}
	if(connect(sockfd, (struct sockaddr *)&client, sizeof(client)) < 0)
	{
		printf("\nFailed to connect\n");
		exit(1);
	}
	return sockfd;                  
}




/*
int connectsock2(const char *hostname, const char *service, const char *transport)
{
	int sock_fd, sock_type;
	struct hostent *hptr;
	struct servent *sptr;
	struct protoent *pptr;
	struct sockaddr_in sin;

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = PF_INET;

	//Map hostname to IP address
	if (hptr = gethostbyname(hostname))
		memcpy(&sin.sin_addr, hptr->h_addr, hptr->h_length);
	else if ((sin.sin_addr.s_addr = inet_addr(hostname)) == INADDR_NONE)
		errexit("Cant get %s host entry\n", hostname);

	//Map service name to port number
	if (sptr = getservbyname(service, transport))
		sin.sin_port = sptr->s_port;
	else if ((sin.sin_port = htons((u_short)atoi(service))) == 0)
		errexit("Cant get %s service entry\n", service);

	//Map protocol name to a protocol number
	if ((pptr = getprotobyname(transport)) == 0)
		errexit("Cant get %s protocol entry\n", transport);
	
	if (strcmp(transport, "udp") == 0)
		sock_type = SOCK_DGRAM;
	else
		sock_type = SOCK_STREAM;
	
	//Allocate a socket
	sock_fd = socket(PF_INET, sock_type, pptr->p_proto);
	if (sock_fd < 0)
		errexit ("Cant create socket: %s\n", strerror(errno));
	
	//Conect the socket
	if (connect(sock_fd, (struct sockaddr *)&sin, sizeof(sin)) < 0)
		errexit("Cant connect to %s. %s: %s\n", hostname, service, strerror(errno));

	return sock_fd;
}

*/
