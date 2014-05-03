/*
 * music_pcm.h  music server PCM part
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
/* $Id: music_pcm.h,v 1.4 2003/08/22 17:09:23 chikama Exp $ */

#ifndef __MUSIC_PCM_H__
#define __MUSIC_PCM_H__

#include "musstream.h"
#include "dri.h"
#include "audio.h"

#define SLOT_CDROMPIPE 129
#define SLOT_MIDIPIPE  130

#define OBJSRC_FILE 1
#define OBJSRC_MEM  2
#define OBJSRC_PIPE 3

#define MAX_PRIVATE 10

struct _pcmobj;

// PCM �Ѵ��˴ؤ��륪�֥�������
struct _sndcnv {
	// ���ϥե����ޥå�
	chanfmt_t ifmt;

	// ���ϥե����ޥå�
	chanfmt_t ofmt;
	
	/* private area for effect */
        char priv[MAX_PRIVATE * 8];

	// src �����ɤ߹���Ĺ��
	int isample;
	
	// ���ȿ��Ѵ�����ʤ����Ѵ��ؿ�
	int (*convert)(struct _pcmobj *, int, int);
	
	// �Хåե�
	void *buf;
};
typedef struct _sndcnv sndcnv_t;

struct _pcmobj {
	chanfmt_t fmt;
	musstream_t *src;
	
	sndcnv_t conv; // �ե����ޥå��Ѵ���
	
	int vollv;   /* volume level */
	int loop;    /* ���꤫������ */
	int cnt;     /* �ºݤ˷����֤����� */
	
	int slot; // ���ɤ���Ƥ��륹��å��ֹ�

	int stype;   // �������ǡ������֥������ȥ����� (FILE,MEM,PIPE)
	void *sdata; // �������ǡ������֥�������
	
	/* ��λ���� client ���Τ餻�뤫�ɤ��� */
	void (* cb_atend)(int fd);
	
	/* ��λ�����Τ餻�� client �� file discpriter */
	int fd;

	/* wavefile �Υǡ�����Ĺ��(msñ��) */
	long data_len;
	
	/* ��׺�������(�Х��ȿ�) */
	long written_len;

	/* �������� */
	boolean paused;

	/* ������ */
	boolean playing;
};
typedef struct _pcmobj pcmobj_t;


extern int muspcm_init();
extern int muspcm_exit();
extern int muspcm_load_no(int slot, int no);
extern int muspcm_load_no_lrsw(int slot, int no);
extern int muspcm_load_mem(int slot, void *mem);
extern int muspcm_load_pipe(int slot, char *cmd);
extern int muspcm_start(int slot, int loop);
extern int muspcm_stop(int slot);
extern int muspcm_unload(int slot);
extern int muspcm_pause(int slot);
extern int muspcm_unpause(int slot);
extern int muspcm_getpos(int slot);
extern int muspcm_setvol(int dev, int slot, int lv);
extern int muspcm_getwavelen(int slot);
extern int muspcm_cb();
extern boolean muspcm_writable(void);
extern int muspcm_write2dev(void);
extern boolean muspcm_isplaying(int slot);
extern int sndcnv_prepare(pcmobj_t *pcm, int outlen);
extern int sndcnv_drain(pcmobj_t *pcm);

#endif /* __MUSIC_PCM_H__ */
