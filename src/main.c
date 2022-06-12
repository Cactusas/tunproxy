/*
 * @file main.c
 * @author Tadas Ivanovas <ivanovastadas@gmail.com>
 * @date June 6, 2022
 */

#include <arpa/inet.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <libnet.h>

#include "links.h"
#include "log.h"
#include "rsock.h"
#include "socks5.h"
#include "utils.h"
#include "viface.h"

#define MAX_EPOLL_EVENTS 64
#define BUFFLEN 4*1024

void print_banner();
void sighandler(int signum);
int parse_args(int argc, char *argv[]);

volatile int end = 0;
uint32_t host = 0;
uint16_t port = 0;
char *log_file = NULL;

int main(int argc, char *argv[]) {
  if (parse_args(argc, argv)) {
    return 1;
  }

  signal(SIGINT, sighandler);
  signal(SIGTERM, sighandler);
  signal(SIGKILL, sighandler);

  print_banner();
  link_init();

  if (log_file && log_init(log_file)) {
    fprintf(stderr, "Cannot initialize log file\n");
    return 1;
  }

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

  int fd_rsock = rsock_init();
  if (fd_rsock < 0) {
    fprintf(stderr, "Failed to create raw socket\n");
    return 1;
  }

  util_cmd("iptables -t mangle -A OUTPUT -p udp ! --sport 12010 -j MARK --set-mark 2;"
           "ip rule add fwmark 2 lookup 100;"
           "ip route add default dev tunproxy table 100;"
           "ip route flush cache");


  struct epoll_event events[MAX_EPOLL_EVENTS];
  int fd_epoll = epoll_create(MAX_EPOLL_EVENTS);
  struct epoll_event ev_socks5_tcp, ev_viface, ev_socks5_udp;

  bzero(&ev_socks5_tcp, sizeof(ev_socks5_tcp));
  ev_socks5_tcp.events = EPOLLET | EPOLLRDHUP; //Handle only EOF event
  ev_socks5_tcp.data.fd = fd_socks5_tcp;
  epoll_ctl(fd_epoll, EPOLL_CTL_ADD, fd_socks5_tcp, &ev_socks5_tcp);

  bzero(&ev_viface, sizeof(ev_viface));
  ev_viface.events = EPOLLET | EPOLLIN;
  ev_viface.data.fd = fd_viface;
  epoll_ctl(fd_epoll, EPOLL_CTL_ADD, fd_viface, &ev_viface);

  bzero(&ev_socks5_udp, sizeof(ev_socks5_udp));
  ev_socks5_udp.events = EPOLLET | EPOLLIN;
  ev_socks5_udp.data.fd = fd_socks5_udp;
  epoll_ctl(fd_epoll, EPOLL_CTL_ADD, fd_socks5_udp, &ev_socks5_udp);

  uint8_t buff[BUFFLEN];

  while (!end) {
    int num_ready = epoll_wait(fd_epoll, events, MAX_EPOLL_EVENTS, 1000);
    for (int i = 0; i < num_ready; i++) {
      if(events[i].events & EPOLLRDHUP) {
        printf("Remote host terminated connection\n");
        return 0;
      } else if (events[i].events & EPOLLIN) {
        if (events[i].data.fd == fd_viface) {
          ssize_t n = read(fd_viface, buff, BUFFLEN);
          if (util_is_udp(buff)) {
            printf("Packet received VIFACE: %lu\n", n);
            log_write(buff, n);
            socks5_send_udp(buff, n, fd_socks5_udp);
          }
        } else if (events[i].data.fd == fd_socks5_udp) {
          size_t n = read(fd_socks5_udp, buff, BUFFLEN);
          uint32_t iprec = *(uint32_t*)(&buff[4]);
          uint16_t portrec = *(uint32_t*)&buff[8];
          viface_send(fd_rsock, iprec, portrec, buff, n);
          printf("Packet received SOCKS5: %lu\n", n);
        }
      }
    }
  }

  return 0;
}

void print_usage() {
  printf("Usage: tunproxy [HOST:PORT] OPTIONS\n");
}

void print_version() {
#if defined(SOFTWARE_MAJOR) && defined(SOFTWARE_MINOR) && defined(SOFTWARE_BUILD)
  printf("v%d.%d.%d\n\n", SOFTWARE_MAJOR, SOFTWARE_MINOR, SOFTWARE_BUILD);
#endif
}

void print_banner() {
  printf(" _                                         \n");
  printf("| |_ _   _ _ __  _ __  _ __ _____  ___   _ \n");
  printf("| __| | | | '_ \\| '_ \\| '__/ _ \\ \\/ / | | |\n");
  printf("| |_| |_| | | | | |_) | | | (_) >  <| |_| |\n");
  printf(" \\__|\\__,_|_| |_| .__/|_|  \\___/_/\\_\\\\__, |\n");
  printf("                |_|                  |___/ \n");
  print_version();
}

void sighandler(int signum) {
  util_cmd("iptables -t mangle -D OUTPUT -p udp ! --sport 12010 -j MARK --set-mark 2;"
           "ip rule del fwmark 2 lookup 100;");
  end = 1;
}

int parse_args(int argc, char *argv[]) {
  if (argc < 2) {
    return 1;
  }
  char *token = strtok(argv[1], ":");

  ///IP address validation
  struct sockaddr_in sa;
  if (inet_pton(AF_INET, token, &(sa.sin_addr)) == 0) {
    fprintf(stderr, "Invalid IP address\n");
    return 1;
  }
  host = inet_addr(token);

  ///Argument format detection [IP:port] or [IP port]
  char *_port;
  if ((token = strtok(NULL, ":")) == NULL && argc >= 3) { //format is [IP port]
    _port = argv[2];
  } else { //format is [IP:port]
    _port = token;
  }

  ///Port validation
  port = strtoul(_port, NULL, 10);
  if (port == 0) { //strtoul() returns 0 if failed, port number cannot be 0 anyway
    fprintf(stderr, "Invalid port\n");
    return 1;
  }

  int opt = 0;
  while((opt = getopt(argc, argv, ":l:v")) != -1)
  {
    switch(opt) {
      case 'l':
        log_file = optarg;
        break;
      case ':':
        printf("Missing argument for -%c\n", optopt);
        return 1;
      default:
        break;
    }
  }

  return 0;
}
