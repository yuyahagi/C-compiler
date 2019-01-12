CFLAGS=-Wall -Wextra -pedantic -std=c11 -g

cc: cc.c

test: cc
	./test.sh

clean:
	rm -f cc *.o tmp*
