/*
 * RCS info
 * $Author: aamirn $
 * $Locker:  $
 * $Date: 2008/01/04 20:54:50 $
 * $Id: saa_file_io.h,v 1.2 2008/01/04 20:54:50 aamirn Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */


/*********************************************************************
File    : saa_file_io.h
Created : Sept. 24,2003
Details : File I/O
Author  : Reji Zachariah

Modification History: Khoi Dang added functions since (October 10)
		1.write_to_saa_total_files
		2.read_from_saa_total_files
		3.open_saa_usp_files
		4.open_saa_total_files
		Delete function open_saa_usp_files (Dec 18) 
		declare variables count_msg_id,lb_descriptor,wr_status to make them
		visible throughout the program ( orginally, these variables are 
		declared in saa_main.c //1/6/04 )
*********************************************************************/

int lb_descriptor;
int usr_data_available;
int count_msg_id;/* 1/6/04/ */
int lb_descriptor ;
int wr_status;
int write_to_files(int fd_ohp);
int read_from_files(int fd_ohp,
		    int* num_ohp_bytes_read);
int open_saa_ohp_file();
int open_saa_total_files(); 
int write_to_saa_total_files(int fd_total);
int read_from_saa_total_files(int fd_total, int * num_tot_bytes_read); 		    
