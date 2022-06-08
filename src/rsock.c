/*
 * @file rsock.c
 * @author Tadas Ivanovas <ivanovastadas@gmail.com>
 * @date June 8, 2022
 */

#include <fcntl.h>
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
  }

  //TODO this piece of code repeats a lot of times. Move it to function
  //Get file descriptor flags
  int flags = fcntl(fd,F_GETFL, 0);
  if (flags < 0)
  {
    perror("fcntl");
    return -1;
  }

  //Make file descriptor non-blocking
  flags |= O_NONBLOCK;
  if (fcntl(fd, F_SETFL, flags))
  {
    perror("fcntl");
    return -1;
  }

  return fd;
}
