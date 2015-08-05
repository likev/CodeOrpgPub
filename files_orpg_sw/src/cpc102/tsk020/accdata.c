/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/05/31 19:29:13 $
 * $Id: accdata.c,v 1.14 2013/05/31 19:29:13 steves Exp $
 * $Revision: 1.14 $
 * $State: Exp $
 */
#include <basedata.h>
#include <orpg.h>
#include <unistd.h>
#include <stdio.h>
#include <a309.h>

/* 
  Static Global Variables. 
*/
static int Continuous_mode = 0;
static int Abbreviated_listing = 0;
static int Verbose = 0;
static Radial_accounting_data Accdata;
static Orpgevt_scan_info_t Vol_info[2];
static int End_volume = 0;
static int New_volume = 0;


/* 
  Function prototypes.
*/
static void Write_volume_accdata( int start_volume, 
                           int end_volume );
static void Write_elev_accdata( int volume ); 
static void Convert_time( long timevalue, char *hr, char *min, char *sec );
static void Calendar_date( short date, char *day, char *month, char *year );
static void To_ASCII( int value, char *string, int type ); 
static void Interactive_volume_accounting();
static void Interactive_elevation_accounting();
static void Continuous_monitoring();
static int Read_options( int argc, char **argv );
static void An_callback( EN_id_t evtcd, char *msg, int msglen, void *arg );
char* Process_event_msg( int where, EN_id_t event, char *msg, int msg_len );

/**************************************************************************

   Main module for the acc1 tool.

**************************************************************************/
int main( int argc, char *argv[] ){

   int response;

   /* 
     Check command line options.  If Continuous Mode selected,
     output Volume Accounting every end of volume. 
   */
   Read_options( argc, argv );
   
   /* 
     Calling this registration function to prevent the silly error 
     message from ORPGDA library.  
   */
   ORPGMISC_init( argc, argv, 100, 0, -1, 0 );

   /* 
     Did the operator want continuous mode? 
   */
   if( Continuous_mode ){

      fprintf( stderr, "\nContinuous Monitoring ---- Volume Accounting\n" );
      if( Abbreviated_listing )
         fprintf( stderr, "--->Outputting Abbreviated Listing\n" );
      fprintf( stderr, "\n" );

      Continuous_monitoring();
      exit(0);

   }

   /* 
     This is interactive mode. 
   */
   while(1){

      /*
        Prompt the operator for a selection. 
      */
      fprintf( stderr, "\nSelect one of the following options:\n");
      fprintf( stderr, "0 - Exit.\n");
      fprintf( stderr, "1 - Volume Accounting.\n");
      fprintf( stderr, "2 - Elevation Accounting. \n");

      /* 
        Read the operators selection. 
      */
      scanf( "%d", &response ); 
  
      /* 
        Process user request. 
      */
      switch (response){

         case 0:
            exit(0);

         case 1:
            Interactive_volume_accounting();
            break;

         case 2:
            Interactive_elevation_accounting();
            break;

         default: {

            fprintf( stderr, "Invalid Option.. Try Again.\n" );
            break;
         
         }

      } /* End of switch(response) */
  
   } /* End of while(1) */

/* End of main() */
}

/***********************************************************************

   Description:
      Handles continuous volume monitoring.   At end of volume, the
      volume accounting data is written to screen.

***********************************************************************/
static void Continuous_monitoring(){

   int ret, current_volume;
   int first_time = 1;

   /* Register for SCAN_INFO Event. */
   ret = EN_register( ORPGEVT_SCAN_INFO, An_callback );
   if( ret < 0 ){

      fprintf( stderr, "EN_register scan info event failed (ret %d)\n", ret);
      exit (10);

   }

   /* Register for RADIAL ACCOUNTING Event. */
   ret = EN_register( ORPGEVT_RADIAL_ACCT, An_callback );
   if( ret < 0 ){

      fprintf( stderr, "EN_register radial accounting event failed (ret %d)\n", ret);
      exit (11);

   }

#ifdef LITTLE_ENDIAN_MACHINE
    /* Set byte swapping function. */
    EN_control (EN_SET_PROCESS_MSG_FUNC, Process_event_msg);
#endif

   /* Do Forever .... */
   while(1){

      /* Check if End_volume flag is set. */
      if( End_volume ){

         End_volume = 0;
         current_volume = Vol_info[1].data.vol_scan_number;
         Write_volume_accdata( current_volume, current_volume );

      }
      else if( New_volume ){

         New_volume = 0;
         if( (ORPGVST_get_previous_status() == ORPGVST_ABORTED)
                                 &&
                            (!first_time) ){

            current_volume = Vol_info[0].data.vol_scan_number - 1;
            if( current_volume < 1 ) 
               current_volume = 80;
            Write_volume_accdata( current_volume, current_volume );
      
         }

         /* Clear the first_time flag. */
         first_time = 0;

      }

      /* Add in sleep.  Sleep will be interrupted when an event
         is received. */
      sleep(5);

   }

/* End of Continuous_monitoring(). */
}

/***********************************************************************

   Description:
      Module to handle interactive volume accounting. 

************************************************************************/
static void Interactive_volume_accounting(){

   int response;
   int start_volume, end_volume;

   while(1){

      /*
        Prompt the operator for a selection. 
      */
      fprintf( stderr, "\nSelect one of the following options:\n");
      fprintf( stderr, "0 - Exit.\n");
      fprintf( stderr, "1 - One volume scan.\n");
      fprintf( stderr, "2 - Range of volume scans. \n");
      fprintf( stderr, "3 - All volume scans.\n");

      /* 
        Read the operators selection. 
      */
      scanf( "%d", &response ); 
  
      /* 
        Process user request. 
      */
      switch (response){

         case 0:
            return;

         case 1: {

            fprintf( stderr, 
               "Enter the volume scan sequence number:" );
            scanf( "%d", &start_volume );
            start_volume = ORPGMISC_vol_scan_num( (unsigned int) start_volume );
            end_volume = start_volume;
            Write_volume_accdata( start_volume, end_volume );
            break;

         }

         case 2: {

            fprintf( stderr, 
               "Enter starting volume scan sequence number: " );
            scanf( "%d", &start_volume );
            fprintf( stderr, 
               "Enter the ending volume scan sequence number: " );
            scanf( "%d", &end_volume );
            start_volume = ORPGMISC_vol_scan_num( (unsigned int) start_volume );
            end_volume = ORPGMISC_vol_scan_num( (unsigned int) end_volume );
            Write_volume_accdata( start_volume, end_volume );
            break;

         }

         case 3: {

            start_volume = 1; 
            end_volume = 80; 
            Write_volume_accdata( start_volume, end_volume );
            break;
      
         }

         default: {

            fprintf( stderr, "Invalid Option.. Try Again.\n" );
            break;
         
         }

      } /* End of switch(response) */

   } /* End of while(1) */

/* End of Interactive_volume_accounting(). */
}

/*******************************************************************************

   Description:
      Module to handle interactive elevation accounting. 

*******************************************************************************/
static void Interactive_elevation_accounting(){

   int response;
   int volume;

   while(1){

      /*
        Prompt the operator for a selection. 
      */
      fprintf( stderr, "\nSelect one of the following options:\n");
      fprintf( stderr, "0 - Exit.\n");
      fprintf( stderr, "1 - Select a volume scan to display.\n");

      /* 
        Read the operators selection. 
      */
      scanf( "%d", &response ); 
  
      /* Process user request. */
      switch (response){

         case 0:
            return;

         case 1: {

            fprintf( stderr, "Enter volume scan sequence number:" );
            scanf( "%d", &volume );
            volume = ORPGMISC_vol_scan_num( (unsigned int) volume );
            Write_elev_accdata( volume );
            break;

         }

         default: {

            fprintf( stderr, "Invalid Option.. Try Again.\n" );
            break;
         
         }

      } /* End of switch(response) */

   } /* End of while(1) */

/* End of Interactive_elevation_accounting(). */
}

/**************************************************************************

   Description:
      Writes volume accounting data from start_volume to end_volume. 

   Inputs:
      start_volume - start volume scan number. 
      end_volume - end volume scan number. 

**************************************************************************/
static void Write_volume_accdata( int start_volume, int end_volume ){

   static char start_hours[3], start_minutes[3];
   static char end_hours[3], end_minutes[3]; 
   static char start_seconds[7], end_seconds[7];
   static char day[3], month[3], year[3];
   static int prev_start_time;

   static int header_once = 0;

   int vol, elv, elev_num, duration = 0;
   int response, full_listing, bytes_read; 
   short date;
   char weather_mode;
   float dopres, overlay_margin, elev_angle;

   long vol_start_time = 0, vol_end_time = 0;

   full_listing = 1;
   if( Continuous_mode && Abbreviated_listing )
      full_listing = 0;

   /* Only in interactive mode. */
   if( !Continuous_mode ){

      while(1){

         /*
           Prompt the operator for a selection. 
         */
         fprintf( stderr, "\nSelect one of the following options:\n");
         fprintf( stderr, "0 - Exit.\n");
         fprintf( stderr, "1 - Full listing.\n");
         fprintf( stderr, "2 - Abbreviated (Spreadsheet format)\n");

         /* 
           Read the operators selection. 
         */
         scanf( "%d", &response );

         /* Process user request. */
         switch (response){

            case 0:
               return;

            case 1: {

               full_listing = 1;
               break;

            }

            case 2: {

               full_listing = 0;
               break;

            }

         } /* End of switch(response) */

         break;

      } /* End of while(1) */

   }

   /* Output header for abbreviated version. */ 
   if( (full_listing == 0) && (header_once == 0) ){

      fprintf( stderr, "\n" );
      fprintf( stderr, "  Date       Start          End       VCP  Duration  #Elevs  Last Elev  Volume\n" );
      fprintf( stderr,
                       "_________________________________________________________________________________\n" );

      /* If continuous mode and abbreviated list, do not write out the 
         header again. */
      if( Continuous_mode && Abbreviated_listing )
         header_once = 1;

   }
                                           
   /* 
     Do for each of volume scan. 
   */
   for( vol = start_volume-1; vol <= end_volume-1; vol++ ){

      /* 
        Read in the radial accounting data. 
      */
      bytes_read = ORPGDA_read( ORPGDAT_ACCDATA, 
                                (char *) &Accdata, 
                                sizeof( Radial_accounting_data ), 
                                vol );

      /*
        Skip to next volume scan if data is available for this volume scan. 
      */
      if( bytes_read <= 0 ){
 
         if( bytes_read != LB_NOT_FOUND ){

            fprintf( stderr, "ERROR Reading ORPGDAT_ACCDATA.  Error = %d", 
                     bytes_read);
            return;

         }

         continue;
 
      }

      /* 
        Set weather mode.
      */
      if( Accdata.accwxmode == PRECIPITATION_MODE )
         weather_mode = 'A';

      else if (Accdata.accwxmode == CLEAR_AIR_MODE )
         weather_mode = 'B';

      else
         weather_mode = 'T';

      /* 
        Set the Doppler data resolution.
      */
      if( Accdata.accdopres == 1 )
         dopres = 0.5;

      else if( Accdata.accdopres == 2 )
         dopres = 1.0;

      else{

         fprintf( stderr, "Unknown Doppler Resolution %d\n", Accdata.accdopres );
         dopres = 0.0;

      }

      /*
        Set overlay margin.
      */
      overlay_margin = (float) Accdata.accthrparm/10.0;

      if( full_listing ){

         /*
           Write volume constants. 
         */
         fprintf( stderr, "\n \n" );
         fprintf( stderr, " SEQUENCE NUMBER: %2d  VCP: %3d  WEATHER MODE: %c\n",
                  vol+1, Accdata.accvcpnum, weather_mode );
         fprintf( stderr, " DOPPLER RESOLUTION: %3.1f m/s   OVERLAY MARGIN: %4.1f dB\n",
                  dopres, overlay_margin );
         fprintf( stderr, " CALIBRATION CONSTANT: %5.2f dB\n", Accdata.acccalib );

         /* 
           Write header. 
         */
         fprintf( stderr, "\n" );
         fprintf( stderr,
            "                   START         END        START    END                    \n");
         fprintf( stderr,
            "EL#    DATE                TIME                AZIMUTH     PSEUDO ELANG RADS\n");
         fprintf( stderr,
            "____________________________________________________________________________\n");

      }

      /* 
        Set previous elevation start time as start time for first elevation cut.
      */
      prev_start_time = Accdata.acceltms[0];

      if( full_listing ){

         /* 
           Do for each of elevation cut. 
         */
         for( elv = 0; elv < Accdata.accnumel; elv++ ){

            /* 
              Set elevation number. 
            */
            elev_num = elv + 1;

            /* 
              Convert the time in milliseconds to HH:MM:SS.sss format.
            */
            Convert_time( Accdata.acceltms[elv], 
                          start_hours, start_minutes, start_seconds );
            Convert_time( Accdata.acceltme[elv], 
                          end_hours, end_minutes, end_seconds );

            if( elv == 0 )
               vol_start_time = Accdata.acceltms[elv];

            if( elv == Accdata.accnumel - 1 );
               vol_end_time = Accdata.acceltme[elv];

            /*
              Convert the date to mm/dd/yy format.
            */
            date = Accdata.acceldts[elv];
            Calendar_date( date, day, month, year );

           /* 
             Write out accounting data. 
           */
           fprintf( stderr,
             " %2d  %s/%s/%s   %s:%s:%s %s:%s:%s  %6.2f  %6.2f  %6.2f  %4.1f  %3d\n",
             elev_num, 
             month,
             day,
             year,
             start_hours, 
             start_minutes, 
             start_seconds, 
             end_hours, 
             end_minutes, 
             end_seconds, 
             Accdata.accbegaz[elv],
             Accdata.accendaz[elv],
             Accdata.accpendaz[elv], 
             Accdata.accbegel[elv], 
             Accdata.accnumrd[elv] );

         }

         duration = (int) (vol_end_time - vol_start_time)/1000;
         if( duration < 0 )
            duration += 86400;
         fprintf( stderr, "\nVOLUME DURATION: %3d\n", duration );

         /* 
           Put in some blank lines. 
         */   
         fprintf( stderr, "\n \n" );

      }
      else{

         /* Abbreviated listing has month/day/year hr:min:sec hr:min:sec VCP Duration #Elvs */
         Convert_time( Accdata.acceltms[0],
                       start_hours, start_minutes, start_seconds );
         Convert_time( Accdata.acceltme[Accdata.accnumel-1],
                       end_hours, end_minutes, end_seconds );

         vol_start_time = Accdata.acceltms[0];
         vol_end_time = Accdata.acceltme[Accdata.accnumel-1];
         duration = (int) (vol_end_time - vol_start_time)/1000;
         if( duration < 0 )
            duration += 86400;

         elev_angle = Accdata.accbegel[Accdata.accnumel-1];

         date = Accdata.acceldts[0];
         Calendar_date( date, day, month, year );

         fprintf( stderr, "%s/%s/%s  %s:%s:%s  %s:%s:%s  %3d     %3d      %2d     %5.1f       %2d\n",
                  month, 
                  day, 
                  year, 
                  start_hours, 
                  start_minutes, 
                  start_seconds,  
                  end_hours,
                  end_minutes,
                  end_seconds, 
                  Accdata.accvcpnum, 
                  duration,
                  Accdata.accnumel,
                  elev_angle,
                  vol+1 );

      }

   }

   /* Put in some blank lines if not in Continuous_mode 
      and not in Abbreviated_listing. */
   if( !Abbreviated_listing )
      fprintf( stderr, "\n \n" );

/* End of Write_volume_accdata(). */
}   


/************************************************************************

   Description:
      Writes volume accounting data from start_volume to end_volume. 

   Inputs:
      volume - volume scan number. 
      
************************************************************************/
static void Write_elev_accdata( int volume ){

   int elv, elev_num, num_sectors;
   int bytes_read; 
   char weather_mode;
   float dopres, overlay_margin;

   /* 
     Read in the radial accounting data for specified volume scan. 
   */
   bytes_read = ORPGDA_read( ORPGDAT_ACCDATA, 
                             (char *) &Accdata, 
                             sizeof( Radial_accounting_data ), 
                             volume-1 );

   /*
     Return if no data is available for this volume scan. 
   */
   if( bytes_read <= 0 ){

      fprintf( stderr, "ERROR Reading ORPGDAT_ACCDATA.  Error = %d", bytes_read);
      return;
 
   }

   /* 
     Set weather mode.
   */
   if( Accdata.accwxmode == PRECIPITATION_MODE )
      weather_mode = 'A';

   else
      weather_mode = 'B';

   /* 
     Set the Doppler data resolution.
   */
   if( Accdata.accdopres == 1 )
      dopres = 0.5;

   else if( Accdata.accdopres == 2 )
      dopres = 1.0;

   else{

      fprintf( stderr, "Unknown Doppler Resolution %d\n", Accdata.accdopres );
      dopres = 0.0;

   }

   /*
     Set overlay margin.
   */
   overlay_margin = (float) Accdata.accthrparm/10.0;

   /*
     Write volume constants. 
   */
   fprintf( stderr, "\n \n" );
   fprintf( stderr, " SEQUENCE NUMBER: %2d  VCP: %3d  WEATHER MODE: %c\n",
            volume, Accdata.accvcpnum, weather_mode );
   fprintf( stderr, " DOPPLER RESOLUTION: %3.1f m/s   OVERLAY MARGIN: %4.1f dB\n",
            dopres, overlay_margin );
   fprintf( stderr, " CALIBRATION CONSTANT: %5.2f dB\n", Accdata.acccalib );

   /*
     Write header. 
   */
   fprintf( stderr, "\n" );
   fprintf( stderr,
      " EL#   SAZM NYVEL UNRG   SAZM NYVEL UNRG   SAZM NYVEL UNRG   ATMOS\n"); 
   fprintf( stderr,
      "             m/s   km          m/s   km          m/s   km    dB/km\n");            
   fprintf( stderr,
      "__________________________________________________________________\n");

   /* 
     Do for each of elevation cut. 
   */
   for( elv = 0; elv < Accdata.accnumel; elv++ ){

      /* 
        Set elevation number. 
      */
      elev_num = elv + 1;

      /* 
        Determine how many sectors are defined. 
      */ 
      for( num_sectors = 0; num_sectors < MAX_SECTS; num_sectors++ )
         if( Accdata.accsecsaz[elv][num_sectors] == -1.0 )
            break;

      if( num_sectors > MAX_SECTS )
         num_sectors = MAX_SECTS;

      /* 
        Write out accounting data. 
      */

      if( num_sectors == 1 )
         fprintf( stderr,
            " %2d     0.0 %5.2f  %3d                                      %5.4f\n",
            elev_num, 
            Accdata.accunvel[elv][0]/100.0,
            Accdata.accunrng[elv][0]/10, 
            Accdata.accatmos[elv]/1000.0 );

      else if( num_sectors == 2 )
         fprintf( stderr,
            " %2d   %5.1f %5.2f  %3d  %5.1f %5.2f %3d                     %5.4f\n",
            elev_num, 
            Accdata.accsecsaz[elv][0],
            Accdata.accunvel[elv][0]/100.0,
            Accdata.accunrng[elv][0]/10, 
            Accdata.accsecsaz[elv][1],
            Accdata.accunvel[elv][1]/100.0,
            Accdata.accunrng[elv][1]/10, 
            Accdata.accatmos[elv]/1000.0 );

      else if( num_sectors == 3 )
         fprintf( stderr,
            " %2d   %5.1f %5.2f  %3d  %5.1f %5.2f  %3d  %5.1f %5.2f  %3d  %5.4f\n",
            elev_num, 
            Accdata.accsecsaz[elv][0],
            Accdata.accunvel[elv][0]/100.0,
            Accdata.accunrng[elv][0]/10, 
            Accdata.accsecsaz[elv][1],
            Accdata.accunvel[elv][1]/100.0,
            Accdata.accunrng[elv][1]/10, 
            Accdata.accsecsaz[elv][2],
            Accdata.accunvel[elv][2]/100.0,
            Accdata.accunrng[elv][2]/10, 
            Accdata.accatmos[elv]/1000.0 );


   }

   /* 
     Put in some blank lines. 
   */   
   fprintf( stderr, "\n \n" );

/* End of Write_elev_accdata(). */
}

/***********************************************************************************
  
   Description:
      Converts time, in milliseconds since midnight, into hrs, min, secs strings.

***********************************************************************************/
static void Convert_time( long timevalue, char *hours, char *minutes, char *seconds ){
 
   int hrs, mins, secs;

   /* 
     Extract the number of hours.
   */
   hrs = timevalue/3600000;

   /* 
     Extract the number of minutes.
   */
   timevalue = timevalue - hrs*3600000;
   mins = timevalue/60000;

   /* 
     Extract the number of seconds.
   */
   secs = timevalue - mins*60000;

   /* 
     Convert numbers to ASCII.
   */
   To_ASCII( hrs, hours, 0 );
   To_ASCII( mins, minutes, 0 );
   To_ASCII( secs, seconds, 1 );

/* End of Convert_time(). */
}

/*********************************************************************************

   Description:
      Takes value (number) and converts to string.

   Inputs:
      value - value to convert.
      type - type of value to convert.

   Outputs:
      string - converted string.

*********************************************************************************/
static void To_ASCII( int value, char *string, int type ){

   int i; 
   unsigned char digit;

   /* 
     Process integer value.
   */
   if( type == 0 ){

      for( i = 1; i >= 0; i-- ){
   
         /* 
           Produce the text string.
         */
         digit = (unsigned char) value%10;
         string[i] = digit + '0';

         value = value/10; 

      }

      /* 
        Pad the string with string terminator.
      */
      string[2] = '\0';

   }
   else{

      for( i = 5; i >= 3; i-- ){
   
         /* 
           Produce the fractional portion of text string.
         */
         digit = (unsigned char) value%10;
         string[i] = digit + '0';

         value = value/10; 

      }

      /* 
        Put in the decimal point.
      */
      string[2] = '.';

      for( i = 1; i >= 0; i-- ){
   
         /* 
           Produce the fractional portion of text string.
         */
         digit = (unsigned char) value%10;
         string[i] = digit + '0';

         value = value/10; 

      }

      /* 
        Pad the string with string terminator.
      */
      string[6] = '\0';

   }
       
/* End of To_ASCII(). */
}


/***************************************************************************

    Description:
       Given the modified Julian date, returns day, month and year strings.

***************************************************************************/
static void Calendar_date( short date, char *day, char *month, char *year ){

   int l,n, julian;
   int dd, dm, dy;

   /* 
     Convert modified julian to type integer 
   */
   julian = date;

   /* 
     Convert modified julian to year/month/day 
   */
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
   dy = dy - 1900;

   /* 
     Convert numbers to ASCII.
   */
   To_ASCII( dd, day, 0 );
   To_ASCII( dm, month, 0 );
   To_ASCII( dy, year, 0 );
 
   return;

/* End of Calendar_date(). */
}


/***********************************************************************

   Description:  
      This function reads command line arguments.

   Input:        
      argc - Number of command line arguments.
      argv - Command line arguments.

   Output:       
      Usage message

   Returns:      
      0 on success or -1 on failure

*********************************************************************/
static int Read_options( int argc, char **argv ){

   extern char *optarg;    /* used by getopt */
   extern int optind;
   int c;                  /* used by getopt */
   int err;                /* error flag */

   /* The opposite of Continuous_mode is interactive mode. */
   Continuous_mode = 0;
   Abbreviated_listing = 0;
   Verbose = 0;

   err = 0;
   while ((c = getopt (argc, argv, "hcav")) != EOF) {

      switch (c) {

         /* Change to Continuous_mode. */
         case 'c':
            Continuous_mode = 1;
            break;

         /* Change to Abbreviated Listing. */
         case 'a':
            Abbreviated_listing = 1;
            break;

         /* Change to Abbreviated Listing. */
         case 'v':
            Verbose = 1;
            break;

         /* Print out the usage information. */
         case 'h':
             err = 1;
             break;

      } /* End of switch() */

   } /* End of while() */

   /* If Abbreviated_listing is selected without Continuousi_mode,
      raise error. */
   if( Abbreviated_listing && !Continuous_mode )
      err = 1;
      
   if (err == 1) {                     /* Print usage message */

      fprintf( stderr, "Usage: %s [options]\n", argv[0] );
      fprintf( stderr, "       Options:\n" );
      fprintf( stderr, "       -c Continuous mode\n" );
      fprintf( stderr, "       -a Abbreviated Listing (Used with -c)\n" );
      exit(0);

   }

   return (0);

/* End of Read_options(). */
}

/******************************************************************

   Description:
      The RPG event callback function.

   Input:      See LB man page.
        
******************************************************************/
static void An_callback( EN_id_t evtcd, char *msg, int msglen, void *arg ){

   Orpgevt_radial_acct_t *radial_acct = NULL;
   Orpgevt_scan_info_t *scan_info = NULL;

   if( evtcd == ORPGEVT_RADIAL_ACCT ){

      radial_acct = (Orpgevt_radial_acct_t *) msg;
      if( radial_acct->radial_status == GENDVOL )
         End_volume = 1;

      /* We check the elevation number since both the 
         surveillance and Doppler cuts on the lowest split
         cut have GOODBVOL radial status. */
      else if( (radial_acct->radial_status == GOODBVOL)
                            &&
               (radial_acct->elev_num == 1) )
         New_volume = 1;

      /* Are we in chatty mode? */
      if( Verbose && 
          ((radial_acct->radial_status == GENDVOL)
                        ||
           ((radial_acct->radial_status == GOODBVOL) 
                        && 
            (radial_acct->elev_num == 1))) )
         fprintf( stderr, 
            "ORPGEVT_RADIAL_ACCT Received. End_volume: %d, New_volume: %d\n",
            End_volume, New_volume );

   }
   else if( evtcd == ORPGEVT_SCAN_INFO ){

      scan_info = (orpgevt_scan_info_t *) msg;
      if( scan_info->key == ORPGEVT_END_VOL )
         memcpy( &Vol_info[1], msg, sizeof(Orpgevt_scan_info_t) );

      else if( scan_info->key == ORPGEVT_BEGIN_VOL )
         memcpy( &Vol_info[0], msg, sizeof(Orpgevt_scan_info_t) );

      /* Are we in chatty mode? */
      if( Verbose &&
          ((scan_info->key == ORPGEVT_END_VOL)
                           ||    
           (scan_info->key == ORPGEVT_BEGIN_VOL)) )
         fprintf( stderr, 
            "ORPGEVT_SCAN_INFO Received. Volume Scan: %d, key: %d\n", 
            scan_info->data.vol_scan_number, scan_info->key );

   }

}

#ifdef LITTLE_ENDIAN_MACHINE
/**************************************************************************

   Description:
      Byte-swapping function for scan info and radial accouting events.

   Note:
      This is required on LITTLE_ENDIAN_MACHINE since the message is
      passed Big Endian format.

**************************************************************************/
char* Process_event_msg( int where, EN_id_t event, char *msg, int msg_len ){

   static char swapped_scan_info[ ORPGEVT_SCAN_INFO_DATA_LEN+1 ];
   static char swapped_radial_acct[ ORPGEVT_RADIAL_ACCT_LEN+1 ];

   int num_ints, num_shorts;

   if( (event == ORPGEVT_RADIAL_ACCT) && (msg != NULL) ){

      num_shorts = msg_len / sizeof( short );
      MISC_bswap( sizeof(short), msg, num_shorts, swapped_radial_acct );
      memcpy( msg, swapped_radial_acct, msg_len );
      return( msg );

   }
   else if( (event == ORPGEVT_SCAN_INFO) && (msg != NULL) ){

      num_ints = msg_len / sizeof( int );
      MISC_bswap( sizeof(int), msg, num_ints, swapped_scan_info );
      memcpy( msg, swapped_scan_info, msg_len );
      return( msg );

   }


   return( msg );

}

#endif

