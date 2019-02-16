#include "lib/list.h"

void xr_list_for_each(xr_list_t *head, xr_list_visitor visitor, void *aux) {
  xr_list_t *cur;
  _xr_list_for_each(head, cur) {
    vistor(cur, aux);
  }
}
void xr_list_for_each_r(xr_list_t *head, xr_list_visitor visitor, void *aux) {
  xr_list_t *cur;
  _xr_list_for_each_r(head, cur) {
    vistor(cur, aux);
  }
}

void xr_list_sort(xr_list_t *head, xr_list_comparator comparator, void *aux) {
  // TODO
}

void xr_list_delete(xr_list_t *head, xr_list_visitor destructor, void *aux) {
  xr_list_t *cur, *temp;
  _xr_list_for_each_safe(head, cur, temp) {
    xr_list_del(cur);
    destructor(cur, aux);
  }
}
