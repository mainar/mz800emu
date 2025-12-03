/* 
 * File:   ui_qdisk.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 11. února 2016, 21:31
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

#include <string.h>
#include <strings.h>
#include <glib.h>
#include <glib/gstdio.h>


#include "ui_main.h"
#include "ui_file_chooser.h"
#include "ui_qdisk.h"
#include "ui_utils.h"

#include "qdisk/qdisk.h"
#include "unicard/unicard.h"


void ui_qdisk_set_path ( char *path ) {
    if ( path [ 0 ] != 0x00 ) {
        if ( g_qdisk.type == QDISK_TYPE_IMAGE ) {
            gtk_menu_item_set_label ( ui_get_menu_item ( "menuitem_qdisk_drive" ), "_QD Drive (image)" );
        } else {
            gtk_menu_item_set_label ( ui_get_menu_item ( "menuitem_qdisk_drive" ), "_QD Drive (directory)" );
        };
        gtk_menu_item_set_label ( ui_get_menu_item ( "menuitem_qdisk_path" ), path );
        gtk_widget_set_sensitive ( ui_get_widget ( "menuitem_qdisk_umount" ), TRUE );
    } else {
        gtk_menu_item_set_label ( ui_get_menu_item ( "menuitem_qdisk_drive" ), "_QD Drive (Empty)" );
        gtk_menu_item_set_label ( ui_get_menu_item ( "menuitem_qdisk_path" ), "Empty" );
        gtk_widget_set_sensitive ( ui_get_widget ( "menuitem_qdisk_umount" ), FALSE );
    };
}


void ui_qdisk_menu_update ( void ) {

    LOCK_UICALLBACKS ( );
    if ( g_qdisk.connected == QDISK_DISCONNECTED ) {
        gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_qdisk_not_connected" ), TRUE );
        gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_qdisk_std" ), FALSE );
        gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_qdisk_virtual" ), FALSE );

        gtk_widget_set_sensitive ( ui_get_widget ( "menuitem_qdisk_drive" ), FALSE );
        gtk_widget_set_sensitive ( ui_get_widget ( "menuitem_qdisk_write_protected" ), FALSE );
        gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_qdisk_write_protected" ), FALSE );

        ui_qdisk_set_path ( "" );

    } else {
        gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_qdisk_not_connected" ), FALSE );
        if ( g_qdisk.type == QDISK_TYPE_IMAGE ) {
            gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_qdisk_std" ), TRUE );
            gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_qdisk_virtual" ), FALSE );
            gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_qdisk_unicard" ), FALSE );
        } else if ( g_qdisk.type == QDISK_TYPE_VIRTUAL ) {
            gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_qdisk_std" ), FALSE );
            gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_qdisk_virtual" ), TRUE );
            gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_qdisk_unicard" ), FALSE );
        } else {
            gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_qdisk_std" ), FALSE );
            gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_qdisk_virtual" ), FALSE );
            gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_qdisk_unicard" ), TRUE );
        }

        if ( g_qdisk.type == QDISK_TYPE_UNICARD ) {
            gtk_widget_set_sensitive ( ui_get_widget ( "menuitem_qdisk_drive" ), FALSE );
            gtk_widget_set_sensitive ( ui_get_widget ( "menuitem_qdisk_write_protected" ), FALSE );
        } else {
            gtk_widget_set_sensitive ( ui_get_widget ( "menuitem_qdisk_drive" ), TRUE );
            gtk_widget_set_sensitive ( ui_get_widget ( "menuitem_qdisk_write_protected" ), TRUE );

            if ( g_qdisk.type == QDISK_TYPE_IMAGE ) {
                gtk_menu_item_set_label ( ui_get_menu_item ( "menuitem_qdisk_mount" ), "_Mount Image..." );
            } else {
                gtk_menu_item_set_label ( ui_get_menu_item ( "menuitem_qdisk_mount" ), "_Mount Directory..." );
            };
        };

        if ( TEST_UNICARD_CONNECTED ) {
            gtk_widget_set_sensitive ( ui_get_widget ( "menuitem_qdisk_unicard" ), TRUE );
        } else {
            gtk_widget_set_sensitive ( ui_get_widget ( "menuitem_qdisk_unicard" ), FALSE );
        };

        if ( g_qdisk.status & QDSTS_IMG_READONLY ) {
            gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_qdisk_write_protected" ), TRUE );
        } else {
            gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_qdisk_write_protected" ), FALSE );
        };

    };

    UNLOCK_UICALLBACKS ( );
}


G_MODULE_EXPORT void on_qdisk_changed ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

    qdisk_close ( );

    if ( gtk_check_menu_item_get_active ( ui_get_check_menu_item ( "menuitem_qdisk_not_connected" ) ) ) {
        g_qdisk.connected = QDISK_DISCONNECTED;
    } else {

        g_qdisk.connected = QDISK_CONNECTED;


        if ( gtk_check_menu_item_get_active ( ui_get_check_menu_item ( "menuitem_qdisk_std" ) ) ) {
            qdisk_deactivate_unicard_boot_loader ( );
            g_qdisk.type = QDISK_TYPE_IMAGE;
        } else if ( gtk_check_menu_item_get_active ( ui_get_check_menu_item ( "menuitem_qdisk_virtual" ) ) ) {
            qdisk_deactivate_unicard_boot_loader ( );
            g_qdisk.type = QDISK_TYPE_VIRTUAL;
        } else {
            qdisk_activate_unicard_boot_loader ( );
            g_qdisk.type = QDISK_TYPE_UNICARD;
        };

        qdisk_open ( );

    };
    ui_qdisk_menu_update ( );
}


G_MODULE_EXPORT void on_menuitem_qdisk_mount ( GtkMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

    qdisk_mount ( );
}


G_MODULE_EXPORT void on_menuitem_qdisk_umount ( GtkMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

    qdisk_umount ( );
}


G_MODULE_EXPORT void on_menuitem_qdisk_write_protected ( GtkMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

    qdisk_set_write_protected ( gtk_check_menu_item_get_active ( ui_get_check_menu_item ( "menuitem_qdisk_write_protected" ) ) );
}


void ui_qdisk_create_show_window ( void ) {

    GtkWidget *window = ui_get_widget ( "window_qdisk_new" );
    if ( gtk_widget_get_visible ( window ) ) return;

    GtkEntry *entry = ui_get_entry ( "entry_qdisk_new_filename" );
    gtk_entry_set_text ( entry, UI_QDISK_NEW_IMAGE_NAME );
    gtk_widget_grab_focus ( GTK_WIDGET ( entry ) );
    gint len = strlen ( UI_QDISK_NEW_IMAGE_NAME );
    gtk_editable_set_position ( GTK_EDITABLE ( entry ), len );
    gtk_editable_select_region ( GTK_EDITABLE ( entry ), 0, -1 );

    GtkWidget *fcdialog = ui_get_widget ( "filechooserbutton_qdisk_new" );
    gtk_file_chooser_set_current_folder ( GTK_FILE_CHOOSER ( fcdialog ), ui_filechooser_get_last_mzq_dir ( ) );

    gtk_widget_show ( window );
}


G_MODULE_EXPORT void on_menuitem_qdisk_create_new ( GtkMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

    ui_qdisk_create_show_window ( );
}


G_MODULE_EXPORT gboolean on_window_qdisk_new_delete_event ( GtkWidget *widget, GdkEvent *event, gpointer user_data ) {
    gtk_widget_hide ( widget );
    return TRUE;
}


G_MODULE_EXPORT gboolean on_window_qdisk_new_key_press_event ( GtkWidget *widget, GdkEventKey *event, gpointer user_data ) {
    if ( event->keyval == GDK_KEY_Escape ) {
        GtkWidget *window = ui_get_widget ( "window_qdisk_new" );
        gtk_widget_hide ( window );
        return TRUE;
    } else if ( ( event->keyval == GDK_KEY_Return ) || ( event->keyval == GDK_KEY_KP_Enter ) ) {
        gtk_button_clicked ( GTK_BUTTON ( ui_get_widget ( "button_qdisk_new_ok" ) ) );

    };
    return FALSE;
}


G_MODULE_EXPORT void on_button_qdisk_new_close_clicked ( GtkButton *button, gpointer user_data ) {
    GtkWidget *window = ui_get_widget ( "window_qdisk_new" );
    gtk_widget_hide ( window );
}


G_MODULE_EXPORT void on_button_qdisk_new_ok_clicked ( GtkButton *button, gpointer user_data ) {
    GtkWidget *window = ui_get_widget ( "window_qdisk_new" );

    GtkEntry *entry = ui_get_entry ( "entry_qdisk_new_filename" );

    gchar *filename = (gchar*) gtk_entry_get_text ( entry );

    int filename_len = strlen ( filename );
    int filename_mzq_len = filename_len;

    int fl_add_surfix = 1;

    if ( filename_len > 4 ) {
        if ( 0 == strcasecmp ( &filename [ filename_len - 4 ], ".mzq" ) ) {
            fl_add_surfix = 0;
        } else {
            filename_mzq_len += 4;
        };
    } else {
        filename_mzq_len += 4;
    }

    char *fname_mzq = g_malloc ( filename_mzq_len );

    strncpy ( fname_mzq, filename, filename_mzq_len );

    if ( fl_add_surfix ) {
        strncat ( fname_mzq, ".mzq", 5 );
    };

    GtkWidget *fcdialog = ui_get_widget ( "filechooserbutton_qdisk_new" );

    gchar *path = gtk_file_chooser_get_filename ( GTK_FILE_CHOOSER ( fcdialog ) );
    if ( path == NULL ) {
        path = g_malloc ( 3 );
        memcpy ( path, "./", 3 );
    };
    ui_filechooser_set_last_mzq_dir ( path );

    gchar *full_name_mzq = g_build_filename ( path, fname_mzq, (gchar*) NULL );

    if ( ui_utils_file_access ( full_name_mzq, F_OK ) == -1 ) {
        qdisk_create_image ( full_name_mzq );
        gtk_widget_hide ( window );
    } else {
        ui_show_error ( "Can't create QD image '%s' - file already exists!", full_name_mzq );
        GtkEntry *entry = ui_get_entry ( "entry_qdisk_new_filename" );
        //gtk_entry_set_text ( entry, fname_mzq );
        gtk_widget_grab_focus ( GTK_WIDGET ( entry ) );
        gtk_editable_set_position ( GTK_EDITABLE ( entry ), filename_len );
        gtk_editable_select_region ( GTK_EDITABLE ( entry ), 0, filename_len );
    };

    g_free ( path );
    g_free ( fname_mzq );
    g_free ( full_name_mzq );
}
