/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2008/04/30 13:46:59 $
 * $Id: cpuuse_main.c,v 1.6 2008/04/30 13:46:59 steves Exp $
 * $Revision: 1.6 $
 * $State: Exp $
*/


#include <cpuuse.h>

/* Macro defintions. */
#define MAX_TASKS			1024

/* Static Global Variables. */
static Task_rec_t Task_list[MAX_TASKS];  /* at some point, this should be linked-list. */
static int Task_list_items = 0;

static Vol_hdr_t Vol_hdr;

/* Global Variables. */
extern Vol_stats_list_t *List;
extern int Vol_stats_list_num_vols;

/* Function prototypes. */
static int Write_menu();
static int Open_usage_file( char *input_file_path, FILE **fd );
static time_t Get_unix_time( void *buf, int type );
static int Get_elevation_cut( void *buf, int type );
static int Set_vol_header( void *buf, int type );
static void Init_vol_header( void *buf, int type );
static int Write_vol_header( Vol_hdr_t *vol_hdr );
static int Write_summary_header( int selection1, int selection2  );
static void* Read_file( FILE *fd, int *type );
static int Get_command_line_args( int Argc, char **Argv, char input_file_path[] );

/**************************************************************************
    Description:

    Input:
       argc - the number of command line arguments.
       argv - the command line arguments.

    Output:

    Returns:
       On failure of any sort, exits with exit code > 0.  Normal termination,
       exit code is 0.

    Notes:
**************************************************************************/
int main(int argc, char **argv){

    int curr_elev_cut = 0, first_time = 1, retval, type;
    char input_file_path[255];
    void *ret;
    FILE *fd = NULL;
    time_t curr_monitor_time = 0, prev_monitor_time = 0;

    /* Get command line arguments.  On failure, exit. */
    if( (retval = Get_command_line_args( argc, argv, input_file_path ) ) != 0)
       exit(retval);

    /* Open the usage file.  Exit on failure. */
    if( (retval = Open_usage_file( input_file_path, &fd )) != 0 )
       exit(retval);

    /* Initialize tables and structures used to compute cpu usage. */
    if( (retval = CPUUSE_initialize( )) < 0 )
       exit(retval);

    /* Read each line of the file ..... return type of record.
       Go until the start of volume record is encountered. */
    while(1){

       if( (ret = Read_file( fd, &type )) == (void *) NULL ){

          fprintf( stderr, "Unable to Find Volume Record In Usage File\n" );
          exit(1);

       }

       /* If not a volume record, keep reading. */
       if( (type & VOL_REC) == 0 )
          continue;

       Init_vol_header( ret, (type | INIT_REC) );

       if( (retval = Set_vol_header( ret, (type | INIT_REC) )) < 0 ){

          fprintf( stderr, "Save_volume_header Failed\n" );
          exit(1);

          curr_elev_cut = 1;
          first_time = 1;

       }

       break;

    }

    /* Read each line of the file ..... return type of record. */
    while(1){

       if( (type & ELEV_REC) != 0 ){
 
          curr_monitor_time = Get_unix_time( ret, type ); 

          if( Task_list_items != 0 ){

             int len = Task_list_items*sizeof(Task_rec_t);
             CPUUSE_compute_cpu_utilization( (char *) Task_list, prev_monitor_time, 
                                             curr_elev_cut, len );

             /* Compute the accumulated volume duration. */
             Set_vol_header( ret, type );

             /* Initialize the task record list. */
             memset( (char *) Task_list, 0, sizeof(Task_rec_t)*MAX_TASKS ); 
             Task_list_items = 0;

          }

          /* Set the previous monitor time to the current monitor time for
             next elevation/volume. */
          prev_monitor_time = curr_monitor_time;
          curr_elev_cut = Get_elevation_cut( ret, type );

       }
       else if( (type & VOL_REC) != 0 ){

          if( !first_time ){

             if( Task_list_items != 0 ){

                int len = Task_list_items*sizeof(Task_rec_t);

                Set_vol_header( ret, type );

                CPUUSE_compute_cpu_utilization( (char *) Task_list, prev_monitor_time, 
                                                curr_elev_cut, len );

                /* Initialize the task record list. */
                memset( (char *) Task_list, 0, sizeof(Task_rec_t)*MAX_TASKS ); 
                Task_list_items = 0;

             }

             /* Save the volume stats. */
             CPUUSE_save_vol_stats( &Vol_hdr );

             /* Reinitialize the volume CPU stats for upcoming volume scan. */
             CPUUSE_init_vol_stats();

             /* Initialize the volume header. */
             Init_vol_header( ret, type );

          }
          else
             first_time = 0;

          Set_vol_header( ret, type );

          /* At the start of volume, the elevation cut number is 1. */
          curr_elev_cut = 1;

       }
       else if( (type & TASK_REC) != 0 ){

          memcpy( (char *) &Task_list[Task_list_items], (char *) ret, sizeof(Task_rec_t) ); 
          Task_list_items++;

       }

       /* Read another file record. */
       if( (ret = Read_file( fd, &type )) == (void *) NULL )
          break;


    }

    /* Prompt the operator */
    Write_menu();

    return 0;

}

/**************************************************************************
    Description:
       Provides the user interface in the form of menus.

    Input:

    Output:

    Returns:
       Always returns 0.

    Notes:

**************************************************************************/
static int Write_menu(){

   Vol_stats_list_t *curr = NULL, **array = NULL;
   int selection1, selection2, i;

   fprintf( stdout, "There are %d Volume Scans to Report\n\n",
            Vol_stats_list_num_vols );

   array = (Vol_stats_list_t **) calloc( 1, sizeof(Vol_stats_list_t *)*Vol_stats_list_num_vols );
   if( array == NULL ){

      fprintf( stderr, "calloc Failed for %d Bytes\n",
               sizeof(Vol_stats_list_t *)*Vol_stats_list_num_vols );
      exit(1);

   }

   /* Build list of volume scans for easy access. */
   curr = List;
   for( i = 0; i < Vol_stats_list_num_vols; i++ ){

      array[i] = curr;
      curr = curr->next;

   }

   while(1){

      for( i = 1; i <= Vol_stats_list_num_vols; i++ ){

          curr = array[i-1];

          fprintf( stdout, "   %3d: Date/Time: %.2d/%.2d/%.2d %.2d:%.2d:%.2d  VCP: %3d\n",
                   i, curr->vol_hdr.vol.dt.month, curr->vol_hdr.vol.dt.day, curr->vol_hdr.vol.dt.year,
                   curr->vol_hdr.vol.dt.hour, curr->vol_hdr.vol.dt.minute, curr->vol_hdr.vol.dt.second,
                   curr->vol_hdr.vol.vcp_number ); 
      
      }

      fprintf( stdout, "\n\n Select A Volume to Display By Selecting Line #, or -1 to Exit\n" );
      fprintf( stdout, " NOTE:  To Select A Range of Volumes, Enter #,#\n" );
      selection1 = selection2 = -1;
      scanf( "%d,%d", &selection1, &selection2 );
      if( selection1 == -1 ){

         free(array);
         return(0);

      }
      
      /* Process the user's request. */
      if( ((selection2 > 0) && (selection1 <= selection2))
                            ||
          ((selection2 < 0) && (selection1 > 0)) ){

         int num_volumes = 0, start_vol;

         /* If only one volume scan selected, .... */
         if( selection2 < 0 )
            selection2 = selection1;

         /* Make sure selections are within bounds. */
         if( selection1 > Vol_stats_list_num_vols )
            selection1 = Vol_stats_list_num_vols;

         if( selection2 > Vol_stats_list_num_vols )
            selection2 = Vol_stats_list_num_vols;
     
         /* Compute the number of volumes in the user's request. */
         num_volumes = (selection2 - selection1) + 1;
         start_vol = selection1;

         /* Do for each volume ...... */
         while( start_vol <= selection2 ){

            curr = array[start_vol-1];

            /* Write the volume header from last volume scan. */
            Write_vol_header( &curr->vol_hdr );

            /* Write the CPU utilization. */
            CPUUSE_write_cpu_utilization( curr );

            start_vol++;

         }

         /* Provide average utilization if more than one volume. */
         if( num_volumes > 1 ){

            Vol_stats_list_t *avg = NULL;
            int ret;
    
            /* Sum and average the volume scan data. */
            ret = CPUUSE_sum_and_avg( array, &avg, selection1, selection2 );

            /* Write the CPU utilization. */
            if( ret >= 0 ){

               Write_summary_header( selection1, selection2 );
               CPUUSE_write_cpu_utilization( avg ); 

            }

            /* Free the memory associated with "avg" */
            if( avg != NULL ){

               free(avg);
               avg = NULL;

            }

         }

      }

   }
   
   return 0;

}

/**************************************************************************
    Description:
       Reads a record from the CPU usage file.  File assumed in rpg_ps format.

    Input:
       fd - file handle for CPU usage file.

    Output:
       type - type of record read.

    Returns:
       If record successfully read and recognized, returns pointer to record.
       Otherwise, returns NULL.

    Notes:

**************************************************************************/
static void*  Read_file( FILE *fd, int *type ){

    char *retptr = NULL;

    static Elev_rec_t elev;
    static Vol_rec_t vol;
    static Task_rec_t task;
    static char buf[255];

    retptr = fgets( buf, 255, fd );

    if( retptr != NULL ){

        if( strstr( buf, "Elevation" ) != NULL ){

            sscanf( buf, "Elevation: # %d  %d/%d/%d %d:%d:%d  vcp %d\n", 
	            &elev.cut_number, &elev.dt.month, &elev.dt.day, &elev.dt.year, &elev.dt.hour, 
                    &elev.dt.minute, &elev.dt.second, &elev.vcp_number);

            *type = ELEV_REC;
            return( (void *) &elev );

        }
        else if( strstr( buf, "Volume" ) != NULL ){

            sscanf( buf, "Volume: # %d  %d/%d/%d %d:%d:%d  vcp %d\n", 
	            &vol.vol_number, &vol.dt.month, &vol.dt.day, &vol.dt.year, &vol.dt.hour, &vol.dt.minute, 
                    &vol.dt.second, &vol.vcp_number );

            *type = VOL_REC;
            return( (void *) &vol );

        }
        else if( strstr( buf, "name" ) != NULL ){

           *type = HEADER_REC;
           return( (void *) buf );

        }
        else{

           sscanf( buf, "%s %d %d %d %d %d %s\n", 
		   task.task_name, &task.instance, 
		   &task.pid, &task.cpu, &task.mem, &task.life, &task.node_name[0] ); 

           *type = TASK_REC;
           return( (void *) &task );
        }

        return ( (void *) NULL );

    }
    else{

       *type = UNKNOWN_REC;
       return( (void *) NULL );

    }

}
   
/**************************************************************************
    Description:
       Gets the UNIX time given hr/min/sec mon/day/yr format.

    Input:
       buf - contains the data to convert
       type - the type of buffer (i.e., VOL_REC, ELEV_REC, etc)

    Output:

    Returns:
       The UNIX time (seconds since midnight 1/1/70)

    Notes:

**************************************************************************/
static time_t Get_unix_time( void *buf, int type ){

   Elev_rec_t *elev_rec = NULL;
   Vol_rec_t *vol_rec = NULL;
   time_t c_time = 0;
   int ret, year;

   if( (type & VOL_REC) != 0 ){

      vol_rec = (Vol_rec_t *) buf;

      if( vol_rec->dt.year >= 70 )
         year = vol_rec->dt.year + 1900;
      else
         year = vol_rec->dt.year + 2000;

      ret = unix_time( &c_time, &year, &vol_rec->dt.month, &vol_rec->dt.day,
                       &vol_rec->dt.hour, &vol_rec->dt.minute, &vol_rec->dt.second );
   }
   else if( (type & ELEV_REC) != 0 ){

      elev_rec = (Elev_rec_t *) buf;

      if( elev_rec->dt.year >= 70 )
         year = elev_rec->dt.year + 1900;

      else
         year = elev_rec->dt.year + 2000;

      ret = unix_time( &c_time, &year, &elev_rec->dt.month, &elev_rec->dt.day,
                       &elev_rec->dt.hour, &elev_rec->dt.minute, &elev_rec->dt.second );

   }

   return( c_time );

}

/**************************************************************************
    Description:
       Returns the elevation cut number of this record.

    Input:
       buf - the record.
       type - type of record (e.g., VOL_REC, ELEV_REC, etc.)

    Output:

    Returns:
       Elevation cut number if record type is ELEV_REC or -1.

    Notes:

**************************************************************************/
static int Get_elevation_cut( void *buf, int type ){

   Elev_rec_t *elev_rec = NULL;

   if( (type & ELEV_REC) != 0 ){

      elev_rec = (Elev_rec_t *) buf;
      return( elev_rec->cut_number );

   }

   return(-1);

}

/**************************************************************************
    Description:
       Sets field(s) in the Volume Header data structure.
 
    Input:
       buf - the input record containing the data.
       type - the type of record (e.g., ELEV_REC, VOL_REC, etc).

    Output:

    Returns:
       Returns 0 if record type is ELEV_REC or VOL_REC.  Otherwise
       returns -1.

    Notes:

**************************************************************************/
static int Set_vol_header( void *buf, int type ){

   time_t c_time;
   static time_t p_time = 0;

   if( (type & INIT_REC) != 0 )
      p_time = Get_unix_time( buf, type );

   if( ((type & ELEV_REC) != 0) || ((type & VOL_REC) != 0) ){

      c_time = Get_unix_time( buf, type );
      Vol_hdr.vcp_duration += (c_time - p_time);

      /* Reset p_time for next pass. */
      p_time = c_time;

      return(0);

   }
   
   return(-1);

}

/**************************************************************************
    Description:
       Initializes the Volume Header data structure.
 
    Input:
       buf - the input record.
       type - type of input record (e.g., ELEV_REC, VOL_REC, etc).

    Output:

    Returns:

    Notes:

**************************************************************************/
static void Init_vol_header( void *buf, int type ){

   Vol_rec_t *vol = NULL;

   if( (type & VOL_REC) != 0 ){

      vol = (Vol_rec_t *) buf;       
      memcpy( &Vol_hdr, vol, sizeof(Vol_rec_t) );
      Vol_hdr.vcp_duration = 0;

   }

}

/**************************************************************************
    Description:
       Writes the volume header information to stdout.

    Input:
       vol_hdr - the volume header.

    Output:

    Returns:
       Always returns 0.

    Notes:

**************************************************************************/
static int Write_vol_header( Vol_hdr_t *vol_hdr ){

   fprintf( stdout, "\n\n" );

   if( Node_name[0] != '\0' )
      fprintf( stdout, "%s %.2d/%.2d/%.2d %.2d:%.2d:%.2d  VCP: %3d VCP Duration: %3d (sec)\n", 
               Node_name, vol_hdr->vol.dt.month, vol_hdr->vol.dt.day, vol_hdr->vol.dt.year, 
               vol_hdr->vol.dt.hour, vol_hdr->vol.dt.minute, vol_hdr->vol.dt.second,
               vol_hdr->vol.vcp_number, vol_hdr->vcp_duration );

   else
      fprintf( stdout, "%.2d/%.2d/%.2d %.2d:%.2d:%.2d  VCP: %3d VCP Duration: %3d (sec)\n", 
               vol_hdr->vol.dt.month, vol_hdr->vol.dt.day, vol_hdr->vol.dt.year, 
               vol_hdr->vol.dt.hour, vol_hdr->vol.dt.minute, vol_hdr->vol.dt.second,
               vol_hdr->vol.vcp_number, vol_hdr->vcp_duration );

   fprintf( stdout, "\n" );

   return 0;
}

/**************************************************************************
    Description:
       Writes the summary header information to stdout.  This is called
       in conjunction with "sum and average".

    Input:
       selection1 - the starting volume number.
       selection2 - the ending volume number.

    Output:

    Returns:
       Always returns 0.

    Notes:

**************************************************************************/
static int Write_summary_header( int selection1, int selection2 ){

   fprintf(stdout, "\n\n" );
   fprintf(stdout, "CPU Summary For Volumes %4d - %4d\n",
                    selection1, selection2 ); 

   fprintf( stdout, "\n" );

   return 0;
}

/**************************************************************************
    Description:
       Opens the input file containing the CPU statistics.

    Input:
       input_file_path - fully qualified path of input file.

    Output:
    fd - file descriptor for opened input file.

    Returns:
      -1 on error or 0 on success.

    Notes:

**************************************************************************/
static int Open_usage_file( char *input_file_path, FILE **fd ){

    errno = 0;
    *fd = fopen(input_file_path, "r");
    if( *fd == NULL ){

        fprintf( stderr, "Unable to fopen usage file: %s (errno %d)\n",
                 input_file_path, errno);
        return(-1);

    }
    else 
        fprintf( stdout, "Opened usage file: %s\n", input_file_path );

    return(0);

}

/****************************************************************************

   Description:
      Process command line arguments.

   Inputs:
      Argc - number of command line arguments.
      Argv - the command line arguments.

   Outputs:
      input_file_path - 

   Returns:
      Exits with non-zero exit code on error, or returns 0 on success.

*****************************************************************************/
static int Get_command_line_args( int Argc, char **Argv, char input_file_path[] ){

   extern char *optarg;
   extern int optind;
   int c, err, ret;
   char tat_name[256], *cfg_dir = NULL;

   /* Initialize input_file_path to NULL. */
   input_file_path[0] = 0;

   /* Initialize the task_attr_table file path. */
   cfg_dir = getenv( "CFG_DIR" );
   if( cfg_dir != NULL ){

      memset( Ttcf_fname, 0, 256 );
      strcat( Ttcf_fname, cfg_dir );

      if( Ttcf_fname[ strlen(cfg_dir)-1 ] == '/' )
         strcat( Ttcf_fname, "task_attr_table" );
      else
         strcat( Ttcf_fname, "/task_attr_table" );
   
   }

   err = 0;
   while ((c = getopt (Argc, Argv, "hp:n:d:")) != EOF) {

      switch (c) {
  
         case 'p':
            if( strlen( optarg ) < 255 ){    

               ret = sscanf(optarg, "%s", input_file_path) ;
               if (ret == EOF) {

                   fprintf( stderr, "sscanf failed to read pathname\n" );
                   err = 1 ;

               }

            }
            else{

               fprintf( stderr, "Input File Path Too Long\n" );
               err = 1;

            }
            break;
 
         case 'n':
            if( strlen( optarg ) < 6 ){    

               ret = sscanf(optarg, "%s", Node_name ) ;
               if (ret == EOF) {

                   fprintf( stderr, "sscanf failed to read node name\n" );
                   err = 1 ;

               }

            }
            else{

               fprintf( stderr, "Node Name Too Long\n" );
               err = 1;

            }
            break;
 
         case 'd':
            if( strlen( optarg ) < 256 ){    

               ret = sscanf(optarg, "%s", tat_name ) ;
               if (ret == EOF) {

                   fprintf( stderr, "sscanf failed to read TAT configuration file name\n" );
                   err = 1 ;

               }

               memcpy( Ttcf_fname, tat_name, strlen(tat_name) + 1 );

            }
            else{

               fprintf( stderr, "TAT configuration file name Too Long\n" );
               err = 1;

            }
            break;
 
         case 'h':
         case '?':
         default:
            err = 1;
            break;
      }

   }

   if( err == 1 ){ 

      printf ("Usage: %s [options]\n", Argv [0]);
      printf ("\toptions:\n");
      printf ("\t\t-p Input File Path Name (required)\n" );
      printf ("\t\t-n Node Name (optional)\n" );
      printf ("\t\t-d TAT Configuration File Name (optional)\n" );
      exit (1);

   }
  
   return (0);

}

