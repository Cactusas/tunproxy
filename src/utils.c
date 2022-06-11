/*
 * @file utils.c
 * @author Tadas Ivanovas <ivanovastadas@gmail.com>
 * @date June 8, 2022
 */

#include <memory.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <libnet.h>

#include "links.h"
#include "utils.h"

uint16_t util_ip_checksum(void* vdata, size_t length) {
  // Cast the data pointer to one that can be indexed.
  char* data=(char*)vdata;

  // Initialise the accumulator.
  uint64_t acc=0xffff;

  // Handle any partial block at the start of the data.
  unsigned int offset=((uintptr_t)data)&3;
  if (offset) {
    size_t count=4-offset;
    if (count>length) count=length;
    uint32_t word=0;
    memcpy(offset+(char*)&word,data,count);
    acc+=ntohl(word);
    data+=count;
    length-=count;
  }

  // Handle any complete 32-bit blocks.
  char* data_end=data+(length&~3);
  while (data!=data_end) {
    uint32_t word;
    memcpy(&word,data,4);
    acc+=ntohl(word);
    data+=4;
  }
  length&=3;

  // Handle any partial block at the end of the data.
  if (length) {
    uint32_t word=0;
    memcpy(&word,data,length);
    acc+=ntohl(word);
  }

  // Handle deferred carries.
  acc=(acc&0xffffffff)+(acc>>32);
  while (acc>>16) {
    acc=(acc&0xffff)+(acc>>16);
  }

  // If the data began at an odd byte address
  // then reverse the byte order to compensate.
  if (offset&1) {
    acc=((acc&0xff00)>>8)|((acc&0x00ff)<<8);
  }

  // Return the checksum in network byte order.
  return htons(~acc);
}

void util_iptolink(void *vdata, struct link_ep *link) {
  uint8_t *data = (uint8_t*)vdata;

  memcpy(&link->src_addr, &data[12], 4);
  memcpy(&link->dst_addr, &data[16], 4);
  memcpy(&link->src_port, &data[20], 2);
  memcpy(&link->dst_port, &data[22], 2);
//  memcpy(link, &data[12], 12);
}

int util_is_udp(const char* buffer) {
  unsigned char version = ((unsigned char) buffer[0]) >> 4;
  if (version == 4) {
    if ((unsigned char)buffer[9] == IPPROTO_UDP) {
      return 1;
    }
  }
  return 0;
}

int util_sock_add_nonblock(int sfd) {
  int flags = fcntl(sfd, F_GETFL, 0);
  if (flags == -1) {
    perror ("fcntl");
    return -1;
  }

  flags |= O_NONBLOCK;
  if (fcntl (sfd, F_SETFL, flags) == -1) {
    perror ("fcntl");
    return -1;
  }

  return 0;
}

int util_cmd(const char *cmd) {
  int ch;

  FILE *p = popen(cmd,"r");
  if( p == NULL){
    perror("Unable to open process");
    return -1;
  }
  while((ch=fgetc(p)) != EOF) {
    putchar(ch);
  }
  pclose(p);

  return 0;
}
