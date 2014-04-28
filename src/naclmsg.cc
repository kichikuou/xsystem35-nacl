#include <stdio.h>
#include <ppapi/cpp/instance.h>
#include <ppapi/cpp/var.h>
#include <ppapi/cpp/var_dictionary.h>

#include "naclmsg.h"

NaclMsg* g_naclMsg;

NaclMsg::NaclMsg(pp::Instance* instance)
  : instance_(instance)
  , next_request_id_(0)
  , result_id_(-1)
  , result_(pp::Var::Null()) {
  pthread_mutex_init(&lock_, NULL);
  pthread_cond_init(&cond_, NULL);
}

NaclMsg::~NaclMsg() {
  pthread_mutex_destroy(&lock_);
  pthread_cond_destroy(&cond_);
}

void NaclMsg::PostMessage(const pp::Var& msg) {
  instance_->PostMessage(msg);
}

pp::Var NaclMsg::SendMessage(pp::VarDictionary& msg) {
  int id = next_request_id_++;
  msg.Set("naclmsg_id", id);
  fprintf(stderr, "naclmsg: send %d\n", id);
  instance_->PostMessage(msg);

  pthread_mutex_lock(&lock_);
  while (result_id_ != id)
    pthread_cond_wait(&cond_, &lock_);
  pp::Var r = result_;
  result_ = pp::Var(pp::Var::Null());
  pthread_mutex_unlock(&lock_);
  return r;
}

void NaclMsg::HandleMessage(const pp::Var& msg) {
  if (!msg.is_dictionary()) {
    fprintf(stderr, "unexpected message\n");
    return;
  }
  pp::VarDictionary dict(msg);
  pp::Var msgid = dict.Get("naclmsg_id");
  if (msgid.is_undefined() || !msgid.is_int()) {
    fprintf(stderr, "unexpected naclmsg_id type\n");
    return;
  }
  fprintf(stderr, "naclmsg: recv %d\n", msgid.AsInt());
  pthread_mutex_lock(&lock_);
  result_id_ = msgid.AsInt();
  result_ = dict.Get("result");
  pthread_mutex_unlock(&lock_);
  pthread_cond_signal(&cond_);
}
