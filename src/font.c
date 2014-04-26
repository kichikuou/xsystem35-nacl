/*
 * font.c  font device selecter
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
/* $Id: font.c,v 1.1 2002/09/18 13:16:22 chikama Exp $ */

#include "config.h"

#include "portab.h"
#include "nact.h"
#include "ags.h"
#include "font.h"

void font_init(int dev) {
	switch(dev) {
#ifdef ENAME_X11FONT
	case FONT_X11:
		nact->ags.font = font_x11_new();
		break;
#endif
#ifdef ENABLE_GTKFONT
	case FONT_GTK:
		nact->ags.font = font_gtk_new();
		break;
#endif
#ifdef ENABLE_TTF
	case FONT_TTF:
		nact->ags.font = font_ttf_new();
		break;
#endif
#ifdef ENABLE_FT2
	case FONT_FT2:
		nact->ags.font = font_ft2_new();
		break;
#endif
	default:
#ifdef ENABLE_SDL
          // nact->ags.font = font_gtk_new();
		break;
#else
		nact->ags.font = font_x11_new();
		break;
#endif
	}
}
