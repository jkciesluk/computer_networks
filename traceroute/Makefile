CFLAGS=-Wall -Wextra -Werror -std=gnu17


all: traceroute.o
	gcc -o traceroute traceroute.o $(CFLAGS)

clean: 
	rm -f *.o

distclean: clean
	rm -f traceroute traceroute.o