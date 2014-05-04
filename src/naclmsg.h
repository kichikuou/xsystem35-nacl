#ifndef __NACLMSG_H_
#define __NACLMSG_H_

#ifdef __cplusplus

#include <pthread.h>
#include <ppapi/cpp/var.h>

namespace pp {
  class Instance;
  class VarDictionary;
}

class NaclMsg {
 public:
  NaclMsg(pp::Instance* instance);
  ~NaclMsg();

  pp::Instance* instance() const { return instance_; }

  void PostMessage(const pp::Var&);
  pp::Var SendMessage(pp::VarDictionary&);

  void HandleMessage(const pp::Var&);

 private:
  pp::Instance* instance_;
  pthread_mutex_t lock_;
  pthread_cond_t cond_;
  int next_request_id_;
  int result_id_;
  pp::Var result_;
};

extern NaclMsg* g_naclMsg;

extern "C" {
#endif // __cplusplus

void naclmsg_setWindowSize(int width, int height);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __NACLMSG_H_
