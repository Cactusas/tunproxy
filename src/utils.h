/*
 * @file utils.h
 * @author Tadas Ivanovas <ivanovastadas@gmail.com>
 * @date June 8, 2022
 */

#ifndef __TUNPROXY_UTILS_H__
#define __TUNPROXY_UTILS_H__

uint16_t util_ip_checksum(void* vdata, size_t length);
void util_iptolink(void *vdata, struct link_ep *link);
int util_is_udp(void *vdata);
int util_sock_add_nonblock(int sfd);
int util_cmd(const char *cmd);

#endif //__TUNPROXY_UTILS_H__