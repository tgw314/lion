CFLAGS=-std=gnu11 -g -static
SRCS=$(filter-out $(wildcard test*.c), $(wildcard *.c))
OBJS=$(SRCS:.c=.o)

# stage1
lion: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJS): lion.h

# stage2
stage2/lion: $(OBJS:%=stage2/%)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

stage2/%.o: stage2/%.s
	as -o $@ $<

stage2/%.s: lion stage2/%.c
	./lion -o $@ stage2/$*.c

stage2/%.c: lion self.py lion.h %.c
	mkdir -p stage2
	./self.py lion.h $*.c > stage2/$*.c

# stage3
stage3/lion: $(OBJS:%=stage3/%)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

stage3/%.o: stage3/%.s
	as -o $@ $<

stage3/%.s: stage2/lion stage2/%.c
	mkdir -p stage3
	./stage2/lion -o $@ stage2/$*.c

# test
test: test-stage1 test-stage2 test-stage3

test-stage1: lion test.c test_common.o
	cc -o- -E -P -C test.c | ./lion -o tmp.s -
	cc -o tmp tmp.s test_common.o
	./tmp

test-stage2: stage2/lion test.c test_common.o
	cc -o- -E -P -C test.c | ./stage2/lion -o tmp.s -
	cc -o tmp tmp.s test_common.o
	./tmp

test-stage3: stage2/lion stage3/lion
	diff stage2/lion stage3/lion

clean:
	rm -rf lion *.o *~ tmp* stage*

.PHONY: test test-stage1 test-stage2 clean
.PRECIOUS: stage2/%.s stage2/%.c stage3/%.s
