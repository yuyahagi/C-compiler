CFLAGS=-Wall -Wextra -pedantic -std=c11 -g
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

cc: $(OBJS)
	cc $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

$(OBJS): cc.h

test: cc
	./cc -test
	./test.sh

clean:
	rm -f cc *.o tmp*
