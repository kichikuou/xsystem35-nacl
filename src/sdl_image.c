/*
 * sdl_image.c  image��� for SDL
 *
 * Copyright (C) 2000-   Fumihiko Murata <fmurata@p1.tcnet.ne.jp>
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
/* $Id: sdl_image.c,v 1.23 2003/07/20 19:30:16 chikama Exp $ */

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <SDL/SDL.h>
#include <glib.h>

#include "portab.h"
#include "system.h"
#include "sdl_core.h"
#include "sdl_private.h"
#include "cg.h"
#include "nact.h"
#include "alpha_plane.h"
#include "image.h"

static unsigned char shlv_tbl[256*256];

/*
 * dib��Ǥγ��硦�̾����ԡ�
 */
void sdl_scaledCopyArea(SDL_Surface *src, SDL_Surface *dst, int sx, int sy, int sw, int sh, int dx, int dy, int dw, int dh, int mirror) {
	float    a1, a2, xd, yd;
	int      *row, *col;
	int      x, y;
	SDL_Surface *ss;
	SDL_Rect r_src, r_dst;
	
	if (src == NULL) {
		src = sdl_dib;
	}

	if (dst == NULL) {
		dst = sdl_dib;
	}
	ss = SDL_AllocSurface(SDL_ANYFORMAT, dw, dh, dst->format->BitsPerPixel, 0, 0, 0, 0);
	
	SDL_LockSurface(ss);

	if (dst->format->BitsPerPixel == 8) {
		memcpy(ss->format->palette->colors, dst->format->palette->colors,
		       sizeof(SDL_Color) * 256);
	}
	
	a1  = (float)sw / (float)dw;
	a2  = (float)sh / (float)dh;
	// src width �� dst width ��Ʊ���Ȥ������꤬����Τ�+1
	row = g_new0(int, dw+1);
	// 1�����������ƽ�������ʤ��� col[dw-1]��col[dw]��Ʊ���ˤʤ�
	// ��ǽ�������롣
	col = g_new0(int, dh+1);
	
	if(mirror & 1){
		/*�岼ȿž added by  tajiri@wizard*/
		for (yd = sh-a2, y = 0; y<dh; y++) {
			col[y] = yd; yd -= a2;
		}
	} else {
		for (yd = 0.0, y = 0; y < dh; y++) {
			col[y] = yd; yd += a2;
		}
	}
	if(mirror & 2){
		/*����ȿž added by  tajiri@wizard*/
		for (xd = sw-a1, x = 0; x <dw; x++) {
			row[x] = xd; xd -= a1;
		}
	} else {
		for (xd = 0.0, x = 0; x < dw; x++) {
			row[x] = xd; xd += a1;
		}
	}

#define sccp(type) \
{ \
	type *p_ss = src->pixels + sx * src->format->BytesPerPixel + sy * src->pitch;\
	type *p_dd = ss->pixels, *p_src, *p_dst;\
	int l=src->pitch/src->format->BytesPerPixel; \
	int m=ss->pitch/ss->format->BytesPerPixel; \
	for (y = 0; y < dh; y++) { \
		p_src = p_ss + col[y] * l; \
		p_dst = p_dd + y * m; \
		for (x = 0; x < dw ; x++) { \
			*(p_dst++) = *(p_src + *(row + x)); \
		} \
	} \
}
	switch(dst->format->BytesPerPixel) {
	case 1:
		sccp(BYTE);
		break;
	case 2:
		sccp(WORD);
		break;
	case 3: {
		BYTE *p_ss=(BYTE *)(src->pixels+sx*src->format->BytesPerPixel+sy*src->pitch);
		BYTE *p_src=p_ss,*p_dd=ss->pixels, *p_dst;
		for (y = 0; y < dh; y++) {
			p_src = p_ss + col[y] * src->pitch;
			p_dst = p_dd + y      * ss->pitch;
			for (x = 0; x < dw ; x++) {
				*(p_dst)   = *(p_src + (*(row + x))*3    );
				*(p_dst+1) = *(p_src + (*(row + x))*3 + 1);
				*(p_dst+2) = *(p_src + (*(row + x))*3 + 2);
				p_dst+=3;
			}
		}
		break;
	}
	case 4:
		sccp(Uint32);
		break;
	}
	
	SDL_UnlockSurface(ss);
	
	setRect(r_src,  0,  0, dw, dh);
	setRect(r_dst, dx, dy, dw, dh);
	SDL_BlitSurface(ss, &r_src, dst, &r_dst);
	
	SDL_FreeSurface(ss);
	
	g_free(row);
	g_free(col);
}

void sdl_zoom(int x, int y, int w, int h) {
	sdl_willUpdateDisplay();
	sdl_scaledCopyArea(sdl_dib, sdl_display, x, y, w, h, 0, 0, view_w, view_h, 0);
	sdl_updateDisplay(0, 0, view_w, view_h);
}


/*
 * dib��16bitCG������
 */
void sdl_drawImage16_fromData(cgdata *cg, int dx, int dy, int w, int h) {
	/* draw alpha pixel */
	SDL_Surface *s;
	SDL_Rect r_src,r_dst;

	if (cg->alpha != NULL && cg->spritecolor != -1) {
		unsigned short *p_ds, *p_src = (WORD *)(cg->pic + cg->data_offset);
		unsigned short *p_dst;
		BYTE *a_src = cg->alpha;
		BYTE *adata = GETOFFSET_ALPHA(sdl_dibinfo, dx, dy);
		int x, y, l;
		
#ifndef WORDS_BIGENDIAN
		s = SDL_AllocSurface(SDL_ANYFORMAT, w, h, 32,
				     0xf800, 0x07e0, 0x001f, 0xFF0000);
#else
		s = SDL_AllocSurface(SDL_ANYFORMAT,w, h, 32,
				     0xf800, 0x07e0, 0x001f, 0xFF000000);
#endif
#if 0
#ifdef HAVE_SDLRLE
		SDL_SetAlpha(s,RLEFLAG(SDL_SRCALPHA),0);
#endif
#endif
		SDL_LockSurface(s);
		p_ds = s->pixels;
		l = s->pitch/2;
		for (y = 0; y < h; y++) {
			p_dst = p_ds;
			memcpy(adata, a_src, w);
			for (x = 0; x < w; x++) {
#ifndef WORDS_BIGENDIAN
				*(p_dst++) = *(p_src++);
				*(p_dst++) = R_ALPHA(*(a_src++));
#else
				*(p_dst++) = R_ALPHA(*(a_src++));
				*(p_dst++) = *(p_src++);
#endif
			}
			p_ds  += l;
			adata += sdl_dibinfo->width;
		}
		SDL_UnlockSurface(s);
	} else {
		BYTE *p_dst;
		unsigned short *p_src = (WORD *)(cg->pic + cg->data_offset);
		int i, l = w * 2;
		
		s = SDL_AllocSurface(SDL_ANYFORMAT, w, h, 16, 0, 0, 0, 0);
		SDL_LockSurface(s);
		p_dst = s->pixels;
		for (i = 0; i < h; i++) {
			memcpy(p_dst, p_src, l);
			p_dst += s->pitch;
			p_src += cg->width;
		}
		if (cg->alpha) {
			BYTE *a_src = cg->alpha;
			BYTE *adata = GETOFFSET_ALPHA(sdl_dibinfo, dx, dy);
			
			for (i = 0; i < h; i++) {
				memcpy(adata, a_src, w);
				adata += sdl_dibinfo->width;
				a_src += cg->width;
			}
		}
		SDL_UnlockSurface(s);
	}
	setRect(r_src,  0,  0, w, h);
	setRect(r_dst, dx, dy, w, h);
	SDL_BlitSurface(s, &r_src, sdl_dib, &r_dst);
	SDL_FreeSurface(s);
}

/*
 * 16bit���Ѥ� dib �λ����ΰ襳�ԡ� alpha�Ĥ�
 */
void sdl_shadow_init(void) {
	int i, j;
	unsigned char *c = shlv_tbl;

	for (i = 0; i < 255; i++) {
		for (j = 0; j < 256; j++) {
			*(c++) = (unsigned char)(R_ALPHA(i * j / 255));
		}
	}
}

void sdl_copyAreaSP16_shadow(int sx, int sy, int w, int h, int dx, int dy, int lv) {
	BYTE *adata = GETOFFSET_ALPHA(sdl_dibinfo, sx, sy);
	BYTE *p_src, *p_dst, *p_ds;
	
	SDL_Surface *s = SDL_AllocSurface(SDL_ANYFORMAT, w, h, 32, sdl_dib->format->Rmask,
					  sdl_dib->format->Gmask, sdl_dib->format->Bmask,
					  0xFF000000);
	SDL_Rect r_src, r_dst;
	int x, y;

	setRect(r_dst, sx, sy, w, h);
	setRect(r_src,  0,  0, w, h);
	SDL_BlitSurface(sdl_dib, &r_dst, s, &r_src);

	SDL_LockSurface(s);

#ifndef WORDS_BIGENDIAN
	p_ds = s->pixels + 3;
#else
	p_ds = s->pixels;
#endif
	switch(lv) {
	case 255:
		for (y = 0; y < h; y++) {
			p_src = adata;
			p_dst = p_ds;
			for (x = 0; x < w; x++) {
				*p_dst = R_ALPHA(*(p_src++));
				p_dst += 4;
			}
			adata += sdl_dibinfo->width;
			p_ds  += s->pitch;
		}
		break;
	default:
		{
			unsigned char *lvtbl = shlv_tbl + lv * 256;
			for (y = 0; y < h; y++) {
				p_src = adata;
				p_dst = p_ds;
				for (x = 0; x < w; x++) {
					*p_dst = lvtbl[(int)(*(p_src++))];
					p_dst += 4;
				}
				adata += sdl_dibinfo->width;
				p_ds += s->pitch;
			}
		}
		break;
	}
	SDL_UnlockSurface(s);
#if 0
#ifdef HAVE_SDLRLE
	SDL_SetAlpha(s, RLEFLAG(SDL_SRCALPHA), 0);
#endif
#endif

	setRect(r_dst, dx, dy, w, h);
	SDL_BlitSurface(s, &r_src, sdl_dib, &r_dst);
	SDL_FreeSurface(s);
}

void sdl_copyAreaSP16_alphaBlend(int sx, int sy, int w, int h, int dx, int dy, int lv) {
	SDL_Rect r_src, r_dst;

	setRect(r_src, sx, sy, w, h);
	setRect(r_dst, dx, dy, w, h);
	SDL_SetAlpha(sdl_dib, RLEFLAG(SDL_SRCALPHA), R_ALPHA(lv));
	SDL_BlitSurface(sdl_dib, &r_src, sdl_dib, &r_dst);
	SDL_SetAlpha(sdl_dib, 0, 0);
}

#define copyAreaSP_level(fn, cn, v) void sdl_copyAreaSP16_##fn(int sx, int sy, int w, int h, int dx, int dy, int lv) { \
	SDL_Rect r_src,r_dst; \
	setRect(r_src, sx, sy, w, h); \
	setRect(r_dst, dx, dy, w, h); \
	SDL_SetAlpha(sdl_dib, RLEFLAG(SDL_SRCALPHA), v); \
	SDL_FillRect(sdl_dib, &r_dst, cn); \
	SDL_BlitSurface(sdl_dib, &r_src, sdl_dib, &r_dst); \
	SDL_SetAlpha(sdl_dib, 0, 0); \
}

#ifdef HAVE_SDLRALPHA
copyAreaSP_level(alphaLevel, 0, lv);
copyAreaSP_level(whiteLevel, sdl_white, 255 - lv);
#else
copyAreaSP_level(alphaLevel, 0, 255 - lv);
copyAreaSP_level(whiteLevel, sdl_white, lv);
#endif

void sdl_copy_from_alpha(int sx, int sy, int w, int h, int dx, int dy, ALPHA_DIB_COPY_TYPE flag) {
	SDL_LockSurface(sdl_dib);
	image_copy_from_alpha(nact->ags.dib, sx, sy, w, h, dx, dy, flag);
	SDL_UnlockSurface(sdl_dib);
}

void sdl_copy_to_alpha(int sx, int sy, int w, int h, int dx, int dy, ALPHA_DIB_COPY_TYPE flag) {
	SDL_LockSurface(sdl_dib);
	image_copy_to_alpha(nact->ags.dib, sx, sy, w, h, dx, dy, flag);
	SDL_UnlockSurface(sdl_dib);
}

/*
 * dib �Υԥ������������
 */
void sdl_getPixel(int x, int y, Pallet *cell) {
	SDL_LockSurface(sdl_dib);

	if (sdl_dib->format->BitsPerPixel == 8) {
		BYTE *rt = setOffset(sdl_dib, x, y);
		cell->pixel = *rt;
	} else {
		Uint32 cl=0;
		BYTE *p = setOffset(sdl_dib, x, y);
		
		switch (sdl_dib->format->BytesPerPixel) {
		case 2:
			{
				unsigned short *pp = (unsigned short *)p;
				cl = *pp;
			}
			break;
		case 3:
#ifndef WORDS_BIGENDIAN
			cl = (p[2]<<16)+(p[1]<<8) + p[0];
#else
			cl = (p[0]<<16)+(p[1]<<8) + p[2];
#endif
			break;
		case 4:
			{
				Uint32 *pp = (Uint32 *)p;
				cl = *pp;
			}
			break;
		}
		SDL_GetRGB(cl, sdl_dib->format, &cell->r, &cell->g, &cell->b);
	}
	
	SDL_UnlockSurface(sdl_dib);
}

/*
 * dib �����ΰ���ڤ�Ф�
 */
SDL_Surface* sdl_saveRegion(int x, int y, int w, int h) {
	SDL_Surface *s;
	SDL_Rect r_src, r_dst;

	s = SDL_AllocSurface(sdl_dib->flags, w, h, sdl_dib->format->BitsPerPixel,
			     sdl_dib->format->Rmask, sdl_dib->format->Gmask,
			     sdl_dib->format->Bmask, sdl_dib->format->Amask);
	if (sdl_dib->format->BitsPerPixel == 8)
		memcpy(s->format->palette->colors, sdl_dib->format->palette->colors,
		       sizeof(SDL_Color) * sdl_dib->format->palette->ncolors);
	setRect(r_src, x, y, w, h);
	setRect(r_dst, 0, 0, w, h);
	SDL_BlitSurface(sdl_dib, &r_src, s, &r_dst);
	return s;
}

/*
 * dib �˥����֤����ΰ�����
 */
void sdl_putRegion(SDL_Surface *src, int x, int y) {
	SDL_Rect r_src,r_dst;
	
	setRect(r_src, 0, 0, src->w, src->h);
	setRect(r_dst, x, y, src->w, src->h);

	SDL_BlitSurface(src, &r_src, sdl_dib, &r_dst);
}

/*
 * dib �˥����֤����ΰ褫�饳�ԡ�
 */
void sdl_CopyRegion(SDL_Surface *src, int sx, int sy, int w,int h, int dx, int dy) {
	SDL_Rect r_src,r_dst;

	setRect(r_src, sx, sy, w, h);
	setRect(r_dst, dx, dy, w, h);

	SDL_BlitSurface(src, &r_src, sdl_dib, &r_dst);
}

/*
 * dib �� dst������塢�����
 */
void sdl_restoreRegion(SDL_Surface *src, int x, int y) {
	sdl_putRegion(src, x ,y);
	SDL_FreeSurface(src);
}

void sdl_sync() {
	// imageXX����surface���ľ������ݤˡ�������lock/unlock��
	// �Ϥ��ޤʤ��Ȥ����ʤ�������ɡ�lock����Ф���ޤǥ��塼�����ä�
	// ���� SDL�����ν��������٤ƽ����ͽ�ۤ����Τ�Xsync��Ʊ�ͤ�
	// ���̤�����Ȼפ���������Unlock���Ƥ���Τ���Ʊ����imageXX�ν�����
	// ��SDL������̿�᤬�����ǽ���⤢�뤬��X11�ǤϤ��֤�����ס�
	SDL_LockSurface(sdl_dib);
	SDL_UnlockSurface(sdl_dib);
}
