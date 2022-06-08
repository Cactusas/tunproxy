/*
 * @file viface.c
 * @author Tadas Ivanovas <ivanovastadas@gmail.com>
 * @date June 7, 2022
 */

#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <linux/if_packet.h>

#include <fcntl.h>
#include <linux/if_tun.h>
#include <memory.h>
#include <net/if.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/sysmacros.h> //makedev
#include <sys/stat.h>
#include <unistd.h>
#include <netinet/in.h>

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
  int fd = open("/dev/net/tunproxy", O_RDWR);
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

  int fd_rsock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
  if(fd_rsock < 0) {
    perror("Unable to create raw socket");
    return -1;
  }

//  int optval = 1;
//  if (setsockopt(fd_rsock, IPPROTO_IP, IP_HDRINCL, &optval, sizeof(int)) < 0) {
//    perror("setsockopt");
//    return -1;
//  }

  //bind tunproxy
  struct ifreq ifreq;
  snprintf(ifreq.ifr_name, sizeof(ifreq.ifr_name), "tunproxy");
  if (ioctl(fd_rsock, SIOCGIFINDEX, &ifreq)) {
    return -1;
  }

  struct sockaddr_ll saddr;
  memset(&saddr, 0, sizeof(saddr));
  saddr.sll_family = AF_PACKET;
  saddr.sll_protocol = htons(ETH_P_ALL);
  saddr.sll_ifindex = ifreq.ifr_ifindex;
  saddr.sll_pkttype = PACKET_HOST;

  if(bind(fd_rsock, (struct sockaddr *)&saddr, sizeof(saddr)) == -1) {
    return -1;
  }

  //TODO this piece of code repeats a lot of times. Move it to function
  //Get file descriptor flags
  int flags = fcntl(fd_rsock,F_GETFL, 0);
  if (flags < 0)
  {
    perror("fcntl");
    return -1;
  }

  //Make file descriptor non-blocking
  flags |= O_NONBLOCK;
  if (fcntl(fd_rsock, F_SETFL, flags))
  {
    perror("fcntl");
    return -1;
  }

  return fd_rsock;
}
