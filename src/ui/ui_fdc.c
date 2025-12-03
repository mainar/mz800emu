/* 
 * File:   ui_fdc.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 9. srpna 2015, 9:36
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

#include "ui_main.h"
#include "ui_fdc.h"
//#include "dsk_tool/ui_dsk_tool.h"

#include "fdc/fdc.h"



G_MODULE_EXPORT void on_fdc_changed ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

    if ( gtk_check_menu_item_get_active ( ui_get_check_menu_item ( "menuitem_fdc_wd279x" ) ) ) {
        g_fdc.connected = FDC_CONNECTED;
    } else {
        g_fdc.connected = FDC_DISCONNECTED;
    };
    ui_fdc_menu_update ( );
}


G_MODULE_EXPORT void on_fdc_fdd0_mount ( GtkMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif
    fdc_mount ( 0 );
}


G_MODULE_EXPORT void on_fdc_fdd1_mount ( GtkMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif
    fdc_mount ( 1 );
}


G_MODULE_EXPORT void on_fdc_fdd2_mount ( GtkMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif
    fdc_mount ( 2 );
}


G_MODULE_EXPORT void on_fdc_fdd3_mount ( GtkMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif
    fdc_mount ( 3 );
}


G_MODULE_EXPORT void on_fdc_fdd0_umount ( GtkMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif
    fdc_umount ( 0 );
}


G_MODULE_EXPORT void on_fdc_fdd1_umount ( GtkMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif
    fdc_umount ( 1 );
}


G_MODULE_EXPORT void on_fdc_fdd2_umount ( GtkMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif
    fdc_umount ( 2 );
}


G_MODULE_EXPORT void on_fdc_fdd3_umount ( GtkMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif
    fdc_umount ( 3 );
}


void ui_fdc_set_dsk ( unsigned drive_id, char *dsk_filename ) {
    char item_fdd_name[] = "menuitem_fdc_fdd0";
    char item_dsk_name[] = "menuitem_fdc_fdd0_dsk";
    char item_umount_name[] = "menuitem_fdc_fdd0_umount";

    item_fdd_name [ 16 ] = '0' + drive_id;
    item_dsk_name [ 16 ] = '0' + drive_id;
    item_umount_name [ 16 ] = '0' + drive_id;

    if ( dsk_filename [ 0 ] != 0x00 ) {
        char fdd_label [ 50 ];
        sprintf ( fdd_label, "FDD _%d", drive_id );
        gtk_menu_item_set_label ( ui_get_menu_item ( item_fdd_name ), fdd_label );
        gtk_menu_item_set_label ( ui_get_menu_item ( item_dsk_name ), dsk_filename );
        gtk_widget_set_sensitive ( ui_get_widget ( item_umount_name ), TRUE );
    } else {
        char fdd_label [ 50 ];
        sprintf ( fdd_label, "FDD _%d (Empty)", drive_id );
        gtk_menu_item_set_label ( ui_get_menu_item ( item_fdd_name ), fdd_label );
        gtk_menu_item_set_label ( ui_get_menu_item ( item_dsk_name ), "Empty" );
        gtk_widget_set_sensitive ( ui_get_widget ( item_umount_name ), FALSE );
    };
}


void ui_fdc_menu_update ( void ) {
    LOCK_UICALLBACKS ( );
    if ( g_fdc.connected == FDC_DISCONNECTED ) {
        gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_fdc_not_connected" ), TRUE );
        gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_fdc_wd279x" ), FALSE );
        gtk_widget_set_sensitive ( ui_get_widget ( "menuitem_fdc_fdd0" ), FALSE );
        gtk_widget_set_sensitive ( ui_get_widget ( "menuitem_fdc_fdd1" ), FALSE );
        gtk_widget_set_sensitive ( ui_get_widget ( "menuitem_fdc_fdd2" ), FALSE );
        gtk_widget_set_sensitive ( ui_get_widget ( "menuitem_fdc_fdd3" ), FALSE );
    } else {
        gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_fdc_not_connected" ), FALSE );
        gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_fdc_wd279x" ), TRUE );
        gtk_widget_set_sensitive ( ui_get_widget ( "menuitem_fdc_fdd0" ), TRUE );
        gtk_widget_set_sensitive ( ui_get_widget ( "menuitem_fdc_fdd1" ), TRUE );
        gtk_widget_set_sensitive ( ui_get_widget ( "menuitem_fdc_fdd2" ), TRUE );
        gtk_widget_set_sensitive ( ui_get_widget ( "menuitem_fdc_fdd3" ), TRUE );
    };
    UNLOCK_UICALLBACKS ( );
}


G_MODULE_EXPORT void on_menuitem_fdc_create_new ( GtkMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

//    ui_dsk_tool_show_window ( );
}
