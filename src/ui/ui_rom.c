/* 
 * File:   ui_rom.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 22. února 2016, 20:07
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

#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>

#include "ui_main.h"
#include "ui_utils.h"
#include "ui_file_chooser.h"
#include "ui_rom.h"

#include "memory/rom.h"
#include "mz800.h"


void ui_rom_menu_update ( void ) {

    LOCK_UICALLBACKS ( );


    gboolean devel_visible = ( g_mz800.development_mode == DEVELMODE_YES ) ? TRUE : FALSE;

    gtk_widget_set_visible ( ui_get_widget ( "menuitem_rom_jss103" ), devel_visible );
    gtk_widget_set_visible ( ui_get_widget ( "menuitem_rom_jss105c" ), devel_visible );
    gtk_widget_set_visible ( ui_get_widget ( "separatormenuitem_rom_devel1" ), devel_visible );


    gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_rom_standard" ), FALSE );
    gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_rom_jss103" ), FALSE );
    gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_rom_jss105c" ), FALSE );
    gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_rom_jss106a" ), FALSE );
    gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_rom_jss108c" ), FALSE );
    gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_rom_willy_en" ), FALSE );
    gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_rom_willy_ge" ), FALSE );
    gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_rom_willy_jap" ), FALSE );
    gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_rom_user_defined" ), FALSE );

    //gtk_widget_set_visible ( ui_get_widget ( "separatormenuitem_rom_user_defined" ), FALSE );
    //gtk_widget_set_visible ( ui_get_widget ( "menuitem_rom_user_defined" ), FALSE );
    //gtk_widget_set_visible ( ui_get_widget ( "menuitem_rom_user_settings" ), FALSE );


    switch ( g_rom.type ) {

        case ROMTYPE_STANDARD:
            gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_rom_standard" ), TRUE );
            break;

        case ROMTYPE_JSS103:
            gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_rom_jss103" ), TRUE );
            break;

        case ROMTYPE_JSS105C:
            gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_rom_jss105c" ), TRUE );
            break;

        case ROMTYPE_JSS106A:
            gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_rom_jss106a" ), TRUE );
            break;

        case ROMTYPE_JSS108C:
            gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_rom_jss108c" ), TRUE );
            break;

        case ROMTYPE_WILLY_EN:
            gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_rom_willy_en" ), TRUE );
            break;

        case ROMTYPE_WILLY_GE:
            gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_rom_willy_ge" ), TRUE );
            break;

        case ROMTYPE_WILLY_JAP:
            gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_rom_willy_jap" ), TRUE );
            break;

        case ROMTYPE_USER_DEFINED:
            gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_rom_user_defined" ), TRUE );
            break;

        default:
            gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_rom_standard" ), TRUE );
            break;
    };

    UNLOCK_UICALLBACKS ( );
}


G_MODULE_EXPORT void on_rom_changed ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

    if ( gtk_check_menu_item_get_active ( ui_get_check_menu_item ( "menuitem_rom_standard" ) ) ) {
        rom_reinstall ( ROMTYPE_STANDARD );
    } else if ( gtk_check_menu_item_get_active ( ui_get_check_menu_item ( "menuitem_rom_jss103" ) ) ) {
        rom_reinstall ( ROMTYPE_JSS103 );
    } else if ( gtk_check_menu_item_get_active ( ui_get_check_menu_item ( "menuitem_rom_jss105c" ) ) ) {
        rom_reinstall ( ROMTYPE_JSS105C );
    } else if ( gtk_check_menu_item_get_active ( ui_get_check_menu_item ( "menuitem_rom_jss106a" ) ) ) {
        rom_reinstall ( ROMTYPE_JSS106A );
    } else if ( gtk_check_menu_item_get_active ( ui_get_check_menu_item ( "menuitem_rom_jss108c" ) ) ) {
        rom_reinstall ( ROMTYPE_JSS108C );
    } else if ( gtk_check_menu_item_get_active ( ui_get_check_menu_item ( "menuitem_rom_willy_en" ) ) ) {
        rom_reinstall ( ROMTYPE_WILLY_EN );
    } else if ( gtk_check_menu_item_get_active ( ui_get_check_menu_item ( "menuitem_rom_willy_ge" ) ) ) {
        rom_reinstall ( ROMTYPE_WILLY_GE );
    } else if ( gtk_check_menu_item_get_active ( ui_get_check_menu_item ( "menuitem_rom_willy_jap" ) ) ) {
        rom_reinstall ( ROMTYPE_WILLY_JAP );
    } else if ( gtk_check_menu_item_get_active ( ui_get_check_menu_item ( "menuitem_rom_user_defined" ) ) ) {
        rom_reinstall ( ROMTYPE_USER_DEFINED );
    };
}


/*
 * 
 * 
 * 
 */

static char* ui_rom_get_filepath ( char *name ) {
    char *filepath = gtk_file_chooser_get_filename ( GTK_FILE_CHOOSER ( ui_get_widget ( name ) ) );
    if ( filepath ) {
        if ( ( strlen ( filepath ) == 0 ) || ( g_file_test ( filepath, ( G_FILE_TEST_IS_DIR ) ) || ( !g_file_test ( filepath, ( G_FILE_TEST_EXISTS ) ) ) ) ) {
            g_free ( filepath );
            filepath = NULL;
        };
    };
    return filepath;
}


static void ui_rom_update_ok_button_sensitivity ( ) {
    en_ROM_BOOL rom_enabled = ( gtk_toggle_button_get_active ( ui_get_toggle ( "checkbutton_rom_settings_enable" ) ) ) ? ROM_BOOL_YES : ROM_BOOL_NO;
    en_ROM_BOOL allinone = ( gtk_toggle_button_get_active ( ui_get_toggle ( "radiobutton_rom_settings_allinone" ) ) ) ? ROM_BOOL_YES : ROM_BOOL_NO;
    en_ROM_CMTHACK cmthack = gtk_combo_box_get_active ( ui_get_combo_box ( "comboboxtext_rom_settings_cmthack" ) );
    en_ROM_BOOL cmthack_allinone = ( gtk_toggle_button_get_active ( ui_get_toggle ( "radiobutton_rom_settings_cmthack_allinone" ) ) ) ? ROM_BOOL_YES : ROM_BOOL_NO;

    char *allinone_fp = ui_rom_get_filepath ( "filechooserbutton_rom_allinone" );
    char *mz700_fp = ui_rom_get_filepath ( "filechooserbutton_rom_mz700" );
    char *cgrom_fp = ui_rom_get_filepath ( "filechooserbutton_rom_cgrom" );
    char *mz800_fp = ui_rom_get_filepath ( "filechooserbutton_rom_mz800" );

    char *cmthack_allinone_fp = ui_rom_get_filepath ( "filechooserbutton_rom_cmthack_allinone" );
    char *cmthack_mz700_fp = ui_rom_get_filepath ( "filechooserbutton_rom_cmthack_mz700" );
    char *cmthack_cgrom_fp = ui_rom_get_filepath ( "filechooserbutton_rom_cmthack_cgrom" );
    char *cmthack_mz800_fp = ui_rom_get_filepath ( "filechooserbutton_rom_cmthack_mz800" );

    gboolean sensitive = TRUE;

    if ( rom_enabled ) {
        if ( ( allinone ) && ( !allinone_fp ) ) {
            sensitive = FALSE;
        } else if ( ( !allinone ) && ( !( ( mz700_fp ) && ( cgrom_fp ) && ( mz800_fp ) ) ) ) {
            sensitive = FALSE;
        };

        if ( cmthack == ROM_CMTHACK_CUSTOM ) {
            if ( ( cmthack_allinone ) && ( !cmthack_allinone_fp ) ) {
                sensitive = FALSE;
            } else if ( ( !cmthack_allinone ) && ( !( ( cmthack_mz700_fp ) && ( cmthack_cgrom_fp ) && ( cmthack_mz800_fp ) ) ) ) {
                sensitive = FALSE;
            };
        };
    };

    gtk_widget_set_sensitive ( ui_get_widget ( "button_rom_user_settings_ok" ), sensitive );
}


static void ui_rom_settings_changed ( ) {

    gboolean rom_enabled = gtk_toggle_button_get_active ( ui_get_toggle ( "checkbutton_rom_settings_enable" ) );
    gboolean rom_allinone = FALSE;
    gboolean rom_separated = FALSE;
    en_ROM_CMTHACK cmthack = ROM_CMTHACK_DISABLED;
    gboolean rom_cmthack_allinone = FALSE;
    gboolean rom_cmthack_separated = FALSE;

    if ( rom_enabled ) {
        rom_allinone = gtk_toggle_button_get_active ( ui_get_toggle ( "radiobutton_rom_settings_allinone" ) );
        rom_separated = !rom_allinone;
        cmthack = gtk_combo_box_get_active ( ui_get_combo_box ( "comboboxtext_rom_settings_cmthack" ) );
    };

    if ( cmthack == ROM_CMTHACK_CUSTOM ) {
        rom_cmthack_allinone = gtk_toggle_button_get_active ( ui_get_toggle ( "radiobutton_rom_settings_cmthack_allinone" ) );
        rom_cmthack_separated = !rom_cmthack_allinone;
    };

    gtk_widget_set_sensitive ( ui_get_widget ( "radiobutton_rom_settings_allinone" ), rom_enabled );
    gtk_widget_set_sensitive ( ui_get_widget ( "radiobutton_rom_settings_separate_files" ), rom_enabled );

    gtk_widget_set_sensitive ( ui_get_widget ( "filechooserbutton_rom_allinone" ), rom_allinone );
    gtk_widget_set_sensitive ( ui_get_widget ( "rom_mz700_label" ), rom_separated );
    gtk_widget_set_sensitive ( ui_get_widget ( "filechooserbutton_rom_mz700" ), rom_separated );
    gtk_widget_set_sensitive ( ui_get_widget ( "rom_cgrom_label" ), rom_separated );
    gtk_widget_set_sensitive ( ui_get_widget ( "filechooserbutton_rom_cgrom" ), rom_separated );
    gtk_widget_set_sensitive ( ui_get_widget ( "rom_mz800_label" ), rom_separated );
    gtk_widget_set_sensitive ( ui_get_widget ( "filechooserbutton_rom_mz800" ), rom_separated );

    gtk_widget_set_sensitive ( ui_get_widget ( "comboboxtext_rom_settings_cmthack" ), rom_enabled );

    gtk_widget_set_sensitive ( ui_get_widget ( "radiobutton_rom_settings_cmthack_allinone" ), ( cmthack == ROM_CMTHACK_CUSTOM ) ? TRUE : FALSE );
    gtk_widget_set_sensitive ( ui_get_widget ( "radiobutton_rom_settings_cmthack_separate_files" ), ( cmthack == ROM_CMTHACK_CUSTOM ) ? TRUE : FALSE );

    gtk_widget_set_sensitive ( ui_get_widget ( "filechooserbutton_rom_cmthack_allinone" ), rom_cmthack_allinone );
    gtk_widget_set_sensitive ( ui_get_widget ( "rom_cmthack_mz700_label" ), rom_cmthack_separated );
    gtk_widget_set_sensitive ( ui_get_widget ( "filechooserbutton_rom_cmthack_mz700" ), rom_cmthack_separated );
    gtk_widget_set_sensitive ( ui_get_widget ( "rom_cmthack_cgrom_label" ), rom_cmthack_separated );
    gtk_widget_set_sensitive ( ui_get_widget ( "filechooserbutton_rom_cmthack_cgrom" ), rom_cmthack_separated );
    gtk_widget_set_sensitive ( ui_get_widget ( "rom_cmthack_mz800_label" ), rom_cmthack_separated );
    gtk_widget_set_sensitive ( ui_get_widget ( "filechooserbutton_rom_cmthack_mz800" ), rom_cmthack_separated );

    ui_rom_update_ok_button_sensitivity ( );
}


static void ui_rom_settings_update_fcb_dirpath ( char *name ) {
    char *filepath = gtk_file_chooser_get_filename ( GTK_FILE_CHOOSER ( ui_get_widget ( name ) ) );
    if ( ( !filepath ) || ( strlen ( filepath ) == 0 ) || ( g_file_test ( filepath, ( G_FILE_TEST_IS_DIR ) ) || ( !g_file_test ( filepath, ( G_FILE_TEST_EXISTS ) ) ) ) ) {
        const char *dirpath = ui_filechooser_get_last_generic_dir ( );
        int len = strlen ( dirpath ) + 2 + 1;
        char *filename = ui_utils_mem_alloc0 ( len );
        snprintf ( filename, len, "%s/.", dirpath );
        gtk_file_chooser_set_filename ( GTK_FILE_CHOOSER ( ui_get_widget ( name ) ), filename );
        gtk_file_chooser_set_current_folder ( GTK_FILE_CHOOSER ( ui_get_widget ( name ) ), dirpath );
        g_free ( filename );
    };
    if ( filepath ) g_free ( filepath );
}


static void ui_rom_settings_update_all_fcb_dirpath ( void ) {
    ui_rom_settings_update_fcb_dirpath ( "filechooserbutton_rom_allinone" );
    ui_rom_settings_update_fcb_dirpath ( "filechooserbutton_rom_mz700" );
    ui_rom_settings_update_fcb_dirpath ( "filechooserbutton_rom_cgrom" );
    ui_rom_settings_update_fcb_dirpath ( "filechooserbutton_rom_mz800" );
    ui_rom_settings_update_fcb_dirpath ( "filechooserbutton_rom_cmthack_allinone" );
    ui_rom_settings_update_fcb_dirpath ( "filechooserbutton_rom_cmthack_mz700" );
    ui_rom_settings_update_fcb_dirpath ( "filechooserbutton_rom_cmthack_cgrom" );
    ui_rom_settings_update_fcb_dirpath ( "filechooserbutton_rom_cmthack_mz800" );
}


void ui_rom_settings_open_window ( void ) {
    gtk_file_chooser_set_filename ( GTK_FILE_CHOOSER ( ui_get_widget ( "filechooserbutton_rom_allinone" ) ), g_rom.user_defined_allinone_fp );
    gtk_file_chooser_set_filename ( GTK_FILE_CHOOSER ( ui_get_widget ( "filechooserbutton_rom_mz700" ) ), g_rom.user_defined_mz700_fp );
    gtk_file_chooser_set_filename ( GTK_FILE_CHOOSER ( ui_get_widget ( "filechooserbutton_rom_cgrom" ) ), g_rom.user_defined_cgrom_fp );
    gtk_file_chooser_set_filename ( GTK_FILE_CHOOSER ( ui_get_widget ( "filechooserbutton_rom_mz800" ) ), g_rom.user_defined_mz800_fp );
    gtk_file_chooser_set_filename ( GTK_FILE_CHOOSER ( ui_get_widget ( "filechooserbutton_rom_cmthack_allinone" ) ), g_rom.user_defined_cmthack_allinone_fp );
    gtk_file_chooser_set_filename ( GTK_FILE_CHOOSER ( ui_get_widget ( "filechooserbutton_rom_cmthack_mz700" ) ), g_rom.user_defined_cmthack_mz700_fp );
    gtk_file_chooser_set_filename ( GTK_FILE_CHOOSER ( ui_get_widget ( "filechooserbutton_rom_cmthack_cgrom" ) ), g_rom.user_defined_cmthack_cgrom_fp );
    gtk_file_chooser_set_filename ( GTK_FILE_CHOOSER ( ui_get_widget ( "filechooserbutton_rom_cmthack_mz800" ) ), g_rom.user_defined_cmthack_mz800_fp );
    ui_rom_settings_update_all_fcb_dirpath ( );
    LOCK_UICALLBACKS ( );
    gtk_toggle_button_set_active ( ui_get_toggle ( "checkbutton_rom_settings_enable" ), ( g_rom.type == ROMTYPE_USER_DEFINED ) );
    gtk_combo_box_set_active ( ui_get_combo_box ( "comboboxtext_rom_settings_cmthack" ), g_rom.user_defined_cmthack_type );
    gtk_toggle_button_set_active ( ui_get_toggle ( "radiobutton_rom_settings_allinone" ), g_rom.user_defined_allinone & 1 );
    gtk_toggle_button_set_active ( ui_get_toggle ( "radiobutton_rom_settings_separate_files" ), ( !g_rom.user_defined_allinone )&1 );
    gtk_toggle_button_set_active ( ui_get_toggle ( "radiobutton_rom_settings_cmthack_allinone" ), g_rom.user_defined_cmthack_allinone & 1 );
    gtk_toggle_button_set_active ( ui_get_toggle ( "radiobutton_rom_settings_cmthack_separate_files" ), ( !g_rom.user_defined_cmthack_allinone )&1 );
    UNLOCK_UICALLBACKS ( );
    ui_rom_settings_changed ( );
    GtkWidget *window = ui_get_widget ( "dialog_rom_user_settings" );
    gtk_widget_show ( window );
}


G_MODULE_EXPORT void on_menuitem_rom_user_settings_activate ( GtkMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

    ui_rom_settings_open_window ( );
}


static void ui_rom_settings_hide_window ( void ) {
    GtkWidget *window = ui_get_widget ( "dialog_rom_user_settings" );
    //ui_main_win_get_pos ( GTK_WINDOW ( window ), &g_uirom.main_pos );
    gtk_widget_hide ( window );
}


G_MODULE_EXPORT gboolean on_dialog_rom_user_settings_delete_event ( GtkWidget *widget, GdkEvent *event, gpointer user_data ) {
    ui_rom_settings_hide_window ( );
    return TRUE;
}


G_MODULE_EXPORT void on_rom_settings_changed ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
    if ( TEST_UICALLBACKS_LOCKED ) return;
    ui_rom_settings_changed ( );
}


G_MODULE_EXPORT void on_button_rom_user_settings_close_clicked ( GtkButton *button, gpointer user_data ) {
    ui_rom_settings_hide_window ( );
}


G_MODULE_EXPORT void on_button_rom_user_settings_ok_clicked ( GtkButton *button, gpointer user_data ) {

    en_ROM_BOOL rom_enabled = ( gtk_toggle_button_get_active ( ui_get_toggle ( "checkbutton_rom_settings_enable" ) ) ) ? ROM_BOOL_YES : ROM_BOOL_NO;
    en_ROM_BOOL allinone = ( gtk_toggle_button_get_active ( ui_get_toggle ( "radiobutton_rom_settings_allinone" ) ) ) ? ROM_BOOL_YES : ROM_BOOL_NO;
    en_ROM_CMTHACK cmthack = gtk_combo_box_get_active ( ui_get_combo_box ( "comboboxtext_rom_settings_cmthack" ) );
    en_ROM_BOOL cmthack_allinone = ( gtk_toggle_button_get_active ( ui_get_toggle ( "radiobutton_rom_settings_cmthack_allinone" ) ) ) ? ROM_BOOL_YES : ROM_BOOL_NO;

    char *allinone_fp = ui_rom_get_filepath ( "filechooserbutton_rom_allinone" );
    char *mz700_fp = ui_rom_get_filepath ( "filechooserbutton_rom_mz700" );
    char *cgrom_fp = ui_rom_get_filepath ( "filechooserbutton_rom_cgrom" );
    char *mz800_fp = ui_rom_get_filepath ( "filechooserbutton_rom_mz800" );

    char *cmthack_allinone_fp = ui_rom_get_filepath ( "filechooserbutton_rom_cmthack_allinone" );
    char *cmthack_mz700_fp = ui_rom_get_filepath ( "filechooserbutton_rom_cmthack_mz700" );
    char *cmthack_cgrom_fp = ui_rom_get_filepath ( "filechooserbutton_rom_cmthack_cgrom" );
    char *cmthack_mz800_fp = ui_rom_get_filepath ( "filechooserbutton_rom_cmthack_mz800" );

    gboolean error = FALSE;

    if ( rom_enabled == ROM_BOOL_YES ) {
        st_ROM_AREA rom;
        st_ROM_AREA rom_cmthack;

        if ( EXIT_SUCCESS != rom_user_defined_rom_area_load ( &rom, allinone, allinone_fp, mz700_fp, cgrom_fp, mz800_fp ) ) {
            printf ( "Can't install user defined ROM!\n" );
            error = TRUE;
        } else if ( cmthack == ROM_CMTHACK_CUSTOM ) {
            if ( EXIT_SUCCESS != rom_user_defined_rom_area_load ( &rom_cmthack, cmthack_allinone, allinone_fp, cmthack_mz700_fp, cmthack_cgrom_fp, cmthack_mz800_fp ) ) {
                printf ( "Can't install user defined cmthack ROM!\n" );
                error = TRUE;
            };
        };

        if ( !error ) {
            g_rom.user_defined_rom_loaded = ROM_BOOL_YES;
            memcpy ( &g_rom.rom_user_defined, &rom, sizeof ( st_ROM_AREA ) );
            memcpy ( &g_rom.rom_user_defined_cmthack, &rom_cmthack, sizeof ( st_ROM_AREA ) );
        };
    } else {
        g_rom.user_defined_rom_loaded = ROM_BOOL_NO;
    };

    if ( !error ) {
        g_rom.user_defined_allinone = allinone;
        g_rom.user_defined_cmthack_type = cmthack;
        g_rom.user_defined_cmthack_allinone = cmthack_allinone;
        rom_set_user_defined_filepath ( &g_rom.user_defined_allinone_fp, allinone_fp );
        rom_set_user_defined_filepath ( &g_rom.user_defined_mz700_fp, mz700_fp );
        rom_set_user_defined_filepath ( &g_rom.user_defined_cgrom_fp, cgrom_fp );
        rom_set_user_defined_filepath ( &g_rom.user_defined_mz800_fp, mz800_fp );
        rom_set_user_defined_filepath ( &g_rom.user_defined_cmthack_allinone_fp, cmthack_allinone_fp );
        rom_set_user_defined_filepath ( &g_rom.user_defined_cmthack_mz700_fp, cmthack_mz700_fp );
        rom_set_user_defined_filepath ( &g_rom.user_defined_cmthack_cgrom_fp, cmthack_cgrom_fp );
        rom_set_user_defined_filepath ( &g_rom.user_defined_cmthack_mz800_fp, cmthack_mz800_fp );

        if ( rom_enabled == ROM_BOOL_YES ) {
            rom_reinstall ( ROMTYPE_USER_DEFINED );
        } else if ( g_rom.type == ROMTYPE_USER_DEFINED ) {
            rom_reinstall ( ROMTYPE_STANDARD );
        };
    };

    if ( allinone_fp ) g_free ( allinone_fp );
    if ( mz700_fp ) g_free ( mz700_fp );
    if ( cgrom_fp ) g_free ( cgrom_fp );
    if ( mz800_fp ) g_free ( mz800_fp );

    if ( cmthack_allinone_fp ) g_free ( cmthack_allinone_fp );
    if ( cmthack_mz700_fp ) g_free ( cmthack_mz700_fp );
    if ( cmthack_cgrom_fp ) g_free ( cmthack_cgrom_fp );
    if ( cmthack_mz800_fp ) g_free ( cmthack_mz800_fp );

    if ( !error ) ui_rom_settings_hide_window ( );
}


static void ui_rom_check_filepath ( GtkFileChooserButton *widget, char **cfgfilepath, uint32_t size ) {
    char *filename = gtk_file_chooser_get_filename ( GTK_FILE_CHOOSER ( widget ) );
    if ( ( !filename ) || ( strlen ( filename ) == 0 ) ) {
        gtk_file_chooser_set_filename ( GTK_FILE_CHOOSER ( widget ), "" );
        return;
    };
    if ( 0 == strcmp ( filename, *cfgfilepath ) ) return;

    char *dirpath = g_path_get_dirname ( filename );
    ui_filechooser_set_last_generic_dir ( dirpath );
    g_free ( dirpath );

    ui_rom_settings_update_all_fcb_dirpath ( );

    if ( EXIT_SUCCESS != rom_user_defined_check_filepath ( &filename, ROM_BOOL_NO ) ) {
        fprintf ( stderr, "%s():%d - File not found '%s'\n", __func__, __LINE__, filename );
        g_free ( filename );
        gtk_file_chooser_set_filename ( GTK_FILE_CHOOSER ( widget ), "" );
        return;
    };

    if ( EXIT_SUCCESS != rom_user_defined_check_size ( &filename, size, ROM_BOOL_NO ) ) {


        fprintf ( stderr, "%s():%d - File is too short (required %d B) '%s'\n", __func__, __LINE__, size, filename );
        g_free ( filename );
        gtk_file_chooser_set_filename ( GTK_FILE_CHOOSER ( widget ), "" );
        return;
    };

    gtk_file_chooser_set_filename ( GTK_FILE_CHOOSER ( widget ), filename );
    g_free ( filename );
}


static void ui_rom_check_filepath_and_sensitivity ( GtkFileChooserButton *widget, char **cfgfilepath, uint32_t size ) {
    ui_rom_check_filepath ( widget, cfgfilepath, size );
    ui_rom_update_ok_button_sensitivity ( );
}


G_MODULE_EXPORT void on_filechooserbutton_rom_allinone_file_set ( GtkFileChooserButton *widget, gpointer user_data ) {
    ui_rom_check_filepath_and_sensitivity ( widget, &g_rom.user_defined_allinone_fp, ROM_SIZE_TOTAL );
}


G_MODULE_EXPORT void on_filechooserbutton_rom_mz700_file_set ( GtkFileChooserButton *widget, gpointer user_data ) {
    ui_rom_check_filepath_and_sensitivity ( widget, &g_rom.user_defined_mz700_fp, ROM_SIZE_MZ700 );
}


G_MODULE_EXPORT void on_filechooserbutton_rom_cgrom_file_set ( GtkFileChooserButton *widget, gpointer user_data ) {
    ui_rom_check_filepath_and_sensitivity ( widget, &g_rom.user_defined_cgrom_fp, ROM_SIZE_CGROM );
}


G_MODULE_EXPORT void on_filechooserbutton_rom_mz800_file_set ( GtkFileChooserButton *widget, gpointer user_data ) {
    ui_rom_check_filepath_and_sensitivity ( widget, &g_rom.user_defined_mz800_fp, ROM_SIZE_MZ800 );
}


G_MODULE_EXPORT void on_filechooserbutton_rom_cmthack_allinone_file_set ( GtkFileChooserButton *widget, gpointer user_data ) {
    ui_rom_check_filepath_and_sensitivity ( widget, &g_rom.user_defined_cmthack_allinone_fp, ROM_SIZE_TOTAL );
}


G_MODULE_EXPORT void on_filechooserbutton_rom_cmthack_mz700_file_set ( GtkFileChooserButton *widget, gpointer user_data ) {
    ui_rom_check_filepath_and_sensitivity ( widget, &g_rom.user_defined_cmthack_mz700_fp, ROM_SIZE_MZ700 );
}


G_MODULE_EXPORT void on_filechooserbutton_rom_cmthack_cgrom_file_set ( GtkFileChooserButton *widget, gpointer user_data ) {
    ui_rom_check_filepath_and_sensitivity ( widget, &g_rom.user_defined_cmthack_cgrom_fp, ROM_SIZE_CGROM );
}


G_MODULE_EXPORT void on_filechooserbutton_rom_cmthack_mz800_file_set ( GtkFileChooserButton *widget, gpointer user_data ) {
    ui_rom_check_filepath_and_sensitivity ( widget, &g_rom.user_defined_cmthack_mz800_fp, ROM_SIZE_MZ800 );
}

