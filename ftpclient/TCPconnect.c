int connectsock(const char *hostname, const char *service, const char *transport);

int connectTCP(const char *hostname, const char *service)
{
	return connectsock(hostname, service, "tcp");
}
