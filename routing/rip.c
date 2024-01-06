/*
Jakub Ciesluk
323892
*/
#include "utils.h"

int main()
{
  int sockfd = create_socket();
  setup_server(sockfd);
  interfaces_t *interfaces = malloc(sizeof(interfaces_t));
  init_interfaces(interfaces);
  routing_table_t *routing_table = malloc(sizeof(routing_table_t));
  init_table_from_interfaces(routing_table, interfaces);
  while (1)
  {
    print_table(routing_table);
    propagate_table(sockfd, routing_table, interfaces);
    listen_for_messages(sockfd, routing_table, interfaces);
    update_reachability(routing_table);
  }
  free(interfaces);
  free(routing_table);
}