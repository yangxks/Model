
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


#include "list.h"
#include "common.h"
#include "event.h"

/***************** configure ***************/
static const char *remote_ip = "0.0.0.0";
static unsigned short remote_port = 80;
static const char *bind_ip = "0.0.0.0";


struct thread_spec {
	unsigned int cur_sec_connectings;	/** 正在建立的 **/
	unsigned int cur_sec_connections;	/** 成功的 **/
	unsigned int cur_sec_errors;		/** 失败的 **/
	unsigned int l_sec_connectings;
	unsigned int l_sec_connections;
	unsigned int l_sec_errors;
	struct list_head list;
};


static pthread_key_t thread_spec_key;

static void thread_key_create(void)
{
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




static void *doit(void *arg)
{
	int epfd;
	int i, ret;
	int fd;
	time_t last = time(NULL), now;

	struct thread_spec *s = spec_get();

	pthread_detach(pthread_self());

	if ((epfd = epoll_create(4096)) < 0) {
		exit(-1);
	}


	while (1) {
		struct epoll_event ev;
		struct epoll_event evs[BUFSIZ];
		/* 首先进行1000个连接 */
		for (i = 0; i < 1000; i ++) {
			if ((fd = tcp_connect(remote_ip, remote_port, bind_ip)) < 0) {
				break;
			}

			ev.events = EPOLLIN | EPOLLOUT;
			ev.data.fd = fd;
			epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
			s->cur_sec_connectings ++;
		}

		ret = epoll_wait(epfd, evs, BUFSIZ, 0);
		for (i = 0; i < ret; i ++) {
			fd = evs[i].data.fd;
			if (evs[i].events & EPOLLIN) {
				/** connect error **/
				close(fd);
				s->cur_sec_errors ++;
			} else if (evs[i].events & EPOLLOUT) {
				/** connect ok **/
				close(fd);
				s->cur_sec_connections ++;
			}
		}

		if (last != (now = time(NULL))) {
			s->l_sec_connections = s->cur_sec_connections;
			s->l_sec_connectings = s->cur_sec_connectings;
			s->l_sec_errors = s->cur_sec_errors;
			s->cur_sec_connectings = 0;
			s->cur_sec_connections = 0;
			s->cur_sec_errors = 0;
			last = now;
		}
	}

	pthread_exit(NULL);
}

static void usage(void)
{
	printf("\nclient: -D -i ip -p port -b bindip\n");
	printf("\t\t-D:         Daemon process\n");
	printf("\t\t-i ip:      Server ip to be connected\n");
	printf("\t\t-p port:    Server port to be connected\n");
	printf("\t\t-b bindip:  Local ip.\n");
	printf("\t\t-h:         Show this help.\n");

	printf("\n");
	exit(-1);
}


/** 测试新建连接数 **/
int main(int argc, char **argv)
{
	int c;
	int tobe_daemon = 0;
	extern char *optarg;
	pthread_t pid;

	while ((c = getopt(argc, argv, "i:p:b:Dh")) > 0) {
		switch (c) {
			case 'i':				   /** listen ip **/
				remote_ip = optarg;
				break;
			case 'p':				   /** listen port **/
				remote_port = atoi(optarg);
				break;
			case 'b':
				bind_ip = optarg;
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

	for (c = 0; c < 10; c ++) {
		pthread_create(&pid, NULL, doit, NULL);
	}


	while (1) {
		struct thread_spec *s;
		sleep(1);
		unsigned int num1 = 0;
		unsigned int num2 = 0;
		unsigned int num3 = 0;

		list_for_each_entry(s, &thread_spec_head, list) {
			num1 += s->l_sec_connectings;
			num2 += s->l_sec_connections;
			num3 += s->l_sec_errors;
		}

		fprintf(stderr, "New Connections: %d/s, OK: %d/s, ERROR: %d/s\n", num1, num2, num3);

	}

	return 0;
}
