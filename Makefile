CFLAGS=-std=gnu11 -g -static
SRCS=$(filter-out $(wildcard test*.c), $(wildcard *.c))
OBJS=$(SRCS:.c=.o)

lion: $(OBJS)
	$(CC) -o lion $(OBJS) $(LDFLAGS)

$(OBJS): lion.h

test: lion test.c test_common.o
	cc -o- -E -P -C test.c | ./lion - > tmp.s
	cc -o tmp tmp.s test_common.o
	./tmp

clean:
	rm -f lion *.o *~ tmp*

.PHONY: test clean
