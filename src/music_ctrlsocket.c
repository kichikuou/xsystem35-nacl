/*
 * music_ctlsocket.c  music server socket control funciton
 *
 * Copyright (C) 1997-1998 Masaki Chikama (Wren) <chikama@kasumi.ipl.mech.nagoya-u.ac.jp>
 *               1998-                           <masaki-c@is.aist-nara.ac.jp>
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
/* $Id: music_ctrlsocket.c,v 1.1 2002/08/18 09:35:29 chikama Exp $ */

#include <stdio.h>
#include <glib.h>
#include <pthread.h>

static void ctrl_write_packet(gpointer data, gint length) {
  if (music_msg.client_data) {
    g_free(music_msg.client_data);
    music_msg.client_data = NULL;
  }

  if (music_msg.status != MUSIC_TX)
    fprintf(stderr, "ctrl_write_packet: wrong msg status %d\n", music_msg.status);
  if (music_msg.server_data)
    fprintf(stderr, "cl_write_packet: server_data is not null\n");

  pthread_mutex_lock(&music_msg.lock);
  music_msg.status = MUSIC_RX;
  music_msg.server_data_length = length;
  if (data && length > 0) {
    music_msg.server_data = g_malloc(length);
    memcpy(music_msg.server_data, data, length);
  }
  pthread_mutex_unlock(&music_msg.lock);
  pthread_cond_signal(&music_msg.cond);
}

static void ctrl_write_gint(gint val) {
	ctrl_write_packet(&val, sizeof (gint));
}

static void ctrl_write_gfloat(gfloat val) {
	ctrl_write_packet(&val, sizeof (gfloat));
}

static void ctrl_write_gboolean(gboolean bool) {
	ctrl_write_packet(&bool, sizeof (gboolean));
}

static void ctrl_write_string(gchar * string) {
	ctrl_write_packet(string, string ? strlen(string) + 1 : 0);
}

static void ctrl_ack_packet(void) {
	ctrl_write_packet(NULL, 0);
}
