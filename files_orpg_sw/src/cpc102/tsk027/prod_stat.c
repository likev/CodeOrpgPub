/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/08/28 19:04:29 $
 * $Id: prod_stat.c,v 1.42 2014/08/28 19:04:29 steves Exp $
 * $Revision: 1.42 $
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



static int Event = 0;
static int Rate = 1;
static int Volume = 0;
static int Print_status = 0;
static char Prod_status_lb_name[NAME_SIZE];
static Summary_Data Summary;
static int Product_codes_only;

static int Read_options (int argc, char **argv);
static void Print_prod_status ();
static int Read_prod_status (char **msg);
static char *HP_print_parameters (short *params);
static char *HP_print_msgid (LB_id_t msg_id, int *color);
static char *HP_print_schedule (char schedule, int *scolor);
static char *HP_print_elev_index (short elev_index);
static char *convert_time( time_t timevalue );
static char *calendar_date( time_t timevalue );
void En_callback( EN_id_t evtcd, char *msg, int msglen,
                  void *arg );


/******************************************************************

    Description: The main function.

******************************************************************/

int main (int argc, char **argv){

    /* read options */
    if (Read_options (argc, argv) != 0)
	exit (-1);

    /* Initialize the LE service */
    if( ORPGMISC_init( argc, argv, 100, 0, -1, 0 ) < 0 ){

       fprintf( stderr, "ORPGMISC_init Failed\n" );;
       ORPGTASK_exit( 0 );

    }

    CS_error ((void (*)())printf);

    /* register event */
    if (Event >= 0) {
	int ret;

	ret = EN_register (Event, En_callback);
	if (ret < 0) {
	    LE_send_msg (0, 
		"EN_register failed (event %d, ret %d)", Event, ret);
	    exit (1);
	}
    }

    if( Event >= 0 ){

       while(1){

         if( Print_status ){

            /* Print the product status. */
            Print_status = 0;
            Print_prod_status ();

         }

         /* Sleep .... waiting for next event. */
	 sleep(100);

       }

    }
    else {

       /* One and done. */
       Print_prod_status ();

    }

    return 0;

}

/**************************************************************************

    Description: This function reads and prints the current product 
		generation status info.

**************************************************************************/

static void Print_prod_status ()
{
    int color = 0, scolor = 0, len;
    char *msg;
    Prod_gen_status_header *hd;
    Prod_gen_status *entry;
    int scan_summary_read, i;

    if ((len = Read_prod_status (&msg)) == 0){

        LE_send_msg( 0, "Read_prod_status Returned 0\n" );
	return;

    }

    /* check */
    hd = (Prod_gen_status_header *)msg;
    if (len < sizeof (Prod_gen_status_header) ||
	hd->list < sizeof (Prod_gen_status_header) || hd->length < 0 ||
	len != hd->list + hd->length * sizeof (Prod_gen_status)) {
	LE_send_msg (0, 
	    "Bad Prod_gen_status msg (msg_len %d, list %d, len %d)",
			len, hd->list, hd->length);
	exit (1);
    }

    /* Read the scan summary data. */
    scan_summary_read = ORPGDA_read( ORPGDAT_SCAN_SUMMARY, &Summary, 
                                     sizeof( Summary_Data ), SCAN_SUMMARY_ID );

    if( scan_summary_read < 0 )
       LE_send_msg( GL_INFO, "Unable to Read Scan Summary Data\n" );

    /* print the info */
    printf ("%d products; %d volumes listed\n", hd->length, hd->vdepth);

    printf ("Volume num  : ");
    for (i = 0; i < hd->vdepth; i++)
	printf ("%10d", (int)hd->vnum[i]);
    printf ("\n");
    printf ("Volume time : ");
    for (i = 0; i < hd->vdepth; i++){

        char *time_string;
 
        time_string = convert_time( hd->vtime[i] ); 
	printf ("  %s", time_string);

    }
    printf ("\n");
    printf ("Volume date : ");
    for (i = 0; i < hd->vdepth; i++){

        char *date_string;
 
        date_string = calendar_date( hd->vtime[i] ); 
	printf ("  %s", date_string);

    }
    printf ("\n");

    if( scan_summary_read > 0 ){

       int vcp_num;

       printf ("VCP/Wx Mode: ");
       for (i = 0; i < hd->vdepth; i++){

          int vol_num;

          vol_num = ORPGMISC_vol_scan_num( (unsigned int) hd->vnum[i] );
          vcp_num = (int) hd->vcpnum[i]; 

          if( hd->wx_mode[i] == PRECIPITATION_MODE )
   	     printf ("   %2d  / A", vcp_num );
          else if( hd->wx_mode[i] == CLEAR_AIR_MODE )
	     printf ("   %2d  / B", vcp_num );
          else if( hd->wx_mode[i] == MAINTENANCE_MODE )
	     printf ("   %2d  / M", vcp_num );

          else{

             int wx_mode;

             wx_mode = Summary.scan_summary[vol_num].weather_mode;
             vcp_num = Summary.scan_summary[vol_num].vcp_number; 
             if( wx_mode == PRECIPITATION_MODE )
   	        printf ("   %2d  / A", vcp_num );
             else if( wx_mode == CLEAR_AIR_MODE )
	        printf ("   %2d  / B", vcp_num );
             else if( hd->wx_mode[i] == MAINTENANCE_MODE )
	        printf ("   %2d  / M", vcp_num );

          }

       }

    }
    else{

       printf ("Weather Mode: ");
       for (i = 0; i < hd->vdepth; i++){

           if( hd->wx_mode[i] == PRECIPITATION_MODE )
   	      printf ("         A");
           else if( hd->wx_mode[i] == CLEAR_AIR_MODE )
	      printf ("         B");
           else if( hd->wx_mode[i] == CLEAR_AIR_MODE )
	      printf ("         M");

       }

    }
    printf ("\n");
    printf ("\n");

    printf (" ID Code Mnc      P1    P2    P3    P4    P5    P6 El# Genpr Schdl        Message IDs\n");

    entry = (Prod_gen_status *)(msg + hd->list);
    for (i = 0; i < hd->length; i++) {

	int k, prod_code;
        char *params, *schedule, *mnemonic, *elev_index;

        params = HP_print_parameters (entry->params);
        schedule = HP_print_schedule (entry->schedule, &scolor);
        mnemonic = ORPGPAT_get_mnemonic( (int) entry->prod_id );
        prod_code = ORPGPAT_get_code( (int) entry->prod_id );
        elev_index = HP_print_elev_index( entry->elev_index );

        /* We don't list AM and UAM since these are narrowband
           line specific and product status does not explicitly
           track generation of these products. */
        if( (prod_code > 0) && (Product_codes_only) ){

            if( prod_code == 9 /* Alert Message */
                             ||
                prod_code == 73 /* User Alert Message */ ){
          
                entry++;
                continue;

            }

        }

        if( (prod_code > 0 && Product_codes_only)
                           ||
            (!Product_codes_only) ){

           if( mnemonic != NULL ){

              if( scolor == YELLOW )
	         printf ("%3d %3d  %3s  %3s %3s %3d  " KYEL "%s  " RESET, 
		         entry->prod_id, prod_code, 
		         mnemonic, params, elev_index, entry->gen_pr,
                         schedule);
              else if( scolor == BLUE )
	         printf ("%3d %3d  %3s  %3s %3s %3d  " KBLU "%s  " RESET, 
		         entry->prod_id, prod_code, 
		         mnemonic, params, elev_index, entry->gen_pr,
                         schedule);
              else
	         printf ("%3d %3d  %3s  %3s %3s %3d  %s  ", 
		         entry->prod_id, prod_code, 
		         mnemonic, params, elev_index, entry->gen_pr,
                         schedule);

           }
           else{
 
              if( scolor == YELLOW )
	         printf ("%3d %3d      %3s %3s %3d  " KYEL "%s  " RESET, 
		         entry->prod_id, prod_code, 
		         params, elev_index, entry->gen_pr, schedule);
              else if( scolor == BLUE )
	         printf ("%3d %3d      %3s %3s %3d  " KBLU "%s  " RESET, 
		         entry->prod_id, prod_code, 
		         params, elev_index, entry->gen_pr, schedule);
              else 
	         printf ("%3d %3d      %3s %3s %3d  %s  ", 
		         entry->prod_id, prod_code, 
		         params, elev_index, entry->gen_pr, schedule);

           }

	   for (k = 0; k < hd->vdepth; k++) {

              char *buf;
              buf = HP_print_msgid (entry->msg_ids[k], &color);
              if( color == YELLOW )
	         printf (KYEL " %s" RESET, buf);
              else if( color == RED )
	         printf (KRED " %s" RESET, buf);
              else if( color == GREEN )
	         printf (KGRN " %s" RESET, buf);
              else     
	         printf (" %s", buf);

           }
	   printf ("\n");
        }

	entry++;
    }

    free(msg);

    if( Event > 0 ){

       printf("==========================================================\n");
       printf("--------------------> End of Listing <--------------------\n");
       printf("==========================================================\n");

    }

    /* Flush stdout. */
    fflush( stdout );

    return;
}

/**************************************************************************

    Description: This function reads the current product generation status 
		info. For simplicity, we use a fixed size buffer.

    Output:	msg - pointer to the prod_status message.

    Return:	returns the message length on success or 0 on failure.

**************************************************************************/

static int Read_prod_status (char **msg)
{
    char *buf = NULL;
    int len;

    /* Read product status from LB. */
    len = ORPGDA_read( ORPGDAT_PROD_STATUS, &buf, LB_ALLOC_BUF, 
                       PROD_STATUS_MSG );
    if (len == LB_TO_COME) {
	LE_send_msg (0, "product status message does not exist");
	return (0);
    }
    if (len <= 0) {
	LE_send_msg( 0, "ORPGDA_read %d failed (ret %d)", 
		     ORPGDAT_PROD_STATUS, len);
	exit (1);
    }
    *msg = buf;
    return (len);
}

/**************************************************************************

    Description: This is the EN callback function.

    Inputs:	evtcd - event number.
		msg - event message.
		msglen - length of the message.

**************************************************************************/

void En_callback( EN_id_t evtcd, char *msg, int msglen,
                  void *arg )
{

    LE_send_msg( 0, "Volume: %d, Rate: %d\n", Volume, Rate );
    if( (Volume % Rate) == 0 ){

       Print_status = 1;
       LE_send_msg( 0, "Print_status: %d\n", Print_status );

    }

    /* Increment the volume number. */
    Volume++;

    return;
}

/**************************************************************************

    Description: This function reads command line arguments.

    Inputs:	argc - number of command arguments
		argv - the list of command arguments

    Return:	It returns 0 on success or -1 on failure.

**************************************************************************/

static int Read_options (int argc, char **argv)
{
    extern char *optarg;    /* used by getopt */
    extern int optind;
    int c;                  /* used by getopt */
    int err;                /* error flag */

    Event = -1;
    Rate = 1;
    Product_codes_only = 1;
    strcpy (Prod_status_lb_name, "prod_status.lb");
    err = 0;
    while ((c = getopt (argc, argv, "aer:ph?")) != EOF) {
	switch (c) {

            /* Dumps product status based on ORPGEVT_PROD_GEN_CONTROL event. */
	    case 'e':
                Event = ORPGEVT_PROD_GEN_CONTROL;
		break;

            /* Specifies the rate at which the status is dumps ... to be used in
               conjunction with -e option. */
	    case 'r':
		if (sscanf (optarg, "%d", &Rate) != 1)
		    err = 1;
		break;

            /* For those who can't get out of the habit .... */
            case 'p':
               break;

            /* Display all product status, not just those that have product codes. */
            case 'a':
               Product_codes_only = 0;
               break;

            /* Help and usage information. */
	    case 'h':
	    case '?':
		err = 1;
		break;
	}
    }

    if (optind == argc - 1) {      /* get the LB name  */
	strncpy (Prod_status_lb_name, argv[optind], NAME_SIZE);
	Prod_status_lb_name[NAME_SIZE - 1] = '\0';
    }

    if (err == 1) {              /* Print usage message */
	printf ("Usage: %s (options) [prod_status_LB]\n", argv[0]);
	printf ("       - reads and prints ORPG produst generation status\n");
	printf ("       Options:\n");
	printf ("       prod_status_LB - product status LB name; \n");
	printf ("                        default - prod_status.lb\n");
	printf ("       -e (prints the prod gen info upon receiving\n");
	printf ("           ORPGEVT_PROD_GEN_CONTROL event;\n");
	printf ("          default: prints immediately and terminates)\n");
        printf ("       -a output contains information about all product codes\n" );
        printf ("       -r output rate, in number of volume scans\n" );
	return (-1);
    }

    return (0);
}

/**************************************************************************

    Description: This function prints the product parameters.

    Inputs:	params - the product parameters.

    Return:	A pointer to the buffer of the printed text.

**************************************************************************/

static char *HP_print_parameters (short *params)
{
    static char buf[100];	/* buffer for the parameter text */
    char *pt;
    int i;

    pt = buf;
    for (i = 0; i < NUM_PROD_DEPENDENT_PARAMS; i++) {
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

static char *HP_print_schedule (char schedule, int *scolor)
{
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

static char *HP_print_elev_index (short elev_index)
{
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

    Description: This function converts UNIX time to hrs, minutes, seconds.

    Inputs:	timevalue - the UNIX time.

    Returns:    Time string in hr:min:sec format.

**************************************************************************/

static char *convert_time( time_t timevalue ){
 
   int hrs, mins, secs;
   static char time_string[10];

   /*
     Convert the UNIX time to seconds since midnight.
   */
   timevalue %= 86400;

   /* 
     Extract the number of hours.
   */
   hrs = timevalue/3600;

   /* 
     Extract the number of minutes.
   */
   timevalue = timevalue - hrs*3600;
   mins = timevalue/60;

   /* 
     Extract the number of seconds.
   */
   secs = timevalue - mins*60;

   /* 
     Convert numbers to ASCII.
   */
   sprintf( time_string, "%02d:%02d:%02d", hrs, mins, secs );

   return (time_string);
}

/**************************************************************************

    Description: This function converts UNIX time to day/month/year.

    Inputs:	timevalue - the UNIX time.

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

   /* 
     Convert numbers to ASCII.
   */
   sprintf(date_string, "%02d/%02d/%02d", dm, dd, dy );
 
   return (date_string);
}
