/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2005/03/09 15:44:12 $
 * $Id: prcprtac_file_io.c,v 1.1 2005/03/09 15:44:12 ryans Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */
/****************************************************************************
   Filename: prcprtac_file_io.c
   Author:   Cham Pham
   Created:  11/25/04

   Description
   ===========
        Performs disk I/O to the Hydromet rate/accumulation disk file.

   Change history:
   ==============
   01/05/05         000         Cham Pham           CCR# NA05-01303
*****************************************************************************/
/* Global include files */
#include <a313h.h>
#include <a313hbuf.h>

/* Local include files */
#include "prcprtac_file_io.h"
#include "prcprtac_Constants.h"

/****************************************************************************
 Function: open_disk_file()
 Details: Open LB for writing. Return the LB descriptor

****************************************************************************/
int open_disk_file( ) 
{
  int lb_flags;
  int ret, fd;

  lb_flags = LB_WRITE|LB_READ;
  fd = -1;

/* Construct the fully qualified file name for HYACCUMS.DAT */

  ret = RPGC_construct_file_name( ACUM_FN, &LB_name );

  if ( ret < 0 ) 
  {
    RPGC_log_msg( GL_ERROR,
               "RPGC_construct_file_name Return Error (%d)\n", ret );
    if (DEBUG) {fprintf(stderr,"File name does not exist %s\n",LB_name);}
  }

/* Open the file with owner and group Read/Write permission. */

  if ( DEBUG ) {fprintf(stderr,"pathacum: %s\n",LB_name);}
  
  fd = LB_open( (char *)LB_name, lb_flags, &attr ); 

  if ( fd <0 ) 
  {
    if (DEBUG) {fprintf(stderr,"LB open Failed (%d)\n%s\n", fd,LB_name);}
    RPGC_log_msg(GL_ERROR,"LB_open %s (write) failed (fd %d)",LB_name,fd);
    return ( fd );
  }

  if ( DEBUG )
    {fprintf(stderr,"Suceeded in Opening %s for writing (fd %d)\n",ACUM_FN,fd);}

  return ( fd );
}

/**************************************************************************
 Function: Header_IO() 
 Details: Performs disk i/o to the hydromet rate/accumulation disk file
          for the rate, period and hourly accumulation header records.
          Returns 0 on success, -1 on failure.

**************************************************************************/
int Header_IO ( int func, int hdrflg ) 
{
  int  buflen, msg_id;
  char *buf = NULL;

/* Read Header records .............................. */
  if ( func == readLB ) 
  { 
    if ( hdrflg == rathdr ) 
    {
      msg_id = orpg_rathdr_rec;

      if (DEBUG) {fprintf(stderr,"msg_id: %d\tfdlb: %d\n",msg_id,fdlb);}

      buflen = LB_read( fdlb, (void *)&buf, LB_ALLOC_BUF, msg_id );

      if (DEBUG) {fprintf(stderr,"Header_IO: LB_read - buflen: %d\n",buflen);}

      if ( (buflen < 0) && (buflen > sizeof(Rate_Header_t)) ) 
      {
        if ( DEBUG ) 
          {fprintf(stderr,"Msg length= %d, msg_id= %d\n",buflen,msg_id);}

        return -1;
      }

      memcpy( &RateHdr, buf, buflen );
      free( buf );

    } 
    else 
    {
      if ( hdrflg == prdhdr ) 
      { 

        msg_id = orpg_prdhdr_rec;
        buflen = LB_read( fdlb, (char *)&buf, LB_ALLOC_BUF, msg_id );

        if ( (buflen < 0) || (buflen > (ACZ_TOT_PRDS*sizeof(Period_Header_t))) )
        {
          if ( DEBUG ) 
            {fprintf(stderr,"Msg length= %d, msg_id= %d\n",buflen,msg_id);} 

          return -1;
        }

        memcpy( PerdHdr, buf, buflen );
        free( buf ); 

      }
      else 
      {

        msg_id = orpg_hlyhdr_rec;
        buflen = LB_read( fdlb, (char *)&buf, LB_ALLOC_BUF, msg_id );

        if ( (buflen < 0) || (buflen > sizeof(Hour_Header_t)) )
        {
          if ( DEBUG ) 
            {fprintf(stderr,"Msg length= %d, msg_id= %d\n",buflen,msg_id);}

          return -1;
        }

        memcpy( &HourlyHdr, buf, buflen );
        free( buf );

      }/* End if block hdrflg equals to hlyhdr */

    }/* End hdrflg equals to rathdr */

  }/* End if block func equals to readLB */
/* Write Header records .................................. */
  else 
  { 
    if ( hdrflg == rathdr ) 
    {
      msg_id = orpg_rathdr_rec;
      buflen = LB_write( fdlb, (char *)&RateHdr, sizeof(Rate_Header_t), msg_id);

      if ( DEBUG ) 
        {fprintf(stderr,"- Write rathdr REC=%d\t- SIZE=%d\n",msg_id,buflen);}

      if ( buflen < 0 ) 
      {
        if ( DEBUG ) {fprintf(stderr,"Failed to write rate header to LB\n");}
        return -1;
      }
    } 
    else 
    {
      if ( hdrflg == prdhdr ) 
      { 

        msg_id = orpg_prdhdr_rec;
        buflen = LB_write( fdlb, (char *)PerdHdr, 
                         ACZ_TOT_PRDS*sizeof(Period_Header_t), msg_id );

        if ( DEBUG ) 
          {fprintf(stderr,"- Write prdhdr REC=%d\t- SIZE=%d\n",msg_id,buflen);}

        if ( buflen < 0 ) 
        {
          if (DEBUG) {fprintf(stderr,"Failed to write rate header to LB\n");}
          return -1;
        }

        free( buf );

      }
      else 
      {
        msg_id = orpg_hlyhdr_rec;
        buflen = LB_write( fdlb, (char *)&HourlyHdr,
                           sizeof(Hour_Header_t), msg_id );

        if ( DEBUG ) 
          {fprintf(stderr,"- Write hlyhdr REC=%d\t- SIZE=%d\n",msg_id,buflen);}

        if ( buflen < 0 ) 
        {
          if ( DEBUG ) {fprintf(stderr,"Failed to write rate header to LB\n");}
          return -1;
        }

      }/* End if block hdrflg equals to hlyhdr */

    }/* End if block hdrflg equals to rathdr */

  }/* End if block func equal to writeLB */

  return 0;
}

/**************************************************************************
 Function: Badscan_IO() 
 Details:  Performs disk i/o to the hydromet rate/accumulation disk file
           for the bad scan time-stamp array record.
           Return the LB descriptor
***************************************************************************/
int Badscan_IO( int func ) 
{
  int buflen, msg_id;
  char *buf = NULL;
  msg_id = orpg_badscn_rec;

/* Read bad scan array */
  if ( func == readLB ) 
  {
    buflen = LB_read( fdlb, (char *)&buf, LB_ALLOC_BUF, msg_id );

    if ( (buflen < 0) || (buflen > (MAX_TSTAMPS*sizeof(a313hbsc_t))) ) 
    {
      if ( DEBUG )
      {
        fprintf(stderr,"Failed to read Badscan from LB: len=%d, msg_id=%d\n",
                                                             buflen, msg_id);
      }
      return -1;
    }

    memcpy( a313hbsc, buf, buflen );
    free ( buf );

  }/* End if block func equals to readLB */

/* Write bad scan array */
  else 
  {
    buflen = LB_write( fdlb, (char *)a313hbsc, 
                       MAX_TSTAMPS*sizeof(a313hbsc_t), msg_id );

    if ( DEBUG ) 
      {fprintf(stderr,"- Write badscn REC=%d\t- SIZE=%d\n",msg_id,buflen/4);}

    if ( buflen < 0 ) 
    {
      if ( DEBUG ) 
      {
        fprintf(stderr,"Failed to write Badscan to LB len=%d, msg_id=%d\n",
                                                           buflen, msg_id);
      }

      return -1;
    }

  }/* End if block func equals to writeLB */

  return 0;
}
/***************************************************************************
 Function: Scan_IO() 
 Details: Performs disk i/o to the hydromet rate/accumulation disk file 
          for the rate scan and period and hourly accumulation scan records. 
          Returns 0 on success, -l on failure.

****************************************************************************/
int Scan_IO( int func, int rec_id, short inbuf[][MAX_RABINS] ) 
{
  ScanData_t buf_ptr;
  char *buf=NULL;
  int buflen, len, msg_id; 
  
  len = sizeof(short)*MAX_AZMTHS*MAX_RABINS;
  msg_id = rec_id + orpg_ratscn_rec;

/* Read accumulation scan records */
  if ( func == readLB ) 
  {

    buflen = LB_read( fdlb, (void *)&buf, LB_ALLOC_BUF, msg_id );

    if ( DEBUG ) 
      {fprintf(stderr,"Scan_IO: Read- REC=  %d (sizebuf %d)\n",msg_id,buflen);}

    if ( (buflen <= 0) || (buflen > len) ) 
    {
      if ( DEBUG ) {fprintf(stderr, "Failed to read Rate/HrlAcum from LB.\n");}
      return -1;
    }

    memcpy( inbuf, buf, buflen );
    free( buf );

  }/* End if block func equal to readLB */

/* Write accumulation scan records */
  else 
  {

    memcpy( &buf_ptr, inbuf, len );
    buflen = LB_write( fdlb,(char *)&buf_ptr, sizeof(ScanData_t), msg_id );

    if ( DEBUG ) 
      {fprintf(stderr,"ScanIO- Write  REC=%d\t-SIZE=%d\n",msg_id,buflen);}

    if ( (buflen <= 0) || (buflen > sizeof(ScanData_t)) ) 
    {
      if ( DEBUG ) {fprintf(stderr, "FAILED to write Rate/HrlAcum to LB.\n");}
      return -1;
    }

  }/* End if block func equal to writeLB */   

  return 0;
}
