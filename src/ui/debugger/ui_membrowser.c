/* 
 * File:   ui_membrowser.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 14. června 2018, 12:08
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

#include <stdio.h>
#include <string.h>

#ifdef MZ800EMU_CFG_DEBUGGER_ENABLED

#include "../ui_main.h"
#include "z80ex/include/z80ex.h"
#include "memory/memory.h"
#include "sharpmz_ascii.h"
#include "debugger/debugger.h"
#include "ui_membrowser.h"
#include "gdg/gdg.h"
#include "iface_sdl/iface_sdl.h"
#include "ui_debugger.h"
#include "ramdisk/ramdisk.h"
#include "cmt/cmthack.h"
#include "ui/ui_hexeditable.h"
#include "ui_memload.h"
#include "ui_memsave.h"
#include "memory/memext.h"


#define MEMBROWSER_MEM_MAX  0x10000

#define MEMBROWSER_ROW_LENGTH (4 + (16 * 3) + (3 * 2) + 2 + 16 + 1)
#define MEMBROWSER_LINES_PER_PAGE  32
#define MEMBROWSER_ADDRESSES_PER_PAGE  (MEMBROWSER_LINES_PER_PAGE * 0x10)

static gboolean g_membrowser_initialised = FALSE;

st_UI_MEMBROWSER g_membrowser;

extern void ui_membrowser_show_page ( uint32_t page );


static uint32_t ui_membrowser_get_page_by_addr ( uint32_t addr ) {
    return ( addr / MEMBROWSER_ADDRESSES_PER_PAGE );
}


static uint32_t ui_membrowser_get_first_addr_on_page ( uint32_t page ) {
    return ( page * MEMBROWSER_ADDRESSES_PER_PAGE );
}


static uint32_t ui_membrowser_get_last_addr_on_page ( uint32_t page ) {
    return ui_membrowser_get_first_addr_on_page ( page ) + MEMBROWSER_ADDRESSES_PER_PAGE - 1;
}


static void ui_membrowser_get_cursor_position ( gint *row, gint *col ) {
    GtkWidget *view = ui_get_widget ( "dbg_membrowser_textview" );
    GtkTextBuffer *buffer = gtk_text_view_get_buffer ( GTK_TEXT_VIEW ( view ) );

    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_mark ( buffer, &iter, gtk_text_buffer_get_insert ( buffer ) );

    *row = gtk_text_iter_get_line ( &iter );
    *col = gtk_text_iter_get_line_offset ( &iter );
}


static uint32_t ui_membrowser_get_addr_by_offset ( uint32_t offset ) {

    gint row = offset / MEMBROWSER_ROW_LENGTH;
    gint col = offset % MEMBROWSER_ROW_LENGTH;

    col -= 5;
    uint32_t addr = ui_membrowser_get_first_addr_on_page ( g_membrowser.page ) + row * 0x10;
    if ( col > 52 ) {
        // MEMBROWSER_MODE_EDIT_ASCII
        addr += col - ( 52 + 3 );
    } else {
        // MEMBROWSER_MODE_EDIT_HEX
        switch ( col ) {
            case 0:
            case 1:
                addr += 0x00;
                break;
            case 3:
            case 4:
                addr += 0x01;
                break;
            case 6:
            case 7:
                addr += 0x02;
                break;
            case 9:
            case 10:
                addr += 0x03;
                break;
                /* | */
            case 14:
            case 15:
                addr += 0x04;
                break;
            case 17:
            case 18:
                addr += 0x05;
                break;
            case 20:
            case 21:
                addr += 0x06;
                break;
            case 23:
            case 24:
                addr += 0x07;
                break;
                /* | */
            case 28:
            case 29:
                addr += 0x08;
                break;
            case 31:
            case 32:
                addr += 0x09;
                break;
            case 34:
            case 35:
                addr += 0x0a;
                break;
            case 37:
            case 38:
                addr += 0x0b;
                break;
                /* | */
            case 42:
            case 43:
                addr += 0x0c;
                break;
            case 45:
            case 46:
                addr += 0x0d;
                break;
            case 48:
            case 49:
                addr += 0x0e;
                break;
            case 51:
            case 52:
                addr += 0x0f;
                break;
        };
    };
    return addr;
}


static uint32_t ui_membrowser_get_addr_by_cursor_position ( void ) {
    gint row, col;
    ui_membrowser_get_cursor_position ( &row, &col );
    uint32_t offset = col + ( row * MEMBROWSER_ROW_LENGTH );
    return ui_membrowser_get_addr_by_offset ( offset );
}


static uint32_t ui_membrowser_get_ascii_offset_by_addr ( uint32_t addr ) {
    uint32_t first_addr = ui_membrowser_get_first_addr_on_page ( g_membrowser.page );
    return ( ( addr - first_addr ) / 0x10 ) * MEMBROWSER_ROW_LENGTH + 5 + 52 + 3 + ( addr % 0x10 );
}


static uint32_t ui_membrowser_get_hex_offset_by_addr ( uint32_t addr ) {
    uint32_t first_addr = ui_membrowser_get_first_addr_on_page ( g_membrowser.page );
    uint32_t offset = ( ( addr - first_addr ) / 0x10 ) * MEMBROWSER_ROW_LENGTH;
    switch ( addr % 0x10 ) {
        case 0x00:
            offset += 0;
            break;
        case 0x01:
            offset += 3;
            break;
        case 0x02:
            offset += 6;
            break;
        case 0x03:
            offset += 9;
            break;
        case 0x04:
            offset += 14;
            break;
        case 0x05:
            offset += 17;
            break;
        case 0x06:
            offset += 20;
            break;
        case 0x07:
            offset += 23;
            break;
        case 0x08:
            offset += 28;
            break;
        case 0x09:
            offset += 31;
            break;
        case 0x0a:
            offset += 34;
            break;
        case 0x0b:
            offset += 37;
            break;
        case 0x0c:
            offset += 42;
            break;
        case 0x0d:
            offset += 45;
            break;
        case 0x0e:
            offset += 48;
            break;
        case 0x0f:
            offset += 51;
            break;
    };

    return offset + 5;
}


static void ui_membrowser_update_selected_page_label ( void ) {
    char buff[10];
    snprintf ( buff, sizeof ( buff ), "%d", g_membrowser.page + 1 );
    gtk_label_set_text ( ui_get_label ( "dbg_membrowser_page_label" ), buff );
}


static void ui_membrowser_update_selected_addr_label ( void ) {
    char buff[10];
    snprintf ( buff, sizeof ( buff ), "0x%04X", g_membrowser.selected_addr );
    gtk_label_set_text ( ui_get_label ( "dbg_membrowser_selected_addr_label" ), buff );
}


static void ui_membrowser_update_addr_in_textview ( uint32_t addr ) {
    g_membrowser.lock_textbuffer_changed = TRUE;

    GtkWidget *view = ui_get_widget ( "dbg_membrowser_textview" );
    GtkTextBuffer *buffer = gtk_text_view_get_buffer ( GTK_TEXT_VIEW ( view ) );

    GtkTextIter start_iter;
    GtkTextIter end_iter;
    uint32_t offset;
    GtkTextTag *tag;
    char buff[3];

    // HEX
    offset = ui_membrowser_get_hex_offset_by_addr ( addr );
    gtk_text_buffer_get_iter_at_offset ( buffer, &start_iter, offset );
    gtk_text_buffer_get_iter_at_offset ( buffer, &end_iter, offset + 2 );
    gtk_text_buffer_delete ( buffer, &start_iter, &end_iter );

    if ( addr == g_membrowser.selected_addr ) {
        tag = ( g_membrowser.mode == MEMBROWSER_MODE_EDIT_HEX ) ? g_membrowser.tag_edit : g_membrowser.tag_text_selected;
    } else {
        tag = ( ( g_membrowser.show_comparative ) && g_membrowser.data_current[addr] != g_membrowser.data_old[addr] ) ? g_membrowser.tag_text_changed : g_membrowser.tag_text;
    };

    snprintf ( buff, sizeof ( buff ), "%02X", g_membrowser.data_current[addr] );
    gtk_text_buffer_insert_with_tags ( buffer, &start_iter, buff, 2, tag, NULL );

    // ASCII
    offset = ui_membrowser_get_ascii_offset_by_addr ( addr );
    gtk_text_buffer_get_iter_at_offset ( buffer, &start_iter, offset );
    gtk_text_buffer_get_iter_at_offset ( buffer, &end_iter, offset + 1 );
    gtk_text_buffer_delete ( buffer, &start_iter, &end_iter );

    if ( addr == g_membrowser.selected_addr ) {
        tag = ( g_membrowser.mode == MEMBROWSER_MODE_EDIT_ASCII ) ? g_membrowser.tag_edit : g_membrowser.tag_text_selected;
    } else {
        tag = ( ( g_membrowser.show_comparative ) && g_membrowser.data_current[addr] != g_membrowser.data_old[addr] ) ? g_membrowser.tag_text_changed : g_membrowser.tag_text;
    };

    buff[0] = ( g_membrowser.sh_ascii_conversion ) ? sharpmz_cnv_from ( g_membrowser.data_current[addr] ) : g_membrowser.data_current[addr];
    if ( !( ( buff[0] >= 0x20 ) && ( buff[0] < 0x7f ) ) ) {
        buff[0] = '.';
    } else if ( ( buff[0] == ' ' ) && ( g_membrowser.data_current[addr] != 0x20 ) ) {
        buff[0] = '.';
    };

    gtk_text_buffer_insert_with_tags ( buffer, &start_iter, buff, 1, tag, NULL );

    g_membrowser.lock_textbuffer_changed = FALSE;
}


static void ui_membrowser_edit_hex_fix_cursor_position_left ( void ) {
    GtkWidget *view = ui_get_widget ( "dbg_membrowser_textview" );
    GtkTextBuffer *buffer = gtk_text_view_get_buffer ( GTK_TEXT_VIEW ( view ) );
    GtkTextIter iter;
    gint row, col;
    ui_membrowser_get_cursor_position ( &row, &col );

    if ( col < 5 ) {
        //if ( row != 0 ) {
        uint32_t offset = 5 + 52 + ( ( row - 1 ) * MEMBROWSER_ROW_LENGTH );
        gtk_text_buffer_get_iter_at_offset ( buffer, &iter, offset );
        gtk_text_buffer_place_cursor ( buffer, &iter );
        //} else {
        //    uint32_t offset = 5;
        //    gtk_text_buffer_get_iter_at_offset ( buffer, &iter, offset );
        //    gtk_text_buffer_place_cursor ( buffer, &iter );
        //};
        return;
    };

    col -= 5;

    gint new_col = col;

    uint32_t offset;
    switch ( col ) {
        case 2:
        case 5:
        case 8:
        case 16:
        case 19:
        case 22:
        case 30:
        case 33:
        case 36:
        case 44:
        case 47:
        case 50:
            new_col--;
            break;

        case 11:
        case 12:
        case 13:
            new_col = 10;
            break;

        case 25:
        case 26:
        case 27:
            new_col = 24;
            break;

        case 39:
        case 40:
        case 41:
            new_col = 38;
            break;
    };

    if ( col != new_col ) {
        offset = 5 + new_col + ( row * MEMBROWSER_ROW_LENGTH );
        gtk_text_buffer_get_iter_at_offset ( buffer, &iter, offset );
        gtk_text_buffer_place_cursor ( buffer, &iter );
    };
}


/**
 * 
 * @return TRUE - pokud vzniknul pozadavek k prechodu na novou stranku
 */
static gboolean ui_membrowser_edit_hex_fix_cursor_position_right ( void ) {
    GtkWidget *view = ui_get_widget ( "dbg_membrowser_textview" );
    GtkTextBuffer *buffer = gtk_text_view_get_buffer ( GTK_TEXT_VIEW ( view ) );
    GtkTextIter iter;
    gint row, col;
    ui_membrowser_get_cursor_position ( &row, &col );

    if ( col < 5 ) {
        uint32_t offset = 5 + ( row * MEMBROWSER_ROW_LENGTH );
        gtk_text_buffer_get_iter_at_offset ( buffer, &iter, offset );
        gtk_text_buffer_place_cursor ( buffer, &iter );
        return FALSE;
    };

    col -= 5;

    if ( ( row == ( MEMBROWSER_MEM_MAX / 0x10 ) - 1 ) && ( col > 52 ) ) {
        uint32_t offset = 5 + 52 + ( ( ( MEMBROWSER_MEM_MAX / 0x10 ) - 1 ) * MEMBROWSER_ROW_LENGTH );
        gtk_text_buffer_get_iter_at_offset ( buffer, &iter, offset );
        gtk_text_buffer_place_cursor ( buffer, &iter );
        return FALSE;
    };

    if ( col > 52 ) {
        if ( row < MEMBROWSER_LINES_PER_PAGE - 1 ) {
            row++;
            uint32_t offset = 5 + ( row * MEMBROWSER_ROW_LENGTH );
            gtk_text_buffer_get_iter_at_offset ( buffer, &iter, offset );
            gtk_text_buffer_place_cursor ( buffer, &iter );
        } else {
            if ( g_membrowser.selected_addr != ( g_membrowser.mem_size - 1 ) ) {
                return TRUE;
            } else {
                uint32_t offset;
                offset = ( 5 + 12 + 2 + 12 + 2 + 12 + 2 + 10 ) + ( row * MEMBROWSER_ROW_LENGTH );
                gtk_text_buffer_get_iter_at_offset ( buffer, &iter, offset );
                gtk_text_buffer_place_cursor ( buffer, &iter );
            };
        };
        return FALSE;
    };

    gint new_col = col;

    uint32_t offset;
    switch ( col ) {
        case 2:
        case 5:
        case 8:
        case 16:
        case 19:
        case 22:
        case 30:
        case 33:
        case 36:
        case 44:
        case 47:
        case 50:
            new_col++;
            break;

        case 11:
        case 12:
        case 13:
            new_col = 14;
            break;

        case 25:
        case 26:
        case 27:
            new_col = 28;
            break;

        case 39:
        case 40:
        case 41:
            new_col = 42;
            break;
    };

    if ( col != new_col ) {
        offset = 5 + new_col + ( row * MEMBROWSER_ROW_LENGTH );
        gtk_text_buffer_get_iter_at_offset ( buffer, &iter, offset );
        gtk_text_buffer_place_cursor ( buffer, &iter );
    };
    return FALSE;
}


static void ui_membrowser_edit_ascii_fix_cursor_position_left ( void ) {
    GtkWidget *view = ui_get_widget ( "dbg_membrowser_textview" );
    GtkTextBuffer *buffer = gtk_text_view_get_buffer ( GTK_TEXT_VIEW ( view ) );
    GtkTextIter iter;
    gint row, col;
    ui_membrowser_get_cursor_position ( &row, &col );

    col -= 5;

    if ( col < 55 ) {
        uint32_t offset;
        if ( row != 0 ) {
            offset = ( MEMBROWSER_ROW_LENGTH - 2 ) + ( ( row - 1 ) * MEMBROWSER_ROW_LENGTH );
        } else {
            offset = 55 + 5;
        };
        gtk_text_buffer_get_iter_at_offset ( buffer, &iter, offset );
        gtk_text_buffer_place_cursor ( buffer, &iter );
    };
}


/**
 * 
 * @return TRUE - pokud vzniknul pozadavek k prechodu na dalsi stranku
 */
static gboolean ui_membrowser_edit_ascii_fix_cursor_position_right ( gboolean mouse_event ) {
    GtkWidget *view = ui_get_widget ( "dbg_membrowser_textview" );
    GtkTextBuffer *buffer = gtk_text_view_get_buffer ( GTK_TEXT_VIEW ( view ) );
    GtkTextIter iter;
    gint row, col;
    ui_membrowser_get_cursor_position ( &row, &col );

    col -= 5;

    if ( col > ( MEMBROWSER_ROW_LENGTH - 2 - 5 ) ) {
        if ( row < MEMBROWSER_LINES_PER_PAGE - 1 ) {
            row++;
            uint32_t offset;
            offset = ( MEMBROWSER_ROW_LENGTH - 17 ) + ( row * MEMBROWSER_ROW_LENGTH );
            gtk_text_buffer_get_iter_at_offset ( buffer, &iter, offset );
            gtk_text_buffer_place_cursor ( buffer, &iter );
        } else {
            if ( ( mouse_event == FALSE ) && ( g_membrowser.selected_addr != ( g_membrowser.mem_size - 1 ) ) ) {
                return TRUE;
            } else {
                uint32_t offset;
                offset = ( MEMBROWSER_ROW_LENGTH - 2 ) + ( row * MEMBROWSER_ROW_LENGTH );
                gtk_text_buffer_get_iter_at_offset ( buffer, &iter, offset );
                gtk_text_buffer_place_cursor ( buffer, &iter );
            };
        };
    };
    return FALSE;
}


static gboolean ui_membrowser_update_textbuffer_set_selected ( gpointer user_data ) {
    gboolean *next_page = (gboolean*) user_data;
    if ( ( user_data == NULL ) || ( *next_page == FALSE ) ) {
        uint32_t old_addr = g_membrowser.selected_addr;
        g_membrowser.selected_addr = ui_membrowser_get_addr_by_cursor_position ( );
        gint row, col;
        ui_membrowser_get_cursor_position ( &row, &col );
        ui_membrowser_update_addr_in_textview ( old_addr );
        ui_membrowser_update_addr_in_textview ( g_membrowser.selected_addr );
        uint32_t offset = col + ( row * MEMBROWSER_ROW_LENGTH );
        GtkWidget *view = ui_get_widget ( "dbg_membrowser_textview" );
        GtkTextBuffer *buffer = gtk_text_view_get_buffer ( GTK_TEXT_VIEW ( view ) );
        GtkTextIter iter;
        gtk_text_buffer_get_iter_at_offset ( buffer, &iter, offset );
        gtk_text_buffer_place_cursor ( buffer, &iter );
        ui_membrowser_update_selected_addr_label ( );
    } else {
        g_membrowser.selected_addr = ui_membrowser_get_first_addr_on_page ( g_membrowser.page );
        ui_membrowser_show_page ( g_membrowser.page + 1 );
    }
    return G_SOURCE_REMOVE;
}


static void ui_membrowser_update_selected_addr ( void ) {
    uint32_t new_addr = ui_membrowser_get_addr_by_cursor_position ( );
    if ( ( new_addr == g_membrowser.selected_addr ) && ( new_addr != ( g_membrowser.mem_size - 1 ) ) ) return;
    gdk_threads_add_idle ( ui_membrowser_update_textbuffer_set_selected, NULL );
}


void ui_membrowser_show_page ( uint32_t page ) {

    g_membrowser.page = page;
    ui_membrowser_update_selected_page_label ( );

    g_membrowser.lock_page_vscale = TRUE;
    gtk_adjustment_set_value ( g_membrowser.page_adjustment, g_membrowser.page );
    g_membrowser.lock_page_vscale = FALSE;

    uint32_t previous_page = ui_membrowser_get_page_by_addr ( g_membrowser.selected_addr );
    uint32_t new_addr = g_membrowser.selected_addr + ( ( g_membrowser.page - previous_page ) * MEMBROWSER_ADDRESSES_PER_PAGE );
    g_membrowser.selected_addr = new_addr;
    ui_membrowser_update_selected_addr_label ( );

    g_membrowser.lock_textbuffer_changed = TRUE;

    GtkWidget *view = ui_get_widget ( "dbg_membrowser_textview" );
    GtkTextBuffer *buffer = gtk_text_view_get_buffer ( GTK_TEXT_VIEW ( view ) );
    gtk_text_buffer_set_text ( buffer, "", 0 );

    GtkTextIter iter;
    gtk_text_buffer_get_end_iter ( buffer, &iter );


    uint32_t start_addr = ui_membrowser_get_first_addr_on_page ( g_membrowser.page );
    uint32_t end_addr = ui_membrowser_get_last_addr_on_page ( g_membrowser.page );

    uint32_t byte_pos = 0;
    uint32_t i;
    for ( i = start_addr; i <= end_addr; i++ ) {

        if ( ( i % 0x10 ) == 0 ) {
            char buff[5];
            snprintf ( buff, sizeof ( buff ), "%04X", i );
            gtk_text_buffer_insert_with_tags ( buffer, &iter, buff, 4, g_membrowser.tag_addr, NULL );
        };

        gtk_text_buffer_insert_with_tags ( buffer, &iter, " ", 1, g_membrowser.tag_text, NULL );

        char buff[3];
        snprintf ( buff, sizeof ( buff ), "%02X", g_membrowser.data_current[i] );

        GtkTextTag *tag;
        if ( i == g_membrowser.selected_addr ) {
            tag = ( g_membrowser.mode == MEMBROWSER_MODE_EDIT_HEX ) ? g_membrowser.tag_edit : g_membrowser.tag_text_selected;
        } else {
            tag = ( ( g_membrowser.show_comparative ) && g_membrowser.data_current[i] != g_membrowser.data_old[i] ) ? g_membrowser.tag_text_changed : g_membrowser.tag_text;
        };

        gtk_text_buffer_insert_with_tags ( buffer, &iter, buff, 2, tag, NULL );

        if ( ( byte_pos == 3 ) || ( byte_pos == 7 ) || ( byte_pos == 11 ) ) {
            gtk_text_buffer_insert_with_tags ( buffer, &iter, " |", 2, g_membrowser.tag_text, NULL );
            byte_pos++;
        } else if ( byte_pos == 0x0f ) {

            gtk_text_buffer_insert_with_tags ( buffer, &iter, "  ", 2, g_membrowser.tag_text, NULL );

            int j;
            for ( j = ( i - 0x0f ); j <= i; j++ ) {

                char buff = ( g_membrowser.sh_ascii_conversion ) ? sharpmz_cnv_from ( g_membrowser.data_current[j] ) : g_membrowser.data_current[j];
                if ( !( ( buff >= 0x20 ) && ( buff < 0x7f ) ) ) {
                    buff = '.';
                } else if ( ( buff == ' ' ) && ( g_membrowser.data_current[j] != 0x20 ) ) {
                    buff = '.';
                };

                GtkTextTag *tag;
                if ( j == g_membrowser.selected_addr ) {
                    tag = ( g_membrowser.mode == MEMBROWSER_MODE_EDIT_ASCII ) ? g_membrowser.tag_edit : g_membrowser.tag_text_selected;
                } else {
                    tag = ( ( g_membrowser.show_comparative ) && g_membrowser.data_current[j] != g_membrowser.data_old[j] ) ? g_membrowser.tag_text_changed : g_membrowser.tag_text;
                };

                gtk_text_buffer_insert_with_tags ( buffer, &iter, &buff, 1, tag, NULL );
            };

            if ( i != ( end_addr ) ) {
                gtk_text_buffer_insert_with_tags ( buffer, &iter, "\n", 1, g_membrowser.tag_text, NULL );
            };
            byte_pos = 0;

        } else {
            byte_pos++;
        };
    };

    g_membrowser.lock_textbuffer_changed = FALSE;

    uint32_t offset;
    if ( g_membrowser.mode != MEMBROWSER_MODE_EDIT_ASCII ) {
        offset = ui_membrowser_get_hex_offset_by_addr ( g_membrowser.selected_addr );
    } else {
        offset = ui_membrowser_get_ascii_offset_by_addr ( g_membrowser.selected_addr );
    };

    gtk_text_buffer_get_iter_at_offset ( buffer, &iter, offset );
    gtk_text_buffer_place_cursor ( buffer, &iter );
}


static void ui_membrowser_close ( ) {
    GtkWidget *window = ui_get_widget ( "dbg_membrowser_window" );
    if ( !gtk_widget_is_visible ( window ) ) return;
    g_free ( g_membrowser.data_current );
    g_free ( g_membrowser.data_old );
    ui_main_win_get_pos ( GTK_WINDOW ( window ), &g_membrowser.main_pos );
    gtk_widget_hide ( window );
}


static int ui_membrowser_check_ramdisk ( void ) {
    if ( g_membrowser.memsrc == MEMBROWSER_SOURCE_PEZIK_68 ) {
        if ( RAMDISK_CONNECTED == g_ramdisk.pezik[RAMDISK_PEZIK_68].connected ) {
            int portbank = 1 << g_membrowser.pezik_bank[RAMDISK_PEZIK_68];
            if ( g_ramdisk.pezik[RAMDISK_PEZIK_68].portmask & portbank ) {
                return EXIT_SUCCESS;
            };
        };
    } else if ( g_membrowser.memsrc == MEMBROWSER_SOURCE_PEZIK_E8 ) {
        if ( RAMDISK_CONNECTED == g_ramdisk.pezik[RAMDISK_PEZIK_E8].connected ) {
            int portbank = 1 << g_membrowser.pezik_bank[RAMDISK_PEZIK_E8];
            if ( g_ramdisk.pezik[RAMDISK_PEZIK_E8].portmask & portbank ) {
                return EXIT_SUCCESS;
            };
        };
    } else if ( g_membrowser.memsrc == MEMBROWSER_SOURCE_MZ1R18 ) {
        if ( g_ramdisk.std.connected == RAMDISK_CONNECTED ) {
            if ( g_membrowser.mr1z18_bank <= g_ramdisk.std.size ) {
                return EXIT_SUCCESS;
            };
        };
    };
    return EXIT_FAILURE;
}


static int ui_membrowser_check_ramdisk_noisily ( void ) {
    if ( EXIT_SUCCESS != ui_membrowser_check_ramdisk ( ) ) {
        ui_show_error ( "Selected memory disc device or his bank is not connected!" );
        g_membrowser.MEM = NULL;
        memset ( g_membrowser.data_current, 0x00, MEMBROWSER_MEM_MAX );
        return EXIT_FAILURE;
    };
    return EXIT_SUCCESS;
}


static int ui_membrowser_check_memext ( void ) {
    if ( g_membrowser.memsrc == MEMBROWSER_SOURCE_MEMEXT_PEHU ) {
        if ( TEST_MEMEXT_CONNECTED_PEHU ) return EXIT_SUCCESS;
    } else if ( g_membrowser.memsrc == MEMBROWSER_SOURCE_MEMEXT_LUFTNER ) {
        if ( TEST_MEMEXT_CONNECTED_LUFTNER ) return EXIT_SUCCESS;
    };
    return EXIT_FAILURE;
}


static int ui_membrowser_check_memext_noisily ( void ) {
    if ( EXIT_SUCCESS != ui_membrowser_check_memext ( ) ) {
        ui_show_error ( "Selected type of MemExt device is not connected!" );
        g_membrowser.MEM = NULL;
        memset ( g_membrowser.data_current, 0x00, MEMBROWSER_MEM_MAX );
        return EXIT_FAILURE;
    };
    return EXIT_SUCCESS;
}


static void on_dbg_membrowser_textbuffer_changed ( GtkTextBuffer *textbuffer, gpointer user_data ) {

    if ( g_membrowser.lock_textbuffer_changed ) return;

    static gboolean lock_this_call = TRUE; // Prozatim nevim proc, ale vola se to 2x. 1. na pozici, kde je zmena, 2. na nasledujici pozici.
    if ( lock_this_call ) {
        lock_this_call = FALSE;
        return;
    };
    lock_this_call = TRUE;


    GtkTextIter start_iter;
    GtkTextIter end_iter;
    gint row, col;
    ui_membrowser_get_cursor_position ( &row, &col );
    uint32_t offset = col + ( row * MEMBROWSER_ROW_LENGTH );
    gtk_text_buffer_get_iter_at_offset ( textbuffer, &start_iter, offset - 1 );
    gtk_text_buffer_get_iter_at_offset ( textbuffer, &end_iter, offset );
    char *c = gtk_text_buffer_get_text ( textbuffer, &start_iter, &end_iter, FALSE );
    uint32_t addr = ui_membrowser_get_addr_by_offset ( offset - 1 );

    if ( g_membrowser.mode == MEMBROWSER_MODE_EDIT_HEX ) {
        Z80EX_BYTE halfbyte = (Z80EX_BYTE) debuger_hextext_to_uint32 ( c );
        uint32_t addr_offset = ui_membrowser_get_hex_offset_by_addr ( addr );
        if ( addr_offset == ( offset - 1 ) ) {
            g_membrowser.data_current[addr] = ( halfbyte << 4 ) | ( g_membrowser.data_current[addr] &0x0f );
        } else {
            g_membrowser.data_current[addr] = halfbyte | ( g_membrowser.data_current[addr] &0xf0 );
        };
    } else {
        if ( g_membrowser.sh_ascii_conversion ) {
            g_membrowser.data_current[addr] = sharpmz_cnv_to ( c[0] );
        } else {
            g_membrowser.data_current[addr] = c[0];
        };
    };

    g_free ( c );

    // ulozeni g_membrowser.data_current[addr] do skutecne pameti

    if ( ( g_membrowser.memsrc >= MEMBROWSER_SOURCE_PEZIK_68 ) && ( g_membrowser.memsrc <= MEMBROWSER_SOURCE_MZ1R18 ) ) {
        ui_membrowser_check_ramdisk_noisily ( );
    } else if ( ( g_membrowser.memsrc >= MEMBROWSER_SOURCE_MEMEXT_PEHU ) && ( g_membrowser.memsrc <= MEMBROWSER_SOURCE_MEMEXT_LUFTNER ) ) {
        ui_membrowser_check_memext_noisily ( );
    };

    if ( g_membrowser.MEM != NULL ) {
        if ( ( ( g_membrowser.memsrc == MEMBROWSER_SOURCE_PEZIK_68 ) || ( g_membrowser.memsrc == MEMBROWSER_SOURCE_PEZIK_E8 ) ) && ( g_membrowser.pezik_addressing == MEMBROWSER_PEZIK_ADDRESSING_LE ) ) {
            g_membrowser.MEM[( ( addr & 0xff ) << 8 ) | ( addr >> 8 )] = g_membrowser.data_current[addr];
        } else {
            g_membrowser.MEM[addr] = g_membrowser.data_current[addr];
            // Pokud jsme psali do VRAM, tak si vyzadame prekresleni obrazu
            if ( ( g_membrowser.MEM == g_memoryVRAM_I ) || ( g_membrowser.MEM == g_memoryVRAM_II ) || ( g_membrowser.MEM == g_memoryVRAM_III ) || ( g_membrowser.MEM == g_memoryVRAM_IV ) ) {
                g_iface_sdl.redraw_full_screen_request = 1;
                // Pokud jsme "paused" a mame nastaven forced srceen refresh, tak udelame screen refresh
                if ( ( g_mz800.emulation_paused ) && ( g_membrowser.forced_screen_refresh ) ) {
                    debugger_forced_screen_update ( );
                };
            };
        };
    } else {
        if ( g_membrowser.memsrc == MEMBROWSER_SOURCE_MAPED ) {
            memory_load_block ( &g_membrowser.data_current[addr], addr, 1, MEMORY_LOAD_MAPED );
            g_membrowser.data_current[addr] = debugger_memory_read_byte ( addr );
            // Pokud jsme psali do VRAM, tak si vyzadame prekresleni obrazu
            if ( memory_test_addr_is_vram ( addr ) ) {
                g_iface_sdl.redraw_full_screen_request = 1;
                // Pokud jsme "paused" a mame nastaven forced srceen refresh, tak udelame screen refresh
                if ( ( g_mz800.emulation_paused ) && ( g_membrowser.forced_screen_refresh ) ) {
                    debugger_forced_screen_update ( );
                };
            }
        } else if ( g_membrowser.memsrc == MEMBROWSER_SOURCE_RAM ) {
            memory_load_block ( &g_membrowser.data_current[addr], addr, 1, MEMORY_LOAD_RAMONLY );
            g_membrowser.data_current[addr] = debugger_pure_ram_read_byte ( addr ); // muzeme narazit na namapovanou FLASH, ktera je R/O
        } else if ( g_membrowser.memsrc == MEMBROWSER_SOURCE_MEMEXT_LUFTNER ) {
            uint8_t *srcmem;
            if ( g_membrowser.memext_luftner_bank & 0x80 ) {
                srcmem = &g_memext.FLASH[( ( g_membrowser.memext_luftner_bank & 0x7f ) * MEMEXT_LUFTNER_BANK_SIZE )];
            } else {
                srcmem = &g_memext.RAM[( g_membrowser.memext_luftner_bank * MEMEXT_LUFTNER_BANK_SIZE )];
            };
            srcmem[addr] = g_membrowser.data_current[addr];
        } else {
            g_membrowser.data_current[addr] = 0x00;
        };
    };

    static gboolean next_page;
    if ( g_membrowser.mode == MEMBROWSER_MODE_EDIT_HEX ) {
        next_page = ui_membrowser_edit_hex_fix_cursor_position_right ( );
    } else {
        next_page = ui_membrowser_edit_ascii_fix_cursor_position_right ( FALSE );
    };

    gdk_threads_add_idle ( ui_membrowser_update_textbuffer_set_selected, &next_page );
}


static void ui_membrowser_set_mode ( en_MEMBROWSER_MODE mode ) {

    if ( ( g_membrowser.mode != MEMBROWSER_MODE_VIEW ) && ( ( mode == MEMBROWSER_MODE_EDIT_HEX ) || ( mode == MEMBROWSER_MODE_EDIT_ASCII ) ) ) {
        g_membrowser.mode = mode;
        return;
    };

    g_membrowser.mode = mode;

    GtkWidget *view = ui_get_widget ( "dbg_membrowser_textview" );
    char *label_text;
    gboolean sensitive;
    if ( mode == MEMBROWSER_MODE_VIEW ) {
        label_text = "Edit mode - OFF";
        gtk_text_view_set_editable ( GTK_TEXT_VIEW ( view ), FALSE );
        gtk_text_view_set_overwrite ( GTK_TEXT_VIEW ( view ), FALSE );
        sensitive = TRUE;

        if ( gtk_widget_is_visible ( ui_get_widget ( "debugger_main_window" ) ) ) {
            ui_debugger_update_disassembled ( ui_debugger_dissassembled_get_first_addr ( ), -1 );
            ui_debugger_update_stack ( );
        };
    } else {
        label_text = "Edit mode - ON";
        gtk_text_view_set_editable ( GTK_TEXT_VIEW ( view ), TRUE );
        gtk_text_view_set_overwrite ( GTK_TEXT_VIEW ( view ), TRUE );
        //if ( !TEST_EMULATION_PAUSED ) {
        //    mz800_pause_emulation ( 1 );
        //};
        sensitive = FALSE;
    };

    GtkTextBuffer *buffer = gtk_text_view_get_buffer ( GTK_TEXT_VIEW ( view ) );
    uint32_t offset;
    if ( mode == MEMBROWSER_MODE_EDIT_ASCII ) {
        offset = ui_membrowser_get_ascii_offset_by_addr ( g_membrowser.selected_addr );
    } else {
        offset = ui_membrowser_get_hex_offset_by_addr ( g_membrowser.selected_addr );
    };
    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_offset ( buffer, &iter, offset );
    gtk_text_buffer_place_cursor ( buffer, &iter );
    ui_membrowser_update_textbuffer_set_selected ( NULL );
    gtk_widget_grab_focus ( view );


    gtk_label_set_text ( ui_get_label ( "dbg_membrowser_switch_mode_label" ), label_text );
    gtk_widget_set_sensitive ( ui_get_widget ( "dbg_membrowser_load_button" ), sensitive );
    gtk_widget_set_sensitive ( ui_get_widget ( "dbg_membrowser_save_button" ), sensitive );
    gtk_widget_set_sensitive ( ui_get_widget ( "dbg_membrowser_source_comboboxtext" ), sensitive );
    gtk_widget_set_sensitive ( ui_get_widget ( "dbg_membrowser_bank_comboboxtext" ), sensitive );
    gtk_widget_set_sensitive ( ui_get_widget ( "dbg_membrowser_pezik_addressing_comboboxtext" ), sensitive );
    //gtk_widget_set_sensitive ( ui_get_widget ( "dbg_membrowser_refresh_button" ), sensitive );
}


static void on_dbg_membrowser_page_adjustment_value_changed ( GtkAdjustment *adjustment, gpointer user_data ) {
    if ( g_membrowser.lock_page_vscale ) return;
    uint32_t page = (uint32_t) gtk_range_get_value ( GTK_RANGE ( g_membrowser.page_vscale ) );
    if ( page == g_membrowser.page ) return;
    ui_membrowser_show_page ( page );
}


static void ui_membrowser_initialise_bank256_combobox ( void ) {

    GtkTreeStore *store = gtk_tree_store_new ( 2, G_TYPE_STRING, G_TYPE_UINT );
    GtkTreeIter iter, iter2;
    char buff[20];

    int i;
    for ( i = 0; i < 0x10; i++ ) {
        snprintf ( buff, sizeof ( buff ), "0x%02x - 0x%02x", i << 4, ( i << 4 ) | 0x0f );
        gtk_tree_store_append ( store, &iter, NULL );
        gtk_tree_store_set ( store, &iter, 0, buff, -1 );
        int j;
        for ( j = 0; j < 0x10; j++ ) {
            uint32_t bank = i << 4 | j;
            snprintf ( buff, sizeof ( buff ), "bank 0x%02x", bank );
            gtk_tree_store_append ( store, &iter2, &iter );
            gtk_tree_store_set ( store, &iter2, 0, buff, 1, bank, -1 );
        };
    };

    g_membrowser.lock_bank256_combobox = TRUE;

    GtkWidget *combo = ui_get_widget ( "dbg_membrowser_bank256_combobox" );
    gtk_combo_box_set_model ( GTK_COMBO_BOX ( combo ), GTK_TREE_MODEL ( store ) );
    g_object_unref ( store );

    GtkCellRenderer *renderer = gtk_cell_renderer_text_new ( );
    gtk_cell_layout_pack_start ( GTK_CELL_LAYOUT ( combo ), renderer, TRUE );
    gtk_cell_layout_set_attributes ( GTK_CELL_LAYOUT ( combo ), renderer, "text", 0, NULL );

    g_membrowser.lock_bank256_combobox = FALSE;
}


static void ui_membrowser_init ( void ) {

    g_membrowser.data_current = (Z80EX_BYTE*) g_malloc0 ( MEMBROWSER_MEM_MAX );
    g_membrowser.data_old = (Z80EX_BYTE*) g_malloc0 ( MEMBROWSER_MEM_MAX );

    g_membrowser.key_shift_l = FALSE;
    g_membrowser.key_shift_r = FALSE;
    g_membrowser.key_ctrl_l = FALSE;
    g_membrowser.key_ctrl_r = FALSE;
    g_membrowser.key_up = FALSE;
    g_membrowser.key_left = FALSE;

    g_membrowser.lock_textbuffer_changed = FALSE;
    g_membrowser.lock_page_vscale = FALSE;
    g_membrowser.lock_source_combotext = FALSE;
    g_membrowser.lock_bank256_combobox = FALSE;
    g_membrowser.lock_bank_combotext = FALSE;
    g_membrowser.lock_pezik_addr_combotext = FALSE;
    g_membrowser.lock_goto_entry = FALSE;

    ui_membrowser_set_mode ( MEMBROWSER_MODE_VIEW );
    //ui_membrowser_set_mode ( MEMBROWSER_MODE_EDIT_HEX );
    //ui_membrowser_set_mode ( MEMBROWSER_MODE_EDIT_ASCII );

    gtk_toggle_button_set_active ( ui_get_toggle ( "dbg_membrowser_switch_mode_togglebutton" ), FALSE );

    if ( g_membrowser_initialised ) return;

    ui_main_setpos ( &g_membrowser.main_pos, -1, -1 );
    // inicializujeme v debugger_init()
#if 0
    g_membrowser.memsrc = MEMBROWSER_SOURCE_MAPED;
    g_membrowser.show_comparative = FALSE;
    g_membrowser.sh_ascii_conversion = FALSE;
    g_membrowser.forced_screen_refresh = FALSE;
    g_membrowser.pezik_addressing = MEMBROWSER_PEZIK_ADDRESSING_BE;
    g_membrowser.vram_plane = 0;
    g_membrowser.pezik_bank[0] = 0;
    g_membrowser.pezik_bank[1] = 0;
    g_membrowser.mr1z18_bank = 0;
    g_membrowser.memext_bank = 0;
#endif
    g_membrowser.selected_addr = 0;
    g_membrowser.page = 0;

    gtk_toggle_button_set_active ( ui_get_toggle ( "dbg_membrowser_comparative_mode_checkbutton" ), g_membrowser.show_comparative );
    gtk_combo_box_set_active ( ui_get_combo_box ( "dbg_membrowser_ascii_cnv_comboboxtext" ), ( g_membrowser.sh_ascii_conversion ) ? 1 : 0 );
    gtk_toggle_button_set_active ( ui_get_toggle ( "dbg_membrowser_force_screen_refresh_checkbutton" ), g_membrowser.forced_screen_refresh );

    /* scale vyrobeny pres glade (asi :) nefunguje spravne */
    g_membrowser.page_adjustment = gtk_adjustment_new ( 0, 0, 128, 1, 8, 0 );
    g_membrowser.page_vscale = gtk_scale_new ( GTK_ORIENTATION_VERTICAL, g_membrowser.page_adjustment );
    gtk_widget_set_name ( g_membrowser.page_vscale, "dbg_membrowser_page_vscale" );
    gtk_widget_show ( g_membrowser.page_vscale );
    gtk_box_pack_start ( GTK_BOX ( ui_get_object ( "dbg_membrowser_hexeditor_box" ) ), g_membrowser.page_vscale, FALSE, TRUE, 2 );
    gtk_scale_set_draw_value ( GTK_SCALE ( g_membrowser.page_vscale ), FALSE );

    g_signal_connect ( (gpointer) g_membrowser.page_adjustment, "value-changed",
                       G_CALLBACK ( on_dbg_membrowser_page_adjustment_value_changed ),
                       NULL );

    GtkWidget *view = ui_get_widget ( "dbg_membrowser_textview" );
    GtkTextBuffer *buffer = gtk_text_view_get_buffer ( GTK_TEXT_VIEW ( view ) );

    g_signal_connect ( (gpointer) buffer, "changed",
                       G_CALLBACK ( on_dbg_membrowser_textbuffer_changed ),
                       NULL );

#if 0
    // nefunguje ve windows verzi GTK
    g_signal_connect_after ( (gpointer) view, "button-press-event",
                             G_CALLBACK ( on_dbg_membrowser_textview_button_press_event_after ),
                             NULL );
#endif

#ifdef WINDOWS
    GdkRGBA rgba;
    gdk_rgba_parse ( &rgba, "#3465a4" );
    gtk_widget_override_background_color ( view, GTK_STATE_FLAG_NORMAL, &rgba );
    gdk_rgba_parse ( &rgba, "#bec7c9" );
    gtk_widget_override_color ( view, GTK_STATE_FLAG_NORMAL, &rgba );
    gtk_widget_override_font ( view, pango_font_description_from_string ( "Monospace 12" ) );
#endif

    g_membrowser.tag_addr = gtk_text_buffer_create_tag ( buffer, "tag_addr",
                                                         "font", "Monospace 12",
                                                         "weight", PANGO_WEIGHT_BOLD,
                                                         "foreground", "#cbc864",
                                                         NULL );

    g_membrowser.tag_text = gtk_text_buffer_create_tag ( buffer, "tag_text",
                                                         "font", "Monospace 12",
                                                         "foreground", "#bec7c9",
                                                         NULL );

    g_membrowser.tag_text_changed = gtk_text_buffer_create_tag ( buffer, "tag_text_chenged",
                                                                 "font", "Monospace 12",
                                                                 "foreground", "#ff1010",
                                                                 NULL );

    g_membrowser.tag_text_selected = gtk_text_buffer_create_tag ( buffer, "tag_text_selected",
                                                                  "font", "Monospace 12",
                                                                  "foreground", "#ffffff",
                                                                  "background", "#06989a",
                                                                  "weight", PANGO_WEIGHT_BOLD,
                                                                  NULL );

    g_membrowser.tag_edit = gtk_text_buffer_create_tag ( buffer, "tag_edit",
                                                         "font", "Monospace 12",
                                                         "foreground", "#ffffff",
                                                         "weight", PANGO_WEIGHT_BOLD,
                                                         NULL );

    ui_membrowser_initialise_bank256_combobox ( );

    // nefunguje pras nastaveni z glade
    gtk_entry_set_input_purpose ( ui_get_entry ( "dbg_mebrowser_goto_addr_dec_entry" ), GTK_INPUT_PURPOSE_DIGITS );

    g_membrowser_initialised = TRUE;
}


void ui_membrowser_set_mem_size ( uint32_t memsize ) {
    g_membrowser.mem_size = memsize;
    g_membrowser.total_pages = memsize / MEMBROWSER_ADDRESSES_PER_PAGE;
    g_membrowser.lock_page_vscale = TRUE;
    gtk_adjustment_set_value ( g_membrowser.page_adjustment, 0 );
    g_membrowser.lock_page_vscale = FALSE;
    gtk_adjustment_set_upper ( g_membrowser.page_adjustment, g_membrowser.total_pages - 1 );
    char buff[10];
    snprintf ( buff, sizeof ( buff ), "%d", g_membrowser.total_pages );
    gtk_label_set_text ( ui_get_label ( "dbg_membrowser_total_pages_label" ), buff );
}


static void ui_membrowser_load_data_from_memsrc ( void ) {

    if ( ( g_membrowser.memsrc == MEMBROWSER_SOURCE_VRAM ) || ( g_membrowser.memsrc == MEMBROWSER_SOURCE_MZ800ROM ) || ( g_membrowser.memsrc == MEMBROWSER_SOURCE_MEMEXT_PEHU ) ) {
        ui_membrowser_set_mem_size ( 0x2000 );
    } else if ( ( g_membrowser.memsrc == MEMBROWSER_SOURCE_MZ700ROM ) || ( g_membrowser.memsrc == MEMBROWSER_SOURCE_CGROM ) || ( g_membrowser.memsrc == MEMBROWSER_SOURCE_MEMEXT_LUFTNER ) ) {
        ui_membrowser_set_mem_size ( 0x1000 );
    } else {
        ui_membrowser_set_mem_size ( 0x10000 );
    };

    g_membrowser.MEM = NULL;
    gboolean pezik_LE = FALSE;
    int memext_luftner_check = EXIT_FAILURE;

    switch ( g_membrowser.memsrc ) {

        case MEMBROWSER_SOURCE_MAPED:
        case MEMBROWSER_SOURCE_RAM:
            break;

        case MEMBROWSER_SOURCE_MEMEXT_LUFTNER:
            memext_luftner_check = ui_membrowser_check_memext_noisily ( );
            break;

        case MEMBROWSER_SOURCE_MEMEXT_PEHU:
            if ( EXIT_SUCCESS == ui_membrowser_check_memext_noisily ( ) ) {
                g_membrowser.MEM = &g_memext.RAM[( g_membrowser.memext_pehu_bank * MEMEXT_PEHU_BANK_SIZE )];
            };
            break;

        case MEMBROWSER_SOURCE_MZ700ROM:
            g_membrowser.MEM = g_memory.ROM;
            break;

        case MEMBROWSER_SOURCE_CGROM:
            g_membrowser.MEM = g_memory.ROM + ROM_SIZE_MZ700;
            break;

        case MEMBROWSER_SOURCE_MZ800ROM:
            g_membrowser.MEM = g_memory.ROM + ROM_SIZE_MZ700 + ROM_SIZE_CGROM;
            break;

        case MEMBROWSER_SOURCE_VRAM:
            switch ( g_membrowser.vram_plane ) {
                case 0:
                    g_membrowser.MEM = g_memoryVRAM_I;
                    break;
                case 1:
                    g_membrowser.MEM = g_memoryVRAM_II;
                    break;
                case 2:
                    g_membrowser.MEM = g_memoryVRAM_III;
                    break;
                case 3:
                    g_membrowser.MEM = g_memoryVRAM_IV;
                    break;
            };
            break;

        case MEMBROWSER_SOURCE_PEZIK_E8:
            if ( EXIT_SUCCESS == ui_membrowser_check_ramdisk_noisily ( ) ) {
                g_membrowser.MEM = &g_ramdisk.pezik[RAMDISK_PEZIK_E8].memory[( g_membrowser.pezik_bank[RAMDISK_PEZIK_E8] << 16 )];
                if ( g_membrowser.pezik_addressing == MEMBROWSER_PEZIK_ADDRESSING_LE ) {
                    pezik_LE = TRUE;
                };
            };
            break;

        case MEMBROWSER_SOURCE_PEZIK_68:
            if ( EXIT_SUCCESS == ui_membrowser_check_ramdisk_noisily ( ) ) {
                g_membrowser.MEM = &g_ramdisk.pezik[RAMDISK_PEZIK_E8].memory[( g_membrowser.pezik_bank[RAMDISK_PEZIK_68] << 16 )];
                if ( g_membrowser.pezik_addressing == MEMBROWSER_PEZIK_ADDRESSING_LE ) {
                    pezik_LE = TRUE;
                };
            };
            break;

        case MEMBROWSER_SOURCE_MZ1R18:
            if ( EXIT_SUCCESS == ui_membrowser_check_ramdisk_noisily ( ) ) {
                g_membrowser.MEM = &g_ramdisk.std.memory[( g_membrowser.mr1z18_bank << 16 )];
            };
            break;
    };

    memcpy ( g_membrowser.data_old, g_membrowser.data_current, MEMBROWSER_MEM_MAX );

    if ( g_membrowser.MEM != NULL ) {
        if ( !pezik_LE ) {
            memcpy ( g_membrowser.data_current, g_membrowser.MEM, g_membrowser.mem_size );
        } else {
            int i;
            for ( i = 0; i < g_membrowser.mem_size; i++ ) {
                g_membrowser.data_current[i] = g_membrowser.MEM[( ( i & 0xff ) << 8 ) | ( i >> 8 )];
            };
        };
    } else {
        if ( g_membrowser.memsrc == MEMBROWSER_SOURCE_MAPED ) {
            int i;
            for ( i = 0; i < g_membrowser.mem_size; i++ ) {
                g_membrowser.data_current[i] = debugger_memory_read_byte ( i );
            };
        } else if ( g_membrowser.memsrc == MEMBROWSER_SOURCE_RAM ) {
            int i;
            for ( i = 0; i < g_membrowser.mem_size; i++ ) {
                g_membrowser.data_current[i] = debugger_pure_ram_read_byte ( i );
            };
        } else if ( ( g_membrowser.memsrc == MEMBROWSER_SOURCE_MEMEXT_LUFTNER ) && ( memext_luftner_check == EXIT_SUCCESS ) ) {
            uint8_t *srcmem;
            if ( g_membrowser.memext_luftner_bank & 0x80 ) {
                srcmem = &g_memext.FLASH[( ( g_membrowser.memext_luftner_bank & 0x7f ) * MEMEXT_LUFTNER_BANK_SIZE )];
            } else {
                srcmem = &g_memext.RAM[( g_membrowser.memext_luftner_bank * MEMEXT_LUFTNER_BANK_SIZE )];
            };
            memcpy ( g_membrowser.data_current, srcmem, g_membrowser.mem_size );

        } else {
            // neznamy zdroj?
            memset ( g_membrowser.data_current, 0x00, MEMBROWSER_MEM_MAX );
        };
    };
}


static void ui_membrowser_show ( void ) {
    GtkWidget *window = ui_get_widget ( "dbg_membrowser_window" );
    if ( gtk_widget_is_visible ( window ) ) return;

    ui_membrowser_init ( );

    if ( ( g_membrowser.memsrc >= MEMBROWSER_SOURCE_PEZIK_68 ) && ( g_membrowser.memsrc <= MEMBROWSER_SOURCE_MZ1R18 ) ) {
        if ( EXIT_SUCCESS != ui_membrowser_check_ramdisk ( ) ) {
            g_membrowser.memsrc = MEMBROWSER_SOURCE_MAPED;
        };
    } else if ( ( g_membrowser.memsrc >= MEMBROWSER_SOURCE_MEMEXT_PEHU ) && ( g_membrowser.memsrc <= MEMBROWSER_SOURCE_MEMEXT_LUFTNER ) ) {
        if ( EXIT_SUCCESS != ui_membrowser_check_memext ( ) ) {
            g_membrowser.memsrc = MEMBROWSER_SOURCE_MAPED;
        };
    };

    gtk_combo_box_set_active ( ui_get_combo_box ( "dbg_membrowser_source_comboboxtext" ), g_membrowser.memsrc );
    ui_membrowser_refresh ( );

    ui_main_win_move_to_pos ( GTK_WINDOW ( window ), &g_membrowser.main_pos );
    gtk_widget_show ( window );
}


void ui_membrowser_show_hide ( void ) {
    GtkWidget *window = ui_get_widget ( "dbg_membrowser_window" );
    if ( gtk_widget_is_visible ( window ) ) {
        ui_membrowser_close ( );
    } else {
        ui_membrowser_show ( );
    };
}


G_MODULE_EXPORT gboolean on_dbg_membrowser_window_delete_event ( GtkWidget *widget, GdkEvent *event, gpointer user_data ) {
    ui_membrowser_close ( );
    return TRUE; // zastavime dalsi propagaci eventu
}


G_MODULE_EXPORT gboolean on_dbg_membrowser_window_key_press_event ( GtkWidget *widget, GdkEventKey *event, gpointer user_data ) {
    if ( g_membrowser.mode != MEMBROWSER_MODE_VIEW ) return FALSE;

    if ( event->keyval == GDK_KEY_Escape ) {
        ui_membrowser_close ( );
        return TRUE;
    };

    return FALSE;
}


G_MODULE_EXPORT void on_dbg_membrowser_textview_toggle_overwrite ( GtkTextView *text_view, gpointer user_data ) {
    if ( TRUE != gtk_text_view_get_overwrite ( text_view ) ) return;
    gtk_text_view_set_overwrite ( text_view, FALSE );
}


G_MODULE_EXPORT void on_dbg_membrowser_textview_move_cursor_after ( GtkTextView *text_view, GtkMovementStep step, gint count, gboolean extend_selection, gpointer user_data ) {
    if ( g_membrowser.key_left || g_membrowser.key_up ) {
        if ( g_membrowser.mode == MEMBROWSER_MODE_EDIT_ASCII ) {
            ui_membrowser_edit_ascii_fix_cursor_position_left ( );
        } else {
            ui_membrowser_edit_hex_fix_cursor_position_left ( );
        };
    } else {
        if ( g_membrowser.mode == MEMBROWSER_MODE_EDIT_ASCII ) {
            ui_membrowser_edit_ascii_fix_cursor_position_right ( FALSE );
        } else {
            ui_membrowser_edit_hex_fix_cursor_position_right ( );
        };
    };
    ui_membrowser_update_selected_addr ( );
}


G_MODULE_EXPORT gboolean on_dbg_membrowser_textview_button_press_event ( GtkWidget *widget, GdkEvent *event, gpointer user_data ) {
    guint button;
    gdk_event_get_button ( event, &button );
    if ( button != 1 ) return TRUE;

    gint x, y;
    gtk_text_view_window_to_buffer_coords ( GTK_TEXT_VIEW ( widget ), GTK_TEXT_WINDOW_WIDGET, event->button.x, event->button.y, &x, &y );

    GtkTextIter iter;
    gtk_text_view_get_iter_at_location ( GTK_TEXT_VIEW ( widget ), &iter, x, y );
    GtkTextBuffer *buffer = gtk_text_view_get_buffer ( GTK_TEXT_VIEW ( widget ) );
    gtk_text_buffer_place_cursor ( buffer, &iter );

    gint row, col;
    ui_membrowser_get_cursor_position ( &row, &col );

    // 57 = posledni znak v HEX, 60 = prvni znak v ASCII
    if ( ( col > 57 ) && ( col < 60 ) ) {
        if ( col < 59 ) {
            col = 56;
        } else {
            col = 60;
        };
        uint32_t offset = ( row * MEMBROWSER_ROW_LENGTH ) + col;
        gtk_text_buffer_get_iter_at_offset ( buffer, &iter, offset );
        gtk_text_buffer_place_cursor ( buffer, &iter );
    } else if ( col < 5 ) {
        col = 5;
        uint32_t offset = ( row * MEMBROWSER_ROW_LENGTH ) + col;
        gtk_text_buffer_get_iter_at_offset ( buffer, &iter, offset );
        gtk_text_buffer_place_cursor ( buffer, &iter );
    } else if ( col > 75 ) {
        col = 75;
        uint32_t offset = ( row * MEMBROWSER_ROW_LENGTH ) + col;
        gtk_text_buffer_get_iter_at_offset ( buffer, &iter, offset );
        gtk_text_buffer_place_cursor ( buffer, &iter );
    };

    if ( g_membrowser.mode != MEMBROWSER_MODE_VIEW ) {
        if ( col <= 57 ) {
            g_membrowser.mode = MEMBROWSER_MODE_EDIT_HEX;
        } else {
            g_membrowser.mode = MEMBROWSER_MODE_EDIT_ASCII;
        };
    };

    if ( col <= 57 ) {
        ui_membrowser_edit_hex_fix_cursor_position_right ( );
    } else {
        ui_membrowser_edit_ascii_fix_cursor_position_right ( TRUE );
    };

    ui_membrowser_update_selected_addr ( );
    gtk_widget_grab_focus ( widget );
    return TRUE;
}

#if 0
// nefunguje ve windows verzi GTK


G_MODULE_EXPORT gboolean on_dbg_membrowser_textview_button_press_event_after ( GtkWidget *widget, GdkEvent *event, gpointer user_data ) {
    gint row, col;
    ui_membrowser_get_cursor_position ( &row, &col );

    if ( g_membrowser.mode != MEMBROWSER_MODE_VIEW ) {
        if ( col <= ( 5 + 55 ) ) {
            g_membrowser.mode = MEMBROWSER_MODE_EDIT_HEX;
        } else {
            g_membrowser.mode = MEMBROWSER_MODE_EDIT_ASCII;
        };

        if ( g_membrowser.mode == MEMBROWSER_MODE_EDIT_HEX ) {
            ui_membrowser_edit_hex_fix_cursor_position_right ( );
        } else {
            ui_membrowser_edit_ascii_fix_cursor_position_right ( );
        };
    } else {
        if ( col <= ( 5 + 55 ) ) {
            ui_membrowser_edit_hex_fix_cursor_position_right ( );
        } else {
            ui_membrowser_edit_ascii_fix_cursor_position_right ( );
        };
    };


    ui_membrowser_update_selected_addr ( );
    return FALSE;
}
#endif


G_MODULE_EXPORT gboolean on_dbg_membrowser_textview_key_press_event ( GtkWidget *widget, GdkEventKey *event, gpointer user_data ) {

    //printf ( "kv: 0x%04x\n", event->keyval );


    // sledovane
    if ( event->keyval == GDK_KEY_Shift_L ) {
        g_membrowser.key_shift_l = TRUE;
        return FALSE;
    };

    if ( event->keyval == GDK_KEY_Shift_R ) {
        g_membrowser.key_shift_r = TRUE;
        return FALSE;
    };

    // kombinace CTRL+xx si obslouzime sami
    if ( event->keyval == GDK_KEY_Control_L ) {
        g_membrowser.key_ctrl_l = TRUE;
        return TRUE;
    };

    // kombinace CTRL+xx si obslouzime sami
    if ( event->keyval == GDK_KEY_Control_R ) {
        g_membrowser.key_ctrl_r = TRUE;
        return TRUE;
    };

    // Obsluha Home a CTRL + Home
    if ( event->keyval == GDK_KEY_Home ) {
        uint32_t offset;
        if ( g_membrowser.key_ctrl_l || g_membrowser.key_ctrl_r ) {
            // Home - zacatek dokumentu
            if ( g_membrowser.page != 0 ) {
                g_membrowser.selected_addr = ui_membrowser_get_first_addr_on_page ( 0 );
                ui_membrowser_show_page ( 0 );
                return TRUE;
            } else {
                if ( g_membrowser.mode == MEMBROWSER_MODE_EDIT_HEX ) {
                    offset = ui_membrowser_get_hex_offset_by_addr ( 0x0000 );
                } else {
                    offset = ui_membrowser_get_ascii_offset_by_addr ( 0x0000 );
                };
            };
        } else {
            // Home - zacatek radku
            if ( g_membrowser.mode == MEMBROWSER_MODE_EDIT_HEX ) {
                offset = ui_membrowser_get_hex_offset_by_addr ( ( g_membrowser.selected_addr / 0x10 ) * 0x10 );
            } else {
                offset = ui_membrowser_get_ascii_offset_by_addr ( ( g_membrowser.selected_addr / 0x10 ) * 0x10 );
            };
        };

        GtkWidget *view = ui_get_widget ( "dbg_membrowser_textview" );
        GtkTextBuffer *buffer = gtk_text_view_get_buffer ( GTK_TEXT_VIEW ( view ) );
        GtkTextIter iter;
        gtk_text_buffer_get_iter_at_offset ( buffer, &iter, offset );
        gtk_text_buffer_place_cursor ( buffer, &iter );
        ui_membrowser_update_selected_addr ( );
        return TRUE;
    };

    // Obsluha End a CTRL + End
    if ( event->keyval == GDK_KEY_End ) {
        uint32_t offset;
        if ( g_membrowser.key_ctrl_l || g_membrowser.key_ctrl_r ) {
            // End - konec dokumentu
            if ( g_membrowser.page != ( g_membrowser.total_pages - 1 ) ) {
                g_membrowser.selected_addr = g_membrowser.mem_size - 1;
                ui_membrowser_show_page ( g_membrowser.total_pages - 1 );
                return TRUE;
            } else {
                if ( g_membrowser.mode == MEMBROWSER_MODE_EDIT_HEX ) {
                    offset = ui_membrowser_get_hex_offset_by_addr ( g_membrowser.mem_size - 1 );
                } else {
                    offset = ui_membrowser_get_ascii_offset_by_addr ( g_membrowser.mem_size - 1 );
                };
            };
        } else {
            // End - konec radku
            if ( g_membrowser.mode == MEMBROWSER_MODE_EDIT_HEX ) {
                offset = ui_membrowser_get_hex_offset_by_addr ( ( ( g_membrowser.selected_addr / 0x10 ) * 0x10 ) + 0x0f );
            } else {
                offset = ui_membrowser_get_ascii_offset_by_addr ( ( ( g_membrowser.selected_addr / 0x10 ) * 0x10 ) + 0x0f );
            };
        };

        GtkWidget *view = ui_get_widget ( "dbg_membrowser_textview" );
        GtkTextBuffer *buffer = gtk_text_view_get_buffer ( GTK_TEXT_VIEW ( view ) );
        GtkTextIter iter;
        gtk_text_buffer_get_iter_at_offset ( buffer, &iter, offset );
        gtk_text_buffer_place_cursor ( buffer, &iter );
        ui_membrowser_update_selected_addr ( );
        return TRUE;
    };

    if ( g_membrowser.mode != MEMBROWSER_MODE_VIEW ) {

        if ( event->keyval == GDK_KEY_Escape ) {
            gtk_toggle_button_set_active ( GTK_TOGGLE_BUTTON ( ui_get_widget ( "dbg_membrowser_switch_mode_togglebutton" ) ), FALSE );
            return TRUE;
        };


        if ( event->keyval == GDK_KEY_Tab ) {

            uint32_t offset;
            if ( g_membrowser.mode == MEMBROWSER_MODE_EDIT_HEX ) {
                g_membrowser.mode = MEMBROWSER_MODE_EDIT_ASCII;
                offset = ui_membrowser_get_ascii_offset_by_addr ( g_membrowser.selected_addr );
            } else {
                g_membrowser.mode = MEMBROWSER_MODE_EDIT_HEX;
                offset = ui_membrowser_get_hex_offset_by_addr ( g_membrowser.selected_addr );
            };
            GtkWidget *view = ui_get_widget ( "dbg_membrowser_textview" );
            GtkTextBuffer *buffer = gtk_text_view_get_buffer ( GTK_TEXT_VIEW ( view ) );
            GtkTextIter iter;
            gtk_text_buffer_get_iter_at_offset ( buffer, &iter, offset );
            gtk_text_buffer_place_cursor ( buffer, &iter );
            gdk_threads_add_idle ( ui_membrowser_update_textbuffer_set_selected, NULL );
            return TRUE;
        };
    };

    //if ( ( event->keyval == GDK_KEY_Up ) || ( event->keyval == GDK_KEY_Down ) || ( event->keyval == GDK_KEY_Left ) || ( event->keyval == GDK_KEY_Right ) || ( event->keyval == GDK_KEY_Page_Up ) || ( event->keyval == GDK_KEY_Page_Down ) || ( event->keyval == GDK_KEY_Home ) || ( event->keyval == GDK_KEY_End ) ) {
    if ( ( event->keyval == GDK_KEY_Up ) || ( event->keyval == GDK_KEY_Down ) || ( event->keyval == GDK_KEY_Left ) || ( event->keyval == GDK_KEY_Right ) || ( event->keyval == GDK_KEY_Page_Up ) || ( event->keyval == GDK_KEY_Page_Down ) ) {

        if ( event->keyval == GDK_KEY_Up ) {
            g_membrowser.key_up = TRUE;
        };

        if ( event->keyval == GDK_KEY_Left ) {
            g_membrowser.key_left = TRUE;
        };

        if ( g_membrowser.key_shift_l || g_membrowser.key_shift_r ) {
            // nechceme povolit selection
            return TRUE;
        } else {

            if ( g_membrowser.mode == MEMBROWSER_MODE_EDIT_ASCII ) {
                if ( g_membrowser.key_ctrl_l || g_membrowser.key_ctrl_r ) {
                    if ( ( event->keyval == GDK_KEY_Left ) || ( event->keyval == GDK_KEY_Right ) ) {
                        // v rezimu EDIT_ASCII nechceme skakat po slovech
                        return TRUE;
                    };
                };
            };


            if ( event->keyval == GDK_KEY_Up ) {
                gint row, col;
                ui_membrowser_get_cursor_position ( &row, &col );
                if ( row != 0 ) return FALSE;
                if ( g_membrowser.page != 0 ) {
                    g_membrowser.selected_addr += MEMBROWSER_ADDRESSES_PER_PAGE - 0x10;
                    ui_membrowser_show_page ( g_membrowser.page - 1 );
                };
                return TRUE;
            };

            if ( event->keyval == GDK_KEY_Down ) {
                gint row, col;
                ui_membrowser_get_cursor_position ( &row, &col );
                if ( row != ( MEMBROWSER_LINES_PER_PAGE - 1 ) ) return FALSE;
                if ( g_membrowser.page != ( g_membrowser.total_pages - 1 ) ) {
                    g_membrowser.selected_addr -= MEMBROWSER_ADDRESSES_PER_PAGE - 0x10;
                    ui_membrowser_show_page ( g_membrowser.page + 1 );
                };
                return TRUE;
            };

            if ( event->keyval == GDK_KEY_Left ) {
                if ( g_membrowser.selected_addr != ui_membrowser_get_first_addr_on_page ( g_membrowser.page ) ) return FALSE;
                gint row, col;
                ui_membrowser_get_cursor_position ( &row, &col );
                if ( ( g_membrowser.mode == MEMBROWSER_MODE_EDIT_HEX ) && ( col > 5 ) ) return FALSE;
                if ( ( g_membrowser.selected_addr != 0x0000 ) ) {
                    g_membrowser.selected_addr = ui_membrowser_get_last_addr_on_page ( g_membrowser.page );
                    ui_membrowser_show_page ( g_membrowser.page - 1 );
                };
                return TRUE;
            };

            if ( event->keyval == GDK_KEY_Right ) {
                if ( g_membrowser.selected_addr != ui_membrowser_get_last_addr_on_page ( g_membrowser.page ) ) return FALSE;
                if ( g_membrowser.selected_addr != ( g_membrowser.mem_size - 1 ) ) {
                    g_membrowser.selected_addr = ui_membrowser_get_first_addr_on_page ( g_membrowser.page );
                    ui_membrowser_show_page ( g_membrowser.page + 1 );
                };
                return TRUE;
            };

            if ( event->keyval == GDK_KEY_Page_Up ) {
                if ( g_membrowser.page != 0 ) {
                    ui_membrowser_show_page ( g_membrowser.page - 1 );
                };
                return TRUE;
            };

            if ( event->keyval == GDK_KEY_Page_Down ) {
                if ( g_membrowser.page != ( g_membrowser.total_pages - 1 ) ) {
                    ui_membrowser_show_page ( g_membrowser.page + 1 );
                };
                return TRUE;
            };

            return TRUE; // pro jistotu
        };
    };

    if ( g_membrowser.mode == MEMBROWSER_MODE_EDIT_HEX ) {
        if (
             ( ( event->keyval >= GDK_KEY_0 ) && ( event->keyval <= GDK_KEY_9 ) ) ||
             ( ( event->keyval >= GDK_KEY_KP_0 ) && ( event->keyval <= GDK_KEY_KP_9 ) ) ||
             ( ( event->keyval >= GDK_KEY_a ) && ( event->keyval <= GDK_KEY_f ) ) ||
             ( ( event->keyval >= GDK_KEY_A ) && ( event->keyval <= GDK_KEY_F ) )
             ) {
            return FALSE;
        } else {
            return TRUE;
        };
    };

    if ( g_membrowser.mode == MEMBROWSER_MODE_EDIT_ASCII ) {
        // BUG: stara verze GTK v ceske verzi windows (?) nelze do TextView napsat znaky ' a "
        if ( ( event->keyval >= 0x20 ) && ( event->keyval < 0x7f ) ) {
            if ( ( event->keyval == GDK_KEY_asciicircum ) || ( event->keyval == GDK_KEY_quoteleft ) ) {
                return TRUE;
            };
            return FALSE;
        } else {
            if ( ( event->keyval >= GDK_KEY_KP_0 ) && ( event->keyval <= GDK_KEY_KP_9 ) ) {
                return FALSE;
            };
            return TRUE;
        };
    };

    return FALSE;
}


G_MODULE_EXPORT gboolean on_dbg_membrowser_textview_key_release_event ( GtkWidget *widget, GdkEventKey *event, gpointer user_data ) {
    if ( event->keyval == GDK_KEY_Shift_L ) {
        g_membrowser.key_shift_l = FALSE;
        return FALSE;
    };

    if ( event->keyval == GDK_KEY_Shift_R ) {
        g_membrowser.key_shift_r = FALSE;
        return FALSE;
    };

    if ( event->keyval == GDK_KEY_Control_L ) {
        g_membrowser.key_ctrl_l = FALSE;
        return FALSE;
    };

    if ( event->keyval == GDK_KEY_Control_R ) {
        g_membrowser.key_ctrl_r = FALSE;
        return FALSE;
    };

    if ( event->keyval == GDK_KEY_Up ) {
        g_membrowser.key_up = FALSE;
        return FALSE;
    };

    if ( event->keyval == GDK_KEY_Left ) {
        g_membrowser.key_left = FALSE;
        return FALSE;
    };
    return FALSE;
}


G_MODULE_EXPORT gboolean on_dbg_membrowser_textview_motion_notify_event ( GtkWidget *widget, GdkEvent *event, gpointer user_data ) {
    // zakazeme selection pomoci mysi
    return TRUE;
}


G_MODULE_EXPORT gboolean on_dbg_membrowser_textview_scroll_event ( GtkWidget *widget, GdkEventScroll *event, gpointer user_data ) {

    GdkScrollDirection direction = event->direction;

    //gdk_event_get_scroll_direction ( (GdkEvent) event, &direction );
    if ( direction == GDK_SCROLL_SMOOTH ) {
        gdouble delta_x = 0;
        gdouble delta_y = 0;
        gdk_event_get_scroll_deltas ( (GdkEvent*) event, &delta_x, &delta_y );
        if ( delta_y < 0 ) {
            direction = GDK_SCROLL_UP;
        } else if ( delta_y > 0 ) {
            direction = GDK_SCROLL_DOWN;
        };
    };

    if ( direction == GDK_SCROLL_UP ) {
        if ( g_membrowser.page != 0 ) {
            ui_membrowser_show_page ( g_membrowser.page - 1 );
        };
    } else if ( direction == GDK_SCROLL_DOWN ) {
        if ( g_membrowser.page != ( g_membrowser.total_pages - 1 ) ) {
            ui_membrowser_show_page ( g_membrowser.page + 1 );
        };
    };
    return TRUE;
}


/*
 * 
 * Callbacky nesouvisejici s textview
 * 
 */


G_MODULE_EXPORT void on_dbg_membrowser_comparative_mode_checkbutton_toggled ( GtkToggleButton *togglebutton, gpointer user_data ) {
    if ( !g_membrowser_initialised ) return;
    g_membrowser.show_comparative = gtk_toggle_button_get_active ( togglebutton );
    ui_membrowser_show_page ( g_membrowser.page );
}


G_MODULE_EXPORT void on_dbg_membrowser_force_screen_refresh_checkbutton_toggled ( GtkToggleButton *togglebutton, gpointer user_data ) {
    if ( !g_membrowser_initialised ) return;
    g_membrowser.forced_screen_refresh = gtk_toggle_button_get_active ( togglebutton );
}


G_MODULE_EXPORT void on_dbg_membrowser_ascii_cnv_comboboxtext_changed ( GtkComboBox *combobox, gpointer user_data ) {
    if ( !g_membrowser_initialised ) return;
    g_membrowser.sh_ascii_conversion = ( gtk_combo_box_get_active ( combobox ) ) ? TRUE : FALSE;
    ui_membrowser_show_page ( g_membrowser.page );
}


G_MODULE_EXPORT void on_dbg_membrowser_switch_mode_togglebutton_toggled ( GtkToggleButton *togglebutton, gpointer user_data ) {
    if ( gtk_toggle_button_get_active ( togglebutton ) ) {
        ui_membrowser_set_mode ( MEMBROWSER_MODE_EDIT_HEX );
    } else {
        ui_membrowser_set_mode ( MEMBROWSER_MODE_VIEW );
    };
}


static int ui_membrowser_source_pezik_is_selected ( int pezik_id ) {

    if ( RAMDISK_CONNECTED != g_ramdisk.pezik[pezik_id].connected ) {
        ui_show_error ( "This memory disc device is not connected!" );
        g_membrowser.lock_source_combotext = TRUE;
        gtk_combo_box_set_active ( GTK_COMBO_BOX ( ui_get_widget ( "dbg_membrowser_source_comboboxtext" ) ), g_membrowser.memsrc );
        g_membrowser.lock_source_combotext = FALSE;
        return EXIT_FAILURE;
    };

    GtkWidget *bank256_combobox = ui_get_widget ( "dbg_membrowser_bank256_combobox" );
    GtkWidget *bank_combotext = ui_get_widget ( "dbg_membrowser_bank_comboboxtext" );
    GtkWidget *pezik_addr_combotext = ui_get_widget ( "dbg_membrowser_pezik_addressing_comboboxtext" );
    GtkWidget *forced_scr_refresh = ui_get_widget ( "dbg_membrowser_force_screen_refresh_checkbutton" );

    g_membrowser.lock_bank_combotext = TRUE;
    gtk_combo_box_text_remove_all ( GTK_COMBO_BOX_TEXT ( bank_combotext ) );
    int first_bank = -1;
    int i;
    Z80EX_BYTE port = ( pezik_id == RAMDISK_PEZIK_68 ) ? 0x68 : 0xe8;
    for ( i = 0; i < 8; i++ ) {
        if ( ( g_ramdisk.pezik[pezik_id].portmask >> i ) & 1 ) {
            if ( first_bank == -1 ) first_bank = i;
            char buff[20];
            snprintf ( buff, sizeof ( buff ), "Bank %d (%02X)", i, port + i );
            gtk_combo_box_text_append_text ( GTK_COMBO_BOX_TEXT ( bank_combotext ), buff );
        };
    };
    int set_bank = ( ( g_ramdisk.pezik[pezik_id].portmask >> g_membrowser.pezik_bank[pezik_id] ) & 1 ) ? g_membrowser.pezik_bank[pezik_id] : first_bank;
    gtk_combo_box_set_active ( GTK_COMBO_BOX ( bank_combotext ), set_bank );
    g_membrowser.pezik_bank[pezik_id] = set_bank;
    g_membrowser.lock_bank_combotext = FALSE;

    g_membrowser.lock_pezik_addr_combotext = TRUE;
    //g_membrowser.pezik_addressing = MEMBROWSER_PEZIK_ADDRESSING_BE;
    gtk_combo_box_set_active ( GTK_COMBO_BOX ( pezik_addr_combotext ), g_membrowser.pezik_addressing );
    g_membrowser.lock_pezik_addr_combotext = FALSE;

    gtk_widget_hide ( bank256_combobox );
    gtk_widget_show ( bank_combotext );
    gtk_widget_show ( pezik_addr_combotext );
    gtk_widget_hide ( forced_scr_refresh );

    return EXIT_SUCCESS;
}


G_MODULE_EXPORT void on_dbg_membrowser_source_comboboxtext_changed ( GtkComboBox *combobox, gpointer user_data ) {
    if ( g_membrowser.lock_source_combotext ) return;

    en_MEMBROWSER_SOURCE memsrc = gtk_combo_box_get_active ( combobox );

    GtkWidget *bank256_combobox = ui_get_widget ( "dbg_membrowser_bank256_combobox" );
    GtkWidget *bank_combotext = ui_get_widget ( "dbg_membrowser_bank_comboboxtext" );
    GtkWidget *pezik_addr_combotext = ui_get_widget ( "dbg_membrowser_pezik_addressing_comboboxtext" );
    GtkWidget *forced_scr_refresh = ui_get_widget ( "dbg_membrowser_force_screen_refresh_checkbutton" );

    switch ( memsrc ) {
        case MEMBROWSER_SOURCE_MAPED:
        case MEMBROWSER_SOURCE_RAM:
        case MEMBROWSER_SOURCE_MZ700ROM:
        case MEMBROWSER_SOURCE_CGROM:
        case MEMBROWSER_SOURCE_MZ800ROM:
            gtk_widget_hide ( bank256_combobox );
            gtk_widget_hide ( bank_combotext );
            gtk_widget_hide ( pezik_addr_combotext );
            if ( memsrc == MEMBROWSER_SOURCE_MAPED ) {
                gtk_widget_show ( forced_scr_refresh );
            } else {
                gtk_widget_hide ( forced_scr_refresh );
            };
            break;

        case MEMBROWSER_SOURCE_VRAM:
            g_membrowser.lock_bank_combotext = TRUE;
            gtk_combo_box_text_remove_all ( GTK_COMBO_BOX_TEXT ( bank_combotext ) );
            gtk_combo_box_text_append_text ( GTK_COMBO_BOX_TEXT ( bank_combotext ), "Plane I" );
            gtk_combo_box_text_append_text ( GTK_COMBO_BOX_TEXT ( bank_combotext ), "Plane II" );
            gtk_combo_box_text_append_text ( GTK_COMBO_BOX_TEXT ( bank_combotext ), "Plane III" );
            gtk_combo_box_text_append_text ( GTK_COMBO_BOX_TEXT ( bank_combotext ), "Plane IV" );
            gtk_combo_box_set_active ( GTK_COMBO_BOX ( bank_combotext ), g_membrowser.vram_plane );
            g_membrowser.lock_bank_combotext = FALSE;
            gtk_widget_hide ( bank256_combobox );
            gtk_widget_show ( bank_combotext );
            gtk_widget_hide ( pezik_addr_combotext );
            gtk_widget_show ( forced_scr_refresh );
            break;

        case MEMBROWSER_SOURCE_PEZIK_68:
            if ( EXIT_SUCCESS != ui_membrowser_source_pezik_is_selected ( RAMDISK_PEZIK_68 ) ) {
                return;
            };
            break;

        case MEMBROWSER_SOURCE_PEZIK_E8:
            if ( EXIT_SUCCESS != ui_membrowser_source_pezik_is_selected ( RAMDISK_PEZIK_E8 ) ) {
                return;
            };
            break;

        case MEMBROWSER_SOURCE_MZ1R18:
            if ( g_ramdisk.std.connected != RAMDISK_CONNECTED ) {
                ui_show_error ( "This memory disc device is not connected!" );
                g_membrowser.lock_source_combotext = TRUE;
                gtk_combo_box_set_active ( combobox, g_membrowser.memsrc );
                g_membrowser.lock_source_combotext = FALSE;
                return;
            };

            if ( g_ramdisk.std.size == RAMDISK_SIZE_16M ) {
                g_membrowser.lock_bank256_combobox = TRUE;

                GtkTreeModel *model = gtk_combo_box_get_model ( GTK_COMBO_BOX ( bank256_combobox ) );
                GtkTreeIter iter;
                char path_txt[20];
                snprintf ( path_txt, sizeof ( path_txt ), "%d:%d", ( g_membrowser.mr1z18_bank >> 4 ), ( g_membrowser.mr1z18_bank & 0x0f ) );
                printf ( "PATH=%s\n", path_txt );
                gtk_tree_model_get_iter_from_string ( model, &iter, path_txt );
                gtk_combo_box_set_active_iter ( GTK_COMBO_BOX ( bank256_combobox ), &iter );
                g_membrowser.lock_bank256_combobox = FALSE;
                gtk_widget_show ( bank256_combobox );
                gtk_widget_hide ( bank_combotext );
            } else {
                g_membrowser.lock_bank_combotext = TRUE;
                gtk_combo_box_text_remove_all ( GTK_COMBO_BOX_TEXT ( bank_combotext ) );
                int i;
                for ( i = 0; i <= g_ramdisk.std.size; i++ ) {
                    char buff[20];
                    snprintf ( buff, sizeof ( buff ), "Bank %d", i );
                    gtk_combo_box_text_append_text ( GTK_COMBO_BOX_TEXT ( bank_combotext ), buff );
                };
                if ( g_membrowser.mr1z18_bank > g_ramdisk.std.size ) {
                    g_membrowser.mr1z18_bank = 0;
                };
                gtk_combo_box_set_active ( GTK_COMBO_BOX ( bank_combotext ), g_membrowser.mr1z18_bank );
                g_membrowser.lock_bank_combotext = FALSE;
                gtk_widget_hide ( bank256_combobox );
                gtk_widget_show ( bank_combotext );
            };

            gtk_widget_hide ( pezik_addr_combotext );
            gtk_widget_hide ( forced_scr_refresh );
            break;

        case MEMBROWSER_SOURCE_MEMEXT_PEHU:
        case MEMBROWSER_SOURCE_MEMEXT_LUFTNER:
            if ( ( memsrc == MEMBROWSER_SOURCE_MEMEXT_PEHU ) && ( !TEST_MEMEXT_CONNECTED_PEHU ) ) {
                ui_show_error ( "This memory disc device is not connected!" );
                g_membrowser.lock_source_combotext = TRUE;
                gtk_combo_box_set_active ( combobox, g_membrowser.memsrc );
                g_membrowser.lock_source_combotext = FALSE;
                return;
            } else if ( ( memsrc == MEMBROWSER_SOURCE_MEMEXT_LUFTNER ) && ( !TEST_MEMEXT_CONNECTED_LUFTNER ) ) {
                ui_show_error ( "This memory disc device is not connected!" );
                g_membrowser.lock_source_combotext = TRUE;
                gtk_combo_box_set_active ( combobox, g_membrowser.memsrc );
                g_membrowser.lock_source_combotext = FALSE;
                return;
            };

            if ( memsrc == MEMBROWSER_SOURCE_MEMEXT_LUFTNER ) {
                g_membrowser.lock_bank256_combobox = TRUE;

                GtkTreeModel *model = gtk_combo_box_get_model ( GTK_COMBO_BOX ( bank256_combobox ) );
                GtkTreeIter iter;
                char path_txt[20];
                snprintf ( path_txt, sizeof ( path_txt ), "%d:%d", ( g_membrowser.memext_luftner_bank >> 4 ), ( g_membrowser.memext_luftner_bank & 0x0f ) );
                gtk_tree_model_get_iter_from_string ( model, &iter, path_txt );
                gtk_combo_box_set_active_iter ( GTK_COMBO_BOX ( bank256_combobox ), &iter );
                g_membrowser.lock_bank256_combobox = FALSE;
                gtk_widget_show ( bank256_combobox );
                gtk_widget_hide ( bank_combotext );
            } else {
                g_membrowser.lock_bank_combotext = TRUE;
                gtk_combo_box_text_remove_all ( GTK_COMBO_BOX_TEXT ( bank_combotext ) );
                int i;
                for ( i = 0; i < MEMEXT_PEHU_BANKS; i++ ) {
                    char buff[20];
                    snprintf ( buff, sizeof ( buff ), "Bank %d", i );
                    gtk_combo_box_text_append_text ( GTK_COMBO_BOX_TEXT ( bank_combotext ), buff );
                };
                if ( g_membrowser.memext_pehu_bank >= MEMEXT_PEHU_BANKS ) {
                    g_membrowser.memext_pehu_bank = 0;
                };
                gtk_combo_box_set_active ( GTK_COMBO_BOX ( bank_combotext ), g_membrowser.memext_pehu_bank );
                g_membrowser.lock_bank_combotext = FALSE;
                gtk_widget_hide ( bank256_combobox );
                gtk_widget_show ( bank_combotext );
            };

            gtk_widget_hide ( pezik_addr_combotext );
            gtk_widget_hide ( forced_scr_refresh );
            break;
    };

    g_membrowser.memsrc = memsrc;
    memset ( g_membrowser.data_current, 0x00, MEMBROWSER_MEM_MAX );
    g_membrowser.selected_addr = 0;
    g_membrowser.page = 0;
    ui_membrowser_load_data_from_memsrc ( );
    ui_membrowser_show_page ( g_membrowser.page );
}


void ui_membrowser_refresh ( void ) {
    ui_membrowser_load_data_from_memsrc ( );
    ui_membrowser_show_page ( g_membrowser.page );
}


G_MODULE_EXPORT void on_dbg_membrowser_refresh_button_clicked ( GtkButton *button, gpointer user_data ) {
    ui_membrowser_refresh ( );
}


G_MODULE_EXPORT void on_dbg_mebrowser_goto_addr_hex_entry_changed ( GtkEditable *ed, gpointer user_data ) {
    if ( g_membrowser.lock_goto_entry ) return;
    g_membrowser.lock_goto_entry = TRUE;
    ui_hexeditable_changed ( ed, user_data );

    uint32_t addr = debuger_hextext_to_uint32 ( gtk_entry_get_text ( ui_get_entry ( "dbg_mebrowser_goto_addr_hex_entry" ) ) );

    char buff[9];
    buff[0] = 0x00;
    if ( gtk_entry_get_text_length ( ui_get_entry ( "dbg_mebrowser_goto_addr_hex_entry" ) ) ) {
        snprintf ( buff, sizeof ( buff ), "%d", addr );
    };
    gtk_entry_set_text ( ui_get_entry ( "dbg_mebrowser_goto_addr_dec_entry" ), buff );
    g_membrowser.lock_goto_entry = FALSE;
}


G_MODULE_EXPORT void on_dbg_mebrowser_goto_addr_dec_entry_changed ( GtkEditable *ed, gpointer user_data ) {
    if ( g_membrowser.lock_goto_entry ) return;
    g_membrowser.lock_goto_entry = TRUE;
    ui_digiteditable_changed ( ed, user_data );

    uint32_t addr = atoi ( gtk_entry_get_text ( ui_get_entry ( "dbg_mebrowser_goto_addr_dec_entry" ) ) );

    char buff[11];
    buff[0] = 0x00;
    if ( gtk_entry_get_text_length ( ui_get_entry ( "dbg_mebrowser_goto_addr_dec_entry" ) ) ) {
        snprintf ( buff, sizeof ( buff ), "%04X", addr );
    };
    g_membrowser.lock_goto_entry = TRUE;
    gtk_entry_set_text ( ui_get_entry ( "dbg_mebrowser_goto_addr_hex_entry" ), buff );
    g_membrowser.lock_goto_entry = FALSE;
}


static void ui_membrowser_goto_addr_action ( void ) {

    if ( !gtk_entry_get_text_length ( ui_get_entry ( "dbg_mebrowser_goto_addr_hex_entry" ) ) ) {
        g_membrowser.lock_goto_entry = TRUE;
        gtk_entry_set_text ( ui_get_entry ( "dbg_mebrowser_goto_addr_hex_entry" ), "0000" );
        g_membrowser.lock_goto_entry = FALSE;
    };

    uint32_t addr = debuger_hextext_to_uint32 ( gtk_entry_get_text ( ui_get_entry ( "dbg_mebrowser_goto_addr_hex_entry" ) ) );

    if ( addr > ( g_membrowser.mem_size - 1 ) ) {
        addr = g_membrowser.mem_size - 1;
        char buff[9];
        snprintf ( buff, sizeof ( buff ), "%04X", addr );
        gtk_entry_set_text ( ui_get_entry ( "dbg_mebrowser_goto_addr_hex_entry" ), buff );
    };

    uint32_t page = ui_membrowser_get_page_by_addr ( addr );
    GtkWidget *view = ui_get_widget ( "dbg_membrowser_textview" );

    if ( page != g_membrowser.page ) {
        g_membrowser.selected_addr = addr;
        ui_membrowser_show_page ( page );
    } else {
        uint32_t offset;
        if ( g_membrowser.mode == MEMBROWSER_MODE_EDIT_ASCII ) {
            offset = ui_membrowser_get_ascii_offset_by_addr ( addr );
        } else {
            offset = ui_membrowser_get_hex_offset_by_addr ( addr );
        };

        GtkTextBuffer *buffer = gtk_text_view_get_buffer ( GTK_TEXT_VIEW ( view ) );
        GtkTextIter iter;
        gtk_text_buffer_get_iter_at_offset ( buffer, &iter, offset );
        gtk_text_buffer_place_cursor ( buffer, &iter );
        ui_membrowser_update_selected_addr ( );
    };
    gtk_widget_grab_focus ( view );
}


G_MODULE_EXPORT void on_dbg_membrowser_goto_addr_button_clicked ( GtkButton *button, gpointer data ) {
    ui_membrowser_goto_addr_action ( );
}


G_MODULE_EXPORT gboolean on_dbg_mebrowser_goto_addr_hex_entry_key_press_event ( GtkWidget *widget, GdkEventKey *event, gpointer user_data ) {
    if ( ( event->keyval == GDK_KEY_Return ) || ( event->keyval == GDK_KEY_KP_Enter ) ) {
        ui_membrowser_goto_addr_action ( );
    };
    return FALSE;
}


G_MODULE_EXPORT gboolean on_dbg_mebrowser_goto_addr_dec_entry_key_press_event ( GtkWidget *widget, GdkEventKey *event, gpointer user_data ) {
    //printf ( "k: 0x%04x\n", event->keyval );
    if ( ( event->keyval == GDK_KEY_Return ) || ( event->keyval == GDK_KEY_KP_Enter ) ) {
        ui_membrowser_goto_addr_action ( );
    };
    return FALSE;
}


G_MODULE_EXPORT void on_dbg_membrowser_bank_comboboxtext_changed ( GtkComboBox *combobox, gpointer user_data ) {
    if ( g_membrowser.lock_bank_combotext ) return;

    int bank = gtk_combo_box_get_active ( combobox );
    if ( g_membrowser.memsrc == MEMBROWSER_SOURCE_VRAM ) {
        g_membrowser.vram_plane = bank;
    } else if ( g_membrowser.memsrc == MEMBROWSER_SOURCE_PEZIK_68 ) {
        g_membrowser.pezik_bank[RAMDISK_PEZIK_68] = bank;
    } else if ( g_membrowser.memsrc == MEMBROWSER_SOURCE_PEZIK_E8 ) {
        g_membrowser.pezik_bank[RAMDISK_PEZIK_E8] = bank;
    } else if ( g_membrowser.memsrc == MEMBROWSER_SOURCE_MZ1R18 ) {
        g_membrowser.mr1z18_bank = bank;
    } else if ( g_membrowser.memsrc == MEMBROWSER_SOURCE_MEMEXT_PEHU ) {
        g_membrowser.memext_pehu_bank = bank;
    } else {
        fprintf ( stderr, "%s():%d - Unknown memsrc 0x%02x\n", __func__, __LINE__, g_membrowser.memsrc );
    };

    ui_membrowser_load_data_from_memsrc ( );
    ui_membrowser_show_page ( g_membrowser.page );
}


G_MODULE_EXPORT void on_dbg_membrowser_bank256_combobox_changed ( GtkComboBox *widget, gpointer user_data ) {
    if ( g_membrowser.lock_bank256_combobox ) return;

    GtkTreeIter iter;
    gtk_combo_box_get_active_iter ( widget, &iter );
    GtkTreeModel *model = gtk_combo_box_get_model ( widget );
    GValue gv_bank = G_VALUE_INIT;
    gtk_tree_model_get_value ( model, &iter, 1, &gv_bank );

    if ( g_membrowser.memsrc == MEMBROWSER_SOURCE_MZ1R18 ) {
        g_membrowser.mr1z18_bank = g_value_get_uint ( &gv_bank );
    } else if ( g_membrowser.memsrc == MEMBROWSER_SOURCE_MEMEXT_LUFTNER ) {
        g_membrowser.memext_luftner_bank = g_value_get_uint ( &gv_bank );
    } else {
        fprintf ( stderr, "%s():%d - Unknown memsrc 0x%02x\n", __func__, __LINE__, g_membrowser.memsrc );
    };

    ui_membrowser_load_data_from_memsrc ( );
    ui_membrowser_show_page ( g_membrowser.page );
}


G_MODULE_EXPORT void on_dbg_membrowser_pezik_addressing_comboboxtext_changed ( GtkComboBox *combobox, gpointer user_data ) {
    if ( g_membrowser.lock_pezik_addr_combotext ) return;
    g_membrowser.pezik_addressing = gtk_combo_box_get_active ( combobox );
    ui_membrowser_load_data_from_memsrc ( );
    ui_membrowser_show_page ( g_membrowser.page );
}


static void ui_membrowser_load_block_cb ( uint32_t addr, uint8_t *data, uint32_t size, void *user_data ) {

    uint32_t mem_max = g_membrowser.mem_size - 1;

    if ( g_membrowser.MEM != NULL ) {

        uint32_t src_addr = 0;

        if ( ( ( g_membrowser.memsrc == MEMBROWSER_SOURCE_PEZIK_E8 ) || ( g_membrowser.memsrc == MEMBROWSER_SOURCE_PEZIK_68 ) ) && ( g_membrowser.pezik_addressing == MEMBROWSER_PEZIK_ADDRESSING_LE ) ) {
            /* pezik little endian */
            while ( size ) {
                unsigned le_addr = addr + src_addr;
                g_membrowser.MEM[ ( ( le_addr & 0xff ) << 8 ) | ( le_addr >> 8 )] = data[src_addr++];
                size--;
            };
        } else {
            while ( size ) {
                uint32_t i = addr + size;
                uint32_t load_size = size;

                if ( i > mem_max ) {
                    load_size = mem_max - addr;
                };

                memcpy ( &g_membrowser.MEM[addr], &data[src_addr], load_size );

                size -= load_size;
                src_addr += load_size;
                addr += load_size + 1;
                addr &= mem_max;
            };
        };

    } else {
        if ( g_membrowser.memsrc == MEMBROWSER_SOURCE_MAPED ) {
            memory_load_block ( data, addr, size, MEMORY_LOAD_MAPED );
        } else if ( g_membrowser.memsrc == MEMBROWSER_SOURCE_RAM ) {
            memory_load_block ( data, addr, size, MEMORY_LOAD_RAMONLY );
        } else {
            fprintf ( stderr, "%s():%d - Unsupported memory source (%d)\n", __func__, __LINE__, g_membrowser.memsrc );
        };
    };

    // Pokud se psalo do VRAM pres "MAPED", tak by se mel provest update obrazu sam, 
    // ale po primem zapisu do g_memoryVRAM_* je potreba udelat update framebufferu a prekreslit okno.
    if ( ( g_membrowser.memsrc == MEMBROWSER_SOURCE_VRAM ) || ( g_membrowser.memsrc == MEMBROWSER_SOURCE_CGROM ) ) {
        debugger_forced_screen_update ( );
    };

    g_free ( data );

    ui_membrowser_refresh ( );
}


G_MODULE_EXPORT void on_dbg_membrowser_load_button_clicked ( GtkButton *button, gpointer data ) {
    ui_memload_select_file ( NULL, ui_membrowser_load_block_cb, g_membrowser.mem_size, NULL );
}


G_MODULE_EXPORT void on_dbg_membrowser_save_button_clicked ( GtkButton *button, gpointer data ) {
    ui_memsave_window_show ( g_membrowser.data_current, g_membrowser.mem_size, TRUE );
}
#endif
