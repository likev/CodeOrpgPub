/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2011/09/19 14:46:26 $
 * $Id: archive_II.h,v 1.22 2011/09/19 14:46:26 ccalvert Exp $
 * $Revision: 1.22 $
 * $State: Exp $
 */

#ifndef ARCHIVE_II_H
#define ARCHIVE_II_H

#include <time.h>

/********************************************************************
The following values are return codes from the ARCHIVE II control
function calls.
********************************************************************/

/* Archive II/LDM Status Values. */
#define ARCHIVE_II_FAIL		        -7
#define ARCHIVE_II_ERROR		-1
#define ARCHIVE_II_UNKNOWN		0
#define ARCHIVE_II_RUNNING		3
#define ARCHIVE_II_NOT_RUNNING		4
#define ARCHIVE_II_SHUT_DOWN		6

/* Archive II/LDM Transmit Values. */
#define ARCHIVE_II_TRANSMIT_ON		1
#define ARCHIVE_II_TRANSMIT_OFF		0

/* Archive II/LDM Commands. */
#define ARCHIVE_II_NEED_TO_START	1
#define ARCHIVE_II_NEED_TO_STOP		2
#define ARCHIVE_II_LOCAL_MODE          -1
#define ARCHIVE_II_LDM_MODE            -2
#define ARCHIVE_II_RECORD_MODE         -3
#define ARCHIVE_II_NO_RECORD_MODE      -4
#define ARCHIVE_II_NORMAL_MODE         -5

/* ORPGDAT_ARCHIVE_II_INFO Message IDs */
#define ARCHIVE_II_COMMAND_ID             1 /* ArchII_command_t */
#define ARCHIVE_II_CLIENTS_ID              2 /* ArchII_clients_t */
#define ARCHIVE_II_FLOW_ID                3 /* ArchII_transmit_status_t */
#define ARCHIVE_II_PERFMON_ID             4 /* ArchII_perfmon_t */

typedef struct{

   time_t ctime;  /* Time the status was issued. */
   int status;    /* Archive II transmit status. */

} ArchII_transmit_status_t;
   
typedef struct{

   time_t ctime;   /* Time the command was issued. */
   int command;    /* Archive II command. */

} ArchII_command_t;

/* The following is used to collect performance statistics for load
   testing. */
typedef struct {

   unsigned int total_bytes_c;	/* Total bytes compressed. */
   unsigned int total_bytes_u;	/* Total bytes uncompressed. */

} ArchII_perfmon_t;
   
#define LDM_MAX_N_CLIENTS          10
#define LDM_CLIENT_INFO_SIZE       24
#define MAX_LDM_CLIENT_INFO_SIZE   (LDM_MAX_N_CLIENTS*LDM_CLIENT_INFO_SIZE)

typedef struct{

  time_t ctime;                         /* Time clients were noted */
  int num_clients;                      /* Number of connected clients. */
  char buf[MAX_LDM_CLIENT_INFO_SIZE];   /* Client information. */

} ArchII_clients_t;
 
#endif
