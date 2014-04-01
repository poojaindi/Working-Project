#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include<sys/types.h>
#include<sys/signal.h>
#include<sys/socket.h>
#include<sys/time.h>
#include<sys/resource.h>
#include<sys/wait.h>
#include<sys/errno.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<stdarg.h>
#include<netdb.h>

#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif
#define LINELEN 127
u_short portbase = 0;
typedef unsigned short u_short;
extern int errno;
int errexit(const char *format, ...);
extern int errno;
int TCPdaytime(const char *host, const char *service);
int errexit(const char *format, ...);
int connectTCP(const char *host, const char *service);
int connectsock(const char *host, const char *service,const char *transport);
int main(int argc, char *argv[])
{
    char *host = "localhost"; /* host to use if none supplied */
    char *service = "echo"; /* default service port */
    switch (argc) {
        case 1:
            host = "localhost";
            break;
        case 3:
            service = argv[2];
        case 2:
            host = argv[1];
            break;
        default:
            fprintf(stderr, "usage: TCPecho [host [port]]\n");
            exit(1);
    }

    TCPecho(host, service);
    exit(0);
}

int TCPecho(const char *host, const char *service)
{
    char buf[LINELEN+1]; 
    int s, n; 
    int outchars, inchars;
    s = connectTCP(host, service);
    while(fgets(buf, sizeof(buf), stdin)){
        buf[LINELEN] = '\0';
        outchars = strlen(buf);
        (void) write(s, buf, outchars);

        for(inchars = 0; inchars < outchars; inchars+=n){
            n = read(s, &buf[inchars], outchars - inchars);
            if(n < 0)
            {
                errexit("socket read failed: %s\n", strerror(errno));
            }
        }
        fputs(buf, stdout);
    }
}


int connectTCP(const char *host, const char *service )
{
    return connectsock( host, service, "tcp");
}

int
    connectsock(const char *host, const char *service, const char *transport )
    {
        struct hostent *phe;
        struct servent *pse; 
        struct protoent *ppe; 
        struct sockaddr_in sin; 
        int s, type;
        memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;

        sin.sin_addr.s_addr = INADDR_ANY;
        sin.sin_port = 7;

        if ( pse = getservbyname(service, transport) )
            sin.sin_port = pse->s_port;
        else if ( (sin.sin_port = htons((u_short)atoi(service))) == 0 )
            errexit("can't get \"%s\" service entry\n", service);

        if ( phe = gethostbyname(host) )
            memcpy(&sin.sin_addr, phe->h_addr, phe->h_length);
        else if ( (sin.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE )
            errexit("can't get \"%s\" host entry\n", host);
        if ( (ppe = getprotobyname(transport)) == 0)
            errexit("can't get \"%s\" protocol entry\n", transport);

        if (strcmp(transport, "udp") == 0)
            type = SOCK_DGRAM;
        else
            type = SOCK_STREAM;
        s = socket(PF_INET,SOCK_STREAM , 0);
        if (s < 0)
            errexit("can't create socket: %s\n", strerror(errno));
        if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
            errexit("can't connect to %s.%s: %s\n", host, service,
            strerror(errno));
        return s;
}

int errexit(const char *format, ...)
{
        va_list args;
     
        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);
        exit(1);
}

