/*
 * xsystem35.c  SYSTEM35 �ǥ�����
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
/* $Id: xsystem35.c,v 1.77 2003/11/16 15:29:52 chikama Exp $ */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <dirent.h>
#include <ctype.h>

#ifdef ENABLE_GTK
#  define GTK_RC_NAME ".gtk/gtkrc"
#  include <gtk/gtk.h>
#endif

#ifdef HAVE_MKDTEMP
#include <stdlib.h>
#else
extern char *mkdtemp (char *template);
#endif


#include "nact.h"
#include "portab.h"
#include "xsystem35.h"
#include "nact.h"
#include "profile.h"
#include "randMT.h"
#include "counter.h"
#include "ags.h"
#include "graphicsdevice.h"
#include "menu.h"
#include "music.h"
#include "music_client.h"
#include "savedata.h"
#include "scenario.h"
#include "variable.h"
#include "dri.h"
#include "ald_manager.h"
#include "filecheck.h"
#include "joystick.h"
#include "s39init.h"

#ifdef ENABLE_MMX
#include "haveunit.h"
#endif

static char *gameResorceFile = "xsystem35.gr";
static void    sys35_usage(boolean verbose);
static void    sys35_init();
static void    sys35_remove();
static boolean sys35_initGameDataResorce();
static void    sys35_ParseOption(int *argc, char **argv);
static void    signal_handler(int sig_num);
static void    signal_handler_segv(int sig_num);
static void    check_profile();
static void    init_signalhandler();

/* for debugging */
static FILE *fpdebuglog;
static int debuglv = DEBUGLEVEL;
int sys_nextdebuglv;

/* game data file name */
static char *gamefname[DRIFILETYPEMAX][DRIFILEMAX];

/* font name from rcfile */
static char *fontname[FONTTYPEMAX];
static char *fontname_tt[FONTTYPEMAX];
static boolean isjix0213_tt[FONTTYPEMAX];
static char fontface[FONTTYPEMAX];

#ifdef ENABLE_EDL
static fontdev_t fontdev = FONT_GTK;
#else
static fontdev_t fontdev = FONT_X11;
#endif

/* antialias on from command line */
static boolean font_antialias;
static boolean font_noantialias;

/* fullscreen on from command line */
static boolean fs_on;

// for reboot
static int saved_argc;
static char **saved_argv;

static void sys35_usage(boolean verbose) {
	if (verbose) {
		puts("System35 for X Window System [proj. Rainy Moon]");
		puts("             (C) Masaki Chikama(Wren) 1997-2002");
		puts("                                  Version "VERSION"\n");
	}
	puts("Usage: xsystem35 [OPTIONS]\n");
	puts("OPTIONS");
	puts(" -gamefile file : set game resouce file to 'file'");
	puts(" -no-shm        : don't use MIT-SHM (use in another display)");
	puts(" -devcd device  : set cdrom device name to 'device'");
	puts(" -devmidi device: set midi device name to 'device'");
	puts(" -devdsp device : set audio device name to 'device'");
	
#ifdef ENABLE_OSS
	puts("                :  /dev/dsp : oss type (device name)");
#endif
#ifdef ENABLE_ALSA
#ifdef ENABLE_ALSA05
	puts("                :  0:0      : alsa type (card and device)");
#endif
#ifdef ENABLE_ALSA09
	puts("                :  hw:0,0   : alsa hardware");
#endif
#endif
	
	puts(" -O?            : select output audio device");
#ifdef ENABLE_OSS
	puts(" -Oo            : OSS mode");
#endif
#ifdef ENABLE_ALSA
	puts(" -Os            : ALSA mode");
#endif
#ifdef ENABLE_ESD
	puts(" -Oe            : Enlightened Sound Daemon mode");
#endif
	puts(" -O0            : Disable Audio output");
	
	puts(" -M?            : select output midi methos");
#ifdef ENABLE_MIDI_EXTPLAYER
	puts(" -Me            : External midi player");
#endif
#ifdef ENABLE_MIDI_RAWMIDI
	puts(" -Mr            : Raw Midi device");
#endif
#ifdef ENABLE_MIDI_SEQMIDI
	puts(" -Ms?           : Sequenceer device (?:devicenumber)");
#endif
	puts(" -M0            : Disable MIDI output");
	puts(" -midiplayer cmd: set external midi player to 'cmd'");
	
	puts(" -devmix device : set mixer device name to 'device' (effective only for oss)");
	puts(" -devjoy device : set joystic device name to 'device'");
	puts("                    if 'device' is set to 'none', don't use the device");
	puts(" -savekanji #   : kanji code of filename (0 or 1 ... 0:euc, 1:sjis)");
#ifdef DEBUG

	puts(" -devfont device: select font device");
#if defined(ENABLE_TTF) || defined(ENABLE_FT2)
	puts(" -devfont ttf   : FreerType (True Type Font)");
#endif
#ifdef ENABLE_X11FONT
	puts(" -devfont x11   : x11");
#endif
#ifdef ENABLE_GTKFONT
	puts(" -devfont gtk   : gtk");
#endif

#ifdef ENABLE_EDL
#ifdef ENABLE_GTK
	puts("                : default is gtk");
#else
	puts("                : default is ttf");
#endif
#else
	puts("                : default is x11");
#endif

#if defined(ENABLE_TTF) || defined(ENABLE_SDLTTF) || defined(ENABLE_FT2)
	puts(" -ttfont_mincho: set TrueType font for mincho");
	puts(" -ttfont_gothic: set TrueType font for mincho");
#endif
	puts(" -font_mincho  : set X11(gtk) font for mincho");
	puts(" -font_gothic  : set X11(gtk) font for mincho");
	
	puts(" -debuglv #     : debug level");
	puts("                :  0: critical error message only ");
	puts("                :  1: + waring message");
	puts("                :  2: + not implemented command message");
	puts("                :  5: + implemented command (write to logfile)");
	puts("                :  6: + message (write to logfile)");
#endif  
	puts(" -antialias     : always draw antialiased string (for !256 colors game)");
	puts(" -noantialias   : nevser use antialiased string (for !256 colors game)");
	puts(" -fullscreen    : start with fullscreen");
	puts(" -noimagecursor : disable image cursor");
	puts(" -version       : show version");
	puts(" -h             : show this message");
	puts(" --help         : show this message");
	exit(1);
}

void sys_message(const char *format, ...) {
	va_list args;
	
	va_start(args, format);
#ifdef DEBUG
	if (debuglv >= sys_nextdebuglv) {
		if (sys_nextdebuglv >= 5) {
			vfprintf(fpdebuglog, format, args);
			fflush(fpdebuglog);
		} else {
			vfprintf(stderr, format, args);
		}
	}
#else
	if (debuglv >= sys_nextdebuglv) {
		vfprintf(stderr, format, args);
	}
#endif
	va_end(args);

}

void sys_error(const char *format, ...) {
	va_list args;
	
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	sys35_remove();
	exit(1);
}

void sys_exit(int code) {
	sys35_remove();
	exit(code);
}

static int check_fontdev(char *devname) {
#ifdef ENABLE_TTF
	if (0 == strcmp(devname, "ttf")) {
		return FONT_TTF;
	}
#endif

#ifdef ENABLE_FT2
	if (0 == strcmp(devname, "ft2")) {
		return FONT_FT2;
	}
	if (0 == strcmp(devname, "ttf")) {
		return FONT_FT2;
	}
#endif

#ifdef ENABLE_X11FONT
	if (0 == strcmp(devname, "x11")) {
		return FONT_X11;
	}
#endif

#ifdef ENABLE_GTKFONT
	if (0 == strcmp(devname, "gtk")) {
		return FONT_GTK;
	}
#endif
	
#ifdef ENABLE_SDL
#ifdef ENABLE_GTK
	return FONT_GTK;
#else
	return FONT_TTF;
#endif
#else
	return FONT_X11;
#endif
}

static void storeDataName(int type, int no, char *src) {
	gamefname[type][no] = strdup(src);
}

static void storeSaveName(int no, char *src) {
	char *home_dir = "";
	char *path = NULL;
	
	if (*src == '~') {
		home_dir = getenv("HOME");
		if (NULL == (path = malloc(strlen(home_dir) + strlen(src) + 1))) {
			NOMEMERR();
		}
		sprintf(path, "%s%s", home_dir, src+1);
		save_register_file(path, no);
		src = path;
	} else {
		save_register_file(src, no);
	}
	
	if (no == 0) {
		char *b = strrchr(src, '/');
		if (b == NULL) {
			SYSERROR("Illigal save filename %s\n", src);
		}
		*b = '\0';
		save_set_path(src);
	}
	if (path) free(path);
}

/* ������Υǡ����ե�����ξ����ǥ��쥯�ȥ꤫����� thanx tajiri@wizard*/
static boolean sys35_initGameDataDir(int* cnt)
{
    DIR* dir;
    struct dirent* d;
    char s1[256], s2[256];
    int dno;
    int i;
    
    getcwd(s1,255);
    if(NULL == (dir= opendir(".")))
    {
        SYSERROR("Game Resouce File open failed\n");
    }
    while(NULL != (d = readdir(dir))){
        char *filename = d->d_name;
        int len = strlen(filename);
        sprintf(s2,"%s%c%s",s1,'/',filename);
        if(strcasecmp(filename,"adisk.ald")==0){
            storeDataName(DRIFILE_SCO, 0, s2);
            cnt[0] = max(1, cnt[0]);
#if 0
        } else if (strcasecmp(filename, "system39.ain") == 0) {
		nact->ain.path_to_ain = strdup(filename);
#endif
	}
        else if(strcasecmp(filename+len-4,".ald")==0){
            dno = toupper(*(filename+len-5)) - 'A';
            if (dno < 0 || dno >= DRIFILEMAX) continue;
            switch(*(filename+len-6)){
                case 'S':
                case 's':
                    storeDataName(DRIFILE_SCO, dno, s2);
                    cnt[0] = max(dno + 1, cnt[0]);
                    break;
                case 'g':
                case 'G':
                    storeDataName(DRIFILE_CG, dno, s2);
                    cnt[1] = max(dno + 1, cnt[1]);
                    break;
                case 'W':
                case 'w':
                    storeDataName(DRIFILE_WAVE, dno, s2);
                    cnt[2] = max(dno + 1, cnt[2]);
                    break;
                case 'M':
                case 'm':
                    storeDataName(DRIFILE_MIDI, dno, s2);
                    cnt[3] = max(dno + 1, cnt[3]);
                    break;
                case 'D':
                case 'd':
                    storeDataName(DRIFILE_DATA, dno, s2);
                    cnt[4] = max(dno + 1, cnt[4]);
                    break;
                case 'R':
                case 'r':
                    storeDataName(DRIFILE_RSC, dno, s2);
                    cnt[5] = max(dno + 1, cnt[5]);
                    break;
                    
            }
        }
    }
    for(i=0;i<SAVE_MAXNUMBER;i++){
        sprintf(s2,"%s/%c%s",s1,'a'+i,"sleep.asd");
        storeSaveName(i, s2);
    }

    if(cnt[0]>0)
        return TRUE;
    else
        return FALSE;
}

    
/* ������Υǡ����ե�����ξ�����ɤ߹��� */
static boolean sys35_initGameDataResorce() {
	int cnt[] = {0, 0, 0, 0, 0, 0, 0};
	int linecnt = 0, dno;
	FILE *fp;
	char s[256];
	char s1[256], s2[256];
	
	if (NULL == (fp = fopen(gameResorceFile, "r"))) {
		if(sys35_initGameDataDir(cnt)==TRUE) {
			goto initdata;
		}
		sys35_usage(TRUE);
	}
	while(fgets(s, 255, fp) != NULL ) {
		linecnt++;
		if (s[0] == '#') continue;
		s2[0] = '\0';
		sscanf(s,"%s %s", s1, s2);
		if (s2[0] == '\0') continue;
		if (0 == strncmp(s1, "Scenario", 8)) {
			dno = s1[8] - 'A';
			if (dno < 0 || dno >= DRIFILEMAX) goto errexit;
			storeDataName(DRIFILE_SCO, dno, s2);
			cnt[0] = max(dno + 1, cnt[0]);
		} else if (0 == strncmp(s1, "Graphics", 8)) {
			dno = s1[8] - 'A';
			if (dno < 0 || dno >= DRIFILEMAX) goto errexit;
			storeDataName(DRIFILE_CG, dno, s2);
			cnt[1] = max(dno + 1, cnt[1]);
		} else if (0 == strncmp(s1, "Wave", 4)) {
			dno = s1[4] - 'A';
			if (dno < 0 || dno >= DRIFILEMAX) goto errexit;
			storeDataName(DRIFILE_WAVE, dno, s2);
			cnt[2] = max(dno + 1, cnt[2]);
		} else if (0 == strncmp(s1, "Midi", 4)) {
			dno = s1[4] - 'A';
			if (dno < 0 || dno >= DRIFILEMAX) goto errexit;
			storeDataName(DRIFILE_MIDI, dno, s2);
			cnt[3] = max(dno + 1, cnt[3]);
		} else if (0 == strncmp(s1, "Data", 4)) {
			dno = s1[4] - 'A';
			if (dno < 0 || dno >= DRIFILEMAX) goto errexit;
			storeDataName(DRIFILE_DATA, dno, s2);
			cnt[4] = max(dno + 1, cnt[4]);
		} else if (0 == strncmp(s1, "Resource", 8)) {
			dno = s1[8] - 'A';
			if (dno < 0 || dno >= DRIFILEMAX) goto errexit;
			storeDataName(DRIFILE_RSC, dno, s2);
			cnt[5] = max(dno + 1, cnt[5]);
		} else if (0 == strncmp(s1, "BGM", 3)) {
			dno = s1[3] - 'A';
			if (dno < 0 || dno >= DRIFILEMAX) goto errexit;
			storeDataName(DRIFILE_BGM, dno, s2);
			cnt[6] = max(dno + 1, cnt[6]);
		} else if (0 == strncmp(s1, "Save", 4)) {
			dno = s1[4] - 'A';
			if (dno < 0 || dno >= DRIFILEMAX) goto errexit;
			storeSaveName(dno, s2);
#if 0
		} else if (0 == strncmp(s1, "Ain", 3)) {
			nact->ain.path_to_ain = strdup(s2);
#endif
		} else if (0 == strncmp(s1, "WAIA", 4)) {
			nact->files.wai = strdup(s2);
		} else if (0 == strncmp(s1, "BGIA", 4)) {
			nact->files.bgi = strdup(s2);
		} else if (0 == strncmp(s1, "SACT01", 6)) {
			nact->files.sact01 = strdup(s2);
		} else if (0 == strncmp(s1, "Init", 4)) {
			nact->files.init = strdup(s2);
		} else if (0 == strncmp(s1, "ALK", 3)) {
			dno = s1[4] - '0';
			if (dno < 0 || dno >= 10) goto errexit;
			nact->files.alk[dno] = strdup(s2);
		} else {
			goto errexit;
		}
	}
	fclose(fp);
initdata:
	if (cnt[0] == 0) {
		SYSERROR("No Scenario data available\n");
	}
	
	if (cnt[0] > 0)
		ald_init(DRIFILE_SCO, gamefname[DRIFILE_SCO], cnt[0], TRUE);
	if (cnt[1] > 0)
		ald_init(DRIFILE_CG,  gamefname[DRIFILE_CG], cnt[1], TRUE);
	if (cnt[2] > 0)
		ald_init(DRIFILE_WAVE, gamefname[DRIFILE_WAVE], cnt[2], TRUE);
	if (cnt[3] > 0)
		ald_init(DRIFILE_MIDI, gamefname[DRIFILE_MIDI], cnt[3], TRUE);
	if (cnt[4] > 0)
		ald_init(DRIFILE_DATA, gamefname[DRIFILE_DATA], cnt[4], TRUE);
	if (cnt[5] > 0)
		ald_init(DRIFILE_RSC, gamefname[DRIFILE_RSC], cnt[5], TRUE);
	if (cnt[6] > 0)
		ald_init(DRIFILE_BGM, gamefname[DRIFILE_BGM], cnt[6], TRUE);

	return 0;

 errexit:
	SYSERROR("Illigal resouce at line(%d) file<%s>\n",linecnt, gameResorceFile);
	return 0;
}

static void sys35_init() {
	int i;
	
	nact_init();
	
	sl_init();

	v_initVars();
	
	nact->fontdev = fontdev;
	
	ags_init();

	for (i = 0; i < FONTTYPEMAX; i++) {
		switch(fontdev) {
		case FONT_TTF:
		case FONT_FT2:
		case FONT_SDLTTF:
			if (fontname_tt[i] == NULL) {
				nact->ags.font->name[i] = strdup(FONT_DEFAULTNAME_TTF);
			} else {
				nact->ags.font->name[i] = fontname_tt[i];
			}
			nact->ags.font->isJISX0213[i] = isjix0213_tt[i];
			nact->ags.font->face[i] = fontface[i];
			break;
			
		case FONT_X11:
		case FONT_GTK:
			if (fontname[i] == NULL) {
				nact->ags.font->name[i] = strdup(FONT_DEFAULTNAME_X);
			} else {
				nact->ags.font->name[i] = fontname[i];
			}
			break;
		default:
			break;
		}
	}
	
	ags_fullscreen(fs_on);
	nact->noantialias = font_noantialias;
	ags_setAntialiasedStringMode(font_antialias);
	
	
	reset_counter(0);

	sgenrand(getpid());

#ifdef ENABLE_MMX
	nact->mmx_is_ok = ((haveUNIT() & tMMX) ? TRUE : FALSE);
#endif

	msg_init();
	sel_init();

#if 0
	s39ain_init();
#endif
	s39ini_init();
}

static void sys35_remove() {
	mus_exit(); 
	ags_remove();
	s39ini_remove();
	/* joy_close(); */
#if DEBUG
	if (debuglv >= 3) {
		fclose(fpdebuglog);
	}
#endif
	if (0 != strcmp(nact->tmpdir, "/tmp")) {
		rmdir(nact->tmpdir);
	}
}

void sys_reset() {
	mus_exit();
	ags_remove();
	s39ini_remove();
	
	if (0 != strcmp(nact->tmpdir, "/tmp")) {
		rmdir(nact->tmpdir);
	}
	
	execvp(saved_argv[0], saved_argv);
	sys_error("exec fail");
}

static void signal_handler(int sig_num) {
	sys35_remove();
	exit(1);
}	

static void signal_handler_segv(int sig_num) {
	fprintf(stderr, "PID(%d), sigsegv caught @ %03d, %05x\n", (int)getpid(), sl_getPage(), sl_getIndex());
	sys35_remove();
	exit(1);
}	

static void sys35_ParseOption(int *argc, char **argv) {
	int i;
	FILE *fp;
	for (i = 1; i < *argc; i++) {
		if (!strcmp(argv[i], "-h")) {
			sys35_usage(TRUE);
		}
		if (!strcmp(argv[i], "--help")) {
			sys35_usage(TRUE);
		}
	}
	for (i = 1; i < *argc; i++) {
		if (0 == strcmp(argv[i], "-gamefile")) {
			if (i == *argc - 1) {
				fprintf(stderr, "xsystem35: The -gamefile option requires file value\n\n");
				sys35_usage(FALSE);
			}
			if (NULL == (fp = fopen(argv[i + 1],"r"))) {
				fprintf(stderr, "xsystem35: gamefile '%s' not found\n\n", argv[i + 1]);
				sys35_usage(FALSE);
			}
			fclose(fp);
			gameResorceFile = argv[i + 1];
		} else if (0 == strcmp(argv[i], "-no-shm")) {
			SetNoShmMode();
		} else if (0 == strcmp(argv[i], "-devcd")) {
			if (argv[i + 1] != NULL) {
				cd_set_devicename(argv[i + 1]);
			}
		} else if (0 == strcmp(argv[i], "-devmidi")) {
			if (argv[i + 1] != NULL) {
				midi_set_devicename(argv[i + 1]);
			}
		} else if (0 == strcmp(argv[i], "-midiplayer")) {
			if (argv[i + 1] != NULL) {
				midi_set_playername(argv[i + 1]);
			}
		} else if (0 == strncmp(argv[i], "-M", 2)) {
			int subdev = 0;
			if (argv[i][3] != '\0') {
				subdev = (argv[i][3] - '0') << 8;
			}
			midi_set_output_device(argv[i][2] | subdev);
		} else if (0 == strcmp(argv[i], "-devdsp")) {
			if (argv[i + 1] != NULL) {
				audio_set_pcm_devicename(argv[i + 1]);
			}
		} else if (0 == strncmp(argv[i], "-O", 2)) {
			audio_set_output_device(argv[i][2]);
		} else if (0 == strcmp(argv[i], "-devmix")) {
			if (argv[i + 1] != NULL) {
				audio_set_mixer_devicename(argv[i + 1]);
			}
		} else if (0 == strcmp(argv[i], "-devjoy")) {
			if (argv[i + 1] != NULL) {
				joy_set_devicename(argv[i + 1]);
			}
		} else if (0 == strcmp(argv[i], "-savekanji")) {
			if (argv[i + 1] != NULL) {
				fc_set_default_kanjicode(argv[i + 1][0] - '0');
			}
		} else if (0 == strcmp(argv[i], "-fullscreen")) {
			fs_on = TRUE;
		} else if (0 == strcmp(argv[i], "-antialias")) {
			font_antialias = TRUE;
		} else if (0 == strcmp(argv[i], "-noantialias")) {
			font_noantialias = TRUE;
		} else if (0 == strcmp(argv[i], "-devfont")) {
			if (argv[i + 1] != NULL) {
				fontdev = check_fontdev(argv[i + 1]);
			}
		} else if (0 == strcmp(argv[i], "-font_gothic")) {
			if (argv[i + 1] != NULL) {
				fontname[FONT_GOTHIC] = argv[i + 1];
			}
		} else if (0 == strcmp(argv[i], "-font_mincho")) {
			if (argv[i + 1] != NULL) {
				fontname[FONT_MINCHO] = argv[i + 1];
			}
		} else if (0 == strcmp(argv[i], "-ttfont_gothic")) {
			if (argv[i + 1] != NULL) {
				fontname_tt[FONT_GOTHIC] = argv[i + 1];
			}
		} else if (0 == strcmp(argv[i], "-ttfont_mincho")) {
			if (argv[i + 1] != NULL) {
				fontname_tt[FONT_MINCHO] = argv[i + 1];
			}
		} else if (0 == strcmp(argv[i], "-noimagecursor")) {
			nact->noimagecursor = TRUE;
		} else if (0 == strcmp(argv[i], "-debuglv")) {
			if (argv[i + 1] != NULL) {
				debuglv = argv[i + 1][0] - '0';
			}
		} else if (0 == strcmp(argv[i], "-version")) {
			puts(VERSION);
			exit(0);
		}
	}
}

static void check_profile() {
	char *param;
	
	/* �ե���ȥǥХ��������� */
	param = get_profile("font_device");
	if (param) {
		fontdev = check_fontdev(param);
	}
	/* �����å��ե���Ȥ����� */
	param = get_profile("font_gothic");
	if (param) {
		fontname[FONT_GOTHIC] = param;
	}
	/* ��ī�ե���Ȥ����� */
	param = get_profile("font_mincho");
	if (param) {
		fontname[FONT_MINCHO] = param;
	}
	/* �����å��ե����(TT)������ */
	param = get_profile("ttfont_gothic");
	if (param) {
		fontname_tt[FONT_GOTHIC] = param;
	}
	/* ��ī�ե����(TT)������ */
	param = get_profile("ttfont_mincho");
	if (param) {
		fontname_tt[FONT_MINCHO] = param;
	}
	/* �����å��ե����(TT)�Υ��������� */
	param = get_profile("ttfont_gothic_code");
	if (param) {
		isjix0213_tt[FONT_GOTHIC] = (strcmp("jisx0213",param) == 0 ? TRUE : FALSE);
	}
	/* ��ī�ե����(TT)�Υ��������� */
	param = get_profile("ttfont_mincho_code");
	if (param) {
		isjix0213_tt[FONT_MINCHO] = (strcmp("jisx0213",param) == 0 ? TRUE : FALSE);
	}
	/* �����å��ե����(TT)�Υե��������� */
	param = get_profile("ttfont_gothic_face");
	if (param) {
		fontface[FONT_GOTHIC] = *param - '0';
	}
	/* ��ī�ե����(TT)�Υե��������� */
	param = get_profile("ttfont_mincho_face");
	if (param) {
		fontface[FONT_MINCHO] = *param - '0';
	}
	/* CD-ROM device name ������ */
	param = get_profile("cdrom_device");
	if (param) {
		cd_set_devicename(param);
	}
	/* DSP device name ������ */
	param = get_profile("dsp_device");
	if (param) {
		audio_set_pcm_devicename(param);
	}
	/* mixer device name ������ */
	param = get_profile("mixer_device");
	if (param) {
		audio_set_mixer_devicename(param);
	}
	/* audio output device ������ */
	param = get_profile("audio_output_device");
	if (param) {
		audio_set_output_device(*param);
	}
	/* joystick device name ������ */
	param = get_profile("joy_device");
	if (param) {
		joy_set_devicename(param);
	}
	/* ����MIDI�ץ졼�䡼������ */
	param = get_profile("midi_player");
	if (param) {
		midi_set_playername(param);
	}
	/* Raw MIDI device name ������ */
	param = get_profile("midi_device");
	if (param) {
		midi_set_devicename(param);
	}
	/* MIDI output device ������ */
	param = get_profile("midi_output_device");
	if (param) {
		int subdev = 0;
		if (*(param+1) != '\0') {
			subdev = (*(param + 1) - '0') << 8;
		}
		midi_set_output_device(*param | subdev);
	}
	/* no-shm flag */
	param = get_profile("no_shm");
	if (param) {
		if (0 == strcmp(param, "Yes")) {
			SetNoShmMode();
		}
	}
	/* qe-kanjicode flag */
	param = get_profile("qe-kanjicode");
	if (param) {
		fc_set_default_kanjicode(*param - '0');
	}
	/* disable image cursor */
	param = get_profile("no_imagecursor");
	if (param) {
		if (0 == strcmp(param, "Yes")) {
			nact->noimagecursor = TRUE;
		}
	}
}

void sys_set_signalhandler(int SIG, void (*handler)(int)) {
	struct sigaction act;
	sigset_t smask;
	
	sigemptyset(&smask);
	sigaddset(&smask, SIG);
	
	act.sa_handler = handler;
	act.sa_mask = smask;
	act.sa_flags = 0;
	
	sigaction(SIG, &act, NULL);
}

void init_signalhandler() {
	sys_set_signalhandler(SIGINT, signal_handler);
	sys_set_signalhandler(SIGTERM, signal_handler);
	sys_set_signalhandler(SIGPIPE, SIG_IGN);
	sys_set_signalhandler(SIGABRT, signal_handler);
	sys_set_signalhandler(SIGSEGV, signal_handler_segv);
}

int main(int argc, char **argv) {
	char *homedir = getenv("HOME"), *rc_name, *rc_path;
	sys_set_signalhandler(SIGINT, SIG_IGN);
	
	saved_argc = argc;
	saved_argv = argv;
	
	load_profile(NULL);
	check_profile();
	sys35_ParseOption(&argc, argv);
	
#if DEBUG
	if (debuglv >= 5) {
		if (NULL == (fpdebuglog = fopen(DEBUGLOGFILE, "w"))) {
			fpdebuglog = stderr;
		}
	}
#endif
	sys35_initGameDataResorce();
	
	init_signalhandler();

	nact->tmpdir = strdup("/tmp/xsystem35-XXXXXX");
	if (NULL == mkdtemp(nact->tmpdir)) {
		free(nact->tmpdir);
		nact->tmpdir = strdup("/tmp");
	}
	
	mus_init();

#ifdef ENABLE_NLS
        bindtextdomain (PACKAGE, LOCALEDIR);
        textdomain(PACKAGE);
#endif

#ifdef ENABLE_GTK
	gtk_set_locale();
	gtk_init(&argc, &argv);
#endif

	sys35_init();	
	
#ifdef ENABLE_GTK
	rc_name = get_profile("gtkrc_path");
	if (!rc_name) {
		rc_name = GTK_RC_NAME;
	}
	rc_path = (char *)g_malloc(sizeof(char) * (strlen(homedir) + strlen(rc_name)) + 2);
	strcpy(rc_path, homedir);
	strcat(rc_path, "/");
	strcat(rc_path, rc_name);
	
	gtk_rc_parse(rc_path);
	g_free(rc_path);
	menu_init();
#endif
	
	nact_main();
	sys35_remove();
	
	return 0;
}
