/*
 * RCS info
 * $Author: priegni $
 * $Locker:  $
 * $Date: 2000/10/05 21:22:48 $
 * $Id: rda_alarm_table.h,v 1.3 2000/10/05 21:22:48 priegni Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

#ifndef RDA_ALARM_TABLE_H
#define RDA_ALARM_TABLE_H

/*
  Defines the structure of RDA alarm data stored in RDA alarm LB.
  Each message is a separate alarm.
*/
typedef struct{

   short month;     /* Moth of year, 1-12				*/
   short day;       /* Day of month, 1-31				*/
   short year;      /* Year, includes century				*/
   short hour;      /* Hour of day, 0 - 23				*/
   short minute;    /* Minute of hour, 0-59				*/
   short second;    /* second of minutes, 0-59				*/
   short code;      /* 0 - cleared alarm, 1 - active alarm		*/
   short alarm;     /* Alarm code (magnitude), 1-800			*/
   short channel;   /* RDA Channel # generating alarm			*/
   short spare;     /* Pad for word boundary alignment.			*/

} RDA_alarm_t;

#endif
