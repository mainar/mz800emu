/* 
 * File:   ui_dbg_memext.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 19. července 2018, 19:51
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
#include <gtk/gtk.h>
#include <stdlib.h>

#ifdef MZ800EMU_CFG_DEBUGGER_ENABLED

#include "ui/ui_main.h"
#include "ui/ui_hexeditable.h"
#include "debugger/debugger.h"

#include "memory/memext.h"


static int g_ui_dbg_memext_state = 0;
static en_MEMEXT_TYPE g_ui_dbg_memext_type = MEMEXT_TYPE_PEHU;
static gboolean g_ui_dbg_memext_lock = FALSE;


G_MODULE_EXPORT gboolean on_dbg_memext_window_delete_event ( GtkWidget *widget, GdkEvent *event, gpointer user_data ) {
    gtk_widget_hide ( widget );
    return TRUE;
}


G_MODULE_EXPORT gboolean on_dbg_memext_window_key_press_event ( GtkWidget *widget, GdkEventKey *event, gpointer user_data ) {
    if ( event->keyval == GDK_KEY_Escape ) {
        gtk_widget_hide ( ui_get_widget ( "dbg_memext_window" ) );
        return TRUE;
    };
    return FALSE;
}


static void ui_dbg_memext_check_ok_sensitivity ( void ) {
    gboolean sensitive = TRUE;
    if ( g_ui_dbg_memext_state == 0 ) {
        sensitive = FALSE;
    } else {
        int i;
        int step = ( g_ui_dbg_memext_type == MEMEXT_TYPE_PEHU ) ? 2 : 1;
        for ( i = 0; i < MEMEXT_RAW_MAP_SIZE; i += step ) {
            char hex_entry_name[22];
            snprintf ( hex_entry_name, sizeof ( hex_entry_name ), "dbg_memext_hex_bank_%X", i );
            if ( !gtk_entry_get_text_length ( ui_get_entry ( hex_entry_name ) ) ) {
                sensitive = FALSE;
                break;
            };
        };
    };

    gtk_widget_set_sensitive ( ui_get_widget ( "dbg_memext_apply_button" ), sensitive );
    gtk_widget_set_sensitive ( ui_get_widget ( "dbg_memext_ok_button" ), sensitive );
}


static void ui_dbg_memext_set_memtype_label ( int mempoint, char *txt ) {
    char label_name[21];
    snprintf ( label_name, sizeof ( label_name ), "dbg_memext_memtype_%X", mempoint );
    gtk_label_set_text ( ui_get_label ( label_name ), txt );
    if ( ( g_ui_dbg_memext_type == MEMEXT_TYPE_PEHU ) && ( !( mempoint & 0x01 ) ) ) {
        snprintf ( label_name, sizeof ( label_name ), "dbg_memext_memtype_%X", mempoint + 1 );
        gtk_label_set_text ( ui_get_label ( label_name ), txt );
    };
}


static void ui_dbg_memext_update_memtype ( int mempoint ) {
    if ( ( g_ui_dbg_memext_type == MEMEXT_TYPE_PEHU ) && ( mempoint & 0x01 ) ) return;
    char hex_entry_name[22];
    snprintf ( hex_entry_name, sizeof ( hex_entry_name ), "dbg_memext_hex_bank_%X", mempoint );

    if ( !gtk_entry_get_text_length ( ui_get_entry ( hex_entry_name ) ) ) {
        ui_dbg_memext_set_memtype_label ( mempoint, "UNKNOWN" );
        return;
    };

    uint32_t value = debuger_hextext_to_uint32 ( gtk_entry_get_text ( ui_get_entry ( hex_entry_name ) ) );
    ui_dbg_memext_set_memtype_label ( mempoint, ( value & 0x80 ) ? "FLASH" : "RAM" );
}


static void ui_dbg_memext_hexedit_changed ( GtkEditable *ed, int mempoint ) {
    if ( g_ui_dbg_memext_lock ) return;
    ui_hexeditable_changed ( ed, NULL );

    char hex_entry_name[22];
    snprintf ( hex_entry_name, sizeof ( hex_entry_name ), "dbg_memext_hex_bank_%X", mempoint );
    char dec_entry_name[22];
    snprintf ( dec_entry_name, sizeof ( dec_entry_name ), "dbg_memext_dec_bank_%X", mempoint );

    if ( !gtk_entry_get_text_length ( ui_get_entry ( hex_entry_name ) ) ) {
        g_ui_dbg_memext_lock = TRUE;
        gtk_entry_set_text ( ui_get_entry ( dec_entry_name ), "" );
        g_ui_dbg_memext_lock = FALSE;
        ui_dbg_memext_update_memtype ( mempoint );
        ui_dbg_memext_check_ok_sensitivity ( );
        return;
    };

    uint32_t max_value = ( g_ui_dbg_memext_type == MEMEXT_TYPE_PEHU ) ? 0x3f : 0xff;
    uint32_t value = debuger_hextext_to_uint32 ( gtk_entry_get_text ( ui_get_entry ( hex_entry_name ) ) );

    g_ui_dbg_memext_lock = TRUE;

    if ( value > max_value ) {
        value = max_value;
        char buff[3];
        snprintf ( buff, sizeof ( buff ), "%02X", value );
        gtk_entry_set_text ( ui_get_entry ( hex_entry_name ), buff );
    };

    char buff[4];
    snprintf ( buff, sizeof ( buff ), "%d", value );
    gtk_entry_set_text ( ui_get_entry ( dec_entry_name ), buff );

    g_ui_dbg_memext_lock = FALSE;

    ui_dbg_memext_update_memtype ( mempoint );
    ui_dbg_memext_check_ok_sensitivity ( );
}


static void ui_dbg_memext_decedit_changed ( GtkEditable *ed, int mempoint ) {
    if ( g_ui_dbg_memext_lock ) return;
    ui_digiteditable_changed ( ed, NULL );

    char hex_entry_name[22];
    snprintf ( hex_entry_name, sizeof ( hex_entry_name ), "dbg_memext_hex_bank_%X", mempoint );
    char dec_entry_name[22];
    snprintf ( dec_entry_name, sizeof ( dec_entry_name ), "dbg_memext_dec_bank_%X", mempoint );

    if ( !gtk_entry_get_text_length ( ui_get_entry ( dec_entry_name ) ) ) {
        g_ui_dbg_memext_lock = TRUE;
        gtk_entry_set_text ( ui_get_entry ( hex_entry_name ), "" );
        g_ui_dbg_memext_lock = FALSE;
        ui_dbg_memext_update_memtype ( mempoint );
        ui_dbg_memext_check_ok_sensitivity ( );
        return;
    };

    uint32_t max_value = ( g_ui_dbg_memext_type == MEMEXT_TYPE_PEHU ) ? 0x3f : 0xff;
    uint32_t value = atoi ( gtk_entry_get_text ( ui_get_entry ( dec_entry_name ) ) );

    g_ui_dbg_memext_lock = TRUE;

    if ( value > max_value ) {
        value = max_value;
        char buff[4];
        snprintf ( buff, sizeof ( buff ), "%d", value );
        gtk_entry_set_text ( ui_get_entry ( dec_entry_name ), buff );
    };

    char buff[3];
    snprintf ( buff, sizeof ( buff ), "%02X", value );
    gtk_entry_set_text ( ui_get_entry ( hex_entry_name ), buff );

    g_ui_dbg_memext_lock = FALSE;

    ui_dbg_memext_update_memtype ( mempoint );
    ui_dbg_memext_check_ok_sensitivity ( );
}


G_MODULE_EXPORT void on_dbg_memext_hex_bank_0_changed ( GtkEditable *ed, gpointer user_data ) {
    ui_dbg_memext_hexedit_changed ( ed, 0x00 );
}


G_MODULE_EXPORT void on_dbg_memext_hex_bank_1_changed ( GtkEditable *ed, gpointer user_data ) {
    ui_dbg_memext_hexedit_changed ( ed, 0x01 );
}


G_MODULE_EXPORT void on_dbg_memext_hex_bank_2_changed ( GtkEditable *ed, gpointer user_data ) {
    ui_dbg_memext_hexedit_changed ( ed, 0x02 );
}


G_MODULE_EXPORT void on_dbg_memext_hex_bank_3_changed ( GtkEditable *ed, gpointer user_data ) {
    ui_dbg_memext_hexedit_changed ( ed, 0x03 );
}


G_MODULE_EXPORT void on_dbg_memext_hex_bank_4_changed ( GtkEditable *ed, gpointer user_data ) {
    ui_dbg_memext_hexedit_changed ( ed, 0x04 );
}


G_MODULE_EXPORT void on_dbg_memext_hex_bank_5_changed ( GtkEditable *ed, gpointer user_data ) {
    ui_dbg_memext_hexedit_changed ( ed, 0x05 );
}


G_MODULE_EXPORT void on_dbg_memext_hex_bank_6_changed ( GtkEditable *ed, gpointer user_data ) {
    ui_dbg_memext_hexedit_changed ( ed, 0x06 );
}


G_MODULE_EXPORT void on_dbg_memext_hex_bank_7_changed ( GtkEditable *ed, gpointer user_data ) {
    ui_dbg_memext_hexedit_changed ( ed, 0x07 );
}


G_MODULE_EXPORT void on_dbg_memext_hex_bank_8_changed ( GtkEditable *ed, gpointer user_data ) {
    ui_dbg_memext_hexedit_changed ( ed, 0x08 );
}


G_MODULE_EXPORT void on_dbg_memext_hex_bank_9_changed ( GtkEditable *ed, gpointer user_data ) {
    ui_dbg_memext_hexedit_changed ( ed, 0x09 );
}


G_MODULE_EXPORT void on_dbg_memext_hex_bank_A_changed ( GtkEditable *ed, gpointer user_data ) {
    ui_dbg_memext_hexedit_changed ( ed, 0x0A );
}


G_MODULE_EXPORT void on_dbg_memext_hex_bank_B_changed ( GtkEditable *ed, gpointer user_data ) {
    ui_dbg_memext_hexedit_changed ( ed, 0x0B );
}


G_MODULE_EXPORT void on_dbg_memext_hex_bank_C_changed ( GtkEditable *ed, gpointer user_data ) {
    ui_dbg_memext_hexedit_changed ( ed, 0x0C );
}


G_MODULE_EXPORT void on_dbg_memext_hex_bank_D_changed ( GtkEditable *ed, gpointer user_data ) {
    ui_dbg_memext_hexedit_changed ( ed, 0x0D );
}


G_MODULE_EXPORT void on_dbg_memext_hex_bank_E_changed ( GtkEditable *ed, gpointer user_data ) {
    ui_dbg_memext_hexedit_changed ( ed, 0x0E );
}


G_MODULE_EXPORT void on_dbg_memext_hex_bank_F_changed ( GtkEditable *ed, gpointer user_data ) {
    ui_dbg_memext_hexedit_changed ( ed, 0x0F );
}


G_MODULE_EXPORT void on_dbg_memext_dec_bank_0_changed ( GtkEditable *ed, gpointer user_data ) {
    ui_dbg_memext_decedit_changed ( ed, 0x00 );
}


G_MODULE_EXPORT void on_dbg_memext_dec_bank_1_changed ( GtkEditable *ed, gpointer user_data ) {
    ui_dbg_memext_decedit_changed ( ed, 0x01 );
}


G_MODULE_EXPORT void on_dbg_memext_dec_bank_2_changed ( GtkEditable *ed, gpointer user_data ) {
    ui_dbg_memext_decedit_changed ( ed, 0x02 );
}


G_MODULE_EXPORT void on_dbg_memext_dec_bank_3_changed ( GtkEditable *ed, gpointer user_data ) {
    ui_dbg_memext_decedit_changed ( ed, 0x03 );
}


G_MODULE_EXPORT void on_dbg_memext_dec_bank_4_changed ( GtkEditable *ed, gpointer user_data ) {
    ui_dbg_memext_decedit_changed ( ed, 0x04 );
}


G_MODULE_EXPORT void on_dbg_memext_dec_bank_5_changed ( GtkEditable *ed, gpointer user_data ) {
    ui_dbg_memext_decedit_changed ( ed, 0x05 );
}


G_MODULE_EXPORT void on_dbg_memext_dec_bank_6_changed ( GtkEditable *ed, gpointer user_data ) {
    ui_dbg_memext_decedit_changed ( ed, 0x06 );
}


G_MODULE_EXPORT void on_dbg_memext_dec_bank_7_changed ( GtkEditable *ed, gpointer user_data ) {
    ui_dbg_memext_decedit_changed ( ed, 0x07 );
}


G_MODULE_EXPORT void on_dbg_memext_dec_bank_8_changed ( GtkEditable *ed, gpointer user_data ) {
    ui_dbg_memext_decedit_changed ( ed, 0x08 );
}


G_MODULE_EXPORT void on_dbg_memext_dec_bank_9_changed ( GtkEditable *ed, gpointer user_data ) {
    ui_dbg_memext_decedit_changed ( ed, 0x09 );
}


G_MODULE_EXPORT void on_dbg_memext_dec_bank_A_changed ( GtkEditable *ed, gpointer user_data ) {
    ui_dbg_memext_decedit_changed ( ed, 0x0A );
}


G_MODULE_EXPORT void on_dbg_memext_dec_bank_B_changed ( GtkEditable *ed, gpointer user_data ) {
    ui_dbg_memext_decedit_changed ( ed, 0x0B );
}


G_MODULE_EXPORT void on_dbg_memext_dec_bank_C_changed ( GtkEditable *ed, gpointer user_data ) {
    ui_dbg_memext_decedit_changed ( ed, 0x0C );
}


G_MODULE_EXPORT void on_dbg_memext_dec_bank_D_changed ( GtkEditable *ed, gpointer user_data ) {
    ui_dbg_memext_decedit_changed ( ed, 0x0D );
}


G_MODULE_EXPORT void on_dbg_memext_dec_bank_E_changed ( GtkEditable *ed, gpointer user_data ) {
    ui_dbg_memext_decedit_changed ( ed, 0x0E );
}


G_MODULE_EXPORT void on_dbg_memext_dec_bank_F_changed ( GtkEditable *ed, gpointer user_data ) {
    ui_dbg_memext_decedit_changed ( ed, 0x0F );
}


static void ui_dbg_memext_refresh ( void ) {
    g_ui_dbg_memext_state = ( TEST_MEMEXT_CONNECTED_PEHU ) | ( TEST_MEMEXT_CONNECTED_LUFTNER << 1 );
    g_ui_dbg_memext_type = g_memext.type;

    if ( !g_ui_dbg_memext_state ) {
        gtk_widget_set_visible ( ui_get_widget ( "dbg_memext_type_label" ), FALSE );
        gtk_widget_set_visible ( ui_get_widget ( "dbg_memext_type_colon_label" ), FALSE );
        gtk_label_set_text ( ui_get_label ( "dbg_memext_type_value_label" ), "MEMEXT IS NOT CONNECTED" );

        gtk_widget_set_visible ( ui_get_widget ( "dbg_memext_bank_size_label" ), FALSE );
        gtk_widget_set_visible ( ui_get_widget ( "dbg_memext_bank_size_colon_label" ), FALSE );
        gtk_widget_set_visible ( ui_get_widget ( "dbg_memext_bank_size_value_label" ), FALSE );

        gtk_widget_set_visible ( ui_get_widget ( "dbg_memext_ram_banks_label" ), FALSE );
        gtk_widget_set_visible ( ui_get_widget ( "dbg_memext_ram_banks_colon_label" ), FALSE );
        gtk_widget_set_visible ( ui_get_widget ( "dbg_memext_ram_banks_value_label" ), FALSE );

        gtk_widget_set_visible ( ui_get_widget ( "dbg_memext_flash_banks_label" ), FALSE );
        gtk_widget_set_visible ( ui_get_widget ( "dbg_memext_flash_banks_colon_label" ), FALSE );
        gtk_widget_set_visible ( ui_get_widget ( "dbg_memext_flash_banks_value_label" ), FALSE );

        int i;
        for ( i = 0; i < MEMEXT_RAW_MAP_SIZE; i++ ) {
            char entry_name[22];
            snprintf ( entry_name, sizeof ( entry_name ), "dbg_memext_hex_bank_%X", i );
            gtk_entry_set_text ( ui_get_entry ( entry_name ), "" );
            gtk_widget_set_visible ( ui_get_widget ( entry_name ), FALSE );
            snprintf ( entry_name, sizeof ( entry_name ), "dbg_memext_dec_bank_%X", i );
            gtk_widget_set_visible ( ui_get_widget ( entry_name ), FALSE );
            ui_dbg_memext_update_memtype ( i );
        };

        ui_dbg_memext_check_ok_sensitivity ( );
        return;
    };

    gtk_widget_set_visible ( ui_get_widget ( "dbg_memext_type_label" ), TRUE );
    gtk_widget_set_visible ( ui_get_widget ( "dbg_memext_type_colon_label" ), TRUE );

    gtk_widget_set_visible ( ui_get_widget ( "dbg_memext_bank_size_label" ), TRUE );
    gtk_widget_set_visible ( ui_get_widget ( "dbg_memext_bank_size_colon_label" ), TRUE );
    gtk_widget_set_visible ( ui_get_widget ( "dbg_memext_bank_size_value_label" ), TRUE );

    gtk_widget_set_visible ( ui_get_widget ( "dbg_memext_ram_banks_label" ), TRUE );
    gtk_widget_set_visible ( ui_get_widget ( "dbg_memext_ram_banks_colon_label" ), TRUE );
    gtk_widget_set_visible ( ui_get_widget ( "dbg_memext_ram_banks_value_label" ), TRUE );

    if ( g_ui_dbg_memext_type == MEMEXT_TYPE_PEHU ) {
        gtk_label_set_text ( ui_get_label ( "dbg_memext_type_value_label" ), "Model 'Peroutka&Hucik'" );
        gtk_label_set_text ( ui_get_label ( "dbg_memext_bank_size_value_label" ), "0x2000" );
        gtk_widget_set_visible ( ui_get_widget ( "dbg_memext_flash_banks_label" ), FALSE );
        gtk_widget_set_visible ( ui_get_widget ( "dbg_memext_flash_banks_colon_label" ), FALSE );
        gtk_widget_set_visible ( ui_get_widget ( "dbg_memext_flash_banks_value_label" ), FALSE );

        int i;
        for ( i = 0; i < MEMEXT_RAW_MAP_SIZE; i++ ) {
            char entry_name[22];
            snprintf ( entry_name, sizeof ( entry_name ), "dbg_memext_hex_bank_%X", i );
            gboolean sensitive;
            if ( i & 1 ) {
                sensitive = FALSE;
                gtk_entry_set_text ( ui_get_entry ( entry_name ), "" );
            } else {
                sensitive = TRUE;
                char buff[3];
                snprintf ( buff, sizeof ( buff ), "%02X", ( g_memext.map[i] / 2 ) );
                gtk_entry_set_text ( ui_get_entry ( entry_name ), buff );
            };
            gtk_widget_set_visible ( ui_get_widget ( entry_name ), sensitive );
            snprintf ( entry_name, sizeof ( entry_name ), "dbg_memext_dec_bank_%X", i );
            gtk_widget_set_visible ( ui_get_widget ( entry_name ), sensitive );
            ui_dbg_memext_update_memtype ( i );
        };

    } else if ( g_ui_dbg_memext_type == MEMEXT_TYPE_LUFTNER ) {
        gtk_label_set_text ( ui_get_label ( "dbg_memext_type_value_label" ), "Model 'Luftner'" );
        gtk_label_set_text ( ui_get_label ( "dbg_memext_bank_size_value_label" ), "0x1000" );
        gtk_widget_set_visible ( ui_get_widget ( "dbg_memext_flash_banks_label" ), TRUE );
        gtk_widget_set_visible ( ui_get_widget ( "dbg_memext_flash_banks_colon_label" ), TRUE );
        gtk_widget_set_visible ( ui_get_widget ( "dbg_memext_flash_banks_value_label" ), TRUE );

        int i;
        for ( i = 0; i < MEMEXT_RAW_MAP_SIZE; i++ ) {
            char entry_name[22];
            snprintf ( entry_name, sizeof ( entry_name ), "dbg_memext_hex_bank_%X", i );
            char buff[3];
            snprintf ( buff, sizeof ( buff ), "%02X", g_memext.map[i] );
            gtk_entry_set_text ( ui_get_entry ( entry_name ), buff );
            gtk_widget_set_visible ( ui_get_widget ( entry_name ), TRUE );
            snprintf ( entry_name, sizeof ( entry_name ), "dbg_memext_dec_bank_%X", i );
            gtk_widget_set_visible ( ui_get_widget ( entry_name ), TRUE );
            ui_dbg_memext_update_memtype ( i );
        };

    } else {
        fprintf ( stderr, "%s():%d - Unknown memext type\n", __func__, __LINE__ );
    };

    ui_dbg_memext_check_ok_sensitivity ( );
    return;
}


void ui_dbg_memext_show ( void ) {
    GtkWidget *window = ui_get_widget ( "dbg_memext_window" );
    if ( gtk_widget_is_visible ( window ) ) {
        gtk_widget_hide ( window );
        gtk_widget_show ( window );
        return;
    };

    g_ui_dbg_memext_state = 0;
    g_ui_dbg_memext_type = -1;
    g_ui_dbg_memext_lock = FALSE;

    ui_dbg_memext_refresh ( );
    gtk_widget_show ( window );
}


G_MODULE_EXPORT void on_dbg_memext_close_button_clicked ( GtkButton *button, gpointer user_data ) {
    gtk_widget_hide ( ui_get_widget ( "dbg_memext_window" ) );
}


G_MODULE_EXPORT void on_dbg_memext_refresh_button_clicked ( GtkButton *button, gpointer user_data ) {
    ui_dbg_memext_refresh ( );
}


static int ui_dbg_memext_save_settings ( void ) {
    int memest_state = ( TEST_MEMEXT_CONNECTED_PEHU ) | ( TEST_MEMEXT_CONNECTED_LUFTNER << 1 );
    if ( g_ui_dbg_memext_state != memest_state ) {
        ui_show_warning ( "MemExt device connection changed.\nPlease click on the refresh button first.\n" );
        return EXIT_FAILURE;
    };
    int i;
    int step = ( g_ui_dbg_memext_type == MEMEXT_TYPE_PEHU ) ? 2 : 1;
    for ( i = 0; i < MEMEXT_RAW_MAP_SIZE; i += step ) {
        char entry_name[22];
        snprintf ( entry_name, sizeof ( entry_name ), "dbg_memext_hex_bank_%X", i );
        uint8_t value = debuger_hextext_to_uint32 ( gtk_entry_get_text ( ui_get_entry ( entry_name ) ) );
        memext_map_pwrite ( i, value );
    };
    return EXIT_SUCCESS;
}


G_MODULE_EXPORT void on_dbg_memext_apply_button_clicked ( GtkButton *button, gpointer user_data ) {
    ui_dbg_memext_save_settings ( );
}


G_MODULE_EXPORT void on_dbg_memext_ok_button_clicked ( GtkButton *button, gpointer user_data ) {
    if ( EXIT_SUCCESS != ui_dbg_memext_save_settings ( ) ) return;
    gtk_widget_hide ( ui_get_widget ( "dbg_memext_window" ) );
}

#endif
