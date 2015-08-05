/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/04/29 14:26:24 $
 * $Id: hci_orda_pmd_functions.c,v 1.11 2013/04/29 14:26:24 steves Exp $
 * $Revision: 1.11 $
 * $State: Exp $
 */

/************************************************************************
 *                                                                      *
 *  Module:  hci_rda_performance_data_functions.c                       *
 *                                                                      *
 *  This file contains a collection of modules used to manipulate       *
 *  rda performance data.                                               *
 *                                                                      *
 ************************************************************************/

/*  Local include file definitions.          */

#include <hci.h>
#include <hci_orda_pmd.h>

/*  Global variables            */

static orda_pmd_t *Perf_data;   /* Performance data buffer */
static char Pdata[sizeof (orda_pmd_t)]; /* Performance data buffer */
static int Init_flag = 0;       /* initialization flag */
static int Update_flag = 0;     /* data update flag */
static hci_perfcheck_info_t Pc_info;

static void Find_perfcheck_info();

/************************************************************************
 *  Description: This function initializes RDA performance data.        *
 *                                                                      *
 *  Input:  NONE                                                        *
 *  Output: NONE                                                        *
 *  Return: status of read operation (error if not positive)            *
 ************************************************************************/

int hci_initialize_orda_pmd ()
{
  int len;

/*  This initialization routine simply calls the read function  */

  len = hci_read_orda_pmd ();

  Init_flag = 1;

  return len;
}

/************************************************************************
 *  Description: This function reads RDA performance data from the      *
 *         RDA Performance linear buffer and saves it in                *
 *         memory so other functions can access it.                     *
 *                                                                      *
 *  Input:  NONE                                                        *
 *  Output: NONE                                                        *
 *  Return: status of read operation (error if not positive)            *
 ************************************************************************/

int hci_read_orda_pmd ()
{
  int len;

  Perf_data = (orda_pmd_t *) Pdata;

  /*  Check to see if any messages exist in the RDA Performance  
      LB.  If they do, then read the contents of the last message 
      in the LB. */

  len = ORPGDA_seek (ORPGDAT_RDA_PERF_MAIN, 0, LB_LATEST, NULL);

  if (len == LB_SUCCESS)
  {

     len = ORPGDA_read (ORPGDAT_RDA_PERF_MAIN,
                         Pdata, sizeof (orda_pmd_t), LB_NEXT);

     if (len <= 0)
     {

        return len;

     }

  }
  else
  {

     return len;
  }

  return len;

}

/************************************************************************
 *  Description: This function returns a pointer to the start of        *
 *               ORDA performance data buffer.                           *
 *                                                                      *
 *  Input:       NONE                                                   *
 *  Output:      NONE                                                   *
 *  Return:      pointer to start of RDA performance data buffer        *
 ************************************************************************/

orda_pmd_t *hci_get_orda_pmd_ptr ()
{
  int len;

  if (!Init_flag || Update_flag)
  {

    len = hci_read_orda_pmd ();

  }

  return (orda_pmd_t *) Perf_data;
}

/*  The following command is used to send a request to the RDA  *
 *  for RDA performance data.          */

void hci_request_new_orda_pmd ()
{
  int status;
  char buf[80];

  sprintf (buf, "Requesting new RDA Performance Data");

  HCI_display_feedback( buf );

  status = ORPGRDA_send_cmd (COM4_REQRDADATA,
                             (int) HCI_INITIATED_RDA_CTRL_CMD,
                             DREQ_PERFMAINT,
                             (int) 0, (int) 0, (int) 0, (int) 0, NULL);

/*  Right now nothing is done if the request to send the command  *
 *  is unsuccessfull (status < 0).          */

}

/************************************************************************
 *  Description: This function returns the value of the specified       *
 *         RDA performance data item.                                   *
 *         NOTE: At this time only generator fuel level data            *
 *         are supported by this function.                              *
 *                                                                      *
 *  Input:  item - ID of data item to extract from buffer               *
 *  Output: NONE                                                        *
 *  Return: data value                                                  *
 ************************************************************************/

int hci_orda_pmd (int item)
{
  int num;
  int len;

  if (!Init_flag || Update_flag)
  {

    len = hci_read_orda_pmd ();

  }

  switch (item)
  {

  case CNVRTD_GEN_FUEL_LEVEL:

    num = Perf_data->pmd.cnvrtd_gnrtr_fuel_lvl;
    break;

  case PERF_CHECK_TIME:

    num = (int) (Perf_data->pmd.perf_check_time);
    break;

  default:

    num = -1;
    break;

  }

  return num;

}

/************************************************************************
 *  Description: This function sets the value of the RDA                *
 *         performance data update flag to the specified                *
 *         value.                                                       *
 *                                                                      *
 *  Input:  state - new update state (should be 0 for no update and     *
 *      non-zero for need update).                                      *
 *  Output: NONE                                                        *
 *  Return: NONE                                                        *
 ************************************************************************/

void hci_set_orda_pmd_update_flag (int state)
{
  Update_flag = state;
}

/************************************************************************
 *  Description: This function gets the value of the RDA                *
 *         performance data update flag.                                *
 *                                                                      *
 *  Input:  NONE                                                        *
 *  Output: NONE                                                        *
 *  Return: update flag (!0 - needs update; 0 - no update)              *
 ************************************************************************/

int hci_get_orda_pmd_update_flag ()
{
  return Update_flag;
}

/************************************************************************
 *  Description: This function returns the value of the RDA             *
 *         performance data initialization flag.  A 0 means             *
 *         that the LB contains no messages.  1 means data              *
 *         has been successfully read and the structure                 *
 *         properly initialized.                                        *
 *                                                                      *
 *  Input:  NONE                                                        *
 *  Output: NONE                                                        *
 *  Return: initialization flag                                         *
 ************************************************************************/

int hci_orda_pmd_initialized ()
{
  return Init_flag;
}

/************************************************************************
 *  Description: This function returns a pointer to a string with       *
 *         the date/time of the latest RDA Performance data             *
 *         message.                                                     *
 *                                                                      *
 *  Input:  NONE                                                        *
 *  Output: NONE                                                        *
 *  Return: pointer to date/time string.                                *
 ************************************************************************/

char *hci_orda_pmd_time ()
{
  static char string[32];
  int month, day, year;
  int hour, minute, second;
  long int seconds;

  if (Init_flag)
  {

    seconds = (Perf_data->msg_hdr.julian_date - 1) * HCI_SECONDS_PER_DAY +
      Perf_data->msg_hdr.milliseconds / HCI_MILLISECONDS_PER_SECOND;

    unix_time (&seconds, &year, &month, &day, &hour, &minute, &second);

    sprintf (string, "%3s %2d,%4d - %2.2d:%2.2d:%2.2d UT",
             HCI_get_month(month), day, year, hour, minute, second);

    return (char *) string;

  }
  else
  {

    return (char *) NULL;

  }
}

static void Find_perfcheck_info()
{
  int diff_time = 0;

  if( ( ORPGRDA_get_status( RS_PERF_CHECK_STATUS ) & 1 ) )
  {
    Pc_info.state = HCI_PC_STATE_PENDING;
    Pc_info.status = HCI_PC_STATUS_PENDING;
    Pc_info.num_hrs = 0;
    Pc_info.num_mins = 0;
  }
  else
  {
    Pc_info.state = HCI_PC_STATE_AUTO;
    if( ( diff_time = hci_orda_pmd( PERF_CHECK_TIME ) - time( NULL ) ) <= 0 )
    {
      Pc_info.status = HCI_PC_STATUS_PENDING;
      diff_time = 0;
    }
    else
    {
      Pc_info.status = HCI_PC_STATUS_NONPENDING;
    }
    Pc_info.num_hrs = diff_time / 3600;
    Pc_info.num_mins = ( diff_time % 3600 ) / 60;
  }
}

hci_perfcheck_info_t *hci_get_perfcheck_info()
{
  if( !Init_flag )
  {
    hci_initialize_orda_pmd();
  }

  Find_perfcheck_info();

  return &Pc_info;
}

