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

void setup_server(int sockfd)
{
  struct sockaddr_in server_address;
  bzero(&server_address, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(PORT_NUMBER);
  server_address.sin_addr.s_addr = htonl(INADDR_ANY);
  if (bind(sockfd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
  {
    fprintf(stderr, "bind error: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
  int broadcastPermission = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (void *)&broadcastPermission, sizeof(broadcastPermission)) < 0)
  {
    fprintf(stderr, "setsockopt error: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
}

void listen_for_messages(int sockfd, routing_table_t *routing_table, interfaces_t *interfaces)
{
  // create timeout for select
  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = ROUND_TIME * 1000;
  int max_fd = sockfd + 1;
  // create fd_set for select
  fd_set read_fds;
  FD_ZERO(&read_fds);
  FD_SET(sockfd, &read_fds);
  datagram_t datagram;
  struct sockaddr_in sender;
  socklen_t sender_len = sizeof(sender);
  while (select(max_fd, &read_fds, NULL, NULL, &timeout) != 0)
  {
    ssize_t datagram_len = recvfrom(sockfd, &datagram, sizeof(datagram_t), 0, (struct sockaddr *)&sender, &sender_len);
    if (datagram_len < 0)
    {
      fprintf(stderr, "recvfrom error: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
    }
    datagram.metric = ntohl(datagram.metric);
    update_table(routing_table, interfaces, &datagram, sender.sin_addr.s_addr);
  }
}

void propagate_table(int sockfd, routing_table_t *routing_table, interfaces_t *interfaces)
{
  for (int i = 0; i < interfaces->n; i++)
  {
    uint32_t broadcast = get_broadcast_address(interfaces->interfaces[i].addr, interfaces->interfaces[i].mask);
    struct sockaddr_in broadcast_address;
    bzero(&broadcast_address, sizeof(broadcast_address));
    broadcast_address.sin_family = AF_INET;
    broadcast_address.sin_port = htons(PORT_NUMBER);
    broadcast_address.sin_addr.s_addr = broadcast;
    datagram_t datagram;
    bool broken = false;
    for (int j = 0; j < routing_table->n; j++)
    {
      datagram.ip = routing_table->networks[j].addr;
      datagram.mask = routing_table->networks[j].mask;
      uint32_t metric;
      if (routing_table->networks[j].via == interfaces->interfaces[i].addr &&
          (datagram.ip != get_network_address(interfaces->interfaces[i].addr, interfaces->interfaces[i].mask || datagram.mask != interfaces->interfaces[i].mask)))
        metric = 0xffffffff;
      else
        metric = ntohl(routing_table->networks[j].metric);
      datagram.metric = metric;
      ssize_t datagram_len = sendto(sockfd, &datagram, sizeof(datagram_t), 0, (struct sockaddr *)&broadcast_address, sizeof(broadcast_address));
      if (datagram_len < 0)
      {
        routing_table->networks[i].reachable = false;
        routing_table->networks[i].metric = 0xFFFFFFFF;

        broken = true;
        break;
      }
    }
    if (!broken)
    {
      routing_table->networks[i].reachable = true;
      routing_table->networks[i].isDirectlyConnected = true;
      routing_table->networks[i].metric = interfaces->interfaces[i].metric;
    }
  }
}