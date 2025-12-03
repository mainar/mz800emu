/* 
 * File:   ui_unicard.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 24. června 2018, 20:51
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

#include "unicard/unicard.h"

#include "ui_main.h"
#include "ui_file_chooser.h"
#include "ui_utils.h"


G_MODULE_EXPORT void on_menuitem_unicard_connected_toggled ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

    unicard_set_connected ( gtk_check_menu_item_get_active ( menuitem ) ? UNICARD_CONNECTION_CONNECTED : UNICARD_CONNECTION_DISCONNECTED );
}


void ui_unicard_update_menu ( void ) {
    LOCK_UICALLBACKS ( );
    gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_unicard_connected" ), TEST_UNICARD_CONNECTED );
    gtk_widget_set_sensitive ( ui_get_widget ( "menuitem_unicard_settings" ), ( !TEST_UNICARD_CONNECTED ) );
    UNLOCK_UICALLBACKS ( );
}


G_MODULE_EXPORT void on_menuitem_unicard_settings_activate ( GtkMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

    char window_title[] = "Select directory for Unicard SD root";
    char *dirpath = ui_file_chooser_open_dir ( unicard_get_sd_root_dirpath ( ), window_title );
    if ( !dirpath ) return;
    unicard_set_sd_root_dirpath ( dirpath );
    ui_utils_mem_free ( dirpath );
}
