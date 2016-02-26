/*
 * modulemain.cc  NaCl entry point
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
#include <errno.h>
#include <stdio.h>
#include <sys/mount.h>

#include <SDL/SDL_main.h>
#include <nacl_io/nacl_io.h>
#include <ppapi_simple/ps_instance.h>
#include <ppapi_simple/ps_interface.h>
#include <ppapi_simple/ps_main.h>

#include "naclmsg.h"

extern "C" void NACL_SetScreenResolution(int width, int height);

/* Wait for the first PSE_INSTANCE_DIDCHANGEVIEW event before starting the app */
static void wait_didChangeView() {
  const PPB_View *ppb_view = PSInterfaceView();

  PSEventSetFilter(PSE_INSTANCE_DIDCHANGEVIEW);
  /* Process all waiting events without blocking */
  bool ready = false;
  while (!ready) {
    PSEvent* ps_event = PSEventWaitAcquire();
    PP_Resource event = ps_event->as_resource;
    if (ps_event->type == PSE_INSTANCE_DIDCHANGEVIEW) {
      /* From DidChangeView, contains a view resource */
      struct PP_Rect rect;
      ppb_view->GetRect(event, &rect);
      NACL_SetScreenResolution(rect.size.width, rect.size.height);
      ready = true;
    }
    PSEventRelease(ps_event);
  }
  PSInterfaceInputEvent()->ClearInputEventRequest(
      PSGetInstanceId(),
      PP_INPUTEVENT_CLASS_WHEEL | PP_INPUTEVENT_CLASS_TOUCH);
}

static int nacl_main(int argc, char* argv[]) {
  umount("/");
  const char* http_mnt = "xsystem35";
  if (mount(http_mnt, "/", "httpfs", 0, "") != 0)
    fprintf(stderr, "%s: mount failed: %s\n", http_mnt, strerror(errno));
  if (mount("", "/gamedata", "html5fs", 0, "type=PERSISTENT,expected_size=62914560") != 0)
    fprintf(stderr, "/gamedata: mount failed: %s\n", strerror(errno));
  if (mount("", "/tmp", "memfs", 0, "") != 0)
    fprintf(stderr, "/tmp: mount failed: %s\n", strerror(errno));

  wait_didChangeView();

  g_naclMsg = new NaclMsg();

  int rtn = SDL_main(argc, argv);
  fprintf(stderr, "SDL: SDL_main returned: %d\n", rtn);
  exit(rtn);
  return NULL;
}

PPAPI_SIMPLE_REGISTER_MAIN(nacl_main)
