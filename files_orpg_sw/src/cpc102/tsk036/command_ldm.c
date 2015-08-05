/* 
 * RCS info
 * $Author $
 * $Locker:  $
 * $Date: 2011/09/19 14:48:08 $
 * $Id: command_ldm.c,v 1.2 2011/09/19 14:48:08 ccalvert Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 *
 *
 */  

/* Includes for the gui start stop options */
#include "archive_II.h"
#include "orpg.h"

/**************************************************************************
 *   Function: Read_archive_II_command
 *
 *   Description: Reads the Archive II status LB and returns the status. 
 *
 *   Input: rpg_lb (rpg LB id)
 *
 *   Output: status
 *
 *   Return: status
 *
 *   Notes:
 **************************************************************************/
int main( int argc, char *argv[] ){

   ArchII_command_t ArchII_command;
   int response, ret;

   while(1){

      fprintf( stdout, "\nSelect from the following commands:\n" );
      fprintf( stdout, "--->1 - Stop Archive II\n" );
      fprintf( stdout, "--->2 - Start Archive II\n" );
      fprintf( stdout, "--->3 - Enter Local Mode\n" );
      fprintf( stdout, "--->4 - Leave Local Mode\n" );
      fprintf( stdout, "--->5 - Enter Record Mode\n" );
      fprintf( stdout, "--->6 - Leave Record Mode\n" );
      fprintf( stdout, "--->7 - Enter Normal Mode\n" );
      fprintf( stdout, "--->0 - Exit Tool\n" );

      scanf( "%d", &response );
      if( (response <= 0) || (response > 7))
         exit(1);

      switch (response){

         case 1:
            ArchII_command.command = ARCHIVE_II_NEED_TO_STOP;
            fprintf( stdout, "\nCommands convert_ldm to Stop Processing Data ----> Stop:\n" );
            fprintf( stdout, "--->Sending Data to LDM (LDM Mode)\n" );
            fprintf( stdout, "--->Sending Data to file (Record Mode)\n" );
            break;

         case 2:
            ArchII_command.command = ARCHIVE_II_NEED_TO_START;
            fprintf( stdout, "\nCommands convert_ldm to Start Processing Data ----> Start:\n" );
            fprintf( stdout, "--->Sending Data to LDM (LDM Mode)\n" );
            fprintf( stdout, "--->Sending Data to file (Record Mode)\n" );
            break;

         case 3:
            ArchII_command.command = ARCHIVE_II_LOCAL_MODE;
            fprintf( stdout, "\nCommand convert_ldm to Stop Sending Data To LDM if Archive II Started\n" );
            break;

         case 4:
            ArchII_command.command = ARCHIVE_II_LDM_MODE;
            fprintf( stdout, "\nCommands convert_ldm to Start Sending Data To LDM if Archive II Started\n" );
            break;

         case 5:
            ArchII_command.command = ARCHIVE_II_RECORD_MODE;
            fprintf( stdout, "\nCommands convert_ldm to Start Recording Data To Local Files\n" );
            break;

         case 6:
            ArchII_command.command = ARCHIVE_II_NO_RECORD_MODE;
            fprintf( stdout, "\nCommands convert_ldm to Stop Recording Data To Local Files\n" );
            break;

         case 7:
            ArchII_command.command = ARCHIVE_II_NORMAL_MODE;
            fprintf( stdout, "\nCommands convert_ldm to Normal Mode:  LDM Mode with No Local Recording\n" );
            break;

         default:
            exit(1);

      }

      /* Write command. */
      ArchII_command.ctime = time(NULL);
      if( (ret = ORPGDA_write( ORPGDAT_ARCHIVE_II_INFO, (char *) &ArchII_command,
                               sizeof( ArchII_command_t ),
                               ARCHIVE_II_COMMAND_ID )) < 0 ){

         fprintf( stderr, "ORPGDA_write( ORPGDAT_ARCHIVE_II_INFO, ARCHIVE_II_COMMAND_ID ) Failed: %d\n",
                  ret );
         exit(0);

      }

   } /* End of while(1) loop. */

   return 0;

} /* End of main() */
