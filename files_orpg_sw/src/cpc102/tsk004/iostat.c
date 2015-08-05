/*
 * RCS info
 * $Author: cmn $
 * $Locker:  $
 * $Date: 2011/05/22 16:48:18 $
 * $Id: iostat.c,v 1.1 2011/05/22 16:48:18 cmn Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */
/*
 * iostat: report CPU and I/O statistics
 * (C) 1998-2006 by Sebastien GODARD (sysstat <at> wanadoo.fr)
 *
 ***************************************************************************
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published  by  the *
 * Free Software Foundation; either version 2 of the License, or (at  your *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it  will  be  useful,  but *
 * WITHOUT ANY WARRANTY; without the implied warranty  of  MERCHANTABILITY *
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License *
 * for more details.                                                       *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA                   *
 ***************************************************************************
 */

/*
  NOTE:  This source file has been modified specifically for use within
         the RPG and/or RDA of the WSR-88D.  It would be totally useless
         to other applications.  
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utsname.h>

#include "iostat.h"
#include "common.h"
#include "ioconf.h"

#include <perf_mon.h>


#ifdef USE_NLS
#include <locale.h>
#include <libintl.h>
#define _(string) gettext(string)
#else
#define _(string) (string)
#endif

/* Type Definitions. */
typedef struct Disk_stats {

   char device[8];            /* Disk device name. */

   float read_requests;       /* Number of read requests issued to device per second. */

   float write_requests;      /* Number of write requests issued to device per second. */

   float kb_read;             /* Number of kilobytes read from device per second. */

   float kb_write;            /* Number of kilobytes written to device per second. */

   float await;               /* Average time (in milliseconds) for I/O requests issued to the 
                                 device to be served.   This includes the time spent by the 
                                 requests in queue and the time spent servicing them. */

   float svctm;               /* The average service time (in milliseconds) for I/O requests
                                 that were issued to the device. */

   float util;                /* Percentage of CPU time during which I/O requests were issued
                                 to the device (bandwidth utilization for the device).  Device
                                 saturation occurs when this value is close to 100%. */

} Disk_stats_t;


static struct comm_stats  comm_stats[2];
static struct io_stats *st_iodev[2];
static struct io_hdr_stats *st_hdr_iodev;
static struct io_dlist *st_dev_list;

/* Nb of devices and partitions found */
static int iodev_nr = 0;

/* Nb of devices entered on the command line */
static int dlist_idx = 0;

static unsigned char timestamp[64];

/* Nb of processors on the machine */
static int cpu_nr = 0;

/* File handle for disk utilization. */
static int Disk_fd = -1;

/* Disk statistics. */
static Disk_stats_t Disk_usage;

static char Buffer[256];

/* Used for long-term averages. */
static double Total_kb_write = 0;
static double Total_kb_read = 0;
static double Total_await = 0;
static double Total_svctm = 0;
static double Total_util = 0;
static double Total_reports = 0;

/* Used for volume averages. */
static double Volume_kb_write = 0;
static double Volume_kb_read = 0;
static double Volume_await = 0;
static double Volume_svctm = 0;
static double Volume_util = 0;
static unsigned int Volume_time = 0;
static int Elev_cnt = 0;


/* Global Variables. */
extern Scan_info_t Scan_info;
extern Scan_info_t Prev_scan_info;
extern int Volume_num;
extern time_t Delta_time;
extern time_t Start_time;
extern struct utsname *Uname_info;
extern int Verbose;

/* Function Prototypes. */
static void End_of_volume( int year, int mon, int day, int hour, int min, int sec,
                           int terminate );
void DS_cleanup();


/*
 ***************************************************************************
 * Initialize stats common structures
 ***************************************************************************
 */
void init_stats(void)
{
   memset(&comm_stats[0], 0, COMM_STATS_SIZE);
   memset(&comm_stats[1], 0, COMM_STATS_SIZE);
}


/*
 ***************************************************************************
 * Set every disk_io entry to inactive state
 ***************************************************************************
 */
void set_entries_inactive(int iodev_nr)
{
   int i;
   struct io_hdr_stats *shi = st_hdr_iodev;

   for (i = 0; i < iodev_nr; i++, shi++)
      shi->active = FALSE;
}


/*
 ***************************************************************************
 * Set structures's state to free for inactive entries
 ***************************************************************************
 */
void free_inactive_entries(int iodev_nr)
{
   int i;
   struct io_hdr_stats *shi = st_hdr_iodev;

   for (i = 0; i < iodev_nr; i++, shi++) {
      if (!shi->active)
	 shi->used = FALSE;
   }
}


/*
 ***************************************************************************
 * Allocate and init I/O devices structures
 ***************************************************************************
 */
void salloc_device(int iodev_nr)
{
   int i;

   for (i = 0; i < 2; i++) {
      if ((st_iodev[i] = (struct io_stats *) malloc(IO_STATS_SIZE * iodev_nr)) == NULL) {
	 perror("malloc");
	 exit(4);
      }
      memset(st_iodev[i], 0, IO_STATS_SIZE * iodev_nr);
   }

   if ((st_hdr_iodev = (struct io_hdr_stats *) malloc(IO_HDR_STATS_SIZE * iodev_nr)) == NULL) {
      perror("malloc");
      exit(4);
   }
   memset(st_hdr_iodev, 0, IO_HDR_STATS_SIZE * iodev_nr);
}


/*
 ***************************************************************************
 * Allocate structures for devices entered on the command line
 ***************************************************************************
 */
void salloc_dev_list(int list_len)
{
   if ((st_dev_list = (struct io_dlist *) malloc(IO_DLIST_SIZE * list_len)) == NULL) {
      perror("malloc");
      exit(4);
   }
   memset(st_dev_list, 0, IO_DLIST_SIZE * list_len);
}


/*
 ***************************************************************************
 * Look for the device in the device list and store it if necessary.
 * Returns the position of the device in the list.
 ***************************************************************************
 */
int update_dev_list(int *dlist_idx, char *device_name)
{
   int i;
   struct io_dlist *sdli = st_dev_list;

   for (i = 0; i < *dlist_idx; i++, sdli++) {
      if (!strcmp(sdli->dev_name, device_name))
	 break;
   }

   if (i == *dlist_idx) {
      /* Device not found: store it */
      (*dlist_idx)++;
      strncpy(sdli->dev_name, device_name, MAX_NAME_LEN - 1);
   }

   return i;
}


/*
 ***************************************************************************
 * Allocate and init structures, according to system state
 ***************************************************************************
 */
void io_sys_init(int *flags)
{
   int i;

   /* Init stat common counters */
   init_stats();

   /* How many processors on this machine ? */
   cpu_nr = get_cpu_nr(~0);

   /* Get number of block devices and partitions in /proc/diskstats */
   if ((iodev_nr = get_diskstats_dev_nr(CNT_PART, CNT_ALL_DEV)) > 0) {
      *flags |= I_F_HAS_DISKSTATS;
      iodev_nr += NR_DEV_PREALLOC;
   }

   if (!HAS_DISKSTATS(*flags) ||
       (DISPLAY_PARTITIONS(*flags) && !DISPLAY_PART_ALL(*flags))) {
      /*
       * If /proc/diskstats exists but we also want stats for the partitions
       * of a particular device, stats will have to be found in /sys. So we
       * need to know if /sys is mounted or not, and set *flags accordingly.
       */

      /* Get number of block devices (and partitions) in sysfs */
      if ((iodev_nr = get_sysfs_dev_nr(DISPLAY_PARTITIONS(*flags))) > 0) {
	 *flags |= I_F_HAS_SYSFS;
	 iodev_nr += NR_DEV_PREALLOC;
      }
      /*
       * Get number of block devices and partitions in /proc/partitions,
       * those with statistics...
       */
      else if ((iodev_nr = get_ppartitions_dev_nr(CNT_PART)) > 0) {
	 *flags |= I_F_HAS_PPARTITIONS;
	 iodev_nr += NR_DEV_PREALLOC;
      }
      /* Get number of "disk_io:" entries in /proc/stat */
      else if ((iodev_nr = get_disk_io_nr()) > 0) {
	 *flags |= I_F_PLAIN_KERNEL24;
	 iodev_nr += NR_DISK_PREALLOC;
      }
      else {
	 /* Assume we have an old kernel: stats for 4 disks are in /proc/stat */
	 iodev_nr = 4;
	 *flags |= I_F_OLD_KERNEL;
      }
   }
   /*
    * Allocate structures for number of disks found.
    * iodev_nr must be <> 0.
    */
   salloc_device(iodev_nr);

   if (HAS_OLD_KERNEL(*flags)) {
      struct io_hdr_stats *shi = st_hdr_iodev;
      /*
       * If we have an old kernel with the stats for the first four disks
       * in /proc/stat, then set the devices names to hdisk[0..3].
       */
      for (i = 0; i < 4; i++, shi++) {
	 shi->used = TRUE;
	 sprintf(shi->name, "%s%d", K_HDISK, i);
      }
   }
}


/*
 ***************************************************************************
 * Save stats for current device or partition
 ***************************************************************************
 */
void save_dev_stats(char *dev_name, int curr, struct io_stats *sdev)
{
   int i;
   struct io_hdr_stats *st_hdr_iodev_i;
   struct io_stats *st_iodev_i;

   /* Look for device in data table */
   for (i = 0; i < iodev_nr; i++) {
      st_hdr_iodev_i = st_hdr_iodev + i;
      if (!strcmp(st_hdr_iodev_i->name, dev_name)) {
	 break;
      }
   }
	
   if (i == iodev_nr) {
      /*
       * This is a new device: look for an unused entry to store it.
       * Thus we are able to handle dynamically registered devices.
       */
      for (i = 0; i < iodev_nr; i++) {
	 st_hdr_iodev_i = st_hdr_iodev + i;
	 if (!st_hdr_iodev_i->used) {
	    /* Unused entry found... */
	    st_hdr_iodev_i->used = TRUE;	/* Indicate it is now used */
	    strcpy(st_hdr_iodev_i->name, dev_name);
	    st_iodev_i = st_iodev[!curr] + i;
	    memset(st_iodev_i, 0, IO_STATS_SIZE);
	    break;
	 }
      }
   }
   if (i < iodev_nr) {
      st_hdr_iodev_i = st_hdr_iodev + i;
      st_hdr_iodev_i->active = TRUE;
      st_iodev_i = st_iodev[curr] + i;
      *st_iodev_i = *sdev;
   }
   /* else it was a new device but there was no free structure to store it */
}


/*
 ***************************************************************************
 * Read stats from /proc/stat file...
 * Useful at least for CPU utilization.
 * May be useful to get disk stats if /sys not available.
 ***************************************************************************
 */
void read_proc_stat(int curr, int flags)
{
   FILE *fp;
   char line[8192];
   int pos, i;
   unsigned long v_tmp[4];
   unsigned int v_major, v_index;
   struct io_stats *st_iodev_tmp[4];
   unsigned long long cc_idle, cc_iowait, cc_steal;
   unsigned long long cc_user, cc_nice, cc_system, cc_hardirq, cc_softirq;


   /*
    * Prepare pointers on the 4 disk structures in case we have a
    * /proc/stat file with "disk_rblk", etc. entries.
    */
   for (i = 0; i < 4; i++)
      st_iodev_tmp[i] = st_iodev[curr] + i;

   if ((fp = fopen(STAT, "r")) == NULL) {
      perror("fopen");
      exit(2);
   }

   while (fgets(line, 8192, fp) != NULL) {

      if (!strncmp(line, "cpu ", 4)) {
	 /*
	  * Read the number of jiffies spent in the different modes,
	  * and compute system uptime in jiffies (1/100ths of a second
	  * if HZ=100).
	  * Some fields are only present in 2.6 kernels.
	  */
	 comm_stats[curr].cpu_iowait = 0;	/* For pre 2.6 kernels */
	 comm_stats[curr].cpu_steal = 0;
	 cc_hardirq = cc_softirq = 0;
	 /* CPU counters became unsigned long long with kernel 2.6.5 */
	 sscanf(line + 5, "%llu %llu %llu %llu %llu %llu %llu %llu",
	        &(comm_stats[curr].cpu_user), &(comm_stats[curr].cpu_nice),
		&(comm_stats[curr].cpu_system), &(comm_stats[curr].cpu_idle),
		&(comm_stats[curr].cpu_iowait), &cc_hardirq, &cc_softirq,
		&(comm_stats[curr].cpu_steal));

	 /*
	  * Time spent in system mode also includes time spent servicing
	  * hard interrupts and softirqs.
	  */
	 comm_stats[curr].cpu_system += cc_hardirq + cc_softirq;
	
	 /*
	  * Compute system uptime in jiffies.
	  * Uptime is multiplied by the number of processors.
	  */
	 comm_stats[curr].uptime = comm_stats[curr].cpu_user +
	                           comm_stats[curr].cpu_nice +
	                           comm_stats[curr].cpu_system +
	                           comm_stats[curr].cpu_idle +
	                           comm_stats[curr].cpu_iowait +
	    			   comm_stats[curr].cpu_steal;
      }

      else if ((!strncmp(line, "cpu0", 4)) && (cpu_nr > 1)) {
	 /*
	  * Read CPU line for proc#0 (if available).
	  * This is necessary to compute time interval since
	  * processors may be disabled (offline) sometimes.
	  * (Assume that proc#0 can never be offline).
	  */
	 cc_iowait = cc_hardirq = cc_softirq = cc_steal = 0;
	 sscanf(line + 5, "%llu %llu %llu %llu %llu %llu %llu %llu",
		&cc_user, &cc_nice, &cc_system, &cc_idle, &cc_iowait,
		&cc_hardirq, &cc_softirq, &cc_steal);
	 comm_stats[curr].uptime0 = cc_user + cc_nice + cc_system +
	    			    cc_idle + cc_iowait +
	    			    cc_hardirq + cc_softirq + cc_steal;
      }

      else if (DISPLAY_EXTENDED(flags) || HAS_DISKSTATS(flags) ||
	       HAS_PPARTITIONS(flags) || HAS_SYSFS(flags))
	 /*
	  * When displaying extended statistics, or if /proc/diskstats or
	  * /proc/partitions exists, or /sys is mounted,
	  * we just need to get CPU info from /proc/stat.
	  */
	 continue;

      else if (!strncmp(line, "disk_rblk ", 10)) {
	 /*
	  * Read the number of blocks read from disk.
	  * A block is of indeterminate size.
	  * The size may vary depending on the device type.
	  */
	 sscanf(line + 10, "%lu %lu %lu %lu",
		&v_tmp[0], &v_tmp[1], &v_tmp[2], &v_tmp[3]);

	 st_iodev_tmp[0]->dk_drive_rblk = v_tmp[0];
	 st_iodev_tmp[1]->dk_drive_rblk = v_tmp[1];
	 st_iodev_tmp[2]->dk_drive_rblk = v_tmp[2];
	 st_iodev_tmp[3]->dk_drive_rblk = v_tmp[3];
      }

      else if (!strncmp(line, "disk_wblk ", 10)) {
	 /* Read the number of blocks written to disk */
	 sscanf(line + 10, "%lu %lu %lu %lu",
		&v_tmp[0], &v_tmp[1], &v_tmp[2], &v_tmp[3]);
	
	 st_iodev_tmp[0]->dk_drive_wblk = v_tmp[0];
	 st_iodev_tmp[1]->dk_drive_wblk = v_tmp[1];
	 st_iodev_tmp[2]->dk_drive_wblk = v_tmp[2];
	 st_iodev_tmp[3]->dk_drive_wblk = v_tmp[3];
      }

      else if (!strncmp(line, "disk ", 5)) {
	 /* Read the number of I/O done since the last reboot */
	 sscanf(line + 5, "%lu %lu %lu %lu",
		&v_tmp[0], &v_tmp[1], &v_tmp[2], &v_tmp[3]);
	
	 st_iodev_tmp[0]->dk_drive = v_tmp[0];
	 st_iodev_tmp[1]->dk_drive = v_tmp[1];
	 st_iodev_tmp[2]->dk_drive = v_tmp[2];
	 st_iodev_tmp[3]->dk_drive = v_tmp[3];
      }

      else if (!strncmp(line, "disk_io: ", 9)) {
	 struct io_stats sdev;
	 char dev_name[MAX_NAME_LEN];
	
	 pos = 9;

	 /* Every disk_io entry is potentially unregistered */
	 set_entries_inactive(iodev_nr);
	
	 /* Read disks I/O statistics (for 2.4 kernels) */
	 while (pos < strlen(line) - 1) {
	    /* Beware: a CR is already included in the line */
	    sscanf(line + pos, "(%u,%u):(%lu,%*u,%lu,%*u,%lu) ",
		   &v_major, &v_index, &v_tmp[0], &v_tmp[1], &v_tmp[2]);

	    sprintf(dev_name, "dev%d-%d", v_major, v_index);
	    sdev.dk_drive      = v_tmp[0];
	    sdev.dk_drive_rblk = v_tmp[1];
	    sdev.dk_drive_wblk = v_tmp[2];
	    save_dev_stats(dev_name, curr, &sdev);

	    pos += strcspn(line + pos, " ") + 1;
	 }

	 /* Free structures corresponding to unregistered disks */
	 free_inactive_entries(iodev_nr);
      }
   }

   fclose(fp);
}


/*
 ***************************************************************************
 * Read sysfs stat for current block device or partition
 ***************************************************************************
 */
int read_sysfs_file_stat(int curr, char *filename, char *dev_name,
			  int dev_type)
{
   FILE *fp;
   struct io_stats sdev;
   int i;

   /* Try to read given stat file */
   if ((fp = fopen(filename, "r")) == NULL)
      return 0;
	
   if (dev_type == DT_DEVICE)
      i = (fscanf(fp, "%lu %lu %llu %lu %lu %lu %llu %lu %lu %lu %lu",
		  &sdev.rd_ios, &sdev.rd_merges,
		  &sdev.rd_sectors, &sdev.rd_ticks,
		  &sdev.wr_ios, &sdev.wr_merges,
		  &sdev.wr_sectors, &sdev.wr_ticks,
		  &sdev.ios_pgr, &sdev.tot_ticks, &sdev.rq_ticks) == 11);
   else
      i = (fscanf(fp, "%lu %llu %lu %llu",
		  &sdev.rd_ios, &sdev.rd_sectors,
		  &sdev.wr_ios, &sdev.wr_sectors) == 4);

   if (i)
      save_dev_stats(dev_name, curr, &sdev);

   fclose(fp);

   return 1;
}


/*
 ***************************************************************************
 * Read sysfs stats for all the partitions of a device
 ***************************************************************************
 */
void read_sysfs_dlist_part_stat(int curr, char *dev_name)
{
   DIR *dir;
   struct dirent *drd;
   char dfile[MAX_PF_NAME], filename[MAX_PF_NAME];

   sprintf(dfile, "%s/%s", SYSFS_BLOCK, dev_name);

   /* Open current device directory in /sys/block */
   if ((dir = opendir(dfile)) == NULL)
      return;

   /* Get current entry */
   while ((drd = readdir(dir)) != NULL) {
      if (!strcmp(drd->d_name, ".") || !strcmp(drd->d_name, ".."))
	 continue;
      sprintf(filename, "%s/%s/%s", dfile, drd->d_name, S_STAT);

      /* Read current partition stats */
      read_sysfs_file_stat(curr, filename, drd->d_name, DT_PARTITION);
   }

   /* Close device directory */
   closedir(dir);
}


/*
 ***************************************************************************
 * Read stats from the sysfs filesystem
 * for the devices entered on the command line
 ***************************************************************************
 */
void read_sysfs_dlist_stat(int curr, int flags)
{
   int dev, ok;
   char filename[MAX_PF_NAME];
   struct io_dlist *st_dev_list_i;

   /* Every I/O device (or partition) is potentially unregistered */
   set_entries_inactive(iodev_nr);

   for (dev = 0; dev < dlist_idx; dev++) {
      st_dev_list_i = st_dev_list + dev;
      sprintf(filename, "%s/%s/%s",
	      SYSFS_BLOCK, st_dev_list_i->dev_name, S_STAT);

      /* Read device stats */
      ok = read_sysfs_file_stat(curr, filename, st_dev_list_i->dev_name, DT_DEVICE);

      if (ok && st_dev_list_i->disp_part)
	 /* Also read stats for its partitions */
	 read_sysfs_dlist_part_stat(curr, st_dev_list_i->dev_name);
   }

   /* Free structures corresponding to unregistered devices */
   free_inactive_entries(iodev_nr);
}


/*
 ***************************************************************************
 * Read stats from the sysfs filesystem
 * for every block devices found
 ***************************************************************************
 */
void read_sysfs_stat(int curr, int flags)
{
   DIR *dir;
   struct dirent *drd;
   char filename[MAX_PF_NAME];
   int ok;

   /* Every I/O device entry is potentially unregistered */
   set_entries_inactive(iodev_nr);

   /* Open /sys/block directory */
   if ((dir = opendir(SYSFS_BLOCK)) != NULL) {

      /* Get current entry */
      while ((drd = readdir(dir)) != NULL) {
	 if (!strcmp(drd->d_name, ".") || !strcmp(drd->d_name, ".."))
	    continue;
	 sprintf(filename, "%s/%s/%s", SYSFS_BLOCK, drd->d_name, S_STAT);
	
	 /* If current entry is a directory, try to read its stat file */
	 ok = read_sysfs_file_stat(curr, filename, drd->d_name, DT_DEVICE);
	
	 /*
	  * If '-p ALL' was entered on the command line,
	  * also try to read stats for its partitions
	  */
	 if (ok && DISPLAY_PART_ALL(flags))
	    read_sysfs_dlist_part_stat(curr, drd->d_name);
      }

      /* Close /sys/block directory */
      closedir(dir);
   }

   /* Free structures corresponding to unregistered devices */
   free_inactive_entries(iodev_nr);
}


/*
 ***************************************************************************
 * Read stats from /proc/diskstats
 ***************************************************************************
 */
void read_diskstats_stat(int curr, int flags)
{
   FILE *fp;
   char line[256], dev_name[MAX_NAME_LEN];
   struct io_stats sdev;
   int i;
   unsigned long rd_ios, rd_merges_or_rd_sec, rd_ticks_or_wr_sec, wr_ios;
   unsigned long ios_pgr, tot_ticks, rq_ticks, wr_merges, wr_ticks;
   unsigned long long rd_sec_or_wr_ios, wr_sec;
   char *ioc_dname;
   unsigned int major, minor;

   /* Every I/O device entry is potentially unregistered */
   set_entries_inactive(iodev_nr);

   if ((fp = fopen(DISKSTATS, "r")) == NULL)
      return;

   while (fgets(line, 256, fp) != NULL) {

      /* major minor name rio rmerge rsect ruse wio wmerge wsect wuse running use aveq */
      i = sscanf(line, "%u %u %s %lu %lu %llu %lu %lu %lu %llu %lu %lu %lu %lu",
		 &major, &minor, dev_name,
		 &rd_ios, &rd_merges_or_rd_sec, &rd_sec_or_wr_ios, &rd_ticks_or_wr_sec,
		 &wr_ios, &wr_merges, &wr_sec, &wr_ticks, &ios_pgr, &tot_ticks, &rq_ticks);

      if (i == 14) {
	 /* Device */
	 sdev.rd_ios     = rd_ios;
	 sdev.rd_merges  = rd_merges_or_rd_sec;
	 sdev.rd_sectors = rd_sec_or_wr_ios;
	 sdev.rd_ticks   = rd_ticks_or_wr_sec;
	 sdev.wr_ios     = wr_ios;
	 sdev.wr_merges  = wr_merges;
	 sdev.wr_sectors = wr_sec;
	 sdev.wr_ticks   = wr_ticks;
	 sdev.ios_pgr    = ios_pgr;
	 sdev.tot_ticks  = tot_ticks;
	 sdev.rq_ticks   = rq_ticks;
      }
      else if (i == 7) {
	 /* Partition */
	 if (DISPLAY_EXTENDED(flags) || (!dlist_idx && !DISPLAY_PARTITIONS(flags)))
	    continue;

	 sdev.rd_ios     = rd_ios;
	 sdev.rd_sectors = rd_merges_or_rd_sec;
	 sdev.wr_ios     = rd_sec_or_wr_ios;
	 sdev.wr_sectors = rd_ticks_or_wr_sec;
      }
      else
	 /* Unknown entry: ignore it */
	 continue;

      if ((ioc_dname = ioc_name(major, minor)) != NULL) {
	 if (strcmp(dev_name, ioc_dname) && strcmp(ioc_dname, K_NODEV))
	    /*
	     * No match: Use name generated from sysstat.ioconf data (if different
	     * from "nodev") works around known issues with EMC PowerPath.
	     */
	    strcpy(dev_name, ioc_dname);
      }

      save_dev_stats(dev_name, curr, &sdev);
   }

   fclose(fp);

   /* Free structures corresponding to unregistered devices */
   free_inactive_entries(iodev_nr);
}


/*
 ***************************************************************************
 * Read stats from /proc/partitions
 ***************************************************************************
 */
void read_ppartitions_stat(int curr, int flags)
{
   FILE *fp;
   char line[256], dev_name[MAX_NAME_LEN];
   struct io_stats sdev;
   unsigned long rd_ios, rd_merges, rd_ticks, wr_ios, wr_merges, wr_ticks;
   unsigned long ios_pgr, tot_ticks, rq_ticks;
   unsigned long long rd_sec, wr_sec;
   char *ioc_dname;
   unsigned int major, minor;

   /* Every I/O device entry is potentially unregistered */
   set_entries_inactive(iodev_nr);

   if ((fp = fopen(PPARTITIONS, "r")) == NULL)
      return;

   while (fgets(line, 256, fp) != NULL) {
      /* major minor #blocks name rio rmerge rsect ruse wio wmerge wsect wuse running use aveq */
      if (sscanf(line, "%u %u %*u %s %lu %lu %llu %lu %lu %lu %llu"
		       " %lu %lu %lu %lu",
		 &major, &minor, dev_name,
		 &rd_ios, &rd_merges, &rd_sec, &rd_ticks, &wr_ios, &wr_merges,
		 &wr_sec, &wr_ticks, &ios_pgr, &tot_ticks, &rq_ticks) == 14) {
	 /* Device or partition */
	 sdev.rd_ios     = rd_ios;  sdev.rd_merges = rd_merges;
	 sdev.rd_sectors = rd_sec;  sdev.rd_ticks  = rd_ticks;
	 sdev.wr_ios     = wr_ios;  sdev.wr_merges = wr_merges;
	 sdev.wr_sectors = wr_sec;  sdev.wr_ticks  = wr_ticks;
	 sdev.ios_pgr    = ios_pgr; sdev.tot_ticks = tot_ticks;
	 sdev.rq_ticks   = rq_ticks;
      }
      else
	 /* Unknown entry: ignore it */
	 continue;

      if ((ioc_dname = ioc_name(major, minor)) != NULL) {
	 if (strcmp(dev_name, ioc_dname) && strcmp(ioc_dname, K_NODEV))
	    /* Compensate for EMC PowerPath driver bug */
	    strcpy(dev_name, ioc_dname);
      }

      save_dev_stats(dev_name, curr, &sdev);
   }

   fclose(fp);

   /* Free structures corresponding to unregistered devices */
   free_inactive_entries(iodev_nr);
}


/*
 ***************************************************************************
 * Display CPU utilization
 ***************************************************************************
 */
void write_cpu_stat(int curr, unsigned long long itv)
{
   printf("avg-cpu:  %%user   %%nice %%system %%iowait  %%steal   %%idle\n");

   printf("         %6.2f  %6.2f  %6.2f  %6.2f  %6.2f  %6.2f\n\n",
	  ll_sp_value(comm_stats[!curr].cpu_user,   comm_stats[curr].cpu_user,   itv),
	  ll_sp_value(comm_stats[!curr].cpu_nice,   comm_stats[curr].cpu_nice,   itv),
	  ll_sp_value(comm_stats[!curr].cpu_system, comm_stats[curr].cpu_system, itv),
	  ll_sp_value(comm_stats[!curr].cpu_iowait, comm_stats[curr].cpu_iowait, itv),
	  ll_sp_value(comm_stats[!curr].cpu_steal,  comm_stats[curr].cpu_steal,  itv),
	  (comm_stats[curr].cpu_idle < comm_stats[!curr].cpu_idle) ?
	  0.0 :
	  ll_sp_value(comm_stats[!curr].cpu_idle, comm_stats[curr].cpu_idle, itv));
}


/*
 ***************************************************************************
 * Display stats header
 ***************************************************************************
 */
void write_stat_header(int flags, int *fctr)
{
   if (DISPLAY_EXTENDED(flags)) {
      /* Extended stats */
/*      printf("Device:         rrqm/s   wrqm/s   r/s   w/s"); */
      if (DISPLAY_MEGABYTES(flags)) {
/*	 printf("    rMB/s    wMB/s"); */
	 *fctr = 2048;
      }
      else if (DISPLAY_KILOBYTES(flags)) {
/*	 printf("    rkB/s    wkB/s"); */
	 *fctr = 2;
      }
/*      else
	 printf("   rsec/s   wsec/s");
      printf(" avgrq-sz avgqu-sz   await  svctm  %%util\n");
*/
   }
   else {
      /* Basic stats */
/*      printf("Device:            tps"); */
      if (DISPLAY_KILOBYTES(flags)) {
/*	 printf("    kB_read/s    kB_wrtn/s    kB_read    kB_wrtn\n"); */
	 *fctr = 2;
      }
      else if (DISPLAY_MEGABYTES(flags)) {
/*	 printf("    MB_read/s    MB_wrtn/s    MB_read    MB_wrtn\n"); */
	 *fctr = 2048;
      }
/*
      else
	 printf("   Blk_read/s   Blk_wrtn/s   Blk_read   Blk_wrtn\n");
*/
   }
}


/*
 ***************************************************************************
 * Display extended stats, read from /proc/{diskstats,partitions} or /sys
 ***************************************************************************
 */
void write_ext_stat(int curr, unsigned long long itv, int flags, int fctr,
		    struct io_hdr_stats *shi, struct io_stats *ioi,
		    struct io_stats *ioj, int scan_type, int first_time )
{
   unsigned long long rd_sec, wr_sec;
   double tput, util, await, svctm, arqsz, nr_ios;

   int size, year, mon, day, hour, min, sec;
	
   /*
    * Counters overflows are possible, but don't need to be handled in
    * a special way: the difference is still properly calculated if the
    * result is of the same type as the two values.
    * Exception is field rq_ticks which is incremented by the number of
    * I/O in progress times the number of milliseconds spent doing I/O.
    * But the number of I/O in progress (field ios_pgr) happens to be
    * sometimes negative...
    */
   nr_ios = (ioi->rd_ios - ioj->rd_ios) + (ioi->wr_ios - ioj->wr_ios);
   tput = ((double) nr_ios) * HZ / itv;
   util = S_VALUE(ioj->tot_ticks, ioi->tot_ticks, itv);
   svctm = tput ? util / tput : 0.0;
   /*
    * kernel gives ticks already in milliseconds for all platforms
    * => no need for further scaling.
    */
   await = nr_ios ?
      ((ioi->rd_ticks - ioj->rd_ticks) + (ioi->wr_ticks - ioj->wr_ticks)) /
      nr_ios : 0.0;

   rd_sec = ioi->rd_sectors - ioj->rd_sectors;
   if ((ioi->rd_sectors < ioj->rd_sectors) && (ioj->rd_sectors <= 0xffffffff))
      rd_sec &= 0xffffffff;
   wr_sec = ioi->wr_sectors - ioj->wr_sectors;
   if ((ioi->wr_sectors < ioj->wr_sectors) && (ioj->wr_sectors <= 0xffffffff))
      wr_sec &= 0xffffffff;

   arqsz = nr_ios ? (rd_sec + wr_sec) / nr_ios : 0.0;

/*   printf("%-13s", shi->name); 
   if (strlen(shi->name) > 10)
      printf("\n          ");
*/
   memcpy( &Disk_usage.device, shi->name, strlen(shi->name) );

   /*       rrq/s wrq/s   r/s   w/s  rsec  wsec  rqsz  qusz await svctm %util */
/*   printf(" %8.2f %8.2f %5.2f %5.2f %8.2f %8.2f %8.2f %8.2f %7.2f %6.2f %6.2f\n",
	  S_VALUE(ioj->rd_merges, ioi->rd_merges, itv),
	  S_VALUE(ioj->wr_merges, ioi->wr_merges, itv),
	  S_VALUE(ioj->rd_ios, ioi->rd_ios, itv),
	  S_VALUE(ioj->wr_ios, ioi->wr_ios, itv),
	  ll_s_value(ioj->rd_sectors, ioi->rd_sectors, itv) / fctr,
	  ll_s_value(ioj->wr_sectors, ioi->wr_sectors, itv) / fctr,
	  arqsz,
	  S_VALUE(ioj->rq_ticks, ioi->rq_ticks, itv) / 1000.0,
	  await,
*/
	  /* The ticks output is biased to output 1000 ticks per second */
/*
	  svctm,
*/
	  /* Again: ticks in milliseconds */
/*
	  util / 10.0);
*/

    Disk_usage.read_requests = S_VALUE(ioj->rd_ios, ioi->rd_ios, itv);
    Disk_usage.write_requests = S_VALUE(ioj->wr_ios, ioi->wr_ios, itv);
    Disk_usage.kb_read = ll_s_value(ioj->rd_sectors, ioi->rd_sectors, itv) / fctr;
    Disk_usage.kb_write = ll_s_value(ioj->wr_sectors, ioi->wr_sectors, itv) / fctr;
    Disk_usage.await = await;
    Disk_usage.svctm = svctm;
    Disk_usage.util = util / 10.0;

    Total_kb_read += Disk_usage.kb_read;
    Total_kb_write += Disk_usage.kb_write;
    Total_await += await;
    Total_svctm += svctm;
    Total_util += Disk_usage.util;
    Total_reports++;

    /* If the gnuplot file is open, write Disk stats to file. */
    if( Disk_fd < 0 )
       return;

    /* Did a start of volume event occur? */
    if( VOLUME_EVENT == scan_type ){

        unix_time( &Scan_info.scan_time, &year, &mon, &day, &hour, &min, &sec );
        if( year >= 2000 )
            year -= 2000;

        else
            year -= 1900;

        /* Do only the first time through. */
        if( first_time ){

          memset( Buffer, 0, sizeof(Buffer) );
          sprintf( Buffer, "#===========================================================\
==========================================\n" );

          size = strlen( Buffer );
          if( write( Disk_fd, Buffer, size ) != size )
              fprintf( stderr, "Write to Disk_fd Failed\n" );

           memset( Buffer, 0, sizeof(Buffer) );
           sprintf( Buffer, "# %02d/%02d/%02d %02d:%02d:%02d   VOLUME #: %4d   VCP: %3d\n",
                    mon, day, year, hour, min, sec, Volume_num, Scan_info.vcp );

           size = strlen( Buffer );
           if( write( Disk_fd, Buffer, size ) != size )
              fprintf( stderr, "Write to Disk_fd Failed\n" );

           memset( Buffer, 0, sizeof(Buffer) );
           sprintf( Buffer,"# Cut    Time (s)    Total (s)      Read (KB)    Write (KB)    Await (ms)  \
SvcTime (ms)      Util (%%)\n" );    

           size = strlen( Buffer );
           if( write( Disk_fd, Buffer, size ) != size )
               fprintf( stderr, "Write to Disk_fd Failed\n" );

        }

    }

    /* The first time through this function will be at start of volume.  We don't want
       to write data to file at this point because we want the data to reflect the
       information between the start of the 2nd cut and the start of volume. */
    if( !first_time ){

         memset( Buffer, 0, sizeof(Buffer) );
         sprintf( Buffer, 
                  " %3d  %12u    %4d        %8.2f      %8.2f       %7.2f        %6.2f         %6.2f\n", 
                  Prev_scan_info.rda_elev_num, (unsigned int) Scan_info.scan_time, 
                  (int) Delta_time, Disk_usage.kb_read, Disk_usage.kb_write, Disk_usage.await, 
                  Disk_usage.svctm, Disk_usage.util );

         size = strlen( Buffer );
         if( write( Disk_fd, Buffer, size ) != size )
             fprintf( stderr, "Write to Disk_fd Failed\n" );

         /* Increment the volume totals. */
         Volume_kb_write += Disk_usage.kb_write;
         Volume_kb_read += Disk_usage.kb_read;
         Volume_await += Disk_usage.await;
         Volume_svctm += Disk_usage.svctm;
         Volume_util += Disk_usage.util;
         Volume_time += Delta_time;
         Elev_cnt++;

         /* Print out the volume statistics at the start of volume event. */
         if( VOLUME_EVENT == scan_type )
             End_of_volume( year, mon, day, hour, min, sec, 0 );

        /* This information goes to the main terminal. */
        if( Verbose )
           fprintf( stderr, 
               "DISK--> Name: %8s, R-KB/s: %8.2f, W-KB/s: %8.2f, AWait: %7.2f (ms), \
Svc Time: %6.2f (ms), Util: %6.2f (%%)\n",
               Disk_usage.device, Disk_usage.kb_read, Disk_usage.kb_write, Disk_usage.await,
               Disk_usage.svctm, Disk_usage.util );

    }

}

/*\//////////////////////////////////////////////////////////////////////////
   
   Description:
      Handles end of volume scan processing.  

   Inputs:
      year - year in yyyy format
      mon - month in mm format
      day - day in dd format
      hour - hour in hh format
      min - minute in mm format
      sec - seconds in ss format
      terminate - 1 - yes, 0 - no

//////////////////////////////////////////////////////////////////////////\*/
static void End_of_volume( int year, int mon, int day, int hour, int min, int sec,
                           int terminate ){

    int size;
    double await = 0, svctm = 0, util = 0;

    memset( Buffer, 0, sizeof(Buffer) );
    sprintf( Buffer, "#-----------------------------------------------------------\
------------------------------------------\n" );

    size = strlen( Buffer );
    if( write( Disk_fd, Buffer, size ) != size )
        fprintf( stderr, "Write to Disk_fd Failed\n" );

    /* In the event Elev_cnt is 0. */
    if( Elev_cnt > 0 ){

       await = Volume_await / (double) Elev_cnt;
       svctm = Volume_svctm / (double) Elev_cnt;
       util = Volume_util / (double) Elev_cnt;

    }
      
    memset( Buffer, 0, sizeof(Buffer) );
    sprintf( Buffer, "# Stats:              %4d        %8.2f      %8.2f       %7.2f       %7.2f         %6.2f \n",
             Volume_time, Volume_kb_read, Volume_kb_write, await, svctm, util );

    size = strlen( Buffer );
    if( write( Disk_fd, Buffer, size ) != size )
        fprintf( stderr, "Write to Disk_fd Failed\n" );

    memset( Buffer, 0, sizeof(Buffer) );
    sprintf( Buffer, "#===========================================================\
==========================================\n" );

    size = strlen( Buffer );
    if( write( Disk_fd, Buffer, size ) != size )
        fprintf( stderr, "Write to Disk_fd Failed\n" );

   /* If terminate flag is set, no more volume scan coming. */
   if( terminate )
      return;

    /* Reset values for next volume. */
    Volume_kb_write = 0;
    Volume_kb_read = 0;
    Volume_await = 0;
    Volume_svctm = 0;
    Volume_util = 0;
    Volume_time = 0;
    Elev_cnt = 0;

    /* Write out the Disk stats header for next volume scan. */
    memset( Buffer, 0, sizeof(Buffer) );
    sprintf( Buffer, "# %02d/%02d/%02d %02d:%02d:%02d   VOLUME #: %4d   VCP: %3d\n",
             mon, day, year, hour, min, sec, Volume_num, Scan_info.vcp );

    size = strlen( Buffer );
    if( write( Disk_fd, Buffer, size ) != size )
        fprintf( stderr, "Write to Disk_fd Failed\n" );

    memset( Buffer, 0, sizeof(Buffer) );
    sprintf( Buffer,"# Cut    Time (s)    Total (s)      Read (KB)    Write (KB)    Await (ms)  \
SvcTime (ms)      Util (%%)\n" );

    size = strlen( Buffer );
    if( write( Disk_fd, Buffer, size ) != size )
        fprintf( stderr, "Write to Disk_fd Failed\n" );

/* End of End_of_volume(). */
}

/*
 ***************************************************************************
 * Write basic stats, read from /proc/stat, /proc/{diskstats,partitions}
 * or from sysfs
 ***************************************************************************
 */
void write_basic_stat(int curr, unsigned long long itv, int flags, int fctr,
		      struct io_hdr_stats *shi, struct io_stats *ioi,
		      struct io_stats *ioj)
{
   unsigned long long rd_sec, wr_sec;

   printf("%-13s", shi->name);
   if (strlen(shi->name) > 13)
      printf("\n             ");

   if (HAS_SYSFS(flags) ||
       HAS_DISKSTATS(flags) || HAS_PPARTITIONS(flags)) {
      /* Print stats coming from /sys or /proc/{diskstats,partitions} */
      rd_sec = ioi->rd_sectors - ioj->rd_sectors;
      if ((ioi->rd_sectors < ioj->rd_sectors) && (ioj->rd_sectors <= 0xffffffff))
	 rd_sec &= 0xffffffff;
      wr_sec = ioi->wr_sectors - ioj->wr_sectors;
      if ((ioi->wr_sectors < ioj->wr_sectors) && (ioj->wr_sectors <= 0xffffffff))
	 wr_sec &= 0xffffffff;

      printf(" %8.2f %12.2f %12.2f %10llu %10llu\n",
	     S_VALUE(ioj->rd_ios + ioj->wr_ios, ioi->rd_ios + ioi->wr_ios, itv),
	     ll_s_value(ioj->rd_sectors, ioi->rd_sectors, itv) / fctr,
	     ll_s_value(ioj->wr_sectors, ioi->wr_sectors, itv) / fctr,
	     (unsigned long long) rd_sec / fctr,
	     (unsigned long long) wr_sec / fctr);
   }
   else {
      /* Print stats coming from /proc/stat */
      printf(" %8.2f %12.2f %12.2f %10lu %10lu\n",
	     S_VALUE(ioj->dk_drive, ioi->dk_drive, itv),
	     S_VALUE(ioj->dk_drive_rblk, ioi->dk_drive_rblk, itv) / fctr,
	     S_VALUE(ioj->dk_drive_wblk, ioi->dk_drive_wblk, itv) / fctr,
	     (ioi->dk_drive_rblk - ioj->dk_drive_rblk) / fctr,
	     (ioi->dk_drive_wblk - ioj->dk_drive_wblk) / fctr);
   }
}


/*
 ***************************************************************************
 * Print everything now (stats and uptime)
 ***************************************************************************
 */
int write_stat(int curr, int flags, struct tm *rectime, int scan_type,
               int first_time )
{
   int dev, i, fctr = 1;
   unsigned long long itv;
   struct io_hdr_stats *shi = st_hdr_iodev;
   struct io_stats *ioi, *ioj;
   struct io_dlist *st_dev_list_i;

   /*
    * Under very special circumstances, STDOUT may become unavailable,
    * This is what we try to guess here
    */
   if (write(STDOUT_FILENO, "", 0) == -1) {
      perror("stdout");
      exit(6);
   }

   /* Print time stamp */
   if (DISPLAY_TIMESTAMP(flags)) {
      strftime(timestamp, sizeof(timestamp), "%X", rectime);
      printf(_("Time: %s\n"), timestamp);
   }

   /*
    * itv is multiplied by the number of processors.
    * This is OK to compute CPU usage since the number of jiffies spent in the
    * different modes (user, nice, etc.) is the sum of all the processors.
    * But itv should be reduced to one processor before displaying disk
    * utilization.
    */
   if (!comm_stats[!curr].uptime)
      /*
       * This is the first report displaying stats since system startup.
       * Only in this case we admit that the interval may be greater
       * than 0xffffffff, else it was an overflow.
       */
      itv = comm_stats[curr].uptime;
   else
      /* uptime in jiffies */
      itv = (comm_stats[curr].uptime - comm_stats[!curr].uptime)
	 & 0xffffffff;
   if (!itv)
      itv = 1;

   if (!DISPLAY_DISK_ONLY(flags))
      /* Display CPU utilization */
      write_cpu_stat(curr, itv);

   if (cpu_nr > 1) {
      /* On SMP machines, reduce itv to one processor (see note above) */
      if (!comm_stats[!curr].uptime0)
	 itv = comm_stats[curr].uptime0;
      else
	 itv = (comm_stats[curr].uptime0 - comm_stats[!curr].uptime0)
	    & 0xffffffff;
      if (!itv)
	 itv = 1;
   }

   if (!DISPLAY_CPU_ONLY(flags)) {

      /* Display stats header */
      write_stat_header(flags, &fctr); 

      if (DISPLAY_EXTENDED(flags) &&
	  (HAS_OLD_KERNEL(flags) || HAS_PLAIN_KERNEL24(flags))) {
	 /* No extended stats with old 2.2-2.4 kernels */
	 printf("\n");
	 return 1;
      }

      for (i = 0; i < iodev_nr; i++, shi++) {
	 if (shi->used) {
	
	    if (dlist_idx && !HAS_SYSFS(flags)) {
	       /*
		* With sysfs, only stats for the requested devices are read.
		* With /proc/{diskstats,partitions}, stats for every devices
		* are read. Thus we need to check if stats for current device
		* are to be displayed.
		*/
	       for (dev = 0; dev < dlist_idx; dev++) {
		  st_dev_list_i = st_dev_list + dev;
		  if (!strcmp(shi->name, st_dev_list_i->dev_name))
		     break;
	       }
	       if (dev == dlist_idx)
		  /* Device not found in list: don't display it */
		  continue;
	    }
	
	    ioi = st_iodev[curr] + i;
	    ioj = st_iodev[!curr] + i;

	    if (!DISPLAY_UNFILTERED(flags)) {
	       if (HAS_OLD_KERNEL(flags) ||
		   HAS_PLAIN_KERNEL24(flags)) {
		  if (!ioi->dk_drive)
		     continue;
	       }
	       else {
		  if (!ioi->rd_ios && !ioi->wr_ios)
		     continue;
	       }
	    }

	    if (DISPLAY_EXTENDED(flags))
	       write_ext_stat(curr, itv, flags, fctr, shi, ioi, ioj, scan_type,
                              first_time );
	    else
	       write_basic_stat(curr, itv, flags, fctr, shi, ioi, ioj);
	 }
      }
/*      printf("\n"); */
   }
   return 1;
}


/*
 ***************************************************************************
 * Main loop: read I/O stats from the relevant sources,
 * and display them.
 ***************************************************************************
 */
void rw_io_stat_loop(int flags, long int count, struct tm *rectime, 
                     int scan_type, int first_time )
{
   static int curr = 1;
   int next;

   do {
      /* Read kernel statistics (CPU, and possibly disks for old kernels) */
      read_proc_stat(curr, flags);

      if (dlist_idx) {
	 /*
	  * A device or partition name was entered on the command line,
	  * with or without -p option (but not -p ALL).
	  */
	 if (HAS_DISKSTATS(flags) && !DISPLAY_PARTITIONS(flags))
	    read_diskstats_stat(curr, flags);
	 else if (HAS_SYSFS(flags))
	    read_sysfs_dlist_stat(curr, flags);
	 else if (HAS_PPARTITIONS(flags) && !DISPLAY_PARTITIONS(flags))
	    read_ppartitions_stat(curr, flags);
      }
      else {
	 /*
	  * No devices nor partitions entered on the command line
	  * (for example if -p ALL was used).
	  */
	 if (HAS_DISKSTATS(flags))
	    read_diskstats_stat(curr, flags);
	 else if (HAS_SYSFS(flags))
	    read_sysfs_stat(curr,flags);
	 else if (HAS_PPARTITIONS(flags))
	    read_ppartitions_stat(curr, flags);
      }

      /* Save time */
      get_localtime(rectime);

      /* Print results */
      if ((next = write_stat(curr, flags, rectime, scan_type, first_time ))
	  && (count > 0))
	 count--;
      fflush(stdout);

      if (count) {
	 pause();
      }

      /* SDS ... it is important for curr to be changed.  If
         it is not allowed to change, then the stats reported
         are always since boot time. */
      if (next)
         curr ^= 1;
    
   }
   while (count);
}

/*
 ***************************************************************************
 * Main entry to the iostat functions.
 ***************************************************************************
 */
void DS_get_disk_usage( int scan_type, int first_time ){

   static int flags = 0;
   static struct tm rectime;

   static int initialized = 0;

#ifdef USE_NLS
   /* Init National Language Support */
   init_nls();
#endif

   /* If this is the first time through this function,
      perform one-time initialization. */
   if( !initialized ){

      /* Get HZ */
      get_HZ();

      /* Allocate structures for device list */
      salloc_dev_list(2);

      /* Initialize flag to 10 (-x -d -k).  See iostat.h. */
      flags = 42;

      /* Ignore device list if '-p ALL' entered on the command line */
      if (DISPLAY_PART_ALL(flags))
         dlist_idx = 0;

      /* Init structures according to machine architecture */
      io_sys_init(&flags);

      get_localtime(&rectime);

      initialized = 1;

   }

   /* Main loop */
   rw_io_stat_loop(flags, 1, &rectime, scan_type, first_time );

}

/*\//////////////////////////////////////////////////////////////////////////
   
   Description:
      Initializes Disk statistics. 

//////////////////////////////////////////////////////////////////////////\*/
int DS_init_disk_usage( ){

   char name[256];
   char file[256];

   /* Set the Disk usage area to all zeros. */
   memset( &Disk_usage, 0, sizeof( Disk_stats_t ) );

   /* Put file to store gnuplot data in the work directory. */
   if( MISC_get_work_dir( name, 256 - 32 ) < 0 ){

      fprintf( stderr, "MISC_get_work_dir failed\n" );

      /* Open files in current working directory. */
      name[0] = '\0';

   }

   sprintf( file, "/%s.disk.stats", Uname_info->nodename );
   strcat( name, file );

   /* Open the file.  If Disk_fd is not negative, the file exists.
      Close the file, remove it, then re-opent the file with create. */
   if( (Disk_fd = open( name, O_RDWR, 0666 )) >= 0 ){
   
      close( Disk_fd );
      unlink( name );
      Disk_fd = -1;

   }

   /* Create the "Disk.stats" file. */
   if( (Disk_fd = open( name, O_RDWR | O_CREAT, 0666 )) < 0 ){

      fprintf( stderr, "Open Failed For FILE %s\n", name );
      Disk_fd = -1;

   }

   return Disk_fd;

/* End of DS_init_disk_usage() */
}

/*\///////////////////////////////////////////////////////////////////////////

   Description:
      Cleanup function for this module.

///////////////////////////////////////////////////////////////////////////\*/
void DS_cleanup(){

   /* Close dsk stats file if file is open. */
   if( Disk_fd >= 0 ){

      int size;
      double ave_await = 0, ave_svctm = 0, ave_util = 0;
      double ave_kb_read = 0, ave_kb_write = 0;
      time_t ctime = Scan_info.scan_time - Start_time;

      /* Do end of volume processing. */
      End_of_volume( 0, 0, 0, 0, 0, 0, 1 );

      /* Write out the average disk utilization. */
      memset( Buffer, 0, sizeof(Buffer) );

      /* In the event Total_reports is 0. */
      if( Total_reports > 0 ){

         ave_await = Total_await / Total_reports;
         ave_svctm = Total_svctm / Total_reports;
         ave_util = Total_util / Total_reports;

      }

      /* In the event Volume_num is 0. */
      if( Volume_num > 0 ){

         ave_kb_read = Total_kb_read / (double) Volume_num;
         ave_kb_write = Total_kb_write / (double) Volume_num;

      }

      sprintf( Buffer, "# Avgs:               %4d        %8.2f      %8.2f       %7.2f       %7.2f         %6.2f \n",
               (int) ctime, ave_kb_read, ave_kb_write, ave_await, ave_svctm, ave_util );

      size = strlen( Buffer );
      if( write( Disk_fd, Buffer, size ) != size )
         fprintf( stderr, "Write to Disk_fd Failed\n" );

      close( Disk_fd );

   }

/* End of DS_cleanup(). */
}

