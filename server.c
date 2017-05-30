#include "util.h"
#include "common.h"

#include <errno.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <poll.h>

//maxlen for full address
#define MAX_ADDR_LEN (INET6_ADDRSTRLEN + MAX_SERVICE_LEN)
#define SA struct sockaddr

static void sigchld_handler(int unused);
static void process_tcp_request(int sfd);
static void echo_tcp(int sfd);
static void process_udp_request(int sfd);

int main(void)
{
	struct sigaction sa;
	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1)
		err_sys_exit("sigaction");

	socklen_t srv_addrlen_tcp = 0;
	int srv_sfd_tcp = inet_listen(PORT_SRV, BACKLOG, &srv_addrlen_tcp);
	if (srv_sfd_tcp < 0)
		err_exit("could not create tcp socket\n");

	printf("waiting for connections on tcp port %s\n", PORT_SRV);

	socklen_t srv_addrlen_udp = 0;
	int srv_sfd_udp = inet_bind(PORT_SRV, SOCK_DGRAM, &srv_addrlen_udp);
	if (srv_sfd_udp < 0)
		err_exit("could not create udp socket\n");

	printf("waiting for connections on udp port %s\n", PORT_SRV);

	unsigned sfds_num = 2; // 1 tcp + 1 udp
	struct pollfd sfds[sfds_num];
	sfds[0].fd = srv_sfd_tcp;
	sfds[0].events = POLLRDNORM;
	sfds[1].fd = srv_sfd_udp;
	sfds[1].events = POLLRDNORM;

	while(1) {
		int rc = poll(sfds, sfds_num, -1);
		if (rc < 0) {
			if (errno == EINTR)
				continue;
			else
				err_sys_exit("poll");
		}

		for (unsigned i = 0; i < sfds_num; i++) {
			if ( !(sfds[i].revents & POLLRDNORM) )
				continue;

			if (sfds[i].fd == srv_sfd_tcp)
				process_tcp_request(srv_sfd_tcp);
			else if (sfds[i].fd == srv_sfd_udp)
				process_udp_request(srv_sfd_udp);
		}
	}

	exit(EXIT_SUCCESS);
}

static void process_tcp_request(int sfd)
{
	struct sockaddr_storage client_addr;
	socklen_t ca_size = sizeof(client_addr);

	int client_sfd = accept(sfd, (SA *)&client_addr, &ca_size);

	if (client_sfd == -1) {
		perror("accept");
		return;
	}

	char addr_str[MAX_ADDR_LEN] = {0};
	inet_addr_str((SA *)&client_addr, ca_size,
	              addr_str, sizeof(addr_str));
	printf("got tcp connection from %s\n", addr_str);

	switch(fork()) {
	case -1:
		perror("fork");
		break;
	case 0:
		;
		pid_t pid = getpid();
		printf("process %d started\n", pid);
		close(sfd);
		echo_tcp(client_sfd);
		close(client_sfd);
		_exit(0);
		break;
	default: // do nothing
		break;
	}

	close(client_sfd);
}

static void echo_tcp(int sfd)
{
	char buf[MAXDSIZE] = {0};
	int numbytes = recv(sfd, buf, MAXDSIZE - 1, 0);
	if (numbytes == -1) {
		perror("recv");
		return;
	}
	buf[numbytes] = '\0';
	printf("process %d got message '%s' from sfd %d\n", getpid(), buf, sfd);

	if (send(sfd, buf, strnlen(buf, MAXDSIZE), 0) == -1)
		perror("send");
}

static void process_udp_request(int sfd)
{
	struct sockaddr_storage client_addr;
	socklen_t ca_size = sizeof(client_addr);

	char buf[MAXDSIZE] = {0};
	int numbytes = recvfrom(sfd, buf, MAXDSIZE - 1, 0, (SA *)&client_addr,
	                        &ca_size);
	if (numbytes == -1) {
		perror("recvfrom");
		return;
	}

	buf[numbytes] = '\0';

	char addr_str[MAX_ADDR_LEN] = {0};
	inet_addr_str((SA *)&client_addr, ca_size,
	              addr_str, sizeof(addr_str));
	printf("got udp message '%s' from %s and sfd is %d\n",
	       buf, addr_str, sfd);

	numbytes = sendto(sfd, buf, strnlen(buf, MAXDSIZE), 0,
	                  (SA *)&client_addr, ca_size);
	if (numbytes == -1)
		perror("sendto");
}

static void sigchld_handler(int unused)
{
	(void)unused;

	// waitpid() might overwrite errno
	int saved_errno = errno;

	while(waitpid(-1, NULL, WNOHANG) > 0);

	errno = saved_errno;
}

