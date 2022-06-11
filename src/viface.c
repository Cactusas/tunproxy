/*
 * @file viface.c
 * @author Tadas Ivanovas <ivanovastadas@gmail.com>
 * @date June 7, 2022
 */

#include <fcntl.h>
#include <linux/if_tun.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <memory.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/sysmacros.h> //makedev
#include <sys/stat.h>
#include <unistd.h>
#include <net/if.h>
#include <netinet/in.h>
#include <stdlib.h>

#include "links.h"
#include "utils.h"
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

  if (util_sock_add_nonblock(fd) < 0) {
    return -1;
  }

  if (util_cmd("ip addr add 10.0.0.2/0 dev tunproxy;"
               "ip link set dev tunproxy up") < 0) {
    return -1;
  }

  return fd;
}

size_t viface_send(int fd, uint32_t dst_addr, uint16_t dst_port, char *data, size_t n) {
  char *packet = malloc(sizeof(struct iphdr) + sizeof(struct udphdr) + n - 10);
  struct iphdr *ip_hdr = (struct iphdr *) packet;
  struct udphdr *udp_hdr = (struct udphdr *) &packet[sizeof(struct iphdr)];

  const struct link_ep *link = link_find_by_dst(dst_addr, dst_port);

  uint16_t len = sizeof(struct iphdr) + sizeof(struct udphdr) + n - 10;

  ip_hdr->ihl = 5;
  ip_hdr->version = 4;
  ip_hdr->tos = 0;
  ip_hdr->tot_len = htons(len);
  ip_hdr->id = htons(0);
  ip_hdr->frag_off = 0;
  ip_hdr->ttl = 64;
  ip_hdr->protocol = IPPROTO_UDP;
  ip_hdr->saddr = link->dst_addr;
  ip_hdr->daddr = link->src_addr;
  ip_hdr->check = util_ip_checksum(&ip_hdr, sizeof(struct iphdr));

  // Ini->ialize the UDP header

//  udp_hdr = (UDP_HDR *)&buf[sizeof(IPV4_HDR)];
  udp_hdr->source = link->dst_port;
  udp_hdr->dest = link->src_port;
  udp_hdr->check = 0;
//  udp_hdr->len = n-10+(sizeof(struct udphdr)); //16128
  udp_hdr->len = htons(n - 10 + (sizeof(struct udphdr)));

  struct sockaddr_in connection;
  memset(&connection, 0, sizeof(connection));
  connection.sin_family = AF_INET;
  connection.sin_addr.s_addr = link->src_addr;

  link_remove(link);
  memcpy(&packet[sizeof(struct iphdr) + sizeof(struct udphdr)], &data[10], n - 10);

  ssize_t sent = sendto(fd, packet, len, 0, (struct sockaddr *)&connection, sizeof(struct sockaddr));
  if (sent < 0) {
    perror("Send failed");
    free(packet);
    return 1;
  }
  printf("VIFACE SENT: %ld\n", sent);
  free(packet);
  return sent;
}
