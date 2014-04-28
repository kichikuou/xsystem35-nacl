/* config.h.in.  Generated from configure.ac by autoheader.  */

/* alsa card no */
#undef ALSACARD

/* aduio device */
#undef AUDIODEV_ALSA

/* aduio device */
#undef AUDIODEV_OSS

/* Define this if X11 Pixmap color order is BGR */
#undef BGR_ORDER

/* General Cache size (MB) */
#undef CACHE_TOTALSIZE

/* cdrom device */
#define CDROM_DEVICE "/dev/cdrom"

/* Define this if you want to Debug */
#define DEBUG 1

/* default audio mode */
#undef DEFAULT_AUDIO_MODE

/* Define this if you have Alsa (libasound) installed */
#undef ENABLE_ALSA

/* Define this if you have 0.5.x <= alsa < 0.9.0 installed */
#undef ENABLE_ALSA05

/* Define this if you have alsa 0.9.x and more installed */
#undef ENABLE_ALSA09

/* Define this if you use cdrom iotctl in *BSD */
#undef ENABLE_CDROM_BSD

/* Define this if you use cdrom ctrl library in IRX */
#undef ENABLE_CDROM_IRIX

/* Define this if your use cdrom ioctl in linux or Solaris */
#undef ENABLE_CDROM_LINUX

/* Define this if you use external player for cdrom audio */
#undef ENABLE_CDROM_MP3

/* Define this if you use NaCl for cdrom audio */
#define ENABLE_CDROM_NACL 1

/* Define this if you have ESD (libesd) installed */
#undef ENABLE_ESD

/* Define this if you have freetype2 installed */
#define ENABLE_FT2 1

/* Define this if you have glib >= 1.2.0 */
#undef ENABLE_GLIB

/* Define this if you have gtk+ >= 1.0.0 */
#undef ENABLE_GTK

/* define this if you use gtk(gdk) font interface */
#undef ENABLE_GTKFONT

/* define this if you use external midi player */
#undef ENABLE_MIDI_EXTPLAYER

/* define this if you use oss raw midi interface */
#undef ENABLE_MIDI_RAWMIDI

/* define thsi if you use oss sequencer interface */
#undef ENABLE_MIDI_SEQMIDI

/* define this if you want to use MMX instructions */
#undef ENABLE_MMX

/* Define to 1 if translation of program messages to the user's native
   language is requested. */
#undef ENABLE_NLS

/* define this if you use OSS audio interface */
#undef ENABLE_OSS

/* define this if you use SDL */
#define ENABLE_SDL 1

/* define this if you use sun stlye audio interface */
#undef ENABLE_SUNAUDIO

/* Define this if you have old(1.x) freetype installed */
#undef ENABLE_TTF

/* define this if you use X11 font interface */
#undef ENABLE_X11FONT

/* define this if you use X11 render etension */
#undef ENABLE_XRENDER

/* define this if you have freetype/freetype.h */
#undef FREETYPE_HAVE_DIR

/* Define if the GNU dcgettext() function is already present or preinstalled.
   */
#undef HAVE_DCGETTEXT

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define if the C complier supports __func__ */
#undef HAVE_FUNC

/* Define if the GNU gettext() function is already present or preinstalled. */
#undef HAVE_GETTEXT

/* Define to 1 if you have the `gettimeofday' function. */
#define HAVE_GETTIMEOFDAY 1

/* Define if you have the iconv() function. */
#define HAVE_ICONV 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the `asound' library (-lasound). */
#undef HAVE_LIBASOUND

/* Define to 1 if you have the <libgen.h> header file. */
#define HAVE_LIBGEN_H 1

/* Define to 1 if you have the <linux/joystick.h> header file. */
#undef HAVE_LINUX_JOYSTICK_H

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the `mkdtemp' function. */
#define HAVE_MKDTEMP 1

/* define this if you use SDL joystick interface */
#undef HAVE_SDLJOY

/* define this if you have SDLRALPHA */
#define HAVE_SDLRALPHA 1

/* define this if you have SDLRLE */
#define HAVE_SDLRLE 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
#define HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define this if you have libXxf86vm installed */
#undef HAVE_XF86VMODE

/* joystick device */
#undef JOY_DEVICE

/* midi device */
#undef MIDI_DEVICE

/* midi player */
#undef MIDI_PLAYER

/* mixer device */
#undef MIXERDEV_ALSA

/* mixer device */
#undef MIXERDEV_OSS

/* Name of package */
#define PACKAGE "xsystem35"

/* Define to the address where bug reports for this package should be sent. */
#undef PACKAGE_BUGREPORT

/* Define to the full name of this package. */
#undef PACKAGE_NAME

/* Define to the full name and version of this package. */
#undef PACKAGE_STRING

/* Define to the one symbol short name of this package. */
#undef PACKAGE_TARNAME

/* Define to the version of this package. */
#undef PACKAGE_VERSION

/* define this if you stop midi stdout */
#undef QUITE_MIDI

/* Define this if X11 Pixmap color order is RGB */
#define RGB_ORDER 1

/* sequencer device */
#undef SEQ_DEVICE

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define to 1 if you can safely include both <sys/time.h> and <time.h>. */
#define TIME_WITH_SYS_TIME 1

/* Define to 1 if your <sys/time.h> declares `struct tm'. */
#undef TM_IN_SYS_TIME

/* Version number of package */
#define VERSION "1.7.2"

/* Define to 1 if your processor stores words with the most significant byte
   first (like Motorola and SPARC, unlike Intel and VAX). */
#undef WORDS_BIGENDIAN

/* Define to 1 if the X Window System is missing or not being used. */
#define X_DISPLAY_MISSING 1

/* Define to empty if `const' does not conform to ANSI C. */
#undef const

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
#undef inline
#endif

/* Define to `int' if <sys/types.h> does not define. */
#undef pid_t

/* Define to `unsigned' if <sys/types.h> does not define. */
#undef size_t
