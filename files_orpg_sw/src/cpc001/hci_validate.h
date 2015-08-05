/************************************************************************
 *									*
 *	hci_validate.h - Header file defining functions used by		*
 *		the hci to validate various input.			*
 *									*
 ************************************************************************/

/* 
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:26:27 $
 * $Id: hci_validate.h,v 1.8 2009/02/27 22:26:27 ccalvert Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */  

#ifndef	HCI_VALIDATE_H
#define	HCI_VALIDATE_H

#include <Xm/Xm.h>
#include <Xm/Text.h>
#include <Xm/MessageB.h>

#define	ASCII_NEGATIVE		45
#define	ASCII_DECIMAL_POINT	46

void	hci_verify_text_callback( Widget, XtPointer, XtPointer );
void	hci_verify_float_callback( Widget, XtPointer, XtPointer );
void	hci_verify_unsigned_float_callback( Widget, XtPointer, XtPointer );
void	hci_verify_unsigned_integer_callback( Widget, XtPointer, XtPointer );
void	hci_verify_signed_integer_callback( Widget, XtPointer, XtPointer );
void	hci_verify_unsigned_list_callback( Widget, XtPointer, XtPointer );
void	hci_verify_date_callback( Widget, XtPointer, XtPointer );
void	hci_verify_month_callback( Widget, XtPointer, XtPointer );
void	hci_verify_day_callback( Widget, XtPointer, XtPointer );
void	hci_verify_year_callback( Widget, XtPointer, XtPointer );
void	hci_verify_time_callback( Widget, XtPointer, XtPointer );
void	hci_verify_hour_callback( Widget, XtPointer, XtPointer );
void	hci_verify_minute_callback( Widget, XtPointer, XtPointer );
void	hci_verify_second_callback( Widget, XtPointer, XtPointer );
int	hci_validate_date( int, int, int );
int	hci_string_in_string( char *, char * );
int	hci_number_found( char * );

#endif

