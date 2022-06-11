/*
 * @file rsock.c
 * @author Tadas Ivanovas <ivanovastadas@gmail.com>
 * @date June 8, 2022
 */

#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>

#include "rsock.h"

int rsock_init() {
  int fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
  if(fd < 0) {
    perror("Unable to create raw socket");
    return -1;
  }

  int optval = 1;
  if (setsockopt(fd, IPPROTO_IP, IP_HDRINCL, &optval, sizeof(int)) < 0) {
    perror("setsockopt");
    return -1;
  }

  return fd;
}
