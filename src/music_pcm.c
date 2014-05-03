/*
 * music_pcm.c  music server PCM part
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
/* $Id: music_pcm.c,v 1.16 2003/11/09 15:06:13 chikama Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "portab.h"
#include "system.h"
#include "music_pcm.h"
#include "music_server.h"
#include "musstream.h"
#include "audio.h"
#include "ald_manager.h"
#include "wavfile.h"
#include "counter.h"
#include "pcmlib.h"

// ����� cb �� audio buffer �β�ʬ�Σ���������뤫
#define SLICE 4

#define IS_LOADED(slot) (prv.pcm[(slot)])


static int pcmmix_mix(short *src, int len);
static int pcmmix_send2devbuf();

extern void Ima_initImaTable(void);


#include "music_pcm_wai.c"

int muspcm_init() {
	int st = audio_init(&prv.audiodev);
	chanfmt_t fmt;
	
	if (st == NG) {
		prv.pcm_valid = FALSE;
		return NG;
	}
	
	fmt.rate = 44100; fmt.bit = 16; fmt.ch = 2;
	if (OK == prv.audiodev.open(&prv.audiodev, fmt)) {
		prv.ofmt = fmt;
	} else if (fmt.rate = 48000,
		   OK == prv.audiodev.open(&prv.audiodev, fmt)) {
		prv.ofmt = fmt;
	} else {
		prv.pcm_valid = FALSE;
		return NG;
	}
	
	{
		// device buffer ������
		audiodevbuf_t *buf = &prv.audiodev.buf;
		
		buf->b[0] = g_malloc0(buf->len);
		buf->b[1] = g_malloc0(buf->len);
		
		buf->cur = buf->b[0];
		buf->end = (char *)(buf->cur + buf->len);
		buf->sw = 0;
		buf->lenmax = 0;
		buf->ready[0] = buf->ready[1] = FALSE;
	}
	
	prv.pcm_valid = TRUE;
	
	Ima_initImaTable();
	muspcm_load_wai();
	
	return OK;
}

int muspcm_exit() {
	prv.audiodev.close(&prv.audiodev);
	prv.audiodev.exit(&prv.audiodev);
	return OK;
}

// �ֹ�����PCM�ե������ɤ߹���
int muspcm_load_no(int slot, int no) {
	pcmobj_t *obj;
	WAVFILE *wfile;
	
	if (IS_LOADED(slot)) muspcm_unload(slot);
	
	wfile = pcmlib_load(no);
	if (wfile == NULL) {
		return NG;
	}
	
	// if has .wai file
	if (wai_mapadr) {
		int ch = WAIMIXCH(no);
		prv.vol_pcm_sub[slot] = ch < 0 ? 0: ch;
	} else {
		prv.vol_pcm_sub[slot] = 0;
	}
	//printf("rate = %d, bits = %d, ch = %d\n", wfile->rate, wfile->bits, wfile->ch);
	
	obj = g_new0(pcmobj_t, 1);
	obj->sdata = (void *)wfile;
	obj->stype = OBJSRC_FILE;
	
	switch(wfile->type) {
	case SND_WAVE:
		obj->src = ms_wav(wfile); break;
	case SND_WAVE_IMAADPCM:
		obj->src = ms_wav_ima(wfile); break;
	case SND_OGG:
		obj->src = ms_ogg(wfile); break;
	}
	
	obj->fmt.rate = wfile->rate;
	obj->fmt.bit  = wfile->bits;
	obj->fmt.ch   = wfile->ch;
	obj->data_len = (wfile->bytes * 1000) / (obj->fmt.rate * (obj->fmt.bit/8) * obj->fmt.ch);
	obj->slot     = slot;
	obj->playing  = FALSE;
	
	sndcnv_prepare(obj, prv.audiodev.buf.len / SLICE);
	
	prv.pcm[slot] = obj;
	
	// printf("pcm load slot = %d, no = %d\n", slot, no);
	
	return OK;
}

// ������PCM�ǡ������ɤ߹���
int muspcm_load_mem(int slot, void *mem) {
	pcmobj_t *obj;
	WAVFILE *wfile;
	
	if (IS_LOADED(slot)) muspcm_unload(slot);
	
	wfile = (WAVFILE *)((BYTE *)mem + sizeof(int));
	// printf("rate = %d, bits = %d, ch = %d\n", wfile->rate, wfile->bits, wfile->ch);
	
	obj = g_new0(pcmobj_t, 1);
	obj->sdata = mem;
	obj->stype = OBJSRC_MEM;
	obj->src   = ms_memory((BYTE *)mem + sizeof(int) + sizeof(WAVFILE), wfile->bytes);
	obj->fmt.rate = wfile->rate;
	obj->fmt.bit  = wfile->bits;
	obj->fmt.ch   = wfile->ch;
	obj->data_len = (wfile->bytes * 1000) / (obj->fmt.rate * (obj->fmt.bit/8) * obj->fmt.ch);
	obj->slot     = slot;
	obj->playing  = FALSE;
	
	sndcnv_prepare(obj, prv.audiodev.buf.len / SLICE);
	
	prv.pcm[slot] = obj;
	
	// printf("pcm load slot = %d, mem = %p\n", slot, mem);
	
	return OK;
}

// �ѥ��פ����PCM�ǡ���������ɤ߹���
int muspcm_load_pipe(int slot, char *cmd) {
	pcmobj_t *obj;
	
	if (IS_LOADED(slot)) muspcm_unload(slot);
	
	obj = g_new0(pcmobj_t, 1);
	obj->sdata = NULL;
	obj->stype = OBJSRC_PIPE;
	obj->src   = ms_pipe(cmd);
	obj->fmt   = prv.ofmt;
	obj->data_len = -1;
	obj->slot     = slot;
	obj->playing  = FALSE;
	
	sndcnv_prepare(obj, prv.audiodev.buf.len / SLICE);
	
	prv.pcm[slot] = obj;
	
	return OK;
}

// ���������촹�����ե����뤫���PCM�κ����Τ�����ɤ߹���
int muspcm_load_no_lrsw(int slot, int no) {
	pcmobj_t *obj;
	int ret = muspcm_load_no(slot, no);
	if (ret == NG) return NG;
	
	obj = prv.pcm[slot];
	if (((WAVFILE *)(obj->sdata))->bits == 8) {
		obj->src->lrswap8 = TRUE;
	} else {
		obj->src->lrswap16 = TRUE;
	}
	
	return OK;
}

// PCM�ǡ��������
int muspcm_start(int slot, int loop) {
	pcmobj_t *obj;
	// printf("pcm start slot = %d, loop = %d\n", slot, loop);
	
	obj = prv.pcm[slot];
	if (obj == NULL) return NG;
	
	obj->loop  = loop;
	obj->cnt   = 0;
	obj->vollv = 100;
	obj->written_len = 0;
	obj->src->seek(obj->src, 0, SEEK_SET);
	obj->playing = TRUE;
	
	if (-1 == g_list_index(prv.pcmplist, obj)) {
		prv.pcmplist = g_list_append(prv.pcmplist, (gpointer)obj);
	}
	
	return OK;
}

// PCM�ǡ����κ������
int muspcm_stop(int slot) {
	pcmobj_t *obj;
	
	obj = prv.pcm[slot];
	if (obj == NULL) return NG;
	
	prv.pcmplist = g_list_remove(prv.pcmplist, obj);
	obj->playing = FALSE;
	
	return OK;
}

// PCM�ǡ����Υ���夫��Υ������
int muspcm_unload(int slot) {
	pcmobj_t *obj;
	
	obj = prv.pcm[slot];
	if (obj == NULL) return NG;
	
	if (obj->playing) muspcm_stop(slot);
	
	switch(obj->stype) {
	case OBJSRC_FILE:
		pcmlib_free((WAVFILE *)(obj->sdata));
		break;
	case OBJSRC_MEM:
		g_free(obj->sdata);
		break;
	}
	sndcnv_drain(obj);
	obj->src->close(obj->src);
	g_free(obj);
	
	prv.pcm[slot] = NULL;
	
	return OK;
}

// PCM�ǡ�������������
int muspcm_pause(int slot) {
	if (prv.pcm[slot] != NULL) {
		prv.pcm[slot]->paused = TRUE;
	}
	return OK;
}

// PCM�ǡ������������߲��
int muspcm_unpause(int slot) {
	if (prv.pcm[slot] != NULL) {
		prv.pcm[slot]->paused = FALSE;
	}
	return OK;
}

static WORD clip(int v) {
	if (v > 32767)  return 32767;
	if (v < -32768) return -32768;
	return v;
}

static int pcmmix_mix(short *src, int len) {
	int len1, len2, i;
	audiodevbuf_t *buf = &prv.audiodev.buf;
	short *dst = (short *)buf->cur;
	
	// WARNING("mix to buf (%d) ptr=%p len = %d\n", buf->sw, buf->cur, len);
	
	buf->lenmax = MAX(buf->lenmax, len);
	
	if (buf->cur + len >= buf->end) {
		len1 = buf->end - buf->cur;
		len2 = len - len1;
	} else {
		len1 = len;
		len2 = 0;
	}
	
	for (i = 0; i < len1; i+=2) {
		*dst = clip((int)*src + (int)*dst);
		src++; dst++;
	}
	
	// ���ΥХåե���
	if (len2 != 0) {
		dst = (short *)buf->b[!buf->sw];
		for (i = 0; i < len2; i+=2) {
			*dst = clip((int)*src + (int)*dst);
			src++; dst++;
		}
	}
	return OK;
}

static int pcmmix_send2devbuf() {
	audiodevbuf_t *buf = &prv.audiodev.buf;
	int len2;
	
	if (buf->cur + buf->lenmax >= buf->end) {
		// ���ΥХåե������ؤ�
		len2 = buf->lenmax - (buf->end - buf->cur);
		buf->ready[buf->sw] = TRUE;
		buf->sw  = !(buf->sw);
		buf->cur = buf->b[buf->sw] + len2;
		buf->end = buf->b[buf->sw] + buf->len;
	} else {
		buf->cur += buf->lenmax;
	}
	
	buf->lenmax = 0;
	
	return OK;
}

// defice �˥ǡ���������줿�Ȥ��� cb �����ꤷ�Ƥ���Ȥ����Ƥ�
static GSList *cb_flush[2];
static void cb_at_devflush4each(gpointer data, gpointer userdata) {
	pcmobj_t *obj = (pcmobj_t *)data;
	obj->cb_atend(obj->fd);
}

static void cb_at_devflush(int sw) {
	if (cb_flush[sw]) {
		g_slist_foreach(cb_flush[sw],cb_at_devflush4each, NULL);
		g_slist_free(cb_flush[sw]);
		cb_flush[sw] = NULL;
	}
}

// PCM�����ꥹ�����PCM����������ǡ������ɤ߹���ǹ���
int muspcm_cb() {
	audiodevbuf_t *buf = &prv.audiodev.buf;
	pcmobj_t *obj;
	GList *node;
	GSList *removelist = NULL, *snode;
	int len;
	
	// �����ꥹ�Ȥ����ξ��Ϥʤˤ⤷�ʤ�
	if (g_list_length(prv.pcmplist) == 0) {
		return 0;
	}
	
	// ���ΥХåե������äѤ��ΤȤ��Ϥʤˤ⤷�ʤ�
	if (buf->ready[!buf->sw]) {
		//printf("buffer is full\n");
		return 0;
	}
	
	// ���Ƥκ����ꥹ����Υ����ͥ�����
	for (node = prv.pcmplist; node; node = g_list_next(node)) {
		obj = (pcmobj_t *)node->data;
		if (obj == NULL) continue;  // why ?
		
		// �����ξ��
		if (obj->paused) continue;
		
		len = obj->conv.convert(obj, obj->vollv * prv.volval[prv.vol_pcm_sub[obj->slot]] / 100, prv.audiodev.buf.len/SLICE);
		if (len <= 0) { // ���顼�ޤ��Ͻ�ü����ã�������
			if (obj->cb_atend) {
				// ���ս�λ���Τ餻���ߤ���
				// �ե����ɤ�pcm_stop�Ǥν�λ�κݤ��Τ餻�ʤ��Ƥ褤��
				cb_flush[buf->sw] = g_slist_append(cb_flush[buf->sw], obj);
			}
			// loop check
			if (obj->loop == 0) { // ̵�¥롼��
				obj->src->seek(obj->src, 0, SEEK_SET);
				obj->written_len = 0;
				continue;
			}
			if (--obj->loop == 0) { // �����֤���λ
				// remove
				// muspcm_stop(obj->slot);
				removelist = g_slist_append(removelist, obj);
				continue;
			} else { // ��Ƭ��
				obj->src->seek(obj->src, 0, SEEK_SET);
				obj->written_len = 0;
				continue;
			}
		}
		// ����
		pcmmix_mix(obj->conv.buf, len);
		obj->written_len += len;
	}
	
	len = prv.audiodev.buf.lenmax;
	if (len > 0) pcmmix_send2devbuf();
	
	// �Ǹ�� pcm playlist ���� stop ���� pcmobj ����
	for (snode = removelist; snode; snode = g_slist_next(snode)) {
		obj = (pcmobj_t *)snode->data;
		if (obj == NULL) continue;
		muspcm_stop(obj->slot);
	}
	
	return len; // �ºݤ˹�������Ĺ��
}

boolean muspcm_writable(void) {
  return prv.audiodev.writable(&prv.audiodev);
}

// �Хåե����PCM�ǡ�����ǥХ����˽��� (pollout���˸ƤФ��)
int muspcm_write2dev(void) {
	audiodevbuf_t *buf = &prv.audiodev.buf;
	int sw;
	
	// �Хåե���audiodevice�˽񤭹���OK����������
	sw = (buf->ready[0] ? 0 : buf->ready[1] ? 1 : -1);
	while (sw < 0) {
		int len = 0;
		if (g_list_length(prv.pcmplist) != 0) {
			// playlist���PCM�ǡ������������������
			// �Хåե������äѤ��ˤʤ�ޤ��ɤ߹���
			len = muspcm_cb();
		}
		if (len == 0) {
			// �ºݤ��ɤ߹��᤿�ǡ��������ξ��
			sw = buf->sw;
			if (buf->cur == buf->b[sw]) {
                          prv.audiodev.write(&prv.audiodev, NULL, 0); // tell the end
                          return OK;
                        }
			// �Ĥ�Υǡ������Ǥ��Ф��ȥХåե��Υ��ꥢ
			//   audiodevice�ΥХåե����������Τ����Ф��ʤ���
			//   FreeBSD+OSS�ǥǥХ�����write ready�ˤʤ�ʤ���
			prv.audiodev.write(&prv.audiodev, buf->b[sw], buf->len);
			memset(buf->b[sw], 0, buf->len);
			buf->ready[sw] = FALSE;
			buf->cur = buf->b[sw];
			buf->end = buf->cur + buf->len;
			
			cb_at_devflush(sw);
			return OK;
		}
		sw = (buf->ready[0] ? 0 : buf->ready[1] ? 1 : -1);
	}
	
	// �񤭹��ߤν������Ǥ��Ƥ�������ǥХ�����
	prv.audiodev.write(&prv.audiodev, buf->b[sw], buf->len);
	memset(buf->b[sw], 0, buf->len);
	buf->ready[sw] = FALSE;
	
	// �ǥХ����˽񤭹���ݤ� callback ������Ф����ƤӽФ�
	cb_at_devflush(sw);
	return OK;
}

// ���ߤκ������֤��֤�
int muspcm_getpos(int slot) {
	pcmobj_t *obj;
	guint64 len;
	
	obj = prv.pcm[slot];
	if (obj == NULL) return 0;
	
	if (!obj->playing) return 0;
	
	len = obj->written_len;
#if 0
	if (prv.audiodev.buf.ready[!prv.audiodev.buf.sw]) {
		len -= prv.audiodev.buf.len;
	}
	if (len < 0) return 0;
#endif	
	return (int)(((guint64)(len * 1000) / (obj->fmt.rate * (obj->fmt.bit/8) * obj->fmt.ch)));
}

// PCM���֥������Ȥ��Ф��ƥܥ�塼��򥻥å�
int muspcm_setvol(int dev, int slot, int lv) {
	pcmobj_t *obj;
	
	obj = prv.pcm[slot];
	if (obj == NULL) return NG;
	
	obj->vollv = lv;
	
	return OK;
}

// PCM�ǡ�����Ĺ�������
int muspcm_getwavelen(int slot) {
	pcmobj_t *obj;
	
	obj = prv.pcm[slot];
	if (obj == NULL) return 0;
	
	if (obj->data_len == -1) return 0;
	
	if (obj->data_len > 65535) return 65535;
	
	return obj->data_len;
}

// ����Υ���åȤ����߱����椫�ɤ��������
boolean muspcm_isplaying(int slot) {
	pcmobj_t *obj;
	
	obj = prv.pcm[slot];
	if (obj == NULL)   return FALSE;
	
	if (!obj->playing) return FALSE;
	
	return TRUE;
}
