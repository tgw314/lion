CFLAGS=-std=gnu11 -g -static
SRCS=$(filter-out test.c, $(wildcard *.c))
OBJS=$(SRCS:.c=.o)

lion: $(OBJS)
	$(CC) -o lion $(OBJS) $(LDFLAGS)

$(OBJS): lion.h

test: lion
	./test.sh

clean:
	rm -f lion *.o *~ tmp*

.PHONY: test clean
