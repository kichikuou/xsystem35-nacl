/*
 * naclmsg.h  NaCl <-> JavaScript messaging
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

#ifndef __NACLMSG_H_
#define __NACLMSG_H_

#ifdef __cplusplus

#include <time.h>
#include <pthread.h>
#include <ppapi/cpp/var.h>
#include <ppapi_simple/ps_instance.h>

namespace pp {
  class Instance;
  class VarDictionary;
}

class NaclMsg {
 public:
  NaclMsg();
  ~NaclMsg();

  void PostMessage(const pp::Var&);
  pp::Var SendMessage(pp::VarDictionary&);

  void HandleMessage(const pp::Var&);

 private:
  pthread_mutex_t lock_;
  pthread_cond_t cond_;
  int next_request_id_;
  int result_id_;
  pp::Var result_;
};

extern NaclMsg* g_naclMsg;

extern "C" {
#endif // __cplusplus

void naclmsg_ready();
void naclmsg_exit(int code);
void naclmsg_setWindowSize(int width, int height);
void naclmsg_setCaption(char *name);
struct tm *naclmsg_localtime(const time_t *timep);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __NACLMSG_H_
