/*
 * RCS info
 * $Author: cmn $
 * $Locker:  $
 * $Date: 2011/05/22 16:48:18 $
 * $Id: iostat.h,v 1.1 2011/05/22 16:48:18 cmn Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */
/*
 * iostat: report CPU and I/O statistics
 * (C) 1999-2006 by Sebastien Godard (sysstat <at> wanadoo.fr)
 */

#ifndef _IOSTAT_H
#define _IOSTAT_H

#include "common.h"

#define MAX_NAME_LEN	72

/* I_: iostat - D_: Display - F_: Flag */
#define I_D_CPU_ONLY		0x0001
#define I_D_DISK_ONLY		0x0002
#define I_D_TIMESTAMP		0x0004
#define I_D_EXTENDED		0x0008
#define I_D_PART_ALL		0x0010
#define I_D_KILOBYTES		0x0020
#define I_F_HAS_SYSFS		0x0040
#define I_F_OLD_KERNEL		0x0080
#define I_D_UNFILTERED		0x0100
#define I_D_MEGABYTES		0x0200
#define I_D_PARTITIONS		0x0400
#define I_F_HAS_DISKSTATS	0x0800
#define I_F_HAS_PPARTITIONS	0x1000
#define I_F_PLAIN_KERNEL24	0x2000

#define DISPLAY_CPU_ONLY(m)	(((m) & I_D_CPU_ONLY) == I_D_CPU_ONLY)
#define DISPLAY_DISK_ONLY(m)	(((m) & I_D_DISK_ONLY) == I_D_DISK_ONLY)
#define DISPLAY_TIMESTAMP(m)	(((m) & I_D_TIMESTAMP) == I_D_TIMESTAMP)
#define DISPLAY_EXTENDED(m)	(((m) & I_D_EXTENDED) == I_D_EXTENDED)
#define DISPLAY_PART_ALL(m)	(((m) & I_D_PART_ALL) == I_D_PART_ALL)
#define DISPLAY_KILOBYTES(m)	(((m) & I_D_KILOBYTES) == I_D_KILOBYTES)
#define DISPLAY_MEGABYTES(m)	(((m) & I_D_MEGABYTES) == I_D_MEGABYTES)
#define HAS_SYSFS(m)		(((m) & I_F_HAS_SYSFS) == I_F_HAS_SYSFS)
#define HAS_OLD_KERNEL(m)	(((m) & I_F_OLD_KERNEL) == I_F_OLD_KERNEL)
#define DISPLAY_UNFILTERED(m)	(((m) & I_D_UNFILTERED) == I_D_UNFILTERED)
#define DISPLAY_PARTITIONS(m)	(((m) & I_D_PARTITIONS) == I_D_PARTITIONS)
#define HAS_DISKSTATS(m)	(((m) & I_F_HAS_DISKSTATS) == I_F_HAS_DISKSTATS)
#define HAS_PPARTITIONS(m)	(((m) & I_F_HAS_PPARTITIONS) == I_F_HAS_PPARTITIONS)
#define HAS_PLAIN_KERNEL24(m)	(((m) & I_F_PLAIN_KERNEL24) == I_F_PLAIN_KERNEL24)

#define DT_DEVICE	0
#define DT_PARTITION	1

/* Device name for old kernels */
#define K_HDISK	"hdisk"

struct comm_stats {
   unsigned long long uptime;
   unsigned long long uptime0;
   unsigned long long cpu_iowait;
   unsigned long long cpu_idle;
   unsigned long long cpu_user;
   unsigned long long cpu_nice;
   unsigned long long cpu_system;
   unsigned long long cpu_steal;
};

#define COMM_STATS_SIZE	(sizeof(struct comm_stats))

/*
 * Structures for I/O stats.
 * The number of structures allocated corresponds to the number of devices
 * present in the system, plus a preallocation number to handle those
 * that can be registered dynamically.
 * The number of devices is found by using /sys filesystem (if mounted),
 * or the number of "disk_io:" entries in /proc/stat (2.4 kernels),
 * else the default value is 4 (for old kernels, which maintained stats
 * for the first four devices in /proc/stat).
 * For each io_stats structure allocated corresponds a io_hdr_stats structure.
 * A io_stats structure is considered as unused or "free" (containing no stats
 * for a particular device) if the 'major' field of the io_hdr_stats
 * structure is set to 0.
 */
struct io_stats {
   /* # of read operations issued to the device */
   unsigned long rd_ios				__attribute__ ((aligned (8)));
   /* # of read requests merged */
   unsigned long rd_merges			__attribute__ ((packed));
   /* # of sectors read */
   unsigned long long rd_sectors		__attribute__ ((packed));
   /* Time of read requests in queue */
   unsigned long rd_ticks			__attribute__ ((packed));
   /* # of write operations issued to the device */
   unsigned long wr_ios				__attribute__ ((packed));
   /* # of write requests merged */
   unsigned long wr_merges			__attribute__ ((packed));
   /* # of sectors written */
   unsigned long long wr_sectors		__attribute__ ((packed));
   /* Time of write requests in queue */
   unsigned long wr_ticks			__attribute__ ((packed));
   /* # of I/Os in progress */
   unsigned long ios_pgr			__attribute__ ((packed));
   /* # of ticks total (for this device) for I/O */
   unsigned long tot_ticks			__attribute__ ((packed));
   /* # of ticks requests spent in queue */
   unsigned long rq_ticks			__attribute__ ((packed));
   /* # of I/O done since last reboot */
   unsigned long dk_drive			__attribute__ ((packed));
   /* # of blocks read */
   unsigned long dk_drive_rblk			__attribute__ ((packed));
   /* # of blocks written */
   unsigned long dk_drive_wblk			__attribute__ ((packed));
};

#define IO_STATS_SIZE	(sizeof(struct io_stats))

struct io_hdr_stats {
   unsigned int  active				__attribute__ ((aligned (8)));
   unsigned int  used				__attribute__ ((packed));
   char name[MAX_NAME_LEN]		        __attribute__ ((packed));
};

#define IO_HDR_STATS_SIZE	(sizeof(struct io_hdr_stats))

/* List of devices entered on the command line */
struct io_dlist {
   /* Indicate whether its partitions are to be displayed or not */
   int  disp_part				__attribute__ ((aligned (8)));
   /* Device name */
   char dev_name[MAX_NAME_LEN]			__attribute__ ((packed));
};

#define IO_DLIST_SIZE	(sizeof(struct io_dlist))

#endif  /* _IOSTAT_H */
