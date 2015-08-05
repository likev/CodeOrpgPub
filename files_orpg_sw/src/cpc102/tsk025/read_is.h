/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/02/11 15:36:29 $
 * $Id: read_is.h,v 1.10 2013/02/11 15:36:29 steves Exp $
 * $Revision: 1.10 $
 * $State: Exp $
 */

#ifndef READ_IS_H
#define READ_IS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#include <infr.h>
#include <basedata.h>
#include <a309.h>

/* Macro definitions. */
#define ICAO_LENGTH             4
#define DEFAULT_WAIT            30
#define MIN_WAIT                15
#define FILENAME_SIZE           64
#define MAX_DIRECTORY_ENTRIES   256

/* The following limits are arbitrary ... may need tweeking. */
#define MAX_ERROR_COUNT         4
#define MAX_WARNING_COUNT       15  

/* Stores the output LB name. */
char LB_name[256];
int LB_fd;
int Verbose;

/* Used in conjunction with libcurl. */
typedef struct MemoryStruct {

   char *memory;
   size_t size;

} MemoryStruct_t;

/* Use for tracking L2 files read. */
typedef struct FileStruct {

   size_t size;
   size_t size_processed;
   char filename[FILENAME_SIZE+1];

} FileStruct_t;

/* Used to handle the case of day changing using LOCAL_URL. */
typedef struct Current_label {

   char label[32];	/* Has the form: AR2.... */

   int year;		/* yyyy */

   int month;		/* mm */

   int day;		/* dd */

   int hour;		/* hh */

   int minute;		/* mm */

   int second;		/* ss */

   char radar_id[8];	/* ICAO */

} Current_label_t;

/* Function Protypes. */
int Process_current_buffer( MemoryStruct_t *data, FileStruct_t *file );

#endif
