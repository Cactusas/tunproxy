/*
 * @file utils.h
 * @author Tadas Ivanovas <ivanovastadas@gmail.com>
 * @date June 8, 2022
 */

#ifndef __TUNPROXY_UTILS_H__
#define __TUNPROXY_UTILS_H__

uint16_t util_ip_checksum(void* vdata, size_t length);

#endif //__TUNPROXY_UTILS_H__