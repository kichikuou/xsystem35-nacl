#include <ppapi/cpp/instance.h>
#include <ppapi/cpp/var_dictionary.h>

#include "portab.h"
#include "cdrom.h"

pp::Instance* getXSystem35Instance();

static int  cdrom_init(char *);
static int  cdrom_exit();
static int  cdrom_start(int);
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
}

int cdrom_start(int trk) {
  pp::Instance* instance = getXSystem35Instance();
  if (!instance)
    return NG;

  pp::VarDictionary msg;
  msg.Set("command", "cd_play");
  msg.Set("track", trk);
  instance->PostMessage(msg);
  return OK;
}

int cdrom_stop() {
  pp::Instance* instance = getXSystem35Instance();
  if (!instance)
    return NG;

  pp::VarDictionary msg;
  msg.Set("command", "cd_stop");
  instance->PostMessage(msg);
  return OK;
}

int cdrom_getPlayingInfo (cd_time *info) {
  info->t = info->m = info->s = info->f = 999;
  return NG;
}
