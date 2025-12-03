/* 
 * File:   ui_memload.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 12. června 2018, 12:37
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
#include <glib/gstdio.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#ifdef MZ800EMU_CFG_DEBUGGER_ENABLED

#include "z80ex/include/z80ex.h"
#include "ui_membrowser.h"
#include "ui/ui_main.h"
#include "ui/ui_file_chooser.h"
#include "ui/ui_hexeditable.h"
#include "debugger/debugger.h"
#include "ramdisk/ramdisk.h"
#include "gdg/gdg.h"
#include "cmt/cmthack.h"
#include "memory/memory.h"
#include "gdg/framebuffer.h"
#include "iface_sdl//iface_sdl.h"
#include "ui_debugger.h"
#include "ui_memload.h"
#include "ui/ui_utils.h"



static FILE *g_memload_file;
static long g_memload_fsize;
static gboolean g_ui_memload_offset_lock = FALSE;
static gboolean g_ui_memload_memaloc_lock = FALSE;
static uint8_t *g_ui_memload_dst;
static uint32_t g_ui_memload_dst_size;
static ui_memload_cb g_ui_memload_cb;
static void *g_ui_memload_cb_data;


static void ui_memload_ok_button_sensitivity ( void ) {
    GtkWidget *button = ui_get_widget ( "dbg_memload_ok_button" );
    uint32_t size = debuger_hextext_to_uint32 ( gtk_entry_get_text ( ui_get_entry ( "dbg_memload_size_hex_entry" ) ) );
    gboolean button_is_enabled = ( size ) ? TRUE : FALSE;
    gtk_widget_set_sensitive ( button, button_is_enabled );
}


static void ui_memload_window_hide ( void ) {
    GtkWidget *window = ui_get_widget ( "dbg_memload_window" );
    gtk_widget_hide ( window );
}


G_MODULE_EXPORT gboolean on_dbg_memload_window_delete_event ( GtkWidget *widget, GdkEvent *event, gpointer user_data ) {
    ui_memload_window_hide ( );
    fclose ( g_memload_file );
    return TRUE;
}


G_MODULE_EXPORT void on_dbg_memload_close_button_clicked ( GtkButton *button, gpointer data ) {
    (void) button;
    (void) data;
    ui_memload_window_hide ( );
    fclose ( g_memload_file );
}


static void ui_memload_check_size ( void ) {
    uint32_t offset = debuger_hextext_to_uint32 ( gtk_entry_get_text ( ui_get_entry ( "dbg_memload_offset_hex_entry" ) ) );
    uint32_t size = debuger_hextext_to_uint32 ( gtk_entry_get_text ( ui_get_entry ( "dbg_memload_size_hex_entry" ) ) );
    if ( size > ( g_memload_fsize - offset ) ) {
        char buff[9];
        snprintf ( buff, sizeof ( buff ), "%04X", (uint32_t) ( g_memload_fsize - offset ) );
        gtk_entry_set_text ( ui_get_entry ( "dbg_memload_size_hex_entry" ), buff );
    };
}


G_MODULE_EXPORT void on_dbg_memload_offset_hex_entry_changed ( GtkEditable *ed, gpointer user_data ) {
    if ( g_ui_memload_offset_lock ) return;
    g_ui_memload_offset_lock = TRUE;

    ui_hexeditable_changed ( ed, user_data );

    uint32_t offset = debuger_hextext_to_uint32 ( gtk_entry_get_text ( ui_get_entry ( "dbg_memload_offset_hex_entry" ) ) );

    char buff[11];

    if ( offset >= g_memload_fsize ) {
        offset = g_memload_fsize - 1;
        snprintf ( buff, sizeof ( buff ), "%X", offset );
        gtk_entry_set_text ( ui_get_entry ( "dbg_memload_offset_hex_entry" ), buff );
    }

    snprintf ( buff, sizeof ( buff ), "%u", offset );

    gtk_entry_set_text ( ui_get_entry ( "dbg_memload_offset_dec_entry" ), buff );
    g_ui_memload_offset_lock = FALSE;

    ui_memload_check_size ( );
}


G_MODULE_EXPORT void on_dbg_memload_offset_dec_entry_changed ( GtkEditable *ed, gpointer user_data ) {
    if ( g_ui_memload_offset_lock ) return;
    g_ui_memload_offset_lock = TRUE;

    ui_digiteditable_changed ( ed, user_data );

    uint32_t offset = atoi ( gtk_entry_get_text ( ui_get_entry ( "dbg_memload_offset_dec_entry" ) ) );

    char buff[11];

    if ( offset >= g_memload_fsize ) {
        offset = g_memload_fsize - 1;
        snprintf ( buff, sizeof ( buff ), "%u", offset );
        gtk_entry_set_text ( ui_get_entry ( "dbg_memload_offset_dec_entry" ), buff );
    }
    snprintf ( buff, sizeof ( buff ), "%X", offset );
    gtk_entry_set_text ( ui_get_entry ( "dbg_memload_offset_hex_entry" ), buff );
    g_ui_memload_offset_lock = FALSE;

    ui_memload_check_size ( );
}


static void ui_memload_set_hex_size ( uint32_t size ) {
    g_ui_memload_memaloc_lock = TRUE;
    char buff[9];
    snprintf ( buff, sizeof ( buff ), "%04X", size );
    gtk_entry_set_text ( ui_get_entry ( "dbg_memload_size_hex_entry" ), buff );
    g_ui_memload_memaloc_lock = FALSE;
    ui_memload_ok_button_sensitivity ( );
}


static void ui_memload_set_dec_size ( uint32_t size ) {
    g_ui_memload_memaloc_lock = TRUE;
    char buff[11];
    snprintf ( buff, sizeof ( buff ), "%d", size );
    gtk_entry_set_text ( ui_get_entry ( "dbg_memload_size_dec_entry" ), buff );
    g_ui_memload_memaloc_lock = FALSE;
    ui_memload_ok_button_sensitivity ( );
}


static void ui_memload_set_size ( uint32_t size ) {
    ui_memload_set_hex_size ( size );
    ui_memload_set_dec_size ( size );
}


static void ui_memload_set_to ( uint32_t to ) {
    g_ui_memload_memaloc_lock = TRUE;
    char buff[9];
    snprintf ( buff, sizeof ( buff ), "%04X", to );
    gtk_entry_set_text ( ui_get_entry ( "dbg_memload_to_entry" ), buff );
    g_ui_memload_memaloc_lock = FALSE;
}


G_MODULE_EXPORT void on_dbg_memload_from_entry_changed ( GtkEditable *ed, gpointer user_data ) {
    if ( g_ui_memload_memaloc_lock ) return;
    ui_hexeditable_changed ( ed, user_data );

    if ( !gtk_entry_get_text_length ( ui_get_entry ( "dbg_memload_to_entry" ) ) ) {
        ui_memload_ok_button_sensitivity ( );
        return;
    };

    uint32_t addr_from = debuger_hextext_to_uint32 ( gtk_entry_get_text ( ui_get_entry ( "dbg_memload_from_entry" ) ) );
    uint32_t addr_to = debuger_hextext_to_uint32 ( gtk_entry_get_text ( ui_get_entry ( "dbg_memload_to_entry" ) ) );

    uint32_t mask = g_ui_memload_dst_size - 1;
    if ( addr_from > ( g_ui_memload_dst_size - 1 ) ) {
        addr_from = ( g_ui_memload_dst_size - 1 );
        char buff[9];
        snprintf ( buff, sizeof ( buff ), "%04X", addr_from );
        g_ui_memload_memaloc_lock = TRUE;
        gtk_entry_set_text ( ui_get_entry ( "dbg_memload_from_entry" ), buff );
        g_ui_memload_memaloc_lock = FALSE;
    };

    ui_memload_set_size ( ( addr_to - addr_from ) & mask );
    ui_memload_check_size ( );
}


static uint32_t ui_memload_get_from ( void ) {
    if ( !gtk_entry_get_text_length ( ui_get_entry ( "dbg_memload_from_entry" ) ) ) {
        g_ui_memload_memaloc_lock = TRUE;
        gtk_entry_set_text ( ui_get_entry ( "dbg_memload_from_entry" ), "0000" );
        g_ui_memload_memaloc_lock = FALSE;
    };

    return debuger_hextext_to_uint32 ( gtk_entry_get_text ( ui_get_entry ( "dbg_memload_from_entry" ) ) );
}


G_MODULE_EXPORT void on_dbg_memload_to_entry_changed ( GtkEditable *ed, gpointer user_data ) {
    if ( g_ui_memload_memaloc_lock ) return;
    ui_hexeditable_changed ( ed, user_data );

    uint32_t addr_from = ui_memload_get_from ( );
    uint32_t addr_to = debuger_hextext_to_uint32 ( gtk_entry_get_text ( ui_get_entry ( "dbg_memload_to_entry" ) ) );

    uint32_t mask = g_ui_memload_dst_size - 1;
    if ( addr_to > ( g_ui_memload_dst_size - 1 ) ) {
        addr_to = ( g_ui_memload_dst_size - 1 );
        char buff[9];
        snprintf ( buff, sizeof ( buff ), "%04X", addr_to );
        g_ui_memload_memaloc_lock = TRUE;
        gtk_entry_set_text ( ui_get_entry ( "dbg_memload_to_entry" ), buff );
        g_ui_memload_memaloc_lock = FALSE;
    };

    ui_memload_set_size ( ( addr_to - addr_from ) & mask );
    ui_memload_check_size ( );
}


G_MODULE_EXPORT void on_dbg_memload_size_hex_entry_changed ( GtkEditable *ed, gpointer user_data ) {
    if ( g_ui_memload_memaloc_lock ) return;
    ui_hexeditable_changed ( ed, user_data );

    uint32_t addr_from = ui_memload_get_from ( );
    uint32_t size = debuger_hextext_to_uint32 ( gtk_entry_get_text ( ui_get_entry ( "dbg_memload_size_hex_entry" ) ) );

    uint32_t mask = g_ui_memload_dst_size - 1;
    if ( size > ( g_ui_memload_dst_size - 1 ) ) {
        size = ( g_ui_memload_dst_size - 1 );
        char buff[9];
        snprintf ( buff, sizeof ( buff ), "%04X", size );
        g_ui_memload_memaloc_lock = TRUE;
        gtk_entry_set_text ( ui_get_entry ( "dbg_memload_size_hex_entry" ), buff );
        g_ui_memload_memaloc_lock = FALSE;
    };

    ui_memload_set_to ( ( addr_from + size ) & mask );
    ui_memload_set_dec_size ( size );
    ui_memload_check_size ( );
}


G_MODULE_EXPORT void on_dbg_memload_size_dec_entry_changed ( GtkEditable *ed, gpointer user_data ) {
    if ( g_ui_memload_memaloc_lock ) return;
    g_ui_memload_memaloc_lock = TRUE;

    ui_digiteditable_changed ( ed, user_data );

    uint32_t addr_from = ui_memload_get_from ( );
    uint32_t atoi_size = atoi ( gtk_entry_get_text ( ui_get_entry ( "dbg_memload_size_dec_entry" ) ) );

    uint32_t mask = g_ui_memload_dst_size - 1;
    if ( atoi_size > ( g_ui_memload_dst_size - 1 ) ) {
        atoi_size = ( g_ui_memload_dst_size - 1 );
        char buff[11];
        snprintf ( buff, sizeof ( buff ), "%d", atoi_size );

        gtk_entry_set_text ( ui_get_entry ( "dbg_memload_size_dec_entry" ), buff );
    };
    g_ui_memload_memaloc_lock = FALSE;

    ui_memload_set_to ( ( addr_from + atoi_size ) & mask );
    ui_memload_set_hex_size ( atoi_size );
    ui_memload_check_size ( );
}


void ui_memload_window_show ( void ) {

    fseek ( g_memload_file, 0, SEEK_END );
    g_memload_fsize = ftell ( g_memload_file );
    if ( g_memload_fsize > 0xffffffff ) {
        ui_show_error ( "This file is too big for this operation.\n" );
        return;
    };

    char buff[11];

    snprintf ( buff, sizeof ( buff ), "0x%04X", (unsigned) g_memload_fsize );
    gtk_label_set_text ( ui_get_label ( "dbg_memload_size_hex_label" ), buff );

    snprintf ( buff, sizeof ( buff ), "%d", (unsigned) g_memload_fsize );
    gtk_label_set_text ( ui_get_label ( "dbg_memload_size_dec_label" ), buff );

    gtk_entry_set_text ( ui_get_entry ( "dbg_memload_offset_hex_entry" ), "0" );

    uint32_t memmax = g_ui_memload_dst_size - 1;

    snprintf ( buff, sizeof ( buff ), "%04X", ( g_memload_fsize < memmax ) ? (unsigned) g_memload_fsize : memmax );
    gtk_entry_set_text ( ui_get_entry ( "dbg_memload_size_hex_entry" ), buff );

    gtk_entry_set_text ( ui_get_entry ( "dbg_memload_offset_hex_entry" ), "0" );

    ui_memload_ok_button_sensitivity ( );

    GtkWidget *window = ui_get_widget ( "dbg_memload_window" );
    gtk_widget_grab_focus ( ui_get_widget ( "dbg_memload_from_entry" ) );
    gtk_widget_show ( window );
}


void ui_memload_select_file ( uint8_t *dst, ui_memload_cb cb, uint32_t dst_size, void *user_data ) {

    g_ui_memload_dst = dst;
    g_ui_memload_dst_size = dst_size;
    g_ui_memload_cb = cb;
    g_ui_memload_cb_data = user_data;

    static char *last_filepath = NULL; //TODO: bylo by slusne to pak pri exitu po sobe uklidit
    char *filepath = ui_file_chooser_open_file ( last_filepath, NULL, "Load file into memory", ui_get_window ( "dbg_membrowser_window" ), FC_MODE_OPEN, NULL );
    if ( !filepath ) return;

    int len = strlen ( filepath ) + 1;
    if ( !last_filepath ) {
        last_filepath = (char*) ui_utils_mem_alloc0 ( len );
    } else {
        last_filepath = (char*) ui_utils_mem_realloc ( last_filepath, len );
    };
    strncpy ( last_filepath, filepath, len );

    g_memload_file = g_fopen ( filepath, "rb" );
    if ( g_memload_file ) {
        ui_memload_window_show ( );
    } else {
        ui_show_error ( "Can't open file '%s'\n", filepath );
    };
    ui_utils_mem_free ( filepath );
}


G_MODULE_EXPORT void on_dbg_memload_ok_button_clicked ( GtkButton *button, gpointer user_data ) {
    ui_memload_window_hide ( );

    uint32_t offset = debuger_hextext_to_uint32 ( gtk_entry_get_text ( ui_get_entry ( "dbg_memload_offset_hex_entry" ) ) );
    uint32_t size = debuger_hextext_to_uint32 ( gtk_entry_get_text ( ui_get_entry ( "dbg_memload_size_hex_entry" ) ) );
    uint32_t addr = debuger_hextext_to_uint32 ( gtk_entry_get_text ( ui_get_entry ( "dbg_memload_from_entry" ) ) );

    fseek ( g_memload_file, offset, SEEK_SET );

    uint8_t *dst = NULL;

    if ( g_ui_memload_dst ) {
        dst = &g_ui_memload_dst[addr];
    } else {
        dst = ui_utils_mem_alloc ( size );
    };

    uint32_t read_size = fread ( dst, 1, size, g_memload_file );

    fclose ( g_memload_file );
    if ( read_size != size ) {
        ui_show_error ( "Can't read data from file\n" );
        if ( !g_ui_memload_dst ) g_free ( dst );
        return;
    };

    if ( g_ui_memload_cb ) g_ui_memload_cb ( addr, dst, size, g_ui_memload_cb_data );

    printf ( "\nLoad done.\n" );

    if ( TEST_EMULATION_PAUSED ) {
        ui_main_debugger_windows_refresh ( );
    };
}

#endif
