CFLAGS=-std=gnu11 -g -static
SRCS=$(filter-out $(wildcard test*.c), $(wildcard *.c))
OBJS=$(SRCS:.c=.o)

lion: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJS): lion.h

stage2/lion: $(OBJS:%=stage2/%)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

stage2/%.o: stage2/%.s
	$(CC) $(CFLAGS) -c -o $@ $<

stage2/%.s: lion self.py lion.h %.c
	mkdir -p stage2
	./self.py lion.h $*.c > stage2/$*.c
	./lion stage2/$*.c > $@

test: test-stage1 test-stage2

test-stage1: lion test.c test_common.o
	cc -o- -E -P -C test.c | ./lion - > tmp.s
	cc -o tmp tmp.s test_common.o
	./tmp

test-stage2: stage2/lion test.c test_common.o
	cc -o- -E -P -C test.c | ./stage2/lion - > tmp.s
	cc -o tmp tmp.s test_common.o
	./tmp

clean:
	rm -rf lion *.o *~ tmp* stage2/

.PHONY: test test-stage1 test-stage2 clean
.PRECIOUS: stage2/%.s
