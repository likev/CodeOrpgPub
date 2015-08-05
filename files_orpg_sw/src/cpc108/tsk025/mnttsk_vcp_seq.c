/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/12/09 22:29:17 $
 * $Id: mnttsk_vcp_seq.c,v 1.2 2014/12/09 22:29:17 steves Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

#include <mnttsk_vcp_seq.h>
#include <ctype.h>

#define CFG_NAME_SIZE		128
#define MAX_NUM_SEQ		10

/* Static Global Variables */
static Vcp_seq_t Vcp_seq[MAX_NUM_SEQ];

/* Static Function Prototypes */
static int    Init_VCP_Seq_tbl();
static int    Read_VCP_Seq_attrs();
static int    Read_VCP_Seq( int num_seqs );
static char*  Set_seq_key( int seq_num );
static int    Get_command_line_options( int argc, char *argv[], int *startup_type );
static int    Get_config_file_name( char *tmpbuf );

/*\////////////////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Adds/removes VCP Sequence data from RPG adaptation data.   The VCP Sequences
//      are assumed defined in configuration file located in $CFGDIR/vcp_sequence
//
//   Inputs:
//      argc - number of command line arguments
//      argv - the command line arguments
//
//   Notes:
//      Normal termination requires exit(0).  Abnormal termination requires non-zero
//      exit code.
//
////////////////////////////////////////////////////////////////////////////////////\*/
int main( int argc, char *argv[] ){

   int retval;
   int startup_type;

   /* Initialize log-error services. */
   (void) ORPGMISC_init(argc, argv, 5000, 0, -1, 0) ;

   /* Get the command line options. */
   if( (retval = Get_command_line_options( argc, argv, &startup_type )) < 0 )
      exit(1) ;

   /* Process startup command line option. */
   if( MNTTSK_VCP_Seq_table( startup_type ) < 0 )
      exit(1);

   /* Normal termination. */
   exit(0);

} /* End of main() */


/*\///////////////////////////////////////////////////////////////////
//
//   Description:
//      Read VCP Sequence configuration file and install the 
//      VCP Sequence data.
//
//   Inputs:
//      startup_action - start up type.
//
//   Outputs:
//
//   Returns:
//      Negative value on error, or 0 on success.
//      
//   Notes:
//      vcp sequence configuration file name is assumed 
//      "vcp_sequence_table".
//
//////////////////////////////////////////////////////////////////\*/
int MNTTSK_VCP_Seq_table( int startup_action ){

   char table_name[ CFG_NAME_SIZE ];
   int err = 0, ret, num_seqs = 0;
   double  dtemp;

   /* Set the error return value. */
   err = 0;

   /* Clear start. */ 
   if( startup_action == CLEAR ){

      /* Initialize the VCP Sequence table. */
      Init_VCP_Seq_tbl();

   }
   else if( (startup_action == STARTUP) || (startup_action == RESTART) ){

      /* Get the configuration file name. */
      Get_config_file_name( table_name ); 
      CS_cfg_name ( table_name );
      LE_send_msg( GL_INFO, "VCP Sequence Table Name: %s\n", table_name );

      /* Set up CS control. */
      CS_control (CS_COMMENT | '#');

      /* Read and process the VCP_Seq. */
      if( (num_seqs = Read_VCP_Seq_attrs()) <= 0 ){

         /* Initialize the VCP Sequence table on error or if none are 
            defined. */
         Init_VCP_Seq_tbl();
         return num_seqs;

      }
        
      /* Read and process the VCP Sequences. */
      if( (num_seqs = Read_VCP_Seq( num_seqs )) <= 0 ){

         /* Initialize the VCP Sequence table on error or if none are 
            defined. */
         Init_VCP_Seq_tbl();
         return num_seqs;
      }

      /* Set adaptation data values. */
      dtemp = -1; /* No active sequence. */
      LE_send_msg( GL_INFO, "Setting Active Sequence to %d\n", (int) dtemp );
      if( (ret = DEAU_set_values( "VCP_sequence.active_sequence", 0, 
                                  (void *) &dtemp, 1, 0 )) < 0 ){

         LE_send_msg( GL_ERROR, "Failed to Set Active Sequence. Error Code: %d\n", ret );
         err = -1;

      }

      dtemp = num_seqs; /* Number of sequences. */
      LE_send_msg( GL_INFO, "Setting Number of Sequences to %d\n", (int) dtemp );
      if( (ret = DEAU_set_values( "VCP_sequence.number_sequences", 0, 
                                  (void *) &dtemp, 1, 0 )) < 0 ){

         LE_send_msg( GL_ERROR, "Failed to Set Number of Sequences. Error Code: %d\n",ret );
         err = -1;

      }

      /* VCP Sequence data. */
      if( (ret = DEAU_set_binary_value( "VCP_sequence.sequences", (void *) &Vcp_seq, 
                                        num_seqs*sizeof(Vcp_seq_t), 0 )) < 0 ){

         LE_send_msg( GL_ERROR, "Failed to Set VCP Sequence Data. Error Code: %d\n", ret );
         err = -1;

      }

   }

   return( err ) ;

/*END of MNTTSK_VCP_Seq_tables()*/
}

/*\////////////////////////////////////////////////////////////////////////
//
//   Description: 
//      This function initializes the VCP Sequence table structure.  
//
//   Return:
//      -1 on error, 0 otherwise.
//
////////////////////////////////////////////////////////////////////////\*/
static int Init_VCP_Seq_tbl( ){

   double dtemp;
   Vcp_seq_t vcp_seq;

   /* Set adaptation data values. */

   /* VCP_sequence.active_sequence = 0 */
   dtemp = -1;
   DEAU_set_values( "VCP_sequence.active_sequence", 0, &dtemp, 1, 0 );

   /* VCP_sequence.number_sequences = 0 */
   dtemp = 0;
   DEAU_set_values( "VCP_sequence.number_sequences", 0, &dtemp, 1, 0 );

   /* VCP_sequences.sequences = {0} */
   memset( (void *) &vcp_seq, 0, sizeof( Vcp_seq_t ) );

   DEAU_set_binary_value( "VCP_sequences.sequences", &vcp_seq, 
                          sizeof(Vcp_seq_t), 0 );

   return 0;

}

/*\////////////////////////////////////////////////////////////////////////
//
//   Description: 
//      This function returns the number of VCP Sequences defined.
//
//   Return:
//      -1 on error, number of sequences otherwise.
//
////////////////////////////////////////////////////////////////////////\*/
static int Read_VCP_Seq_attrs(){

   int num_seqs = 0;

   /* Verify the VCP attributes section exist. */
   if( (CS_entry (VCP_SEQ_CS_ATTR_KEY, 0, 0, NULL) < 0 ) 
                               || 
       (CS_level( CS_DOWN_LEVEL ) < 0 )){

      LE_send_msg( GL_ERROR, "Could Not Find %s Key\n", VCP_SEQ_CS_ATTR_KEY );
      return (-1);

   }

   /* Parse the "num_seqs" keys. */
   if( (CS_entry( VCP_SEQ_CS_NUM_SEQ_KEY, VCP_SEQ_CS_NUM_SEQ_TOK, 0, 
                  (void *) &num_seqs ) <= 0) ){

      LE_send_msg( GL_ERROR, "Error parsing num_seqs: %d\n", num_seqs );
      return( -1 );

   }

   /* Return the number of sequences. */
   CS_level (CS_UP_LEVEL);
   return num_seqs;
    
/* End of Read_VCP_Seq_attrs() */
}

/*\/////////////////////////////////////////////////////////////////////
//
//   Description:
//      Parser for the elevation cut data.
//
//   Input:
//      num_seqs - number of VCP Sequences to parse.
//
//   Returns:
//      -1 on error, 0 on success. 
//
/////////////////////////////////////////////////////////////////////\*/
static int Read_VCP_Seq( int num_seqs ){

   int ret, num_in_seq = 0, err = 0, seq, seq_cnt = 0, cnt = 0;
   short vcp_num = 0;
   char *seq_key = NULL;


   /* Reset the error flag. */
   err = 0;

   /* Do For All VCP Sequences ..... */
   for( seq = 0; seq < num_seqs; seq++ ){

      seq_key = Set_seq_key( seq );
      if( (ret = CS_entry( seq_key, 0, 0, NULL )) < 0 ){

         LE_send_msg( GL_ERROR, "Could Not Find VCP Sequence Key %s (%d)\n", 
                      seq_key, ret );
         return -1;

      }

      CS_level( CS_DOWN_LEVEL );
      CS_control( CS_KEY_REQUIRED );

      /* Parse the elevation angle and waveform type fields. */
      num_in_seq = 0;
      if( CS_entry( VCP_SEQ_CS_NUM_VCPS_KEY, VCP_SEQ_CS_NUM_VCPS_TOK, 
                     0, (void *) &num_in_seq ) <= 0 ){ 

         LE_send_msg( GL_ERROR, "Required # VCPs in Seq Field %s Not Found For Seq %s\n",
                      VCP_SEQ_CS_NUM_VCPS_KEY, seq_key );
         return -1;

      }

      /* Parse all the VCPs. */
      if( num_in_seq > 0) {  

         /* Initialize VCP Sequence information for this sequence. */
         Vcp_seq[seq_cnt].active = 0;
         Vcp_seq[seq_cnt].num_in_seq = num_in_seq;
         memset( &Vcp_seq[seq_cnt].vcps, 0, MAX_NUM_SEQ*sizeof(int) );

         cnt = 0;
         while( (cnt < num_in_seq) 
                      &&
                (CS_entry( VCP_SEQ_CS_VCPS_KEY, ((cnt + 1) | CS_SHORT), 0,
                           (char *) &vcp_num ) > 0)){

             Vcp_seq[seq_cnt].vcps[cnt] = vcp_num;
             cnt++;

         }

         /* Check that we have read all VCPs numbers in the sequence and 
            number is correct. */
         if( cnt != num_in_seq ){

            err = 1;
            LE_send_msg( GL_ERROR, "Bad VCP Sequence List\n" );

         }

         /* Also check that the VCPs are defined. */
         for( cnt = 0; cnt < num_in_seq; cnt++ ){
         
            if( ORPGVCP_index( Vcp_seq[seq_cnt].vcps[cnt] ) < 0 ){

               LE_send_msg( GL_ERROR, "VCP %d in Sequence %d Undefined\n",
                            Vcp_seq[seq_cnt].vcps[cnt], seq_cnt );
               err = 1;
               break;

            }

         }

         /* If no error occurred, increment the number of sequences. */
         if( err == 0 )
            seq_cnt++;

      }

      CS_level( CS_UP_LEVEL );

      /* Write out information about this elevation cut. */
      if( err == 0 ){

         LE_send_msg( GL_INFO, "---> VCP Sequence %d\n", seq );
         LE_send_msg( GL_INFO, "------>Number of VCPs in Sequence: %d\n", num_in_seq );
         for( cnt = 0; cnt < num_in_seq; cnt++ )
            LE_send_msg( GL_INFO, "--------->VCP: %d\n", Vcp_seq[seq_cnt-1].vcps[cnt] );

      } 

      /* Reset err in preparation for next sequence. */
      err = 0;

   } /* End of "For All Sequences" loop. */

   /* On error, return error code. */
   if( err )
      return -1;

   /* Normal returns. */
   return seq_cnt;

} /* End of Read_VCP_Seq() */


/*\///////////////////////////////////////////////////////////////////
//
//   Description:
//      Convenience function for setting the key used to parse the 
//      VCP sequence definitions.
//
//   Input:
//      seq_cut - VCP sequence number (unit indexed).
/
//   Returns:
//      Pointer to string holding the key.
//
///////////////////////////////////////////////////////////////////\*/
static char* Set_seq_key( int seq_num ){

   static char seq_key[10];

   seq_key[0] = 0;
   sprintf( seq_key, "Seq_%0d", seq_num );

   return( seq_key );

} /* End of Set_seq_key() */


/*\//////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Process command line arguments.
//
//   Inputs:
//      argc - number of command line arguments.
//      argv - the command line arguments.
//
//   Outputs:
//      startup_action - start up action (STARTUP or RESTART)
//
//   Returns:
//      exits on error, or returns 0 on success.
//
///////////////////////////////////////////////////////////////////////////\*/
static int Get_command_line_options( int argc, char *argv[], int *startup_action ){

   extern char *optarg;
   extern int optind;
   int c, err, ret;
   char start_up[255];
   
   /* Initialize startup_action to RESTART and vcp_num to 0. */
   *startup_action = RESTART;

   err = 0;
   while ((c = getopt (argc, argv, "ht:")) != EOF) {

      switch (c) {

         case 't':
            if( strlen( optarg ) < 255 ){

               ret = sscanf(optarg, "%s", start_up) ;
               if (ret == EOF) {

                  LE_send_msg( GL_INFO, "sscanf Failed To Read Startup Action\n" ) ;
                  err = 1 ;

               }
               else{

                  if( strstr( start_up, "startup" ) != NULL )
                     *startup_action = STARTUP;

                  else if( strstr( start_up, "restart" ) != NULL )
                     *startup_action = RESTART;

                  else if( strstr( start_up, "clear" ) != NULL )
                     *startup_action = CLEAR;

                  else
                     *startup_action = RESTART;

               }

            }
            else
               err = 1;

            break;

         case 'h':
         case '?':
         default:
            err = 1;
            break;
      }

   }

   if (err == 1) {              /* Print usage message */
      printf ("Usage: %s [options]\n", MISC_string_basename(argv [0]));
      printf ("\toptions:\n");
      printf ("\t\t-h Help (print usage msg and exit)\n");
      printf ("\t\t-t Startup Mode/Action (optional - default: restart)\n" );
      exit (1);
   }

   return (0);

/* End of Get_command_line_options() */
}


/*\///////////////////////////////////////////////////////////////////////////////
//
//  Description:
//      Gets the CS configuration file name.  The file is assume to be located
//      in the configuration directory, in subdirectory vcp_sequence with
//      filename vcp_sequence_table.
//
//   Inputs:
//      tmpbuf - holds the CS configuration file name.
//
//   Returns:
//      Negative number on error, 0 on success.
//
//////////////////////////////////////////////////////////////////////////////\*/
static int Get_config_file_name( char *tmpbuf ){

   char cfg_dir[CFG_NAME_SIZE];
   int err = 0;                /* error flag */
   int len;

   memset( tmpbuf, 0, sizeof(tmpbuf) );

   /* Get the configuration source directory. */
   len = MISC_get_cfg_dir (cfg_dir, CFG_NAME_SIZE);
   if (len > 0)
      strcat (cfg_dir, "/");

   /* Append the filename to the CFG_DIR.  If CFG_DIR not defined,
      return error. */
   if( len <= 0 ){

      err = -1;
      LE_send_msg (GL_INFO, "CFG Directory Undefined\n");

    }
    else{

       /* Construct the VCP Sequence Definition Table name. */
       strcpy (tmpbuf, cfg_dir);
       strcat (tmpbuf, "vcp_sequence/vcp_sequence_table");

    }

    return (err);

} /* End of Get_config_file_name() */

