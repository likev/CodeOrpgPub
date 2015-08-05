
/***********************************************************************

    Description: Wideband (RDA/RPG) comms monitor (mon_wb) header file.

***********************************************************************/

/* 
 * RCS info
 * $Author: jclose $
 * $Locker:  $
 * $Date: 2009/06/29 16:02:04 $
 * $Id: mon_wb_gui.h,v 1.1 2009/06/29 16:02:04 jclose Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */  


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <glib.h>
#include "mon_wb_gui_lib.h"

/* GLOBALS */
extern GMutex *wbmutex;
extern guiPointerStruct *ptrList;
extern GList* messageList;
extern GList* filterList;
extern int bufferSize;
GtkTextView* popup_txtview;
GtkWidget* popup_txtwindow;
void inner_g_list_free(const void *a, const void *b);
void set_global_pointers(GladeXML *builder);
int get_int_val_in_col_from_tree_view(GtkTreeView *tree_view, int col);
int get_string_in_col_from_tree_view(GtkTreeView *tree_view, int col, char** buf);
char* map_message_type_to_color(int msgType);
char* map_message_type_to_description(int msgType);
void add_row_to_available_filter_tree(char* val, char* color, int col_id);
void add_row_to_current_filter_tree(char* val, char* color, int col_id);
void add_row_to_constant_tree(GtkWidget* widget, char* field, char* value, char* color);
void add_top_filter_renderer(GladeXML *builder);
void add_bottom_filter_renderer(GladeXML *builder);
void remove_first_row_from_tree_view(GtkTreeView* tree_view);
void remove_last_row_from_tree_view(GtkTreeView* tree_view);
void add_row_to_message_list_tree(GtkTreeView * widget, char* time, int id, char* label, int msgTypeID, int messageLen, int order);
void add_constants_renderer(GladeXML *builder);
void add_message_list_renderer(GladeXML *builder);
int get_selected_indice(GtkTreeView* tree_view);
int get_request_lb_id();
int get_response_lb_id();
int get_message_contents(int msg_id_num, char **buf);
char* get_message_output_buf(); 
void set_verbose_level(int verb);
int get_verbose_level();
void update_verbosity_spinbutton(int level);
void updateIntConstantColumn(int index, int value);
void updateFloatConstantColumn(int index, float value);
void updateStringConstantColumn(int index, char* value);
void update_textfield(char* txt);
void initiate_constant_columns(GtkWidget* widget);
void initiate_available_filters_columns();
void add_packet_to_list(char* packet, char* labelMsg, char* disp, int msgID, int messageType, int msgLen, int order);
void add_filter_to_list();
void remove_filter_from_list();
void add_element_to_filter_list(int filterNum);
void remove_element_from_filter_list(int filterNum);
int is_message_type_in_filter(int msgType);
void remove_packet_type_from_message_list(int msgType);
void set_popup_window(GtkTextView* textView, GtkWidget* txtwindow);
