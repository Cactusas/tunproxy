/*
 * @file log.h
 * @author Tadas Ivanovas <ivanovastadas@gmail.com>
 * @date June 12, 2022
 */

#ifndef __TUNPROXY_LOG_H__
#define __TUNPROXY_LOG_H__

int log_init(const char *filename);
void log_write(void *vdata, ssize_t n);

#endif //__TUNPROXY_LOG_H__