/*
 * @file main.c
 * @author Tadas Ivanovas <ivanovastadas@gmail.com>
 * @date June 6, 2022
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "socks5.h"

int main( int argc, char *argv[]) {
  char *host = argv[1];
  uint16_t port = atoi(argv[2]);

  int fd_socks5_tcp = socks5_init(host, port);

  return 0;
}
