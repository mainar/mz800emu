/* 
 * File:   ui_ide8.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 14. července 2018, 22:51
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
#include <stdio.h>
#include <string.h>

#include "ide8/ide8.h"

#include "ui_main.h"
#include "ui_file_chooser.h"
#include "ui_utils.h"


void ui_ide8_update_menu ( void ) {
    LOCK_UICALLBACKS ( );
    gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_ide8_hdd0_connected" ), TEST_IDE8_MASTER_CONNECTED );
    gtk_widget_set_sensitive ( ui_get_widget ( "menuitem_ide8_hdd0_image" ), ( !TEST_IDE8_MASTER_CONNECTED ) );
    gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_ide8_hdd1_connected" ), TEST_IDE8_SLAVE_CONNECTED );
    gtk_widget_set_sensitive ( ui_get_widget ( "menuitem_ide8_hdd1_image" ), ( !TEST_IDE8_SLAVE_CONNECTED ) );
    UNLOCK_UICALLBACKS ( );
}


G_MODULE_EXPORT void on_menuitem_ide8_hdd0_connected_toggled ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

    ide8_drive_set_connected ( IDE8_DRIVE_MASTER, gtk_check_menu_item_get_active ( menuitem ) ? IDE8_STATE_CONNECTED : IDE8_STATE_DISCONNECTED );
    ui_ide8_update_menu ( );
}


G_MODULE_EXPORT void on_menuitem_ide8_hdd1_connected_toggled ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

    ide8_drive_set_connected ( IDE8_DRIVE_SLAVE, gtk_check_menu_item_get_active ( menuitem ) ? IDE8_STATE_CONNECTED : IDE8_STATE_DISCONNECTED );
    ui_ide8_update_menu ( );
}


static void ui_ide8_drive_open_image ( en_IDE8_DRIVE drive_id ) {
    const char title[] = "Select HDD image file or create a new";
    const char *dirpath = ui_filechooser_get_last_generic_dir ( );
    char *filepath = ui_file_chooser_open_file ( g_ide8.drive[drive_id].filepath, dirpath, title, NULL, FC_MODE_OPEN_OR_NEW, NULL );
    if ( !filepath ) return;
    // zmenu image povolujeme jen pri odpojenem disku, takze ted staci pouze nastavit drive->filepath
    int len = strlen ( filepath ) + 1;
    g_ide8.drive[drive_id].filepath = (char*) ui_utils_mem_realloc ( g_ide8.drive[drive_id].filepath, len );
    strncpy ( g_ide8.drive[drive_id].filepath, filepath, len );
    char *dp = g_path_get_dirname ( filepath );
    ui_filechooser_set_last_generic_dir ( dp );
    ui_utils_mem_free ( filepath );
    ui_utils_mem_free ( dp );
}


G_MODULE_EXPORT void on_menuitem_ide8_hdd0_image_activate ( GtkMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif
    ui_ide8_drive_open_image ( IDE8_DRIVE_MASTER );
}


G_MODULE_EXPORT void on_menuitem_ide8_hdd1_image_activate ( GtkMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif
    ui_ide8_drive_open_image ( IDE8_DRIVE_SLAVE );
}
