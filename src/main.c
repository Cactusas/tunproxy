/*
 * @file main.c
 * @author Tadas Ivanovas <ivanovastadas@gmail.com>
 * @date June 6, 2022
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/epoll.h>

#include "socks5.h"
#include "viface.h"

#define MAX_EPOLL_EVENTS 64

int main( int argc, char *argv[]) {
  char *host = argv[1];
  uint16_t port = atoi(argv[2]);

  int fd_socks5_tcp = socks5_init(host, port);
  if (fd_socks5_tcp < 0) {
    fprintf(stderr, "Failed to connect to SOCKS5 server\n");
    return 1;
  }

  int fd_socks5_udp = socks5_udp_associate(fd_socks5_tcp);
  if (fd_socks5_udp < 0) {
    fprintf(stderr, "SOCKS5 UDP association failed\n");
    return 1;
  }

  int fd_viface = viface_init();
  if (fd_viface < 0) {
    fprintf(stderr, "Failed to create virtual interface\n");
    return 1;
  }

  struct epoll_event events[MAX_EPOLL_EVENTS];
  int fd_epoll = epoll_create(MAX_EPOLL_EVENTS);
  struct epoll_event ev_socks5_tcp, ev_viface;
  ev_socks5_tcp.events = EPOLLET | EPOLLRDHUP; //Handle only EOF event
  ev_socks5_tcp.data.fd = fd_socks5_tcp;
  epoll_ctl(fd_epoll, EPOLL_CTL_ADD, fd_socks5_tcp, &ev_socks5_tcp);

  ev_viface.events = EPOLLET | EPOLLIN;
  ev_viface.data.fd = fd_viface;
  epoll_ctl(fd_epoll, EPOLL_CTL_ADD, fd_viface, &ev_viface);

  //TODO this is very bad, move to popen or find workaround
  system("ip addr add 10.0.0.2/0 dev tunproxy");
  system("ip link set dev tunproxy up");
  system("ip route add default dev tunproxy table 100");
  system("ip route flush cache");

  while (1) {
    int num_ready = epoll_wait(fd_epoll, events, MAX_EPOLL_EVENTS, 1000);
    for (int i = 0; i < num_ready; i++) {
      if(events[i].events & EPOLLRDHUP) {
        printf("Remote host terminated connection\n");
        return 0;
      } else if (events[i].events & EPOLLIN) {
        printf("Packet received\n");
      }
    }
  }

  return 0;
}
