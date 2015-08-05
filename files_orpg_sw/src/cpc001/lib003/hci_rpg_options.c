/* 
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2011/10/03 21:47:48 $
 * $Id: hci_rpg_options.c,v 1.10 2011/10/03 21:47:48 ccalvert Exp $
 * $Revision: 1.10 $
 * $State: Exp $
 */

/************************************************************************
 *									*
 *	Module:       hci_rpg_options					*
 *									*
 *	Description:  Handles special RPG options defined in the 	*
 *		      "rpg_options" configuration file			*
 *									*
 ************************************************************************/

/*	Local include file definitions.					*/

#include <hci.h>
#include <hci_rpg_options.h>
#include <dirent.h>

static int Num_init_options = 0;   /* Num initialization options defined */
static int Init_options_flag = HCI_NO_FLAG;  /* Read options data */
static  Hci_init_options_t Init_options[HCI_MAX_INIT_OPTIONS];

static void Init_rpg_options();
static void Read_rpg_options_file( char * );
static int  Get_next_file_name( char *, char *, char *, int );

/************************************************************************
 *	Description: This function returns the number of initialization	*
 *		     options defined.					*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: number of initialization options			*
 ************************************************************************/

int HCI_num_init_options()
{
  if( Init_options_flag == HCI_NO_FLAG )
  {
    Init_rpg_options();
    HCI_LE_log( "%d Initialization options are configured", Num_init_options );
  }
  return( Num_init_options );
}


/************************************************************************
 *	Description: This function returns a pointer to the description	*
 *		     string for the specified option.			*
 *									*
 *	Input:  indx - option index					*
 *	Output: NONE							*
 *	Return: pointer to option description string			*
 ************************************************************************/

char* HCI_get_init_options_description( int indx )
{
  char* temp = "";
  if( Init_options_flag == HCI_NO_FLAG ){ Init_rpg_options(); }

  if( (indx >= 0) && (indx < Num_init_options) )
  {
    temp = Init_options[indx].description;
  }

  HCI_LE_log( "Init description[%d] is \"%s\"", indx, temp );
  if( ( temp != NULL ) && (strcmp( temp, "" ) == 0 ) ){ temp = NULL; }
  return( temp );
}

/************************************************************************
 *	Description: This function returns the name of specified	*
 *		     intialization option.				*
 *									*
 *	Input:  indx - index of option					*
 *	Output: NONE							*
 *	Return: A pointer to the initialization option name.		*
 *	Notes:								*
 ************************************************************************/

char *HCI_get_init_options_name( int indx )
{
  char* temp = "";
  if( Init_options_flag == HCI_NO_FLAG ){ Init_rpg_options(); }

  if( (indx >= 0) && (indx < Num_init_options) )
  {
    temp = Init_options[indx].name;
  }

  HCI_LE_log( "Init name[%d] is \"%s\"", indx, temp );
  if( ( temp != NULL ) && (strcmp( temp, "" ) == 0 ) ){ temp = NULL; }
  return( temp );
}

/************************************************************************
 *	Description: This function returns the action of specified	*
 *		     initialization option.				*
 *	Input:  indx - index of option					*
 *	Output: NONE							*
 *	Return: A pointer to the initialization option action.		*
 *	Notes:								*
 ************************************************************************/

char *HCI_get_init_options_action( int indx )
{
  char* temp = "";
  if( Init_options_flag == HCI_NO_FLAG ){ Init_rpg_options(); }

  if( (indx >= 0) && (indx < Num_init_options) )
  {
    temp = Init_options[indx].action;
  }

  HCI_LE_log( "Init action%d] is \"%s\"", indx, temp );
  if( ( temp != NULL ) && (strcmp( temp, "" ) == 0 ) ){ temp = NULL; }
  return( temp );
}

/************************************************************************
 *	Description: This function returns the states the specified	*
 *	 	     initialization option are allowed.			*
 *									*
 *	Input:  indx - index of option					*
 *	Output: NONE							*
 *	Returns: A pointer to the initialization option action.		*
 *	Notes:								*
 ************************************************************************/

char *HCI_get_init_options_state( int indx )
{
  char* temp = "";
  if( Init_options_flag == HCI_NO_FLAG ){ Init_rpg_options(); }

  if( (indx >= 0) && (indx < Num_init_options) )
  {
    temp = Init_options[indx].state;
  }

  HCI_LE_log( "Init state[%d] is \"%s\"", indx, temp );
  if( ( temp != NULL ) && (strcmp( temp, "" ) == 0 ) ){ temp = NULL; }
  return( temp );
}

/************************************************************************
 *	Description: This function returns the permissions required to	*
 *	 	     activate the given initialization option.		*
 *									*
 *	Input:  indx - index of option					*
 *	Output: NONE							*
 *	Returns: A pointer to the initialization option action.		*
 *	Notes:								*
 ************************************************************************/

char *HCI_get_init_options_permission( int indx )
{
  char* temp = "";
  if( Init_options_flag == HCI_NO_FLAG ){ Init_rpg_options(); }

  if( (indx >= 0) && (indx < Num_init_options) )
  {
    temp = Init_options[indx].permission;
  }

  HCI_LE_log( "Init permission[%d] is \"%s\"", indx, temp );
  if( ( temp != NULL ) && (strcmp( temp, "" ) == 0 ) ){ temp = NULL; }
  return( temp );
}

/************************************************************************
 *	Description: This function returns the group name associated 	*
 *	 	     with the given initialization option.		*
 *									*
 *	Input:  indx - index of option					*
 *	Output: NONE							*
 *	Returns: A pointer to the initialization option action.		*
 *	Notes:								*
 ************************************************************************/

char *HCI_get_init_options_group( int indx )
{
  char* temp = "";
  if( Init_options_flag == HCI_NO_FLAG ){ Init_rpg_options(); }

  if( (indx >= 0) && (indx < Num_init_options) )
  {
    temp = Init_options[indx].group;
  }

  HCI_LE_log( "Init group[%d] is \"%s\"", indx, temp );
  if( ( temp != NULL ) && (strcmp( temp, "" ) == 0 ) ){ temp = NULL; }
  return( temp );
}


/************************************************************************
 *	Description: This function returns a pointer to a message which	*
 *		     is written to the system log file when the option	*
 *		     is executed.					*
 *	Input:  indx - index of option					*
 *	Output: NONE							*
 *	Return: A pointer to the message.				*
 *	Notes:								*
 ************************************************************************/

char *HCI_get_init_options_msg( int indx )
{
  char* temp = "";
  if( Init_options_flag == HCI_NO_FLAG ){ Init_rpg_options(); }

  if( (indx >= 0) && (indx < Num_init_options) )
  {
    temp = Init_options[indx].msg;
  }

  HCI_LE_log( "Init msg[%d] is \"%s\"", indx, temp );
  if( ( temp != NULL ) && (strcmp( temp, "" ) == 0 ) ){ temp = NULL; }
  return( temp );
}

/************************************************************************
 *	Description: This function executes the specified		*
 *	 	     initialization option.				*
 *									*
 *	Input:  indx - index of option					*
 *	Output: NONE							*
 *	Return: status of remote procedure call (0 = error).		*
 *	Notes:								*
 ************************************************************************/

int HCI_init_options_exec( int indx )
{
  char* action = NULL;
  char func[HCI_BUF_256];
  char output_buffer[HCI_BUF_2048];
  int rpc_ret = -1;
  int func_ret = -1;
  int n_bytes = -1;

  if( Init_options_flag == HCI_NO_FLAG ){ Init_rpg_options(); }

  action = HCI_get_init_options_action( indx );
  if( ( action != NULL ) && ( strlen(action) > 0 ) )
  {
    sprintf( func, "%s:MISC_system_to_buffer", HCI_orpg_machine_name() );
    HCI_LE_log( "HCI RPG init: %s %s", func, action );
    rpc_ret = RSS_rpc( func, "i-r s-i ba-%d-io i ia-io", HCI_BUF_2048,
                 &func_ret, action, output_buffer, HCI_BUF_2048, &n_bytes );
    func_ret = func_ret >> 8;

    if( rpc_ret != 0 )
    {
      HCI_LE_log( "RSS_rpc failed (%d). Unable to perform action.", rpc_ret );
      return -2;
    }
    else if( func_ret != 0 )
    {
      HCI_LE_log( "Action failed (%d).", func_ret );
      return -3;
    }

    if( HCI_get_init_options_msg(indx) != NULL )
    {
      HCI_LE_status_log("%s", HCI_get_init_options_msg(indx));
    }
  }

  return 0;
}

/************************************************************************
   Description: Returns the name of the first (dir_name != NULL)        *
                or the next (dir_name = NULL) file in directory         *
                "dir_name" whose name matches "basename".*. The caller  *
                provides the buffer "buf" of size "buf_size" for        *
                returning the file name.                                *
                                                                        *
   Inputs:      dir_name - directory name or NULL                       *
                basename - product table base name                      *
                buf - receiving buffer for next file name               *
                buf_size - size of receiving buffer                     *
   Outputs:     buf - contains next file name                           *
   Returns:     It returns 0 on success or -1 on failure.               *
   Notes:                                                               *
*************************************************************************/

static int Get_next_file_name( char *dir_name, char *basename,
                               char *buf, int buf_size )
{
  static DIR *dir = NULL;     /* the current open dir */
  static char saved_dirname[HCI_BUF_64] = "";
  struct dirent *dp = NULL;

  /* If directory is not NULL, open directory. */
  if( dir_name != NULL )
  {
    int len = strlen( dir_name );
    if( len+1 >= HCI_BUF_64 )
    {
      HCI_LE_error( "dir name (%s) does not fit in tmp buffer", dir_name );
      return -1;
    }
    strcpy( saved_dirname, dir_name );
    if( len == 0 || saved_dirname[len-1] != '/' )
    {
      strcat( saved_dirname, "/" );
    }
    if( dir != NULL ){ closedir( dir ); }
    dir = opendir( dir_name );
    if( dir == NULL ){ return -1; }
  }

  if( dir == NULL ){ return -1; }

  /* Read the directory. */
  while( ( dp = readdir( dir ) ) != NULL )
  {
    struct stat st;
    char fullpath[2*HCI_BUF_64];

    if( strncmp( basename, dp->d_name, strlen( basename ) ) != 0 )
    {
      continue;
    }
    if( strlen( dp->d_name ) >= HCI_BUF_64 )
    {
      HCI_LE_error( "file name (%s) does not fit in tmp buffer", dp->d_name );
      continue;
    }
    strcpy( fullpath, saved_dirname );
    strcat( fullpath, dp->d_name );
    if( stat( fullpath, &st ) < 0 )
    {
      HCI_LE_error( "stat (%s) failed, errno %d", fullpath, errno );
      continue;
    }
    /* If not a regular file, continue. */
    if( !( st.st_mode & S_IFREG ) ){ continue; }
    if( strlen( fullpath ) >= buf_size )
    {
      HCI_LE_error( "caller's buffer is too small (for %s)", fullpath );
      continue;
    }
      strcpy( buf, fullpath );
      return 0;
  }

  return -1;
}

/************************************************************************
 *      Description: This function initializes the Init_options struct. *
 *                                                                      *
 *      Input:  NONE                                                    *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    * 
 *      Notes:                                                          *
 ************************************************************************/

static void Init_rpg_options()
{
  char ext_name[HCI_BUF_128];
  char *call_name = NULL;
  char cfg_dir[HCI_BUF_128];
  char tmpbuf[HCI_BUF_256];
  int len = 0;

  Num_init_options = 0;

  /* Check to see if the configuration source (CFG_SRC) environmental
     variable is defined.  If so, make this the directory path to the
     various ASCII configuration files. */
  len = MISC_get_cfg_dir( cfg_dir, HCI_BUF_128 );
  if( len > 0 ){ strcat( cfg_dir, "/" ); }

  /* Process the master rpg_options file. */
  strcpy( tmpbuf, cfg_dir );
  strcat( tmpbuf, "rpg_options" );

  call_name = tmpbuf;
  Read_rpg_options_file( call_name );

  /* Process the configuration extensions. */
  strcpy( tmpbuf, cfg_dir );
  strcat( tmpbuf, "extensions" );

  call_name = tmpbuf;
  while( Get_next_file_name( call_name, "rpg_options",
                                   ext_name, HCI_BUF_128 ) == 0 )
  {
    HCI_LE_log( "---> Reading RPG Options Extension %s", ext_name );
    Read_rpg_options_file( ext_name );
    call_name = NULL;
  }

  Init_options_flag = HCI_YES_FLAG;
}

/************************************************************************
 *      Description: This function reads an rpg_options file            *
 *                                                                      *
 *      Input:  filename - file containing RPG Options                  *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    * 
 *      Notes:                                                          *
 ************************************************************************/

static void Read_rpg_options_file( char *filename )
{
  char name[HCI_BUF_64];
  char description[HCI_BUF_512];
  char action[HCI_BUF_512];
  char state[HCI_BUF_128];
  char permission[HCI_BUF_8];
  char group[HCI_BUF_64];
  char msg[HCI_BUF_128];
  char strtmp[16];

  CS_cfg_name( filename );
  CS_control( CS_COMMENT | '#' );

  while( Num_init_options < HCI_MAX_INIT_OPTIONS )
  {
    if( CS_entry( CS_NEXT_LINE, 0, NAME_SIZE, strtmp ) < 0 ){ break; }

    if( !strcmp( strtmp,"Task" ) )
    {
      if( CS_level( CS_DOWN_LEVEL ) < 0 )
      {
        CS_report( "unexpected line" );
        break;
      }
    }

    CS_entry( "name",        1, HCI_BUF_64, (void *) name );
    CS_entry( "description", 1, HCI_BUF_512, (void *)description );
    CS_entry( "action",      1, HCI_BUF_512, (void *)action );
    CS_entry( "state",       1, HCI_BUF_128, (void *)state );
    CS_entry( "permission",  1, HCI_BUF_8, (void *)permission );
    CS_entry( "group",       1, HCI_BUF_64, (void *)group );
    CS_entry( "msg",         1, HCI_BUF_128, (void *)msg );

    if( strlen( name ) )
    {
      Init_options[Num_init_options].name = calloc( strlen( name )+1, 1 );
      strncpy( Init_options[Num_init_options].name, name, strlen( name ) );
    }
    else
    {
      Init_options[Num_init_options].name = NULL;
    }

    if( strlen( description ) )
    {
      Init_options[Num_init_options].description = calloc( strlen( description )+1, 1 );
      strncpy( Init_options[Num_init_options].description, description, strlen( description ) );
    }
    else
    {
      Init_options[Num_init_options].description = NULL;
    }

    if( strlen( action ) )
    {
      Init_options[Num_init_options].action = calloc( strlen( action )+1, 1 );
      strncpy( Init_options[Num_init_options].action, action, strlen( action ) );
    }
    else
    {
      Init_options[Num_init_options].action = NULL;
    }

    if( strlen( state ) )
    {
      Init_options[Num_init_options].state = calloc( strlen( state )+1, 1 );
      strncpy( Init_options[Num_init_options].state, state, strlen( state ) );
    }
    else
    {
      Init_options[Num_init_options].state = NULL;
    }

    if( strlen( permission ) )
    {
      Init_options[Num_init_options].permission = calloc( strlen( permission )+1, 1 );
      strncpy( Init_options[Num_init_options].permission, permission, strlen( permission ) );
    }
    else
    {
      Init_options[Num_init_options].permission = NULL;
    }

    if( strlen( group ) )
    {
      Init_options[Num_init_options].group = calloc( strlen( group )+1, 1 );
      strncpy( Init_options[Num_init_options].group, group, strlen( group ) );
    }
    else
    {
      Init_options[Num_init_options].group = NULL;
    }

    if( strlen( msg ) )
    {
      Init_options[Num_init_options].msg = calloc( strlen( msg )+1, 1 );
      strncpy( Init_options[Num_init_options].msg, msg, strlen( msg ) );
    }
    else
    {
      Init_options[Num_init_options].msg = NULL;
    }

    Num_init_options++;

    CS_level( CS_UP_LEVEL );
  }

  /*  Reset CS so that sys_cfg is the current configuration file  
      In some situations, this must be reset. Eventually, the
      multiple fd versions of the CS api functions should be used
      so that this forced clear is not necessary
  */

  CS_cfg_name( "" );
}


