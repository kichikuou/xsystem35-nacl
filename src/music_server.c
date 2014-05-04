/*
 * music_server.c  music server main
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
/* $Id: music_server.c,v 1.13 2004/10/31 04:18:06 chikama Exp $ */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/poll.h>
#include <sys/wait.h>
#include <glib.h>
#include <pthread.h>

#include "portab.h"
#include "music.h"
#include "system.h"
#include "music_server.h"
#include "music_fader.h"
#include "music_bgm.h"
#include "music_ctrlsocket.c"
#include "nact.h"

/* music server 内共通変数 */
struct _musprvdat musprv;
MusicMessage music_msg;

static pthread_t g_musserv_thread;

static int get_command() {
	int v[10];

#ifndef ENABLE_NACL
	if (prv.audiodev.fd != -1) {
		prv.ufds[1].fd = prv.audiodev.fd;
		prv.ufds[1].events = POLLOUT;
		prv.nfds = 2;
	} else {
		prv.nfds = 1;
	}
	
	len = sizeof(saddr);
	
	if (-1 == poll(prv.ufds, prv.nfds, 10)) {
		perror("poll");
		return 0;
	}
	
	if (prv.nfds == 2) {
		if (prv.ufds[1].revents & POLLOUT) {
			muspcm_write2dev();
		}
	}
	
	if ((prv.ufds[0].revents & POLLIN) == 0) {
		//puts("timeout");
		return 0;
	}
	
	if (-1 == (fd = accept(socketfd, (struct sockaddr *)&saddr, &len))) {
		perror("accept");
		return 0;
	}
	
	pkt = g_malloc0(sizeof(PacketNode));
	read(fd, &pkt->hdr, sizeof(ClientPktHeader));
	if (pkt->hdr.data_length) {
		BYTE *p;
		int l;
		p = pkt->data = g_malloc0(pkt->hdr.data_length);
		l = pkt->hdr.data_length;
		while(l) {
			int len;
			len = read(fd, p, pkt->hdr.data_length);
			l -= len;
			p += len;
		}
	}
	pkt->fd = fd;
#else
        pthread_mutex_lock(&music_msg.lock);
        while (music_msg.status != MUSIC_TX && !muspcm_writable())
          pthread_cond_wait(&music_msg.cond, &music_msg.lock);
        pthread_mutex_unlock(&music_msg.lock);
#endif

        if (muspcm_writable())
          muspcm_write2dev();

        if (music_msg.status != MUSIC_TX)
          return 0;

	// fprintf(stderr, "get command %d, len = %d\n", music_msg.command, music_msg.client_data_length);
	switch(music_msg.command) {
	case MUS_CDROM_START:
		v[0] = ((int *)music_msg.client_data)[0];
		v[1] = ((int *)music_msg.client_data)[1];
		muscd_start(v[0], v[1]);
		ctrl_ack_packet();
		break;
		
	case MUS_CDROM_STOP:
		muscd_stop();
		ctrl_ack_packet();
		break;
		
	case MUS_CDROM_GETPOSITION: {
		cd_time tm = muscd_getpos();
		ctrl_write_packet(&tm, sizeof(tm));
		break;
	}
	
	case MUS_CDROM_GETMAXTRACK: {
		int trk = prv.cd_maxtrk;
		ctrl_write_packet(&trk, sizeof(trk));
		break;
	}
	
	case MUS_PCM_START:
		v[0] = ((int *)music_msg.client_data)[0]; // ch
		v[1] = ((int *)music_msg.client_data)[1]; // loop
		muspcm_start(v[0], v[1]);
		ctrl_ack_packet();
		break;
		
	case MUS_PCM_STOP:
		v[0] = ((int *)music_msg.client_data)[0]; /* ch */
		v[1] = ((int *)music_msg.client_data)[1]; /* msec  */
		musfade_new(MIX_PCM, v[0], v[1], -1, 1);
		ctrl_ack_packet();
		break;

	case MUS_PCM_LOAD_NO:
		v[0] = ((int *)music_msg.client_data)[0]; /* ch */
		v[1] = ((int *)music_msg.client_data)[1]; /* no  */
		muspcm_load_no(v[0], v[1]);
		ctrl_ack_packet();
		break;

	case MUS_PCM_LOAD_MEM:
		v[0] = ((int *)music_msg.client_data)[0]; /* ch */
		muspcm_load_mem(v[0], music_msg.client_data);
                music_msg.client_data = NULL; /* take ownership */
		ctrl_ack_packet();
		break;

	case MUS_PCM_LOAD_LRSW:
		v[0] = ((int *)music_msg.client_data)[0]; /* ch  */
		v[1] = ((int *)music_msg.client_data)[1]; /* no */
		muspcm_load_no_lrsw(v[0], v[1]);
		ctrl_ack_packet();
		break;

	case MUS_PCM_UNLOAD:
		v[0] = ((int *)music_msg.client_data)[0]; /* ch */
		muspcm_unload(v[0]);
		ctrl_ack_packet();
		break;
		
	case MUS_PCM_GETPOSITION: {
		int time;
		v[0] = ((int *)music_msg.client_data)[0]; /* ch */
		time = muspcm_getpos(v[0]);
		ctrl_write_gint(time);
		break;
	}
	
	case MUS_PCM_WAITEND:
		v[0] = ((int *)music_msg.client_data)[0]; /* ch */
		if (!muspcm_isplaying(v[0])) {
			ctrl_ack_packet();
			break;
		}
		prv.pcm[v[0]]->cb_atend = musserv_send_ack;
		prv.pcm[v[0]]->fd = 0;
		break;
		
	case MUS_PCM_GETWAVETIME: {
		int time;
		v[0] = ((int *)music_msg.client_data)[0]; /* ch */
		time = muspcm_getwavelen(v[0]);
		ctrl_write_gint(time);
		break;
	}

	case MUS_MIDI_START:
		v[0] = ((int *)music_msg.client_data)[0]; /* no */
		v[1] = ((int *)music_msg.client_data)[1]; /* loop */
		musmidi_start(v[0], v[1]);
		ctrl_ack_packet();
		break;
		
	case MUS_MIDI_STOP:
		musmidi_stop();
		ctrl_ack_packet();
		break;

	case MUS_MIDI_PAUSE:
		musmidi_pause();
		ctrl_ack_packet();
		break;
		
	case MUS_MIDI_UNPAUSE:
		musmidi_unpause();
		ctrl_ack_packet();
		break;
		
	case MUS_MIDI_GETPOSITION: {
		midiplaystate state = musmidi_getpos();
		ctrl_write_packet(&state, sizeof(state));
		break;
		
	}
	case MUS_MIDI_SETFLAG:
		v[0] = ((int *)music_msg.client_data)[0]; /* mode */
		v[1] = ((int *)music_msg.client_data)[1]; /* index */
		v[2] = ((int *)music_msg.client_data)[2]; /* val */
		musmidi_setflag(v[0], v[1], v[2]);
		ctrl_ack_packet();
		break;

	case MUS_MIDI_GETFLAG: {
		int val;
		v[0] = ((int *)music_msg.client_data)[0]; /* mode */
		v[1] = ((int *)music_msg.client_data)[1]; /* index */
		val = musmidi_getflag(v[0], v[1]);
		ctrl_write_gint(val);
		break;
	}
	
	case MUS_FADE_START:
		v[0] = ((int *)music_msg.client_data)[0]; /* device */
		v[1] = ((int *)music_msg.client_data)[1]; /* pcm subdev  */
		v[2] = ((int *)music_msg.client_data)[2]; /* time  */
		v[3] = ((int *)music_msg.client_data)[3]; /* vol  */
		v[4] = ((int *)music_msg.client_data)[4]; /* stop  */
		musfade_new(v[0], v[1], v[2], v[3], v[4]); 
		ctrl_ack_packet();
		break;

	case MUS_FADE_STOP:
		v[0] = ((int *)music_msg.client_data)[0]; /* device */
		v[1] = ((int *)music_msg.client_data)[1]; /* pcm subdev  */
		musfade_stop(v[0], v[1]);
		ctrl_ack_packet();
		break;
		
	case MUS_FADE_GETSTATE: {
		boolean st;
		
		v[0] = ((int *)music_msg.client_data)[0]; /* device */
		v[1] = ((int *)music_msg.client_data)[1]; /* pcm subdev  */
		st = musfade_getstate(v[0], v[1]);
		ctrl_write_gboolean(st);
		break;
	}
	
	case MUS_MIXER_GETLEVEL: {
		int lv;
		v[0] = ((int *)music_msg.client_data)[0]; /* dev */
		
		lv = musfade_getvol(v[0]);
		ctrl_write_gint(lv);
		break;
	}

	case MUS_MIXER_SETVOLVAL: {
		int num = music_msg.client_data_length / sizeof(int);
		musfade_setvolval((int *)music_msg.client_data, num);
		ctrl_ack_packet();
		break;
	}
	
	case MUS_BGM_PLAY:
		v[0] = ((int *)music_msg.client_data)[0]; /* no */
		v[1] = ((int *)music_msg.client_data)[1]; /* time */
		v[2] = ((int *)music_msg.client_data)[2]; /* vol */
		musbgm_play(v[0], v[1], v[2]);
		ctrl_ack_packet();
		break;
		
	case MUS_BGM_STOP:
		v[0] = ((int *)music_msg.client_data)[0]; /* no */
		v[1] = ((int *)music_msg.client_data)[1]; /* time */
		musbgm_stop(v[0], v[1]);
		ctrl_ack_packet();
		break;
		
	case MUS_BGM_STOPALL:
		v[0] = ((int *)music_msg.client_data)[1]; /* time */
		musbgm_stopall(v[0]);
		ctrl_ack_packet();
		break;
		
	case MUS_BGM_FADE:
		v[0] = ((int *)music_msg.client_data)[0]; /* no */
		v[1] = ((int *)music_msg.client_data)[1]; /* time */
		v[2] = ((int *)music_msg.client_data)[2]; /* vol */
		musbgm_fade(v[0], v[1], v[2]);
		ctrl_ack_packet();
		break;
		
	case MUS_BGM_GETPOS: {
		int val;
		v[0] = ((int *)music_msg.client_data)[0]; /* no */
		if (!musbgm_isplaying(v[0])) {
			val = 0;
		} else {
			val = musbgm_getpos(v[0]);
		}
		ctrl_write_gint(val);
		break;
	}

	case MUS_BGM_GETLEN: {
		int val;
		v[0] = ((int *)music_msg.client_data)[0]; /* no */
		if (!musbgm_isplaying(v[0])) {
			val = 0;
		} else {
			val = musbgm_getlen(v[0]);
		}
		ctrl_write_gint(val);
		break;
	}
	
	case MUS_BGM_WAIT:
		v[0] = ((int *)music_msg.client_data)[0]; /* no */
		v[1] = ((int *)music_msg.client_data)[1]; /* time */
		if (!musbgm_isplaying(v[0])) {
			ctrl_ack_packet();
			break;
		}
		
		ctrl_ack_packet();
		break;
		
	case MUS_BGM_WAITPOS:
		v[0] = ((int *)music_msg.client_data)[0]; /* no */
		v[1] = ((int *)music_msg.client_data)[1]; /* timeout */
		ctrl_ack_packet();
		WARNING("NOT IMPLEMENTED\n");
		break;
		
	case MUS_GET_VALIDSUBSYSTEM: {
		ValidSubsystem sub;
		sub.cdrom = prv.cd_valid;
		sub.midi  = prv.midi_valid;
		sub.pcm   = prv.pcm_valid;
		ctrl_write_packet(&sub, sizeof(sub));
		break;
	}
	
	case MUS_EXIT:
		if (prv.cd_valid) muscd_exit();
		if (prv.midi_valid) musmidi_exit();
		if (prv.pcm_valid) musfade_exit();
		if (prv.pcm_valid) muspcm_exit();
		ctrl_ack_packet();
		return -1;
		
	default:
		break;
	}
	
	return 0;
}

static void do_command() {
	if (prv.pcm_valid) musfade_cb();
	if (prv.cd_valid) muscd_cb();
	if (prv.midi_valid) musmidi_cb();
	if (prv.pcm_valid) muspcm_cb();
}

static void do_music_server() {
	int cnt = 0;
	while(0 == get_command()) {
		do_command();
		if (cnt > 100) {
			cnt = 0;
			usleep(10); // avoid packet storm
		}
		cnt++;
	}
}

void musserv_send_ack(int fd) {
	ctrl_ack_packet();
}

static void* music_thread_start(void* arg) {
	muscd_init();
	musmidi_init();
	muspcm_init();
	musfade_init();
	musbgm_init();
	
	do_music_server();
        return NULL;
}

int musserv_init() {
        pthread_mutex_init(&music_msg.lock, NULL);
        pthread_cond_init(&music_msg.cond, NULL);
        music_msg.status = MUSIC_IDLE;
        music_msg.client_data = NULL;
        music_msg.server_data = NULL;
        prv.audiodev.pcm_cond = &music_msg.cond;

        pthread_create(&g_musserv_thread, NULL, music_thread_start, NULL);
	
	return OK;
}

int musserv_exit() {
        pthread_join(g_musserv_thread, NULL);

        pthread_mutex_destroy(&music_msg.lock);
        pthread_cond_destroy(&music_msg.cond);
	
	return OK;
}
