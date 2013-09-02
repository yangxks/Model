

CFLAGS ?= -g -Wall -Werror

LDFLAGS = -pthread


COMMON = common.o event.o

all: server client client1 

server: $(COMMON) server.o
	cc $(LDFLAGS) -o $@ $^
	-@strip $@

client: $(COMMON) client.o
	cc $(LDFLAGS) -o $@ $^
	-@strip $@

client1: $(COMMON) client1.o
	cc $(LDFLAGS) -o $@ $^
	-@strip $@

client2: $(COMMON) client2.o
	cc $(LDFLAGS) -o $@ $^
	#-@strip $@

-include .depend

%.o: %.c
	cc $(CFLAGS) -o $@ -c $<


clean:
	rm -fr server client client1 client2 *.o

distclean: clean
	rm -fr .depend

dep depend:
	cc $(CFLAGS) -M -MM *.c > .depend
