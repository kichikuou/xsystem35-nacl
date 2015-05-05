/*
 * sdl_darw.c  SDL event handler
 *
 * Copyright (C) 2000-     Fumihiko Murata       <fmurata@p1.tcnet.ne.jp>
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
/* $Id: sdl_event.c,v 1.5 2001/12/16 17:12:56 chikama Exp $ */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <SDL/SDL.h>

#include "portab.h"
#include "system.h"
#include "counter.h"
#include "nact.h"
#include "sdl_private.h"
#include "key.h"
#include "menu.h"
#include "imput.h"
#include "joystick.h"
#include "sdl_input.c"

static void sdl_getEvent(void);
static void keyEventProsess(SDL_KeyboardEvent *e, boolean bool);
static int  check_button(void);


/* pointer �ξ��� */
static int mousex, mousey, mouseb;
boolean RawKeyInfo[256];
/* SDL Joystick */
static int joyinfo=0;

static int mouse_to_rawkey(int button) {
	switch(button) {
	case SDL_BUTTON_LEFT:
		return KEY_MOUSE_LEFT;
	case SDL_BUTTON_MIDDLE:
		return KEY_MOUSE_MIDDLE;
	case SDL_BUTTON_RIGHT:
		return KEY_MOUSE_RIGHT;
	}
	return 0;
}

/* Event���� */
static void sdl_getEvent(void) {
	SDL_Event e;
	boolean m2b = FALSE, msg_skip = FALSE, had_event = FALSE;
	int i;
	
	while (SDL_PollEvent(&e)) {
		had_event = TRUE;
		switch (e.type) {
		case SDL_ACTIVEEVENT:
			if (e.active.state & SDL_APPMOUSEFOCUS) {
				ms_active = e.active.gain;
#if 0
				if (sdl_fs_on) {
					if (ms_active) {
						SDL_WM_GrabInput(SDL_GRAB_ON);
					} else {
						SDL_WM_GrabInput(SDL_GRAB_OFF);
					}
				}
#endif
			}
			if (e.active.state & SDL_APPINPUTFOCUS) {
			}
			
			break;
		case SDL_KEYDOWN:
			keyEventProsess(&e.key, TRUE);
			break;
		case SDL_KEYUP:
			keyEventProsess(&e.key, FALSE);
			if (e.key.keysym.sym == SDLK_F1) msg_skip = TRUE;
			if (e.key.keysym.sym == SDLK_F4) {
				SDL_WM_ToggleFullScreen(sdl_display);
				sdl_fs_on = !sdl_fs_on;
			}
			break;
		case SDL_MOUSEMOTION:
			mousex = e.motion.x;
			mousey = e.motion.y;
			break;
		case SDL_MOUSEBUTTONDOWN:
			mouseb |= (1 << e.button.button);
			RawKeyInfo[mouse_to_rawkey(e.button.button)] = TRUE;
#if 0
			if (e.button.button == 2) {
				keywait_flag=TRUE;
			}
#endif
			break;
		case SDL_MOUSEBUTTONUP:
			mouseb &= (0xffffffff ^ (1 << e.button.button));
			RawKeyInfo[mouse_to_rawkey(e.button.button)] = FALSE;
			if (e.button.button == 2) {
				m2b = TRUE;
			}
			break;
#if HAVE_SDLJOY
		case SDL_JOYAXISMOTION:
			if (abs(e.jaxis.value) < 0x4000) {
				joyinfo &= e.jaxis.axis == 0 ? ~0xc : ~3;
			} else {
				i = (e.jaxis.axis == 0 ? 2 : 0) + 
					(e.jaxis.value > 0 ? 1 : 0);
				joyinfo |= 1 << i;
			}
			break;
		case SDL_JOYBALLMOTION:
			break;
		case SDL_JOYHATMOTION:
			break;
		case SDL_JOYBUTTONDOWN:
		case SDL_JOYBUTTONUP:
			if (e.jbutton.button < 4) {
				i = 1 << (e.jbutton.button+4);
				if (e.jbutton.state == SDL_PRESSED)
					joyinfo |= i;
				else
					joyinfo &= ~i;
			} else {
				if (e.jbutton.state == SDL_RELEASED) {
				}
			}
			break;
#endif
		case SDL_VIDEORESIZE:
			SDL_UpdateRect(sdl_display, 0, 0, 0, 0);
			break;
		default:
			printf("ev %x\n", e.type);
			break;
		}
	}
	if (!had_event) usleep(1000);
	
	if (m2b) {
		menu_open();
	}
	
	if (msg_skip) set_skipMode(!get_skipMode());
}

int sdl_keywait(int msec, boolean cancel) {
	int key=0, n;
	int cnt = get_high_counter(SYSTEMCOUNTER_MSEC);
	
	if (msec < 0) return 0;
	
	while (msec > (get_high_counter(SYSTEMCOUNTER_MSEC) - cnt)) {
		sdl_getEvent();
		key = check_button() | sdl_getKeyInfo() | joy_getinfo();
		if (cancel && key) break;
		n = msec - (get_high_counter(SYSTEMCOUNTER_MSEC) - cnt);
		if (n < 0) break;
		if (n < 10) {
			usleep(1000 * n);
		} else {
			usleep(10000);
		}
		nact->callback();
	}
	
	return key;
}

/* ��������μ��� */
static void keyEventProsess(SDL_KeyboardEvent *e, boolean bool) {
	RawKeyInfo[sdl_keytable[e->keysym.sym]] = bool;
}

int sdl_getKeyInfo() {
	int rt;
	
	sdl_getEvent();
	
	rt = ((RawKeyInfo[KEY_UP]     || RawKeyInfo[KEY_PAD_8])       |
	      ((RawKeyInfo[KEY_DOWN]  || RawKeyInfo[KEY_PAD_2]) << 1) |
	      ((RawKeyInfo[KEY_LEFT]  || RawKeyInfo[KEY_PAD_4]) << 2) |
	      ((RawKeyInfo[KEY_RIGHT] || RawKeyInfo[KEY_PAD_6]) << 3) |
	      (RawKeyInfo[KEY_RETURN] << 4) |
	      (RawKeyInfo[KEY_SPACE ] << 5) |
	      (RawKeyInfo[KEY_ESC]    << 6) |
	      (RawKeyInfo[KEY_TAB]    << 7));
	
	return rt;
}

static int check_button(void) {
	int m1, m2;
	
	m1 = mouseb & (1 << 1) ? SYS35KEY_RET : 0;
	m2 = mouseb & (1 << 3) ? SYS35KEY_SPC : 0;
	
	return m1 | m2;
}

double g_nacl_scale = 1.0;

int sdl_getMouseInfo(MyPoint *p) {
	sdl_getEvent();
	
	if (!ms_active) {
		if (p) {
			p->x = 0; p->y = 0;
		}
		return 0;
	}
	
	if (p) {
		p->x = (mousex - winoffset_x) / g_nacl_scale;
		p->y = (mousey - winoffset_y) / g_nacl_scale;
	}
	return check_button();
}

#ifdef HAVE_SDLJOY
int sdl_getjoyinfo(void) {
	sdl_getEvent();
	return joyinfo;
}
#endif

