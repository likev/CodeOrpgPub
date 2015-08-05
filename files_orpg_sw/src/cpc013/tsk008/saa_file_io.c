/*
 * RCS info
 * $Author: aamirn $
 * $Locker:  $
 * $Date: 2008/01/04 20:54:49 $
 * $Id: saa_file_io.c,v 1.4 2008/01/04 20:54:49 aamirn Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */


/*********************************************************************
File    : saa_file_io.c
Created : Sept. 24,2003
Details : File I/O
Author  : Reji Zachariah

Modification History: Khoi Dang added the functions (since October 10)
		1. open_saa_ohp_files
		2. open_saa_total_files
		3. read_from_saa_total_files
		4. write_to_saa_total_files
		The function read_from_files and write_to_files are modified 
		so that they now take care of the saa_ohp struct only.  The
		saa_usp struct, modified to handle only the current clock-hour
		total is also written to saa_total file.  (Added 10/27/2004 by 
		W. David Zittel)
		5.  The array previous_snow_rate is written to saa_total file 
		to handle instances when RPG software is shut down and 
		restarted.  (Added 10/27/2004 by W. Dave Zittel for Build8)

*********************************************************************/

#include "saa.h"
#include "saa_compute_products.h"
#include "saaConstants.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define FILE_PERMISSION    0660   
#define MAX_PATH_LEN       255

/*********************************************************  
Method : write_to_files
Details: write to ohp and usp files
*********************************************************/
int write_to_files(int fd_ohp){
	int status;
	
	
	if(SAA_DEBUG){
		fprintf(stderr, "Entering Write To Files Function...\n");
	}
	status  = 0;
	
	status = write(fd_ohp,&saa_ohp, sizeof(saa_ohp));
	
	
	if(status >= 0){
		if(SAA_DEBUG){
			fprintf(stderr,"Number of bytes written = %d.\n", status);
		
		}
		return 0;
	}
	else{
		if(SAA_DEBUG){
			fprintf(stderr,"Write OHP File Error, status = %d.\n",status);
		}
		return -1;
	}
	


}/*write_to_file */

/*********************************************************  
Method : read_from_files
Details: read from the ohp and usp files
*********************************************************/
int read_from_files(int fd_ohp,int* num_ohp_bytes_read){

	int status;
      

	
	if(SAA_DEBUG){
		fprintf(stderr, "Entering Read From Files Function...\n");
	}
	
	
	status  = 0;
	
	status = read(fd_ohp,&saa_ohp, sizeof(saa_ohp));
	*num_ohp_bytes_read = status;
	if(SAA_DEBUG){fprintf(stderr,"Number of bytes read from OHP file = %d\n",*num_ohp_bytes_read);}
	
	if(status >= 0){
		if(SAA_DEBUG){
			fprintf(stderr,"Read data from file: OHP Struct Size = %d, First = %d.\n",saa_ohp.size,saa_ohp.first);
		     
		}
		return 0;
	}
	else{
		if(SAA_DEBUG){
			fprintf(stderr,"Read OHP File Error, status = %d.\n",status);
		}
		return -1;
	}
   

}/*end function read_from_files */

/*********************************************************  
Method :open_saa_ohp_files
Details: open the files to back up ohp  kd 10/28
*********************************************************/
int open_saa_ohp_file()
{
     	int ret, fd_ohp;
        mode_t mode = (mode_t) FILE_PERMISSION;
        char *path = NULL;

        /* Initialize the file descriptor. */
      	fd_ohp=-1; 

        /* Construct the fully qualified file name for
           SAAHOURLY. */
        ret = RPGC_construct_file_name( SAAHOURLY, &path );
        if( ret <= 0 ){

           RPGC_log_msg( GL_ERROR,
               "RPGC_construct_file_name Return Error (%d)\n", ret ); 
           return fd_ohp;

        }

        /* Open the file with owner and group Read/Write permission. */
   	fd_ohp = open( path, O_CREAT|O_RDWR, mode );

        /* Free memory malloc'd by RPGC_construct_file_name() */
        if( path != NULL )
           free( path ); 
   
   	return fd_ohp;

}

/*********************************************************  
Method :open_saa_total_files
Details: open the files to back up ohp total kd 10/28
*********************************************************/
int open_saa_total_files()
{
  	int ret, fd_total;
        mode_t mode = (mode_t) FILE_PERMISSION;
        char *path = NULL;

   
        /* Initialize the file descriptor. */
  	fd_total=-1;

        /* Construct the fully qualified file name for
           SAATOTAL. */
        ret = RPGC_construct_file_name( SAATOTAL, &path );
        if( ret <= 0 ){

           RPGC_log_msg( GL_ERROR,
               "RPGC_construct_file_name Return Error (%d)\n", ret ); 
           return fd_total;

        }


        /* Open the file with owner and group Read/Write permission. */
 	fd_total = open( path, O_CREAT|O_RDWR, mode );

        /* Free memory malloc'd by RPGC_construct_file_name() */
        if( path != NULL )
           free( path ); 

   	return fd_total;

}

/*********************************************************  
Method : read_from_saa_total_files
Details: read from the saa total files kd  10/28
Change
history:
	10/27/2004	W.D. Zittel	Added read of partial accumulation
					USP current hour and previous rate
					arrays for Build 8
*********************************************************/
int read_from_saa_total_files(int fd_total, int* num_bytes_read){

	int status;
       

	
	if(SAA_DEBUG){
		fprintf(stderr, "Entering read_from_saa_total Function...\n");
	}
	
	/*Check for NULL pointers  */
	if (num_bytes_read == NULL){
		LE_send_msg(GL_ERROR,"SAA:read_from_files- NULL pointers passed in as arguments.\n");
	  	return -1;
	}/*end if */
	
	status  = 0;
	/*read from the file continuously to the arrays.If something wrong happens, the function will return -1 */
	/*if successfully reading, it returns 0 */
	status = read(fd_total,&saa_swe_storm_total, sizeof(saa_swe_storm_total));
	
	if(status >= 0){
		if(SAA_DEBUG){
			fprintf(stderr,"Number of bytes read from saa_swe_storm_total = %d\n",status);
		}
		
	}
	else{
		if(SAA_DEBUG){
			fprintf(stderr,"Read saa_swe_storm_total Error, status = %d.\n",status);
		}
		return -1;
	}
	*num_bytes_read = *num_bytes_read + status; 
	
        status =read(fd_total,&saa_swe_oh_total,sizeof(saa_swe_oh_total) );
	if(status >= 0){
		if(SAA_DEBUG){
			fprintf(stderr,"Number of bytes read from saa_swe_oh_total = %d\n",status);
		}
		
	}
	else{
		if(SAA_DEBUG){
			fprintf(stderr,"Read saa_swe_oh_total Error, status = %d.\n",status);
		}
		return -1;
	}
	
	*num_bytes_read = *num_bytes_read+status;
	
        status = read(fd_total,&saa_sd_storm_total, sizeof(saa_sd_storm_total));
        if(status >= 0){
		if(SAA_DEBUG){
			fprintf(stderr,"Number of bytes read from saa_sd_storm_total = %d\n",status);
		}
		
	}
	else{
		if(SAA_DEBUG){
			fprintf(stderr,"Read saa_sd_storm_total Error, status = %d.\n",status);
		}
		return -1;
	}
	*num_bytes_read = *num_bytes_read+status;

        status =read(fd_total,&saa_sd_oh_total,sizeof(saa_sd_oh_total) );
  	if(status >= 0){
		if(SAA_DEBUG){
			fprintf(stderr,"Number of bytes read from saa_sd_oh_total = %d\n",status);
		}
		
	}
	else{
		if(SAA_DEBUG){
			fprintf(stderr,"Read saa_sd_oh_total Error, status = %d.\n",status);
		}
		return -1;
	}
	*num_bytes_read = *num_bytes_read+status;

	/* Next 30 lines added for Build8 to retrieve partial accumulations and previous snow rate for usp products */
	status = read(fd_total,&saa_usp, sizeof(saa_usp) );
	
  	if(status >= 0){
		if(SAA_DEBUG){
			fprintf(stderr,"Number of bytes read from saa_usp = %d\n",status);
		}
		
	}
	else{
		if(SAA_DEBUG){
			fprintf(stderr,"Read saa_usp Error, status = %d.\n",status);
		}
		return -1;
	}
	*num_bytes_read = *num_bytes_read+status;
	
	status = read(fd_total,&previous_snow_rate, sizeof(previous_snow_rate) );
	
  	if(status >= 0){
		if(SAA_DEBUG){
			fprintf(stderr,"Number of bytes read from previous_snow_rate = %d\n",status);
		}
		
	}
	else{
		if(SAA_DEBUG){
			fprintf(stderr,"Read previous_snow_rate Error, status = %d.\n",status);
		}
		return -1;
	}
	*num_bytes_read = *num_bytes_read+status;

	return 0;


}/*end function read_from_saa_total_files */

/*********************************************************  
Method : write_to_saa_total_files
Details: write to ohp and usp files kd 10/28/03
Change
history:
	10/27/2004	W.D. Zittel	Added write of partial accumulation
					USP current hour and previous rate
					arrays for Build8
*********************************************************/
int write_to_saa_total_files(int fd_total){
	int status;
	
	if(SAA_DEBUG){
		fprintf(stderr, "Entering write_to_saa_total_files Function...\n");
	}
	status  = 0;
	
	status = write(fd_total,&saa_swe_storm_total, sizeof(saa_swe_storm_total));
	
	
	if(status >= 0){
		if(SAA_DEBUG){
			fprintf(stderr,"Number of saa_swe_storm_total bytes written = %d.\n", status);
		
		}
		
	}
	else{
		if(SAA_DEBUG){
			fprintf(stderr,"Write saa_swe_storm_total  Error, status = %d.\n",status);
		}
		return -1;
	}
	status = write(fd_total,&saa_swe_oh_total, sizeof(saa_swe_oh_total));
	
	
	if(status >= 0){
		if(SAA_DEBUG){
			fprintf(stderr,"Number of saa_swe_oh_total bytes written = %d.\n", status);
		
		}
		
	}
	else{
		if(SAA_DEBUG){
			fprintf(stderr,"Write saa_swe_oh_total File Error, status = %d.\n",status);
		}
		return -1;
	}
	status = write(fd_total,&saa_sd_storm_total, sizeof(saa_sd_storm_total));
	
	
	if(status >= 0){
		if(SAA_DEBUG){
			fprintf(stderr,"Number of saa_sd_storm_total bytes written = %d.\n", status);
		
		}
		
	}
	else{
		if(SAA_DEBUG){
			fprintf(stderr,"Write saa_sd_storm_total File Error, status = %d.\n",status);
		}
		return -1;
	}
	status = write(fd_total,&saa_sd_oh_total, sizeof(saa_sd_oh_total));
	
	
	if(status >= 0){
		if(SAA_DEBUG){
			fprintf(stderr,"Number of saa_sd_oh_total bytes written = %d.\n", status);
		
		}
		
	}
	else{
		if(SAA_DEBUG){
			fprintf(stderr,"Write saa_sd_oh_total File Error, status = %d.\n",status);
		}
		return -1;
	}

	/*  Next 30 lines added for Build8 to save partial clock hour sums for usp products  */
	status = write(fd_total,&saa_usp,sizeof(saa_usp));
	
	if(status >= 0){
		if(SAA_DEBUG){
			fprintf(stderr,"Number of saa_usp = %d.\n", status);
		
		}
		
	}
	else{
		if(SAA_DEBUG){
			fprintf(stderr,"Write saa_usp File Error, status = %d.\n",status);
		}
		return -1;
	}
	
	status = write(fd_total,&previous_snow_rate,sizeof(previous_snow_rate));
	
	if(status >= 0){
		if(SAA_DEBUG){
			fprintf(stderr,"Number of previous_snow_rate bytes written = %d.\n", status);
		
		}
		
	}
	else{
		if(SAA_DEBUG){
			fprintf(stderr,"Write previous_snow_rate File Error, status = %d.\n",status);
		}
		return -1;
	}
	

	return 0;

}/*write_to_file */
/*******************************************************************/
	
