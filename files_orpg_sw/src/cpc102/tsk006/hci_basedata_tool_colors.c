/************************************************************************
 *									*
 *	Module:	hci_basedata_tool_colors.c				*
 *									*
 *	Description: This module is used to initialize and maintain	*
 *		     a set of read only colors to be used for		*
 *		     displaying in the Base Data Display Tool.		*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:31:08 $
 * $Id: hci_basedata_tool_colors.c,v 1.2 2009/02/27 22:31:08 ccalvert Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/*	Local include file definitions					*/

#include <hci.h>
#include <hci_basedata_tool.h>

/************************************************************************
 *	Description: This function assigns a group of colors to the	*
 *		     specified product.					*
 *									*
 *	Input:  pcode   - product code of product			*
 *	Return: None							*
 ************************************************************************/

void hci_basedata_tool_get_product_colors( int pcode, int *pcolor )
{
  /* Special color-scale for velocity. */
  if( pcode == 25 )
  {
    pcolor  [0] = hci_get_read_color (BLACK);
    pcolor  [1] = hci_get_read_color (LIGHTGREEN);
    pcolor  [2] = hci_get_read_color (GREEN1);
    pcolor  [3] = hci_get_read_color (GREEN2);
    pcolor  [4] = hci_get_read_color (GREEN3);
    pcolor  [5] = hci_get_read_color (GREEN4);
    pcolor  [6] = hci_get_read_color (DARKGREEN);
    pcolor  [7] = hci_get_read_color (LIGHTGRAY);
    pcolor  [8] = hci_get_read_color (DARKGRAY);
    pcolor  [9] = hci_get_read_color (RED4);
    pcolor  [10] = hci_get_read_color (RED3);
    pcolor  [11] = hci_get_read_color (RED2);
    pcolor  [12] = hci_get_read_color (RED1);
    pcolor  [13] = hci_get_read_color (INDIANRED1);
    pcolor  [14] = hci_get_read_color (PINK);
    pcolor  [15] = hci_get_read_color (PURPLE);
  }
  /* Special color-scale for spectrum width. */
  else if( pcode == 28 )
  {
    pcolor  [0] = hci_get_read_color (BLACK);
    pcolor  [1] = hci_get_read_color (CYAN);
    pcolor  [2] = hci_get_read_color (CYAN);
    pcolor  [3] = hci_get_read_color (GREEN);
    pcolor  [4] = hci_get_read_color (GREEN);
    pcolor  [5] = hci_get_read_color (SEAGREEN);
    pcolor  [6] = hci_get_read_color (SEAGREEN);
    pcolor  [7] = hci_get_read_color (YELLOW);
    pcolor  [8] = hci_get_read_color (YELLOW);
    pcolor  [9] = hci_get_read_color (ORANGE);
    pcolor  [10] = hci_get_read_color (ORANGE);
    pcolor  [11] = hci_get_read_color (RED);
    pcolor  [12] = hci_get_read_color (RED);
    pcolor  [13] = hci_get_read_color (WHITE);
    pcolor  [14] = hci_get_read_color (WHITE);
    pcolor  [15] = hci_get_read_color (PURPLE);
  }
  /* All other data. */
  else
  {
    pcolor  [0] = hci_get_read_color (BLACK);
    pcolor  [1] = hci_get_read_color (CYAN);
    pcolor  [2] = hci_get_read_color (STEELBLUE);
    pcolor  [3] = hci_get_read_color (BLUE);
    pcolor  [4] = hci_get_read_color (GREEN);
    pcolor  [5] = hci_get_read_color (GREEN3);
    pcolor  [6] = hci_get_read_color (SEAGREEN);
    pcolor  [7] = hci_get_read_color (YELLOW);
    pcolor  [8] = hci_get_read_color (GOLD);
    pcolor  [9] = hci_get_read_color (ORANGE);
    pcolor  [10] = hci_get_read_color (RED);
    pcolor  [11] = hci_get_read_color (RED3);
    pcolor  [12] = hci_get_read_color (RED4);
    pcolor  [13] = hci_get_read_color (MAGENTA);
    pcolor  [14] = hci_get_read_color (MAGENTA3);
    pcolor  [15] = hci_get_read_color (WHITE);
  }
}
