/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2009/05/14 15:08:59 $
 * $Id: mem_use.c,v 1.2 2009/05/14 15:08:59 steves Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

#include <mem_use.h>

/* Static Variables. */

/* Process ID. */
static int Pid;

/* Output Long Listing option. */
static int Long_listing;

/* File pointer for the /proc/<pid>/smaps file. */
static FILE *Fp_smaps = NULL;
static char Smaps_file_name[256];

/* File pointer for the /proc/<pid>/cmdline file. */
static FILE *Fp_cmdline = NULL;

/* Command Line. */
static char Command_line[256];

/* Note: 1 kB = 1024 bytes. */
typedef struct mapping{

   char label[128];			/* Path to library or executable. */
   
   unsigned long long vm_size;		/* Virtual memory size, in kB. */

   unsigned long long rss_size;		/* Resident set size, in kB. */

   unsigned long long private_clean;	/* Clean, private memory, in kB. */

   unsigned long long private_dirty;	/* Dirty, private memory, in kB. */

   unsigned long long shared_clean;	/* Clean, shared memory, in kB. */
   
   unsigned long long shared_dirty;	/* Dirty, shared memory, in kB. */

   unsigned long long swap;		/* Swap size, in kB. */

} Mapping_t;

typedef struct mapping_list {

   Mapping_t map;

   struct mapping_list *next;

} Mapping_list_t;

/* Linked list Head and Tail pointers, and the number of elements
   in the list.  */
static Mapping_list_t *List_head = NULL;
static Mapping_list_t *List_tail = NULL;
static int List_size = 0;

/* Information about memory usage. */
static unsigned long long Private_clean = 0;
static unsigned long long Private_dirty = 0;
static unsigned long long Shared_clean = 0;
static unsigned long long Shared_dirty = 0;
static unsigned long long Rss_size = 0;
static unsigned long long Vm_size = 0;
static unsigned long long Vm_size_max = 0;
static unsigned long long Rss_size_max = 0;
static unsigned long long Shared_size_max = 0;
static unsigned long long Private_size_max = 0;

/* Function Prototypes. */
static void Parse_line( int line_id, char *str, Mapping_list_t *list );
static unsigned long long Find_value( char *str );
static Mapping_list_t* Add_to_list();
static void Display_long_listing();
static void Get_proc_args();
static void Free_list();


/*\/////////////////////////////////////////////////////////////////////////

   Description:
      Initializes the memory use library.

   Inputs:
      pid - Process ID for process to monitor.
      long_listing - Summary or extended smaps formatted output.

   Returns:
      -1 on error, 0 on success.

/////////////////////////////////////////////////////////////////////////\*/
int MU_initialize( int pid, int long_listing ){

   /* Set file scope variables. */
   Pid = pid;
   Long_listing = long_listing;

   /* Construct the file name for the smaps file. */
   sprintf( Smaps_file_name, "/proc/%d/smaps", pid );
   
   /* Get the command line string. */
   Get_proc_args( pid );

   return 0;

/* End of MU_initialize(). */
}

#define MAX_LENGTH		256
#define MAX_LINES		  8

#define MEMORY_LABEL		0
#define VM_SIZE			1
#define RES_SET_SIZE		2
#define SHARED_CLEAN		3
#define SHARED_DIRTY		4
#define PRIVATE_CLEAN		5
#define PRIVATE_DIRTY		6
#define SWAP			7

/*\////////////////////////////////////////////////////////////////////////

   Description:
      Reads the smaps file out of the /proc/<pid> directory.   Currently
      the format of this file is (see file fs/proc/task_mmu.c in the 
      Linux kernel code):

      label - String containing library or executable name.
      Virtual Memory size - "Size:          %8lu kB\n"
      Resident Set size - "Rss:           %8lu kB\n"
      Shared Clean size - "Shared_Clean:  %8lu kB\n"
      Shared Dirty size - "Shared_Dirty:  %8lu kB\n"
      Private Clean size - "Private_Clean: %8lu kB\n"
      Private Dirty size - "Private_Dirty: %8lu kB\n",

   Returns:
      -1 on error, 0 on success.

///////////////////////////////////////////////////////////////////////\*/
int MU_display_smaps(){

   static char label[MAX_LINES][MAX_LENGTH];

   int i;
   char *str = NULL;
   Mapping_list_t *list = NULL;
   unsigned long long shared_size = 0, private_size = 0;

   /* Initialize accumulators. */
   Private_clean = 0;
   Private_dirty = 0;
   Shared_clean = 0;
   Shared_dirty = 0;
   Rss_size = 0;
   Vm_size = 0;

   /* Open the smaps file for read-only.   Return to caller on error. */
   Fp_smaps = fopen( Smaps_file_name, "r" );
   if( Fp_smaps == NULL ){

      fprintf( stderr, "Error Opening File %s.\n", Smaps_file_name );
      return -1;

   }

   /* Read until the end of the file. */
   while(1){

      /* Assumes there are MAX_LINES for each library or executable
         name. */      
      for( i = 0; i < MAX_LINES; i++ ){

          str = fgets( label[i], MAX_LENGTH, Fp_smaps );
          if( str == NULL )
             break;

          /* Add a new list element. */
          if( i == MEMORY_LABEL )
             list = Add_to_list();

          Parse_line( i, label[i], list );

      }

      /* An error must have occurred or end of input.  Break out of
         loop. */
      if( str == NULL )
         break;

   }

   /* Display information. */
   list = List_head;
   for( i = 0; i < List_size; i++ ){

      /* Check that the list pointer is defined.  It should always
         be defined. */
      if( list == NULL ){

         fprintf( stderr, "Coding Error .... \n" );
         exit(2);

      }
    
      Vm_size += list->map.vm_size;
      Rss_size += list->map.rss_size;
      Shared_clean += list->map.shared_clean;
      Shared_dirty += list->map.shared_dirty;
      Private_clean += list->map.private_clean;
      Private_dirty += list->map.private_dirty;

      /* Go to the next list element. */
      list = list->next;

   }

   /* Write out summary. */
   fprintf( stderr, "-----------------------------------------\n" );
   fprintf( stderr, "CMD LINE:    %s\n", Command_line );
   fprintf( stderr, "VMSIZE:      %8llu kB\n", Vm_size );
   fprintf( stderr, "RSS:         %8llu kB total\n", Rss_size );

   shared_size = Shared_clean + Shared_dirty;
   fprintf( stderr, "SHARED:      %8llu kB total\n", shared_size );

/* For the time being, we comment this out.  It probably does not have
   significant meaning to average user of this tool. 
   fprintf( stderr, "             %8llu kB clean\n", Shared_clean );
   fprintf( stderr, "             %8llu kB dirty\n", Shared_dirty );
*/

   private_size = Private_clean + Private_dirty;
   fprintf( stderr, "PRIVATE:     %8llu kB total\n", private_size );

/* For the time being, we comment this out.  It probably does not have
   significant meaning to average user of this tool. 
   fprintf( stderr, "             %8llu kB private clean\n", Private_clean );
   fprintf( stderr, "             %8llu kB private dirty\n", Private_dirty );
*/

   if( Long_listing )
      Display_long_listing();

   /* Set maximum values. */
   if( Vm_size > Vm_size_max )
      Vm_size_max = Vm_size;

   if( Rss_size > Rss_size_max )
      Rss_size_max = Rss_size;

   if( shared_size > Shared_size_max )
      Shared_size_max = shared_size;

   if( private_size > Private_size_max )
      Private_size_max = private_size;

   /* Free the mapping linked list. */
   Free_list();

   /* Close the /proc/<pid>/smaps file. */
   fclose( Fp_smaps );

   return 0;

/* End of MU_display_smaps(). */
}


/*\/////////////////////////////////////////////////////////////////////////

   Description:
      Prints out the maximum size values for VM, RSS, PRIVATE and SHARED.

/////////////////////////////////////////////////////////////////////////\*/
void MU_print_max_values(){

   fprintf( stderr, "\n=========================================\n" );
   fprintf( stderr, "Max VMSIZE:  %8llu kB\n", Vm_size_max );
   fprintf( stderr, "Max RSS:     %8llu kB total\n", Rss_size_max );
   fprintf( stderr, "Max SHARED:  %8llu kB total\n", Shared_size_max );
   fprintf( stderr, "Max PRIVATE: %8llu kB total\n", Private_size_max );
   fprintf( stderr, "=========================================\n" );

/* End of MU_print_max_values(). */
}


/*\////////////////////////////////////////////////////////////////////////

   Description: 
      This function reads command line arguments.

   Inputs:     
      argc - number of command arguments
      argv - the list of command arguments

   Returns:     
      It returns 0 on success or -1 on failure.

///////////////////////////////////////////////////////////////////////\*/
void Parse_line( int line_id, char *str, Mapping_list_t *list ){

   char *substr = NULL;
   unsigned long long value = 0;

   /* Check if list is NULL ... If it is, just return. */
   if( list == NULL )
      return;
 
   /* Process based on line id. */
   switch( line_id ){

      case MEMORY_LABEL:
      {
         substr = strstr( str, "/" );
         
         if( substr != NULL )
            strcpy( list->map.label, substr );

         else{

            substr = strstr( str, "[" );
     
            if( substr != NULL )
               strcpy( list->map.label, substr );

            else
               strcpy( list->map.label, "[anon]" );

         }

         if( list->map.label[strlen(list->map.label)-1] == '\n' )
            list->map.label[strlen(list->map.label)-1] = '\0';

         break;
      }

      case VM_SIZE:
      {
         value = Find_value( str );
         list->map.vm_size = value;

         break;
      }

      case RES_SET_SIZE:
      {
         value = Find_value( str );
         list->map.rss_size = value;

         break;
      }


      case SHARED_CLEAN:
      {
         value = Find_value( str );
         list->map.shared_clean = value;

         break;
      }

      case SHARED_DIRTY:
      {
         value = Find_value( str );
         list->map.shared_dirty = value;

         break;
      }
      
      case PRIVATE_CLEAN:
      {
         value = Find_value( str );
         list->map.private_clean = value;

         break;
      }

      case PRIVATE_DIRTY:
      {
         value = Find_value( str );
         list->map.private_dirty = value;

         break;
      }

      case SWAP:
      {
         value = Find_value( str );
         list->map.swap = value;

         break;
      }

   }

/* End of Parse_line() */
}

/*\////////////////////////////////////////////////////////////////////////

   Description: 
      This function allocates a Mapping_list_t element and appends this
      element to the end of a linked list.

   Returns:
      Address of the newly created list element.

////////////////////////////////////////////////////////////////////////\*/
static Mapping_list_t* Add_to_list(){

   Mapping_list_t *list;

   /* Allocate a Mapping_list element. */
   list = (Mapping_list_t *) calloc( 1, sizeof(Mapping_list_t) );

   /* A calloc failure is fatal .... */
   if( list == NULL ){

      fprintf( stderr, "calloc Failed!!\n" );
      exit(1);

   }

   /* Set up the list head if not already defined. */
   if( List_head ==  NULL )
      List_head = list;

   /* Link in this list element to the list tail. */
   if( List_tail != NULL )
      List_tail->next = list;

   List_tail = list;
   List_tail->next = NULL;

   /* Increment the list size. */
   List_size++;

   /* Return the address of the list element. */
   return list;

/* End of Add_to_list(). */
}

/*\////////////////////////////////////////////////////////////////////////

   Description: 
      This function parses a string for an unsigned long value.

   Returns:
      The unsigned long value.

////////////////////////////////////////////////////////////////////////\*/
static unsigned long long Find_value( char *str ){

   char *substr1 = NULL;
   char *substr2 = NULL;

   unsigned long value;

   /* Initialize the return value to 0. */
   value = 0;

   /* Find the semi-colon (:). */
   substr1 = strstr( str, ":" );

   /* Find the units identifier (kB). */
   substr2 = strstr( str, "kB" );
        
   /* If neither substring is NULL, extract unsigned ling value. */
   if( (substr1 != NULL) && (substr2 != NULL) ){

      substr1++;
      substr2--;
         
      if( substr1 <= substr2 )
         value = strtoull( substr1, &substr2, 10 );

   }

   return value;

/* End of Find_value(). */
}

/*\///////////////////////////////////////////////////////////////////////////

   Description:
      Does a long listing of the memory mapping.

///////////////////////////////////////////////////////////////////////////\*/
static void Display_long_listing(){

   Mapping_list_t *list;
   int i;

   /* First display the private mappings. */
   fprintf( stderr, "\nPRIVATE MAPPINGS:\n" );

   /* Write header. */
   fprintf( stderr, "    vm size       rss size     clean size     dirty size      file\n" );

   /* Display information. */
   list = List_head;
   for( i = 0; i < List_size; i++ ){

      /* Check that the list pointer is defined.  It should always
         be defined. */
      if( list == NULL ){

         fprintf( stderr, "Coding Error .... \n" );
         exit(2);

      }

      if( (list->map.private_clean != 0) || (list->map.private_dirty != 0) )
         fprintf( stderr, "%8llu kB    %8llu kB    %8llu kB    %8llu kB    %s\n", 
                  list->map.vm_size, list->map.rss_size, list->map.private_clean,
                  list->map.private_dirty, list->map.label );

      /* Go to the next list element. */
      list = list->next;

   }

   /* Last, disply the shared mappings. */
   fprintf( stderr, "\n\nSHARED MAPPINGS:\n" );

   /* Write header. */
   fprintf( stderr, "    vm size       rss size     clean size     dirty size      file\n" );

   /* Display information. */
   list = List_head;
   for( i = 0; i < List_size; i++ ){

      /* Check that the list pointer is defined.  It should always
         be defined. */
      if( list == NULL ){

         fprintf( stderr, "Coding Error .... \n" );
         exit(2);

      }

      if( (list->map.shared_clean != 0) || (list->map.shared_dirty != 0) )
         fprintf( stderr, "%8llu kB    %8llu kB    %8llu kB    %8llu kB    %s\n",
                  list->map.vm_size, list->map.rss_size, list->map.shared_clean,
                  list->map.shared_dirty, list->map.label );

      /* Go to the next list element. */
      list = list->next;

   }


/* End of Display_long_listing() */
}

/*\///////////////////////////////////////////////////////////////////////////

   Description:
      Creates the command line string of the process we are displaying 
      memory utilization for.

///////////////////////////////////////////////////////////////////////////\*/
static void Get_proc_args( ){

   char file_name[255];
   char temp_args[255];
   char *args_ptr = NULL;
   int i, len;

   /* Construct the file name for the smaps file. */
   sprintf( file_name, "/proc/%d/cmdline", Pid );

   /* Open the cmdline file for read-only.   Exit on error. */
   Fp_cmdline = fopen( file_name, "r" );
   if( Fp_cmdline == NULL ){
   
      fprintf( stderr, "Error Opening File %s.\n", file_name );
      exit(0);

   }

   /* Read the command line. */
   args_ptr = fgets( temp_args, 256, Fp_cmdline );

   /* If error reading, return. */
   if( (args_ptr == NULL) || (args_ptr != temp_args) )
      return;

   /* Create the command line string. */
   len = 0;
   i = 0;
   while(1){

      while( (len < 255) && (temp_args[len] != '\0') ){

         Command_line[i] = temp_args[len];
         i++;
         len++;

      }

      if( len>= 255 )
         break;

      Command_line[i] = ' ';
      i++;

      /* Move past the end NULL in the input string. */
      len++;
      if( temp_args[len] == '\0' )
         break;

   }

   /* Close the /proc/<pid>/cmdline file. */
   fclose( Fp_cmdline);

/* End of Get_proc_args() */
}

/*\///////////////////////////////////////////////////////////////////////////

   Description:
      Frees memory associated with mapping linked-list. 

///////////////////////////////////////////////////////////////////////////\*/
static void Free_list( ){

   Mapping_list_t *list = NULL;
   Mapping_list_t *list_next = NULL;

   /* Nothing to do on an empty list. */
   if( List_size == 0 )
      return;
     
   list = List_head;

   /* Free all the list nodes. */
   while( list != NULL ){

      list_next = list->next;
      free( list );
      list = list_next;

   }
   
   /* Initialize the list head and tail pointers and set the list size to 0. */
   List_head = NULL;
   List_tail = NULL;
   List_size = 0;

/* End of Free_list(). */
}
