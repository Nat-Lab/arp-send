CFLAGS=-O3 -Wall
LIBS=-lnet
OBJS=arp_send.o

arp-send: $(OBJS)
	$(CC) $(CFLAGS) $(LIBS) -o arp-send $(OBJS)

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
