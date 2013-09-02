
#ifndef __SMARTGRID_SMARTCOMMON_EVENT_H__
#define __SMARTGRID_SMARTCOMMON_EVENT_H__


#include <stdint.h>
#include <sys/epoll.h>


extern int event_init(void);
extern void *event_set(int fd, void *priv, int dure_sec,
		int (*read)(int, void *priv),
		int (*write)(int, void *priv),
		int (*timeout)(int, void *priv));
extern int event_add(void *handle, unsigned int event);
extern int event_mod(void *handle, unsigned int event);
extern void event_dispatch_loop(void);
extern void event_destroy(void *handle);

extern int event_timer(void *priv, int (*timer)(void *priv));


#define ERROR_AND_EXIT(func) do { perror(#func); exit(-1); } while (0)

#endif
