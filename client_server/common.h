

#ifndef __COMMON_H__
#define __COMMON_H__

#include <arpa/inet.h>

int tcp_listen(const char *ip, unsigned short port);
int tcp_accept(int lsnfd, struct sockaddr_in *addr);
int tcp_connect(const char *ip, unsigned short port, const char *bind_ip);

int set_nolimit(void);

#endif
