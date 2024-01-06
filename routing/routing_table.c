/*
Jakub Ciesluk
323892
*/
#include "routing_table.h"

void init_interfaces(interfaces_t *interfaces)
{
  int n;

  scanf("%d", &n);
  interfaces->n = n;
  for (int i = 0; i < n; i++)
  {
    uint8_t mask;
    uint32_t ip[4];
    uint32_t metric;
    scanf("%d.%d.%d.%d/%hhd distance %d", &ip[0], &ip[1], &ip[2], &ip[3], &mask, &metric);
    // transform ip to uint32_t in network byte order
    uint32_t addr = 0;
    addr = addr | (ip[3] << 24);
    addr = addr | (ip[2] << 16);
    addr = addr | (ip[1] << 8);
    addr = addr | ip[0];
    interfaces->interfaces[i].addr = addr;
    interfaces->interfaces[i].mask = mask;
    interfaces->interfaces[i].metric = metric;
    interfaces->interfaces[i].reachable = true;
  }
}

void init_table_from_interfaces(routing_table_t *table, interfaces_t *interfaces)
{
  table->n = interfaces->n;
  for (int i = 0; i < table->n; i++)
  {
    uint32_t addr = get_network_address(interfaces->interfaces[i].addr, interfaces->interfaces[i].mask);
    table->networks[i].addr = addr;
    table->networks[i].mask = interfaces->interfaces[i].mask;
    table->networks[i].via = interfaces->interfaces[i].addr;
    table->networks[i].isDirectlyConnected = true;
    table->networks[i].metric = interfaces->interfaces[i].metric;
    table->networks[i].lastUpdated = time(NULL);
    table->networks[i].reachable = true;
    table->networks[i].infititySent = 0;
  }
}

void print_table(routing_table_t *table)
{
  printf("\n");
  for (int i = 0; i < table->n; i++)
  {
    uint32_t ip = table->networks[i].addr;
    char addr_str[20];
    inet_ntop(AF_INET, &ip, addr_str, sizeof(addr_str));
    uint8_t mask = table->networks[i].mask;
    uint32_t via = table->networks[i].via;
    uint32_t metric = table->networks[i].metric;
    bool reachable = table->networks[i].reachable;
    bool isDirectlyConnected = table->networks[i].isDirectlyConnected;
    if (isDirectlyConnected && reachable)
    {
      printf("%s/%d distance %d connected directly\n", addr_str, mask, metric);
    }
    else if (isDirectlyConnected)
    {
      printf("%s/%d unreachable connected directly\n", addr_str, mask);
    }
    else if (reachable && !dist_is_inf(metric))
    {
      char via_str[20];
      my_inet_ntoa(via, via_str);
      printf("%s/%d distance %d via %s\n", addr_str, mask, metric, via_str);
      table->networks[i].infititySent = 0;
    }
    else if (table->networks[i].infititySent < 3)
    {
      char via_str[20];
      my_inet_ntoa(via, via_str);
      printf("%s/%d distance inf via %s\n", addr_str, mask, via_str);
      table->networks[i].infititySent += 1;
    }
  }
}

bool dist_is_inf(uint32_t dist)
{
  return dist > 32;
}

void update_table(routing_table_t *table, interfaces_t *interfaces, datagram_t *datagram, uint32_t interface_ip)
{
  uint32_t addr = datagram->ip;
  uint8_t mask = datagram->mask;
  uint32_t metric = datagram->metric;
  for (int i = 0; i < interfaces->n; i++)
  {
    if (interfaces->interfaces[i].addr == interface_ip)
    {
      return;
    }
    if (get_network_address(interfaces->interfaces[i].addr, interfaces->interfaces[i].mask) == get_network_address(interface_ip, interfaces->interfaces[i].mask) && !dist_is_inf(metric))
    {
      metric += interfaces->interfaces[i].metric;
      break;
    }
  }
  for (int i = 0; i < table->n; i++)
  {
    if (table->networks[i].addr == addr && table->networks[i].mask == mask)
    {
      if (table->networks[i].metric > metric && !dist_is_inf(metric))
      {
        table->networks[i].metric = metric;
        table->networks[i].isDirectlyConnected = false;
        table->networks[i].via = interface_ip;
        table->networks[i].lastUpdated = time(NULL);
        table->networks[i].reachable = true;
      }
      else if (table->networks[i].via == interface_ip && !table->networks[i].isDirectlyConnected)
      {
        table->networks[i].metric = metric;
        table->networks[i].lastUpdated = time(NULL);
        table->networks[i].reachable = true;
      }
      return;
    }
  }
  if (table->n == MAX_ENTRIES || dist_is_inf(metric))
    return;

  table->networks[table->n].addr = addr;
  table->networks[table->n].mask = mask;
  table->networks[table->n].metric = metric;
  table->networks[table->n].via = interface_ip;
  table->networks[table->n].lastUpdated = time(NULL);
  table->networks[table->n].reachable = true;
  table->networks[table->n].isDirectlyConnected = false;
  table->networks[table->n].infititySent = 0;
  table->n++;
  return;
}

void update_reachability(routing_table_t *table)
{
  time_t now = time(NULL);
  for (int i = 0; i < table->n; i++)
  {
    if (now - table->networks[i].lastUpdated > 30 && !table->networks[i].isDirectlyConnected)
    {
      table->networks[i].reachable = false;
      table->networks[i].metric = 0xFFFFFFFF;
    }
  }
}

uint32_t get_broadcast_address(uint32_t ip, uint8_t mask)
{
  uint32_t broadcast = ip | (0xFFFFFFFF << mask);
  return broadcast;
}

uint32_t get_network_address(uint32_t ip, uint8_t mask)
{
  uint32_t network = ip & (0xFFFFFFFF >> (32 - mask));
  return network;
}

void my_inet_ntoa(uint32_t ip, char *str)
{
  sprintf(str, "%d.%d.%d.%d", ip & 0xFF, (ip >> 8) & 0xFF, (ip >> 16) & 0xFF, (ip >> 24) & 0xFF);
}