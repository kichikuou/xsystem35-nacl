#include "config.h"

#include <stdio.h>
#include <string.h>
#include <ppapi/cpp/instance.h>
#include <ppapi/cpp/var_array_buffer.h>
#include <ppapi/cpp/var_dictionary.h>

#include "naclmsg.h"

extern "C" {
#include "system.h"
#include "counter.h"
}

#include "portab.h"
#include "midi.h"

static int midi_initilize(char *devnm, int subdev);
static int midi_exit();
static int midi_start(int no, char *data, int datalen);
static int midi_stop();
static int midi_pause(void);
static int midi_unpause(void);
static int midi_get_playing_info(midiplaystate *st);
static int midi_getflag(int mode, int index);
static int midi_setflag(int mode, int index, int val);

extern "C" mididevice_t midi_nacl;
mididevice_t midi_nacl = {
	midi_initilize,
	midi_exit,
	midi_start,
	midi_stop,
	midi_pause,
	midi_unpause,
	midi_get_playing_info,
	midi_getflag,
	midi_setflag,
	NULL,
	NULL
};

static int counter;
static int midino;

static int midi_initilize(char *devnm, int subdev) {
	reset_counter_high(SYSTEMCOUNTER_MIDI, 10, 0);
	return OK;
}

static int midi_exit() {
	return OK;
}

static int midi_start(int no, char *data, int datalen) {
	pp::VarArrayBuffer buf(datalen);
	void* buf_data = buf.Map();
	memcpy(buf_data, data, datalen);
	buf.Unmap();

	pp::VarDictionary msg;
	msg.Set("command", "midi_start");
	msg.Set("data", buf);
	g_naclMsg->PostMessage(msg);

	midino = no;
	counter = get_high_counter(SYSTEMCOUNTER_MIDI);
	return OK;
}

static int midi_stop() {
	pp::VarDictionary msg;
	msg.Set("command", "midi_stop");
	g_naclMsg->PostMessage(msg);

	midino  = 0;

	return OK;
}

static int midi_pause(void) {
	NOTICE("midi_pause\n");
	// TODO: implement
	return OK;
}

static int midi_unpause(void) {
	NOTICE("midi_unpause\n");
	// TODO: implement
	return OK;
}

static int midi_get_playing_info(midiplaystate *st) {
	if (midino == 0) {
		st->in_play = FALSE;
		st->play_no = 0;
		st->loc_ms  = 0;
		return OK;
	}

	int cnt = get_high_counter(SYSTEMCOUNTER_MIDI) - counter;

	st->in_play = TRUE;
	st->play_no = midino;
	st->loc_ms = cnt * 10;
	return OK;
}

static int midi_getflag(int mode, int index) {
	NOTICE("midi_getflag\n");
	// TODO: implement
	return 0;
}

static int midi_setflag(int mode, int index, int val) {
	NOTICE("midi_setflag\n");
	// TODO: implement
	return NG;
}
