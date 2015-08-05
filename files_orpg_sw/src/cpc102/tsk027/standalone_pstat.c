/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/12/09 22:27:54 $
 * $Id: standalone_pstat.c,v 1.1 2014/12/09 22:27:54 steves Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <infr.h>
#include <orpg.h>
#include <prod_status.h>

#define BUFFER_SIZE 24000

/* For writing in color. */
#define YELLOW		1
#define KYEL  		"\x1B[33m"
#define RED		2
#define KRED		"\x1B[31;1m"
#define GREEN		3
#define KGRN  		"\x1B[32m"
#define BLUE		4
#define KBLU  		"\x1B[34;1m"
#define RESET 		"\033[0m"

typedef struct {

   short prod_id;

   short prod_code;

   char mnem[4];

} Pat_t;

static char Prod_status_lb_name[NAME_SIZE+1];
static char Prod_attr_table_name[NAME_SIZE+1];
static Pat_t Pat[2000];
static int Prod_codes_only;

static int Read_options (int argc, char **argv);
static void Print_prod_status ();
static int Read_prod_status (char **msg);
static char *HP_print_parameters (short *params);
static char *HP_print_msgid (LB_id_t msg_id, int *color);
static char *HP_print_schedule (char schedule, int *scolor);
static char *HP_print_elev_index (short elev_index);
static int Read_ASCII_PAT( char *file_name );
static char *convert_time( time_t timevalue );
static char *calendar_date( time_t timevalue );

/******************************************************************

    Description: The main function.

******************************************************************/

int main (int argc, char **argv){

   /* read options */
   if( Read_options (argc, argv) != 0 )
      exit (-1);

   /* One and done. */
   Print_prod_status ();

   return 0;

}

/**************************************************************************

   Description: This function reads and prints the current product 
		generation status info.

**************************************************************************/
static void Print_prod_status (){

   int pat_avail = 1, color = 0, scolor = 0, len;
   char *msg = NULL;
   Prod_gen_status_header *hd;
   Prod_gen_status *entry;
   int i, k;

   if ((len = Read_prod_status (&msg)) == 0){

       LE_send_msg( 0, "Read_prod_status Returned 0\n" );
       return;

   }

   pat_avail = Read_ASCII_PAT( Prod_attr_table_name );

   /* check */
   hd = (Prod_gen_status_header *)msg;
   if( (len < sizeof (Prod_gen_status_header)) 
                     ||
       (hd->list < sizeof (Prod_gen_status_header)) 
                     || 
               (hd->length < 0)
                     ||
       (len != hd->list + hd->length * sizeof (Prod_gen_status)) ){

      fprintf( stderr, "Bad Prod_gen_status msg (msg_len %d, list %d, len %d)",
	       len, hd->list, hd->length);
      exit (1);

   }

   /* print the info */
   fprintf( stderr, "%d products; %d volumes listed\n", hd->length, hd->vdepth );

   fprintf( stderr, "Volume num  : " );
   for (i = 0; i < hd->vdepth; i++)
      fprintf ( stderr, "%10d", (int)hd->vnum[i] );

   fprintf( stderr, "\n" );
   fprintf( stderr, "Volume time : " );
   for (i = 0; i < hd->vdepth; i++){

      char *time_string;

      time_string = convert_time( hd->vtime[i] );
      fprintf( stderr, "  %s", time_string );

   }

   fprintf( stderr, "\n" );
   fprintf( stderr, "Volume date : " );
   for( i = 0; i < hd->vdepth; i++ ){

      char *date_string;

      date_string = calendar_date( hd->vtime[i] );
      fprintf( stderr, "  %s", date_string );

   }

   fprintf( stderr, "\n" );
   fprintf( stderr, "VCP/Wx Mode: " );
   for (i = 0; i < hd->vdepth; i++){

      int vcp_num = (int) hd->vcpnum[i];

      if( hd->wx_mode[i] == PRECIPITATION_MODE )
         fprintf( stderr, "   %2d  / A", vcp_num );

      else if( hd->wx_mode[i] == CLEAR_AIR_MODE )
         fprintf( stderr, "   %2d  / B", vcp_num );

      else
         fprintf( stderr, "   %2d  / ?", vcp_num );

   }

   fprintf( stderr, "\n" );
   fprintf( stderr, "\n" );
   fprintf( stderr, " ID  Code Mnc     P1    P2    P3    P4    P5    P6 El# Genpr Schdl        Message IDs\n");

   entry = (Prod_gen_status *)(msg + hd->list);
   for (i = 0; i < hd->length; i++) {

      char *params, *schedule, *elev_index, mnemonic[4], prod_code[4];

      params = HP_print_parameters (entry->params);
      schedule = HP_print_schedule (entry->schedule, &scolor);
      elev_index = HP_print_elev_index( entry->elev_index );
      strcpy( prod_code, "   " ); 
      strcpy( mnemonic, "   " ); 
      if( pat_avail >= 0 ){

         strcpy( mnemonic, Pat[entry->prod_id].mnem );
         sprintf( prod_code, "%3d", Pat[entry->prod_id].prod_code );

      }

      /* We don't list AM and UAM since these are narrowband
         line specific and product status does not explicitly
         track generation of these products. */
      if( (Pat[entry->prod_id].prod_code > 0) && (Prod_codes_only) ){

         if( (entry->prod_id == 1) /* Alert Message */
                     ||
             (entry->prod_id == 43) /* User Alert Message */ ){
          
            entry++;
            continue;

         }

      }

      if( ((Pat[entry->prod_id].prod_code > 0) && (Prod_codes_only))
                                  ||
                          (!Prod_codes_only) ){
 
         if( scolor == YELLOW )
            fprintf( stderr, "%4d %3s  %3s %36s %3s %3d  " KYEL "%s  " RESET, 
	             entry->prod_id, prod_code, mnemonic, params, elev_index, entry->gen_pr, schedule );
         else if( scolor == BLUE )
            fprintf( stderr, "%4d %3s  %3s %36s %3s %3d  " KBLU "%s  " RESET, 
	             entry->prod_id, prod_code, mnemonic, params, elev_index, entry->gen_pr, schedule );
         else
            fprintf( stderr, "%4d %3s  %3s %36s %3s %3d  %s  ", 
	             entry->prod_id, prod_code, mnemonic, params, elev_index, entry->gen_pr, schedule );

         for( k = 0; k < hd->vdepth; k++ ){

            char *buf;
            buf = HP_print_msgid (entry->msg_ids[k], &color);
            if( color == YELLOW )
	       fprintf( stderr, KYEL " %s" RESET, buf );
            else if( color == RED )
	       fprintf( stderr, KRED " %s" RESET, buf );
            else if( color == GREEN )
	       fprintf( stderr, KGRN " %s" RESET, buf );
            else     
	       fprintf( stderr, " %s", buf );

         }

         fprintf( stderr, "\n" );

      }

      entry++;

   }

   free( msg );
   return;

}

/**************************************************************************

   Description: This function reads the current product generation status 
		info. For simplicity, we use a fixed size buffer.

   Output:	msg - pointer to the prod_status message.

   Return:	returns the message length on success or 0 on failure.

**************************************************************************/
static int Read_prod_status (char **msg){

   char *buf = NULL;
   int len, lbfd;

   /* Open the LB. */
   lbfd = LB_open( Prod_status_lb_name, LB_READ, NULL );
   if( lbfd < 0 ){

      fprintf( stderr, "LB_open( %s ) Failed: %d\n", 
               Prod_status_lb_name, lbfd );
      exit(0);

   }
 
   /* Read product status from LB. */
   len = LB_read( lbfd, &buf, LB_ALLOC_BUF, PROD_STATUS_MSG );
   if( len == LB_TO_COME ){

      fprintf( stderr, "product status message does not exist");
      return (0);

   }
   else if( len <= 0 ){

      fprintf( stderr, "LB_read %d failed (ret %d)", ORPGDAT_PROD_STATUS, len);
      exit (1);

   }

   *msg = buf;
   return (len);

}

/**************************************************************************

    Description: This function reads command line arguments.

    Inputs:	argc - number of command arguments
		argv - the list of command arguments

    Return:	It returns 0 on success or -1 on failure.

**************************************************************************/
static int Read_options (int argc, char **argv){

   extern char *optarg;    /* used by getopt */
   extern int optind;
   int c;                  /* used by getopt */
   int err;                /* error flag */
   int len = 0;

   strcpy (Prod_status_lb_name, "prod_status.lb");
   Prod_attr_table_name[0] = '\0';
   Prod_codes_only = 1;

   err = 0;
   while ((c = getopt (argc, argv, "apP:h?")) != EOF) {
      
      switch (c) {

         /* Display all product status, not just those that have product codes. */
         case 'a':
            Prod_codes_only = 0;
            break;

         /* For those who can't get out of the habit .... */
         case 'p':
            break;

         case 'P':
            strncpy( &Prod_attr_table_name[0], optarg, NAME_SIZE );
            Prod_attr_table_name[NAME_SIZE-1] = '\0';
            break;

         /* Help and usage information. */
	 case 'h':
	 case '?':
	    err = 1;
	    break;

      }

   }

   if( optind == argc - 1 ){

      /* get the LB name  */
      strncpy (Prod_status_lb_name, argv[optind], NAME_SIZE);
      Prod_status_lb_name[NAME_SIZE - 1] = '\0';

   }

   /* If Product_attr_table name is not defined, list all products. */
   if( (len = strlen( Prod_attr_table_name )) == 0 )
      Prod_codes_only = 0;

   if( err == 1 ){ /* Print usage message */

      fprintf( stderr, "Usage: %s (options) [prod_status_LB]\n", argv[0] );
      fprintf( stderr, "       - reads and prints ORPG produst generation status\n" );
      fprintf( stderr, "       Options:\n" );
      fprintf( stderr, "       -a All Products \n" );
      fprintf( stderr, "       -P Product Attribute Table Name \n" );
      fprintf( stderr, "       -h Prints usage\n" );
      fprintf( stderr, "       prod_status_LB - product status LB name; \n" );
      fprintf( stderr, "                        default - prod_status.lb\n" );
      return (-1);

    }

    return (0);

}

/**************************************************************************

    Description: This function prints the product parameters.

    Inputs:	params - the product parameters.

    Return:	A pointer to the buffer of the printed text.  The param
                string is a fixed length of 6 parameters with 6 chars 
                each for a total length of 36 chars.

**************************************************************************/
static char *HP_print_parameters( short *params ){

   static char buf[100];	/* buffer for the parameter text */
   char *pt;
   int i;

   pt = buf;
   for( i = 0; i < NUM_PROD_DEPENDENT_PARAMS; i++ ){

      int p;

      p = params[i];
      switch (p) {

         case PARAM_UNUSED:
	    strcpy (pt, "   UNU");
	    pt += 6;
	    break;

	 case PARAM_ANY_VALUE:
	    strcpy (pt, "   ANY");
	    pt += 6;
	    break;

         case PARAM_ALG_SET:
	    strcpy (pt, "   ALG");
	    pt += 6;
	    break;

	 case PARAM_ALL_VALUES:
	    strcpy (pt, "   ALL");
	    pt += 6;
	    break;

	 case PARAM_ALL_EXISTING:
	    strcpy (pt, "   EXS");
	    pt += 6;
	    break;

	 default:
	    sprintf (pt, "%6d", p);
	    pt += 6;
	    break;

      }

   }

   return (buf);

}

/**************************************************************************

    Description: This function prints the scheduling information.

    Inputs:	msg_id - the message id parameter.
                scolor = color code for text.

    Return:	A pointer to the buffer of the printed text.

**************************************************************************/
static char *HP_print_msgid( LB_id_t msg_id, int *color ){

    static char buf[9];	/* buffer for the scheduling text */
    char *pt;

    pt = buf;
    *color = 0;
    switch (msg_id) {
       case PGS_GEN_OK:
	  strcpy (pt, "   GENOK");
          *color = GREEN;
       	  break;
       case PGS_UNKNOWN:
	  strcpy (pt, "   UNKWN");
          *color = YELLOW;
	  break;
       case PGS_SCHEDULED:
	  strcpy (pt, "   SCHDL");
          *color = GREEN;
	  break;
       case PGS_NOT_SCHEDULED:
	  strcpy (pt, "  NSCHDL");
          *color = YELLOW;
	  break;
       case PGS_VOLUME_ABORTED:
	  strcpy (pt, "  VABORT");
          *color = YELLOW;
	  break;
       case PGS_TASK_NOT_RUNNING:
	  strcpy (pt, "  NOTRUN");
          *color= YELLOW;
	  break;
       case PGS_PRODUCT_NOT_GEN:
          strcpy (pt, "  NOTGEN");
          *color = YELLOW;
          break;
       case PGS_INAPPR_WX_MODE:
	  strcpy (pt, "  IWXMOD");
          *color = YELLOW;
	  break;
       case PGS_TIMED_OUT:
	  strcpy (pt, "  TIMOUT");
          *color = YELLOW;
	  break;
       case PGS_REQED_NOT_SCHED:
	  strcpy (pt, "  RQNSCH");
	  break;
       case PGS_DISABLED_MOMENT:
	  strcpy (pt, "  DISMOM");
          *color = YELLOW;
	  break;
       case PGS_TASK_FAILED:
	  strcpy (pt, "  TSKFLR");
          *color = RED;
	  break;
       case PGS_SLOT_UNAVAILABLE:
	  strcpy (pt, "  SLOTUN");
          *color = YELLOW;
	  break;
       case PGS_INVALID_PARAMS:
	  strcpy (pt, "  INPARM");
          *color = YELLOW;
	  break;
       case PGS_DATA_SEQ_ERROR:
	  strcpy (pt, "  SEQERR");
          *color = YELLOW;
	  break;
       case PGS_TASK_SELF_TERM:
	  strcpy (pt, "   STERM");
          *color = RED;
	  break;
       case PGS_PRODUCT_DISABLED:
	  strcpy (pt, "   PRDIS");
          *color = YELLOW;
	  break;
       default:
	  sprintf (pt, "%8d", msg_id);
	  break;
    }
    return (buf);
}

/**************************************************************************

    Description: This function prints the scheduling information.

    Inputs:	schedule - the scheduling parameter.
                scolor = color code for text.

    Return:	A pointer to the buffer of the printed text.

**************************************************************************/
static char *HP_print_schedule (char schedule, int *scolor){

    static char buf[8];	/* buffer for the scheduling text */
    char *pt;

    pt = buf;
    *scolor = 0;
    switch (schedule) {
       case PGS_SCH_NOT_SCHEDULED:
	  strcpy (pt, " NOTSCH");
          *scolor = YELLOW;
       	  break;
       case PGS_SCH_SCHEDULED:
	  strcpy (pt, "    SCH");
       	  break;
       case PGS_SCH_BY_REQUEST:
	  strcpy (pt, "    REQ");
          *scolor = BLUE;
	  break;
       case (PGS_SCH_BY_REQUEST + PGS_SCH_SCHEDULED):
          strcpy (pt, "SCH/REQ");
          *scolor = BLUE;
          break;
       case PGS_SCH_BY_DEFAULT:
	  strcpy (pt, "    DEF");
	  break;
       case (PGS_SCH_SCHEDULED + PGS_SCH_BY_DEFAULT):
	  strcpy (pt, "SCH/DEF");
	  break;
       case (PGS_SCH_BY_REQUEST + PGS_SCH_BY_DEFAULT):
	  strcpy (pt, "REQ/DEF");
          *scolor = BLUE;
	  break;
       case (PGS_SCH_SCHEDULED + PGS_SCH_BY_REQUEST + PGS_SCH_BY_DEFAULT):
	  strcpy (pt, "S/RQ/DF");
          *scolor = BLUE;
	  break;
       default:
	  sprintf (pt, "%7d", schedule);
	  break;
    }

    return (buf);

}
/**************************************************************************

    Description: This function prints the elevation index information.

    Inputs:	elev_index - the elevation index parameter.

    Return:	A pointer to the buffer of the printed text.

**************************************************************************/
static char *HP_print_elev_index (short elev_index){

    static char buf[4];	/* buffer for the scheduling text */
    char *pt;

    pt = buf;
    switch (elev_index) {
       case PGS_ELIND_ALL_ELEVATIONS:
	  strcpy (pt, " AE");
       	  break;
       case PGS_ELIND_NOT_SCHEDULED:
	  strcpy (pt, " NS");
       	  break;
       default:
	  sprintf (pt, "%3d", elev_index);
	  break;
    }

    return (buf);

}

/**************************************************************************
   Description: 
      This function parses the ASCII PAT.

   Input:
      file_name - File name of the ASCII PAT.

   Output: 
      NONE

   Return:
      It returns the number of entries of the table on success
      or -1 on failure.

**************************************************************************/
static int Read_ASCII_PAT( char *file_name ){

    int         len, err, offset;
    prod_id_t   prod_id;
    short       prod_code;
    char        mne [4];
    char        desc [128];
    Pat_t       *pat = NULL;

    err = 0;

    /* Initialize the PAT. */
    memset( Pat, 0, sizeof(Pat_t)*MAX_PAT_TBL_SIZE );

    /* Check if the file_name is defined. */
    if( (len = strlen( file_name )) == 0 ){

       fprintf( stderr, "Product Attribute Table Not Defined/Available.\n" );
       return -1;

    }

    CS_cfg_name ( file_name );
    CS_control (CS_COMMENT | '#');
    CS_control (CS_RESET);

    /*  Repeat for all the product definitions in the product_attributes    
        configuration file. */

    do {

        if (CS_level (CS_DOWN_LEVEL) < 0)
            continue;

        if (CS_entry ("prod_id", 1 | CS_SHORT, 0,
                                        (char *)&prod_id) <= 0 ||
            CS_entry ("prod_code", 1 | CS_SHORT, 0,
                                        (char *)&prod_code) <= 0) {
            err = 1;
            break;

        }

        if ((len = CS_entry ("desc", 1, 256, (char *)desc)) < 0) {

            err = 1;
            break;

        }

        if( (prod_id < 0) || (prod_id >= MAX_PAT_TBL_SIZE) )
           break;

        /* Initialize the product mnemonic. */
        memset (mne, 0, 4);

        /* Since the product mnemonic is at the beginning of the   
           description, move the pointer to the first character    
           after the first space character. */

        offset = 0;

        while (strncmp ((desc + offset)," ",1)) {

           offset++;

        }

        offset++;

        if (offset > 1) {

           if (offset > MAX_MNE_LENGTH+1) {

              offset = MAX_MNE_LENGTH;

           }

           strncpy (mne, desc, offset-1);

        }

        /* Add this product to the PAT. */
        pat = (Pat_t *) &Pat[prod_id];

        pat->prod_id = prod_id;
        pat->prod_code = prod_code;
        memcpy( pat->mnem, mne, 4 );

        /* Prepare for next product_attr_table entry. */
        CS_level (CS_UP_LEVEL);

    } while (CS_entry (CS_NEXT_LINE, 0, 0, NULL) >= 0);

    CS_cfg_name ("");

    if( err )
       return -1;

    return 0;

}

/**************************************************************************

    Description: This function converts UNIX time to day/month/year.

    Inputs:     timevalue - the UNIX time.

    Returns:    Date string in dd/mm/yy format.

**************************************************************************/
static char *calendar_date( time_t timevalue ){

   int l,n, julian;
   int dd, dm, dy;
   short date;
   static char date_string[10];

   /* Convert UNIX time to modified Julian date */
   date = timevalue/86400 + 1;

   /* Convert modified julian to type integer */
   julian = date;

   /* Convert modified julian to year/month/day */
   julian += 2440587;
   l = julian + 68569;
   n = 4*l/146097;
   l = l -  (146097*n + 3)/4;
   dy = 4000*(l+1)/1461001;
   l = l - 1461*dy/4 + 31;
   dm = 80*l/2447;
   dd= l -2447*dm/80;
   l = dm/11;
   dm = dm+ 2 - 12*l;
   dy = 100*(n - 49) + dy + l;
   if( dy >= 2000 )
      dy = dy - 2000;
   else
      dy = dy - 1900;

   /* Convert numbers to ASCII. */
   sprintf(date_string, "%02d/%02d/%02d", dm, dd, dy );

   return (date_string);
}

/**************************************************************************

    Description: This function converts UNIX time to hrs, minutes, seconds.

    Inputs:     timevalue - the UNIX time.

    Returns:    Time string in hr:min:sec format.

**************************************************************************/
static char *convert_time( time_t timevalue ){

   int hrs, mins, secs;
   static char time_string[10];

   /* Convert the UNIX time to seconds since midnight. */
   timevalue %= 86400;

   /* Extract the number of hours. */
   hrs = timevalue/3600;

   /* Extract the number of minutes. */
   timevalue = timevalue - hrs*3600;
   mins = timevalue/60;

   /* Extract the number of seconds. */
   secs = timevalue - mins*60;

   /* Convert numbers to ASCII. */
   sprintf( time_string, "%02d:%02d:%02d", hrs, mins, secs );

   return (time_string);
}

