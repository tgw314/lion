CFLAGS=-std=c11 -g -static
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

lion: $(OBJS)
	$(CC) -o lion $(OBJS) $(LDFLAGS)

$(OBJS): lion.h

test: lion
	./test.sh

clean:
	rm -f lion *.o *~ tmp*

.PHONY: test clean
