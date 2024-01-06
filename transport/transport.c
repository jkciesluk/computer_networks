/*
Jakub Cieśluk
323892
*/
#include "utils.h"

int main(int argc, char **argv)
{
  if (argc != 5)
  {
    fprintf(stderr, "usage: %s <ip_address> <port_number> <filename> <size>\n", argv[0]);
    exit(EXIT_FAILURE);
  }
  uint32_t ip_addr = inet_addr(argv[1]);
  uint32_t port_number = atoi(argv[2]);
  char *filename = argv[3];
  uint32_t size = atoi(argv[4]);
  int sockfd = create_socket();
  struct sockaddr_in server_addr;
  bzero(&server_addr, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port_number);
  server_addr.sin_addr.s_addr = ip_addr;

  FILE *file = fopen(filename, "w");

  if (file == NULL)
  {
    fprintf(stderr, "fopen error: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
  window_t window;
  init_window(&window, size);
  while (window.start < window.end)
  {
    request_part(sockfd, server_addr, &window);
    read_responses(sockfd, server_addr, &window);
    shift_and_write(&window, file);
    uint32_t received = window.start == window.end ? size : window.start * SEGMENT_SIZE;
    printf("%0.3f%% done\n", (float)received / size * 100);
  }

  return 0;
}