#include "util.h"
#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	char *usage_msg = "usage: client ip protocol(tcp or udp) \"message\"\n";
	if (argc < 4)
		err_exit(usage_msg);

	int stype = -1;
	if (strcmp(argv[2], "tcp") == 0) {
		stype = SOCK_STREAM;
	} else if (strcmp(argv[2], "udp") == 0) {
		stype = SOCK_DGRAM;
	} else {
		fprintf(stderr, "unknown protocol %s\n", argv[2]);
		err_exit(usage_msg);
	}

	int srv_sfd = inet_connect(argv[1], PORT_SRV, stype);
	if (srv_sfd < 0)
		err_exit("socket error\n");

	char msg[MAXDSIZE] = {0};
	strncpy(msg, argv[3], MAXDSIZE - 1);
	if (send(srv_sfd, msg, strlen(msg), 0) == -1)
		err_sys_exit("send");

	char buf[MAXDSIZE] = {0};
	int numbytes = recv(srv_sfd, buf, MAXDSIZE - 1, 0);
	if (numbytes == -1)
		err_sys_exit("recv");

	buf[numbytes] = '\0';
	printf("received message '%s'\n", buf);

	exit(EXIT_SUCCESS);
}

