/*
 * @file viface.c
 * @author Tadas Ivanovas <ivanovastadas@gmail.com>
 * @date June 7, 2022
 */

#include <fcntl.h>
#include <linux/if_tun.h>
#include <memory.h>
#include <net/if.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/sysmacros.h> //makedev
#include <sys/stat.h>
#include <unistd.h>

#include "viface.h"

int viface_init() {
  struct stat st = {0};

  if (stat("/dev/net", &st) == -1) {
    mkdir("/dev/net", 0700);
  }

  if (mknod("/dev/net/tunproxy", S_IFCHR | S_IRUSR, makedev(10, 200)) < 0) {
    perror("mknod");
  }

  //Open TUN/TAP device
  int fd = open("/dev/net/tunproxy", O_RDONLY);
  if (fd < 0) {
    perror("open");
    return -1;
  }

  struct ifreq ifr;
  bzero(&ifr, sizeof(ifr));
  ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
  strncpy(ifr.ifr_name, "tunproxy", IFNAMSIZ-1);

  // Register a network device with the kernel
  if (ioctl(fd, TUNSETIFF, &ifr) != 0) {
    perror("ioctl");
    if (close(fd) < 0) {
      perror("close");
    }
    return -1;
  }

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
