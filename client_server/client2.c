

#define _GNU_SOURCE
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>

#include <pthread.h>
#include <sys/stat.h>
#include <sys/mman.h>


#include "list.h"
#include "common.h"
#include "event.h"

/***************** configure ***************/
static const char *remote_ip = "0.0.0.0";
static const char *remote_port = "80";
static const char *bind_ip = "0.0.0.0";
static unsigned int max_connections = 4096>>2;

static const char *request_file = NULL;

/***************** request ***************/

static char *request = NULL;
static size_t request_len = 0;

static int load_request(void)
{
	if (request_file != NULL) {
		int fd;
		struct stat st;
		if (stat(request_file, &st) != 0) {
			request_file = NULL;
			goto nofile;
		}

		if ((fd = open(request_file, O_RDONLY)) < 0) {
			request_file = NULL;
			goto nofile;
		}

		if ((request = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0)) == MAP_FAILED) {
			close(fd);
			request_file = NULL;
			goto nofile;
		}
		request_len = st.st_size;
		return st.st_size;
	}
nofile:
	if (request_file == NULL) {
		if ((request = malloc(128)) == NULL) {
			perror("malloc: ");
			exit(-1);
		}
		sprintf(request, "GET / HTTP/1.1\r\n"
				"Host: %s:%s\r\n"
				"Connection: keep-alive\r\n\r\n",
				remote_ip, remote_port);
		request_len =  strlen(request);
		return request_len;
	}
	return -1;
}

/***************** status ***************/

struct thread_spec {
	unsigned int cur_connected_connections;
	unsigned int cur_connecting_connections;
	unsigned int cur_sec_requests;

	struct list_head list;
};


static pthread_key_t thread_spec_key;

static void thread_key_create(void)
{
	load_request();
	pthread_key_create(&thread_spec_key, NULL);
}

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static LIST_HEAD(thread_spec_head);

static struct thread_spec *spec_get(void)
{
	struct thread_spec *s;
	static pthread_once_t once_control = PTHREAD_ONCE_INIT;
	pthread_once(&once_control, thread_key_create);

	if ((s = pthread_getspecific(thread_spec_key)) == NULL) {
		if ((s = calloc(1, sizeof(struct thread_spec))) == NULL) {
			ERROR_AND_EXIT(calloc);
		}
		pthread_setspecific(thread_spec_key, s);

		pthread_mutex_lock(&lock);
		list_add_tail(&s->list, &thread_spec_head);
		pthread_mutex_unlock(&lock);
	}

	return s;
}



struct node {
	int fd;

#define CONNECTING		0
#define CONNECTED		1
	int state;

	char *pos_write;	/** 需要写入的数据, pointer to request_file **/
	int len_write;		/** 需要写入的数据长度 **/

	char header[512];
	char *line;
	int len_read;
	int content_len;
	int header_ok;

	void *handle;
};


int client_read(int fd, void *priv)
{
	int ret;
	struct node *n = priv;
	struct thread_spec *s = spec_get();
	char *p;

	if (n->header_ok) {
		/** 已经有了http头了，只需要读取http-content **/
		goto next;
	}

	if ((ret = read(n->fd, n->header + n->len_read,
					512 - n->len_read)) <= 0) {
		if (errno == EAGAIN) {
			goto out;
		}
		goto close_connection;
	}
	n->len_read += ret;

	if (n->line == NULL) {
		n->line = n->header;
	}

again:

	/** 查找 Content-Length **/
	if (strncmp(n->line, "Content-Length:", strlen("Content-Length:")) == 0) {
		p = n->line + strlen("Content-Length:");
		while (*p == ' ') {p ++;}
		n->content_len = atoi(p);
	}

	/** 查找本行结尾 **/
	if ((p = strchr(n->line, '\r')) != NULL 
			|| (p = strchr(n->line, '\n')) != NULL) {
		n->line = p;
		/** \r\n\r\n **/
		/** \n\n **/

		if (strncmp(n->line, "\r\n\r\n", 4) == 0) {
			n->header_ok = 1;
			s->cur_sec_requests ++;
		} else if (strncmp(n->line, "\n\n", 2) == 0) {
			n->header_ok = 1;
			s->cur_sec_requests ++;
		}

		while (*(n->line) == '\r' || *(n->line) == '\n') {
			n->line ++;
		}

		if (n->header_ok) {
			/** 修正length **/
			n->len_read -= (n->line - n->header);
			goto next;
		}

		goto again;
	} else {
		goto out;
	}

next:

	if (n->header_ok == 1 && n->content_len == 0) {
		goto close_connection;
	}

	if (n->len_read >= n->content_len) {
		goto out;
	}

	if ((ret = read(n->fd, n->header, 512)) <= 0) {
		if (errno == EAGAIN) {
			goto out;
		}
		goto close_connection;
	}

	n->len_read += ret;
out:
	if (n->header_ok == 1 && n->content_len != 0 
			&& n->len_read >= n->content_len) {
		/** read response ok **/
		n->header_ok = 0;
		n->len_read = 0;
		n->content_len = 0;
		n->line = NULL;
		event_mod(n->handle, EPOLLIN | EPOLLOUT);
	}

	return 0;
close_connection:
	if (n->state == CONNECTING) {
		s->cur_connecting_connections --;
	} else {
		s->cur_connected_connections --;
	}

	event_destroy(n->handle);
	close(n->fd);
	free(n);
	return 123;
}

int client_write(int fd, void *priv)
{
	struct thread_spec *s = spec_get();
	struct node *n = priv;
	int ret;

	if (n->state == CONNECTING) {
		s->cur_connecting_connections --;
		s->cur_connected_connections ++;
		n->state = CONNECTED;
	}

	/** 重新初始化 **/
	if (n->len_write == 0) {
		n->len_write = request_len;
		n->pos_write = request;
	}

	/** 执行写操作 **/
	if ((ret = write(fd, n->pos_write, n->len_write)) > 0) {
		n->len_write -= ret;
		n->pos_write += ret;
	}

	if (ret < 0) {
		if (n->state == CONNECTING) {
			s->cur_connecting_connections --;
		} else {
			s->cur_connected_connections --;
		}

		event_destroy(n->handle);
		close(n->fd);
		free(n);
		return 123;
	}

	/** 判断是否写完 **/
	if (n->len_write == 0) {
		event_mod(n->handle, EPOLLIN);
	}
	return 0;
}


static int connect_timer(void *priv)
{
	struct node *n;
	unsigned short port = atoi(remote_port);
	struct thread_spec *s = spec_get();

	while (1) {
		/** 当前连接数与最大连接数比较 **/
		if (s->cur_connecting_connections + s->cur_connected_connections 
				>= max_connections) {
			break;
		}

		/** 新建链接 **/
		if ((n = calloc(1, sizeof(struct node))) == NULL) {
			break;
		}
		if ((n->fd = tcp_connect(remote_ip, port, bind_ip)) < 0) {
			goto error;
		}
		n->state = CONNECTING;
		if ((n->handle = event_set(n->fd, n, -1, client_read, client_write,
						NULL)) == NULL) {
			goto error1;
		}

		if (event_add(n->handle, EPOLLIN | EPOLLOUT) != 0) {
			goto error2;
		}

		s->cur_connecting_connections ++;
	}

	if (0) {
error2:
		event_destroy(n->handle);
error1:
		close(n->fd);
error:
		free(n);
	}
	return 0;
}

static void usage(void)
{
	printf("\nclient: -D -i ip -p port -b bindip\n");
	printf("\t\t-D:         Daemon process\n");
	printf("\t\t-i ip:      Server ip to be connected\n");
	printf("\t\t-p port:    Server port to be connected\n");
	printf("\t\t-m num:     Max connections, default: 4096\n");
	printf("\t\t-f file:    request file.\n");
	printf("\t\t-h:         Show this help.\n");

	printf("\n");
	exit(-1);
}


/** 测试HTTP请求数 **/
int main(int argc, char **argv)
{
	int c;
	int tobe_daemon = 0;
	extern char *optarg;

	while ((c = getopt(argc, argv, "i:p:Db:m:n:t:r:f:h")) > 0) {
		switch (c) {
			case 'i':				   /** listen ip **/
				remote_ip = optarg;
				break;
			case 'p':				   /** listen port **/
				remote_port = optarg;
				break;
			case 'm':
				max_connections = atoi(optarg);
				max_connections = (max_connections+3)/4;

				if ((atoi(optarg) & 0x3) != 0) {
					printf("max connections \"%d\" will be aligned to \"%d\"\n",
							atoi(optarg), max_connections*4);
				}
				break;
			case 'f':
				request_file = optarg;
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

	event_timer(NULL, connect_timer);

	while (1) {
		struct thread_spec *s;
		unsigned int num = 0;

		sleep(1);

		list_for_each_entry(s, &thread_spec_head, list) {
			num += s->cur_sec_requests;
			s->cur_sec_requests = 0;
		}

		fprintf(stderr, "New Requests: %u/s\n", num);
	}

	return 0;
}
