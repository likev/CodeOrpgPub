/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2005/10/04 19:32:06 $
 * $Id: orpgerr.h,v 1.31 2005/10/04 19:32:06 steves Exp $
 * $Revision: 1.31 $
 * $State: Exp $
 */

/**************************************************************************

      Module: orpgerr.h

 Description: Global Open Systems Radar Product Generator (ORPG) error
              header file.
       Notes: All constants defined in this file begin with the prefix
              ORPGERR_.
              The ANSIC error constants are based on the list of Standard C
              library functions listed in the POSIX Programmer's Guide.
              The POSIX error constants are based on the list of POSIX
              library functions listed in the POSIX Programmer's Guide.
 **************************************************************************/

#ifndef ORPGERR_H
#define ORPGERR_H

#include <errno.h>
#include <infr.h>

/* for c++ compatability */
#ifdef __cplusplus
extern "C"
{
#endif

/**
  * LE Message Code bitflags
  *
  * Note that only GL_ERROR messages currently map to critical LE messages.
  */
#define GL_STATUS_BIT 0x10000000
#define GL_ERROR_BIT  0x20000000
#define GL_GLOBAL_BIT 0x40000000
#define GL_CRIT_BIT   (LE_CRITICAL_BIT)


#define GL_STATUS	((GL_STATUS_BIT) | (LE_CRITICAL))
#define GL_GLOBAL	((GL_GLOBAL_BIT) | (LE_CRITICAL))
#define GL_ERROR	((GL_ERROR_BIT) | (LE_CRITICAL))
#define GL_INFO		0

/* An ORPG LE_send_msg call looks like LE_send_msg (GL_STATUS | number, "..."),
   where GL_STATUS can be replaced by GL_ERROR or GL_INFO. "number", in the
   range of [1, 0xffff], is used as an ID of the particular LE_send_msg
   call. This is true for LE_send_msg calls from both task and library.

   GL_GLOBAL bit in the LE_send_msg's first argument can be set for specifying
   additional ID info. This, for example, can be used for specifying that it
   is called from a library routine.

   There is still one byte that is not used in the first argument of 
   LE_send_msg. One way to use this is to specify a group number. A library 
   module can then be treated as such a group. See the following GL_ORPGDAT 
   example.

   Use of globally defined group numbers introduces dependencies. It should not
   be needed for all LE reports from ORPG applications. We may use it for 
   identifying the particular library module from which an LE message is 
   reported if we feel it is necessary. */

/* An example showing one posible way to use GL_GLOBAL for library LE */
enum {GL_DAT_LIB_GID=1,
      GL_TASK_LIB_GID} ;
#define GL_ORPGDAT	(((GL_DAT_LIB_GID) << 16) | (GL_GLOBAL))
#define GL_ORPGTASK	(((GL_TASK_LIB_GID) << 16) | (GL_GLOBAL))

/* ORPG error message macros. Steve suggested that we should use macros for
   common message text strings. */
#define GL_malloc(s) \
	(LE_gen_text ("Memory allocation (size %d) failed", s))
#define GL_ORPG_read(data, id, retval) \
	(LE_gen_text ("ORPG_read (data %d, msgid %d, retval %d) failed", data, id, retval))
#define GL_ORPG_write(data, id, retval) \
	(LE_gen_text ("ORPG_write (data %d, msgid %d, retval %d) failed", data, id, retval))
#define GL_EN_post(evtcd, retval) \
	(LE_gen_text ("EN_post (evtcd %d, retval %d) failed", evtcd, retval))

/* LE message RPG message types.  The types of messages are specified in bits 16-18.   
   Bit 21 is reserved and denotes Alarm Cleared bit. */
#define LE_RPG_MSG_SHIFT                16
#define LE_RPG_MSG_MASK                 (31 << LE_RPG_MSG_SHIFT)
#define LE_RPG_MSG_TYPE_MASK            (15 << LE_RPG_MSG_SHIFT)

/* The following are general type RPG messages. */
#define LE_RPG_STATUS_TYPE_MASK         (7 << LE_RPG_MSG_SHIFT)
#define LE_RPG_INFO_MSG                 (1 << LE_RPG_MSG_SHIFT)
#define LE_RPG_WARN_STATUS              (2 << LE_RPG_MSG_SHIFT)
#define LE_RPG_GEN_STATUS               (3 << LE_RPG_MSG_SHIFT)
#define LE_RPG_COMMS                    (4 << LE_RPG_MSG_SHIFT)

/* The following are RPG alarms. */
#define LE_RPG_ALARM_SHIFT              19
#define LE_RPG_ALARM_MASK               (7 << LE_RPG_ALARM_SHIFT)
#define LE_RPG_ALARM_TYPE_MASK          (3 << LE_RPG_ALARM_SHIFT)
#define LE_RPG_AL_LS                    (1 << LE_RPG_ALARM_SHIFT)
#define LE_RPG_AL_MAR                   (2 << LE_RPG_ALARM_SHIFT)
#define LE_RPG_AL_MAM                   (3 << LE_RPG_ALARM_SHIFT)
#define LE_RPG_AL_CLEARED               (4 << LE_RPG_ALARM_SHIFT)

/* RDA alarm levels.  The higher the level, the more severe the alarm. 
   Bits 22 - 26 are used to describe the alarm.   Bit 26 is reserved for the
   Alarm Cleared bit. */
#define LE_RDA_ALARM_SHIFT              22     
#define LE_RDA_ALARM_MASK               (15 << LE_RDA_ALARM_SHIFT)
#define LE_RDA_ALARM_TYPE_MASK          (7 << LE_RDA_ALARM_SHIFT)

#define LE_GET_RDA_ALARM_LVL(code)      (((code) >> LE_RDA_ALARM_SHIFT) & 0x15)

/* The following are RDA alarms. */
#define LE_RDA_AL_NOT_APP       (1 << LE_RDA_ALARM_SHIFT)
#define LE_RDA_AL_SEC           (2 << LE_RDA_ALARM_SHIFT)
#define LE_RDA_AL_MAR           (3 << LE_RDA_ALARM_SHIFT)
#define LE_RDA_AL_MAM           (4 << LE_RDA_ALARM_SHIFT)
#define LE_RDA_AL_INOP          (5 << LE_RDA_ALARM_SHIFT)
#define LE_RDA_AL_CLEARED       (8 << LE_RDA_ALARM_SHIFT)



/* $$$$$       The following will be phased out       $$$$$ */


/*
 * Categorize error codes based upon the following scheme:
 *	Standard C (ANSIC)
 *	Portable Operating System Interface for Computer Environments (POSIX)
 *	System V Interface Description (SVID)
 *	X Portability Group (XPG)
 */

/* special code types */
#define GL_LIBRARY	((1 << 28) | LE_CRITICAL)
					/* for library type LE */
#define GL_UNIX		((2 << 28) | LE_CRITICAL)
					/* for UNIX system call type LE */
#define GL_STDC		((3 << 28) | LE_CRITICAL)
					/* for standard C function type LE */

#define GL_ERRNO_MASK	0xffff		/* error number field mask */
#define GL_ERRNO_SHIFT	8		/* errno field shift */

#define GL_ERRNO ((errno & GL_ERRNO_MASK) << GL_ERRNO_SHIFT)

/*
 * ANSI C Error Constant Definitions
 */
#define GLC_ABORT	GL_STDC | GL_ERRNO | 000
#define GLC_ABS		GL_STDC | GL_ERRNO | 001
#define GLC_ACOS	GL_STDC | GL_ERRNO | 002
#define GLC_ASCTIME	GL_STDC | GL_ERRNO | 003
#define GLC_ASIN	GL_STDC | GL_ERRNO | 004
#define GLC_ATAN	GL_STDC | GL_ERRNO | 005
#define GLC_ATAN2	GL_STDC | GL_ERRNO | 006
#define GLC_ATEXIT	GL_STDC | GL_ERRNO | 007
#define GLC_ATOF	GL_STDC | GL_ERRNO | 008
#define GLC_ATOI	GL_STDC | GL_ERRNO | 009
#define GLC_ATOL	GL_STDC | GL_ERRNO | 010
#define GLC_BSEARCH	GL_STDC | GL_ERRNO | 011
#define GLC_CEIL	GL_STDC | GL_ERRNO | 012
#define GLC_CALLOC	GL_STDC | GL_ERRNO | 013
#define GLC_CLEARERR	GL_STDC | GL_ERRNO | 014
#define GLC_CLOCK	GL_STDC | GL_ERRNO | 015
#define GLC_COS		GL_STDC | GL_ERRNO | 016
#define GLC_COSH	GL_STDC | GL_ERRNO | 017
#define GLC_CTIME	GL_STDC | GL_ERRNO | 018
#define GLC_DIFFTIME	GL_STDC | GL_ERRNO | 019
#define GLC_DIV		GL_STDC | GL_ERRNO | 020
#define GLC_EXIT	GL_STDC | GL_ERRNO | 021
#define GLC_EXP		GL_STDC | GL_ERRNO | 022
#define GLC_FABS	GL_STDC | GL_ERRNO | 023
#define GLC_FCLOSE	GL_STDC | GL_ERRNO | 024
#define GLC_FEOF	GL_STDC | GL_ERRNO | 025
#define GLC_FERROR	GL_STDC | GL_ERRNO | 026
#define GLC_FFLUSH	GL_STDC | GL_ERRNO | 027
#define GLC_FGETC	GL_STDC | GL_ERRNO | 028
#define GLC_FGETPOS	GL_STDC | GL_ERRNO | 029
#define GLC_FGETS	GL_STDC | GL_ERRNO | 030
#define GLC_FLOOR	GL_STDC | GL_ERRNO | 031
#define GLC_FMOD	GL_STDC | GL_ERRNO | 032
#define GLC_FOPEN	GL_STDC | GL_ERRNO | 033
#define GLC_FPRINTF	GL_STDC | GL_ERRNO | 034
#define GLC_FPUTC	GL_STDC | GL_ERRNO | 035
#define GLC_FPUTS	GL_STDC | GL_ERRNO | 036
#define GLC_FREAD	GL_STDC | GL_ERRNO | 037
#define GLC_FREE	GL_STDC | GL_ERRNO | 038
#define GLC_FREOPEN	GL_STDC | GL_ERRNO | 039
#define GLC_FREXP	GL_STDC | GL_ERRNO | 040
#define GLC_FSCANF	GL_STDC | GL_ERRNO | 041
#define GLC_FSETPOS	GL_STDC | GL_ERRNO | 042
#define GLC_FTELL	GL_STDC | GL_ERRNO | 043
#define GLC_FWRITE	GL_STDC | GL_ERRNO | 044
#define GLC_GETC	GL_STDC | GL_ERRNO | 045
#define GLC_GETCHAR	GL_STDC | GL_ERRNO | 046
#define GLC_GETENV	GL_STDC | GL_ERRNO | 047
#define GLC_GETS	GL_STDC | GL_ERRNO | 048
#define GLC_GMTIME	GL_STDC | GL_ERRNO | 049
#define GLC_ISALNUM	GL_STDC | GL_ERRNO | 050
#define GLC_ISALPHA	GL_STDC | GL_ERRNO | 051
#define GLC_ISCNTRL	GL_STDC | GL_ERRNO | 052
#define GLC_ISDIGIT	GL_STDC | GL_ERRNO | 053
#define GLC_ISGRAPH	GL_STDC | GL_ERRNO | 054
#define GLC_ISLOWER	GL_STDC | GL_ERRNO | 055
#define GLC_ISPRINT	GL_STDC | GL_ERRNO | 056
#define GLC_ISPUNCT	GL_STDC | GL_ERRNO | 057
#define GLC_ISSPACE	GL_STDC | GL_ERRNO | 058
#define GLC_ISUPPER	GL_STDC | GL_ERRNO | 059
#define GLC_ISXDIGIT	GL_STDC | GL_ERRNO | 060
#define GLC_LABS	GL_STDC | GL_ERRNO | 061
#define GLC_LDEXP	GL_STDC | GL_ERRNO | 062
#define GLC_LDIV	GL_STDC | GL_ERRNO | 063
#define GLC_LOCALECONV	GL_STDC | GL_ERRNO | 064
#define GLC_LOCALTIME	GL_STDC | GL_ERRNO | 065
#define GLC_LOG		GL_STDC | GL_ERRNO | 066
#define GLC_LOG10	GL_STDC | GL_ERRNO | 067
#define GLC_LONGJMP	GL_STDC | GL_ERRNO | 068
#define GLC_MALLOC	GL_STDC | GL_ERRNO | 069
#define GLC_MBLEN	GL_STDC | GL_ERRNO | 070
#define GLC_MBSTOWCS	GL_STDC | GL_ERRNO | 071
#define GLC_MBTOWC	GL_STDC | GL_ERRNO | 072
#define GLC_MEMCHR	GL_STDC | GL_ERRNO | 073
#define GLC_MEMCMP	GL_STDC | GL_ERRNO | 074
#define GLC_MEMCPY	GL_STDC | GL_ERRNO | 075
#define GLC_MEMMOVE	GL_STDC | GL_ERRNO | 076
#define GLC_MEMSET	GL_STDC | GL_ERRNO | 077
#define GLC_MKTIME	GL_STDC | GL_ERRNO | 078
#define GLC_MODF	GL_STDC | GL_ERRNO | 079
#define GLC_PERROR	GL_STDC | GL_ERRNO | 080
#define GLC_PRINTF	GL_STDC | GL_ERRNO | 081
#define GLC_PUTC	GL_STDC | GL_ERRNO | 082
#define GLC_PUTCHAR	GL_STDC | GL_ERRNO | 083
#define GLC_PUTS	GL_STDC | GL_ERRNO | 084
#define GLC_QSORT	GL_STDC | GL_ERRNO | 085
#define GLC_RAISE	GL_STDC | GL_ERRNO | 086
#define GLC_RAND	GL_STDC | GL_ERRNO | 087
#define GLC_REALLOC	GL_STDC | GL_ERRNO | 088
#define GLC_REMOVE	GL_STDC | GL_ERRNO | 089
#define GLC_RENAME	GL_STDC | GL_ERRNO | 090
#define GLC_REWIND	GL_STDC | GL_ERRNO | 091
#define GLC_SCANF	GL_STDC | GL_ERRNO | 092
#define GLC_SETBUF	GL_STDC | GL_ERRNO | 093
#define GLC_SETLOCALE	GL_STDC | GL_ERRNO | 094
#define GLC_SETVBUF	GL_STDC | GL_ERRNO | 095
#define GLC_SIN		GL_STDC | GL_ERRNO | 096
#define GLC_SPRINTF	GL_STDC | GL_ERRNO | 097
#define GLC_SQRT	GL_STDC | GL_ERRNO | 098
#define GLC_SRAND	GL_STDC | GL_ERRNO | 099
#define GLC_STRCMP	GL_STDC | GL_ERRNO | 100
#define GLC_SSCANF	GL_STDC | GL_ERRNO | 101
#define GLC_STRCAT	GL_STDC | GL_ERRNO | 102
#define GLC_STRCHR	GL_STDC | GL_ERRNO | 103
#define GLC_STRCOLL	GL_STDC | GL_ERRNO | 104
#define GLC_STRCPY	GL_STDC | GL_ERRNO | 105
#define GLC_STRCSPN	GL_STDC | GL_ERRNO | 106
#define GLC_STRERROR	GL_STDC | GL_ERRNO | 107
#define GLC_STRFTIME	GL_STDC | GL_ERRNO | 108
#define GLC_STRLEN	GL_STDC | GL_ERRNO | 109
#define GLC_STRNCAT	GL_STDC | GL_ERRNO | 110
#define GLC_STRNCMP	GL_STDC | GL_ERRNO | 111
#define GLC_STRNCPY	GL_STDC | GL_ERRNO | 112
#define GLC_STRPBRK	GL_STDC | GL_ERRNO | 113
#define GLC_STRRCHR	GL_STDC | GL_ERRNO | 114
#define GLC_STRSPN	GL_STDC | GL_ERRNO | 115
#define GLC_STRSTR	GL_STDC | GL_ERRNO | 116
#define GLC_STRTOD	GL_STDC | GL_ERRNO | 117
#define GLC_STRTOK	GL_STDC | GL_ERRNO | 118
#define GLC_STRTOL	GL_STDC | GL_ERRNO | 119
#define GLC_STRTOUL	GL_STDC | GL_ERRNO | 120
#define GLC_STRXFRM	GL_STDC | GL_ERRNO | 121
#define GLC_SYSTEM	GL_STDC | GL_ERRNO | 122
#define GLC_TAN		GL_STDC | GL_ERRNO | 123
#define GLC_TANH	GL_STDC | GL_ERRNO | 124
#define GLC_TIME	GL_STDC | GL_ERRNO | 125
#define GLC_TMPFILE	GL_STDC | GL_ERRNO | 126
#define GLC_TMPNAM	GL_STDC | GL_ERRNO | 127
#define GLC_TOLOWER	GL_STDC | GL_ERRNO | 128
#define GLC_TOUPPER	GL_STDC | GL_ERRNO | 129
#define GLC_UNGETC	GL_STDC | GL_ERRNO | 130
#define GLC_VFPRINTF	GL_STDC | GL_ERRNO | 131
#define GLC_VPRINTF	GL_STDC | GL_ERRNO | 132
#define GLC_VSPRINTF	GL_STDC | GL_ERRNO | 133
#define GLC_WCSTOMBS	GL_STDC | GL_ERRNO | 134
#define GLC_WCSTOMB	GL_STDC | GL_ERRNO | 135

/*
 * POSIX Error Constant Definitions
 */
#define GLU_ACCESS	GL_UNIX | GL_ERRNO | 000
#define GLU_ALARM	GL_UNIX | GL_ERRNO | 001
#define GLU_CFGETISPEED	GL_UNIX | GL_ERRNO | 003
#define GLU_CFGETOSPEED	GL_UNIX | GL_ERRNO | 004
#define GLU_CFSETISPEED	GL_UNIX | GL_ERRNO | 005
#define GLU_CFSETOSPEED	GL_UNIX | GL_ERRNO | 006
#define GLU_CHDIR	GL_UNIX | GL_ERRNO | 007
#define GLU_CHMOD	GL_UNIX | GL_ERRNO | 008
#define GLU_CHOWN	GL_UNIX | GL_ERRNO | 009
#define GLU_CLOSE	GL_UNIX | GL_ERRNO | 010
#define GLU_CLOSEDIR	GL_UNIX | GL_ERRNO | 011
#define GLU_CREAT	GL_UNIX | GL_ERRNO | 012
#define GLU_CTERMID	GL_UNIX | GL_ERRNO | 013
#define GLU_CUSERID	GL_UNIX | GL_ERRNO | 014
#define GLU_DUP		GL_UNIX | GL_ERRNO | 015
#define GLU_DUP2	GL_UNIX | GL_ERRNO | 016
#define GLU_EXECL	GL_UNIX | GL_ERRNO | 017
#define GLU_EXECLE	GL_UNIX | GL_ERRNO | 018
#define GLU_EXECLP	GL_UNIX | GL_ERRNO | 019
#define GLU_EXECV	GL_UNIX | GL_ERRNO | 020
#define GLU_EXECVE	GL_UNIX | GL_ERRNO | 021
#define GLU_EXECVP	GL_UNIX | GL_ERRNO | 022
#define GLU__EXIT	GL_UNIX | GL_ERRNO | 023
#define GLU_FCNTL	GL_UNIX | GL_ERRNO | 024
#define GLU_FDOPEN	GL_UNIX | GL_ERRNO | 025
#define GLU_FORK	GL_UNIX | GL_ERRNO | 026
#define GLU_FPATHCONF	GL_UNIX | GL_ERRNO | 027
#define GLU_FSTAT	GL_UNIX | GL_ERRNO | 028
#define GLU_GETCWD	GL_UNIX | GL_ERRNO | 029
#define GLU_GETEGID	GL_UNIX | GL_ERRNO | 030
#define GLU_GETEUID	GL_UNIX | GL_ERRNO | 032
#define GLU_GETGID	GL_UNIX | GL_ERRNO | 033
#define GLU_GETGRGID	GL_UNIX | GL_ERRNO | 034
#define GLU_GETGRNAM	GL_UNIX | GL_ERRNO | 035
#define GLU_GETGROUPS	GL_UNIX | GL_ERRNO | 036
#define GLU_GETLOGIN	GL_UNIX | GL_ERRNO | 037
#define GLU_GETPGRP	GL_UNIX | GL_ERRNO | 038
#define GLU_GETPID	GL_UNIX | GL_ERRNO | 039
#define GLU_GETPPID	GL_UNIX | GL_ERRNO | 040
#define GLU_GETPWNAM	GL_UNIX | GL_ERRNO | 041
#define GLU_GETPWUID	GL_UNIX | GL_ERRNO | 042
#define GLU_GETUID	GL_UNIX | GL_ERRNO | 043
#define GLU_ISATTY	GL_UNIX | GL_ERRNO | 044
#define GLU_KILL	GL_UNIX | GL_ERRNO | 045
#define GLU_LINK	GL_UNIX | GL_ERRNO | 046
#define GLU_LSEEK	GL_UNIX | GL_ERRNO | 048
#define GLU_MKDIR	GL_UNIX | GL_ERRNO | 049
#define GLU_MKFIFO	GL_UNIX | GL_ERRNO | 050
#define GLU_OPEN	GL_UNIX | GL_ERRNO | 051
#define GLU_OPENDIR	GL_UNIX | GL_ERRNO | 052
#define GLU_PATHCONF	GL_UNIX | GL_ERRNO | 053
#define GLU_PAUSE	GL_UNIX | GL_ERRNO | 054
#define GLU_PIPE	GL_UNIX | GL_ERRNO | 055
#define GLU_READ	GL_UNIX | GL_ERRNO | 056
#define GLU_READDIR	GL_UNIX | GL_ERRNO | 057
#define GLU_REWINDDIR	GL_UNIX | GL_ERRNO | 059
#define GLU_RMDIR	GL_UNIX | GL_ERRNO | 060
#define GLU_SETGID	GL_UNIX | GL_ERRNO | 061
#define GLU_SETJMP	GL_UNIX | GL_ERRNO | 062
#define GLU_SETPGID	GL_UNIX | GL_ERRNO | 064
#define GLU_SETUID	GL_UNIX | GL_ERRNO | 065
#define GLU_SIGACTION	GL_UNIX | GL_ERRNO | 066
#define GLU_SIGADDSET	GL_UNIX | GL_ERRNO | 067
#define GLU_SIGDELSET	GL_UNIX | GL_ERRNO | 068
#define GLU_SIGEMPTYSET	GL_UNIX | GL_ERRNO | 069
#define GLU_SIGFILLSET	GL_UNIX | GL_ERRNO | 070
#define GLU_SIGISMEMBER	GL_UNIX | GL_ERRNO | 071
#define GLU_SIGLONGJMP	GL_UNIX | GL_ERRNO | 072
#define GLU_SIGPENDING	GL_UNIX | GL_ERRNO | 073
#define GLU_SIGPROCMASK	GL_UNIX | GL_ERRNO | 074
#define GLU_SIGSETJMP	GL_UNIX | GL_ERRNO | 075
#define GLU_SIGSUSPEND	GL_UNIX | GL_ERRNO | 076
#define GLU_SLEEP	GL_UNIX | GL_ERRNO | 077
#define GLU_STAT	GL_UNIX | GL_ERRNO | 078
#define GLU_SYSCONF	GL_UNIX | GL_ERRNO | 079
#define GLU_TCDRAIN	GL_UNIX | GL_ERRNO | 080
#define GLU_TCFLOW	GL_UNIX | GL_ERRNO | 081
#define GLU_TCFLUSH	GL_UNIX | GL_ERRNO | 082
#define GLU_TCGETATTR	GL_UNIX | GL_ERRNO | 083
#define GLU_TCGETPGRP	GL_UNIX | GL_ERRNO | 084
#define GLU_TCSENDBREAK	GL_UNIX | GL_ERRNO | 085
#define GLU_TCSETATTR	GL_UNIX | GL_ERRNO | 086
#define GLU_TCSETPGRP	GL_UNIX | GL_ERRNO | 087
#define GLU_TIMES	GL_UNIX | GL_ERRNO | 089
#define GLU_TTYNAME	GL_UNIX | GL_ERRNO | 090
#define GLU_TZSET	GL_UNIX | GL_ERRNO | 091
#define GLU_UMASK	GL_UNIX | GL_ERRNO | 092
#define GLU_UNAME	GL_UNIX | GL_ERRNO | 093
#define GLU_UNLINK	GL_UNIX | GL_ERRNO | 094
#define GLU_UTIME	GL_UNIX | GL_ERRNO | 095
#define GLU_WAITPID	GL_UNIX | GL_ERRNO | 096
#define GLU_WRITE	GL_UNIX | GL_ERRNO | 097


/*
 * SVID Error Constant Definitions
 */
#define GLU_IOCTL	GL_UNIX | GL_ERRNO | 160
#define GLU_GETOPT	GL_UNIX | GL_ERRNO | 161



/* to make the macros SHORT, we use prefix GL which stands for ORPG_LE */
/* G - RPG; A - RDA; P - PUP */

#define GL_ACTION_SHIFT		24	/* action field shifts */

/*
 * Action Codes - 0 to 3 (two bits only)
 *
 * Use of these codes is TBD and will be documented here and elsewhere
 * in the future (ABD:06FEB1998)
 */
#define GL_TERM	(LE_CRITICAL | (1 << GL_ACTION_SHIFT))
                               /* Process terminated by exit() in code    */
#define GL_WAIT	(2 << GL_ACTION_SHIFT)
                               /* TBD                                     */
#define GL_LEGACY_ABORT	(LE_CRITICAL | (3 << GL_ACTION_SHIFT))
                               /* Legacy (or legacy-style) task aborted   */
                               /* will restart itself when data are       */
                               /* available ...                           */
#define GL_CONT	0

/*
 * Category Codes
 *
 * Two possible uses:
 *
 * (1) use to build LE message code for LE message written directly by
 *     application software process
 * (2) pass to ORPGTASK_exit(), which will then use the category code
 *     to build the LE message code for the TERMINAL LE message written
 *     to the process's LE logfile
 *
 *     NOTE: GL_NORMAL_SIG and GL_ABNORMAL_SIG are reserved for the
 *           use of the the ORPGASKM_* termination signal handler.
 *       
 * NOTE: ABDodson added GL_ALARM_BIT 17JUN1999 ... if there's a clean way to
 *       unroll that bit, then we can remove this #define
 */
#define GL_ALARM_BIT 13

#define GL_MEMORY	(GL_ERROR)
#define GL_FILE		(GL_ERROR)
#define GL_OS		(GL_ERROR)
#define GL_CONFIG	(GL_ERROR)
#define GL_INPUT	(GL_ERROR)
#define GL_COMMS	(GL_ERROR)
/* #define GL_ERROR	(GL_ERROR) */
#define GL_CODE		(GL_ERROR)
#define GL_ALARM(code)	((GL_ERROR))

#ifdef NEVERDEFINED
#define GL_EXIT_SUCCESS	(GL_STATUS) /* like EXIT_SUCCESS         */
#define GL_EXIT_FAILURE	((GL_ERROR)) /* like EXIT_FAILURE         */
#define GL_NORMAL_SIG	((GL_STATUS)) /* e.g. SIGTERM (-lorpg only)*/
#define GL_ABNORMAL_SIG	((GL_ERROR)) /* e.g. SIGSEGV (-lorpg only)*/
#endif
/***
 *** TEMPORARY: pending replacement with ORPGTASK_EXIT_ enumerations
 ***/
enum {GL_EXIT_SUCCESS=1,
      GL_EXIT_FAILURE,
      GL_EXIT_ABORTED,
      GL_NORMAL_SIG,
      GL_ABNORMAL_SIG} ;


/* category macros for library functions */
#define GL_LB(ret)	(GL_ERROR)
#define GL_EN(ret)	(GL_ERROR)
#define GL_MISC(ret)	(GL_ERROR)
#define GL_RPG(ret)	(GL_ERROR)
#define GL_ORPGDA(ret)	(GL_ERROR)
#define GL_ORPGCMI(ret)	(GL_ERROR)
#define GL_UMC(ret)	(GL_ERROR)
#define GL_MALRM(ret)	(GL_ERROR)
#define GL_LE(ret)	(GL_ERROR)
#define GL_CS(ret)	(GL_ERROR)

/*
 * XPG Error Constant Definitions
 */




#ifdef __cplusplus
}
#endif

#endif /*DO NOT REMOVE!*/
