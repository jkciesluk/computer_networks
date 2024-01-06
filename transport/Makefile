CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=gnu17

SRCS = window.c utils.c transport.c
HEADERS = window.h utils.h
OBJS = $(SRCS:.c=.o)

.PHONY: all clean

all: transport

transport: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@


clean: 
	rm -f $(OBJS)

distclean:
	rm -f $(OBJS) transport
