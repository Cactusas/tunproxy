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

#include "links.h"
#include "socks5.h"
#include "utils.h"

static uint32_t _bnd_addr = 0;
static uint16_t _bnd_port = 0;

int socks5_init(uint32_t host_addr, uint16_t port) {
  int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (fd < 0) {
    perror("Could not create SOCKS5 TCP socket");
    return -1;
  }

  struct sockaddr_in host;
  bzero(&host, sizeof(struct sockaddr_in));
//  host.sin_addr.s_addr = inet_addr(host_addr);
  host.sin_addr.s_addr = host_addr;
  host.sin_port = htons(port);
  host.sin_family = AF_INET;

  if (connect(fd, (struct sockaddr*)&host, sizeof(host)) < 0) {
    //fprintf(stderr, "Connection to %s:%u failed: ", host_addr, port);
    perror("");
    return -1;
  }
  //printf("TCP handshake with %s:%u completed\n", host_addr, port);

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

  if (util_sock_add_nonblock(fd_tcp) < 0) {
    return -1;
  }

  if (util_sock_add_nonblock(fd_udp) < 0) {
    return -1;
  }

  return fd_udp;
}

ssize_t socks5_send_udp(void* vdata, size_t length, int fd) {
  uint8_t *data = (uint8_t*)vdata;

  struct link_ep link;
  util_iptolink(vdata, &link);
  link_add(&link);

  uint8_t buff[4*1024];

  buff[0] = 0x0; //RSV
  buff[1] = 0x0; //RSV
  buff[2] = 0x0; //FRAG
  buff[3] = 0x01; //ATYP IPv4
  memcpy(&buff[4], &link.dst_addr, sizeof(link.dst_addr)); //DST.ADDR
  memcpy(&buff[8], &link.dst_port, sizeof(link.dst_port)); //DST.PORT
  memcpy(&buff[10], &data[28], length-28); //USER DATA

  struct sockaddr_in send_to;
  bzero(&send_to, sizeof(send_to));
  send_to.sin_addr.s_addr = _bnd_addr;
  send_to.sin_port = _bnd_port;
  send_to.sin_family = AF_INET;

  ssize_t ret = sendto(fd, buff, length-18, 0, (struct sockaddr*)&send_to, sizeof(send_to));
  printf("SOCKS5 SENT: %ld\n", ret);
  return ret;
}
