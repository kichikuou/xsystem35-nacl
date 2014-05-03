#include <glib.h>

void g_slist_foreach(GSList *list, GFunc func, gpointer user_data) {
  for (; list; list = list->next)
    func(list->data, user_data);
}

void g_slist_free(GSList *list) {
  while (list) {
    GSList* next = list->next;
    free(list);
    list = next;
  }
}

GSList* g_slist_append(GSList *list, void *data) {
  GSList* node = malloc(sizeof(GSList));
  node->data = data;
  node->next = NULL;

  if (!list)
    return node;

  GSList* last = list;
  while (list->next)
    last = list->next;
  last->next = node;
  return list;
}

GSList* g_slist_remove(GSList *list, const void *data) {
  if (!list)
    return NULL;

  GSList* node = list;
  if (list->data == data) {
    list = list->next;
    free(node);
    return list;
  }

  for (; node->next; node++) {
    if (node->next->data == data) {
      GSList* nodeToRemove = node->next;
      node->next = nodeToRemove->next;
      free(nodeToRemove);
      break;
    }
  }
  return list;
}

int g_slist_index(GSList *list, const void *data) {
  int i;
  for (i = 0; list; list = list->next, i++) {
    if (list->data == data)
      return i;
  }
  return -1;
}

unsigned g_slist_length(GSList *list) {
  unsigned len;
  for (len = 0; list; list = list->next)
    len++;
  return len;
}
