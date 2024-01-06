CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=gnu17

SRCS = routing_table.c utils.c rip.c
HEADERS = routing_table.h utils.h
OBJS = $(SRCS:.c=.o)

.PHONY: all clean

all: router

router: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@


clean: 
	rm -f $(OBJS)

distclean:
	rm -f $(OBJS) router
