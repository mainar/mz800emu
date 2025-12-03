/* 
 * File:   ui_breakpoints.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 1. října 2015, 11:32
 * 
 * 
 * ----------------------------- License -------------------------------------
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 * ---------------------------------------------------------------------------
 */

#include "mz800emu_cfg.h"

#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <gtk-3.0/gtk/deprecated/gtktoggleaction.h>
#include <gtk-3.0/gtk/gtktogglebutton.h>


#ifdef MZ800EMU_CFG_DEBUGGER_ENABLED

#include "ui/debugger/ui_breakpoints.h"
#include "ui/ui_main.h"
#include "debugger/breakpoints.h"
#include "debugger/debugger.h"
#include "z80ex/include/z80ex.h"

#include "cfgfile/cfgroot.h"
#include "cfgfile/cfgcommon.h"

#define BPT_DEFAULT_GROUP   "Breakpoint Group"
#define BPT_DEFAULT_EVENT   "Addr: 0x"


typedef enum en_BRKTYPE {
    BRKTYPE_GROUP,
    BRKTYPE_EVENT,
} en_BRKTYPE;


typedef enum en_DBG_BREAKPOINTS {
    BRK_ID = 0,
    BRK_TYPE,
    BRK_NAME,
    BRK_ENABLED,
    BRK_MAY_BE_ENABLED,
    BRK_NAME_STRIKETHROUGH,
    BRK_NAME_WEIGHT,
    BRK_ADDR,
    BRK_ADDR_TXT,
    BRK_FG_COLOR,
    BRK_BG_COLOR,
    BRK_FG_R,
    BRK_FG_G,
    BRK_FG_B,
    BRK_BG_R,
    BRK_BG_G,
    BRK_BG_B,
    BRK_COUNT_COLUMNS
} en_DBG_BREAKPOINTS;


typedef enum en_UIPBDND {
    UIPBDND_DONE,
    UIPBDND_ACTION,
    UIPBDND_BAD_DESTINATION
} en_UIPBDND;


typedef struct st_UIBPOINTS {
    unsigned id;

    en_UIPBDND dnd_action;
    int dnd_src_parent_id;
    unsigned dnd_src_position;
    unsigned dnd_moved_id;

    int edit_id;
    en_BRKTYPE add_type;
    int add_to_parent_id;

    int add_event_focus;

    st_UIWINPOS main_pos;
    st_UIWINPOS settings_pos;

} st_UIBPOINTS;


static st_UIBPOINTS g_uibpoints;


gchar* ui_breakpoints_alocate_new_event_defname ( Z80EX_WORD addr ) {
    return g_strdup_printf ( "%s%04X", BPT_DEFAULT_EVENT, addr );
}


#define ui_breakpoints_add_group( model, iter, parent, id, name, enabled, fg, bg )           ui_breakpoints_add_item ( model, iter, parent, BRKTYPE_GROUP, id, name, enabled, 0x0000, fg, bg )
#define ui_breakpoints_add_event( model, iter, parent, id, name, enabled, addr, fg, bg )     ui_breakpoints_add_item ( model, iter, parent, BRKTYPE_EVENT, id, name, enabled, addr, fg, bg )


gboolean ui_breakpoints_add_item ( GtkTreeModel *model, GtkTreeIter *iter, GtkTreeIter *parent, en_BRKTYPE type, guint id, const gchar *name, gboolean enabled, Z80EX_WORD addr, GdkRGBA *fg_color, GdkRGBA *bg_color ) {

    /* TODO: test, zda vlozene item id jeste neexistuje */

    if ( parent != NULL ) {

        GValue gv_type = G_VALUE_INIT;
        gtk_tree_model_get_value ( model, parent, BRK_TYPE, &gv_type );

        if ( g_value_get_uint ( &gv_type ) != BRKTYPE_GROUP ) return FALSE;
    };

    gtk_tree_store_append ( GTK_TREE_STORE ( model ), iter, parent );

    gchar addr_txt [ 7 ];

    if ( type == BRKTYPE_GROUP ) {
        addr_txt [ 0 ] = 0x00;
    } else {
        sprintf ( addr_txt, "0x%04X", addr );
    };

    GdkRGBA def_fg_color;
    GdkRGBA def_bg_color;

    if ( fg_color == NULL ) {
        gdk_rgba_parse ( &def_fg_color, "black" );
        fg_color = &def_fg_color;
    };
    if ( bg_color == NULL ) {
        gdk_rgba_parse ( &def_bg_color, "white" );
        bg_color = &def_bg_color;
    };

    gtk_tree_store_set ( GTK_TREE_STORE ( model ), iter,
                         BRK_ID, id,
                         BRK_TYPE, type,
                         BRK_NAME, name,
                         BRK_ENABLED, enabled,
                         BRK_ADDR, addr,
                         BRK_ADDR_TXT, addr_txt,
                         BRK_FG_COLOR, fg_color,
                         BRK_BG_COLOR, bg_color,
                         BRK_FG_R, fg_color->red,
                         BRK_FG_G, fg_color->green,
                         BRK_FG_B, fg_color->blue,
                         BRK_BG_R, bg_color->red,
                         BRK_BG_G, bg_color->green,
                         BRK_BG_B, bg_color->blue,
                         -1 );

    return TRUE;
}


int ui_breakpoints_simple_add_event ( unsigned addr ) {
    char *name = ui_breakpoints_alocate_new_event_defname ( addr );
    GtkTreeIter iter;
    GtkTreeModel *model = GTK_TREE_MODEL ( ui_get_object ( "dbg_breakpoints_treestore" ) );
    int id = g_uibpoints.id++;
    gboolean ret = ui_breakpoints_add_event ( model, &iter, NULL, id, name, TRUE, addr, NULL, NULL );
    g_free ( name );
    if ( ret == FALSE ) {
        g_uibpoints.id--;
        return -1;
    }
    return id;
}

#if 0


void ui_breakpoints_experiments ( ) {


    GtkTreeModel *model = GTK_TREE_MODEL ( ui_get_object ( "dbg_breakpoints_treestore" ) );
    g_uibpoints.id = 0;
    GtkTreeIter iter;
    GtkTreeIter iter1;
    GtkTreeIter iter2;


    GdkRGBA color;
    gdk_rgba_parse ( &color, "green" );

    ui_breakpoints_add_group ( model, &iter, NULL, g_uibpoints.id++, "General Event Group", TRUE, NULL, NULL );
    ui_breakpoints_add_group ( model, &iter, NULL, g_uibpoints.id++, "Another Group", TRUE, NULL, NULL );

    ui_breakpoints_add_group ( model, &iter, NULL, g_uibpoints.id++, "Test Group", FALSE, NULL, NULL );
    ui_breakpoints_add_group ( model, &iter1, &iter, g_uibpoints.id++, "Test Group", TRUE, NULL, NULL );
    ui_breakpoints_add_group ( model, &iter2, &iter1, g_uibpoints.id++, "Test Group", FALSE, NULL, NULL );
    ui_breakpoints_add_group ( model, &iter1, &iter, g_uibpoints.id++, "Test Group", FALSE, NULL, NULL );

    ui_breakpoints_add_group ( model, &iter, NULL, g_uibpoints.id++, "Flappy Debugging", TRUE, NULL, NULL );
    ui_breakpoints_add_group ( model, &iter, NULL, g_uibpoints.id++, "CP/M debug", TRUE, &color, NULL );
    ui_breakpoints_add_group ( model, &iter, NULL, g_uibpoints.id++, "This is verry verry realy too much verry long name", TRUE, NULL, NULL );

    //ui_breakpoint_add_event ( model, &iter, NULL, g_uibpoints.id++, "Addr: 0xABCD", TRUE, 0xabcd, NULL, NULL );
    ui_breakpoints_simple_add_event ( 0xabcd );
    ui_breakpoints_simple_add_event ( 0xdead );
    ui_breakpoints_simple_add_event ( 0xbeef );
}
#endif


void ui_breakpoints_show_window ( void ) {

    GtkWidget *window = ui_get_widget ( "dbg_breakpoints_window" );
    if ( gtk_widget_get_visible ( window ) ) return;

    g_uibpoints.add_event_focus = 0;
    gtk_widget_show ( window );

    if ( g_debugger.auto_save_breakpoints ) {
        gtk_toggle_button_set_active ( ui_get_toggle ( "bpt_autosave_checkbutton" ), TRUE );
    } else {
        gtk_toggle_button_set_active ( ui_get_toggle ( "bpt_autosave_checkbutton" ), FALSE );
    };

    static int initialised = 0;
    if ( initialised == 0 ) {
        initialised = 1;
        ui_main_setpos ( &g_uibpoints.main_pos, -1, -1 );
        ui_main_setpos ( &g_uibpoints.settings_pos, -1, -1 );
    };
    ui_main_win_move_to_pos ( GTK_WINDOW ( window ), &g_uibpoints.main_pos );

#if 0
    static int firstrun = 1;
    if ( firstrun ) {
        firstrun = 0;
        ui_breakpoints_experiments ( );
    };
#endif
}


void ui_breakpoints_hide_window ( void ) {
    GtkWidget *window = ui_get_widget ( "dbg_breakpoints_window" );
    ui_main_win_get_pos ( GTK_WINDOW ( window ), &g_uibpoints.main_pos );
    gtk_widget_hide ( window );
}


void ui_breakpoints_show_hide_window ( void ) {
    GtkWidget *window = ui_get_widget ( "dbg_breakpoints_window" );
    if ( gtk_widget_get_visible ( window ) ) {
        ui_breakpoints_hide_window ( );
    } else {
        ui_breakpoints_show_window ( );
    };
}


void ui_breakpoints_copy_iter ( GtkTreeModel *model, GtkTreeIter *dst_iter, GtkTreeIter *src_iter ) {

    GValue gv_id = G_VALUE_INIT;
    GValue gv_type = G_VALUE_INIT;
    GValue gv_name = G_VALUE_INIT;
    GValue gv_enabled = G_VALUE_INIT;
    GValue gv_addr = G_VALUE_INIT;
    GValue gv_addr_txt = G_VALUE_INIT;

    GValue gv_fg_r = G_VALUE_INIT;
    GValue gv_fg_g = G_VALUE_INIT;
    GValue gv_fg_b = G_VALUE_INIT;

    GValue gv_bg_r = G_VALUE_INIT;
    GValue gv_bg_g = G_VALUE_INIT;
    GValue gv_bg_b = G_VALUE_INIT;

    gtk_tree_model_get_value ( model, src_iter, BRK_ID, &gv_id );
    gtk_tree_model_get_value ( model, src_iter, BRK_TYPE, &gv_type );
    gtk_tree_model_get_value ( model, src_iter, BRK_NAME, &gv_name );
    gtk_tree_model_get_value ( model, src_iter, BRK_ENABLED, &gv_enabled );
    gtk_tree_model_get_value ( model, src_iter, BRK_ADDR, &gv_addr );
    gtk_tree_model_get_value ( model, src_iter, BRK_ADDR_TXT, &gv_addr_txt );

    gtk_tree_model_get_value ( model, src_iter, BRK_FG_R, &gv_fg_r );
    gtk_tree_model_get_value ( model, src_iter, BRK_FG_G, &gv_fg_g );
    gtk_tree_model_get_value ( model, src_iter, BRK_FG_B, &gv_fg_b );

    gtk_tree_model_get_value ( model, src_iter, BRK_BG_R, &gv_bg_r );
    gtk_tree_model_get_value ( model, src_iter, BRK_BG_G, &gv_bg_g );
    gtk_tree_model_get_value ( model, src_iter, BRK_BG_B, &gv_bg_b );

    GdkRGBA fg_color;
    fg_color.red = g_value_get_double ( &gv_fg_r );
    fg_color.green = g_value_get_double ( &gv_fg_g );
    fg_color.blue = g_value_get_double ( &gv_fg_b );
    fg_color.alpha = 0xffff;

    GdkRGBA bg_color;
    bg_color.red = g_value_get_double ( &gv_bg_r );
    bg_color.green = g_value_get_double ( &gv_bg_g );
    bg_color.blue = g_value_get_double ( &gv_bg_b );
    bg_color.alpha = 0xffff;

    gtk_tree_store_set ( GTK_TREE_STORE ( model ), dst_iter,
                         BRK_ID, g_value_get_uint ( &gv_id ),
                         BRK_TYPE, g_value_get_uint ( &gv_type ),
                         BRK_NAME, g_value_get_string ( &gv_name ),
                         BRK_ENABLED, g_value_get_boolean ( &gv_enabled ),
                         BRK_ADDR, g_value_get_uint ( &gv_addr ),
                         BRK_ADDR_TXT, g_value_get_string ( &gv_addr_txt ),
                         BRK_FG_COLOR, &fg_color,
                         BRK_BG_COLOR, &bg_color,
                         BRK_FG_R, g_value_get_double ( &gv_fg_r ),
                         BRK_FG_G, g_value_get_double ( &gv_fg_g ),
                         BRK_FG_B, g_value_get_double ( &gv_fg_b ),
                         BRK_BG_R, g_value_get_double ( &gv_bg_r ),
                         BRK_BG_G, g_value_get_double ( &gv_bg_g ),
                         BRK_BG_B, g_value_get_double ( &gv_bg_b ),
                         -1 );

    GtkTreeIter child_src_iter;

    if ( gtk_tree_model_iter_children ( model, &child_src_iter, src_iter ) ) {
        do {
            GtkTreeIter child_dst_iter;
            gtk_tree_store_append ( GTK_TREE_STORE ( model ), &child_dst_iter, dst_iter );
            ui_breakpoints_copy_iter ( model, &child_dst_iter, &child_src_iter );
        } while ( gtk_tree_model_iter_next ( model, &child_src_iter ) );
    };
}


/*
 * 
 * Projdeme rekurzivne vsechny prvky - hledame ten, ktery ma pozadovane ID.
 * 
 */
gboolean ui_breakpoints_get_bpoint_iter_by_id ( GtkTreeModel *model, GtkTreeIter *iter, GtkTreeIter *parent_iter, unsigned id ) {

    if ( parent_iter == NULL ) {
        if ( !gtk_tree_model_get_iter_first ( model, iter ) ) return FALSE;
    } else {
        if ( !gtk_tree_model_iter_children ( model, iter, parent_iter ) ) return FALSE;
    };

    do {
        GValue gv_id = G_VALUE_INIT;
        gtk_tree_model_get_value ( model, iter, BRK_ID, &gv_id );

        if ( id == g_value_get_uint ( &gv_id ) ) return TRUE;

        if ( gtk_tree_model_iter_has_child ( model, iter ) ) {
            GtkTreeIter child_iter;
            if ( ui_breakpoints_get_bpoint_iter_by_id ( model, &child_iter, iter, id ) ) {
                memcpy ( iter, &child_iter, sizeof ( GtkTreeIter ) );
                return TRUE;
            };
        }

    } while ( gtk_tree_model_iter_next ( model, iter ) );

    return FALSE;
}


void ui_breakpoints_select_iter ( GtkTreeIter *iter ) {
    GtkTreeView *treeview = ui_get_tree_view ( "dbg_breakpoints_treeview" );
    //gtk_tree_view_expand_all ( treeview );
    GtkTreeModel *tree_model = gtk_tree_view_get_model ( treeview );
    GtkTreePath *path = gtk_tree_model_get_path ( tree_model, iter );
    gtk_tree_view_expand_to_path ( treeview, path );
    gtk_tree_path_free ( path );
    GtkTreeSelection *selection = gtk_tree_view_get_selection ( treeview );
    gtk_tree_selection_select_iter ( selection, iter );
}


void ui_breakpoints_select_id ( int id ) {
    GtkTreeView *treeview = ui_get_tree_view ( "dbg_breakpoints_treeview" );
    GtkTreeModel *tree_model = gtk_tree_view_get_model ( treeview );
    GtkTreeIter iter;
    if ( FALSE == ui_breakpoints_get_bpoint_iter_by_id ( tree_model, &iter, NULL, id ) ) return;
    ui_breakpoints_select_iter ( &iter );
}


#define ui_breakpoints_edit_iter( model, iter ) ui_breakpoints_edit_row ( model, iter, 0, 0, 0 )
#define ui_breakpoints_edit_new_group( model, parent ) ui_breakpoints_edit_row ( model, NULL, parent, BRKTYPE_GROUP, 0 )
#define ui_breakpoints_edit_new_event( model, parent, addr ) ui_breakpoints_edit_row ( model, NULL, parent, BRKTYPE_EVENT, addr )


void ui_breakpoints_edit_row ( GtkTreeModel *model, GtkTreeIter *iter, int parent_id, en_BRKTYPE type, unsigned addr ) {

    GValue gv_name = G_VALUE_INIT;
    const gchar *set_name = NULL;
    gboolean enabled = TRUE;

    GdkRGBA fg_color;
    GdkRGBA bg_color;

    if ( iter != NULL ) {

        GValue gv_id = G_VALUE_INIT;
        GValue gv_type = G_VALUE_INIT;
        GValue gv_enabled = G_VALUE_INIT;
        GValue gv_addr = G_VALUE_INIT;

        GValue gv_fg_r = G_VALUE_INIT;
        GValue gv_fg_g = G_VALUE_INIT;
        GValue gv_fg_b = G_VALUE_INIT;

        GValue gv_bg_r = G_VALUE_INIT;
        GValue gv_bg_g = G_VALUE_INIT;
        GValue gv_bg_b = G_VALUE_INIT;


        gtk_tree_model_get_value ( model, iter, BRK_ID, &gv_id );
        gtk_tree_model_get_value ( model, iter, BRK_TYPE, &gv_type );
        gtk_tree_model_get_value ( model, iter, BRK_NAME, &gv_name );
        gtk_tree_model_get_value ( model, iter, BRK_ENABLED, &gv_enabled );
        gtk_tree_model_get_value ( model, iter, BRK_ADDR, &gv_addr );

        gtk_tree_model_get_value ( model, iter, BRK_FG_R, &gv_fg_r );
        gtk_tree_model_get_value ( model, iter, BRK_FG_G, &gv_fg_g );
        gtk_tree_model_get_value ( model, iter, BRK_FG_B, &gv_fg_b );

        gtk_tree_model_get_value ( model, iter, BRK_BG_R, &gv_bg_r );
        gtk_tree_model_get_value ( model, iter, BRK_BG_G, &gv_bg_g );
        gtk_tree_model_get_value ( model, iter, BRK_BG_B, &gv_bg_b );

        g_uibpoints.edit_id = g_value_get_uint ( &gv_id );

        fg_color.red = g_value_get_double ( &gv_fg_r );
        fg_color.green = g_value_get_double ( &gv_fg_g );
        fg_color.blue = g_value_get_double ( &gv_fg_b );
        fg_color.alpha = 0xffff;

        bg_color.red = g_value_get_double ( &gv_bg_r );
        bg_color.green = g_value_get_double ( &gv_bg_g );
        bg_color.blue = g_value_get_double ( &gv_bg_b );
        bg_color.alpha = 0xffff;

        enabled = g_value_get_boolean ( &gv_enabled );
        type = g_value_get_uint ( &gv_type );
        addr = g_value_get_uint ( &gv_addr );
        set_name = g_value_get_string ( &gv_name );
    } else {
        g_uibpoints.edit_id = -1;
        g_uibpoints.add_to_parent_id = parent_id;

        gdk_rgba_parse ( &fg_color, "black" );
        gdk_rgba_parse ( &bg_color, "white" );

        if ( BRKTYPE_GROUP == type ) {
            set_name = BPT_DEFAULT_GROUP;
        } else {
            set_name = "";
        };
    }

    g_uibpoints.add_type = type;

    gtk_color_chooser_set_rgba ( GTK_COLOR_CHOOSER ( ui_get_widget ( "bpt_fg_colorbutton" ) ), &fg_color );
    gtk_color_chooser_set_rgba ( GTK_COLOR_CHOOSER ( ui_get_widget ( "bpt_bg_colorbutton" ) ), &bg_color );

    gtk_toggle_button_set_active ( ui_get_toggle ( "bpt_enabled_checkbutton" ), enabled );

    GtkWidget *window = ui_get_widget ( "bpt_settings_window" );

    if ( BRKTYPE_GROUP == type ) {
        gtk_window_set_title ( GTK_WINDOW ( window ), "Breakpoint Group Settings" );
        gtk_label_set_markup ( ui_get_label ( "bpt_name_label" ), "<b>Group Name:</b>" );
        gtk_widget_hide ( ui_get_widget ( "bpt_event_settings_frame" ) );

        gtk_widget_grab_focus ( ui_get_widget ( "bpt_name_entry" ) );

        gtk_entry_set_text ( ui_get_entry ( "bpt_name_entry" ), set_name );
        gtk_editable_set_position ( GTK_EDITABLE ( ui_get_widget ( "bpt_name_entry" ) ), strlen ( set_name ) );
        gtk_editable_select_region ( GTK_EDITABLE ( ui_get_widget ( "bpt_name_entry" ) ), 0, -1 );

    } else {
        gtk_window_set_title ( GTK_WINDOW ( window ), "Breakpoint Event Settings" );
        gtk_label_set_markup ( ui_get_label ( "bpt_name_label" ), "<b>Event Name:</b>" );
        gtk_widget_show ( ui_get_widget ( "bpt_event_settings_frame" ) );

        gtk_entry_set_text ( ui_get_entry ( "bpt_name_entry" ), set_name );

        char addr_txt[5];
        g_sprintf ( addr_txt, "%04X", addr );
        gtk_entry_set_text ( ui_get_entry ( "bpt_addr_entry" ), addr_txt );

        gtk_widget_grab_focus ( ui_get_widget ( "bpt_addr_entry" ) );
        gtk_editable_set_position ( GTK_EDITABLE ( ui_get_widget ( "bpt_addr_entry" ) ), 4 );
        gtk_editable_select_region ( GTK_EDITABLE ( ui_get_widget ( "bpt_addr_entry" ) ), 0, -1 );

    };

    gtk_widget_show ( window );
    ui_main_win_move_to_pos ( GTK_WINDOW ( window ), &g_uibpoints.settings_pos );
}


void ui_breakpoints_add_new ( en_BRKTYPE type ) {
    GtkTreeModel *model = GTK_TREE_MODEL ( ui_get_object ( "dbg_breakpoints_treestore" ) );
    GtkTreeSelection *selection = gtk_tree_view_get_selection ( ui_get_tree_view ( "dbg_breakpoints_treeview" ) );
    GtkTreeIter iter;
    GtkTreeIter parent_iter;

    int parent_id = -1;

    if ( gtk_tree_selection_get_selected ( selection, &model, &iter ) ) {
        GValue gv_type = G_VALUE_INIT;
        GValue gv_id = G_VALUE_INIT;
        gtk_tree_model_get_value ( model, &iter, BRK_TYPE, &gv_type );

        if ( BRKTYPE_GROUP == g_value_get_uint ( &gv_type ) ) {
            gtk_tree_model_get_value ( model, &iter, BRK_ID, &gv_id );
            parent_id = g_value_get_uint ( &gv_id );

        } else if ( gtk_tree_model_iter_parent ( model, &parent_iter, &iter ) ) {
            gtk_tree_model_get_value ( model, &parent_iter, BRK_ID, &gv_id );
            parent_id = g_value_get_uint ( &gv_id );
        };
    };

    if ( BRKTYPE_GROUP == type ) {
        ui_breakpoints_edit_new_group ( model, parent_id );
    } else {
        ui_breakpoints_edit_new_event ( model, parent_id, 0x0000 );
    };
}


void ui_breakpoints_remove_item_from_bptable ( GtkTreeModel *model, GtkTreeIter *iter ) {

    GValue gv_type = G_VALUE_INIT;
    GValue gv_enabled = G_VALUE_INIT;
    GValue gv_may_be_enabled = G_VALUE_INIT;

    gtk_tree_model_get_value ( model, iter, BRK_TYPE, &gv_type );
    gtk_tree_model_get_value ( model, iter, BRK_ENABLED, &gv_enabled );
    gtk_tree_model_get_value ( model, iter, BRK_MAY_BE_ENABLED, &gv_may_be_enabled );

    gboolean is_enabled = g_value_get_boolean ( &gv_enabled ) & g_value_get_boolean ( &gv_may_be_enabled );

    if ( is_enabled != TRUE ) return;

    if ( BRKTYPE_EVENT == g_value_get_uint ( &gv_type ) ) {
        GValue gv_id = G_VALUE_INIT;
        GValue gv_addr = G_VALUE_INIT;
        gtk_tree_model_get_value ( model, iter, BRK_ID, &gv_id );
        gtk_tree_model_get_value ( model, iter, BRK_ADDR, &gv_addr );
        breakpoints_event_clear ( (Z80EX_WORD) g_value_get_uint ( &gv_addr ), g_value_get_uint ( &gv_id ) );
        return;
    };

    GtkTreeIter child_iter;
    if ( gtk_tree_model_iter_children ( model, &child_iter, iter ) ) {
        do {
            ui_breakpoints_remove_item_from_bptable ( model, &child_iter );
        } while ( gtk_tree_model_iter_next ( model, &child_iter ) );
    };
}


void ui_breakpoints_prepare_cfgfile ( st_CFGROOT *cfgroot, GtkTreeModel *model, GtkTreeIter *parent, char **childs_poi ) {

    GtkTreeIter iter;

    if ( !gtk_tree_model_iter_children ( model, &iter, parent ) ) return;

    do {
        GValue gv_id = G_VALUE_INIT;
        GValue gv_type = G_VALUE_INIT;
        GValue gv_name = G_VALUE_INIT;
        GValue gv_enabled = G_VALUE_INIT;
        GValue gv_addr = G_VALUE_INIT;
        GValue gv_fg_r = G_VALUE_INIT;
        GValue gv_fg_g = G_VALUE_INIT;
        GValue gv_fg_b = G_VALUE_INIT;
        GValue gv_bg_r = G_VALUE_INIT;
        GValue gv_bg_g = G_VALUE_INIT;
        GValue gv_bg_b = G_VALUE_INIT;

        GdkRGBA fg_color;
        GdkRGBA bg_color;

        gtk_tree_model_get_value ( model, &iter, BRK_ID, &gv_id );
        gtk_tree_model_get_value ( model, &iter, BRK_TYPE, &gv_type );
        gtk_tree_model_get_value ( model, &iter, BRK_NAME, &gv_name );
        gtk_tree_model_get_value ( model, &iter, BRK_ENABLED, &gv_enabled );
        gtk_tree_model_get_value ( model, &iter, BRK_ADDR, &gv_addr );

        gtk_tree_model_get_value ( model, &iter, BRK_FG_R, &gv_fg_r );
        gtk_tree_model_get_value ( model, &iter, BRK_FG_G, &gv_fg_g );
        gtk_tree_model_get_value ( model, &iter, BRK_FG_B, &gv_fg_b );

        gtk_tree_model_get_value ( model, &iter, BRK_BG_R, &gv_bg_r );
        gtk_tree_model_get_value ( model, &iter, BRK_BG_G, &gv_bg_g );
        gtk_tree_model_get_value ( model, &iter, BRK_BG_B, &gv_bg_b );

        fg_color.red = g_value_get_double ( &gv_fg_r );
        fg_color.green = g_value_get_double ( &gv_fg_g );
        fg_color.blue = g_value_get_double ( &gv_fg_b );
        fg_color.alpha = 0xffff;

        bg_color.red = g_value_get_double ( &gv_bg_r );
        bg_color.green = g_value_get_double ( &gv_bg_g );
        bg_color.blue = g_value_get_double ( &gv_bg_b );
        bg_color.alpha = 0xffff;

        unsigned id = g_value_get_uint ( &gv_id );
        unsigned id_digits = 1;
        unsigned size = 10;
        while ( id > ( size - 1 ) ) {
            id_digits++;
            size *= 10;
        };

        char *modulename = malloc ( 4 + id_digits );
        CFGCOMMON_MALLOC_ERROR ( modulename == NULL );
        sprintf ( modulename, "ID_%d", id );

        CFGMOD *cmod = cfgroot_register_new_module ( cfgroot, modulename );
        free ( modulename );

        unsigned old_len = strlen ( *childs_poi );

        if ( old_len == 0 ) {
            *childs_poi = realloc ( *childs_poi, id_digits );
            CFGCOMMON_MALLOC_ERROR ( *childs_poi == NULL );
            sprintf ( *childs_poi, "%d", id );
        } else {
            char *old_poi = *childs_poi;
            *childs_poi = malloc ( old_len + 1 + id_digits + 1 );
            sprintf ( *childs_poi, "%s %d", old_poi, id );
            free ( old_poi );
        };


        en_BRKTYPE type = g_value_get_uint ( &gv_type );

        cfgmodule_register_new_element ( cmod, "type", CFGENTYPE_KEYWORD, type,
                                         BRKTYPE_GROUP, "GROUP",
                                         BRKTYPE_EVENT, "EVENT",
                                         -1 );
        cfgmodule_register_new_element ( cmod, "enabled", CFGENTYPE_BOOL, g_value_get_boolean ( &gv_enabled ) );
        cfgmodule_register_new_element ( cmod, "name", CFGENTYPE_TEXT, g_value_get_string ( &gv_name ) );

        gchar *fg_colortxt = gdk_rgba_to_string ( &fg_color );
        cfgmodule_register_new_element ( cmod, "fg_color", CFGENTYPE_TEXT, fg_colortxt );
        g_free ( fg_colortxt );

        gchar *bg_colortxt = gdk_rgba_to_string ( &bg_color );
        cfgmodule_register_new_element ( cmod, "bg_color", CFGENTYPE_TEXT, bg_colortxt );
        g_free ( bg_colortxt );

        if ( type == BRKTYPE_GROUP ) {

            GtkTreeIter child_iter;
            if ( gtk_tree_model_iter_children ( model, &child_iter, &iter ) ) {

                char *childs = malloc ( 1 );
                CFGCOMMON_MALLOC_ERROR ( childs == NULL );
                childs [ 0 ] = 0x00;

                ui_breakpoints_prepare_cfgfile ( cfgroot, model, &iter, &childs );

                cfgmodule_register_new_element ( cmod, "childs", CFGENTYPE_TEXT, childs );

                free ( childs );
            };

        } else {
            char addr_txt [ 7 ];
            sprintf ( addr_txt, "0x%04x", g_value_get_uint ( &gv_addr ) );
            cfgmodule_register_new_element ( cmod, "address", CFGENTYPE_TEXT, addr_txt );
        };

    } while ( gtk_tree_model_iter_next ( model, &iter ) );
}


void ui_breakpoints_save ( void ) {

    st_CFGROOT *cfgroot = cfgroot_new ( BREAKPOINTS_INI_FILENAME );

    CFGMOD *cmod = cfgroot_register_new_module ( cfgroot, "BREAKPOINTS" );

    GtkTreeModel *model = GTK_TREE_MODEL ( ui_get_object ( "dbg_breakpoints_treestore" ) );

    char *childs = malloc ( 1 );
    CFGCOMMON_MALLOC_ERROR ( childs == NULL );
    childs [ 0 ] = 0x00;

    ui_breakpoints_prepare_cfgfile ( cfgroot, model, NULL, &childs );

    cfgmodule_register_new_element ( cmod, "childs", CFGENTYPE_TEXT, childs );

    free ( childs );

    cfgroot_save ( cfgroot );
    cfgroot_destroy ( cfgroot );
}


void ui_breakpoints_parse_cfg ( st_CFGROOT *cfgroot, GtkTreeModel *model, GtkTreeIter *parent, char *childs ) {

    while ( strlen ( childs ) ) {
        while ( *childs == ' ' ) {
            childs++;
        };
        if ( ( *childs == 0x00 ) || ( *childs < '0' ) || ( *childs > '9' ) ) break;
        unsigned id_digits = 0;
        unsigned value = 0;
        do {
            id_digits++;
            if ( value == 0 ) {
                value = 1;
            } else {
                value *= 10;
            };
        } while ( ( childs [ id_digits ] >= '0' ) && ( childs [ id_digits ] <= '9' ) );


        char *modulename = malloc ( 4 + id_digits );
        CFGCOMMON_MALLOC_ERROR ( modulename == NULL );

        unsigned id = 0;
        while ( id_digits-- ) {
            id += ( *childs - '0' ) * value;
            childs++;
            value /= 10;
        };

        sprintf ( modulename, "ID_%d", id );

        CFGMOD *cmod = cfgroot_register_new_module ( cfgroot, modulename );


        CFGELM *elm_type = cfgmodule_register_new_element ( cmod, "type", CFGENTYPE_KEYWORD, BRKTYPE_GROUP,
                                                            BRKTYPE_GROUP, "GROUP",
                                                            BRKTYPE_EVENT, "EVENT",
                                                            -1 );
        CFGELM *elm_enabled = cfgmodule_register_new_element ( cmod, "enabled", CFGENTYPE_BOOL, 0 );
        CFGELM *elm_name = cfgmodule_register_new_element ( cmod, "name", CFGENTYPE_TEXT, "Default Name" );
        CFGELM *elm_fg_color = cfgmodule_register_new_element ( cmod, "fg_color", CFGENTYPE_TEXT, "rgba(0,0,0,255)" );
        CFGELM *elm_bg_color = cfgmodule_register_new_element ( cmod, "bg_color", CFGENTYPE_TEXT, "rgba(255,255,255,255)" );
        CFGELM *elm_childs = cfgmodule_register_new_element ( cmod, "childs", CFGENTYPE_TEXT, "" );
        CFGELM *elm_address = cfgmodule_register_new_element ( cmod, "address", CFGENTYPE_TEXT, "0x0000" );

        cfgmodule_parse ( cmod );

        GdkRGBA bg_color_cfg;
        GdkRGBA fg_color_cfg;
        GdkRGBA *bg_color;
        GdkRGBA *fg_color;

        if ( gdk_rgba_parse ( &bg_color_cfg, cfgelement_get_text_value ( elm_bg_color ) ) ) {
            bg_color = &bg_color_cfg;

        } else {
            bg_color = NULL;
        };

        if ( gdk_rgba_parse ( &fg_color_cfg, cfgelement_get_text_value ( elm_fg_color ) ) ) {
            fg_color = &fg_color_cfg;
        } else {
            fg_color = NULL;
        };

        GtkTreeIter iter;

        if ( BRKTYPE_GROUP == cfgelement_get_keyword_value ( elm_type ) ) {
            ui_breakpoints_add_group ( model, &iter, parent, g_uibpoints.id++, cfgelement_get_text_value ( elm_name ), cfgelement_get_bool_value ( elm_enabled ), fg_color, bg_color );
            ui_breakpoints_parse_cfg ( cfgroot, model, &iter, cfgelement_get_text_value ( elm_childs ) );
        } else {

            char *addr_txt = cfgelement_get_text_value ( elm_address );

            if ( strncmp ( addr_txt, "0x", 2 ) ) {
                addr_txt += 2;
            } else if ( strncmp ( addr_txt, "0X", 2 ) ) {
                addr_txt += 2;
            } else if ( *addr_txt == '#' ) {
                addr_txt++;
            };

            unsigned addr = debuger_hextext_to_uint32 ( addr_txt );
            ui_breakpoints_add_event ( model, &iter, parent, g_uibpoints.id++, cfgelement_get_text_value ( elm_name ), cfgelement_get_bool_value ( elm_enabled ), addr, fg_color, bg_color );
        };

        free ( modulename );
    };
}


void ui_breakpoints_init ( void ) {

    g_uibpoints.id = 0;

    st_CFGROOT *cfgroot = cfgroot_new ( BREAKPOINTS_INI_FILENAME );

    CFGMOD *cmod = cfgroot_register_new_module ( cfgroot, "BREAKPOINTS" );
    CFGELM *elm = cfgmodule_register_new_element ( cmod, "childs", CFGENTYPE_TEXT, "" );

#if 0
    if ( EXIT_SUCCESS == cfgmodule_parse ( cmod ) ) {
        printf ( "INFO: restore breakpoints from %s\n", BREAKPOINTS_INI_FILENAME );
    }
#else
    cfgmodule_parse ( cmod );
#endif

    GtkTreeModel *model = GTK_TREE_MODEL ( ui_get_object ( "dbg_breakpoints_treestore" ) );

    char *childs = cfgelement_get_text_value ( elm );

    ui_breakpoints_parse_cfg ( cfgroot, model, NULL, childs );

    cfgroot_destroy ( cfgroot );
}


/*
 * 
 * Udalosti datoveho modelu
 * 
 */

G_MODULE_EXPORT void on_dbg_breakpoints_treestore_row_changed ( GtkTreeModel *tree_model, GtkTreePath *path, GtkTreeIter *iter, gpointer user_data ) {
    //    printf ( "%s()\n", __func__ );

    static int lock = 0;

    if ( lock ) return;

    GtkTreeIter new_parent_iter;
    gboolean this_may_be_enabled = TRUE;

    if ( gtk_tree_model_iter_parent ( tree_model, &new_parent_iter, iter ) ) {

        GValue gv_enabled = G_VALUE_INIT;
        gtk_tree_model_get_value ( tree_model, &new_parent_iter, BRK_ENABLED, &gv_enabled );

        GValue gv_may_be_enabled = G_VALUE_INIT;
        gtk_tree_model_get_value ( tree_model, &new_parent_iter, BRK_MAY_BE_ENABLED, &gv_may_be_enabled );

        this_may_be_enabled = g_value_get_boolean ( &gv_enabled ) & g_value_get_boolean ( &gv_may_be_enabled );

    };

    unsigned name_weight;
    GValue gv_type = G_VALUE_INIT;
    gtk_tree_model_get_value ( tree_model, iter, BRK_TYPE, &gv_type );

    if ( g_value_get_uint ( &gv_type ) == BRKTYPE_GROUP ) {
        name_weight = 1200;
    } else {
        name_weight = 400;
    };


    if ( g_value_get_uint ( &gv_type ) == BRKTYPE_EVENT ) {

        GValue gv_id = G_VALUE_INIT;
        GValue gv_addr = G_VALUE_INIT;
        GValue gv_enabled = G_VALUE_INIT;
        gtk_tree_model_get_value ( tree_model, iter, BRK_ID, &gv_id );
        gtk_tree_model_get_value ( tree_model, iter, BRK_ADDR, &gv_addr );
        gtk_tree_model_get_value ( tree_model, iter, BRK_ENABLED, &gv_enabled );

        int addr = breakpoints_event_get_addr_by_id ( g_value_get_uint ( &gv_id ) );

        if ( FALSE == ( g_value_get_boolean ( &gv_enabled ) & this_may_be_enabled ) ) {
            if ( -1 != addr ) {
                /* bpt je zakazan - pokud je v bptable, tak ho odstranime */
                printf ( "INFO - remove breakpoint on addr: 0x%04x\n", (Z80EX_WORD) addr );
                breakpoints_event_clear_addr ( (Z80EX_WORD) addr );
            };
        } else {
            /* bpt je povolen */
            if ( -1 != addr ) { /* nachazi se v bptable? */
                if ( addr != g_value_get_uint ( &gv_addr ) ) { /* zustava na stejne adrese? pokud ne, tak ho odstranime */
                    printf ( "INFO - remove breakpoint on addr: 0x%04x\n", (Z80EX_WORD) addr );
                    breakpoints_event_clear_addr ( (Z80EX_WORD) addr );
                };
            };

            if ( addr != g_value_get_uint ( &gv_addr ) ) {
                /* aktualni ndni v btp - jdeme ho tam vlozit */
                int id = breakpoints_event_get_id_by_addr ( (Z80EX_WORD) g_value_get_uint ( &gv_addr ) );
                if ( id > BREAKPOINT_TYPE_NONE ) {
                    /* tohle misto v pameti uz obsadil jiny bpt */
                    printf ( "WARNING - duplicate breakpoint on addr: 0x%04x\n", (Z80EX_WORD) g_value_get_uint ( &gv_addr ) );
                    lock = 1;
                    gtk_tree_store_set ( GTK_TREE_STORE ( tree_model ), iter, BRK_ENABLED, FALSE, -1 );
                    lock = 0;
                } else {
                    /* neni v bptable - vlozime novy zaznam */
                    printf ( "INFO - add breakpoint on addr: 0x%04x\n", (Z80EX_WORD) g_value_get_uint ( &gv_addr ) );
                    breakpoints_event_add ( (Z80EX_WORD) g_value_get_uint ( &gv_addr ), g_value_get_uint ( &gv_id ) );
                };
            };
        };
    };


    lock = 1;
    gtk_tree_store_set ( GTK_TREE_STORE ( tree_model ), iter,
                         BRK_MAY_BE_ENABLED, this_may_be_enabled,
                         BRK_NAME_STRIKETHROUGH, ~this_may_be_enabled & 1,
                         BRK_NAME_WEIGHT, name_weight,
                         -1 );
    lock = 0;

    GtkTreeIter child_iter;

    if ( gtk_tree_model_iter_children ( tree_model, &child_iter, iter ) ) {

        GValue gv_enabled = G_VALUE_INIT;
        gtk_tree_model_get_value ( tree_model, iter, BRK_ENABLED, &gv_enabled );

        GValue gv_may_be_enabled = G_VALUE_INIT;
        gtk_tree_model_get_value ( tree_model, iter, BRK_MAY_BE_ENABLED, &gv_may_be_enabled );

        gboolean child_may_be_enabled = g_value_get_boolean ( &gv_enabled ) & g_value_get_boolean ( &gv_may_be_enabled );

        do {
            gtk_tree_store_set ( GTK_TREE_STORE ( tree_model ), &child_iter, BRK_MAY_BE_ENABLED, child_may_be_enabled, BRK_NAME_STRIKETHROUGH, ~child_may_be_enabled & 1, -1 );
        } while ( gtk_tree_model_iter_next ( tree_model, &child_iter ) );
    };
}


G_MODULE_EXPORT void on_dbg_breakpoints_treestore_row_inserted ( GtkTreeModel *tree_model, GtkTreePath *path, GtkTreeIter *iter, gpointer user_data ) {
    //    printf ( "%s()\n", __func__ );

    GtkTreeIter new_parent_iter;
    if ( gtk_tree_model_iter_parent ( tree_model, &new_parent_iter, iter ) ) {

        GValue gv_type = G_VALUE_INIT;
        gtk_tree_model_get_value ( tree_model, &new_parent_iter, BRK_TYPE, &gv_type );

        if ( BRKTYPE_GROUP != g_value_get_uint ( &gv_type ) ) {

            if ( g_uibpoints.dnd_action == UIPBDND_ACTION ) {
                g_uibpoints.dnd_action = UIPBDND_BAD_DESTINATION;
            } else {
                assert ( BRKTYPE_GROUP != g_value_get_uint ( &gv_type ) );
            };
        };
    };
}


/*
 * 
 * Callbacks
 * 
 */

G_MODULE_EXPORT gboolean on_dbg_breakpoints_window_delete_event ( GtkWidget *widget, GdkEvent *event, gpointer user_data ) {
    ui_breakpoints_hide_window ( );
    return TRUE;
}


G_MODULE_EXPORT gboolean on_dbg_breakpoints_window_key_press_event ( GtkWidget *widget, GdkEventKey *event, gpointer user_data ) {

    if ( event->keyval == GDK_KEY_Escape ) {
        ui_breakpoints_hide_window ( );
        return TRUE;
    } else if ( g_uibpoints.add_event_focus == 1 ) {
        if ( ( event->keyval == GDK_KEY_Return ) || ( event->keyval == GDK_KEY_KP_Enter ) ) {
            gtk_button_clicked ( GTK_BUTTON ( ui_get_widget ( "bpt_add_event_button" ) ) );
        };
    };
    return FALSE;
}


G_MODULE_EXPORT void on_bpt_hide_menuitem_activate ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
    ui_breakpoints_hide_window ( );
}


G_MODULE_EXPORT void on_bpt_save_menuitem_activate ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
    ui_breakpoints_save ( );
}


G_MODULE_EXPORT void on_bpt_enabled_cellrenderertoggle_toggled ( GtkCellRendererToggle *cell_renderer, gchar *path_str, gpointer user_data ) {

    GtkTreeModel *model = GTK_TREE_MODEL ( ui_get_object ( "dbg_breakpoints_treestore" ) );
    GtkTreeIter iter;

    gtk_tree_model_get_iter_from_string ( model, &iter, path_str );
    GValue gv = G_VALUE_INIT;
    gtk_tree_model_get_value ( model, &iter, BRK_ENABLED, &gv );

    gboolean enabled = ( ~g_value_get_boolean ( &gv ) ) & 1;

    gtk_tree_store_set ( GTK_TREE_STORE ( model ), &iter, BRK_ENABLED, enabled, -1 );
}


/*
 * 
 * drag & drop - reordering
 * 
 */

G_MODULE_EXPORT void on_dbg_breakpoints_treeview_drag_begin ( GtkWidget *widget, GdkDragContext *context, gpointer user_data ) {
    //printf ( "%s()\n", __func__ );
    g_uibpoints.dnd_action = UIPBDND_ACTION;
    g_uibpoints.dnd_moved_id = -1;
}


G_MODULE_EXPORT void on_dbg_breakpoints_treeview_drag_data_received ( GtkWidget *widget, GdkDragContext *context, gint x, gint y, GtkSelectionData *data, guint info, guint time, gpointer user_data ) {
    //    printf ( "%s()\n", __func__ );

    if ( g_uibpoints.dnd_moved_id == -1 ) {
        //GtkTreeModel *model = GTK_TREE_MODEL ( ui_get_object ( "dbg_breakpoints_treestore" ) );
        GtkTreeModel *tree_model = gtk_tree_view_get_model ( GTK_TREE_VIEW ( widget ) );

        GtkTreePath *path;
        gtk_tree_get_row_drag_data ( data, &tree_model, &path );
        //printf ( "ret: %s\n", gtk_tree_path_to_string ( path ) );
        //printf ( "indices: %d\n", gtk_tree_path_get_indices ( path ) [ gtk_tree_path_get_depth ( path ) - 1 ] );
        //printf ( "depth: %d\n", gtk_tree_path_get_depth ( path ) );

        g_uibpoints.dnd_src_position = gtk_tree_path_get_indices ( path ) [ gtk_tree_path_get_depth ( path ) - 1 ];

        GtkTreeIter iter;

        gtk_tree_model_get_iter ( tree_model, &iter, path );
        gtk_tree_path_free ( path );

        GValue gv_id = G_VALUE_INIT;
        gtk_tree_model_get_value ( tree_model, &iter, BRK_ID, &gv_id );
        g_uibpoints.dnd_moved_id = g_value_get_uint ( &gv_id );

        GtkTreeIter parent_iter;

        if ( gtk_tree_model_iter_parent ( tree_model, &parent_iter, &iter ) ) {
            GValue gv_id = G_VALUE_INIT;
            gtk_tree_model_get_value ( tree_model, &parent_iter, BRK_ID, &gv_id );
            g_uibpoints.dnd_src_parent_id = g_value_get_uint ( &gv_id );
        } else {
            g_uibpoints.dnd_src_parent_id = -1;
        };
    };
}


G_MODULE_EXPORT void on_dbg_breakpoints_treeview_drag_end ( GtkWidget *widget, GdkDragContext *context, gpointer user_data ) {
    //    printf ( "%s()\n", __func__ );

    GtkTreeModel *model = gtk_tree_view_get_model ( GTK_TREE_VIEW ( widget ) );
    GtkTreeIter moved_iter;

    if ( g_uibpoints.dnd_action == UIPBDND_BAD_DESTINATION ) {
        //printf ( "Bad DND destination! Restoring previous state! moved id: %d, prev. parent id: %d\n", g_uibpoints.dnd_moved_id, g_uibpoints.dnd_src_parent_id );

        GtkTreeIter src_parent_iter;
        GtkTreeIter *old_parent = NULL;

        if ( g_uibpoints.dnd_src_parent_id != -1 ) {
            gboolean parent_found = ui_breakpoints_get_bpoint_iter_by_id ( model, &src_parent_iter, NULL, g_uibpoints.dnd_src_parent_id );
            assert ( parent_found == TRUE );
            old_parent = &src_parent_iter;
        };
        gboolean moved_found = ui_breakpoints_get_bpoint_iter_by_id ( model, &moved_iter, NULL, g_uibpoints.dnd_moved_id );
        assert ( moved_found == TRUE );

        //printf ( "restore prepared\n" );
        GtkTreeIter iter;
        gtk_tree_store_insert ( GTK_TREE_STORE ( model ), &iter, old_parent, g_uibpoints.dnd_src_position );

        ui_breakpoints_copy_iter ( model, &iter, &moved_iter );

        gtk_tree_store_remove ( GTK_TREE_STORE ( model ), &moved_iter );

        ui_breakpoints_select_iter ( &iter );

    } else {
        gboolean moved_found = ui_breakpoints_get_bpoint_iter_by_id ( model, &moved_iter, NULL, g_uibpoints.dnd_moved_id );
        assert ( moved_found == TRUE );
        ui_breakpoints_select_iter ( &moved_iter );
    }
    g_uibpoints.dnd_action = UIPBDND_DONE;
}


G_MODULE_EXPORT gboolean on_dbg_breakpoints_treeview_button_press_event ( GtkWidget *widget, GdkEventButton *event, gpointer user_data ) {

    if ( 3 == event->button ) {
#ifdef WINDOWS
        // win32 a win64 GTK ve stavajici verzi umi jen gtk_menu_popup, ktery je vsak v novych verzich deprecated
        gtk_menu_popup ( GTK_MENU ( ui_get_widget ( "bpt_menu" ) ), NULL, NULL, NULL, NULL, 0, 0 );
#else
        gtk_menu_popup_at_pointer ( GTK_MENU ( ui_get_widget ( "bpt_menu" ) ), (GdkEvent*) event );
#endif

        GtkTreeModel *model = gtk_tree_view_get_model ( GTK_TREE_VIEW ( widget ) );
        GtkTreeSelection *selection = gtk_tree_view_get_selection ( ui_get_tree_view ( "dbg_breakpoints_treeview" ) );
        GtkTreeIter iter;
        gboolean sensitive = FALSE;
        if ( gtk_tree_selection_get_selected ( selection, &model, &iter ) ) {
            sensitive = TRUE;
        };
        gtk_widget_set_sensitive ( ui_get_widget ( "bpt_edit_menuitem" ), sensitive );
        gtk_widget_set_sensitive ( ui_get_widget ( "bpt_delete_menuitem" ), sensitive );

    };
    return FALSE;
}


G_MODULE_EXPORT void on_bpt_edit_menuitem_activate ( GtkMenuItem *menuitem, gpointer data ) {
    (

      void) menuitem;
    (void) data;

    GtkTreeModel *model = GTK_TREE_MODEL ( ui_get_object ( "dbg_breakpoints_treestore" ) );
    GtkTreeSelection *selection = gtk_tree_view_get_selection ( ui_get_tree_view ( "dbg_breakpoints_treeview" ) );
    GtkTreeIter iter;
    gtk_tree_selection_get_selected ( selection, &model, &iter );

    ui_breakpoints_edit_iter ( model, &iter );
}


G_MODULE_EXPORT void on_bpt_collapse_menuitem_activate ( GtkMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
    gtk_tree_view_collapse_all ( ui_get_tree_view ( "dbg_breakpoints_treeview" ) );
}


G_MODULE_EXPORT void on_bpt_expand_menuitem_activate ( GtkMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
    gtk_tree_view_expand_all ( ui_get_tree_view ( "dbg_breakpoints_treeview" ) );
}


G_MODULE_EXPORT void on_bpt_add_event_menuitem_activate ( GtkMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
    ui_breakpoints_add_new ( BRKTYPE_EVENT );
}


G_MODULE_EXPORT void on_bpt_add_group_menuitem_activate ( GtkMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
    ui_breakpoints_add_new ( BRKTYPE_GROUP );
}


G_MODULE_EXPORT void on_bpt_delete_menuitem_activate ( GtkMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;

    GtkTreeModel *model = GTK_TREE_MODEL ( ui_get_object ( "dbg_breakpoints_treestore" ) );
    GtkTreeSelection *selection = gtk_tree_view_get_selection ( ui_get_tree_view ( "dbg_breakpoints_treeview" ) );
    GtkTreeIter iter;
    gtk_tree_selection_get_selected ( selection, &model, &iter );

    ui_breakpoints_remove_item_from_bptable ( model, &iter );

    gtk_tree_store_remove ( GTK_TREE_STORE ( model ), &iter );
}


G_MODULE_EXPORT void on_bpt_delete_all_menuitem_activate ( GtkMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;

    GtkTreeModel *model = GTK_TREE_MODEL ( ui_get_object ( "dbg_breakpoints_treestore" ) );
    GtkTreeIter iter;

    while ( gtk_tree_model_get_iter_first ( model, &iter ) ) {
        gtk_tree_store_remove ( GTK_TREE_STORE ( model ), &iter );
    };

    breakpoints_clear_all ( );
}


G_MODULE_EXPORT void on_dbg_breakpoints_treeview_row_activated ( GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data ) {


    GtkTreeModel *model = gtk_tree_view_get_model ( tree_view );
    GtkTreeIter iter;
    gtk_tree_model_get_iter ( model, &iter, path );

    ui_breakpoints_edit_iter ( model, &iter );
}


/*
 * 
 * Udalosti btp_settings
 * 
 */

void bpt_settings_hide_window ( void ) {
    GtkWidget *w = ui_get_widget ( "bpt_settings_window" );
    ui_main_win_get_pos ( GTK_WINDOW ( w ), &g_uibpoints.settings_pos );
    gtk_widget_hide ( w );
}


G_MODULE_EXPORT gboolean on_bpt_settings_window_delete_event ( GtkWidget *widget, GdkEvent *event, gpointer user_data ) {
    bpt_settings_hide_window ( );
    return TRUE;
}


G_MODULE_EXPORT gboolean on_bpt_settings_window_key_press_event ( GtkWidget *widget, GdkEventKey *event, gpointer user_data ) {

    if ( event->keyval == GDK_KEY_Escape ) {
        bpt_settings_hide_window ( );
        return TRUE;
    };

    return FALSE;
}


G_MODULE_EXPORT void on_bpt_settings_close_button_clicked ( GtkButton *button, gpointer user_data ) {
    bpt_settings_hide_window ( );
}


G_MODULE_EXPORT void on_bpt_settings_ok_button_clicked ( GtkButton *button, gpointer user_data ) {

    GtkTreeIter iter;
    GtkTreeModel *tree_model = gtk_tree_view_get_model ( ui_get_tree_view ( "dbg_breakpoints_treeview" ) );
    en_BRKTYPE type = BRKTYPE_GROUP;
    GtkTreeIter *parent_iter = NULL;
    GtkTreeIter p_iter;

    if ( g_uibpoints.edit_id != -1 ) {
        gboolean found = ui_breakpoints_get_bpoint_iter_by_id ( tree_model, &iter, NULL, g_uibpoints.edit_id );
        assert ( found == TRUE );
        GValue gv_type = G_VALUE_INIT;
        gtk_tree_model_get_value ( tree_model, &iter, BRK_TYPE, &gv_type );
        type = g_value_get_uint ( &gv_type );
    } else {
        if ( g_uibpoints.add_to_parent_id != -1 ) {
            gboolean found = ui_breakpoints_get_bpoint_iter_by_id ( tree_model, &p_iter, NULL, g_uibpoints.add_to_parent_id );
            assert ( found == TRUE );
            parent_iter = &p_iter;
        };
        type = g_uibpoints.add_type;
    };

    GdkRGBA fg_color;
    GdkRGBA bg_color;

    gtk_color_chooser_get_rgba ( GTK_COLOR_CHOOSER ( ui_get_widget ( "bpt_fg_colorbutton" ) ), &fg_color );
    gtk_color_chooser_get_rgba ( GTK_COLOR_CHOOSER ( ui_get_widget ( "bpt_bg_colorbutton" ) ), &bg_color );

    if ( BRKTYPE_GROUP == type ) {

        if ( g_uibpoints.edit_id == -1 ) {
            ui_breakpoints_add_group ( tree_model, &iter,
                                       parent_iter,
                                       g_uibpoints.id++,
                                       gtk_entry_get_text ( ui_get_entry ( "bpt_name_entry" ) ),
                                       gtk_toggle_button_get_active ( ui_get_toggle ( "bpt_enabled_checkbutton" ) ),
                                       &fg_color,
                                       &bg_color );
        } else {
            gtk_tree_store_set ( GTK_TREE_STORE ( tree_model ), &iter,
                                 BRK_NAME, gtk_entry_get_text ( ui_get_entry ( "bpt_name_entry" ) ),
                                 BRK_ENABLED, gtk_toggle_button_get_active ( ui_get_toggle ( "bpt_enabled_checkbutton" ) ),
                                 BRK_FG_COLOR, &fg_color,
                                 BRK_BG_COLOR, &bg_color,
                                 BRK_FG_R, fg_color.red,
                                 BRK_FG_G, fg_color.green,
                                 BRK_FG_B, fg_color.blue,
                                 BRK_BG_R, bg_color.red,
                                 BRK_BG_G, bg_color.green,
                                 BRK_BG_B, bg_color.blue,
                                 -1 );
        };

    } else {

        const gchar *addr_entry_txt = gtk_entry_get_text ( ui_get_entry ( "bpt_addr_entry" ) );
        if ( 0 == strlen ( addr_entry_txt ) ) {
            /* err */
            return;
        };

        unsigned addr = debuger_hextext_to_uint32 ( addr_entry_txt );
        char addr_txt [ 7 ];
        g_sprintf ( addr_txt, "0x%04X", addr );

        int flag_create_default_name = 0;

        /* pokud je name prazdne, tak vytvorime default */
        if ( 0 == strlen ( gtk_entry_get_text ( ui_get_entry ( "bpt_name_entry" ) ) ) ) flag_create_default_name = 1;

        if ( 0 == flag_create_default_name ) {
            if ( g_uibpoints.edit_id != -1 ) {
                GValue gv_name = G_VALUE_INIT;
                GValue gv_addr = G_VALUE_INIT;
                gtk_tree_model_get_value ( tree_model, &iter, BRK_NAME, &gv_name );
                gtk_tree_model_get_value ( tree_model, &iter, BRK_ADDR, &gv_addr );

                char *old_event_name = ui_breakpoints_alocate_new_event_defname ( g_value_get_uint ( &gv_addr ) );

                if ( 0 == strcmp ( gtk_entry_get_text ( ui_get_entry ( "bpt_name_entry" ) ), g_value_get_string ( &gv_name ) ) ) {
                    /* name nebylo zmeneno - pokud je v default formatu, tak vyrobime nove default name */
                    if ( 0 == strcmp ( old_event_name, g_value_get_string ( &gv_name ) ) ) flag_create_default_name = 1;
                    //} else {
                    /* name bylo zmeneno - aplikujeme co co je v entry */
                    //flag_create_default_name = 0;
                };

                g_free ( old_event_name );
            };
        };

        gchar *set_name = NULL;
        if ( 1 == flag_create_default_name ) {
            set_name = ui_breakpoints_alocate_new_event_defname ( addr );
        } else {
            set_name = g_strdup_printf ( "%s", gtk_entry_get_text ( ui_get_entry ( "bpt_name_entry" ) ) );
        };


        if ( g_uibpoints.edit_id == -1 ) {
            ui_breakpoints_add_event ( tree_model, &iter, parent_iter,
                                       g_uibpoints.id++,
                                       set_name,
                                       gtk_toggle_button_get_active ( ui_get_toggle ( "bpt_enabled_checkbutton" ) ),
                                       addr,
                                       &fg_color,
                                       &bg_color );
        } else {
            gtk_tree_store_set ( GTK_TREE_STORE ( tree_model ), &iter,
                                 BRK_NAME, set_name,
                                 BRK_ADDR, addr,
                                 BRK_ADDR_TXT, addr_txt,
                                 BRK_ENABLED, gtk_toggle_button_get_active ( ui_get_toggle ( "bpt_enabled_checkbutton" ) ),
                                 BRK_FG_COLOR, &fg_color,
                                 BRK_BG_COLOR, &bg_color,
                                 BRK_FG_R, fg_color.red,
                                 BRK_FG_G, fg_color.green,
                                 BRK_FG_B, fg_color.blue,
                                 BRK_BG_R, bg_color.red,
                                 BRK_BG_G, bg_color.green,
                                 BRK_BG_B, bg_color.blue,
                                 -1 );
        };

        g_free ( set_name );

    };

    //printf ( "r: %lf\nr: %lf\nr: %lf\n\n", fg_color.red, fg_color.green, fg_color.blue );

    bpt_settings_hide_window ( );

    ui_breakpoints_select_iter ( &iter );
}


G_MODULE_EXPORT void on_bpt_add_event_button_clicked ( GtkButton *button, gpointer user_data ) {

    const gchar *addr_entry_txt = gtk_entry_get_text ( ui_get_entry ( "bpt_add_event_entry" ) );
    if ( 0 == strlen ( addr_entry_txt ) ) {
        /* err */
        return;
    };
    unsigned addr = debuger_hextext_to_uint32 ( addr_entry_txt );
    ui_breakpoints_simple_add_event ( addr );
    gtk_entry_set_text ( ui_get_entry ( "bpt_add_event_entry" ), "" );
}


G_MODULE_EXPORT gboolean on_bpt_add_event_entry_focus_in_event ( GtkWidget *widget, GdkEvent *event, gpointer user_data ) {
    //    printf ( "focus in\n" );
    g_uibpoints.add_event_focus = 1;
    return FALSE;
}


G_MODULE_EXPORT gboolean on_bpt_add_event_entry_focus_out_event ( GtkWidget *widget, GdkEvent *event, gpointer user_data ) {
    //    printf ( "focus out\n" );
    g_uibpoints.add_event_focus = 0;
    return FALSE;
}


G_MODULE_EXPORT void on_bpt_save_button_clicked ( GtkButton *button, gpointer user_data ) {
    ui_breakpoints_save ( );
}


G_MODULE_EXPORT void on_bpt_autosave_checkbutton_toggled ( GtkToggleButton *togglebutton, gpointer user_data ) {
    g_debugger.auto_save_breakpoints = gtk_toggle_button_get_active ( togglebutton );
}

#endif
