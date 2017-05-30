#include "util.h"
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static int inet_psocket(const char *service, int type,
                        socklen_t *addrlen, bool do_listen, int backlog);

int inet_connect(const char *host, const char *service, int type)
{
	struct addrinfo hints;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = type;

	struct addrinfo *res;
	if (getaddrinfo(host, service, &hints, &res) != 0)
		return -1;

	int sfd;
	struct addrinfo *ai;
	for (ai = res; ai != NULL; ai = ai->ai_next) {
		sfd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if (sfd == -1)
			continue;

		if (connect(sfd, ai->ai_addr, ai->ai_addrlen) != -1)
			break; // success

		close(sfd);
	}

	freeaddrinfo(res);

	return (ai == NULL) ? -1 : sfd;
}

int inet_listen(const char *service, int backlog, socklen_t *addrlen)
{
	return inet_psocket(service, SOCK_STREAM, addrlen, true, backlog);
}

int inet_bind(const char *service, int type, socklen_t *addrlen)
{
	return inet_psocket(service, type, addrlen, false, 0);
}

char * inet_addr_str(const struct sockaddr *addr, socklen_t addrlen,
                     char *addr_str, int addr_str_len)
{
	char host[NI_MAXHOST], service[NI_MAXSERV];

	if (getnameinfo(addr, addrlen, host, NI_MAXHOST,
	                service, NI_MAXSERV,
	                NI_NUMERICHOST | NI_NUMERICSERV) == 0)
		snprintf(addr_str, addr_str_len, "%s:%s", host, service);
	else
		snprintf(addr_str, addr_str_len, "?UNKNOWN?");

	return addr_str;
}

int socket_service(int sockfd, char *srv, size_t srvlen)
{
	struct sockaddr_storage ss;
	socklen_t len = sizeof(ss);

	if (getsockname(sockfd, (struct sockaddr *)&ss, &len))
		return -1;

	char host[NI_MAXHOST], service[NI_MAXSERV];

	if (getnameinfo((struct sockaddr *)&ss, len, host, NI_MAXHOST,
	                service, NI_MAXSERV,
	                NI_NUMERICHOST | NI_NUMERICSERV) == 0) {
		snprintf(srv, srvlen, "%s", service);
	} else {
		snprintf(srv, srvlen, "ERR");
		return -1;
	}

	return 0;
}

void err_sys_exit(char *msg)
{
	perror(msg);
	exit(EXIT_FAILURE);
}

void err_exit(char *msg)
{
	fprintf(stderr, msg);
	exit(EXIT_FAILURE);
}

static int inet_psocket(const char *service, int type, socklen_t *addrlen,
                        bool do_listen, int backlog)
{
	struct addrinfo hints;
	struct addrinfo *res, *ai;
	int sfd;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;
	hints.ai_socktype = type;
	hints.ai_family = AF_UNSPEC;        /* IPv4 or IPv6 */
	hints.ai_flags = AI_PASSIVE;

	if ( getaddrinfo(NULL, service, &hints, &res) != 0)
		return -1;

	for (ai = res; ai != NULL; ai = ai->ai_next) {
		sfd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if (sfd == -1)
			continue;

		if (do_listen) {
			int yes = 1;
			if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR,
			               &yes, sizeof(yes)) == -1) {
				close(sfd);
				freeaddrinfo(res);
				return -1;
			}
		}

		if (bind(sfd, ai->ai_addr, ai->ai_addrlen) == 0)
			break;                      /* success */

		close(sfd);
	}

	if (ai != NULL && do_listen) {
		if (listen(sfd, backlog) == -1) {
			freeaddrinfo(res);
			return -1;
		}
	}

	if (ai != NULL && addrlen != NULL)
		*addrlen = ai->ai_addrlen;

	freeaddrinfo(res);

	return (ai == NULL) ? -1 : sfd;
}

