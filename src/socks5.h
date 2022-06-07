/*
 * @file socks5.h
 * @author Tadas Ivanovas <ivanovastadas@gmail.com>
 * @date June 6, 2022
 */

#ifndef __TUNPROXY_SOCKS5_H__
#define __TUNPROXY_SOCKS5_H__

int socks5_init(const char* host_addr, uint16_t port);

#endif //__TUNPROXY_SOCKS5_H__