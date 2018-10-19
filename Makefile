CFLAGS=-O3 -Wall
LIBS=-lnet -pthread
OBJS=arp_send.o

arp-send: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIBS) -o arp-send

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: install
install:
	cp arp-send /usr/local/sbin/arp-send

.PHONY: uninstall
uninstall:
	rm -f /usr/local/sbin/atp-send

.PHONY: clean
clean:
	rm -f arp-send
	rm -f *.o
