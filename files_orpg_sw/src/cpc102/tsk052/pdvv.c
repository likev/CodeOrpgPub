
/******************************************************************

	file: pdvv.c

	This is the product distribution V&V tool.
	
******************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2008/02/19 16:00:24 $
 * $Id: pdvv.c,v 1.11 2008/02/19 16:00:24 steves Exp $
 * $Revision: 1.11 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <orpg.h> 
#include <mrpg.h> 
#include <infr.h> 
 
#include <prod_user_msg.h>

#define VV_NAME_SIZE 	128

static int Line_index;		/* product distribution line index */
static int Command;		/* V&V control command */
static int Print;               /* Format each entry. */
static int Queue;               /* Format Write Queue. */
static int Vv_fd;		/* V&V data file fd */

static int Read_options (int argc, char **argv);
static void Print_usage (char **argv);
static void Print_pdvv_file ();
static void Print_pdvv_queue ();
static void Sort_records( Vv_record_t *array, int size );
static int Open_pdvv_file ();
static void Print_vv_record (Vv_record_t *vv);
static void Print_vv_queue (Vv_record_t *vv, int num_recs);
static char *Print_parameters (short *params);
static char *Print_rr_error_code( int error );


/******************************************************************

    Description: The main function.

******************************************************************/
int main (int argc, char **argv){

    if (Read_options (argc, argv) != 0)
	exit (1);

    ORPGDA_set_event_msg_byteswap_function ();
    if (Command >= 0) {
	if (ORPGNBC_send_NB_vv_control_command (Command) != 0)
	    fprintf (stderr, "ORPGNBC_send_NB_vv_control_command failed\n");
	else
	    fprintf (stderr, "V&V command sent to p_servers\n");
	exit (0);
    }

    if (Open_pdvv_file () != 0)
	exit (1);

    if( Print )
       Print_pdvv_file ();

    else if( Queue )
       Print_pdvv_queue ();

    exit (0);
}

/******************************************************************

    Prints the V&V data in queue format.

******************************************************************/
static void Print_pdvv_queue (){

    Vv_record_t *vv = NULL, *temp_vv = NULL;
    int num_recs = 0, temp_max_recs, max_recs = 0;


    while (1) {

        if( num_recs >= max_recs ){

           temp_vv = vv;
           temp_max_recs = max_recs;
           max_recs += 100;
           vv = (Vv_record_t *) calloc( 1, sizeof( Vv_record_t )*max_recs );
           if( vv == NULL ){

              fprintf( stderr, "calloc failed for vv\n" );
              exit(1);

           }

           if( temp_vv != NULL ){

              memcpy( vv, temp_vv, temp_max_recs*sizeof( Vv_record_t ) );
              free( temp_vv );
              temp_vv = NULL;

           }

        }

	if (read (Vv_fd, &vv[num_recs], sizeof (Vv_record_t)) != 
						sizeof (Vv_record_t))
	    break;

        num_recs++;

    }

    Sort_records( vv, num_recs);
    Print_vv_queue( vv, num_recs );

}

/******************************************************************

    Prints the V&V data in queue format.

******************************************************************/
static void Print_pdvv_file (){

    Vv_record_t vv;

    while (1) {

	if (read (Vv_fd, &vv, sizeof (Vv_record_t)) != 
						sizeof (Vv_record_t))
	    break;

        Print_vv_record (&vv);
    }

}

/******************************************************************

    Prints a V&V data record.

******************************************************************/
static void Print_vv_record (Vv_record_t *vv) {
    char buf[512], *cpt;
    int yy, mon, dd, hh, min, ss;
    int prod_id = 0;
    char prod_str[32], *mnemonic = NULL;
    static char *status[] = { "Success  ",
                              "Lost     ",
                              "Loadshed ",
                              "Discarded" };

    static char *alert_status[] = { "???????????",
                                    "1st-Time   ",
                                    "Alert Ended" };

    cpt = buf;
    prod_str[0] = '\0';

    if( (vv->type == VV_ALERT) || (vv->type == VV_PRODUCT) ){

      if (vv->type == VV_ALERT)
          strcpy (cpt, "ALERT:");
       else if (vv->type == VV_PRODUCT)
	  strcpy (cpt, "PRODUCT:");
       else {
	  printf ("Unknown V&V data type %d\n", vv->type);
	  return;
       }
       cpt += strlen (cpt);

       unix_time (&(vv->t_gen), &yy, &mon, &dd, &hh, &min, &ss);
       sprintf (cpt, 
	" Status: %s, Gen Time: %.2d/%.2d/%.2d %.2d:%.2d:%.2d, Times (Secs): Enqueue: %d, In Queue: %d,", 
		status[vv->status], mon, dd, (yy - 1900) % 100, hh, min, ss, 
		(int)vv->t_eq - (int)vv->t_gen, (int)vv->t_dq - (int)vv->t_eq);
       cpt += strlen (cpt);

       if (vv->status == VV_SUCCESS) {
          sprintf (cpt, " Xmt: %d", (int)vv->t_sent - (int)vv->t_dq);
 	  cpt += strlen (cpt);
       }

       sprintf (cpt, "\n\t Line Index: %d, Vol #: %d, Xmt Prty: %d, Seq #: %d, Size: %d bytes", 
		vv->line_ind, vv->vol_num, vv->priority, vv->seq_num, vv->size);
       cpt += strlen (cpt);

       *prod_str = '\0';
       if( vv->type == VV_ALERT )
          prod_id = ORPGPAT_get_prod_id_from_code( vv->t.alert.code );
       else if( vv->type == VV_PRODUCT )
          prod_id = ORPGPAT_get_prod_id_from_code( vv->t.prod.id );

       if( prod_id != ORPGPAT_ERROR ){

          mnemonic = ORPGPAT_get_mnemonic( prod_id );
          if( mnemonic != NULL )
             memcpy( prod_str, mnemonic, strlen(mnemonic) + 1 );
    
       }

       if (vv->type == VV_ALERT){

           short area_number = vv->t.alert.area_number;
           short category = vv->t.alert.category;
           short status = vv->t.alert.status;

           unix_time (&(vv->t.alert.t_det), &yy, &mon, &dd, &hh, &min, &ss);
	   sprintf (cpt, "\n\t Prod Code: %3d (%s), Area: %d, Cat: %d, Status: %s, Time Det: %.2d:%.2d:%.2d\n",
	            vv->t.alert.code, prod_str, SHORT_BSWAP_L(area_number), 
	            SHORT_BSWAP_L(category), alert_status[SHORT_BSWAP_L(status)], hh, min, ss); 

       }
       else if (vv->type == VV_PRODUCT)
   	   sprintf (cpt, "\n\t Prod Code: %3d (%s), Params: %s\n",
	            vv->t.prod.id, prod_str, Print_parameters( vv->t.prod.params ));

    }
    else if (vv->type == VV_RR){

       strcpy (cpt, "REQ/RSP:");
       cpt += strlen (cpt);

       unix_time (&(vv->t_eq), &yy, &mon, &dd, &hh, &min, &ss);
       sprintf (cpt, 
	" Status: %s, Enqueue Time: %.2d/%.2d/%.2d %.2d:%.2d:%.2d, Times (Secs): In Queue: %d,", 
		status[vv->status], mon, dd, (yy - 1900) % 100, hh, min, ss, 
		(int)vv->t_dq - (int)vv->t_eq);
       cpt += strlen (cpt);

       if (vv->status == VV_SUCCESS) {
          sprintf (cpt, " Xmt: %d", (int)vv->t_sent - (int)vv->t_dq);
 	  cpt += strlen (cpt);
       }

       sprintf (cpt, "\n\t Line Index: %d, Vol #: %d, Xmt Prty: %d, Seq #: %d, Size: %d bytes", 
            vv->line_ind, vv->vol_num, vv->priority, vv->seq_num, vv->size);
       cpt += strlen (cpt);

       prod_id = ORPGPAT_get_prod_id_from_code( vv->t.rr.prod_code );

       if( prod_id != ORPGPAT_ERROR ){

          mnemonic = ORPGPAT_get_mnemonic( prod_id );
          if( mnemonic != NULL )
             memcpy( prod_str, mnemonic, strlen(mnemonic) + 1 );
    
       }

       sprintf (cpt, "\n\t Prod Code: %3d (%s), Elev: %d, Reason: %s\n",
	    vv->t.rr.prod_code, prod_str, vv->t.rr.elev, Print_rr_error_code(vv->t.rr.er_code));
       cpt += strlen (cpt);

    }

    printf ("%s\n", buf);
}

/******************************************************************

    Prints V&V output in Queue format.

******************************************************************/
static void Print_vv_queue (Vv_record_t *vv, int num_recs) {

   int i, j, prod_id = 0, prod_code = 0, write_header;
   int total_size;
   time_t sent_time = 0;
   char prod_str[4], *mnemonic = NULL;

   for( i = 0; i < num_recs; i++ ){
  
      if( (vv[i].type == VV_PRODUCT) ){

         if( (vv[i].status == VV_LOST)
                      ||
             (vv[i].status == VV_LOADSHED)
                      ||
             (vv[i].status == VV_DISCARDED) ){

            Print_vv_record( &vv[i] );
            continue;

         }

      }

      sent_time = vv[i].t_sent;
      Print_vv_record( &vv[i] );

      j = i+1;
      write_header = 1;
      total_size = 0;
      while( j < num_recs ){

         if( vv[j].t_eq <= sent_time){

            if( vv[j].status != VV_SUCCESS && vv[j].type == VV_PRODUCT ){

               int k, ignore = 0;

               for( k = j+1; k < num_recs; k++ ){

                  if( vv[k].type == VV_RR && vv[k].t_eq <= sent_time ){

                     if( vv[k].seq_num == vv[j].seq_num ){

                        ignore = 1;
                        break;

                     }

                  }

               }
   
               if( ignore ){

                  j++;
                  continue;

               }

            }

            if( write_header ){

               printf( "--->Products In Queue\n" );
               write_header = 0;

            }

            strcpy( prod_str, "   " );
            if( vv[j].type == VV_ALERT ){

               prod_code = vv[j].t.alert.code;
               prod_id = ORPGPAT_get_prod_id_from_code( prod_code );

            }
            else if( vv[j].type == VV_PRODUCT ){

               prod_code = vv[j].t.prod.id;
               prod_id = ORPGPAT_get_prod_id_from_code( prod_code );

            }
            if( prod_id != ORPGPAT_ERROR && vv[j].type != VV_RR ){
   
               mnemonic = ORPGPAT_get_mnemonic( prod_id );
               if( mnemonic != NULL )
                  memcpy( prod_str, mnemonic, strlen(mnemonic) );

            }
            else if( vv[j].type == VV_RR ){

               prod_code = vv[j].t.rr.code;
               strcpy( prod_str, "RR " );

            }
   
            printf( "        Prod Code: %3d (%s), Vol #: %2d, Seq #: %5d, Size: %6d, Xmt Prty: %8d, LS Prty: %8d\n",
                    prod_code, prod_str, vv[j].vol_num, vv[j].seq_num, vv[j].size, vv[j].priority,
                    vv[j].ls_priority );

            total_size += vv[j].size;

         }     
         j++;

      }

      printf( "    Total Number of Bytes In Queue: %d\n\n", total_size );

   }

}

/*******************************************************************
   Description:
      This function sorts the entries into increasing order
      of enqueue time.  
  
   Inputs:
      array - pointer to array to sort.
      size - size of array to sort.
  
   Returns:
      Returns void.

   Notes:
      Insertion sort was choosen since the input array should be
      very nearly sorted. 

******************************************************************/
static void Sort_records( Vv_record_t *array, int size ){

   int index, place;
   Vv_record_t current;
   
   /* Sort the array. */
   for( index = 1; index < size; index++ ){
  
      if( (array + index)->t_dq < (array + index - 1)->t_dq ){

         current = *(array+index); 
         for( place = index-1; place >= 0; place-- ){

            *(array + place + 1) = *(array + place);
            if( (place == 0) 
                      ||
                ((array + place - 1)->t_dq <= current.t_dq) )
               break; 

         }

         *(array + place) = current;

      }
  
   /* End of "for" loop. */
   }
   
/* End of Sort_records( ) */
}              

/**************************************************************************

   Description: This function prints the product parameters.
  
   Inputs:     params - the product parameters.
 
   Return:     A pointer to the buffer of the printed text.
 
**************************************************************************/
static char *Print_parameters (short *params){

   static char buf[64];        /* buffer for the parameter text */
   char *pt;
   int i;
  
   pt = buf;
   for (i = 0; i < NUM_PROD_DEPENDENT_PARAMS; i++) {

      int p;
  
      p = params[i];
      switch (p) {

         case PARAM_UNUSED:
            strcpy (pt, "UNU    ");
            pt += 7;
            break;
         case PARAM_ANY_VALUE:
            strcpy (pt, "ANY    ");
            pt += 7;
            break;
         case PARAM_ALG_SET:
            strcpy (pt, "ALG    ");
            pt += 7;
            break;
         case PARAM_ALL_VALUES:
            strcpy (pt, "ALL    ");
            pt += 7;
            break;
         case PARAM_ALL_EXISTING:
            strcpy (pt, "EXS    ");
            pt += 7;
            break;
         default:
            sprintf (pt, "%6d ", p);
            pt += strlen (pt);
            break;

      }

   }
   return (buf);

/* End of Print_parameters() */
}                       

/**************************************************************************

   Description: Enocdes the RR error code as a string.

   Inputs:      error - Request/Response Message error code.

**************************************************************************/
static char* Print_rr_error_code( int error_code ){

   static char err[24];
   unsigned int error = (unsigned int) error_code;

   switch( error ){

      case FULLWORD_SHIFT(RR_NO_SUCH_MSG):

         sprintf( err, "  NO SUCH MSG CODE" );
         break;

      case FULLWORD_SHIFT(RR_NO_SUCH_PROD):

         sprintf( err, " NO SUCH PROD CODE" );
         break;

      case FULLWORD_SHIFT(RR_NOT_GENERATED):

         sprintf( err, "PROD NOT GENERATED" );
         break;

      case FULLWORD_SHIFT(RR_ONETIME_GEN_FAILED): 

         sprintf( err, "     OT GEN FAILED" );
         break;

      case FULLWORD_SHIFT(RR_NB_LOADSHED):  

         sprintf( err, "       NB LOADSHED" );
         break;

      case FULLWORD_SHIFT(RR_ILLEGAL_REQ): 

         sprintf( err, "   ILLEGAL REQUEST" );
         break;

      case FULLWORD_SHIFT(RR_MEM_LOADSHED):

         sprintf( err, "   MEMORY LOADSHED" );
         break;

      case FULLWORD_SHIFT(RR_CPU_LOADSHED): 

         sprintf( err, "      CPU LOADSHED" );
         break;

      case FULLWORD_SHIFT(RR_SLOT_FULL):   

         sprintf( err, "  UNAVAILABLE SLOT" );
         break;
  
      case FULLWORD_SHIFT(RR_TASK_FAILED): 

         sprintf( err, "      TASK FAILURE" );
         break;

      case FULLWORD_SHIFT(RR_TASK_UNLOADED): 

         sprintf( err, "  TASK UNAVAILABLE" );
         break;

      case FULLWORD_SHIFT(RR_AVAIL_NEXT_VOL): 

         sprintf( err, "   AVAIL NEXT SCAN" );
         break;

      case FULLWORD_SHIFT(RR_MOMENT_DISABLED):

         sprintf( err, "   MOMENT DISABLED" );
         break;

      case FULLWORD_SHIFT(RR_INVALID_PASSWD): 

         sprintf( err, "  INVALID PASSWORD" );
         break;

      default:

         sprintf( err, "?????? %4x", error );
         break;

   /* End of "switch" statement. */
   }

   return (err);

/* End of Print_rr_error_code() */
}

/******************************************************************

    Opens the V&V data file.

    Returns 0 on success or -1 on failure.

******************************************************************/

static int Open_pdvv_file () {
    int len;
    char path[VV_NAME_SIZE];

    if ((len = MISC_get_work_dir (path, VV_NAME_SIZE)) <= 0 ||
	len + 24 >= VV_NAME_SIZE) {	/* 24 is the max file name len */
	fprintf (stderr, 
	    "MISC_get_work_dir (V&V) failed (ret %d)\n", len);
	return (-1);
    }
    sprintf (path + strlen (path), "/pdvv.%d", Line_index);
    Vv_fd = open (path, O_RDONLY);
    if (Vv_fd < 0) {
	fprintf (stderr, "open V&V file (%s) failed\n", path);
	return (-1);
    }
    return (0);
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

    err = 0;
    Line_index = -1;
    Command = -1;
    Print = 1;
    Queue = 0;
    while ((c = getopt (argc, argv, "qh?")) != EOF) {
	switch (c) {
            case 'q':
                Queue = 1;
                Print = 0;
                break;
	    case 'h':
	    case '?':
		Print_usage (argv);
		break;
	}
    }

    if (optind == argc - 1) {      /* get the comm manager index  */
	if (strcmp (argv[optind], "on") == 0)
	    Command = CMD_VV_ON;
	else if (strcmp (argv[optind], "off") == 0)
	    Command = CMD_VV_OFF;
	else
	    sscanf (argv[optind], "%d", &Line_index);
    }

    if (err == 0 && Line_index < 0 && Command < 0) {
	LE_send_msg (GL_ERROR | 75,  "Line_index not specified or incorrect\n");
	err = -1;
    }

    return (err);
}

/**************************************************************************

    Description: This function prints the usage info.

**************************************************************************/

static void Print_usage (char **argv)
{
    printf ("Usage: %s (options) line_index\n", argv[0]);
    printf ("       %s on/off - turns on/off the V&V mode\n", argv[0]);
    printf ("       Options:\n");
    printf ("         -q Print Write Queue Format \n");
    exit (0);
}
