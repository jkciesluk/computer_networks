/*
Jakub Ciesluk
323892
*/
#include "window.h"

#define HEADER_SIZE 32
#define RTT 100

int create_socket();
void send_datagram(int sockfd, struct sockaddr_in server_address, uint32_t start, uint32_t size);
void request_part(int sockfd, struct sockaddr_in server_address, window_t *window);
void read_responses(int sockfd, struct sockaddr_in server_address, window_t *window);