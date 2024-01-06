// Jakub Ciesluk 323892

#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

u_int16_t compute_icmp_checksum(const void *buff, int length)
{
  u_int32_t sum;
  const u_int16_t *ptr = buff;
  assert(length % 2 == 0);
  for (sum = 0; length > 0; length -= 2)
    sum += *ptr++;
  sum = (sum >> 16) + (sum & 0xffff);
  return (u_int16_t)(~(sum + (sum >> 16)));
}

int create_icmp_socket()
{
  int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
  if (sockfd < 0)
  {
    fprintf(stderr, "socket error: %s\n", strerror(errno));
    return EXIT_FAILURE;
  }
  return sockfd;
}

struct sockaddr_in get_recipient_socket(char *ipaddr)
{
  struct sockaddr_in recipient;
  memset(&recipient, 0, sizeof(recipient));
  recipient.sin_family = AF_INET;
  inet_pton(AF_INET, ipaddr, &recipient.sin_addr);
  return recipient;
}

struct icmp make_icmp_header(uint16_t id, uint16_t seq)
{
  struct icmp header;
  header.icmp_type = ICMP_ECHO;
  header.icmp_code = 0;
  header.icmp_hun.ih_idseq.icd_id = id;
  header.icmp_hun.ih_idseq.icd_seq = seq;
  header.icmp_cksum = 0;
  header.icmp_cksum = compute_icmp_checksum(
      (u_int16_t *)&header, sizeof(header));
  return header;
}

int send_icmp_packet(int sockfd, struct sockaddr_in recipient, int ttl, uint16_t id)
{
  struct icmp header = make_icmp_header(id, ttl);

  setsockopt(sockfd, IPPROTO_IP, IP_TTL, &ttl, sizeof(int));
  ssize_t packet_len = sendto(sockfd, &header, sizeof(header), 0,
                              (struct sockaddr *)&recipient, sizeof(recipient));
  if (packet_len < 0)
  {
    fprintf(stderr, "sendto error: %s\n", strerror(errno));
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

int select_receiver(int sockfd, struct timeval *timeout)
{
  fd_set readfds;
  FD_ZERO(&readfds);
  FD_SET(sockfd, &readfds);
  return select(sockfd + 1, &readfds, NULL, NULL, timeout);
}

ssize_t recv_packet(int sockfd, struct sockaddr_in *sender, unsigned char *buffer)
{
  socklen_t sender_len = sizeof(*sender);
  ssize_t packet_len = recvfrom(sockfd, buffer, IP_MAXPACKET, 0,
                                (struct sockaddr *)sender, &sender_len);
  if (packet_len < 0)
  {
    fprintf(stderr, "recvfrom error: %s\n", strerror(errno));
    return EXIT_FAILURE;
  }

  return packet_len;
}

int is_icmp_reply(unsigned char *buffer, uint16_t seq, uint16_t id)
{
  struct ip *ip_header = (struct ip *)buffer;
  ssize_t ip_header_len = 4 * ip_header->ip_hl;
  struct icmp *icmp_header = (struct icmp *)(buffer + ip_header_len);
  ssize_t icmp_header_len = 8;
  if (icmp_header->icmp_type == ICMP_ECHOREPLY)
  {
    if (icmp_header->icmp_hun.ih_idseq.icd_id == id && icmp_header->icmp_hun.ih_idseq.icd_seq == seq)
      return 1;
  }
  else if (icmp_header->icmp_type == ICMP_TIME_EXCEEDED)
  {
    struct ip *ip_header = (struct ip *)(buffer + ip_header_len + icmp_header_len);
    ssize_t ip_header_len = 4 * ip_header->ip_hl;
    struct icmp *icmp_header = (struct icmp *)(buffer + ip_header_len + ip_header_len + icmp_header_len);
    if (icmp_header->icmp_hun.ih_idseq.icd_id == id && icmp_header->icmp_hun.ih_idseq.icd_seq == seq)
      return 1;
  }
  return 0;
}

float receive_packets(int sockfd, struct in_addr *senders, int *received_msgs, int ttl, uint16_t id)
{
  struct timeval timeout;
  timeout.tv_sec = 1;
  timeout.tv_usec = 0;
  float rtt = 0;
  int n_replies = 0;
  int select_res;
  while ((select_res = select_receiver(sockfd, &timeout)) != 0)
  {
    unsigned char buffer[IP_MAXPACKET];
    struct sockaddr_in sender;
    recv_packet(sockfd, &sender, buffer);
    if (!is_icmp_reply(buffer, ttl, id))
      continue;
    senders[n_replies] = sender.sin_addr;
    n_replies++;
    rtt += (float)(1000000 - timeout.tv_usec) / 1000.0;
    if (n_replies == 3)
      break;
  }
  *received_msgs = n_replies;
  return rtt / 3.0;
}

int dest_in_repliers(struct in_addr *repliers, int n_repliers, struct sockaddr_in *dest)
{
  int dest_replies = 0;
  for (int i = 0; i < n_repliers; i++)
    if (repliers[i].s_addr == dest->sin_addr.s_addr)
      dest_replies++;
  return dest_replies;
}

void print_replies(struct in_addr *repliers, int n_repliers, int ttl, float mean_rtt)
{
  printf("%d. ", ttl);
  for (int i = 0; i < n_repliers; i++)
  {
    int was_printed = 0;
    for (int j = i - 1; j >= 0; j--)
    {
      if (repliers[i].s_addr == repliers[j].s_addr)
      {
        was_printed = 1;
        break;
      }
    }
    if (was_printed)
      continue;
    char *ip = inet_ntoa(repliers[i]);
    printf("%s ", ip);
  }
  if (n_repliers == 0)
    printf("*\n");
  else if (n_repliers == 3)
    printf("%.3f ms\n", mean_rtt);
  else
    printf("???\n");
}

int main(int argc, char **argv)
{
  if (argc != 2)
  {
    fprintf(stderr, "Usage: %s <hostname>\n", argv[0]);
    return EXIT_FAILURE;
  }

  struct sockaddr_in dest = get_recipient_socket(argv[1]);
  int sockfd = create_icmp_socket();
  pid_t pid = getpid();

  int replies = 0;
  struct in_addr repliers[3];
  float mean_rtt = 0;
  int dest_replies = 0;
  printf("%s \n", inet_ntoa(dest.sin_addr));
  for (int ttl = 1; ttl < 30; ttl++)
  {
    for (int i = 0; i < 3; i++)
      (void)send_icmp_packet(sockfd, dest, ttl, pid);
    mean_rtt = receive_packets(sockfd, repliers, &replies, ttl, pid);
    if ((dest_replies = dest_in_repliers(repliers, replies, &dest)) == 3)
    {
      print_replies(repliers, dest_replies, ttl, mean_rtt);
      break;
    }
    print_replies(repliers, replies, ttl, mean_rtt);
  }

  return EXIT_SUCCESS;
}