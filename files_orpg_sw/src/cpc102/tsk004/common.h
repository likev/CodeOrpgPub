/*
 * RCS info
 * $Author: cmn $
 * $Locker:  $
 * $Date: 2011/05/22 16:48:17 $
 * $Id: common.h,v 1.1 2011/05/22 16:48:17 cmn Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */
/*
 * sysstat: System performance tools for Linux
 * (C) 1999-2006 by Sebastien Godard (sysstat <at> wanadoo.fr)
 */

#ifndef _COMMON_H
#define _COMMON_H

#include <time.h>

#ifndef FALSE
#define FALSE	0
#endif

#ifndef TRUE
#define TRUE	1
#endif

#define MINIMUM(a,b)	((a) < (b) ? (a) : (b))

#define NR_CPUS		1024

/*
 * Size of /proc/interrupts line (at NR_CPUS # of cpus)
 * 4 spaces for interrupt # field ; 11 spaces for each interrupt field.
 */
#define INTERRUPTS_LINE	(4 + 11 * NR_CPUS)

/* Keywords */
#define K_ISO	"ISO"
#define K_ALL	"ALL"
#define K_UTC	"UTC"

/* Files */
#define STAT		"/proc/stat"
#define PPARTITIONS	"/proc/partitions"
#define DISKSTATS	"/proc/diskstats"
#define INTERRUPTS	"/proc/interrupts"
#define SYSFS_BLOCK	"/sys/block"
#define SYSFS_DEVCPU	"/sys/devices/system/cpu"
#define S_STAT		"stat"

#define MAX_FILE_LEN	256
#define MAX_PF_NAME	1024

#define NR_DEV_PREALLOC		4
#define NR_DISK_PREALLOC	3

#define CNT_DEV		0
#define CNT_PART	1
#define CNT_ALL_DEV	0
#define CNT_USED_DEV	1

#define S_VALUE(m,n,p)	(((double) ((n) - (m))) / (p) * HZ)

/* new define to normalize to %; HZ is 1024 on IA64 and % should be normalized to 100 */
#define SP_VALUE(m,n,p)	(((double) ((n) - (m))) / (p) * 100)

/*
 * 0: stats at t,
 * 1: stats at t' (t+T or t-T),
 * 2: average.
 */
#define DIM	3

/* Environment variables */
#define ENV_TIME_FMT	"S_TIME_FORMAT"
#define ENV_TIME_DEFTM	"S_TIME_DEF_TIME"

#define DIGITS		"0123456789"

#define UTSNAME_LEN	65

#define NR_DISKS	4

#define DISP_HDR	1

/* Number of ticks per second */
#define HZ		hz
extern unsigned int hz;

/* Number of bit shifts to convert pages to kB */
extern unsigned int kb_shift;

/* Functions */
extern char	   *device_name(char *);
extern void	    get_HZ(void);
extern unsigned int get_disk_io_nr(void);
extern void	    get_kb_shift(void);
extern time_t	    get_localtime(struct tm *);
extern time_t	    get_time(struct tm *);
extern int	    get_cpu_nr(unsigned int);
extern int	    get_sysfs_dev_nr(int);
extern int	    get_diskstats_dev_nr(int, int);
extern int	    get_ppartitions_dev_nr(int);
extern int	    get_win_height(void);
extern void	    init_nls(void);
extern double	    ll_s_value(unsigned long long, unsigned long long,
			       unsigned long long);
extern double	    ll_sp_value(unsigned long long, unsigned long long,
				unsigned long long);
extern void	    print_gal_header(struct tm *, char *, char *, char *);
extern void	    print_version(void);

#endif  /* _COMMON_H */
