/*
 * music_fader.c  music server fader part
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
/* $Id: music_fader.c,v 1.5 2003/07/14 16:22:51 chikama Exp $ */

#include <stdio.h>
#include <glib.h>

#include "portab.h"
#include "system.h"
#include "music_server.h"
#include "counter.h"

struct _fadeobj {
	/* �ե����� volume �����ꤹ�� callback */
	int (*setvol)(int mdev, int subdev, int vol);
	
	/* �ե����ɽ�λ���˱��դ�λ������ callback */
	// int (*stop_playback)();
	
	/* mixer main device type */
	int mdev;
	
	/* mixer subdevice number */
	int subdev; /* 1-128 */
	
	/* start / end / current / previous volume */
	int st_vol, ed_vol, cur_vol, pre_vol;
	
	/* start / end / current time in 10 msec */
	long st_time, ed_time, cur_time;
	
        /* ��λ���� cd/midi/pcm ��ߤ�뤫�ɤ��� */
        boolean stop_at_end;
};
typedef struct _fadeobj fadeobj_t;

static int musfade_free(fadeobj_t *obj);
static int musfade_setvol(int dev, int lv);
static int musfade_setvol_dev(int dev, int subdev, int lv);
static int musfade_stop_playback(int dev, int subdev);
static void fade_calc_time(fadeobj_t *obj);
static int setvolcd(int lv);
static int setvolmidi(int lv);

int musfade_init() {
	int i;
	
	if (prv.cddev.setvol == NULL) {
		prv.cddev.setvol = setvolcd;
	}
	if (prv.mididev.setvol == NULL) {
		prv.mididev.setvol = setvolmidi;
	}
	
	// init counter for fader
	reset_counter_high(SYSTEMCOUNTER_FADE, 10, 0);
	
	// init workarea volume
	prv.vol_cd     = 100;
	prv.vol_midi   = 100;
	prv.vol_pcm    = 100;
	prv.vol_master = 100;
	
	for (i = 0; i < 16; i++) {
		prv.volval[i] = 100;
	}
	return OK;
}

int musfade_exit() {
	return OK;
}

static int setvolcd(int lv) {
	prv.audiodev.mix_set(&prv.audiodev, MIX_CD, lv);
	return OK;
}

static int setvolmidi(int lv) {
	prv.audiodev.mix_set(&prv.audiodev, MIX_MIDI, lv);
	return OK;
}

/*
 * �������ե������ѥ��֥������Ȥκ���
 *   dev      : �оݥߥ����� device (MASTER/CD/MIDI/PCM)
 *   subdev   : PCM device �ξ�硢subdevice �ֹ� (0-130)
 *   time     : �ե����ɽ�λ����
 *   ed_vol   : �ե����ɺǽ��ܥ�塼��
 *   stop     : �ե����ɽ�λ���˺�����λ���뤫�ɤ��� (1:�ߤ��, 0:�ߤ�ʤ�)
 */
int musfade_new(int dev, int subdev, int time, int ed_vol, int stop) {
	fadeobj_t *obj = g_new0(fadeobj_t, 1);

	// printf("fade new %d, %d, %d, %d, %d\n", dev, subdev, time, vol, stop);
	
	obj->mdev = dev;
	obj->subdev = subdev;
	
	obj->st_time = get_high_counter(SYSTEMCOUNTER_FADE);
	obj->ed_time = obj->st_time + time / 10; /* fade counter �� 10msec */
	
	obj->ed_vol = ed_vol;
	obj->stop_at_end = (stop == 1 ? TRUE : FALSE);
	
	// if subdev == 0 then set real mixer deive 
	if (subdev == 0) {
		// set volumeset function
		obj->setvol = musfade_setvol_dev;
		
		// set start volume
		switch(dev) {
		case MIX_CD:
			obj->st_vol = prv.vol_cd;
			break;
		case MIX_MIDI:
			obj->st_vol = prv.vol_midi;
			break;
		case MIX_PCM:
			obj->st_vol = prv.vol_pcm;
		}
	} else {
		// set volumeset function
		obj->setvol = muspcm_setvol;
		
		// set start volume
		if (prv.pcm[subdev] == NULL) {
			// null �ˤʤ�äƤ��Ȥ� start���Ƥ��ʤ� pcm �� stop
			// ���褦�Ȥ��Ƥ��������ɡ���ä������Ƥ��٤�����
			obj->st_vol = -1;
		} else {
			obj->st_vol = prv.pcm[subdev]->vollv;
		}
	}
	
	prv.fadelist = g_list_append(prv.fadelist, obj);
	
	return OK;
}

/*
  ����Υե����ɥ��֥������Ȥ���
    obj: �ä����֥�������
*/
static int musfade_free(fadeobj_t *obj) {
	prv.fadelist = g_list_remove(prv.fadelist, (gpointer)obj);
	g_free(obj);
	return OK;
}

/*
  �ե��������
    dev:    main device
    subdev: sub device
*/
int musfade_stop(int dev, int subdev) {
	GList *node;
	fadeobj_t *obj;

	for (node = prv.fadelist; node; node = g_list_next(node)) {
		obj = (fadeobj_t *)node->data;
		if (obj == NULL) continue;
		
		if (obj->mdev == dev && 
		    obj->subdev == subdev) {
			musfade_free(obj);
			break;
		}
	}
	return OK;
}

/*
  �ե������椫�ɤ���
    dev:    main device
    subdev: sub device

    ret: TRUE or FALSE
*/
boolean musfade_getstate(int dev, int subdev) {
	GList *node;
	fadeobj_t *obj;
	
	for (node = prv.fadelist; node; node = g_list_next(node)) {
		obj = (fadeobj_t *)node->data;
		if (obj == NULL) continue;
		
		if (obj->mdev == dev && 
		    obj->subdev == subdev) {
			return TRUE;
		}
	}

	return FALSE;
}

/*
  ����ΥǥХ����Υ�������ǤΥܥ�塼��μ���
    dev: ��������ǥХ���
*/
int musfade_getvol(int dev) {
	switch(dev) {
	case MIX_MASTER:
		return prv.vol_master;
	case MIX_PCM:
		return prv.vol_pcm;
	case MIX_MIDI:
		return prv.vol_midi;
	case MIX_CD:
		return prv.vol_cd;
	default:
		return 0;
	}
}

/*
  ����ΥǥХ����Υ�������ǤΥܥ�塼�������
    dev: ���ꤹ��ǥХ���
    val: ���ꤹ����
*/
static int musfade_setvol(int dev, int lv) {
	switch(dev) {
	case MIX_MASTER:
		prv.vol_master = lv;
		break;
		
	case MIX_PCM:
		prv.vol_pcm = lv;
		break;
		
	case MIX_MIDI:
		prv.vol_midi = lv;
		break;
		
	case MIX_CD:
		prv.vol_cd = lv;
		break;
		
	default:
		break;
	}
	return OK;
}

static int musfade_setvol_dev(int dev, int subdev, int lv) {
	switch(dev) {
	case MIX_MASTER:
	case MIX_PCM:
		prv.audiodev.mix_set(&prv.audiodev, dev, lv);
		break;
	case MIX_CD:
		prv.cddev.setvol(lv);
		break;
	case MIX_MIDI:
		prv.mididev.setvol(lv);
		break;
	default:
		break;
	}
	
	return OK;
}

static int musfade_stop_playback(int dev, int subdev) {
	switch(dev) {
	case MIX_CD:
		muscd_stop();
		break;
	case MIX_MIDI:
		musmidi_stop();
		break;
	case MIX_PCM:
		muspcm_stop(subdev);
		break;
	default:
		break;
	}
	return OK;
}

static void fade_calc_time(fadeobj_t *obj) {
	/* ���ߤλ������� */
	// usleep(1);
	obj->cur_time = get_high_counter(SYSTEMCOUNTER_FADE);

	obj->pre_vol = obj->cur_vol;
	
	/* ��λ���郎�᤮�Ƥ�����ǽ��ܥ�塼��� */
	if (obj->cur_time > obj->ed_time) {
		obj->cur_vol = obj->ed_vol;
		goto out;
	}
	
	/* �ե����ɻ��֤����ξ���ǽ��ܥ�塼��� */
	if (obj->ed_time == obj->st_time) {
		obj->cur_vol = obj->ed_vol;
		goto out;
	}
	
	/* �����ǥܥ�塼������ */
	obj->cur_vol  = ((obj->ed_vol - obj->st_vol) * (obj->cur_time - obj->st_time) / ( obj->ed_time - obj->st_time)) + obj->st_vol;
	
 out:
	/* prv. �ΰ��ȿ�� */
	if (obj->subdev == 0) {
		musfade_setvol(obj->mdev, obj->cur_vol);
	}
}
        
int musfade_cb() {
	fadeobj_t *obj;
	GList *node, *next;
	
	for (node = prv.fadelist; node; node = next) {
		next = g_list_next(node);
		obj = (fadeobj_t *)node->data;
		if (obj == NULL) continue;
		
		// ed_vol = -1 : �����˽�λ
		if (obj->ed_vol == -1) {
			musfade_stop_playback(obj->mdev, obj->subdev);
			musfade_free(obj);
			continue;
		}
		
		// �ե����ɻ��ַ׻�
		fade_calc_time(obj);
		
		if (obj->cur_vol != obj->pre_vol) {
			// set ��Ǥϸ��ߤ� volume �� obj->cur_vol % �ǽ���
			obj->setvol(obj->mdev, obj->subdev, obj->cur_vol);
		}
		
		if (obj->cur_vol == obj->ed_vol) {
			if (obj->stop_at_end) {
				// stop
				musfade_stop_playback(obj->mdev, obj->subdev);
			}
			// remove fadeobj
			musfade_free(obj);
		}
	}
	
	return OK;
}

int musfade_setvolval(int *valance, int num) {
	int i;
	
	for (i = 0; i < MIN(num, 16); i++) {
		prv.volval[i] = valance[i];
	}
	
	return OK;
}
