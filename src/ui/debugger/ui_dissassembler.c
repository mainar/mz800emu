/* 
 * File:   ui_dissassembler.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 6. června 2018, 19:55
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

#include <stdio.h>
#include <string.h>
#include "ui/ui_main.h"
#include "ui/ui_file_chooser.h"
#include "ui/ui_hexeditable.h"
#include "ui_dissassembler.h"
#include "z80ex/include/z80ex_dasm.h"
#include "debugger/debugger.h"
#include "ui_debugger.h"
#include "z80ex_common.h"
#include "ui/ui_utils.h"


static void ui_dissassembler_hide_window ( void ) {
    GtkWidget *window = ui_get_widget ( "dissassembler_window" );
    gtk_widget_hide ( window );
}


G_MODULE_EXPORT gboolean on_dissassembler_window_delete_event ( GtkWidget *widget, GdkEvent *event, gpointer user_data ) {
    ui_dissassembler_hide_window ( );
    return TRUE;
}


G_MODULE_EXPORT gboolean on_dissassembler_window_key_press_event ( GtkWidget *widget, GdkEventKey *event, gpointer user_data ) {
    if ( event->keyval == GDK_KEY_Escape ) {
        ui_dissassembler_hide_window ( );
        return TRUE;
    };
    return FALSE;
}


G_MODULE_EXPORT void on_dissassembler_close_imagemenuitem_activate ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
    ui_dissassembler_hide_window ( );
}


G_MODULE_EXPORT void on_dissassembler_save_imagemenuitem_activate ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;

    char *filepath = ui_file_chooser_open_file ( "dissassembled.s", ui_filechooser_get_last_generic_dir ( ), "Save File", ui_get_window ( "dissassembler_window" ), FC_MODE_SAVE, NULL );
    if ( !filepath ) return;

    printf ( "Save: %s\n", filepath );
    FILE *file = fopen ( filepath, "w+b" );
    if ( !file ) {
        fprintf ( stderr, "%s():%d - Can't open file '%s'\n", __func__, __LINE__, filepath );
    } else {
        GtkWidget *view = ui_get_widget ( "dissassembler_textview" );
        GtkTextBuffer *buffer = gtk_text_view_get_buffer ( GTK_TEXT_VIEW ( view ) );
        GtkTextIter start;
        GtkTextIter end;
        gtk_text_buffer_get_start_iter ( buffer, &start );
        gtk_text_buffer_get_end_iter ( buffer, &end );
        char *text = gtk_text_buffer_get_text ( buffer, &start, &end, FALSE );
        fprintf ( file, "%s", text );
        g_free ( text );
        fclose ( file );
    };

    char *dirpath = g_path_get_dirname ( filepath );
    ui_filechooser_set_last_generic_dir ( dirpath );

    ui_utils_mem_free ( dirpath );
    ui_utils_mem_free ( filepath );
}


static void ui_dissassembler_clear_textview ( void ) {
    GtkWidget *view = ui_get_widget ( "dissassembler_textview" );
    GtkTextBuffer *buffer = gtk_text_view_get_buffer ( GTK_TEXT_VIEW ( view ) );
    gtk_text_buffer_set_text ( buffer, "", 0 );
}


void ui_dissassembler_show_window ( void ) {
    GtkWidget *window = ui_get_widget ( "dissassembler_window" );
    if ( gtk_widget_is_visible ( window ) ) return;
    GtkWidget *entry_from = ui_get_widget ( "dissassembler_from_entry" );
    gtk_entry_set_text ( GTK_ENTRY ( entry_from ), "" );
    gtk_widget_grab_focus ( entry_from );
    gtk_entry_set_text ( ui_get_entry ( "dissassembler_to_entry" ), "" );
    ui_dissassembler_clear_textview ( );
    gtk_widget_show ( window );
}


static void ui_dissassembler_start ( void ) {
    ui_dissassembler_clear_textview ( );

    Z80EX_WORD addr = debuger_hextext_to_uint32 ( gtk_entry_get_text ( ui_get_entry ( "dissassembler_from_entry" ) ) );
    Z80EX_WORD addr_to = debuger_hextext_to_uint32 ( gtk_entry_get_text ( ui_get_entry ( "dissassembler_to_entry" ) ) );
    int dissassemble_size = (Z80EX_WORD) addr_to - addr;

    if ( !dissassemble_size ) return;

    gboolean ram_only = gtk_toggle_button_get_active ( GTK_TOGGLE_BUTTON ( ui_get_widget ( "dissassembler_ram_only_checkbutton" ) ) );
    gboolean add_ticks = gtk_toggle_button_get_active ( GTK_TOGGLE_BUTTON ( ui_get_widget ( "dissassembler_add_ticks_checkbutton" ) ) );

    z80ex_dasm_readbyte_cb memory_read_cb = ( ram_only ) ? debugger_dasm_pure_ram_read_cb : debugger_dasm_read_cb;

    GtkWidget *view = ui_get_widget ( "dissassembler_textview" );
    GtkTextBuffer *buffer = gtk_text_view_get_buffer ( GTK_TEXT_VIEW ( view ) );

    while ( dissassemble_size > 0 ) {
        char mnemonic [ DEBUGGER_MNEMONIC_MAXLEN ];
        int t_states, t_states2;
        unsigned bytecode_length = z80ex_dasm ( mnemonic, DEBUGGER_MNEMONIC_MAXLEN - 1, 0, &t_states, &t_states2, memory_read_cb, addr, NULL );

        char ticks_buff[20];
        char *ticks_txt = "";
        if ( add_ticks ) {
            if ( t_states2 ) {
                snprintf ( ticks_buff, sizeof ( ticks_buff ), "\t; ticks: %d/%d", t_states, t_states2 );
            } else {
                snprintf ( ticks_buff, sizeof ( ticks_buff ), "\t; ticks: %d", t_states );
            };
            ticks_txt = ticks_buff;
        };

        char dissassembled_txt[100];
        snprintf ( dissassembled_txt, sizeof ( dissassembled_txt ), "%04X:\t%s%s\n", addr, mnemonic, ticks_txt );

        GtkTextIter iter;
        gtk_text_buffer_get_iter_at_line ( buffer, &iter, gtk_text_buffer_get_line_count ( buffer ) );
        gtk_text_buffer_insert ( buffer, &iter, dissassembled_txt, -1 );

        addr += bytecode_length;
        dissassemble_size -= bytecode_length;
    };

}


G_MODULE_EXPORT void on_dissassembler_start_button_clicked ( GtkButton *button, gpointer data ) {
    (void) button;
    (void) data;
    ui_dissassembler_start ( );
}


G_MODULE_EXPORT gboolean on_dissassembler_from_entry_key_press_event ( GtkWidget *widget, GdkEventKey *event, gpointer user_data ) {
    if ( ( event->keyval == GDK_KEY_Return ) || ( event->keyval == GDK_KEY_KP_Enter ) ) {
        if ( gtk_entry_get_text_length ( GTK_ENTRY ( widget ) ) ) {
            gtk_widget_grab_focus ( ui_get_widget ( "dissassembler_to_entry" ) );
            return TRUE;
        };
    };
    return FALSE;
}


G_MODULE_EXPORT gboolean on_dissassembler_to_entry_key_press_event ( GtkWidget *widget, GdkEventKey *event, gpointer user_data ) {
    if ( ( event->keyval == GDK_KEY_Return ) || ( event->keyval == GDK_KEY_KP_Enter ) ) {
        if ( gtk_entry_get_text_length ( GTK_ENTRY ( widget ) ) ) {
            ui_dissassembler_start ( );
            return TRUE;
        };
    };
    return FALSE;
}
