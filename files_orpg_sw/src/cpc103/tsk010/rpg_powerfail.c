/************************************************************************

      The main source file of the tool "rpg_powerfail".

************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2007/01/17 17:43:12 $
 * $Id: rpg_powerfail.c,v 1.2 2007/01/17 17:43:12 ccalvert Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/* Include files. */

#include <unistd.h>
#include <stdlib.h>
#include <misc.h>

/* Macros. */

#define	ROOT_UID		0
#define	MAX_TEXT_OUTPUT_SIZE	1024

/**************************************************************************

    The main function.

**************************************************************************/

int main( int argc, char **argv )
{
  int ret = -1;
  char output_buffer[ MAX_TEXT_OUTPUT_SIZE ];
  int n_bytes = -1;

  /* Become root and send SIGPWR to init. */

  if( seteuid( ( uid_t ) ROOT_UID ) != 0 )
  {
    printf( "Error: unable to seteuid to root.\n" );
    exit( 1 );
  }

  ret = MISC_system_to_buffer( "kill -SIGPWR 1", output_buffer,
                               MAX_TEXT_OUTPUT_SIZE, &n_bytes );
  ret = ret >> 8;

  if( ret != 0 )
  {
    printf( "Error: command kill -SIGPWR 1 failed (%d).\nError message: %s\n",
            ret, output_buffer );
    exit( 1 );
  }

  exit( 0 );
}
