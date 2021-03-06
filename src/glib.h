/*
 * glib.h  Minimal GLib replacement for xsystem35-nacl
 *
 * Copyright (C) 2014-2016 <KichikuouChrome@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
*/

#ifndef __GLIB_H_
#define __GLIB_H_

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

typedef int gint;
typedef float gfloat;
typedef int gboolean;
typedef char gchar;
typedef int16_t gint16;
typedef int32_t gint32;
typedef int64_t gint64;
typedef uint16_t guint16;
typedef uint32_t guint32;
typedef uint64_t guint64;
typedef void* gpointer;

#ifndef FALSE
#define FALSE           0
#undef  TRUE
#define TRUE            (!FALSE)
#endif

#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))

#define g_malloc malloc
#define g_free free

#define g_malloc0(n) calloc(1, n);
#define g_new(t, n) malloc(sizeof(t) * (n))
#define g_new0(t, n) calloc((n), sizeof(t))

typedef struct GSList_t {
  void *data;
  struct GSList_t *next;
} GSList;

#define g_slist_next(lst) (lst)->next
typedef void (*GFunc)(gpointer data, gpointer user_data);
void g_slist_foreach(GSList *list, GFunc func, gpointer user_data);
void g_slist_free(GSList *list);
GSList* g_slist_append(GSList *list, void *data);
GSList* g_slist_remove(GSList *list, const void *data);
int g_slist_index(GSList *list, const void *data);
unsigned g_slist_length(GSList *list);

#define GList GSList

#define g_list_next g_slist_next
#define g_list_append g_slist_append
#define g_list_remove g_slist_remove
#define g_list_index g_slist_index
#define g_list_length g_slist_length

#endif // __GLIB_H_
