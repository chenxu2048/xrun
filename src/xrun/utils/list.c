#include "xrun/utils/list.h"

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
