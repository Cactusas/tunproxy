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

static struct link_t *link_head = NULL;
static struct link_t *link_tail = NULL;

void link_init() {
  link_head = malloc(sizeof(struct link_t));
  link_tail = link_head;
}

void link_add(const struct link_ep *l) {
  memcpy(&link_tail->link, l, sizeof(struct link_ep));
  link_tail->next = malloc(sizeof(struct link_t));
  link_tail = link_tail->next;
  link_tail->next = NULL;
}

int link_exist(const struct link_ep *l) {
  struct link_t *ptr = link_head;
  do {
    if (memcmp(&ptr->link, l, sizeof(struct link_ep)) == 0) {
      return 1;
    }
    ptr = ptr->next;
  } while (ptr != NULL);

  return 0;
}

const struct link_ep* link_find_by_dst(uint32_t dst_addr, uint16_t dst_port) {
  struct link_t *ptr = link_head;
  do {
    if (ptr->link.dst_addr == dst_addr && ptr->link.dst_port == dst_port) {
      return &ptr->link;
    }
    ptr = ptr->next;
  } while (ptr != NULL);

  return NULL;
}

int link_remove(const struct link_ep *l) {
  struct link_t *ptr = link_head;
  struct link_t *ptr_prev = NULL;
  do {
    if (memcmp(&ptr->link, l, sizeof(struct link_ep)) == 0) {
      if (ptr_prev != NULL) {
        ptr_prev->next = ptr->next;
      }
      free(ptr);
      return 0;
    }
    ptr_prev = ptr;
    ptr = ptr->next;
  } while (ptr != NULL);

  return 1;
}
