/*
 * @file links.h
 * @author Tadas Ivanovas <ivanovastadas@gmail.com>
 * @date June 8, 2022
 */

#ifndef __TUNPROXY_LINKS_H__
#define __TUNPROXY_LINKS_H__

struct link_ep{
  uint32_t src_addr;
  uint32_t dst_addr;
  uint16_t src_port;
  uint16_t dst_port;
};

void link_add(const struct link_ep *l);
int link_exist(const struct link_ep *l);
const struct link_ep* link_find_by_dst(uint32_t dst_addr, uint16_t dst_port);
int link_remove(const struct link_ep *l);
void link_init();

#endif //__TUNPROXY_LINKS_H__
