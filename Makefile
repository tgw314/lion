CFLAGS=-std=gnu11 -g -static
SRCS=$(filter-out $(wildcard test*.c), $(wildcard *.c))
OBJS=$(SRCS:.c=.o)

lion: $(OBJS)
	$(CC) -o lion $(OBJS) $(LDFLAGS)

$(OBJS): lion.h

test: lion test.c test_common.o
	TEST=$$(mktemp --suffix=.c) && \
	cc -o $$TEST -E -P -C test.c && \
	./lion $$TEST > tmp.s && \
	cc -o tmp tmp.s test_common.o && \
	./tmp && \
	rm -f $$TEST

clean:
	rm -f lion *.o *~ tmp*

.PHONY: test clean
