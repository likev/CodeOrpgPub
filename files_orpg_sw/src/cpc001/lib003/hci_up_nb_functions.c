/*
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2011/10/08 19:26:30 $
 * $Id: hci_up_nb_functions.c,v 1.23 2011/10/08 19:26:30 jing Exp $
 * $Revision: 1.23 $
 * $State: Exp $
 */

/************************************************************************
 *									*
 *	hci_up_nb_functions.c - This file contains modules used to	*
 *		retrieve various user profile and narrowband status	*
 *		information.						*
 *									*
 ************************************************************************/

/* Local include file definitions. */

#include <hci.h>
#include <hci_up_nb.h>

static int hci_up_nb_set_db_server_addr();
static int nb_status = NB_HAS_NO_CONNECTIONS;
static int nb_init = HCI_NO_FLAG;

/**********************************************************************

    Description: Returns the detailed info about line "ls".

    Input:  ls - pointer to line status data
    Output: NONE
    Return: Pointer to the Line_details structure.

***********************************************************************/

Line_details *hci_up_nb_get_line_details( Line_status *ls )
{
  int ret = -1;
  int len = -1;
  char buf[128];
  char sqlbuf[128];
  char *lb_name = NULL;
  char *msg_buf = NULL;
  void *qr = NULL; /* query results */
  Pd_user_entry *pd_line_user = NULL;
  Pd_user_entry *pd_user = NULL;
  RPG_up_rec_t *rec = NULL;
  static Line_details *ld = NULL;

  if( ld != NULL )
  {
    free( ld );
    ld = (Line_details *) NULL;
  }

  ld = (Line_details *) MISC_malloc( sizeof( Line_details ) );
  memset ((char *)ld, 0, sizeof (Line_details));

  if( ls->line_num <= 0 ){ return ld; }

  /* Set selected line details data using line status data. */

  ld->line_num = ls->line_num;
  ld->type     = ls->type;
  ld->protocol = ls->protocol;
  ld->uclass   = ls->uclass;
  ld->user_id  = ls->user_id;

  /* First, lets query all "Line_users" for a match for the current
     line index.  We can get the class number and also a name (if
     one is defined. */

  if( hci_up_nb_set_db_server_addr() >= 0 )
  {
    sprintf( sqlbuf, "up_type = %d", UP_LINE_USER );
    sprintf( sqlbuf+strlen( sqlbuf ), " and line_ind = %d", (int) ls->line_num );
    if( ( lb_name = ORPGDA_lbname( ORPGDAT_USER_PROFILES ) ) == NULL ||
        ( ret = SDQ_select( lb_name, sqlbuf, (void **)&qr) ) < 0 )
    {
      HCI_LE_error( "SDQ_select (%s) failed, ret %d", sqlbuf, ret );
    }
    else
    {
      if( SDQ_get_n_records_returned( qr ) > 0 )
      {
        SDQ_get_query_record( qr, 0, (void **)&rec );
        len = ORPGDA_read( ORPGDAT_USER_PROFILES,
                           &msg_buf, LB_ALLOC_BUF, rec->msg_id);

        if( len <= 0 )
        {
          free( msg_buf );
          msg_buf = (char *) NULL;
          HCI_LE_error( "ORPGDA_read(ORPGDAT_USER_PROFILES %d) DEDICATED failed (%d)",
                        rec->msg_id, len );
        }
        else
        {
          pd_line_user = (Pd_user_entry *) msg_buf;
          strcpy( ld->port_pswd, pd_line_user->user_password );
        }
      }
     }
    }
    free( qr );
    qr = (void *) NULL;

    /* If we can determine who is connected to the line, we need
       to query the user profile database to find the name of the
       dedicated or dial user. */

    if( ( ld->user_id >= 0 ) &&
	( ( ls->type == DEDICATED ) ||
          ( ls->type == DIAL_IN ) ||
          ( ls->type == WAN_LINE ) ) )
    {
      if( hci_up_nb_set_db_server_addr() >= 0 )
      {
        sprintf( buf, "user_id = %d", (int) ld->user_id );
        if( ls->type == DEDICATED )
        {
          sprintf( buf+strlen( buf ), " and up_type = %d", UP_DEDICATED_USER );
        }
        else if( ls->type == DIAL_IN || ls->type == WAN_LINE )
        {
          sprintf( buf+strlen( buf ), " and up_type = %d", UP_DIAL_USER );
        }

        if( ( lb_name = ORPGDA_lbname( ORPGDAT_USER_PROFILES ) ) == NULL ||
            ( ret = SDQ_select( lb_name, buf, (void **)&qr ) ) < 0 )
        {
          HCI_LE_error( "SDQ_select (%s) failed, ret %d", buf, ret );
        }
        else
        {
          if( SDQ_get_n_records_returned( qr ) > 0 )
          {
            SDQ_get_query_record( qr, 0, (void **)&rec );
            len = ORPGDA_read( ORPGDAT_USER_PROFILES,
                               &msg_buf, LB_ALLOC_BUF, rec->msg_id );

            if( len <= 0 )
            {
              free( msg_buf );
              msg_buf = (char *) NULL;
              HCI_LE_error( "ORPGDA_read (ORPGDAT_USER_PROFILES %d) DEDICATED failed (%d)",
                            rec->msg_id, len);
            }
            else
            {
              pd_user = (Pd_user_entry *) msg_buf;
            }
          }
        }
      }
    }

    free( qr );
    qr = (void *) NULL;

    /* If a line user record was found, set the class, user_id, and
       distribution method from the line user record.  If a user ID
       was determined, then get the name from the dedicated or dial
       user record.  Otherwise, default to the name associated with
       the line (if it exists). */

    if( pd_line_user != NULL )
    {
      if( ld->user_id >= 0 )
      {
        if( pd_user != NULL )
        {
          strcpy( ld->user_name, pd_user->user_name );
        }
        else
        {
          strcpy( ld->user_name, pd_line_user->user_name );
        }
      }
      else
      {
        strcpy( ld->user_name, pd_line_user->user_name );
      }
      ld->distri_method = pd_line_user->distri_method;
      ld->uclass        = pd_line_user->class;
      ld->defined       = pd_line_user->defined;
    }
    else
    {
      strcpy( ld->user_name, "" );
      ld->distri_method =  0;
      ld->uclass        =  0;
      ld->user_id       = -1;
    }

    if (pd_line_user != NULL)
      free( pd_line_user );
    pd_line_user = (Pd_user_entry *) NULL;
    if (pd_user != NULL)
      free( pd_user );
    pd_user = (Pd_user_entry *) NULL;

    return ld;
}

/******************************************************************

    Description: Sets the user profile data base server address.
	    The address needs to be set only once.

    Input:  NONE
    Output: NONE
    Return: 0 on success or -1 on failure.

******************************************************************/

static int hci_up_nb_set_db_server_addr()
{
  return 0;
}

/******************************************************************

    Description: This function reads the user profile database and
		 builds a table of all dial-in users.  The argument
		 "dial" contains the table.  The function returns
		 the number of table items

    Input:  none
    Output: structure containing dial users table
    Return: number of dial users

******************************************************************/

int hci_up_nb_update_dial_user_table( void *out )
{
  int i = -1;
  int j = -1;
  int ret = 0;
  int users = 0;
  int cnt = 0;
  int defined = -1;
  int temp = -1;
  int len = -1;
  char buf[128];
  char *lb_name = NULL;
  char *msg_buf = NULL;
  void *qr = NULL; /* query results */
  Dial_details *dial = NULL;
  Dial_details tmp_dial;
  RPG_up_rec_t *rec = NULL;
  Pd_user_entry *up = NULL;
  static Dial_details_tbl_t *dial_tbl = NULL;

  if( hci_up_nb_set_db_server_addr() >= 0 )
  {
    /* First lets query the user profile data base for all dial
       users so we can build a table of users. */

    SDQ_set_query_mode( SDQM_LOWEND_SEARCH );
    sprintf( buf, "up_type = %d", UP_DIAL_USER );
    sprintf( buf+strlen( buf ), 
             " and user_id >= %d and user_id <= %d", 0, 32767 );
    if( ( lb_name = ORPGDA_lbname( ORPGDAT_USER_PROFILES ) ) == NULL ||
        ( ret = SDQ_select( lb_name, buf, (void **)&qr ) ) < 0 )
    {
      HCI_LE_error( "SDQ_select (%s) failed, ret %d", buf, ret );
    }
    else
    {
      users = SDQ_get_n_records_returned( qr );

      /* Malloc memory for the dial table.  If the initial
         value is not NULL, free it first. */

      if( dial_tbl != NULL )
      {
        free( dial_tbl );
        dial_tbl = (Dial_details_tbl_t *) NULL;
      }

      temp = sizeof( Dial_details_tbl_t ) + ( users * sizeof( Dial_details ) );
      dial_tbl = (Dial_details_tbl_t *) MISC_malloc( temp );
      *((Dial_details_tbl_t **)out) = dial_tbl;
      dial_tbl->data = (Dial_details *) ( (char *) dial_tbl + sizeof( Dial_details_tbl_t ) );;
      dial = (Dial_details *) dial_tbl->data;

      cnt = 0;

      for( i = 0; i < users; i++ )
      {
        SDQ_get_query_record( qr, i, (void **)&rec );
        len = ORPGDA_read( ORPGDAT_USER_PROFILES,
                           &msg_buf, LB_ALLOC_BUF, rec->msg_id );

        if( len <= 0 )
        {
          HCI_LE_error( "ORPGDA_read (ORPGDAT_USER_PROFILES %d) UP_DIAL_USER failed (%d)",
          rec->msg_id, len );
        }
        else
        {
          up = (Pd_user_entry *) msg_buf;

          /* If the dial user already defined, ignore the new one. */

          defined = 0;

          for( j = 0; j < i; j++ )
          {
            if( dial[j].user_id == up->user_id )
            {
              defined = 1;
              break;
            }
          }

          if( !defined )
          {
            dial[cnt].msg_id           = rec->msg_id;
            dial[cnt].max_connect_time = up->max_connect_time;
            dial[cnt].defined          = up->defined;
            dial[cnt].class            = up->class;
            dial[cnt].distri_method    = up->distri_method;
            dial[cnt].user_id          = up->user_id;
            dial[cnt].delete_flag      = 0;

            strcpy( dial[cnt].user_name, up->user_name );
            strcpy( dial[cnt].user_password, up->user_password );

            if( up->defined & UP_CD_OVERRIDE )
            {
              if( up->cntl & UP_CD_OVERRIDE )
              {
                dial[cnt].disconnect_override = 1;
              }
              else
              {
                dial[cnt].disconnect_override = 0;
              }
            }
            else
            {
              dial[cnt].disconnect_override = 0;
            }
            cnt++;
          }
        }
        free( msg_buf );
        msg_buf = (char *) NULL;
      }

      /* Lets sort the records by user ID for consistency.
         When we update a record in the table, the database
         manager updates its internal tables by moving the
         updated record to the bottom of its list.  So when
         we reread the table, the list order is different.
         Without sorting this would probably be a problem to
         the user. */

      users = cnt;

      for( i = 0; i < users-1; i++ )
      {
        for( j = i+1; j < users; j++ )
        {
          if( dial[j].user_id < dial[i].user_id )
          {
            memcpy( &tmp_dial,&dial[i], sizeof( Dial_details ) );
            memcpy( &dial[i], &dial[j], sizeof( Dial_details ) );
            memcpy( &dial[j], &tmp_dial,sizeof( Dial_details ) );
          }
        }
      }

      dial_tbl->sizeof_data = users;
    }
  }

  if (qr != NULL)
    free( qr );
  qr = (void *) NULL;
  if (users == 0) {
      if( dial_tbl != NULL )
        free( dial_tbl );
      dial_tbl = MISC_malloc( sizeof( Dial_details_tbl_t ) );
      dial_tbl->sizeof_data = 0;
      *((Dial_details_tbl_t **)out) = dial_tbl;
  }

  return users;
}

/******************************************************************

    Description: This function reads the user profile database and
		 builds a table of all dedicated users.  The argument
		 "user" contains the table.  The function returns
		 the number of table items.

    Input:  none
    Output: structure containing dedicated users table
    Return: number of dedicated users

******************************************************************/

int hci_up_nb_update_dedicated_user_table( void *out )
{
  int i = -1;
  int j = -1;
  int ret = 0;
  int users = 0;
  int cnt = 0;
  int defined = -1;
  int temp = -1;
  int len = -1;
  char buf[128];
  char *lb_name = NULL;
  char *msg_buf = NULL;
  void *qr = NULL; /* query results */
  Dial_details *user = NULL;
  Dial_details tmp_user;
  RPG_up_rec_t *rec = NULL;
  Pd_user_entry *up = NULL;
  static Dial_details_tbl_t *user_tbl = NULL;

  if( hci_up_nb_set_db_server_addr() >= 0 )
  {
    /* First lets query the user profile data base for all dial
       users so we can build a table of users. */

    SDQ_set_query_mode( SDQM_LOWEND_SEARCH );
    sprintf( buf, "up_type = %d", UP_DEDICATED_USER );
    sprintf( buf+strlen( buf ), 
             " and user_id >= %d and user_id <= %d", 0, 32767);
    if( ( lb_name = ORPGDA_lbname( ORPGDAT_USER_PROFILES ) ) == NULL ||
        ( ret = SDQ_select( lb_name, buf, (void **)&qr ) ) < 0 )
    {
      HCI_LE_error( "SDQ_select (%s) failed (%d)", buf, ret );
    }
    else
    {
      users = SDQ_get_n_records_returned( qr );

      /* Malloc memory for the users table.  If the initial
         value is not NULL, free it first. */

      if( user_tbl != NULL )
      {
        free( user_tbl );
        user_tbl = (Dial_details_tbl_t *) NULL;
      }

      temp = sizeof( Dial_details_tbl_t ) + ( users * sizeof( Dial_details ) );
      user_tbl = (Dial_details_tbl_t *) MISC_malloc( temp );
      *((Dial_details_tbl_t **)out) = user_tbl;
      user_tbl->data = (Dial_details *) ( (char *) user_tbl + sizeof( Dial_details_tbl_t ) );
      user = (Dial_details *) user_tbl->data;

      cnt = 0;

      for( i = 0; i < users; i++ )
      {
        SDQ_get_query_record( qr, i, (void **)&rec );
        len = ORPGDA_read( ORPGDAT_USER_PROFILES,
                           &msg_buf, LB_ALLOC_BUF, rec->msg_id );

        if( len <= 0 )
        {
          HCI_LE_error( "ORPGDA_read (ORPGDAT_USER_PROFILES %d) UP_DEDICATED_USER failed (%d)",
          rec->msg_id, len );
        }
        else
        {
          up = (Pd_user_entry *) msg_buf;

          /* If the class/method already defined, ignore the new one. */

          defined = 0;

          for( j = 0; j < i; j++ )
          {
            if( user[j].user_id == up->user_id )
            {
              defined = 1;
              break;
            }
          }

          if( !defined )
          {
            user[cnt].msg_id           = rec->msg_id;
            user[cnt].max_connect_time = up->max_connect_time;
            user[cnt].defined          = up->defined;
            user[cnt].class            = up->class;
            user[cnt].distri_method    = up->distri_method;
            user[cnt].user_id          = up->user_id;
            user[cnt].delete_flag      = 0;

            strcpy( user[cnt].user_name, up->user_name );
            strcpy( user[cnt].user_password, up->user_password );

            if( up->defined & UP_CD_OVERRIDE )
            {
              if( up->cntl & UP_CD_OVERRIDE )
              {
                user[cnt].disconnect_override = 1;
              }
              else
              {
                user[cnt].disconnect_override = 0;
              }
            }
            else
            {
              user[cnt].disconnect_override = 0;
            }
            cnt++;
          }
        }
        free( msg_buf );
        msg_buf = (char *) NULL;
      }

      /* Lets sort the records by user name for consistency.
         When we update a record in the table, the database
         manager updates its internal tables by moving the
         updated record to the bottom of its list.  So when
         we reread the table, the list order is different.
         Without sorting this would probably be a problem to
         the user. */

      users = cnt;

      for( i = 0; i < users-1; i++ )
      {
        for( j = i+1; j < users; j++ )
        {
          if( strcasecmp( user[i].user_name,user[j].user_name ) > 0 )
          {
            memcpy( &tmp_user,&user[i], sizeof( Dial_details ) );
            memcpy( &user[i], &user[j], sizeof( Dial_details ) );
            memcpy( &user[j], &tmp_user,sizeof( Dial_details ) );
          }
        }
      }

      user_tbl->sizeof_data = users;
    }
  }

  if (qr != NULL)
    free( qr );
  qr = (void *) NULL;
  if (users == 0) {
      if( user_tbl != NULL )
        free( user_tbl );
      user_tbl = MISC_malloc( sizeof( Dial_details_tbl_t ) );
      user_tbl->sizeof_data = 0;
      *((Dial_details_tbl_t **)out) = user_tbl;
  }

  return users;
}

/******************************************************************

    Description: This function queries the user_profiles database
		 and returns a table of all defined classes.  A class
		 may be defined more than once for multiple
		 distribution methods.  The table is returned using
		 the "class" argument.  The function returns the number
		 of class entries found.

    Input:  none
    Output: structure containing class table
    Return: number of classes

******************************************************************/

int hci_up_nb_update_class_table( void *out )
{
  int i = -1;
  int j = -1;
  int ret = 0;
  int classes = 0;
  int cnt = 0;
  int defined = -1;
  int temp = -1;
  int len = -1;
  char buf[128];
  char *lb_name = NULL;
  char *msg_buf = NULL;
  void *qr = NULL; /* query results */
  Class_details *class = NULL;
  Class_details temp_class;
  RPG_up_rec_t *rec = NULL;
  Pd_user_entry *up = NULL;
  static Class_details_tbl_t *class_tbl = NULL;

  if( hci_up_nb_set_db_server_addr() >= 0 )
  {
    /* Lets query the user profile data base for all class
       definitions and associated methods so we can use this info
       to build a list of distribution methods. */


    SDQ_set_query_mode( SDQM_LOWEND_SEARCH );
    sprintf( buf, "up_type = %d", UP_CLASS );
    sprintf( buf+strlen( buf ), 
             " and class_num >= %d and class_num <= %d", 1, 32767 );
    if( ( lb_name = ORPGDA_lbname( ORPGDAT_USER_PROFILES ) ) == NULL ||
        ( ret = SDQ_select( lb_name, buf, (void **)&qr ) ) < 0 )
    {
      HCI_LE_error( "SDQ_select (%s) failed (%d)", buf, ret );
    }
    else
    {
      classes = SDQ_get_n_records_returned( qr );
      cnt = 0;

      /* Malloc memory for the class table.  If the initial
         value is not NULL, free it first. */

      if( class_tbl != NULL )
      {
        free( class_tbl );
        class_tbl = (Class_details_tbl_t *) NULL;
      }

      temp = ( sizeof( Class_details_tbl_t )+(classes*(sizeof(Class_details))));
      class_tbl = (Class_details_tbl_t *) MISC_malloc( temp );
      *((Class_details_tbl_t **)out) = class_tbl;
      class_tbl->data = (Class_details *) ( (char *) class_tbl + sizeof( Class_details_tbl_t ) );
      class = (Class_details *) class_tbl->data;

      for( i = 0; i < classes; i++ )
      {
        SDQ_get_query_record( qr, i, (void **)&rec );
        len = ORPGDA_read( ORPGDAT_USER_PROFILES,
                           &msg_buf, LB_ALLOC_BUF, rec->msg_id );

        if( len <= 0 )
        {
          HCI_LE_error( "ORPGDA_read (ORPGDAT_USER_PROFILES %d) CLASS failed (%d)",
          rec->msg_id, len );
        }
        else
        {
          up = (Pd_user_entry *) msg_buf;

          /* If the class/method already defined, ignore the new one. */

          defined = 0;

          for( j = 0; j < i; j++ )
          {
            if( ( class[j].class == up->class ) &&
                ( class[j].distri_method == up->distri_method ) )
            {
              defined = 1;
              break;
            }
          }

          if( !defined )
          {
            memset( class[cnt].name,0,USER_NAME_LEN );
            class[cnt].class         = up->class;
            class[cnt].line_ind      = up->line_ind;
            class[cnt].distri_method = up->distri_method;
            class[cnt].defined       = up->defined;
            class[cnt].max_connect_time = up->max_connect_time;
            strncpy( class[cnt].name, up->user_name, strlen( up->user_name ) );
            cnt++;
          }
        }
        free( msg_buf );
        msg_buf = (char *) NULL;
      }

      /* Sort the class data in ascending order. */

      if( class != NULL )
      {
        for( i = 1; i < cnt; i++ )
        {
          for( j = 0; j < i; j++ )
          {
            if( class[i].class < class[j].class )
            {
              temp_class = class[j];
              class[j] = class[i];
              class[i] = temp_class;
            }
          }
        }
      }

      class_tbl->sizeof_data = cnt;
    }
  }

  if (qr != NULL)
    free( qr );
  qr = (void *) NULL;
  if (cnt <= 0) {
      if( class_tbl != NULL )
        free( class_tbl );
      class_tbl = MISC_malloc( sizeof( Class_details_tbl_t ) );
      class_tbl->sizeof_data = 0;
      *((Class_details_tbl_t **)out) = class_tbl;
  }

  return cnt;
}

/************************************************************
 * Description: Read various LBs to determine status of
 *    narrowband lines.
 ************************************************************/

void hci_read_nb_connection_status()
{
  int ret = -1;
  int i = -1;
  int wideband_line_index = -1;
  int num_nb_lines = -1;
  int nb_line_index = -1;
  int return_code = NB_HAS_NO_CONNECTIONS;
  Hci_pd_block_t *Pd_block = NULL;

  /* Read product generation/distribution info LB. If read fails or
     the number of bytes read indicates invalid data, then assume
     line failure. */

  ret = ORPGDA_read( ORPGDAT_HCI_DATA,
                     &Pd_block, LB_ALLOC_BUF, HCI_PROD_INFO_STATUS_MSG_ID );

  if( ret < 0 )
  {
    HCI_LE_error("ORPGDA_read(ORPGDAT_HCI_DATA) failed (%d)", ret );
    free( Pd_block );
    Pd_block = (Hci_pd_block_t *) NULL;
    nb_status = NB_HAS_FAILED_CONNECTIONS;
    return;
  }
  else if( ret < sizeof( Pd_block ) )
  {
    HCI_LE_error("Bad Pd_block msg size (%d)", ret );
    free( Pd_block );
    Pd_block = (Hci_pd_block_t *) NULL;
    nb_status = NB_HAS_FAILED_CONNECTIONS;
    return;
  }

  /* Set number of narrowband lines. Loop through each line (except
     wideband line) and determine status. */
     
  num_nb_lines = Pd_block->pd_info.n_lines;
  wideband_line_index = ORPGCMI_rda_response() - ORPGDAT_CM_RESPONSE;

  for( i = 0; i < num_nb_lines; i++ )
  {
    nb_line_index = Pd_block->pd_line_info[i].line_ind;

    if( nb_line_index == wideband_line_index || nb_line_index < 0 )
    {
      continue;
    }

    if( Pd_block->pd_user_status[i].line_stat == US_LINE_FAILED )
    {
      return_code = NB_HAS_FAILED_CONNECTIONS;
      break;
    }

    /* If the line is connected, set flag. */ 

    if( Pd_block->pd_user_status[i].link == US_CONNECTED )
    {
      return_code = NB_HAS_CONNECTIONS;
    }
  }

  /* Cleanup. */

  free( Pd_block );
  Pd_block = (Hci_pd_block_t *) NULL;

  /* Set status. */

  nb_status = return_code;
}

/************************************************************
 * Description: Read various LBs to determine status of
 *    narrowband lines.
 ************************************************************/

int hci_get_nb_connection_status()
{
  if( nb_init == HCI_NO_FLAG )
  {
    nb_init = HCI_YES_FLAG;
    hci_read_nb_connection_status();
  }

  return nb_status;
}
