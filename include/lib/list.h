#ifndef XR_LIST_H
#define XR_LIST_H

#include <stdbool.h>

struct xr_list_s;

typedef struct xr_list_s xr_list_t;

struct xr_list_s {
  xr_list_t *prev;
  xr_list_t *next;
};

struct xr_list_head_s {
  xr_list_t list;
};

typedef struct xr_list_head_s xr_list_head_t;

#define xr_list_entry(list, type, member) \
  ((type *)((long)(list)-offsetof(type, member)))

static inline bool xr_list_empty(xr_list_t *list) {
  return list->prev == list->next;
}

static inline void xr_list_add(xr_list_t *prev, xr_list_t *elem) {
  elem->prev = prev;
  elem->next = prev->next;
  prev->next = elem;
}

static inline void xr_list_del(xr_list_t *cur) {
  cur->prev->next = cur->next;
  cur->next->prev = cur->prev;
}

static inline void xr_list_init(xr_list_t *head) {
  head->next = head->prev = head;
}

#define _xr_list_for_each(head, cur) \
  for (cur = (head)->next; cur != (head); cur = cur->next)

#define _xr_list_for_each_entry(head, entry, type, member) \
  for (entry = xr_list_entry((head)->next, type, member);  \
       entry->member.next != (head);                       \
       entry = xr_list_entry(entry->member.next, type, member))

#define _xr_list_for_each_r(head, cur) \
  for (cur = (head)->prev; cur != (head); cur = cur->prev)

#define _xr_list_for_each_entry_r(head, entry, type, member) \
  for (entry = xr_list_entry((head)->prev, type, member);    \
       entry->member.prev != (head);                         \
       entry = xr_list_entry(entry->member.prev, type, member))

#define _xr_list_for_each_safe(head, cur, temp)             \
  for (cur = (head)->next, temp = cur->next; cur != (head); \
       cur = temp, temp = cur->next)

typedef void xr_list_visitor(xr_list_t *list, void *aux);
typedef void xr_list_comparator(xr_list_t *left, xr_list_t *right, void *aux);

void xr_list_for_each(xr_list_t *head, xr_list_visitor visitor, void *aux);
void xr_list_for_each_r(xr_list_t *head, xr_list_visitor visitor, void *aux);
void xr_list_sort(xr_list_t *head, xr_list_comparator comparator, void *aux);

#endif
