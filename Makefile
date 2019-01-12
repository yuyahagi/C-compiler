CFLAGS=-Wall -Wextra -pedantic -std=c11

cc: cc.c

test: cc
	./test.sh

clean:
	rm -f cc *.o tmp*
