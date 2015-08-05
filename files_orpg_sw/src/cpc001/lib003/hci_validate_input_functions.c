/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2010/03/10 18:46:58 $
 * $Id: hci_validate_input_functions.c,v 1.30 2010/03/10 18:46:58 ccalvert Exp $
 * $Revision: 1.30 $
 * $State: Exp $
 */

/************************************************************************
 *									*
 *	Module:  hci_validate_input_functions.c				*
 *									*
 *	Description:  This module contains a collection of routines	*
 *		      used to verify specific inputs in the ORPG HCI.	*
 *									*
 ************************************************************************/

/*	Local include file definitions.					*/

#include <hci.h>
#include <hci_validate.h>

/*	Macros.								*/

#define	MAX_FLOAT_STRING	9 /* Max length for an input float string
				     if one isn't specified. */
#define	MAX_INT_STRING		7 /* Max length for an input integer string
				     if one isn't specified. */

char	buf [HCI_BUF_128]; /* local buffer for string manipulation */

/************************************************************************
 *	Description: This function should be used for text widgets	*
 *		     XmNmodifyVerifyCallback if used for text data.	*
 *									*
 *	Input:  w - widget ID of text widget				*
 *		client_data - max length of string (NULL uses default)	*
 *		call_data - text widget data				*
 *									*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_verify_text_callback (
Widget		parent_widget,
XtPointer	client_data,
XtPointer	call_data
)
{
	char	*text;

	XmTextVerifyCallbackStruct	*cbs =
		(XmTextVerifyCallbackStruct *) call_data;

	if (cbs->text->ptr == NULL)
		return;

	if (cbs->text->length == 0)	/* Handle Backspace	*/
		return;

	text = XmTextGetString (parent_widget);

/*	Let's limit the number of elements in the string.  This	*
 *	prevents the user from overloading the item.		*/

	if (client_data != NULL) {

	    int	len;
	    int	i;

	    len = strlen (text);

/*	    We do not want to count any blanks in the string	*
 *	    since they are probably there as filler.		*/

	    for (i=0;i<len;i++) {

		if (!strncmp ((text+i)," ",1)) {

		    len--;

		}
	    }

	    if (len >= (int) client_data) {

		char	*selection;

		selection = XmTextGetSelection (parent_widget);

/*		If any text is highlighed then we want to ignore it.	*/

		if (selection != NULL) {

		    if ((len-strlen(selection)) >= (int) client_data) {

			cbs->text->length--;

			if (cbs->text->length == 0) {

			    cbs->doit = False;

			}
			XtFree (selection);
			XtFree (text);
			return;

		    }

		    XtFree (selection);

		} else if (cbs->startPos == cbs->endPos)  {

		    cbs->text->length--;

		    if (cbs->text->length == 0) {

			cbs->doit = False;

		    }
		}

		XtFree (text);
		return;

	    }
	}

	XtFree (text);
}

/************************************************************************
 *	Description: This function should be used for text widgets	*
 *		     XmNmodifyVerifyCallback if used for unsigned	*
 *		     real numbers.					*
 *									*
 *	Input:  w - widget ID of text widget				*
 *		client_data - max length of string (NULL uses default)	*
 *		call_data - text widget data				*
 *									*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_verify_unsigned_float_callback (
Widget		parent_widget,
XtPointer	client_data,
XtPointer	call_data
)
{
	int	len;
	int	decimal_flag;
	char	*text;

	XmTextVerifyCallbackStruct	*cbs =
		(XmTextVerifyCallbackStruct *) call_data;

	if (cbs->text->ptr == NULL)
		return;

	if (cbs->text->length == 0)	/* Handle Backspace	*/
		return;

	text = XmTextGetString (parent_widget);

/*	Let's limit the number of elements in the string.  This	*
 *	prevents the user from overloading the item.		*/

	if (client_data != NULL) {

	    int	len;
	    int	i;

	    len = strlen (text);

/*	    We do not want to count any blanks in the string	*
 *	    since they are probably there as filler.		*/

	    for (i=0;i<len;i++) {

		if (!strncmp ((text+i)," ",1)) {

		    len--;

		}
	    }

	    if (len >= (int) client_data) {

		char	*selection;

		selection = XmTextGetSelection (parent_widget);

/*		If any text is highlighed then we want to ignore it.	*/

		if (selection != NULL) {

		    if ((len-strlen(selection)) >= (int) client_data) {

			cbs->text->length--;

			if (cbs->text->length == 0) {

			    cbs->doit = False;

			}
			XtFree (selection);
			XtFree (text);
			return;

		    }

		    XtFree (selection);

		    len = cbs->text->length-1;

		    if (!isdigit ((int) cbs->text->ptr [len])) {

			cbs->text->length--;

		    }

		} else if (cbs->startPos == cbs->endPos)  {

		    cbs->text->length--;

		}

		if (cbs->text->length == 0) {

		    cbs->doit = False;

		}

		XtFree (text);
		return;

	    }

	} else if (strlen (text) > MAX_FLOAT_STRING) {

	    cbs->text->length--;

	    if (cbs->text->length == 0) {

		cbs->doit = False;

	    }
	    XtFree (text);
	    return;

	}

/*	Next check to see if the string so far has a decimal point	*
 *	in it.								*/

	if (strstr (text,".") != (char *) NULL) {

	    decimal_flag = 1;

	} else {

	    decimal_flag = 0;

	}

/*	Filter out everything other than digits, and one decimal point.	*/

	len = cbs->text->length-1;

	if (!isdigit ((int) cbs->text->ptr [len])) {

/*	    Not digit so check to see if it is a decimal point or	*
 *	    a negative sign.						*/

	    if (((cbs->text->ptr [len] == ASCII_DECIMAL_POINT) && 
		 (decimal_flag  == 1)) ||
		(cbs->text->ptr [len] != ASCII_DECIMAL_POINT)) {

		cbs->text->length--;

	    }
	}

/*	If something other than a digit or a space was detected, don't	*
 *	keep it.							*/

	if (cbs->text->length == 0) {

	    cbs->doit = False;

	}

	XtFree (text);

}

/************************************************************************
 *	Description: This function should be used for text widgets	*
 *		     XmNmodifyVerifyCallback if used for signed		*
 *		     real numbers.					*
 *									*
 *	Input:  w - widget ID of text widget				*
 *		client_data - max length of string (NULL uses default)	*
 *		call_data - text widget data				*
 *									*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_verify_float_callback (
Widget		parent_widget,
XtPointer	client_data,
XtPointer	call_data
)
{
	int	len;
	int	i;
	int	negative_flag;
	int	decimal_flag;
	char	*text;

	XmTextVerifyCallbackStruct	*cbs =
		(XmTextVerifyCallbackStruct *) call_data;

	if (cbs->text->ptr == NULL)
		return;

	if (cbs->text->length == 0)	/* Handle Backspace	*/
		return;

/*	First check if the string so far has a - sign in it.	*/

	text = XmTextGetString (parent_widget);

/*	Let's limit the number of elements in the string.  This	*
 *	prevents the user from overloading the item.		*/

	if (client_data != NULL) {

	    int	len;
	    int	i;

	    len = strlen (text);

/*	    We do not want to count any blanks in the string	*
 *	    since they are probably there as filler.		*/

	    for (i=0;i<len;i++) {

		if (!strncmp ((text+i)," ",1)) {

		    len--;

		}
	    }

/*	    If a max length was specified, do not allow the string to	*
 *	    be longer than it.						*/

	    if (len >= (int) client_data) {

		char	*selection;

		selection = XmTextGetSelection (parent_widget);

/*		If any text is highlighed then we want to ignore it.	*/

		if (selection != NULL) {

		    if ((len-strlen(selection)) >= (int) client_data) {

			cbs->text->length--;

			if (cbs->text->length == 0) {

			    cbs->doit = False;

			}
			XtFree (text);
			XtFree (selection);
			return;

		    }

		    XtFree (selection);

		} else if (cbs->startPos == cbs->endPos)  {

		    cbs->text->length--;

		    if (cbs->text->length == 0) {

			cbs->doit = False;

		    }
		    XtFree (text);
		    return;
		}
	    }

	} else if (strlen (text) > MAX_FLOAT_STRING) {

	    cbs->text->length--;

	    if (cbs->text->length == 0) {

		cbs->doit = False;

	    }
	    XtFree (text);
	    return;

	}

	if (strstr (text,"-") != (char *) NULL) {

/*	    The text string already has a negative sign in it. However,	*
 *	    if the negative sign is in any highlighed text then we can	*
 *	    accept it.							*/

	    char	*selection;

	    selection = XmTextGetSelection (parent_widget);

	    if (selection != NULL) {

		if (strstr (selection,"-") != (char *) NULL) {

		    negative_flag = 0;

		} else {

		    negative_flag = 1;

		}

		XtFree (selection);

	    } else {

		negative_flag = 1;

	    }

	} else {

/*	Check to see if anything is in the string before the current	*
 *	position other than blanks.  If so, then a - sign is not a	*
 *	valid character to enter.					*/

	    negative_flag = 0;

	    for (i=0;i<cbs->startPos;i++) {

		if (strncmp (&text[i]," ",1)) {

		    negative_flag = 1;
		    break;

		}
	    }

/*	    There can be no space between the - sign and a valid	*
 *	    numeric digit.						*/

	    for (i=cbs->startPos;i<strlen(text);i++) {

		if (!strncmp (&text[i]," ",1)) {

		    negative_flag = 1;
		    break;

		}
	    }
	}

/*	Next check to see if the string so far has a decimal point	*
 *	in it.								*/

	if (strstr (text,".") != (char *) NULL) {

/*	    The text string already has a decimal point in it. However,	*
 *	    if the decimal point is in any highlighed text then we can	*
 *	    accept it.							*/

	    char	*selection;

	    selection = XmTextGetSelection (parent_widget);

	    if (selection != NULL) {

		if (strstr (selection,".") != (char *) NULL) {

		    decimal_flag = 0;

		} else {

		    decimal_flag = 1;

		}

		XtFree (selection);

	    } else {

		decimal_flag = 1;

	    }


	} else {

	    decimal_flag = 0;

	}

/*	Filter out everything other than digits, a leading negative	*
 *	sign and one decimal point					*/

	len = cbs->text->length-1;

	if (!isdigit ((int) cbs->text->ptr [len])) {

/*	    Not digit so check to see if it is a decimal point or	*
 *	    a negative sign.						*/

	    if (((cbs->text->ptr [len] == ASCII_NEGATIVE) && 
		 (negative_flag == 1)) ||
		((cbs->text->ptr [len] == ASCII_DECIMAL_POINT) && 
		 (decimal_flag  == 1)) ||
		((cbs->text->ptr [len] != ASCII_NEGATIVE) &&
		 (cbs->text->ptr [len] != ASCII_DECIMAL_POINT))) {

		cbs->text->length--;

	    }
	}

/*	If something other than a digit or a space was detected, don't	*
 *	keep it.							*/

	if (cbs->text->length == 0) {

	    cbs->doit = False;

	}

	XtFree (text);

}

/************************************************************************
 *	Description: This function should be used for text widgets	*
 *		     XmNmodifyVerifyCallback if used for signed		*
 *		     integer numbers.					*
 *									*
 *	Input:  w - widget ID of text widget				*
 *		client_data - max length of string (NULL uses default)	*
 *		call_data - text widget data				*
 *									*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_verify_signed_integer_callback (
Widget		parent_widget,
XtPointer	client_data,
XtPointer	call_data
)
{
	int	len;
	int	negative_flag;
	char	*text;
	int	i;

	XmTextVerifyCallbackStruct	*cbs =
		(XmTextVerifyCallbackStruct *) call_data;

	if (cbs->text->ptr == NULL)
		return;

	if (cbs->text->length == 0)	/* Handle Backspace	*/
		return;

	text = XmTextGetString (parent_widget);

/*	Let's limit the number of elements in the string.  This	*
 *	prevents the user from overloading the item.		*/

	if (client_data != NULL) {

	    int	len;
	    int	i;

	    len = strlen (text);

/*	    We do not want to count any blanks in the string	*
 *	    since they are probably there as filler.		*/

	    for (i=0;i<len;i++) {

		if (!strncmp ((text+i)," ",1)) {

		    len--;

		}
	    }

	    if (len >= (int) client_data) {

		char	*selection;

		selection = XmTextGetSelection (parent_widget);

/*		If any text is highlighed then we want to ignore it.	*/

		if (selection != NULL) {

		    if ((len-strlen(selection)) >= (int) client_data) {

			cbs->text->length--;

			if (cbs->text->length == 0) {

			    cbs->doit = False;

			}
			XtFree (selection);
			XtFree (text);
			return;

		    }

		    XtFree (selection);

		} else if (cbs->startPos == cbs->endPos)  {

		    cbs->text->length--;

		    if (cbs->text->length == 0) {

			cbs->doit = False;

		    }
		    XtFree (text);
		    return;

	        }
	    }

	} else if (strlen (text) > MAX_INT_STRING) {

	    cbs->text->length--;

	    if (cbs->text->length == 0) {

		cbs->doit = False;

	    }
	    XtFree (text);
	    return;

	}

/*	First check if the string so far has a - sign in it.	*/

	if (strstr (text,"-") != (char *) NULL) {

	    negative_flag = 1;

	} else {

/*	Check to see if anything is in the string before the current	*
 *	position other than blanks.  If so, then a - sign is not a	*
 *	valid character to enter.					*/

	    negative_flag = 0;

	    for (i=0;i<cbs->startPos;i++) {

		if (strncmp (&text[i]," ",1)) {

		    negative_flag = 1;
		    break;

		}
	    }

/*	    There can be no space between the - sign and a valid	*
 *	    numeric digit.						*/

	    for (i=cbs->startPos;i<strlen(text);i++) {

		if (!strncmp (&text[i]," ",1)) {

		    negative_flag = 1;
		    break;

		}
	    }
	}

/*	Filter out everything other than digits and one - sign		*/

	len = cbs->text->length-1;

	if (!isdigit ((int) cbs->text->ptr [len])) {

/*	    Not digit so check to see if it is a decimal point or	*
 *	    a negative sign.						*/

	    if (((cbs->text->ptr [len] == ASCII_NEGATIVE) && 
		 (negative_flag == 1)) ||
		 (cbs->text->ptr [len] != ASCII_NEGATIVE)) {

		cbs->text->length--;

	    }
	}

/*	If something other than a digit or a space was detected, don't	*
 *	keep it.							*/

	if (cbs->text->length == 0) {

	    cbs->doit = False;

	}

	XtFree (text);
}

/************************************************************************
 *	Description: This function should be used for text widgets	*
 *		     XmNmodifyVerifyCallback if used for unsigned	*
 *		     integer numbers.					*
 *									*
 *	Input:  w - widget ID of text widget				*
 *		client_data - max length of string (NULL uses default)	*
 *		call_data - text widget data				*
 *									*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_verify_unsigned_integer_callback (
Widget		parent_widget,
XtPointer	client_data,
XtPointer	call_data
)
{
	int	len;
	char	*text;

	XmTextVerifyCallbackStruct	*cbs =
		(XmTextVerifyCallbackStruct *) call_data;

	if (cbs->text->ptr == NULL)
		return;

	if (cbs->text->length == 0)	/* Handle Backspace	*/
		return;

	text = XmTextGetString (parent_widget);

/*	Let's limit the number of elements in the string.  This	*
 *	prevents the user from overloading the item.		*/

	if (client_data != NULL) {

	    int	len;
	    int	i;

	    len = strlen (text);

/*	    We do not want to count any blanks in the string	*
 *	    since they are probably there as filler.		*/

	    for (i=0;i<len;i++) {

		if (!strncmp ((text+i)," ",1)) {

		    len--;

		}
	    }

	    if (len >= (int) client_data) {

		char	*selection;

		selection = XmTextGetSelection (parent_widget);

/*		If any text is highlighed then we want to ignore it.	*/

		if (selection != NULL) {

		    if ((len-strlen(selection)) >= (int) client_data) {

			cbs->text->length--;

			if (cbs->text->length == 0) {

			    cbs->doit = False;

			}
			XtFree (selection);
			XtFree (text);
			return;

		    }

		    XtFree (selection);

		} else if (cbs->startPos == cbs->endPos)  {

		    cbs->text->length--;

		    if (cbs->text->length == 0) {

			cbs->doit = False;

		    }
		    XtFree (text);
		    return;
		}
	    }

	} else if (strlen (text) > MAX_INT_STRING) {

	    cbs->text->length--;

	    if (cbs->text->length == 0) {

		cbs->doit = False;

	    }
	    XtFree (text);
	    return;

	}

/*	Filter out everything other than digits				*/

	len = cbs->text->length-1;

	if (!isdigit ((int) cbs->text->ptr [len])) {

	    cbs->text->length--;

	}

/*	If something other than a digit or a space was detected, don't	*
 *	keep it.							*/

	if (cbs->text->length == 0) {

	    cbs->doit = False;

	}

	XtFree (text);
}

/************************************************************************
 *	Description: This function should be used for text widgets	*
 *		     XmNmodifyVerifyCallback if used for unsigned	*
 *		     integer list numbers.				*
 *									*
 *	Input:  w - widget ID of text widget				*
 *		client_data - max length of string (NULL uses default)	*
 *		call_data - text widget data				*
 *									*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_verify_unsigned_list_callback (
Widget		parent_widget,
XtPointer	client_data,
XtPointer	call_data
)
{
	int	len;

	XmTextVerifyCallbackStruct	*cbs =
		(XmTextVerifyCallbackStruct *) call_data;

	if (cbs->text->ptr == NULL)
		return;

	if (cbs->text->length == 0)	/* Handle Backspace	*/
		return;

/*	Filter out everything other than digits and spaces		*/

	len = cbs->text->length-1;

/*	First check for digit.  If true, then kep it.		*/

	if ((!isdigit ((int) cbs->text->ptr [len])) &&
	    (!isspace ((int) cbs->text->ptr [len]))) {

	    cbs->text->length--;

	}

/*	If something other than a digit or a space was detected, don't	*
 *	keep it.							*/

	if (cbs->text->length == 0) {

	    cbs->doit = False;

	}
}

/************************************************************************
 *	Description: This function searches for the case insensitive	*
 *		     occurrence of string s2 in string s1.  If a match	*
 *		     is found, the function returns 1.  Otherwise, 0 is	*
 *		     returned.						*
 *									*
 *	Input:  s1 - pointer to main string				*
 *		s2 - pointer to search string				*
 *	Output: NONE							*
 *	Return: 1 if match was found, otherwise 0.			*
 ************************************************************************/

int
hci_string_in_string (
char	*s1,
char	*s2
)
{
	int	len1;
	int	len2;
	int	i;

	len1 = strlen (s1);
	len2 = strlen (s2);

	if ((len1 == 0) ||
	    (len2 == 0) ||
	    (len2 > len1)) {

	    return 0;

	}

	for (i=0;i<=len1-len2;i++) {

	    if (strncasecmp (&s1[i],s2,len2) == 0) {

		return 1;

	    }
	}

	return 0;

}

/************************************************************************
 *	Description: This function should be used for text widgets	*
 *		     XmNactivateCallback and XmNlosingFocusCallback	*
 *		     if used for date data (MMDDYY)			*
 *									*
 *	Input:  w - widget ID of text widget				*
 *		client_data - unused					*
 *		call_data - text widget data				*
 *									*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_verify_date_callback (
Widget		parent_widget,
XtPointer	client_data,
XtPointer	call_data
)
{
	char	*string;
	int	date;
	int	month, day, year;
	XtPointer	data;

	string = XmTextGetString (parent_widget);

	sscanf (string,"%d", &date);

/*	NOTE:  Allow a date of 0 to indicate not set condition.	*/

	month = date/10000;
	day   = (date - month*10000)/100;
	year  = date - month*10000 - day*100;

	if ((date != 0) &&
	    ((month < 1) || (month > 12) ||
	     (day   < 1) || (day   > 31))) {

	    XtVaGetValues (parent_widget,
		XmNuserData,	&data,
		NULL);

	    sprintf (buf,"%d ", (int) data);
	    XmTextSetString (parent_widget, buf);

	    sprintf (buf,
		"You entered an invalid date of %d.\nThe valid range is 0 (not set)\to 123187.\nThe month and day fields should be valid.\nNo check is made for days in month\n", date);
            hci_warning_popup( parent_widget, buf, NULL ); 

	} else {

	    XtVaSetValues (parent_widget,
		XmNuserData,	(XtPointer) date,
		NULL);

	}

	XtFree (string);
}

/************************************************************************
 *	Description: This function should be used for text widgets	*
 *		     XmNactivateCallback and XmNlosingFocusCallback	*
 *		     if used for month data (0-12).			*
 *									*
 *	Input:  w - widget ID of text widget				*
 *		client_data - unused					*
 *		call_data - text widget data				*
 *									*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_verify_month_callback (
Widget		parent_widget,
XtPointer	client_data,
XtPointer	call_data
)
{
	char	*string;
	int	month;
	XtPointer	data;

	string = XmTextGetString (parent_widget);

	sscanf (string,"%d", &month);

/*	NOTE:  Allow a month of 0 to indicate not set condition.	*/

	if ((month <  0) ||
	    (month > 12)) {

	    XtVaGetValues (parent_widget,
		XmNuserData,	&data,
		NULL);

	    sprintf (buf,"%d ", (int) data);
	    XmTextSetString (parent_widget, buf);

	    sprintf (buf,
		"You entered an invalid month of %d.\nThe valid range is 0 (not set)\nto 12.\n", month);
            hci_warning_popup( parent_widget, buf, NULL ); 

	} else {

	    XtVaSetValues (parent_widget,
		XmNuserData,	(XtPointer) month,
		NULL);

	}

	XtFree (string);

}

/************************************************************************
 *	Description: This function should be used for text widgets	*
 *		     XmNactivateCallback and XmNlosingFocusCallback	*
 *		     if used for day data (0-31).			*
 *									*
 *	Input:  w - widget ID of text widget				*
 *		client_data - unused					*
 *		call_data - text widget data				*
 *									*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_verify_day_callback (
Widget		parent_widget,
XtPointer	client_data,
XtPointer	call_data
)
{
	char	*string;
	int	day;
	XtPointer	data;

	string = XmTextGetString (parent_widget);

	sscanf (string,"%d", &day);

/*	NOTE:  Allow a day of 0 to indicate not set condition.	*/

	if ((day <  0) ||
	    (day > 31)) {

	    XtVaGetValues (parent_widget,
		XmNuserData,	&data,
		NULL);

	    sprintf (buf,"%d ", (int) data);
	    XmTextSetString (parent_widget, buf);

	    sprintf (buf,
		"You entered an invalid day of %d.\nThe valid range is 0 (not set)\nto 31.\n", day);
            hci_warning_popup( parent_widget, buf, NULL ); 

	} else {

	    XtVaSetValues (parent_widget,
		XmNuserData,	(XtPointer) day,
		NULL);

	}

	XtFree (string);
}

/************************************************************************
 *	Description: This function should be used for text widgets	*
 *		     XmNactivateCallback and XmNlosingFocusCallback	*
 *		     if used for year data (0,1988-2100).		*
 *									*
 *	Input:  w - widget ID of text widget				*
 *		client_data - unused					*
 *		call_data - text widget data				*
 *									*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_verify_year_callback (
Widget		parent_widget,
XtPointer	client_data,
XtPointer	call_data
)
{
	char	*string;
	int	year;
	XtPointer	data;

	string = XmTextGetString (parent_widget);

	sscanf (string,"%d", &year);

/*	NOTE:  Allow a year of 0 to indicate not set condition.	*/

	if ((year !=  0) &&
	    ((year < 1988) || (year>2100))) {

	    XtVaGetValues (parent_widget,
		XmNuserData,	&data,
		NULL);

	    sprintf (buf,"%d ", (int) data);
	    XmTextSetString (parent_widget, buf);

	    sprintf (buf,
		"You entered an invalid year of %d.\nThe valid range is 0 (not set)\nand 1988-2100.\n", year);
            hci_warning_popup( parent_widget, buf, NULL ); 

	} else {

	    XtVaSetValues (parent_widget,
		XmNuserData,	(XtPointer) year,
		NULL);

	}

	XtFree (string);
}

/************************************************************************
 *	Description: This function should be used for text widgets	*
 *		     XmNactivateCallback and XmNlosingFocusCallback	*
 *		     if used for time data (HHMMSS).			*
 *									*
 *	Input:  w - widget ID of text widget				*
 *		client_data - unused					*
 *		call_data - text widget data				*
 *									*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_verify_time_callback (
Widget		parent_widget,
XtPointer	client_data,
XtPointer	call_data
)
{
	char	*string;
	int	hhmmss;
	int	hour, minute, second;
	XtPointer	data;

	string = XmTextGetString (parent_widget);

	sscanf (string,"%d", &hhmmss);

	hour   = hhmmss/10000;
	minute = (hhmmss - hour*10000)/100;
	second = hhmmss - hour*10000 - minute*100;

	if ((hhmmss <      0) ||
	    (hhmmss > 235959) ||
	    (minute >     59) ||
	    (second >     59)) {

	    XtVaGetValues (parent_widget,
		XmNuserData,	&data,
		NULL);

	    sprintf (buf,"%d ", (int) data);
	    XmTextSetString (parent_widget, buf);

	    sprintf (buf,
		"You entered an invalid time of %d.\nThe valid range is 0 - 235959.\nThe minute and second fields must be less\nthan 60.\n", hhmmss);
            hci_warning_popup( parent_widget, buf, NULL ); 

	} else {

	    XtVaSetValues (parent_widget,
		XmNuserData,	(XtPointer) hhmmss,
		NULL);

	}

	XtFree (string);
}

/************************************************************************
 *	Description: This function should be used for text widgets	*
 *		     XmNactivateCallback and XmNlosingFocusCallback	*
 *		     if used for hour data (0-23).			*
 *									*
 *	Input:  w - widget ID of text widget				*
 *		client_data - unused					*
 *		call_data - text widget data				*
 *									*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_verify_hour_callback (
Widget		parent_widget,
XtPointer	client_data,
XtPointer	call_data
)
{
	char	*string;
	int	hour;
	XtPointer	data;

	string = XmTextGetString (parent_widget);

	sscanf (string,"%d", &hour);

	if ((hour <  0) ||
	    (hour > 23)) {

	    XtVaGetValues (parent_widget,
		XmNuserData,	&data,
		NULL);

	    sprintf (buf,"%d ", (int) data);
	    XmTextSetString (parent_widget, buf);

	    sprintf (buf,
		"You entered an invalid hour of %d.\nThe valid range is 0 - 23.\n", hour);
            hci_warning_popup( parent_widget, buf, NULL ); 

	} else {

	    XtVaSetValues (parent_widget,
		XmNuserData,	(XtPointer) hour,
		NULL);

	}

	XtFree (string);
}

/************************************************************************
 *	Description: This function should be used for text widgets	*
 *		     XmNactivateCallback and XmNlosingFocusCallback	*
 *		     if used for minute data (0-59).			*
 *									*
 *	Input:  w - widget ID of text widget				*
 *		client_data - unused					*
 *		call_data - text widget data				*
 *									*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_verify_minute_callback (
Widget		parent_widget,
XtPointer	client_data,
XtPointer	call_data
)
{
	char	*string;
	int	minute;
	XtPointer	data;

	string = XmTextGetString (parent_widget);

	sscanf (string,"%d", &minute);

	if ((minute <  0) ||
	    (minute > 59)) {

	    XtVaGetValues (parent_widget,
		XmNuserData,	&data,
		NULL);

	    sprintf (buf,"%d ", (int) data);
	    XmTextSetString (parent_widget, buf);

	    sprintf (buf,
		"You entered an invalid minute of %d.\nThe valid range is 0 - 59.\n", minute);
            hci_warning_popup( parent_widget, buf, NULL ); 

	} else {

	    XtVaSetValues (parent_widget,
		XmNuserData,	(XtPointer) minute,
		NULL);

	}

	XtFree (string);
}

/************************************************************************
 *	Description: This function should be used for text widgets	*
 *		     XmNactivateCallback and XmNlosingFocusCallback	*
 *		     if used for second data (0-59).			*
 *									*
 *	Input:  w - widget ID of text widget				*
 *		client_data - unused					*
 *		call_data - text widget data				*
 *									*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_verify_second_callback (
Widget		parent_widget,
XtPointer	client_data,
XtPointer	call_data
)
{
	char	*string;
	int	second;
	XtPointer	data;

	string = XmTextGetString (parent_widget);

	sscanf (string,"%d", &second);

	if ((second <  0) ||
	    (second > 59)) {

	    XtVaGetValues (parent_widget,
		XmNuserData,	&data,
		NULL);

	    sprintf (buf,"%d ", (int) data);
	    XmTextSetString (parent_widget, buf);

	    sprintf (buf,
		"You entered an invalid second of %d.\nThe valid range is 0 - 59.\n", second);
            hci_warning_popup( parent_widget, buf, NULL ); 

	} else {

	    XtVaSetValues (parent_widget,
		XmNuserData,	(XtPointer) second,
		NULL);

	}

	XtFree (string);
}

/************************************************************************
 *	Description: This function is used to validate a date.  It	*
 *		     takes into account the number of days in a month	*
 *		     and leap years.  The function returns 0 on success	*
 *		     and 1 on failure.					*
 *									*
 *	Input:  month - calendar month (1-12)				*
 *		day   - calendar day (1-31)				*
 *		year  - calendar year (1970-2100)			*
 *	Output: NONE							*
 *	Return: 0 on success; 1 on error				*
 ************************************************************************/

int
hci_validate_date (
int	month,
int	day,
int	year
)
{
	int	flag;
	struct tm t;
	time_t	tt;
	int	yy, mm, dd, hh, nn, ss;

	t.tm_year  = year-1900;
	t.tm_mon   = month-1;
	t.tm_mday  = day;
	t.tm_hour  = 0;
	t.tm_min   = 0;
	t.tm_sec   = 1;
	t.tm_isdst = -1;

/*	Convert the date to unix time.				*/

	tt = mktime (&t);

/*	Convert unix time back to calendar date/time.		*/

	unix_time (&tt, &yy, &mm, &dd, &hh, &nn, &ss);

	if ((month != mm) || (day != dd)) {

	    flag = 1;

	} else {

	    flag = 0;

	}

	return (flag);
}

/************************************************************************
 *	Description: This function returns 1 if a numeric digit was	*
 *		     found in the input string or 0 if one wasn't.	*
 *									*
 *	Input:  buf - pointer to string					*
 *	Output: NONE							*
 *	Return: 1 if digit found; 0 if digit wasn't found		*
 ************************************************************************/

int
hci_number_found (
char	*buf
)
{
	int	i;

	i = 0;

	if (strlen (buf)) {

	    while (i < strlen (buf)) {

		if (isdigit ((int) *(buf+i))) {

		    return (1);

		}

		i++;

	    }
	}

	return (0);

}
