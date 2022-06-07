/*
 * @file socks5.c
 * @author Tadas Ivanovas <ivanovastadas@gmail.com>
 * @date June 6, 2022
 */

#include <arpa/inet.h>
#include <memory.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

#include "socks5.h"

int socks5_init(const char* host_addr, uint16_t port) {
  int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (fd < 0) {
    perror("Could not create SOCKS5 TCP socket");
    return -1;
  }

  struct sockaddr_in host;
  bzero(&host, sizeof(struct sockaddr_in));
  host.sin_addr.s_addr = inet_addr(host_addr);
  host.sin_port = htons(port);
  host.sin_family = AF_INET;

  if (connect(fd, (struct sockaddr*)&host, sizeof(host)) < 0) {
    fprintf(stderr, "Connection to %s:%u failed: ", host_addr, port);
    perror("");
    return -1;
  }

  ///Version identification/method selection
  uint8_t msg[3];
  msg[0] = 0x05; //SOCKS version
  msg[1] = 0x01; //Number of authentication methods
  msg[2] = 0x05; //No Authentication
  if (send(fd, msg, sizeof(msg), 0) < 0) {
    perror("SOCKS5 communication failed");
    return -1;
  }

  uint8_t msg_reply[2] = {0}; //Expected 2 bytes response from server
  if (read(fd, msg_reply, 2) < 0) {
    perror("Failed to read SOCKS5 server reply");
    return -1;
  }

  if (msg_reply[0] != 0x05 || msg_reply[1] != 0x00) { //SOCKS version and authentication method should match
    fprintf(stderr, "Unexpected response from server: %0xX %0xX\n", msg_reply[0], msg_reply[1]);
    return -1;
  }

  return fd;
}
