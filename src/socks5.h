/*
 * @file socks5.h
 * @author Tadas Ivanovas <ivanovastadas@gmail.com>
 * @date June 6, 2022
 */

#ifndef __TUNPROXY_SOCKS5_H__
#define __TUNPROXY_SOCKS5_H__

int socks5_init(uint32_t host_addr, uint16_t port);
int socks5_udp_associate(int fd_tcp);
ssize_t socks5_send_udp(void* vdata, size_t length, int fd);

#endif //__TUNPROXY_SOCKS5_H__
