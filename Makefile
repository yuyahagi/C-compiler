CFLAGS=-Wall -Wextra -pedantic -std=c11 -g
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

cc: $(OBJS)
	cc $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

$(OBJS): cc.h

.PHONY: test
test: cc
	./cc -test
	./test.sh

.PHONY: clean
clean:
	rm -f cc *.o tmp* test/tmp*

