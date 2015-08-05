/******************************************************************
    This file goes with validate_a2.  It provides the 
    xpath functionality that is needed.
******************************************************************/

/* 
 * RCS info
 * $Author: jclose $
 * $Locker:  $
 * $Date: 2009/10/06 17:02:00 $
 * $Id: validate_a2_xml.c,v 1.4 2009/10/06 17:02:00 jclose Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */  
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <glib.h>
#include <misc.h>
#include <math.h>
#include <libxml/debugXML.h>
#include "validate_a2_xml.h"

/* Global variables from validate_a2.c */
extern int printMsg[32];
extern int segment_number;
extern int printValues;
extern int verbosity_level;
extern int curr_type;
extern char* errBuf;
extern gchar* summaryBuf;

/*******************************************************************************
* Function: print_xml_field
*
* Description: This function prints all of the values in an xml_field.  It is 
*              good for debugging purposes.
*
* Inputs: xml_field xf - An xml_field
*
* Returns: nothing
*
*******************************************************************************/
void print_xml_field(xml_field xf)
{
    int j;
    fprintf(stdout, "\nEntry:\n");
    fprintf(stdout, "Name: '%s'\n", xf.name);
    fprintf(stdout, "Description: %s\n", xf.description);
    fprintf(stdout, "Upperbound: %f\n", xf.upperbound);
    fprintf(stdout, "Lowerbound: %f\n", xf.lowerbound);
    fprintf(stdout, "Type: %d\n", xf.type);
    fprintf(stdout, "Special Code: %d\n", xf.special_code);
    fprintf(stdout, "Arr_size : %d\n", xf.arr_size);
    fprintf(stdout, "Size offset: %d\n", xf.size_offset);
    fprintf(stdout, "Max size: %d\n", xf.max_size);
    fprintf(stdout, "Start offset: %d\n", xf.start_offset);
    fprintf(stdout, "Offset: %d\n", xf.offset);
    fprintf(stdout, "String-val: %s\n", xf.string_val);
    fprintf(stdout, "Verbosity: %d\n", xf.verbosity);
    fprintf(stdout, "Print Output: %d\n", xf.print_output);
    if (xf.distinct_arr_size > 0)
    {
        fprintf(stdout, "Distinct_arr_size: %d\n", xf.distinct_arr_size);
        fprintf(stdout, "Distinct Values: ");
        for (j = 0; j < xf.distinct_arr_size; j++)
            fprintf(stdout, "%f, ", xf.distinct_arr[j]);
        fprintf(stdout, "\n");
    }
} /* end print_xml_field */

/*******************************************************************************
* Function: set_data_type
*
* Description: This function sets the data type of an int (probably the 
*              memory location of the type field of an xml_field) based
*              upon the content of the node.
*
* Inputs: int* ptr - A pointer to an integer
*         xmlNode* cur_node - The xpath node
*
* Returns: nothing
*
*******************************************************************************/
void set_data_type(xml_field* ptr, xmlNode* cur_node)
{
    if (!g_ascii_strcasecmp((char*)cur_node->children->content, "unsigned short"))
        ptr->type = 0;
    else if (!g_ascii_strcasecmp((char*)cur_node->children->content, "short"))
        ptr->type  = 1;
    else if (!g_ascii_strcasecmp((char*)cur_node->children->content, "float"))
        ptr->type = 2;
    else if (!g_ascii_strcasecmp((char*)cur_node->children->content, "int"))
        ptr->type = 3;
    else if (!g_ascii_strcasecmp((char*)cur_node->children->content, "unsigned int"))
        ptr->type = 4;
    else if (!g_ascii_strcasecmp((char*)cur_node->children->content, "unsigned char"))
        ptr->type = 5;
    else if (!g_ascii_strcasecmp((char*)cur_node->children->content, "string"))
        ptr->type = 6;
    else
        ptr->type = 3;
    if (ptr->data_bit_size < 1)
        ptr->data_bit_size = 8 * get_sizeof_type(*ptr);
} /* end set_data_type */

/*******************************************************************************
* Function: load_field_values
*
* Description: This function sets the values in an xml_field based upon the 
*              xpath query that was performed.  It iterates through the 
*              node, and based upon the name of the node, sets the proper
*              values.
*
* Inputs: xmlNode* cur_node - The xpath node
*         xml_field* xf - The pointer to an xml_field that we are going to set.
*         int num_children - The number of children in this node.
*
* Returns: nothing
*
*******************************************************************************/
void load_field_values(xmlNode* cur_node, xml_field* xf, int num_children)
{
    reset_xml_field(xf);
    int j;
    for (j = 0; j < num_children; j++)
    {
        if (!g_ascii_strcasecmp((char *)cur_node->name, "name"))
            xf->name = g_strdup((char *)cur_node->children->content);
        else if (!g_ascii_strcasecmp((char *)cur_node->name, "lowerbound")) 
            xf->lowerbound = atof((char *)cur_node->children->content);
        else if (g_strrstr((char *)cur_node->name, "upperbound") != NULL)
            xf->upperbound = atof((char *)cur_node->children->content);
        else if (!g_ascii_strcasecmp((char *)cur_node->name, "distinct-value"))
        {
            xf->distinct_arr[xf->distinct_arr_size] = atof((char *)cur_node->children->content);
            xf->distinct_arr_size++;
        }
        else if (!g_ascii_strcasecmp((char*)cur_node->name, "offset"))
            xf->offset = atoi((char*)cur_node->children->content);
        else if (!g_ascii_strcasecmp((char*)cur_node->name, "array-size"))
            xf->arr_size = atoi((char*)cur_node->children->content);
        else if (!g_ascii_strcasecmp((char*)cur_node->name, "even"))
            xf->special_code = 1;
        else if (!g_ascii_strcasecmp((char*)cur_node->name, "odd"))
            xf->special_code = 2;
        else if (!g_ascii_strcasecmp((char*)cur_node->name, "type"))
            set_data_type(xf, cur_node);
        else if (!g_ascii_strcasecmp((char*)cur_node->name, "size-offset"))
            xf->size_offset = atoi((char*)cur_node->children->content);
        else if (!g_ascii_strcasecmp((char*)cur_node->name, "max-size"))
            xf->max_size= atoi((char*)cur_node->children->content);
        else if (!g_ascii_strcasecmp((char*)cur_node->name, "start-offset"))
            xf->start_offset = atoi((char*)cur_node->children->content);
        else if (!g_ascii_strcasecmp((char*)cur_node->name, "string-value"))
            xf->string_val = g_strdup((char*)cur_node->children->content);
        else if (!g_ascii_strcasecmp((char*)cur_node->name, "length"))
            xf->arr_size = atoi((char*)cur_node->children->content);
        else if (!g_ascii_strcasecmp((char*)cur_node->name, "verbosity"))
            xf->verbosity = atoi((char*)cur_node->children->content);
        else if (!g_ascii_strcasecmp((char*)cur_node->name, "print"))
            xf->print_output = 1;
        else if (!g_ascii_strcasecmp((char*)cur_node->name, "description"))
            xf->description = g_strdup((char *)cur_node->children->content);
        else if (!g_ascii_strcasecmp((char*)cur_node->name, "data-bit-size"))
            xf->data_bit_size = atoi((char *)cur_node->children->content);
        cur_node = cur_node->next;
    } /* end for */ 
}

/*******************************************************************************
* Function: reset_xml_field
*
* Description: This function resets all of the values in an xml_field.
*
* Inputs: xml_field* xf - The pointer to the xml_field we are looking at
*
* Returns: nothing
*
*******************************************************************************/
void reset_xml_field(xml_field *xf)
{
    if (xf->name != NULL)
    {
        g_free(xf->name);
        xf->name = NULL;
    }
    if (xf->string_val != NULL)
    {
        g_free(xf->string_val);
        xf->string_val = NULL;
    }
    if (xf->description != NULL)
    {
        g_free(xf->description);
        xf->description = NULL;
    }

    xf->lowerbound = 0.0;
    xf->upperbound = 0.0;
    xf->offset = 0;
    xf->arr_size = 0;
    xf->type = 0;
    xf->distinct_arr_size = 0;
    xf->special_code = 0;
    xf->size_offset = 0;
    xf->max_size = 0;
    xf->start_offset = 0;
    xf->verbosity = 1;
    xf->print_output = 0;
    xf->data_bit_size = 0;
} /* end reset_xml_field */

/*******************************************************************************
* Function: get_sizeof_type
*
* Description: This function determines what type of data we are looking at, 
*              and returns the size of that type of data.  Important for 
*              offsets and byte swapping
*
* Inputs: xml_field xf - The xml_field we are looking at
*
* Returns: int - The size of that data type
*
*******************************************************************************/
int get_sizeof_type(xml_field xf)
{
    switch (xf.type)
    {
        case 0:
            return sizeof(unsigned short);
        case 1:
            return sizeof(short);
        case 2:
            return sizeof(float);
        case 3:
            return sizeof(int);
        case 4:
            return sizeof(unsigned int);
        case 5:
            return sizeof(unsigned char);
        case 6:
            return sizeof(unsigned char);
        default:
            return sizeof(short);
    } /* end switch */
} /* end get_sizeof_type */

/*******************************************************************************
* Function: handle_string
*
* Description: This function handles an string xml_field value. It casts
*              the buffer as a float buffer, and then depending on if we
*              have an array, decides what to do.  It will make the proper 
*              call to check the values.
*
* Inputs: char* buffer - The buffer that we will byte swap...
*         xml_field xf - The xml_field we are looking at
*         int message_num - What message is this (for output purposes)
*
* Returns: nothing
*
*******************************************************************************/
void handle_string(char* buffer, xml_field xf, int message_num)
{
    gchar* tmpStr = g_strndup(buffer + xf.offset, xf.arr_size);
    if ((xf.print_output) && (strstr(summaryBuf, xf.description) != NULL)) {
        char *tmp = summaryBuf;
        summaryBuf = g_strdup_printf("%s%s = %s\n", tmp, xf.description, tmpStr);
        g_free (tmp);
    }
    check_string_xml_field(tmpStr, xf, message_num);
    g_free(tmpStr);
} /* end handle_string */

/*******************************************************************************
* Function: handle_unsigned_char
*
* Description: This function handles an unsigned char xml_field value. It casts
*              the buffer as a float buffer, and then depending on if we
*              have an array, decides what to do.  It will make the proper 
*              call to check the values.
*
* Inputs: char* buffer - The buffer that we will byte swap...
*         xml_field xf - The xml_field we are looking at
*         int message_num - What message is this (for output purposes)
*
* Returns: nothing
*
*******************************************************************************/
void handle_unsigned_char(char* buffer, xml_field xf, int message_num)
{
    char c;
    int s, i;
    if (xf.arr_size < 1)
        xf.arr_size = 1;
    for (i = 0; i < xf.arr_size; i++)
    {
        c = buffer[xf.offset + (i * sizeof(unsigned char))];
        s = (int)c;
        if (xf.print_output)
            add_to_summary_buff(xf.description, '\0', 0, 0, s, 0, 0, 3);
        check_unsigned_short_xml_field((unsigned short)get_resized_data(xf, s), xf, message_num);
    } /* end for */
} /* end handle_unsigned_short */

/*******************************************************************************
* Function: handle_unsigned_int
*
* Description: This function handles an unsigned int xml_field value. It casts
*              the buffer as a float buffer, and then depending on if we
*              have an array, decides what to do.  It will byte swap the 
*              int(s), and then make the proper call to check the values.
*
* Inputs: char* buffer - The buffer that we will byte swap...
*         xml_field xf - The xml_field we are looking at
*         int message_num - What message is this (for output purposes)
*
* Returns: nothing
*
*******************************************************************************/
void handle_unsigned_int(char* buffer, xml_field xf, int message_num)
{
    int* ipt = (int *)(buffer + xf.offset);
    if (xf.arr_size < 1)
        xf.arr_size = 1;
#ifdef LITTLE_ENDIAN_MACHINE
    MISC_swap_longs(xf.arr_size, (long*)ipt);
#endif
    int i;
    unsigned int lBound, uBound;
    for (i = 0; i < xf.arr_size; i++)
    {
        lBound = (int)xf.lowerbound;
        uBound = (int)xf.upperbound;
        if (xf.print_output)
            add_to_summary_buff(xf.description, '\0', 0, 0, 0, (unsigned int)ipt[i], 0, 4);
        check_unsigned_int_xml_field((unsigned int)get_resized_data(xf, ipt[i]) , xf, message_num);
    } /* end for */
} /* end handle_unsigned_short */

/*******************************************************************************
* Function: handle_int
*
* Description: This function handles an int xml_field value. It casts
*              the buffer as a float buffer, and then depending on if we
*              have an array, decides what to do.  It will byte swap the 
*              int(s), and then make the proper call to check the values.
*
* Inputs: char* buffer - The buffer that we will byte swap...
*         xml_field xf - The xml_field we are looking at
*         int message_num - What message is this (for output purposes)
*
* Returns: nothing
*
*******************************************************************************/
void handle_int(char* buffer, xml_field xf, int message_num)
{
    int* ipt = (int *)(buffer + xf.offset);
    if (xf.arr_size < 1)
        xf.arr_size = 1;
#ifdef LITTLE_ENDIAN_MACHINE
    MISC_swap_longs(xf.arr_size, (long*)ipt);
#endif
    int i, lBound, uBound;
    for (i = 0; i < xf.arr_size; i++)
    {
        lBound = (int)xf.lowerbound;
        uBound = (int)xf.upperbound;
        if (xf.print_output)
            add_to_summary_buff(xf.description, '\0', 0, 0, 0, ipt[i], 0, 4);
        check_int_xml_field(get_resized_data(xf, ipt[i]), xf, message_num);
    } /* end for */
} /* end handle_unsigned_short */

/*******************************************************************************
* Function: handle_unsigned_short
*
* Description: This function handles an unsigned short xml_field value. It casts
*              the buffer as a float buffer, and then depending on if we
*              have an array, decides what to do.  It will byte swap the 
*              short(s), and then make the proper call to check the values.
*
* Inputs: char* buffer - The buffer that we will byte swap...
*         xml_field xf - The xml_field we are looking at
*         int message_num - What message is this (for output purposes)
*
* Returns: nothing
*
*******************************************************************************/
void handle_unsigned_short(char* buffer, xml_field xf, int message_num)
{
    short* spt = (short *)(buffer + xf.offset);
    if (xf.arr_size < 1)
       xf.arr_size = 1;
#ifdef LITTLE_ENDIAN_MACHINE
    MISC_swap_shorts(xf.arr_size, spt);
#endif
    int i;
    for (i = 0; i < xf.arr_size; i++)
    {
        if (xf.print_output)
            add_to_summary_buff(xf.description, '\0', 0, (unsigned short)spt[i], 0, 0, 0, 0);
        check_unsigned_short_xml_field((unsigned short)get_resized_data(xf, (int)spt[i]), xf, message_num);
    }
} /* end handle_unsigned_short */

/*******************************************************************************
* Function: handle_short
*
* Description: This function handles a short xml_field value.  It casts
*              the buffer as a float buffer, and then depending on if we
*              have an array, decides what to do.  It will byte swap the 
*              short(s), and then make the proper call to check the values.
*
* Inputs: char* buffer - The buffer that we will byte swap...
*         xml_field xf - The xml_field we are looking at
*         int message_num - What message is this (for output purposes)
*
* Returns: nothing
*
*******************************************************************************/
void handle_short(char* buffer, xml_field xf, int message_num)
{
    short* spt = (short *)(buffer + xf.offset);
    if (xf.arr_size < 1)
        xf.arr_size = 1;
    int i;
    /* Yes, we are iterating through twice, but it simplifies the work */
#ifdef LITTLE_ENDIAN_MACHINE
    MISC_swap_shorts(xf.arr_size, spt);
#endif
    for (i = 0; i < xf.arr_size; i++)
    {
        if (xf.print_output)
            add_to_summary_buff(xf.description, '\0', spt[i], 0, 0, 0, 0, 1);
        check_short_xml_field((short)get_resized_data(xf, (int)spt[i]), xf, message_num);
    }
} /* end handle short */

/*******************************************************************************
* Function: handle_float
*
* Description: This function handles a float xml_field value.  It casts
*              the buffer as a float buffer, and then depending on if we
*              have an array, decides what to do.  It will byte swap the 
*              float(s), and then make the proper call to check the values.
*
* Inputs: char* buffer - The buffer that we will byte swap...
*         xml_field xf - The xml_field we are looking at
*         int message_num - What message is this (for output purposes)
*
* Returns: nothing
*
*******************************************************************************/
void handle_float(char* buffer, xml_field xf, int message_num)
{
    float* fpt = (float*)(buffer + xf.offset);
    if (xf.arr_size < 1)
        xf.arr_size = 1;
#ifdef LITTLE_ENDIAN_MACHINE
    MISC_swap_floats(xf.arr_size, fpt);
#endif
    int i;
    for (i = 0; i < xf.arr_size; i++)
    {
        if (xf.print_output)
            add_to_summary_buff(xf.description, '\0', 0, 0, 0, 0, fpt[i], 2);
        check_float_xml_field(fpt[i], xf, message_num);
    } /* end for */
} /* end handle_float */

/*******************************************************************************
* Function: find_value_offset
*
* Description: This function gives us the proper offset if we are looping.
*              It looks at the start offset, the loop index, and then 
*              adds the proper padding to the offset.
*
* Inputs: char* buffer - The buffer that we will byte swap...
*         xml_field xf - The xml_field we are looking at
*         int arr_index - The number of times we have looped
*         int message_num - What message is this (for output purposes)
*         int loop_start_offset - The starting offset for the loop
*         int loop_max_size - The maximum size of the loop
*
* Returns: int - The offset
*
*******************************************************************************/
int find_value_offset(char* buffer, xml_field xf, int arr_index, int message_num, 
        int loop_start_offset, int loop_max_size)
{
    /* We are not in a loop */
    if (!xf.start_offset)
        return loop_start_offset;
    else
    {
        if (arr_index > loop_max_size)
            return loop_start_offset;
        xf.offset += loop_start_offset;
        if (xf.arr_size > 0)
            return loop_start_offset + (xf.arr_size * get_sizeof_type(xf));
        return loop_start_offset;
    } /* end else */
} /* end fine_value_offset */

/*******************************************************************************
* Function: initiate_xpath
*
* Description: This is somewhat equivalent of initate_xpath, but
*              this will let you get the values of a specific object, 
*              not a message type.  This is useful for message 31.
*
* Inputs: char* buffer - The buffer that we will byte swap...
*         char* object_name - The name of the object in the xml
*         int message_num - What message is this (for output purposes)
*         xmlDocPtr* doc - The xmlDocPtr pointer
*         xmlXPathContextPtr* xpathCtx - The xmlXPathContextPtr pointer
*         xmlXPathObjectPtr* xpathObj - The xmlXPathObjectPtr pointer
*
* Returns: none
*
*******************************************************************************/
void xpath_specific_object(char* buffer, char* object_name, int message_num, 
        xmlDocPtr* doc, xmlXPathContextPtr* xpathCtx, xmlXPathObjectPtr* xpathObj)
{
    int i;
    gchar* xpath_string = NULL;
    xpath_string = g_strdup_printf("/archive-2-data/message[object-name='%s']/field-list/field", 
                                    object_name);
    run_xpath_query(xpath_string, doc, xpathCtx, xpathObj);
    g_free(xpath_string);
    run_normal_query(buffer, message_num, (*xpathObj)->nodesetval);
    xmlXPathFreeObject(*xpathObj);
    for (i=1; ; i++)
    {
        xpath_string = g_strdup_printf("/archive-2-data/message[object-name='%s']/field-list/loop[%d]/field", 
                object_name, i);
        run_xpath_query(xpath_string, doc, xpathCtx, xpathObj);
        g_free(xpath_string);
        if (!run_loop_query(buffer, message_num, (*xpathObj)->nodesetval))
        {
            xmlXPathFreeObject(*xpathObj);
            break;
        }
    }
} /* end xpath_specific_object */

/*******************************************************************************
* Function: initiate_xpath
*
* Description: This is the function that is called by everything else.
*              It initiates all xpath stuff.  You just need to give it
*              the buffer to byte swap & compare to, the message number, 
*              and the pointers.
*
* Inputs: char* buffer - The buffer that we will byte swap...
*         int message_num - What message is this (for output purposes)
*         xmlDocPtr* doc - The xmlDocPtr pointer
*         xmlXPathContextPtr* xpathCtx - The xmlXPathContextPtr pointer
*         xmlXPathObjectPtr* xpathObj - The xmlXPathObjectPtr pointer
*
* Returns: none
*
*******************************************************************************/
void initiate_xpath(char* buffer, int message_num, 
        xmlDocPtr* doc, xmlXPathContextPtr* xpathCtx, xmlXPathObjectPtr* xpathObj)
{
    int i;
    gchar* xpath_string = NULL;
    xpath_string = g_strdup_printf("/archive-2-data/message[number=%d]/field-list/field", message_num);
    run_xpath_query(xpath_string, doc, xpathCtx, xpathObj);
    g_free (xpath_string);
    run_normal_query(buffer, message_num, (*xpathObj)->nodesetval);
    xmlXPathFreeObject(*xpathObj);
    for (i = 1; ; i++)
    {
        xpath_string = g_strdup_printf("/archive-2-data/message[number=%d]/field-list/loop[%d]/field", message_num, i);
        run_xpath_query(xpath_string, doc, xpathCtx, xpathObj);
        g_free (xpath_string);
        if (!run_loop_query(buffer, message_num, (*xpathObj)->nodesetval))
        {
            xmlXPathFreeObject(*xpathObj);
            break;
        }
    }
} /* end initiate_xpath */

/*******************************************************************************
* Function: run_loop_query
*
* Description: This function actually performs the work of whatever the
*              query returns, but it does so for loops.  This only works on 
*              fields that have loops, and where the size of the loop is set
*              in an offset somewhere.
*              Before this function is called, initiate_xpath
*              must be called first to initialize the pointers.  Essentially,
*              we have 1 xml_field, and we go in and put the values of each 
*              field return in the query in that field, and then we test
*              the values of that field.  
*
* Inputs: char* buffer - The buffer that we will byte swap...
*         int message_num - What message is this (for output purposes)
*         char* query - The query to be run
*         xmlDocPtr* doc - The xmlDocPtr pointer
*         xmlXPathContextPtr* xpathCtx - The xmlXPathContextPtr pointer
*         xmlXPathObjectPtr* xpathObj - The xmlXPathObjectPtr pointer
* Returns: none
*
*******************************************************************************/
int run_loop_query(char* buffer, int message_num, xmlNodeSetPtr nodes)
{
    int size, i, j;
    xmlNode* cur_node = NULL;
    size = (nodes) ? nodes->nodeNr : 0;
    if (size < 1)
        return 0;

    xml_field xf[size];

    /* set these first */
    int loop_start_offset, loop_max_size, loop_size_offset, loop_size;
    loop_size_offset = -1;
    loop_max_size = 0;
    loop_start_offset = 0;
    loop_size = 0;

    /* We don't reuse the xml_fields here.  Just go in and set them all */
    for (i = 0; i < size; i++)
    {
        xf[i].name = NULL;
        xf[i].string_val = NULL;
        xf[i].description = NULL;
        cur_node = nodes->nodeTab[i]->children;
        load_field_values(cur_node, &xf[i], xmlLsCountNode(nodes->nodeTab[i]));
    } /* end for */

    /* This data should all be the same in the loop.  It really only needs to 
     * be defined in the first element.  There is no reason to waste time 
     * and check to see if it is the same for every element */
    loop_size_offset = xf[0].size_offset;
    loop_max_size = xf[0].max_size;
    loop_start_offset = xf[0].start_offset;
    
    /* we need to know how much to loop */
    char_to_short tmpShort;
    tmpShort.c_arr[0] = buffer[loop_size_offset];
    tmpShort.c_arr[1] = buffer[loop_size_offset + 1];
    loop_size = (tmpShort.s < loop_max_size ? tmpShort.s : loop_max_size);
   

    /* This may look complicated, but it's not.  We have a starting offset, a
     * size offset, and a current offset.  The size offset tells us where the
     * size of this loop is.  We look at that and see how many times we are to 
     * loop (loop_size).  Then we have a starting offset.  This is basically 
     * the equivalent to starting at offset 0.  We are just going to add the 
     * starting offset to the current offset for every field */

    for (i = 1; i < loop_size + 1; i++)
    {
        for (j = 0; j < size; j++)
        {
            /* we need to know where our start offset is.*/
            xf[j].start_offset = loop_start_offset;
            xf[j].max_size = loop_max_size;

            /* calculate the new offset */
            loop_start_offset = find_value_offset(buffer, xf[j], i, message_num, loop_start_offset, loop_max_size );
            make_handle_decision(buffer, xf[j], message_num);
        } /* end for j */
        loop_start_offset += get_sizeof_type(xf[j - 1]);
    } /* end for i */
    
    for (i = 0; i < size; i++)
        g_free(xf[i].name);
    return size;
} /* end run_loop_query */

/*******************************************************************************
* Function: run_normal_query
*
* Description: This function actually performs the work of whatever the
*              query returns.  Before this function is called, initiate_xpath
*              must be called first to initialize the pointers.  Essentially,
*              we have 1 xml_field, and we go in and put the values of each 
*              field return in the query in that field, and then we test
*              the values of that field.  
*
* Inputs: char* buffer - The buffer that we will byte swap...
*         int message_num - What message is this (for output purposes)
*         char* query - The query to be run
*         xmlDocPtr* doc - The xmlDocPtr pointer
*         xmlXPathContextPtr* xpathCtx - The xmlXPathContextPtr pointer
*         xmlXPathObjectPtr* xpathObj - The xmlXPathObjectPtr pointer
* Returns: none
*
*******************************************************************************/
int run_normal_query(char* buffer, int message_num, xmlNodeSetPtr nodes)
{
    int size, i;
    xmlNode* cur_node;
    xml_field xf;
    xf.name = NULL;
    xf.string_val = NULL;
    xf.description = NULL;

    size = (nodes) ? nodes->nodeNr : 0;
    /* If nothing is returned */
    if (size < 1)
        return 0;
    for(i = 0; i < size; ++i)
    {
        cur_node = nodes->nodeTab[i]->children;
        /* so at this point, cur_node is the field we are interested in */
        load_field_values(cur_node, &xf, xmlLsCountNode(nodes->nodeTab[i]));
        /* Decide what to do with it */
        make_handle_decision(buffer, xf, message_num);
        g_free(xf.name);
        xf.name = NULL;
    } /* end for loop */
    return size;
} /* end run_normal_query */

/*******************************************************************************
* Function: make_handle_decision
*
* Description: This function makes the decision of how to check our fields
*              based upon the field's data type.
*
* Inputs: char* buffer - The buffer that we will byte swap...
*         xml_field xf - The field that we will compare the value to
*         int message_num - What message is this (for output purposes)
*
* Returns: none
*
*******************************************************************************/
void make_handle_decision(char* buffer, xml_field xf, int message_num)
{
    if (xf.type == 0)
        handle_unsigned_short(buffer, xf, message_num);
    else if (xf.type == 1)
        handle_short(buffer, xf, message_num);
    else if (xf.type == 3)
        handle_int(buffer, xf, message_num);
    else if (xf.type == 4)
        handle_unsigned_int(buffer, xf, message_num);
    else if (xf.type == 5)
        handle_unsigned_char(buffer, xf, message_num);
    else if (xf.type == 6)
        handle_string(buffer, xf, message_num);
    else 
        handle_float(buffer, xf, message_num);
} /* end make_handle_decision */

/*******************************************************************************
* Function: initiate_xpath_vars
*
* Description: This function initiates all of the xpath variables that are 
*              needed in order to properly use libxml.  It will load in the 
*              context of the xml file.
*
* Inputs: char* xml_path - The path to the xml file
*         xmlDocPtr* doc - A pointer to the xmlDocPtr
*         xmlPathContextPtr* xpathCtx - A pointer to the xmlPathContextPtr
*
* Returns: none
*
*******************************************************************************/
void initiate_xpath_vars(char* xml_path, 
        xmlDocPtr* doc, xmlXPathContextPtr* xpathCtx)
{
    /* Load XML document */
    *doc = xmlParseFile(xml_path);
    if (*doc == NULL) {
        fprintf(stderr, "Error: unable to parse file \"%s\"\n", xml_path);
        return;
    }
                            
    /* Create xpath evaluation context */
    *xpathCtx = xmlXPathNewContext(*doc);
    if(*xpathCtx == NULL) {
        fprintf(stderr,"Error: unable to create new XPath context\n");
        xmlFreeDoc(*doc); 
    }
} /* end initiate_xpath_vars */

/*******************************************************************************
* Function: run_xpath_query
*
* Description: This function runs an xpath query on the preloaded xmlDoc, 
*              and sets the pointers accordingly.
*
* Inputs: char* query - The query to be run
*         xmlDocPtr* doc - A pointer to the xmlDocPtr
*         xmlPathContextPtr* xpathCtx - A pointer to the xmlPathContextPtr
*         xmlPathObjectPtr* xpathCtx - A pointer to the xmlPathObjectPtr
*
* Returns: none
*
*******************************************************************************/
void run_xpath_query(char* query, xmlDocPtr* doc, 
        xmlXPathContextPtr* xpathCtx, xmlXPathObjectPtr* xpathObj)
{
    /* Evaluate xpath expression */
    *xpathObj = xmlXPathEvalExpression((xmlChar *)query, *xpathCtx);
    if(xpathObj == NULL) {
        fprintf(stderr,"Error: unable to evaluate xpath expression \"%s\"\n", query);
        xmlXPathFreeContext(*xpathCtx);
        xmlFreeDoc(*doc);
    }
} /* end run_xpath_query */

/*******************************************************************************
* Function: add_value_to_printbuf
*
* Description: This function adds a message to the msgPrintBuf with a 
*              description of the value and some packet information.
*
* Inputs: int val - The value of the data that we are reporting
*         int packNum - The type of packet that this is
*         char* valName - The field name.
*
* Returns: none
*
*******************************************************************************/
void add_value_to_printbuf(int val, int packNum, char* valName, int printMsg[32])
{
    /* Make sure this packet type should be reported */
    if (printMsg[packNum])
        fprintf(stdout, "Segment %d: %s = %d\n", 
                              segment_number, valName, val);
} /* end add_value_to_printbuf */

/*******************************************************************************
* Function: get_resized_data
*
* Description: This function is one that is meant to allow us to utilize only
*              X number of bits in a number.  So for instance, if you have a
*              4-byte integer, but you want to compare only the first 10 bits
*              to a particular value, you would use this number to get the
*              value of those first 10 bits.  
*
* Inputs: xml_field xf - The xml_field we are looking at. 
*         int val - The value that we are changing.
*
* Returns: none
*
*******************************************************************************/
int get_resized_data(xml_field xf, int val)
{
    /* If it is already the right size, then do nothing */
    if (xf.data_bit_size >= (get_sizeof_type(xf) * 8))
      return val;
    
    int newVal, i, tmp;
    newVal = val;
    /* Start at the MSB, and work your way down */
    for (i = (get_sizeof_type(xf) * 8); i >= xf.data_bit_size; i--)
    {
        tmp = pow(2, i);
        if (newVal > tmp)
            newVal = newVal - tmp;
    }
    return newVal;
}

/*******************************************************************************
* Function: check_short_xml_field
*
* Description: This function performs the value comparisons for a short.
*              If we were given a range (lowerval and upperval), we will
*              make a call to check_short_range_values.  If we were given 
*              a distinct array, we will just deal with that here.
*
* Inputs: int val - The value of the data that we are reporting
*         xml_field xf - The xml_field that we are looking into.
*         int packNum - The message type
*
* Returns: none
*
*******************************************************************************/
void check_short_xml_field(short val, xml_field xf, int packNum) 
{
    if (!printMsg[packNum])
        return;
 
    if (xf.distinct_arr_size == 0)
        check_short_range_values(val, (short)xf.lowerbound, (short)xf.upperbound, packNum, 
            xf.name, xf.special_code, xf.verbosity);
    else
    {
        /* we will print the value as long as the user has entered all of the 
         * proper values, including the print flag & verbosity level */
        if (printValues && (verbosity_level >= xf.verbosity))
            fprintf(stdout, "Segment %d: %s = %d\n", 
                      segment_number, xf.name, val);

        int i;
        for (i = 0; i < xf.distinct_arr_size; i++)
            if ((short)xf.distinct_arr[i] == val)
               return;
        gchar* tmp = NULL;
        tmp = g_strdup_printf(" - Segment %d - %s - %d not a correct value.\n", segment_number, xf.name, val);
        add_msg_to_error_buf(tmp);
        g_free(tmp);
    } /* end else */
} /* end check_short_xml_field */

/*******************************************************************************
* Function: check_short_range_values
*
* Description: This function checks the value of data, making sure it is 
*              between and upper & lower boundary.  If the value is not 
*              within the boundaries, a message is added to the errBuf.  
*              A message may also be printed depending on 
*              what flags the user passed in.
*
* Inputs: short val - The value of the data that we are reporting
*         int lowerVal - The lower boundary
*         int upperVal - The upper boundary
*         int packNum - The type of packet that this is
*         char* valName - The field name.
*         int special_code - Tests for even/odd (1/2)
*         int verbosity - The verbosity
*
* Returns: nothing
*
*******************************************************************************/
void check_short_range_values(short val, 
                             int lowerVal, int upperVal, 
                             int packNum, char* valName, int special_code, int verbosity)
{
    if (!printMsg[packNum])
        return;
    gchar* tmp = NULL;

    /* we will print the value as long as the user has entered all of the 
     * proper values, including the print flag & verbosity level */
    if (printValues && (verbosity_level >= verbosity))
        fprintf(stdout, "Segment %d: %s = %d\n", 
                              segment_number, valName, val);

    /* Actually check the values */
    if ((val < lowerVal) || (val > upperVal))
    {
        if (val < lowerVal)
            tmp = g_strdup_printf(" - Segment %d - %s out of range. %d should not be less than %d\n", 
                        segment_number, valName, val, lowerVal);
        else
            tmp = g_strdup_printf(" - Segment %d - %s out of range. %d should not be greater than %d\n", 
                        segment_number, valName, val, upperVal);

        add_msg_to_error_buf(tmp);
        g_free(tmp);
    }
    /* Even */
    else if (special_code == 1)
    {
        if (val % 2)
        {
            tmp = g_strdup_printf(" - Segment %d - %s is a bad value. %d should not be an odd number.\n", 
                        segment_number, valName, val);
            add_msg_to_error_buf(tmp);
            g_free(tmp);
        }
    }
    /* Odd */
    else if (special_code == 2)
    {
        if (!(val % 2))
        {
            tmp = g_strdup_printf(" - Segment %d - %s is a bad value. %d should not be an even number.\n", 
                        segment_number, valName, val);
            add_msg_to_error_buf(tmp);
            g_free(tmp);
        }
    } /* end else if */
} /* end check_short_range_values */

/*******************************************************************************
* Function: check_string_values
*
* Description: This function checks the value of data, making sure it matches
*              the value it is supposed to be.  If the value is not 
*              within the boundaries, a message is added to the errBuf.  
*              A message may also be printed depending on 
*              what flags the user passed in.
*
* Inputs: char* val - The value of the data that we are reporting
*         char* cmpVal - The value to compare to
*         int packNum - The type of packet that this is
*         char* valName - The field name.
*         int verbosity - The verbosity
*
* Returns: nothing
*
*******************************************************************************/
void check_string_xml_field(char *val, xml_field xf, int packNum)
{
    if (!printMsg[packNum])
        return;
    if ((printValues) && (verbosity_level >= xf.verbosity))
        fprintf(stdout, "Segment %d: %s = %s\n", 
                              segment_number, xf.name, val);

    if (xf.string_val == NULL)
        return;

    if (strncmp(val, xf.string_val, strlen(xf.string_val)))
    {
        gchar* tmp = NULL;
        tmp = g_strdup_printf(" - Segment %d - %s incorrect. '%s' should equal '%s'\n", 
                                segment_number, xf.name, val, xf.string_val);
        add_msg_to_error_buf(tmp);
        g_free(tmp);
    } /* end if */
} /* end check_string_values */

/*******************************************************************************
* Function: check_unsigned_short_xml_field
*
* Description: This function performs the value comparisons for an unsigned 
*              short. If we were given a range (lowerval and upperval), we will
*              make a call to check_unsigned_short_range_values.  If we were 
*              given a distinct array, we will just deal with that here.
*
* Inputs: int val - The value of the data that we are reporting
*         xml_field xf - The xml_field that we are looking into.
*         int packNum - The message type
*
* Returns: none
*
*******************************************************************************/
void check_unsigned_short_xml_field(unsigned short val, xml_field xf, int packNum)
{
    if (!printMsg[packNum])
        return;
    if (xf.distinct_arr_size == 0)
        check_unsigned_short_range_values(val, (unsigned short)xf.lowerbound, (unsigned short)xf.upperbound, 
            packNum, xf.name, xf.special_code, xf.verbosity);
    else
    {
        if ((printValues) && (verbosity_level >= xf.verbosity))
            fprintf(stdout, "Segment %d: %s = %d\n", 
                              segment_number, xf.name, val);
        int i;
        for (i = 0; i < xf.distinct_arr_size; i++)
            if ((unsigned short)xf.distinct_arr[i] == val)
                return; 
        gchar* tmp = NULL;
        tmp = g_strdup_printf(" - Segment %d - %s - %d not a correct value.\n", segment_number, xf.name, val);
        add_msg_to_error_buf(tmp);
        g_free(tmp);
    } /* end else */
}

/*******************************************************************************
* Function: check_unsigned_short_range_values
*
* Description: This function checks the value of data, making sure it is 
*              between and upper & lower boundary.  If the value is not 
*              within the boundaries, a message is added to the errBuf.  
*              A message may also be printed depending on 
*              what flags the user passed in.
*
* Inputs: unsigned short val - The value of the data that we are reporting
*         unsigned short lowerVal - The lower boundary
*         unsigned short upperVal - The upper boundary
*         int packNum - The type of packet that this is
*         char* valName - The field name.
*         int special_code - Tests for even/odd (1/2)
*         int verbosity - The verbosity
*
* Returns: nothing
*
*******************************************************************************/
void check_unsigned_short_range_values(unsigned short val, 
        unsigned short lowerVal, unsigned short upperVal, int packNum, 
        char* valName, int special_code, int verbosity)
{
    if (!printMsg[packNum])
        return;
    if ((printValues) && (verbosity_level >= verbosity))
        fprintf(stdout, "Segment %d: %s = %d\n", 
                              segment_number, valName, val);

    gchar* tmp = NULL;
    if (val < lowerVal)
        tmp = g_strdup_printf(" - Segment %d - %s out of range. %d should not be less than %u\n", segment_number, valName, val, lowerVal);
    else if (val > upperVal)
        tmp = g_strdup_printf(" - Segment %d - %s out of range. %d should not be greater than %u\n", segment_number, valName, val, upperVal);
    else if ((special_code == 1) && (val % 2))
        tmp = g_strdup_printf(" - Segment %d - %s is a bad value. %d should not be an odd number.\n", 
                        segment_number, valName, val);
    else if ((special_code == 2) && (!(val % 2)))
        tmp = g_strdup_printf(" - Segment %d - %s is a bad value. %d should not be an even number.\n", 
                        segment_number, valName, val);
    if (tmp != NULL)
    {
        add_msg_to_error_buf(tmp);
        g_free(tmp);
    }
}  /* end check_unsigned_short_range_values */

/*******************************************************************************
* Function: check_int_xml_field 
*
* Description: This function performs the value comparisons for an int. 
*              If we were given a range (lowerval and upperval), we will
*              make a call to check_int_range_values.  If we were 
*              given a distinct array, we will just deal with that here.
*
* Inputs: int val - The value of the data that we are reporting
*         xml_field xf - The xml_field that we are looking into.
*         int packNum - The message type
*
* Returns: none
*
*******************************************************************************/
void check_int_xml_field(int val, xml_field xf, int packNum)
{
    if (!printMsg[packNum])
        return;
    if (xf.distinct_arr_size == 0)
        check_int_range_values(val, (int)xf.lowerbound, (int)xf.upperbound,
                           packNum, xf.name, xf.special_code, xf.verbosity);
    else
    {
        if ((printValues) && (verbosity_level >= xf.verbosity))
            fprintf(stdout, "Segment %d: %s = %d\n", 
                              segment_number, xf.name, val);
        int i;
        for (i = 0; i < xf.distinct_arr_size; i++)
            if ((int)xf.distinct_arr[i] == val)
               return; 
        gchar* tmp = NULL;
        tmp = g_strdup_printf(" - Segment %d - %s - %d not a correct value.\n", segment_number, xf.name, val);
        add_msg_to_error_buf(tmp);
        g_free(tmp);
    }
} /* end check_int_range_values */

/*******************************************************************************
* Function: check_int_range_values
*
* Description: This function checks the value of data, making sure it is 
*              between and upper & lower boundary.  If the value is not 
*              within the boundaries, a message is added to the errBuf.  
*              A message may also be printed depending on 
*              what flags the user passed in.  This also adds array info.
*
* Inputs: int val - The value of the data that we are reporting
*         int lowerVal - The lower boundary
*         int upperVal - The upper boundary
*         int packNum - The type of packet that this is
*         char* valName - The field name.
*         int special_code - Tests for even/odd (1/2)
*         int verbosity - The verbosity
*
* Returns: nothing
*
*******************************************************************************/
void check_int_range_values(int val,
                           int lowerVal, int upperVal,
                           int packNum, char* valName, int special_code, int verbosity)
{
    if (!printMsg[packNum])
        return;
    gchar* tmp = NULL;
    if ((printValues) && (verbosity_level >= verbosity))
        fprintf(stdout, "Segment %d: %s = %d\n", 
                              segment_number, valName, val);
    if (val < lowerVal)
        tmp = g_strdup_printf(" - Segment %d - %s out of range. %d should not be less than %d\n", segment_number, valName, val, lowerVal);
    else if (val > upperVal)
        tmp = g_strdup_printf(" - Segment %d - %s out of range. %d should not be greater than %d\n", segment_number, valName, val, upperVal);
    else if ((special_code == 1) && (val % 2))
        tmp = g_strdup_printf(" - Segment %d - %s is a bad value. %d should not be an odd number.\n", 
                        segment_number, valName, val);
    else if ((special_code == 2) && (!(val % 2)))
        tmp = g_strdup_printf(" - Segment %d - %s is a bad value. %d should not be an even number.\n", 
                        segment_number, valName, val);
    if (tmp != NULL)
    {
        add_msg_to_error_buf(tmp);
        g_free(tmp);
    }
} /* end check_int_range_values */

/*******************************************************************************
* Function: check_unsigned_int_xml_field
*
* Description: This function performs the value comparisons for an unsigned int. 
*              If we were given a range (lowerval and upperval), we will
*              make a call to check_unsigned_int_range_values.  If we were 
*              given a distinct array, we will just deal with that here.
*
* Inputs: int val - The value of the data that we are reporting
*         xml_field xf - The xml_field that we are looking into.
*         int packNum - The message type
*
* Returns: none
*
*******************************************************************************/
void check_unsigned_int_xml_field(unsigned int val, xml_field xf, int packNum)
{
    if (!printMsg[packNum])
        return;
    if (xf.distinct_arr_size == 0)
        check_unsigned_int_range_values(val, (unsigned int)xf.lowerbound, (unsigned int)xf.upperbound, 
                                        packNum, xf.name, xf.special_code, xf.verbosity);
    else
    {
        if ((printValues) && (verbosity_level >= xf.verbosity))
            fprintf(stdout, "Segment %d: %s = %d\n", 
                              segment_number, xf.name, val);
        int i;
        for (i = 0; i < xf.distinct_arr_size; i++)
            if ((unsigned int)xf.distinct_arr[i] == val)
               return; 
        gchar* tmp = NULL;
        tmp = g_strdup_printf(" - Segment %d - %s - %d not a correct value.\n", segment_number, xf.name, val);
        add_msg_to_error_buf(tmp);
        g_free(tmp);
    } /* end else */
} /* end check_unsigned_int_range_values */

/*******************************************************************************
* Function: check_unsigned_int_range_values
*
* Description: This function checks the value of data, making sure it is 
*              between and upper & lower boundary.  If the value is not 
*              within the boundaries, a message is added to the errBuf.  
*              A message may also be printed depending on 
*              what flags the user passed in.  
*
* Inputs: unsigned int val - The value of the data that we are reporting
*         unsigned int lowerVal - The lower boundary
*         unsigned int upperVal - The upper boundary
*         int packNum - The type of packet that this is
*         char* valName - The field name.
*         int special_code - Lets us know if we should test for even/odd (1/2)
*         int verbosity - The verbosity
*
* Returns: nothing
*
*******************************************************************************/
void check_unsigned_int_range_values(unsigned int val, unsigned int lowerVal, unsigned int upperVal, 
    int packNum, char* valName, int special_code, int verbosity)
{
    if (!printMsg[packNum])
        return;
    gchar* tmp = NULL;
    if ((printValues) && (verbosity_level >= verbosity))
        fprintf(stdout, "Segment %d: %s = %d\n", 
                              segment_number, valName, val);
    if (val < lowerVal)
        tmp = g_strdup_printf(" - Segment %d - %s out of range. %d should not be less than %d\n", 
                                segment_number, valName, val, lowerVal);
    else if (val > upperVal)
        tmp = g_strdup_printf(" - Segment %d - %s out of range. %d should not be greater than %d\n", 
                                segment_number, valName, val, upperVal);
    else if ((special_code == 1) && (val % 2))
        tmp = g_strdup_printf(" - Segment %d - %s is a bad value. %d should not be an odd number.\n", 
                                segment_number, valName, val);
    else if ((special_code == 2) && (!(val % 2)))
        tmp = g_strdup_printf(" - Segment %d - %s is a bad value. %d should not be an even number.\n", 
                                segment_number, valName, val);
    if (tmp != NULL)
    {
        add_msg_to_error_buf(tmp);
        g_free(tmp);
    }
} /* end check_unsigned_int_range_values */

/*******************************************************************************
* Function: check_float_xml_field
*
* Description: This function performs the value comparisons for an unsigned int. 
*              If we were given a range (lowerval and upperval), we will
*              make a call to check_float_range_values.  If we were 
*              given a distinct array, we will just deal with that here.
*
* Inputs: int val - The value of the data that we are reporting
*         xml_field xf - The xml_field that we are looking into.
*         int packNum - The message type
*
* Returns: none
*
*******************************************************************************/
void check_float_xml_field(float val, xml_field xf, int packNum)
{
    if (!printMsg[packNum])
        return;
    
    if (xf.distinct_arr_size == 0)
        check_float_range_values(val, (float)xf.lowerbound, (float)xf.upperbound, 
                             packNum, xf.name, xf.verbosity);
    else
    {
        if ((printValues) && (verbosity_level >= xf.verbosity))
            fprintf(stdout, "Segment %d: %s = %f\n", segment_number, xf.name, val);
        int i;
        for (i = 0; i < xf.distinct_arr_size; i++)
            if ((float)xf.distinct_arr[i] == val)
               return; 
        gchar* tmp = NULL;
        tmp = g_strdup_printf(" - Segment %d - %s - %f not a correct value.\n", segment_number, xf.name, val);
        add_msg_to_error_buf(tmp);
        g_free(tmp);
    }
} /* end check_float_xml_field */

/*******************************************************************************
* Function: check_float_range_values
*
* Description: This function checks the value of data, making sure it is 
*              between and upper & lower boundary.  If the value is not 
*              within the boundaries, a message is added to the errBuf.  
*              A message may also be printed depending on 
*              what flags the user passed in.  
*
* Inputs: float val - The value of the data that we are reporting
*         float lowerVal - The lower boundary
*         float upperVal - The upper boundary
*         int packNum - The type of packet that this is
*         char* valName - The field name.
*         int verbosity - The verbosity
*
* Returns: nothing
*
*******************************************************************************/
void check_float_range_values(float val, 
                             float lowerVal, float upperVal, 
                             int packNum, char* valName, int verbosity)
{
    if (!printMsg[packNum])
        return;
    if ((printValues) && (verbosity_level >= verbosity))
        fprintf(stdout, "Segment %d: %s = %f\n", 
                              segment_number, valName, val);
    
    gchar* tmp = NULL;
    if (val < lowerVal)
        tmp = g_strdup_printf(" - Segment %d - %s out of range. %f should not be less than %.5f\n", segment_number, valName, val, lowerVal);
    else if (val > upperVal)
        tmp = g_strdup_printf(" - Segment %d - %s out of range. %f should not be greater than %.5f\n", segment_number, valName, val, upperVal);
    if (tmp != NULL)
    {
        add_msg_to_error_buf(tmp);
        g_free(tmp);
    }
} /* end check_float_range_values */

/*******************************************************************************
* Function: add_message_to_print_buf
*
* Description: This function adds a message to the print buffer.
*
* Inputs: int val - The value of the data that we are reporting
*         int packNum - The type of packet that this is
*         char* valName - The field name.
*
* Returns: none 
*
*******************************************************************************/
void add_message_to_print_buf(int val, int packNum, char* valName) 
{
    if (printValues && printMsg[packNum])
        fprintf(stdout, "Segment %d: %s = %d\n", 
                              segment_number, valName, val);
} /* end add_message_to_print_buf */

/*******************************************************************************
* Function: add_message_to_error_buf
*
* Description: This function adds a message to the error buffer.
*
* Inputs: char* msg - The message that we are reporting
*         int packNum - The type of packet that this is
*
* Returns: none 
*
*******************************************************************************/
void add_message_to_error_buf(char* msg, int packNum)
{
    if (!printMsg[packNum])
        return;
    if (errBuf == NULL)
       errBuf = g_strdup_printf(" - Message %d - %s\n", segment_number, msg);
    else {
       char *tmp = errBuf;
       errBuf = g_strdup_printf("%s - Message %d - %s\n", tmp, segment_number, msg);
       g_free (tmp);
    }
} /* end add_message_to_error_buf */

/*******************************************************************************
* Function: add_float_message_to_print_buf
*
* Description: This function adds a message to the error buffer.
*
* Inputs: float val - The value of the data
*         int packNum - The type of packet that this is
*         char* valName - The name of the field
*
* Returns: none 
*
*******************************************************************************/
void add_float_message_to_print_buf(float val, int packNum, char* valName) 
{
    if (printValues && printMsg[packNum])
        fprintf(stdout, "Segment %d: %s = %f\n", 
                              segment_number, valName, val);
} /* end add_float_message_to_print_buf */

/*******************************************************************************
* Function: packet_type_to_string
*
* Description: This function tells us what kind of packet something is based
*              upon the packet type as a number.
*
* Inputs: int packNum - The type of packet that this is
*
* Returns: char* - The packet string. 
*
*******************************************************************************/
char* packet_type_to_string(int packNum)
{
    switch(packNum)
    {
        case 0:
            return "RDA RPG Message Header";
        case 1:
            return "Digital Radar Data Message";
        case 2:
            return "RDA Status Data Message";
        case 3:
            return "Performance/Maintenance Data Message";
        case 4:
            return "Console Message";
        case 5:
            return "Volume Coverage Pattern Data Message";
        case 6:
            return "RDA Control Commands Message";
        case 7:
            return "Volume Coverage Pattern Data Message";
        case 8:
            return "Clutter Censor Zones Message";
        case 9:
            return "Request for Data Message";
        case 10:
            return "Console Message";
        case 12:
            return "Loopback Test Message";
        case 13:
            return "Clutter Filter Bypass Map Message";
        case 15:
            return "Clutter Map Filter Message";
        case 18:
            return "RDA Adaptation Data Message";
        case 31:
            return "Digital Radar Data Generic Message";
        default:
            return "Digital Radar Data Generic Message";
    } /* end switch */
} /* end packet_type_to_string */

/*******************************************************************************
 * Function: print_line_of_stars
 *
 * Description: This function prints 80 characters of stars
 *
 * Inputs: none
 *
 * Returns: none
 *
 ******************************************************************************/
void print_line_of_stars()
{
    fprintf(stdout, "*******************************************");
    fprintf(stdout, "************************************\n");
} /* end print_line_of_starts */

/*******************************************************************************
 * Function: clear_error_buf
 *
 * Description: This function returns the number of segments required 
 *               for a particular message.
 *
 * Inputs: int type - The number of message types.
 *
 * Returns: int - The number of segments for that type
 *
 ******************************************************************************/
void clear_error_buf()
{
    if (errBuf == NULL)
       return;
    free(errBuf);
    errBuf = NULL;
} /* end clear_error_buf */

/*******************************************************************************
 * Function: print_error_buf
 *
 * Description: This function prints the errBuf
 *
 * Inputs: none
 *
 * Returns: none
 ******************************************************************************/
void print_error_buf()
{
    if (errBuf != NULL)
    {
        print_line_of_stars();
        fprintf(stdout, "ERRORS: %s(%d) - SEGMENT %d\n",
        packet_type_to_string(curr_type), curr_type, segment_number);
        print_line_of_stars();
        fprintf(stdout, "%s\n", errBuf);
    }
} /* print_error_buf */

/*******************************************************************************
 * Function: add_msg_to_error_buf
 * 
 * Description:  This function adds a message to errBuf
 * 
 * Inputs: char* msg - The message to be added
 * 
 * Returns: none
 * 
 ******************************************************************************/
void add_msg_to_error_buf(char* msg)
{
    if (msg == NULL)
      return;
    if (errBuf == NULL)
        errBuf = g_strdup(msg);
    else
        errBuf = g_strconcat(errBuf, msg, NULL);

    /* If this is a bad file, the error message buffer can fill up pretty 
     * quickly.  If it gets too big, it will allocate too much memory, 
     * and then the program slows down, eventually using up all system
     * resources.  So we will purge it before it gets too big. */
    if (strlen(errBuf) > 2048)
    {
        clear_error_buf();
        print_error_buf();
    }
} /* end add_msg_to_error_buf */

/*******************************************************************************
 * Function: add_to_summary_buff
 * 
 * Description:  This function adds a message to the summary buffer.  It 
 *               makes sure we haven't already recorded this field, and then
 *               makes the addition.
 * 
 * Inputs: char* val_desc - The description
 *         char cArg - The character value
 *         short sArg - The short value
 *         unsigned short usArg - The unsigned short value
 *         int iArg - The int value
 *         unsigned int uiArg - The unsigned int value
 *         float fArg - The float value
 *         int arg_type - The argument type. Tells us what we are printing.
 * 
 * Returns: none
 * 
 ******************************************************************************/
void add_to_summary_buff(char* val_desc, char cArg, short sArg, unsigned short usArg, 
        int iArg, unsigned int uiArg, float fArg, int arg_type)
{
    char *tmp = summaryBuf;
    if (strstr(summaryBuf, val_desc) != NULL)
        return;
    switch (arg_type)
    {
        case 0:
           summaryBuf = g_strdup_printf("%s%s = %d\n", tmp, val_desc, usArg);
           g_free(tmp);
           return;
        case 1:
           summaryBuf = g_strdup_printf("%s%s = %d\n", tmp, val_desc, sArg);
           g_free(tmp);
           return;
        case 2:
           summaryBuf = g_strdup_printf("%s%s = %f\n", tmp, val_desc, fArg);
           g_free(tmp);
           return;
        case 3:
           summaryBuf = g_strdup_printf("%s%s = %d\n", tmp, val_desc, iArg);
           g_free(tmp);
           return;
        case 4:
           summaryBuf = g_strdup_printf("%s%s = %d\n", tmp, val_desc, uiArg);
           g_free(tmp);
           return;
        case 5:
           summaryBuf = g_strdup_printf("%s%s = %c\n", tmp, val_desc, cArg);
           g_free(tmp);
           return;
        case 6:
           summaryBuf = g_strdup_printf("%s%s = %c\n", tmp, val_desc, cArg);
           g_free(tmp);
           return;
        default:
           g_free(tmp);
           return;
    }
} /* end add_to_summary_buff */
