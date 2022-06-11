/*
 * @file links.c
 * @author Tadas Ivanovas <ivanovastadas@gmail.com>
 * @date June 8, 2022
 */

#include <malloc.h>
#include <memory.h>
#include <stdint.h>

#include "links.h"

struct link_t{
  struct link_ep link;
  struct link_t *next;
};

static struct link_t *link_front = NULL;
static struct link_t *link_back = NULL;

void link_init() {
  link_front = malloc(sizeof(struct link_t));
  link_back = link_front;
}

void link_add(const struct link_ep *l) {
  memcpy(&link_back->link, l, sizeof(struct link_ep));
  link_back->next = malloc(sizeof(struct link_t));
  link_back = link_back->next;
  link_back->next = NULL;
}

int link_exist(const struct link_ep *l) {
  struct link_t *ptr = link_front;
  do {
    if (memcmp(&ptr->link, l, sizeof(struct link_ep)) == 0) {
      return 1;
    }
    ptr = ptr->next;
  } while (ptr != NULL);

  return 0;
}

const struct link_ep* link_find_by_dst(uint32_t dst_addr, uint16_t dst_port) {
  struct link_t *ptr = link_front;
  while (ptr != NULL) {
    if (ptr->link.dst_addr == dst_addr && ptr->link.dst_port == dst_port) {
      return &ptr->link;
    }
    ptr = ptr->next;
  }

  return NULL;
}

int link_remove(const struct link_ep *l) {
  struct link_t *ptr = link_front;
  struct link_t *ptr_prev = NULL;

  while (ptr != NULL) {
    if (memcmp(&ptr->link, l, sizeof(struct link_ep)) == 0) {
      if (ptr_prev != NULL) {
        ptr_prev->next = ptr->next;
      }
      if (ptr == link_front) {
        link_front = ptr->next;
      }
      free(ptr);
      return 0;
    }
    ptr_prev = ptr;
    ptr = ptr->next;
  }

  return 1;
}
