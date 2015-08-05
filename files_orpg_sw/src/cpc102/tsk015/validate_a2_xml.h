/******************************************************************
    This is a tool that validates and verifies an archive 2 file
    against an xml file.
 ******************************************************************/

/* 
 * RCS info
 * $Author: jclose $
 * $Locker:  $
 * $Date: 2009/10/06 17:02:01 $
 * $Id: validate_a2_xml.h,v 1.2 2009/10/06 17:02:01 jclose Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */  
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

typedef struct xml_field {
    char* name;
    char* description;
    int type;
    double upperbound;
    double lowerbound;
    double distinct_arr[16];
    int distinct_arr_size;
    int size_offset;
    int max_size;
    int start_offset;
    int offset;
    int arr_size;
    int special_code;
    int verbosity;
    int print_output;
    int data_bit_size;
    char* string_val;
} xml_field;

typedef union {
    char c_arr[2];
    short s;
} char_to_short;

void print_xml_field(xml_field xf);
void set_data_type(xml_field* ptr, xmlNode* cur_node);
void load_field_values(xmlNode* cur_node, xml_field* xf, int num_children);
void reset_xml_field(xml_field *xf);
int  get_sizeof_type(xml_field xf);
void handle_unsigned_char(char* buffer, xml_field xf, int message_num);
void handle_unsigned_short(char* buffer, xml_field xf, int message_num);
void handle_short(char* buffer, xml_field xf, int message_num);
void handle_float(char* buffer, xml_field xf, int message_num);
void xpath_specific_object(char* buffer, char* object_name, int message_num, 
        xmlDocPtr* doc, xmlXPathContextPtr* xpathCtx, xmlXPathObjectPtr* xpathObj);
void initiate_xpath(char* buffer, int message_num, 
        xmlDocPtr* doc, xmlXPathContextPtr* xpathCtx, xmlXPathObjectPtr* xpathObj);
int  find_value_offset(char* buffer, xml_field xf, int arr_index, int message_num, 
        int loop_start_offset, int loop_max_size);
int  run_loop_query(char* buffer, int message_num, xmlNodeSetPtr nodes);
int  run_normal_query(char* buffer, int message_num, xmlNodeSetPtr nodes);
void make_handle_decision(char* buffer, xml_field xf, int message_num);
void initiate_xpath_vars(char* xml_path, xmlDocPtr* doc, 
        xmlXPathContextPtr* xpathCtx);
void run_xpath_query(char* query, xmlDocPtr* doc, 
        xmlXPathContextPtr* xpathCtx, xmlXPathObjectPtr* xpathObj);
void print_element_names(xmlNode * a_node);
char* packet_type_to_string(int packNum);
void add_value_to_printbuf(int val, int packNum, char* valName, int printMsg[32]);
int get_resized_data(xml_field xf, int val);
void check_short_xml_field(short val, xml_field xf, int packNum);
void check_short_range_values(short val, int lowerVal, int upperVal, 
        int packNum, char* valName, int special_code, int verbosity);
void check_unsigned_short_range_values(unsigned short val, 
        unsigned short lowerVal, unsigned short upperVal, 
        int packNum, char* valName, int special_code, int verbosity);
void check_unsigned_short_xml_field(unsigned short val, xml_field xf, int packNum);
void check_unsigned_short_range_values_from_distinct_array(unsigned short val, 
        int packNum, char* valName, double distinct_arr[16], int arr_size, 
        int special_code, int verbosity);
void check_string_xml_field(char *val, xml_field xf, int packNum);
void check_unsigned_int_xml_field(unsigned int val, xml_field xf, int packNum);
void check_unsigned_int_range_values(unsigned int val, 
        unsigned int lowerVal, unsigned int upperVal, 
        int packNum, char* valName, int special_code, int verbosity);
void check_int_xml_field(int val, xml_field xf, int packNum);
void check_int_range_values(int val, int lowerVal, int upperVal, 
        int packNum, char* valName, int special_code, int verbosity);
void check_int_range_values_from_distinct_array(int val, 
        int packNum, char* valName, double distinct_arr[16], 
        int arr_size, int verbosity);
void check_float_xml_field(float val, xml_field xf, int packNum);
void check_float_range_values(float val, float lowerVal, float upperVal, 
        int packNum, char* valName, int verbosity);
void check_char_values_from_distinct_array(char val, int packNum, char* valName,
        double distinct_arr[16], int arr_size, int verbosity);
void add_message_to_print_buf(int val, int packNum, char* valName);
void add_float_message_to_print_buf(float val, int packNum, char* valName); 
void add_message_to_error_buf(char* msg, int packNum);
void print_line_of_stars();
void clear_error_buf();
void print_error_buf();
void add_msg_to_error_buf(char* msg);
void add_to_summary_buff(char* val_desc, char cArg, 
        short sArg, unsigned short usArg, int iArg, 
        unsigned int uiArg, float fArg, int arg_type);
