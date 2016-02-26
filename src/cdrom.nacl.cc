/*
 * cdrom.nacl.cc  NaCl CD-ROM shim
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
#include <stdio.h>

#include <ppapi/cpp/instance.h>
#include <ppapi/cpp/var_dictionary.h>

#include "naclmsg.h"
#include "portab.h"
#include "cdrom.h"

extern "C" {
#include "system.h"
}

static int  cdrom_init(char *);
static int  cdrom_exit();
static int  cdrom_start(int, int);
static int  cdrom_stop();
static int  cdrom_getPlayingInfo(cd_time *);

#define cdrom cdrom_nacl
extern "C" cdromdevice_t cdrom;
cdromdevice_t cdrom = {
	cdrom_init,
	cdrom_exit,
	cdrom_start,
	cdrom_stop,
	cdrom_getPlayingInfo,
	NULL,
	NULL
};

int cdrom_init(char *dev_cd) {
  return OK;
}

int cdrom_exit() {
  cdrom_stop();
  return OK;
}

int cdrom_start(int trk, int loop) {
  if (!g_naclMsg)
    return NG;

  pp::VarDictionary msg;
  msg.Set("command", "cd_play");
  msg.Set("track", trk);
  msg.Set("loop", loop);
  g_naclMsg->PostMessage(msg);
  return OK;
}

int cdrom_stop() {
  if (!g_naclMsg)
    return NG;

  pp::VarDictionary msg;
  msg.Set("command", "cd_stop");
  g_naclMsg->PostMessage(msg);
  return OK;
}

int cdrom_getPlayingInfo(cd_time *info) {
  info->t = info->m = info->s = info->f = 999;

  if (!g_naclMsg)
    return NG;

  pp::VarDictionary msg;
  msg.Set("command", "cd_getposition");

  pp::Var result = g_naclMsg->SendMessage(msg);
  assert(result.is_int());
  int t = result.AsInt();
  if (!t)
    return NG;

  info->t = t & 0xff;
  FRAMES_TO_MSF(t >> 8, &info->m, &info->s, &info->f);

  NOTICE("cdrom_getPlayingInfo: %d %d %d %d\n",
		 info->t, info->m, info->s, info->f);

  return OK;
}
