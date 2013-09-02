


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>

#include <pthread.h>


#include "common.h"
#include "event.h"

static const char *listen_ip = "0.0.0.0";
static const char *listen_port = "80";

struct node {
	int fd;
	struct sockaddr_in addr;

#define REQUEST_LENGTH	512
	char request[REQUEST_LENGTH];
	int len_read;

	char *pos_write;
	int len_write;

	void *handle;
};

static int client_read(int fd, void *priv)
{
	int ret;
	struct node *n = priv;

	if ((ret = read(fd, n->request + n->len_read, 
					REQUEST_LENGTH - n->len_read)) <= 0) {
		goto close_connection;
	}
	n->len_read += ret;

	if (n->len_read >= REQUEST_LENGTH) {
		n->request[REQUEST_LENGTH-1] = 0;
	} else {
		n->request[n->len_read] = 0;
	}

	if (n->len_read < 4) {
		return 0;
	}

	if (strstr(n->request, "\n\n") != NULL ||
			strstr(n->request, "\r\n\r\n") != NULL) {
		n->len_read = 0;
		event_mod(n->handle, EPOLLIN | EPOLLOUT);
	}
	return 0;
close_connection:
	close(fd);
	event_destroy(n->handle);
	free(n);
	return 123;
}

static int client_write(int fd, void *priv)
{
	const char *buff = 
		"HTTP/1.1 200 OK\r\n"
		"Content-Length: 0\r\n"
		"Content-Type: text/html\r\n"
		"Connection: keep-alive\r\n"
		"\r\n";
	struct node *n = priv;
	int ret;

	/** 重新初始化 **/
	if (n->len_write == 0) {
		n->len_write = strlen(buff);
		n->pos_write = (char*)buff;
	}

	/** 执行写操作 **/
	if ((ret = write(fd, n->pos_write, n->len_write)) > 0) {
		n->len_write -= ret;
		n->pos_write += ret;
	} else if (ret < 0 && errno != EAGAIN) {
		goto close_connection;
	}

	/** 判断是否写完 **/
	if (n->len_write == 0) {
		event_mod(n->handle, EPOLLIN);
	}
	return 0;
close_connection:
	close(fd);
	event_destroy(n->handle);
	free(n);
	return 123;
}

static int server_accept(int lsnfd, void *priv)
{
	struct node *n;

	if ((n = calloc(1, sizeof(struct node))) == NULL) {
		close(accept(lsnfd, NULL, NULL));
		return 0;
	}

	if ((n->fd = tcp_accept(lsnfd, &n->addr)) < 0) {
		goto error;
	}

	if ((n->handle = event_set(n->fd, n, 0, client_read, client_write,
					NULL)) == NULL) {
		goto error1;
	}

	if (event_add(n->handle, EPOLLIN) != 0) {
		goto error2;
	}
	return 0;
error2:
	event_destroy(n->handle);
error1:
	close(n->fd);
error:
	free(n);
	return -1;
}

int server_listen(const char *ip, unsigned short port)
{
	int fd;
	struct event *e;
	if ((fd = tcp_listen(ip, port)) < 0) {
		ERROR_AND_EXIT(event_add);
	}

	if ((e = event_set(fd, NULL, 0, server_accept, NULL, NULL)) == NULL) {
		ERROR_AND_EXIT(event_set);
	}
	if (event_add(e, EPOLLIN) != 0) {
		ERROR_AND_EXIT(event_add);
	}
	return 0;
}

static void usage(void)
{
	printf("\nserver: -D -i ip -p port\n");
	printf("\t\t-D:         Daemon process\n");
	printf("\t\t-i ip:      Server ip to be listened\n");
	printf("\t\t-p port:    Server port to be listened\n");
	printf("\t\t-h:         Show this help.\n");

	printf("\n");
	exit(-1);
}

static pthread_key_t thread_spec_key;

static void thread_key_create(void)
{
	server_listen(listen_ip, atoi(listen_port));
	pthread_key_create(&thread_spec_key, NULL);
}

static int server_timer(void *priv)
{
	static pthread_once_t once_control = PTHREAD_ONCE_INIT;
	pthread_once(&once_control, thread_key_create);
	return 0;
}


int main(int argc, char **argv)
{
	int c;
	int tobe_daemon = 0;
	extern char *optarg;

	while ((c = getopt(argc, argv, "i:p:Dh")) > 0) {
		switch (c) {
			case 'i':				   /** listen ip **/
				listen_ip = optarg;
				break;
			case 'p':				   /** listen port **/
				listen_port = optarg;
				break;
			case 'D':				  /** daemon process **/
				tobe_daemon = 1;
				break;
			default:
				usage();
				break;
		}
	}

	if (tobe_daemon) {
		daemon(1, 1);
	}

	if (getuid() == 0) {
		set_nolimit();
	}

	if (event_init() < 0) {
		ERROR_AND_EXIT(event_init);
	}
	event_dispatch_loop();
	event_timer(NULL, server_timer);

	while (1) {
		sleep(1);
	}
	return 0;
}
