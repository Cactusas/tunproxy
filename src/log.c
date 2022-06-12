/*
 * @file log.c
 * @author Tadas Ivanovas <ivanovastadas@gmail.com>
 * @date June 12, 2022
 */

#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include "log.h"

#define BUFFLEN 4*1024

FILE *fptr = NULL;

const char HEX[] = {
'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
'a', 'b', 'c', 'd', 'e', 'f',
};

static void hex(const uint8_t *source, char *dest, ssize_t count)
{
  for (ssize_t i = 0; i < count; ++i) {
    unsigned char data = source[i];
    dest[2 * i] = HEX[data >> 4];
    dest[2 * i + 1] = HEX[data & 15];
  }
  dest[2 * count] = '\0';
}

int log_init(const char *filename) {
  fptr = fopen(filename,"w");
  if(fptr == NULL) {
    perror("log file");
    return 1;
  }
  return 0;
}

void log_write(void *vdata, ssize_t n) {
  uint8_t *data = (uint8_t*)vdata;
  uint16_t sport = data[20] << 8 | data[21];
  uint16_t dport = data[22] << 8 | data[23];

  char hex_data[2*BUFFLEN + 1];
  hex(&data[28], hex_data, n-28); //IP header + UDP header = 28 bytes

  struct timespec now;
  clock_gettime(CLOCK_REALTIME, &now);
  char time_data[100];
  strftime(time_data, 100, "%b %e %X", localtime(&now.tv_sec));

  fprintf(fptr, "%s src=%u.%u.%u.%u:%u dst=%u.%u.%u.%u:%u data:%s\n", time_data,
          (uint8_t)data[12], (uint8_t)data[13], (uint8_t)data[14], (uint8_t)data[15], sport,
          (uint8_t)data[16], (uint8_t)data[17], (uint8_t)data[18], (uint8_t)data[19], dport,
          hex_data);
  fflush(fptr);
}
