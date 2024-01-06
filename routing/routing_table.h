/*
Jakub Ciesluk
323892
*/

#include <stdlib.h>
#include <stdio.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <time.h>

#define MAX_ENTRIES 40

typedef struct routing_table_entry
{
    uint32_t addr;
    uint8_t mask;
    uint32_t via;
    bool isDirectlyConnected;
    uint32_t metric;
    time_t lastUpdated;
    bool reachable;
    uint8_t infititySent;
} routing_table_entry_t;

typedef struct routing_table
{
    routing_table_entry_t networks[MAX_ENTRIES];
    int n;
} routing_table_t;

typedef struct interface
{
    uint32_t addr;
    uint8_t mask;
    uint32_t metric;
    bool reachable;
} interface_t;

typedef struct interfaces
{
    interface_t interfaces[MAX_ENTRIES];
    int n;
} interfaces_t;

// create struct with size of 9 bytes, 4 first bytes is ip address, then 1 byte mask and 4 bytes metric
#pragma pack(1)
typedef struct datagram
{
    uint32_t ip;     // 4 bytes for IP address
    uint8_t mask;    // 1 byte for mask
    uint32_t metric; // 4 bytes for metric
} datagram_t;

void init_table_from_interfaces(routing_table_t *table, interfaces_t *interfaces);

void init_interfaces(interfaces_t *interfaces);

void print_table(routing_table_t *table);

void update_table(routing_table_t *table, interfaces_t *interfaces, datagram_t *datagram, uint32_t interface_ip);
bool dist_is_inf(uint32_t dist);
uint32_t get_network_address(uint32_t ip, uint8_t mask);
uint32_t get_broadcast_address(uint32_t ip, uint8_t mask);
void update_reachability(routing_table_t *table);
void my_inet_ntoa(uint32_t ip, char *str);