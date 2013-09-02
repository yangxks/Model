
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>

#include <pthread.h>
#include <sys/stat.h>
#include <sys/mman.h>


#include "common.h"
#include "event.h"

/***************** configure ***************/
static const char *listen_ip = "0.0.0.0";
static const char *listen_port = "80";
static const char *bind_ip = "0.0.0.0";
static unsigned int max_connections = 4096>>2;
static unsigned int max_sec_connections = 4096>>2;
static unsigned int max_sec_requests = 0;
static unsigned int max_duration_sec = -1;

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
				listen_ip, listen_port);
		request_len =  strlen(request);
		return request_len;
	}
	return -1;
}

/***************** status ***************/

struct thread_spec {
	unsigned int cur_connected_connections;
	unsigned int cur_connecting_connections;
	unsigned int cur_sec_connections;
	unsigned int cur_sec_requests;

	unsigned int l_connected_connections;
	unsigned int l_connecting_connections;
	unsigned int l_sec_connections;
	unsigned int l_sec_requests;
};


static pthread_key_t thread_spec_key;

static void thread_key_create(void)
{
	load_request();
	pthread_key_create(&thread_spec_key, NULL);
}

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
	}

	return s;
}



struct node {
	int fd;

#define CONNECTING		0
#define CONNECTED		1
	int state;

	char *pos_write;	/** 需要写入的数据 **/
	int len_write;		/** 需要写入的数据长度 **/

	void *handle;
};


int client_read(int fd, void *priv)
{
	int ret;
	char buff[BUFSIZ];
	struct node *n = priv;
	struct thread_spec *s = spec_get();

	if ((ret = read(fd, buff, BUFSIZ)) <= 0) {
		if (errno == EAGAIN) {
			goto out;
		}
		goto close_connection;
	}
out:
	return 0;
close_connection:
	if (n->state == CONNECTING) {
		s->cur_connecting_connections --;
	} else {
		s->cur_connected_connections --;
	}

	close(n->fd);
	event_destroy(n->handle);
	free(n);
	return -1;
}

int client_write(int fd, void *priv)
{
	struct thread_spec *s = spec_get();
	struct node *n = priv;
	int ret;

	/** 立即关闭 **/
	if (max_duration_sec == 0) {
		s->cur_connecting_connections --;
		close(n->fd); 
		event_destroy(n->handle);
		free(n);
		return 123;
	}

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

	/** 判断是否写完 **/
	if (n->len_write == 0) {
		if (++ s->cur_sec_requests >= max_sec_requests) {
			s->l_sec_requests = s->cur_sec_requests;
			s->cur_sec_requests = 0;
			event_mod(n->handle, EPOLLIN);
		}
	}
	return 0;
}

int client_timeout(int fd, void *priv)
{
	struct thread_spec *s = spec_get();
	struct node *n = priv;

	if (max_sec_requests == 0) {
		/** life timeout, close it **/
		if (n->state == CONNECTING) {
			s->cur_connecting_connections --;
		} else {
			s->cur_connected_connections --;
		}
		close(n->fd); 
		event_destroy(n->handle);
		free(n);
		return 123;
	}
	event_mod(n->handle, EPOLLIN | EPOLLOUT);
	return 0;
}


static void show_thread_spec(void)
{
	struct thread_spec *s = spec_get();
	printf("thread: %lu, active: %u, "
			"non-active: %u, "
			"new connections: %u/s, "
			"requests: %u/s\n",
			pthread_self(), 
			s->l_connected_connections, 
			s->l_connecting_connections, 
			s->l_sec_connections, 
			s->l_sec_requests);
}

static int connect_timer(void *priv)
{
	struct node *n;
	unsigned short port = atoi(listen_port);
	struct thread_spec *s = spec_get();

	while (1) {
		/** 当前连接数与最大连接数比较 **/
		if (s->cur_connecting_connections + s->cur_connected_connections 
				>= max_connections) {
			goto next;
		}

		/** 每秒新建连接数比较 **/
		if (s->cur_sec_connections >= max_sec_connections) {
			goto next;
		}

		/** 新建链接 **/
		if ((n = calloc(1, sizeof(struct node))) == NULL) {
			goto next;
		}
		if ((n->fd = tcp_connect(listen_ip, port, bind_ip)) < 0) {
			goto error;
		}
		n->state = CONNECTING;
		if ((n->handle = event_set(n->fd, n, max_duration_sec, client_read, client_write,
						client_timeout)) == NULL) {
			goto error1;
		}

		if (event_add(n->handle, EPOLLIN | EPOLLOUT) != 0) {
			goto error2;
		}

		s->cur_connecting_connections ++;
		s->cur_sec_connections ++;
	}

	if (0) {
error2:
		event_destroy(n->handle);
error1:
		close(n->fd);
error:
		free(n);
next:
		show_thread_spec();
		s->l_connected_connections = s->cur_connected_connections;
		s->l_connecting_connections = s->cur_connecting_connections;
		s->l_sec_connections = s->cur_sec_connections;
		s->cur_sec_connections = 0;
	}
	return 0;
}

static void usage(void)
{
	printf("\nclient: -D -i ip -p port -b bindip\n");
	printf("\t\t-D:         Daemon process\n");
	printf("\t\t-i ip:      Server ip to be connected\n");
	printf("\t\t-p port:    Server port to be connected\n");
	printf("\t\t-b bindip:  Local ip.\n");
	printf("\t\t-n num:     New connections per second, default: 4096\n");
	printf("\t\t-m num:     Max connections, default: 4096\n");
	printf("\t\t-t sec:     life of each connection, default: -1\n");
	printf("\t\t                     -1: nevel stop\n");
	printf("\t\t                      0: close immediate after connected\n");
	printf("\t\t                 others: close after sec seconds\n");
	printf("\t\t-r num:     requests number per second. default: 0\n");
	printf("\t\t-f file:    request file.\n");
	printf("\t\t-h:         Show this help.\n");

	printf("\n");
	exit(-1);
}

int main(int argc, char **argv)
{
	int c;
	int tobe_daemon = 0;
	extern char *optarg;

	while ((c = getopt(argc, argv, "i:p:Db:m:n:t:r:f:h")) > 0) {
		switch (c) {
			case 'i':				   /** listen ip **/
				listen_ip = optarg;
				break;
			case 'p':				   /** listen port **/
				listen_port = optarg;
				break;
			case 'b':
				bind_ip = optarg;
				break;
			case 'm':
				max_connections = atoi(optarg);
#if 0
				if (max_connections > 60000) {
					printf("max connections \"%d\" will be aligned to \"60000\"\n",
							atoi(optarg));
					optarg = "60000";
					max_connections = 60000;
				}
#endif
				max_connections = (max_connections+3)/4;

				if ((atoi(optarg) & 0x3) != 0) {
					printf("max connections \"%d\" will be aligned to \"%d\"\n",
							atoi(optarg), max_connections*4);
				}
				break;
			case 'n':
				max_sec_connections = atoi(optarg);
				max_sec_connections = (max_sec_connections+3)/4;
				if ((atoi(optarg) & 0x3) != 0) {
					printf("new connections \"%d\" will be aligned to \"%d\"\n",
							atoi(optarg), max_sec_connections*4);
				}
				break;
			case 't':
				max_duration_sec = atoi(optarg);
				break;
			case 'r':
				printf("request is omited now. \n");
				break;

				max_sec_requests	= atoi(optarg);
				max_sec_requests = (max_sec_requests+3)/4;
				if ((atoi(optarg) & 0x3) != 0) {
					printf("requests number \"%d\" will be aligned to \"%d\"\n",
							atoi(optarg), max_sec_requests*4);
				}
				printf("the life of connection will be ormit.\n");
				max_duration_sec = 1;
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
		sleep(1);
	}

	return 0;
}
