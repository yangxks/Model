

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <netinet/in.h>

#include "list.h"
#include "event.h"
#include <pthread.h>


#define EPOLL_THREADS		4

struct event {
	int epfd;
	int fd;
	void *priv;
	int dure_sec;

	time_t last;

	struct list_head list;

	int (*read) (int fd, void *priv);
	int (*write) (int fd, void *priv);
	int (*timeout) (int fd, void *priv);
};



#define HASH_SIZE	16
struct thread_spec {
	int epfd;
	struct list_head queue[HASH_SIZE];
};

static pthread_key_t thread_spec_key;

static void thread_key_create(void)
{
	pthread_key_create(&thread_spec_key, NULL);
}

static struct thread_spec *spec_get(void)
{
	int i;
	struct thread_spec *s;
	static pthread_once_t once_control = PTHREAD_ONCE_INIT;
	pthread_once(&once_control, thread_key_create);

	if ((s = pthread_getspecific(thread_spec_key)) == NULL) {
		if ((s = calloc(1, sizeof(struct thread_spec))) == NULL) {
			ERROR_AND_EXIT(calloc);
		}
		for (i = 0; i < HASH_SIZE; i ++) {
		INIT_LIST_HEAD(&s->queue[i]);
		}
		pthread_setspecific(thread_spec_key, s);
	}

	return s;
}



int event_init(void)
{
#if 0
	int i;

	for (i = 0; i < EPOLL_THREADS; i++) {
		if ((epfds[i] = epoll_create(4096)) < 0) {
			ERROR_AND_EXIT(epoll_create);
		}
	}
#endif
	return 0;
}

void event_destroy(void *handle)
{
	struct event *e = handle;
	list_del(&e->list);
	free(e);
}


int event_add(void *handle, unsigned int event)
{
	struct epoll_event ev;
	struct event *e = handle;
	struct thread_spec *s = spec_get();
	static int i = 0;

	memset(&ev, '\0', sizeof(struct epoll_event));
	ev.events = event;
	ev.data.ptr = e;

	if (epoll_ctl(e->epfd, EPOLL_CTL_ADD, e->fd, &ev) != 0) {
		perror("epoll_ctl");
		goto out;
	}

	e->last = time(NULL);
	list_add_tail(&e->list, &s->queue[(i++)&(HASH_SIZE-1)]);
	return 0;
out:
	return -1;
}

extern int event_mod(void *handle, unsigned int event)
{
	struct epoll_event ev;
	struct event *e = handle;
	memset(&ev, '\0', sizeof(struct epoll_event));
	ev.events = event;
	ev.data.ptr = e;

	if (epoll_ctl(e->epfd, EPOLL_CTL_MOD, e->fd, &ev) == -1) {
		perror("epoll_ctl");
		goto out;
	}

	e->last = time(NULL);
	return 0;
out:
	return -1;
}


void *event_set(int fd, void *priv, int dure_sec,
		int (*read) (int, void *), int (*write) (int, void *),
		int (*timeout)(int, void *priv))
{
	struct event *e;
	struct thread_spec *s = spec_get();

	if ((e = calloc(1, sizeof(struct event))) == NULL)
		goto out;

	e->epfd = s->epfd;
	e->fd = fd;
	e->dure_sec = dure_sec;
	e->priv = priv;
	e->read = read;
	e->write = write;
	e->timeout = timeout;

	return e;
out:
	return NULL;
}


struct ev_timer {
	struct list_head list;
	void *priv;
	int (*timer)(void *priv);
};

static LIST_HEAD(time_queue);

int event_timer(void *priv, int (*timer)(void *priv))
{
	struct ev_timer *t;

	if ((t = malloc(sizeof(struct ev_timer))) == NULL) {
		return -1;
	}

	t->priv = priv;
	t->timer = timer;

	list_add_tail(&t->list, &time_queue);
	return 0;
}

static void *event_thread(void *arg)
{
	int i, count;
	struct event *e, *tmp;
	struct epoll_event ev[8192];
	struct ev_timer *t;
	time_t last, now;
	struct thread_spec *s = spec_get();

	now = last = time(NULL); /** get now **/

	pthread_detach(pthread_self());

	if ((s->epfd = epoll_create(4096)) < 0) {
		ERROR_AND_EXIT(epoll_create);
	}

	for (;;) {
		count = epoll_wait(s->epfd, ev, 8192, 1);
		for (i = 0; i < count; i++) {
			e = (struct event *) ev[i].data.ptr;
			if (ev[i].events & EPOLLIN && e->read) {
				if (e->read(e->fd, e->priv) != 0)
					continue;
			}
			if (ev[i].events & EPOLLOUT && e->write) {
				if (e->write(e->fd, e->priv) != 0) {
					continue;
				}
			}
			e->last = now;
		}

		if ((now = time(NULL)) != last) {
			list_for_each_entry(t, &time_queue, list) {
				t->timer(t->priv);
			}

			for (i = 0; i < HASH_SIZE; i ++) {
				count = 0;
				list_for_each_entry_safe(e, tmp, &s->queue[i], list) {
					if (count ++ >= 500) {
						break;
					}
					if (e->dure_sec > 0 && e->timeout && 
							(now - e->last) >= e->dure_sec) {
						if (e->timeout(e->fd, e->priv) != 0) {
							continue;
						}
					}

					list_del(&e->list);
					list_add_tail(&e->list, &s->queue[i]);
				}
			}

			last = now;
		}
	}
	pthread_exit(NULL);
}

void event_dispatch_loop(void)
{
	int i;
	pthread_t pid;
	for (i = 0; i < EPOLL_THREADS; i++) {
		if (pthread_create(&pid, NULL, event_thread, NULL) != 0) {
			ERROR_AND_EXIT(pthread_create);
		}
	}
}
