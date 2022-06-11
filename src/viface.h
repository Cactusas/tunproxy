/*
 * @file viface.h
 * @author Tadas Ivanovas <ivanovastadas@gmail.com>
 * @date June 7, 2022
 */

#ifndef __TUNPROXY_VIFACE_H__
#define __TUNPROXY_VIFACE_H__

int viface_init();
size_t viface_send(int fd, uint32_t dst_addr, uint16_t dst_port, char *data, size_t n);

#endif //__TUNPROXY_VIFACE_H__
