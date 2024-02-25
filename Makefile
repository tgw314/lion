CFLAGS=-std=c11 -g -static

lion: lion.c

test: lion
	./test.sh

clean:
	rm -f lion *.o *~ tmp*

.PHONY: test clean
