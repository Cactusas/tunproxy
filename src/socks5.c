/*
 * @file socks5.c
 * @author Tadas Ivanovas <ivanovastadas@gmail.com>
 * @date June 6, 2022
 */

#include <arpa/inet.h>
#include <fcntl.h>
#include <memory.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

#include "socks5.h"

static uint32_t _bnd_addr = 0;
static uint16_t _bnd_port = 0;

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
  printf("TCP handshake with %s:%u completed\n", host_addr, port);

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
  printf("SOCKS version/identification method selection completed\n");

  return fd;
}

int socks5_udp_associate(int fd_tcp) {
  uint8_t msg[10] = {0};
  msg[0] = 0x05; //SOCKS version
  msg[1] = 0x03; //CMD UDP ASSOCIATE
  msg[2] = 0x00; //RSV
  msg[3] = 0x01; //ATYP IPv4
  //Other fields can be left empty, it is necessary to send 10 bytes packet
  if (send(fd_tcp, msg, sizeof(msg), 0) < 0) {
    perror("SOCKS5 communication failed");
    return -1;
  }

  uint8_t msg_reply[10] = {0}; //Expected reply of 10 bytes
  if (read(fd_tcp, msg_reply, 10) < 0) {
    perror("Failed to read SOCKS5 server reply");
    return -1;
  }

  //SOCKS version, Reply(0x0=success) and Address Type(0x01=IPv4) check.
  if (msg_reply[0] != 0x05 || msg_reply[1] != 0x00 || msg_reply[3] != 0x01) {
    fprintf(stderr, "Unexpected response from server: VER:%0xX REP:%0xX ATYP:%0xX\n",
            msg_reply[0], msg_reply[1], msg_reply[3]);
    return -1;
  }

  _bnd_addr = *(uint32_t*)(&msg_reply[4]); //Received IPv4 address
  _bnd_port = *(uint16_t*)(&msg_reply[8]); //Received UDP port
  struct in_addr ip_addr = {.s_addr = _bnd_addr};
  printf("UDP association completed %s:%u\n", inet_ntoa(ip_addr), _bnd_port);

  int fd_udp = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd_udp == -1) {
    perror("Could not create SOCKS5 UDP socket");
    return -1;
  }

  //TODO not bind port statically
  struct sockaddr_in fd_udp_addr;
  bzero(&fd_udp_addr, sizeof(fd_udp_addr));
  fd_udp_addr.sin_family = AF_INET;
  fd_udp_addr.sin_addr.s_addr = INADDR_ANY;
  fd_udp_addr.sin_port = htons(12010);
  if (bind(fd_udp, (struct sockaddr*)&fd_udp_addr, sizeof(struct sockaddr_in)) < 0) {
    perror("Unable to bind port to UDP socket");
  }

  //TODO this piece of code repeats a lot of times. Move it to function
  ///Make TCP socket non-blocking for EOF handling purposes
  //Get socket flags
  int flags = fcntl(fd_tcp,F_GETFL, 0);
  if (flags < 0)
  {
    perror("fcntl");
    return -1;
  }

  //Make socket non-blocking
  flags |= O_NONBLOCK;
  if (fcntl(fd_tcp, F_SETFL, flags))
  {
    perror("fcntl");
    return -1;
  }

  return fd_udp;
}
