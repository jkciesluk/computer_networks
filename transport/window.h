/*
Jakub Ciesluk
323892
*/
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/select.h>

#define SEGMENT_SIZE 1000
#define MAX_SEGMENTS 1024
#define WINDOW_SIZE 700

typedef struct window
{
  uint32_t start;
  uint32_t size;
  uint32_t filesize;
  uint32_t end;
  char acks[WINDOW_SIZE];
  char data[WINDOW_SIZE * SEGMENT_SIZE];

} window_t;

void init_window(window_t *window, uint32_t filesize);

void shift_and_write(window_t *window, FILE *file);

uint32_t is_set(window_t *window, uint32_t n);
void set_ack(window_t *window, uint32_t n);
uint32_t all_set(window_t *window);