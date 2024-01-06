/*
Jakub Ciesluk
323892
*/
#include "utils.h"

int create_socket()
{
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0)
  {
    fprintf(stderr, "socket error: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
  return sockfd;
}

void send_datagram(int sockfd, struct sockaddr_in server_address, uint32_t start, uint32_t size)
{
  char buffer[HEADER_SIZE];
  bzero(buffer, HEADER_SIZE);
  sprintf(buffer, "GET %d %d\n", start, size);
  if (sendto(sockfd, buffer, HEADER_SIZE, 0, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
  {
    fprintf(stderr, "sendto error: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
}

void request_part(int sockfd, struct sockaddr_in server_address, window_t *window)
{
  for (uint32_t i = 0; i < window->size; i++)
  {
    uint32_t segment_n = window->start + i;
    if (segment_n >= window->end)
    {
      break;
    }
    uint32_t start = segment_n * SEGMENT_SIZE;
    uint32_t size = (segment_n == window->end - 1) ? window->filesize % SEGMENT_SIZE : SEGMENT_SIZE;
    if (!is_set(window, i))
    {
      send_datagram(sockfd, server_address, start, size);
    }
  }
}

void read_responses(int sockfd, struct sockaddr_in server_address, window_t *window)
{
  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = RTT * 1000;
  int max_fd = sockfd + 1;
  fd_set read_fds;
  FD_ZERO(&read_fds);
  FD_SET(sockfd, &read_fds);
  char buffer[HEADER_SIZE + SEGMENT_SIZE];
  bzero(buffer, HEADER_SIZE + SEGMENT_SIZE);
  struct sockaddr_in sender_addr;
  socklen_t sender_addr_len = sizeof(sender_addr);
  int datagram_len;
  while (select(max_fd, &read_fds, NULL, NULL, &timeout) != 0)
  {
    if ((datagram_len = recvfrom(sockfd, buffer, HEADER_SIZE + SEGMENT_SIZE, 0, (struct sockaddr *)&sender_addr, &sender_addr_len)) <= 0)
    {
      fprintf(stderr, "recvfrom error: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
    }
    uint32_t start, size;
    sscanf(buffer, "DATA %d %d\n", &start, &size);
    uint32_t segment_n = start / SEGMENT_SIZE;
    char *data = buffer + (datagram_len - size);
    if (sender_addr.sin_addr.s_addr != server_address.sin_addr.s_addr || sender_addr.sin_port != server_address.sin_port)
    {
      continue;
    }
    if (segment_n < window->start || segment_n > window->start + window->size || is_set(window, segment_n - window->start))
    {
      continue;
    }
    set_ack(window, segment_n - window->start);
    for (uint32_t i = 0; i < size; i++)
    {
      window->data[(segment_n - window->start) * SEGMENT_SIZE + i] = data[i];
    }
  }
}