/*   @(#) commupdate.c 99/11/02 Version 1.32   */

/*<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:

                            Copyright (c) 1991 by
 
               +++    +++                           +++     +++
               +++    +++                           +++     +++
               +++    +++                            +++   +++
               +++    +++   +++++     + +    +++   +++++   +++
               +++    +++  +++++++  +++ +++  +++   ++++++ ++++
               +++    +++ ++++++++ ++++ ++++ ++++  ++++++ +++
               +++    +++++++   ++ ++++ ++++ ++++  +++ ++++++
               +++    ++++++      +++++ ++++++++++ +++ +++++
               +++    ++++++      +++++ ++++++++++ +++  +++
               +++    ++++++      ++++   +++++++++ +++  +++
               +++    ++++++                             +
               +++    ++++++      ++++   +++++++ +++++  +++
               +++    ++++++      +++++ ++++++++ +++++  +++
               +++    ++++++      +++++ ++++++++ +++++ +++++
               +++    +++++++   ++ ++++ ++++ +++  ++++ +++++
               +++    +++ ++++++++ ++++ ++++ +++  +++++++++++
               +++    +++  +++++++  +++ +++  +++   ++++++ ++++
                +++  +++    +++++     + +    +++   ++++++  +++
                ++++++++                             +++    +++
                 ++++++         Corporation         ++++    ++++
                  ++++   All the right connections  +++      +++
 
 :<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>*/


/*<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:
 
   This software is furnished  under  a  license and may be used and
   copied only  in  accordance  with  the  terms of such license and
   with the inclusion of the above copyright notice.   This software
   or any other copies thereof may not be provided or otherwise made
   available to any other person.   No title to and ownership of the
   program is hereby transferred.
 
   The information  in  this  software  is subject to change without
   notice and should not  be  considered  as  a  commitment by UconX
   Corporation.
                          UconX Corporation
                        San Diego, California
 
 :<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>*/

/*******************************************************************
Modifications

1. 22-apr-96    pmt    removed restriction of 3MB; added printfs to clarify
                       how to use; added byte-swapping to support little-
                       endian clients.
2. 25-jul-96    pmt    fixed byte swapping. use ntohl when reading values
                       from file, then htonl when writing. 
3. 13-sep-96    pmt    new menu added. can now tweak the system report setting
4. 16-dec-96    pmt    allow specification of pathname as command line option
5. 14-mar-97    lmm    don't include <netinet/in.h> for DGUX
6. 17-apr-97    lmm    rewrote this utility
7. 29-apr-97    lmm    added further validation in buffer_config
8. 06-JUN-97    mpb    No "&" when passing mmap in fread() since it is a
                       structure (the sunos compiler complains).
9. 06-JUN-97    mpb    'totalSize' should have default value (do not assume
                       it gets initialized to zero).
10. 23-JUL-97   lmm    support for DECUX
11. 28-aug-97   LMM    Check only 1st 6 chars for PTI330
12. 05-NOV-97   mpb    main() should return void to aVOID compiler warnings.
13. 02-JUL-98   LMM    Memory for TCP mbufs and mclbufs is now configurable
                       Also added ramsize to config struct
14. 02-SEP-98   mpb    Make example path string Windows NT style if compiled
                       under NT.
15. 22-JAN-99   lmm    Added memory resize, deleted report frequency
16. 05-MAY-99   tdg    Ported to VxWorks.
*******************************************************************/

#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>
#ifndef VMS
#ifndef WINNT
#include    <unistd.h>    /* required for SunOS */
#endif /* !WINNT */
#endif /* !VMS */
#include        <sys/types.h>

#ifndef WINNT

#ifndef DGUX
#include     <netinet/in.h>
#else
#define ntohl(x)        (x)
#define ntohs(x)        (x)
#define htonl(x)        (x)
#define htons(x)        (x)
#endif /* !DGUX */

#else

typedef char *    caddr_t;
#include <winsock.h>
int    getopt ( int, char *const *, const char * );

#endif /* !WINNT */

#ifdef VMS
#include <stat.h>
#else
#include <sys/stat.h>
#endif /* VMS */

/* #10 - for DECUX need to use 32-bit pointers here */
#ifdef DECUX
#pragma pointer_size save
#pragma pointer_size short
#endif
#include "xstopts.h"
#include "xconfig.h"
#include "commupdate.h"
#ifdef VXWORKS
#include        "mpsproto.h"
#endif /* VXWORKS */

static XCON        xcon;
static MMap        mmap [MAX_MAPSIZE];
#ifdef DECUX
#pragma pointer_size restore
#endif

#define MAXITEMS 3
#define VALIDOPTIONS "f:p:"

/* #14 */
#ifdef WINNT
#define EXAMPLE_STRING  "c:\\UconX\\Load\\config.hdlc.0"
#else
#define EXAMPLE_STRING  "/usr/UconX/Load/config.hdlc.0"
#endif /* WINNT */

extern char     *optarg;
extern int      optind, opterr;

static char        fname  [ 129 ];
static char        inpbuf [ 129 ];
static int         changes_made = 0;
static int         file_size;
static char        *the_whole_file;
static int         initial_ramsize;	/* #15 - original ramsize */
static int         *mapptr; 		/* #15 - ptr to last entry in map */

CPARMS cp;

/* Function declarations. */
static void buffer_config ( );
static int  getline ( );
static int  read_config_file ( );
static void memory_config ( );		/* #15 */
static int  write_config_file ( );
static void exit_program ( );

/*<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:

        main

 :<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>*/

#ifdef VXWORKS
int commupdate (argc, argv, ptg)
#else
int main(argc, argv)
#endif /* VXWORKS */
int argc;
char *argv[];
#ifdef VXWORKS
pti_TaskGlobals *ptg;
#endif
{
   int  c, item, do_exit, valid_response;
   int forever = TRUE;

#ifdef VXWORKS
   init_mps_thread(ptg);
#endif

   if (( c = getopt ( argc, argv, VALIDOPTIONS ) ) != EOF )
   {
      if ( (c == 'p') || (c == 'f') )
      {
         strcpy ( fname, optarg );
      }
      else 
         exit_program(1);

   }
   else
   {
      /* #14 */
      printf ( "\nEnter config filename below (e.g. %s)\n", EXAMPLE_STRING );
      printf ( "Enter config filename: " );
      if ( getline ( inpbuf, FALSE ) )
         strcpy ( fname, inpbuf );
   }

   /* read config file and extract parameters of interest */
   if ( read_config_file ( fname ) == ERROR )
      exit_program (-1);

   display_memory_usage();

   while ( forever )
   {
      printf("\n  Communications Server Update Utility\n\n");
      printf("      1   Buffer Pool Configuration\n");
      printf("      2   Update Memory Size\n");
      printf("      3   Save Configuration File Updates\n");
      printf("      0   Quit\n\n");

      valid_response = FALSE;
      while (!valid_response)
      {
         item = 1; /*  buffer pool is default */
         printf("   Enter Menu Item (%d):", item);
         if ( getline ( inpbuf, FALSE ) )
         {
            item = ( unsigned int ) strtol ( inpbuf, 0, 10 );
            if ( ( item < 0 ) || ( item > MAXITEMS ) )
               printf("   Error: %d out of range.\n\n", item);
            else
               valid_response = TRUE;
         }
         else
            valid_response = TRUE;
      }

      /* Process the menu item. Exit program if not updating file. */
      switch (item)
      {
        /* Configure buffers */
        case 1:
            buffer_config();
        break;

        /* #15 - Modify memory size */
        case 2:
            memory_config();
        break;

        /* Save config file updates */
        case 3:
            if ( write_config_file ( fname ) == ERROR )
               exit_program (-1);
            break;

        /* Exit */
        default:
        case 0:
            do_exit = TRUE;
            if ( changes_made )
            {
               valid_response = FALSE;
               do_exit = FALSE;
               while (!valid_response)
               {
                  printf ( "\nQuit without saving changes? (n) " );
                  if ( ( ! getline ( inpbuf, FALSE ) ) ||
                       ( inpbuf [ 0 ] == 'n' ) || ( inpbuf [ 0 ] == 'N' ) )
                     valid_response = TRUE;

                  if ( ( inpbuf [ 0 ] == 'y' ) || ( inpbuf [ 0 ] == 'Y' ) )
                  {
                     valid_response = TRUE;
                     do_exit   = TRUE;
                  }
               }
            }

            /* if no changes made or user elected to quit, exit */
            if ( do_exit )
            {
               exit_program(0);
            }
            break;
      } /* end of switch on item selected */

   } /* do forever */

   exit_program(0);
   return 0;    /* Appease the compiler. */

} /* end main */

/*<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:

read_config_file - read configuration file and memory map and extract
                   parameters of interest 

 :<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>*/

int read_config_file ( fname )
char *fname;
{
   int   i, count, last_index, mmap_offset, mapsize;
   FILE  *fd;
   struct stat fs;

   /* open config file to read info from file */
   if ( ( fd = fopen ( fname, "rb" ) ) == NULL )
   {
      printf ( "Unable to open file %s for reading.\n", fname );
      return ( ERROR );
   }

   /* For VMS reasons (see write_config_file for more details), we need to
      save the whole file in memory. */
   if ( stat ( fname, &fs ) == -1 )
   {
      printf ( "Error with configuration file\n" );
      fclose ( fd );
      return ( ERROR );
   }

   file_size = fs.st_size;
 
   the_whole_file = ( char * ) malloc ( file_size );
   if ( !the_whole_file )
   {
      printf ( "Error with malloc()\n" );
      fclose ( fd );
      return ( ERROR );
   }

   fread ( the_whole_file, sizeof(char), file_size, fd );

   /* Rewind to beginning of file. */
   if ( fseek ( fd, 0, SEEK_SET ) == -1 )
   {
      printf ( "Could not rewind configuration file.\n" );
      fclose ( fd );
      return ( ERROR );
   }

   /* read config parameters */
   if ( (count = fread(&xcon, sizeof(char), sizeof(XCON), fd))
              != sizeof(XCON) )
   {
      printf ( "Could not read configuration file.\n" );
      fclose ( fd );
      return ( ERROR );
   }
 
   /* Extract and byte-swap parameters of interest */
   cp.nclass = ntohl(xcon.nclass);
   if (cp.nclass < 1 || cp.nclass > MAXCLASS)
   {
      printf ( "Invalid number of classes=%d in config file.\n", cp.nclass);
      fclose ( fd );
      return ( ERROR );
   }

   /* Data block info */
   for ( i = 0; i < cp.nclass; ++i )
   {
      cp.dblk[i].size = ntohl(xcon.dblk[i].size);
      cp.dblk[i].num = ntohl(xcon.dblk[i].num);
   }
 
   cp.npri      = ntohl(xcon.npri);          /* priorities */
   cp.ntoken    = ntohl(xcon.ntoken);        /* tokens */
   cp.ntimer    = ntohl(xcon.ntimer);        /* timers */
   cp.nstream   = ntohl(xcon.nstream);       /* streams */
   cp.nqueue    = ntohl(xcon.nqueue);        /* queues */
   cp.nmuxlink  = ntohl(xcon.nmuxlink);      /* mux links */
   cp.nstacks   = ntohl(xcon.nstacks);       /* stacks */
   cp.stacksize = ntohl(xcon.stack_size);    /* stack size */
   cp.ramsize   = ntohl(xcon.ramsize);       /* ram size */	/* #13 */
   cp.nmbufs    = ntohl(xcon.nmbufs);        /* mbufs */	/* #13 */
   cp.nmclbufs  = ntohl(xcon.nmclbufs);      /* mclbufs */	/* #13 */
   cp.cid       = ntohl(xcon.l_cid);         /* controller id */
   cp.nboards   = 0;                         /* number of controllers */
   cp.n330s     = 0;                         /* number of 330 controllers */
   strcpy ( cp.btype, xcon.l_btype );        /* board type */

   /* save intial ramsize */
   initial_ramsize = cp.ramsize;
 
   /* count number of controllers (MPS3000 only) */
   if ( ntohl (xcon.cf_flags) & CTLR_PRESENT )
   {
      for (i=0; i < (MAXCTLRS - 1); i++)
      {
         if (strcmp(xcon.boards[i].btype, "unused"))
         {
            cp.nboards++;
            if (!strncmp(xcon.boards[i].btype, "PTI330", 6))  /* #11 */
               cp.n330s++;
         }
      }
   }
 
   /* Get offset to memory map */
   mmap_offset = (sizeof(XCON) + 0xf) & ~0xf;
 
   /* Skip to offset containing size of map */
   mmap_offset += 4;
 
   /* Position file to get mapsize */
   if ( fseek ( fd, mmap_offset, SEEK_SET ) == -1 )
   {
      printf ( "Could not seek configuration file.\n" );
      fclose ( fd );
      return ( ERROR );
   }
 
   /* Now read mapsize */
   if ( (count = fread(&mapsize, sizeof(int), 1, fd)) != 1 )
   {
      printf ( "Could not determine map size\n" );
      fclose ( fd );
      return ( ERROR );
   }
 
   mapsize = ntohl(mapsize);

   /* Determine size of entire map - we really only want enough of
      the map to size amount of free memory */
   if ( mapsize > MAX_MAPSIZE )
      mapsize = MAX_MAPSIZE;
 
   mapsize = mapsize * sizeof (MMap);
 
   /* Read in the memory map */
   /* #8 */
   if ( (count = fread(mmap, sizeof(char), mapsize, fd)) != mapsize )
   {
      printf ( "Could not memory map. count=%d\n", count );
      fclose ( fd );
      return ( ERROR );
   }
 
   /* Determine amount of free memory - the list is terminated
      by an entry with a null size */

   for (i=0, cp.freemem=0; ( i<MAX_MAPSIZE && ntohl(mmap[i].size) ); i++)
   {
      cp.freemem += ntohl ( mmap[i].size );
      last_index = i;
   }

   /* #15 - Save ptr to last entry in map - remember that the initial entry
            in the map contains the number of entries, so we need to add 1
            to "last_index" in the calculation below */

   mapptr = (int *) ((u_int) the_whole_file + mmap_offset + 
                             ( last_index + 1 ) * sizeof(MMap));

   fclose ( fd );
   return 0;

} /* end read_config_file */

/*<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:

write_config_file - write updated configuration file 

 :<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>*/

int write_config_file ( fname )
char *fname;
{
   int  i, count, valid_response;
   int  map_value;
   FILE *fd;

   valid_response = FALSE;
   while (!valid_response)
   {
      printf ( "\nDo you really want to update %s? (n) ", fname );
      if ( ( ! getline ( inpbuf, FALSE ) ) ||
           ( inpbuf [ 0 ] == 'n' ) || ( inpbuf [ 0 ] == 'N' ) )
         return 0;

      if ( ( inpbuf [ 0 ] == 'y' ) || ( inpbuf [ 0 ] == 'Y' ) )
         valid_response = TRUE;
   }

   /* We want to just update the XCON structure of the file which is at
      the beginning of the file.  So, open for update, and the file
      pointer will be at the start of the file. */
   if ( ( fd = fopen ( fname, "wb" ) ) == NULL )
   {
      printf ( "Unable to open file %s for writing.\n", fname );
      return ( ERROR );
   }

   /* swap back before writing to file */
   for ( i = 0; i < MAXCLASS; ++i )
   {
      xcon.dblk[i].size = htonl(cp.dblk[i].size);
      xcon.dblk[i].num = htonl(cp.dblk[i].num);
   }

   xcon.nclass = htonl(cp.nclass); 
   xcon.ramsize = htonl(cp.ramsize);		/* #15 */

   /***
      In an ideal world, we would like to just update the XCON structure
      information in the config file.  This would amount to opening the
      file for updating, and write the XCON structure.  Tis would not
      change the memory map information which is after the structure.
      
      BUT, VMS is not ideal.  For some unexplained reason, doing it the way
      mentioned does not work.  So, we will open the file as a brand new
      one (re-writing the stuff in it), write the updated XCON struct, then
      write the rest of the config file stuff which we saved in 
      read_config_file(), seek back to the beginning.
   ***/

   if ( ( count = fwrite ( &xcon, sizeof ( char ),
               sizeof ( XCON ), fd ) ) != sizeof ( XCON ) )
   {
      printf ( "Could not update configuration file.\n" );
      return ( ERROR );
   }

   /* #15 - if memory size was changed,  update last entry in map */
   if ( cp.ramsize != initial_ramsize )
   {
      /* get amount of memory available in last map entry */
      map_value = ntohl(*mapptr);

      /* modify this value according to resized memory */ 
      map_value += ( cp.ramsize - initial_ramsize );

      /* store in buffer in host order */
      *mapptr =  htonl( map_value );
   }

   fwrite ( the_whole_file+sizeof(XCON), sizeof(char), 
            file_size-sizeof(XCON), fd );
 
   changes_made = FALSE;

   fclose ( fd );
   return 0; 

} /* end write_config_file */


/*<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:

buffer_config - configure buffer pools

 :<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>*/

void buffer_config ()
{
   int    i, j;
   int    totalSize = 0;          /* #9 */
   int    valid_response;
   int    value;

   printf ( "\nCurrent Configuration:\n" );
   for ( i = 0; i < cp.nclass; ++i )
   {
      printf ( "Class %d\n", i + 1 );
      /* round up block size to multiple of 16 */
      cp.dblk [ i ].size = ( cp.dblk [ i ].size + 15 ) & 0xfffffff0;
      printf ( "   Size     %d\n", cp.dblk [ i ].size );
      printf ( "   Number   %d\n", cp.dblk [ i ].num );
      totalSize += (cp.dblk[i].num * cp.dblk[i].size);
   }
   
   printf ( "\nYou may configure up to %d classes of data blocks, where\n" , 
             MAXCLASS);
   printf ( "each class contains blocks of a different size.\n" );
   printf ( "You must configure at least one class.\n\n" );

   valid_response = FALSE;
   while (!valid_response)
   {
      printf ( "Enter number of data block classes (%d)", cp.nclass );
      if ( getline ( inpbuf, FALSE ) )
      {
         value = ( unsigned int ) strtol ( inpbuf, 0, 10 );
         if ( value < 1 || value > MAXCLASS )
            printf("Error: %d out of range.\n", value);
         else
         {
            cp.nclass = ( int ) value;
            valid_response = TRUE;
            changes_made = TRUE;
     }
      }
      else
         valid_response = TRUE;
   }
   
   printf ( "\nFor each class, specify the block size and the number of\n" );
   printf ( "blocks to build.  Configure the class with the smallest\n" );
   printf ( "block size first, in order up to the largest.\n\n");
   printf ( "Note that buffer sizes will be rounded up to multiples of 16.\n\n");

   for ( i = 0; i < cp.nclass; ++i )
   {
      printf ( "Class %d\n", i + 1 );
      valid_response = FALSE;
      while (!valid_response)
      {
         printf ( "Enter block size (%d)", cp.dblk [ i ].size );
         if ( getline ( inpbuf, FALSE ) )
         {
            value = strtol ( inpbuf, 0, 10 );
            if ( value <= 0 )
            {
               printf("Error: %d out of range.\n", value);
            }
            else if ( i > 0 && ( value <= cp.dblk [ i - 1 ].size ) )
            {
               printf ( "Please specify a block size larger than %d\n",
                         cp.dblk [ i -1 ].size );
            }
            else
            {
               /* save value and round up to multiple of 16 */
               cp.dblk [ i ].size = ( value + 15 ) & 0xfffffff0;
               changes_made = TRUE;
               valid_response = TRUE;
            }
         }
         else
         {
            value = cp.dblk [ i ].size;
            if ( i > 0 && ( value <= cp.dblk [ i - 1 ].size ) )
            {
               printf ( "Please specify a block size larger than %d\n",
                         cp.dblk [ i -1 ].size );
            }
            else
               valid_response = TRUE;
         }
      }

      valid_response = FALSE;
      while (!valid_response)
      {
         printf("Enter Number of blocks to build (%d)", cp.dblk [ i ].num);
         if ( getline ( inpbuf, FALSE ) )
         {
            value = ( unsigned int ) strtol ( inpbuf, 0, 10 );
            if ( value <= 0 )
               printf("Error: %d out of range.\n", value);
            else
            {
               cp.dblk [ i ].num = value;
               changes_made = TRUE;
               valid_response = TRUE;
            }
         }
         else
            valid_response = TRUE;
      }
   }

   for ( j = i; j < MAXCLASS; ++j )
   {
      cp.dblk [ j ].size = 0;
      cp.dblk [ j ].num  = 0;
   }

   totalSize = 0;

   printf ( "\nNew Configuration:\n" );
   for ( i = 0; i < cp.nclass; ++i )
   {
      printf ( "Class %d\n", i + 1 );
      printf ( "   Size     %d\n", cp.dblk [ i ].size );
      printf ( "   Number   %d\n", cp.dblk [ i ].num );

      totalSize += (cp.dblk[i].num * cp.dblk[i].size);
   }

   display_memory_usage();

} /* end buffer_config */

/*<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:

#15 - memory_config - modify memory size  

 :<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>*/

void memory_config ( )
{
   int oldsize, value;

   oldsize = cp.ramsize;

   printf ("\nWARNING: If you modify the memory size, you MUST be sure that\n");
   printf ("the server/controller hardware has sufficent memory onboard!\n"); 

   printf ("\nCurrent memory size is %d MB\n", cp.ramsize/0x100000 );
   for ( ;; )
   {
      printf ("Enter new memory size in MB (4, 8, 16, or 32): ");
      if ( getline ( inpbuf, FALSE ) )
      {
         value = ( unsigned int ) strtol ( inpbuf, 0, 10 );
         if ( (value != 4) && (value !=8) && (value !=16) && (value !=32) )
            printf("Error: %d out of range.\n", value);
         else
         {
            cp.ramsize = ( int ) value*0x100000;
            changes_made = TRUE;
            break;
         }
      }
      else
         break;              /* take default */
   }

   if ( changes_made )
   {
      cp.freemem += ( cp.ramsize - oldsize ); 
      display_memory_usage();
   }

} /* end memory_config */

/*<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:

getline - reads a line of input from the terminal.

 :<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>*/

int getline ( p_buffer, echo )
char    *p_buffer;
int     echo;
{
   int          i;
   char         c;

   printf ( " > " );
   for ( i = 0; i < 80; ++i )
   {
      c = getchar ( );

      switch ( c )
      {

/* Ignore spaces */

         case 0x20:
            --i;
            continue;

/* BS or DEL            */

         case 0x08:
         case 0x7f:
            if ( i )
            {
               if ( echo )
               {
                  putchar ( 0x8  );
                  putchar ( 0x20 );
                  putchar ( 0x8  );
               }
               i -= 2;                  /* Back out the char & BS/DEL */
            }
            else
               --i;                     /* Beginning of line    */
            continue;

/* CR -- This will end the input */

         case '\n':
/******
            putchar ( '\n' );
******/
            break;

/* All other chars */

         default:
            if ( echo )
               putchar ( c );
            *( p_buffer + i ) = c;
            continue;
      }
      break;
   }
   if ( i )
      *( p_buffer + i ) = ( unsigned char ) 0;   /* Null terminate */
   return ( i );

} /* end getline */

/**********

exit_program() ---

Instead of just exiting, clean up system before exit.

**********/
#ifdef    ANSI_C
static void exit_program ( int error_val )
#else
static void exit_program ( error_val )
int    error_val;
#endif    /* ANSI_C */
{

#ifdef VXWORKS
    cleanup_mps_thread ( );
#endif 

    exit ( error_val );
} /* end exit_program() */
