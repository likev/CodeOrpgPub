
/******************************************************************

    This tool edits the user-defined DEA paramter. It is based on
    cpc102/tsk081/edit_dea.c, but doesn't print any extraneous info.
    The tool is primarily to be used for setting values on an
    operational system for field tests.
	
******************************************************************/

/* 
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2010/01/08 17:01:56 $
 * $Id: change_dea_value.c,v 1.1 2010/01/08 17:01:56 ccalvert Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */  

#include <orpg.h> 
#include <infr.h> 

#define	MAX_NUM_DOUBLE_VALUES	1024

static char *Dea_id = NULL;
static char *New_value = NULL;
static int Index_of_new_value = 0;
static int Update_baseline = 0;

static void Read_options( int, char ** );
static void Print_usage( char ** );

/******************************************************************
  The main function.
******************************************************************/

int main( int argc, char **argv )
{
  char *db_name = NULL;
  char *new_string_array = NULL;
  char *current_string_array = NULL;
  char char_value;
  int i = 0;
  int ret_code = 0;
  int dea_data_type = 0;
  int num_values = 0;
  int  array_size = 0;
  double double_array[MAX_NUM_DOUBLE_VALUES+1];
  double double_value = -1.0;
  DEAU_attr_t *at = NULL;

  /* Make sure DEA DB exists. */
  if( ( db_name = ORPGDA_lbname( ORPGDAT_ADAPT_DATA ) ) == NULL )
  {
    fprintf( stderr, "ORPGDA_lbname (%d) failed\n", ORPGDAT_ADAPT_DATA );
    exit(1);
  }
  DEAU_LB_name( db_name );

  /* Parse command line. */
  Read_options( argc, argv );

  /* DEA ID to change missing from command line. */
  if( Dea_id == NULL )
  {
    fprintf( stderr, "No DEA id provided on command line\n" );
    exit(1);
  }

  /* New value for DEA ID missing from command line. */
  if( New_value == NULL )
  {
    fprintf( stderr, "No new value for DEA id provided on command line\n" );
    exit(1);
  }

  /* Make sure DEA ID exists in the DEA DB. */
  if( ( ret_code = DEAU_get_attr_by_id( Dea_id, &at ) ) == DEAU_DE_NOT_FOUND )
  {
    fprintf( stderr, "DEA id not found\n" );
    exit(1);
  }
  else if( ret_code < 0 )
  {
    fprintf( stderr, "DEAU_get_attr_by_id failed (%d)\n", ret_code );
    exit(1);
  }

  /* DEA ID exists. Get pointer to value(s). All IDs are
     considered to be an array of values. Some may only
     have a length of 1 which has index 0. */
  if( ( dea_data_type = DEAU_get_data_type( at ) ) == DEAU_T_STRING )
  {
    if( ( num_values = DEAU_get_string_values( Dea_id, &current_string_array ) ) < 0 )
    {
      fprintf( stderr, "DEAU_get_string_values failed (%d)\n", num_values );
      exit(1);
    }
  }
  else
  {
    if( ( num_values = DEAU_get_values( Dea_id, double_array, MAX_NUM_DOUBLE_VALUES ) ) < 0 )
    {
      fprintf( stderr, "DEAU_get_values failed (%d)\n", num_values );
      exit(1);
    }
  }

  /* Ensure index is valid. */
  if( Index_of_new_value > num_values )
  {
    fprintf( stderr, "Array index (%d) is not valid\n", Index_of_new_value );
    exit(1);
  }

  /* Determine if array size needs to change. */
  array_size = num_values;
  if( Index_of_new_value == num_values )
  {
    array_size = num_values + 1;
  }

  /* Modify/add new value according to data type. */
  if( dea_data_type == DEAU_T_STRING )
  {
    for( i = 0; i < array_size; i++ )
    {
      /* Carry previous values forward and replace value
         corresponding to Index_of_new_value. */
      if( i != Index_of_new_value )
      {
        new_string_array = STR_append( new_string_array,
                                    current_string_array,
                                    strlen( current_string_array )+1 );
      }
      else
      {
        new_string_array = STR_append( new_string_array,
                                             New_value,
                                             strlen( New_value )+1 );
      }
      current_string_array += strlen( current_string_array )+1;
    }
    ret_code = DEAU_set_values( Dea_id, 1, new_string_array, array_size, 0 );
    if( Update_baseline && ret_code == 0 )
    {
      DEAU_set_values( Dea_id, 1, new_string_array, array_size, 1 );
    }
  }
  else
  {
    if( sscanf( New_value, "%lf%c", &double_value, &char_value ) != 1 )
    {
      fprintf( stderr, "Value (%s) isn't numerical\n", New_value );
      exit(1);
    }
    double_array[Index_of_new_value] = double_value;
    ret_code = DEAU_set_values( Dea_id, 0, double_array, array_size, 0 );
    if( Update_baseline && ret_code == 0 )
    {
      DEAU_set_values( Dea_id, 0, double_array, array_size, 1 );
    }
  }

  /* Failed to set value. */
  if( ret_code == DEAU_BAD_RANGE )
  {
    /* New value is not in valid range. */
    fprintf( stderr, "Value \"%s\" is not valid\n", New_value );
    exit(1);
  }
  else if( ret_code < 0 )
  {
    /* Set failed for some other reason. */
    fprintf( stderr, "DEAU_set_values failed (%d)\n", ret_code );
    exit(1);
  }

  /* Print success message and account for index != 0. */
  if( Index_of_new_value > 0 )
  {
    fprintf( stdout, "Id: %s Index: %d set to %s\n",
             Dea_id, Index_of_new_value, New_value );
  }
  else
  {
    fprintf( stdout, "Id: %s set to %s\n", Dea_id, New_value );
  }

  exit(0);
}

/**************************************************************************
  Description: This function reads command line arguments.

  Inputs: argc - number of command arguments
          argv - the list of command arguments
**************************************************************************/

static void Read_options( int argc, char **argv )
{
  extern char *optarg;    /* used by getopt */
  extern int optind;      /* used by getopt */
  int c;

  while( ( c = getopt( argc, argv, "n:i:bh?" ) ) != EOF )
  {
    switch(c)
    {
      case 'n':
        New_value = STR_copy( New_value, optarg );
        break;

      case 'b':
        Update_baseline = 1;
        break;

      case 'i':
        if( sscanf( optarg, "%d", &Index_of_new_value ) != 1 ||
            Index_of_new_value < 0 )
        {
          fprintf( stderr, "Bad -i option\n" );
          Print_usage( argv );
        }
        break;

      case 'h':
      case '?':
        Print_usage( argv );
        break;
    }
  }

  /* Extract DEA if present. */
  if( optind == argc - 1 )
  {
    Dea_id = STR_copy( Dea_id, argv[optind] );
  }
}

/**************************************************************************
  Description: This function prints the usage info.
**************************************************************************/

static void Print_usage( char **argv )
{
  fprintf( stdout, "Usage: %s [options] [DE_ID]\n", argv[0] );
  fprintf( stdout, "\n" );
  fprintf( stdout, "Sets the value of DEA parameter DE_ID.\n" );
  fprintf( stdout, "Options:\n" );
  fprintf( stdout, "    -n new_value (default = no change)\n" );
  fprintf( stdout, "    -i index (if part of array, default = 0)\n" );
  fprintf( stdout, "    -b (also update baseline value)\n" );
  fprintf( stdout, "    -h (show usage info)\n" );
  exit(0);
}

