CFLAGS = -O2 -std=gnu99 -g -Wall -lm

TARGETS=busyboy sleepyboy

.PHONY: all clean install install-links

all: ${TARGETS}

busyboy: busyboy.c Makefile
	$(CC) -o $@ $(CFLAGS) $< -lpthread -lm

sleepyboy: sleepyboy.c Makefile
	$(CC) -o $@ $(CFLAGS) $<

clean:
	-rm -f *.o *~ ${TARGETS}

install: ${TARGETS}
	install -b -m 0755 $^ ${DEST_BIN}

install-links: ${TARGETS}
	mkdir -p ${DEST_BIN}
	ln -fs $(foreach n,$^,$(shell pwd)/${n}) ${DEST_BIN}/
