#include <stdio.h>
#include <unistd.h>
#include <ppapi/cpp/var.h>
#include <ppapi/cpp/var_array_buffer.h>
#include <ppapi/cpp/var_dictionary.h>
#include <ppapi_simple/ps.h>
#include <ppapi_simple/ps_instance.h>
#include <ppapi_simple/ps_interface.h>

#include "naclmsg.h"

NaclMsg* g_naclMsg;

static void MessageHandler(PP_Var key,
						   PP_Var value,
						   void* user_data) {
	((NaclMsg*)user_data)->HandleMessage(pp::Var(value));
}

NaclMsg::NaclMsg()
	: next_request_id_(0)
	, result_id_(-1)
	, result_(pp::Var::Null()) {
	pthread_mutex_init(&lock_, NULL);
	pthread_cond_init(&cond_, NULL);
	PSEventRegisterMessageHandler("naclmsg", MessageHandler, this);
}

NaclMsg::~NaclMsg() {
	pthread_mutex_destroy(&lock_);
	pthread_cond_destroy(&cond_);
}

void NaclMsg::PostMessage(const pp::Var& msg) {
	PSInterfaceMessaging()->PostMessage(PSGetInstanceId(), msg.pp_var());
}

pp::Var NaclMsg::SendMessage(pp::VarDictionary& msg) {
	int id = next_request_id_++;
	msg.Set("naclmsg_id", id);
	// fprintf(stderr, "naclmsg: send %d\n", id);
	PSInterfaceMessaging()->PostMessage(PSGetInstanceId(), msg.pp_var());

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
	// fprintf(stderr, "naclmsg: recv %d\n", msgid.AsInt());
	pthread_mutex_lock(&lock_);
	result_id_ = msgid.AsInt();
	result_ = dict.Get("result");
	pthread_mutex_unlock(&lock_);
	pthread_cond_signal(&cond_);
}

void naclmsg_exit(int code) {
	if (!g_naclMsg)
		return;
	pp::VarDictionary msg;
	msg.Set("command", "exit");
	msg.Set("code", code);
	g_naclMsg->PostMessage(msg);
	for (;;)
		usleep(1000*1000);
}

void naclmsg_setWindowSize(int width, int height) {
	if (!g_naclMsg)
		return;
	pp::VarDictionary msg;
	msg.Set("command", "set_window_size");
	msg.Set("width", width);
	msg.Set("height", height);
	g_naclMsg->PostMessage(msg);
}

void naclmsg_setCaption(char *name) {
	if (!g_naclMsg)
		return;

	int len = strlen(name);
	pp::VarArrayBuffer buf(len);
	void* buf_data = buf.Map();
	memcpy(buf_data, name, len);
	buf.Unmap();
	pp::VarDictionary msg;

	msg.Set("command", "set_caption");
	msg.Set("caption", buf);
	g_naclMsg->PostMessage(msg);
}

struct tm *naclmsg_localtime(const time_t *timep) {
	if (!timep)
		return NULL;

	pp::VarDictionary msg;
	msg.Set("command", "localtime");
	msg.Set("time", pp::Var(static_cast<double>(*timep)));

	pp::Var result = g_naclMsg->SendMessage(msg);
	if (!result.is_array())
		return NULL;
	pp::VarArray array(result);
	static struct tm ltime;
	ltime.tm_sec = array.Get(0).AsInt();
	ltime.tm_min = array.Get(1).AsInt();
	ltime.tm_hour = array.Get(2).AsInt();
	ltime.tm_mday = array.Get(3).AsInt();
	ltime.tm_mon = array.Get(4).AsInt();
	ltime.tm_year = array.Get(5).AsInt();
	ltime.tm_wday = array.Get(6).AsInt();

	return &ltime;
}
