cc: cc.c

test: cc
	./test.sh

clean:
	rm -f cc *.o tmp*
