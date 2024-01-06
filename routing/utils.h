/*
Jakub Ciesluk
323892
*/
#include "routing_table.h"
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/select.h>

#define ROUND_TIME 5000
#define PORT_NUMBER 54321

int create_socket();
void setup_server(int sockfd);
void listen_for_messages(int sockfd, routing_table_t *routing_table, interfaces_t *interfaces);
void propagate_table(int sockfd, routing_table_t *routing_table, interfaces_t *interfaces);