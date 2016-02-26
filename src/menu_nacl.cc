/*
 * menu_nacl.cc  menu implementation for NaCl
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
#include "config.h"

#include <string>
#include <stdio.h>
#include <ppapi/cpp/var.h>
#include <ppapi/cpp/var_array_buffer.h>
#include <ppapi/cpp/var_dictionary.h>
#include "naclmsg.h"

extern "C" {
#include "portab.h"
#include "system.h"
#include "menu.h"
}

void menu_open(void) {
	return;
}

void menu_quitmenu_open(void) {
	return;
}

pp::VarArrayBuffer toArrayBuffer(const char* str) {
	int len = strlen(str);
	pp::VarArrayBuffer var(len);
	void* data = var.Map();
	memcpy(data, str, len);
	var.Unmap();
	return var;
}

std::string fromArrayBuffer(pp::VarArrayBuffer var) {
	void* data = var.Map();
	std::string str(static_cast<const char*>(data), var.ByteLength());
	var.Unmap();
	return str;
}

boolean menu_inputstring(INPUTSTRING_PARAM *p) {
	static std::string result_string;
	p->newstring = p->oldstring;

	if (!g_naclMsg)
		return TRUE;

	pp::VarDictionary msg;
	msg.Set("command", "input_string");
	msg.Set("title", toArrayBuffer(p->title));
	msg.Set("oldstring", toArrayBuffer(p->oldstring));
	msg.Set("maxlen", p->max);

	pp::Var result = g_naclMsg->SendMessage(msg);
	if (!result.is_array_buffer())
		return FALSE;

	result_string = fromArrayBuffer(pp::VarArrayBuffer(result));
	p->newstring = const_cast<char*>(result_string.c_str());  // TODO: Is this safe?

	return TRUE;
}

boolean menu_inputstring2(INPUTSTRING_PARAM *p) {
	p->newstring = p->oldstring;
	NOTICE("menu_inputstring2\n");
	return TRUE;
}

boolean menu_inputnumber(INPUTNUM_PARAM *p) {
	p->value = p->def;

	if (!g_naclMsg)
		return TRUE;

	pp::VarDictionary msg;
	msg.Set("command", "input_number");
	msg.Set("title", toArrayBuffer(p->title));
	msg.Set("max", p->max);
	msg.Set("min", p->min);
	msg.Set("default", p->def);

	pp::Var result = g_naclMsg->SendMessage(msg);
	if (!result.is_number())
		return FALSE;

	p->value = result.AsInt();
	if (p->value < p->min)
		p->value = p->min;
	else if (p->value > p->max)
		p->value = p->max;
	return TRUE;
}

void menu_msgbox_open(char *msg) {
	NOTICE("menu_msgbox_open\n");
	return;
}

void menu_init(void) {
	return;
}

void menu_widget_reinit(boolean reset_colortmap) {
	return;
}

void menu_gtkmainiteration() {
	return;
}
