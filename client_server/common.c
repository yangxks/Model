

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/resource.h>

#include "common.h"

static int set_nonblock(int fd)
{
	int flag;

	if ((flag = fcntl(fd, F_GETFL, 0)) < 0) {
		return -1;
	}
	return fcntl(fd, F_SETFL, flag | O_NONBLOCK);
}


static int set_socket_reuse(int fd)
{
	int on = 1;
	return setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
}

static int set_socket_linger(int fd)
{
	struct linger l = {1, 0};
	
	return setsockopt(fd, SOL_SOCKET, SO_LINGER, &l, sizeof(l));
}


static int tcp_bind(int fd, const char *ip, unsigned short port)
{
	struct sockaddr_in addr;

	if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0) {
		perror("inet_pton");
		return -1;
	}

	if (set_socket_reuse(fd) != 0) {
		perror("setsockopt");
		return -1;
	}

	if (port == 0) {
		static unsigned short lport = 1025;
		if (lport < 1025) {
			lport = 1025;
		}
		port = lport ++;
	}

	memset(&addr, '\0', sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);

	if (bind(fd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
		return -1;
	}
	return 0;
}

int tcp_listen(const char *ip, unsigned short port)
{
	int fd;

	if ((fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		perror("socket");
		goto out;
	}

	if (tcp_bind(fd, ip, port) != 0) {
		goto err;
	}

	if (set_nonblock(fd) < 0) {
		perror("set_nonblock");
		goto err;
	}

	if (listen(fd, 8192) == -1) {
		perror("listen");
		goto err;
	}

	return fd;
err:
	close(fd);
out:
	return -1;
}




int tcp_accept(int lsnfd, struct sockaddr_in *addr)
{
	int fd;
	socklen_t len = sizeof(struct sockaddr);

	if ((fd = accept(lsnfd, (struct sockaddr *) addr, &len)) < 0) {
		return -1;
	}

	if (set_nonblock(fd) < 0) {
		close(fd);
		return -1;
	}

	if (set_socket_linger(fd) != 0) {
		perror("setsockopt");
		return -1;
	}

	return fd;
}


int tcp_connect(const char *ip, unsigned short port,
		const char *bind_ip)
{
	struct sockaddr_in addr;
	int fd, tried = 5;
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		perror("socket");
		goto out;
	}

again:
	if (bind_ip && tcp_bind(fd, bind_ip, 0) != 0) {
		if (-- tried > 0) {
			goto again;
		}
		goto err;
	}

	if (set_nonblock(fd) < 0) {
		perror("set_nonblock");
		goto err;
	}

	if (set_socket_linger(fd) != 0) {
		perror("setsockopt");
		return -1;
	}

	memset(&addr, '\0', sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0) {
		perror("inet_pton");
		goto err;
	}

	if (connect(fd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
		if (errno != EINPROGRESS) {
			//fprintf(stderr, "errno = %d, str = %s\n", errno, strerror(errno));
			goto err;
		}
	}
	return fd;
err:
	close(fd);
out:
	return -1;
}


static int write_to_system_config(const char *file, const char *value)
{
	char buff[BUFSIZ];
	if (access(file, F_OK) != 0) {
		return 0;
	}
	sprintf(buff, "echo \"%s\" > %s", value, file);
	system(buff);
	return 0;
}


int set_nolimit(void)
{
	struct rlimit rlim;
	if (getrlimit(RLIMIT_NOFILE, &rlim) != 0) {
		perror("getrlimit");
		return -1;
	}
	rlim.rlim_cur = rlim.rlim_max = 1 << 20 /*RLIM_INFINITY*/;
	if (setrlimit(RLIMIT_NOFILE, &rlim) != 0) {
		perror("setrlimit");
		return -1;
	}

	write_to_system_config("/proc/sys/net/ipv4/ip_local_port_range", "1025 65535");
	write_to_system_config("/proc/sys/net/ipv4/tcp_fin_timeout", "5");
	write_to_system_config("/proc/sys/net/ipv4/ip_conntrack_max", "5000000");
	write_to_system_config("/proc/sys/net/nf_conntrack_max", "5000000");
	write_to_system_config("/proc/sys/net/ipv4/tcp_max_syn_backlog", "2000000");
	write_to_system_config("/proc/sys/net/ipv4/netfilter/ip_conntrack_max", "5000000");


	return 0;
}
