/*
 * s39ain.h  System39.ain read
 *
 * Copyright (C) 1997-1998 Masaki Chikama (Wren) <chikama@kasumi.ipl.mech.nagoya-u.ac.jp>
 *               1998-                           <masaki-c@is.aist-nara.ac.jp>
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
/* $Id: s39ain.h,v 1.1 2001/09/16 15:59:11 chikama Exp $ */

#ifndef __S39AIN_H__
#define __S39AIN_H__

// #include <ltdl.h>
#include "portab.h"

/* DLL ��ؿ����� */
typedef struct {
	char *name; /* �ؿ�̾ */
	int   argc; /* �ؿ��ΰ����ο� */
	int  *argv; /* �ؿ��ΰ����Τ��줾��μ��� */
} S39AIN_DLLFN;

/* DLL ���� */
typedef struct {
  // lt_dlhandle *handle;       /* DLL handler */
	char        *name;         /* DLL ̾      */
	int          function_num; /* �ؿ��ο�    */
	S39AIN_DLLFN       *function; /* �ؿ����� */
} S39AIN_DLLINF;

/* ���ʥꥪ�ؿ����� */
typedef struct {
	char *name; /* ���ʥꥪ�ؿ�̾ */
	int page;   /* ���ʥꥪ��ΰ��� (�ڡ����ֹ�) */
	int index;  /* ���ʥꥪ��ΰ��� (���ɥ쥹)   */
} S39AIN_FUNCNAME;

/* System39.ain ���Τξ��� */
typedef struct {
	char *path_to_ain; /* system39.ain �ؤΥѥ�  */
	char *path_to_dll; /* DLL �⥸�塼��ؤΥѥ� */
	
	int   dllnum; /* DLL  �ο� */
	int   fncnum; /* FUNC �ο� */
	int   varnum; /* VARI �ο� */
	int   msgnum; /* MSGI �ο� */
	
	S39AIN_DLLINF   *dll; /* DLL  �˴ؿ������ */
	S39AIN_FUNCNAME *fnc; /* FUNC �˴ؤ������ */
	char **var;           /* VARI �˴ؤ������ */
	char **msg;           /* MSGI �˴ؤ������ */
} S39AIN;

extern int s39ain_init(void);

#endif /* __S39AIN_H__ */
