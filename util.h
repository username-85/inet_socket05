#ifndef UTIL_H
#define UTIL_H

#include <sys/socket.h>
#include <netdb.h>

int inet_connect(const char *host, const char *service, int type);
int inet_listen(const char *service, int backlog, socklen_t *addrlen);
int inet_bind(const char *service, int type, socklen_t *addrlen);
char * inet_addr_str(const struct sockaddr *addr, socklen_t addrlen,
                     char *addr_str, int addr_str_len);

// get port for local socket (not for peer socket)
int socket_service(int sockfd, char *srv, size_t srvlen);

void err_sys_exit(char *msg);
void err_exit(char *msg);

#endif
