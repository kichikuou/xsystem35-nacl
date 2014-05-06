#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/mount.h>

#include <SDL/SDL_main.h>
#include <SDL/SDL_nacl.h>
#include <nacl_io/nacl_io.h>
#include <ppapi/cpp/input_event.h>
#include <ppapi/cpp/instance.h>
#include <ppapi/cpp/module.h>
#include <ppapi/cpp/var.h>

#include "naclmsg.h"

static void* sdl_thread_start(void* arg) {
  char* argv[2];
  int rtn;

  fprintf(stderr, "SDL: calling SDL_main\n");
  argv[0] = (char*)"xsystem35";
  argv[1] = NULL;
  rtn = SDL_main(1, argv);
  fprintf(stderr, "SDL: SDL_main returned: %d\n", rtn);
  exit(rtn);
  return NULL;
}

class XSystem35Instance : public pp::Instance {
public:
  explicit XSystem35Instance(PP_Instance instance)
      : pp::Instance(instance) {}
  virtual ~XSystem35Instance() {}

  virtual bool Init(uint32_t argc, const char* argn[], const char* argv[]) {
    RequestInputEvents(PP_INPUTEVENT_CLASS_MOUSE);
    RequestFilteringInputEvents(PP_INPUTEVENT_CLASS_KEYBOARD);

    nacl_io_init_ppapi(pp_instance(), pp::Module::Get()->get_browser_interface());
    umount("/");

    for (int i = 0; i < argc; i++) {
      if (!strcmp(argn[i], "gamedir")) {
        if (mount(argv[i], "/", "httpfs", 0, "") != 0)
          fprintf(stderr, "%s: mount failed: %s\n", argv[i], strerror(errno));
      }
    }

    if (mount("fonts", "fonts", "httpfs", 0, "") != 0)
      fprintf(stderr, "fonts: mount failed: %s\n", strerror(errno));
    if (mount("", "save", "html5fs", 0, "type=TEMPORARY,expected_size=1048576") != 0)
      fprintf(stderr, "save: mount failed: %s\n", strerror(errno));

    return true;
  }

  virtual void DidChangeView(const pp::View& view) {
    pp::Rect view_rect = view.GetRect();
    fprintf(stderr, "DidChangeView: %d x %d\n",
            view_rect.width(), view_rect.height());

    if (view_rect.width() <= 0 || view_rect.height() <= 0)
      return;

    SDL_NACL_SetInstance(pp_instance(),
                         pp::Module::Get()->get_browser_interface(),
                         view_rect.width(), view_rect.height());

    if (sdl_initialized_)
      return;

    pthread_create(&sdl_thread_, NULL, sdl_thread_start, NULL);
    sdl_initialized_ = true;
    g_naclMsg = new NaclMsg(this);
  }

  virtual bool HandleInputEvent(const pp::InputEvent& event) {
    SDL_NACL_PushEvent(event.pp_resource());
    return true;
  }

  virtual void HandleMessage(const pp::Var& message) {
    g_naclMsg->HandleMessage(message);
  }

private:
  pthread_t sdl_thread_;
  bool sdl_initialized_;
};

class XSystem35Module : public pp::Module {
public:
  XSystem35Module() : pp::Module() {}
  virtual ~XSystem35Module() {}

  virtual pp::Instance* CreateInstance(PP_Instance instance) {
    return new XSystem35Instance(instance);
  }
};

namespace pp {

Module* CreateModule() {
  return new XSystem35Module();
}

}  // namespace pp
