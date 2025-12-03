/* 
 * File:   ui_display.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 13. srpna 2015, 10:05
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

#include "display.h"
#include "iface_sdl/iface_sdl.h"


G_MODULE_EXPORT void on_menuitem_display_colors_normal ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif
    display_set_colors ( DISPLAY_NORMAL );
}


G_MODULE_EXPORT void on_menuitem_display_colors_grayscale ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif
    display_set_colors ( DISPLAY_GRAYSCALE );
}


G_MODULE_EXPORT void on_menuitem_display_colors_green ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif
    display_set_colors ( DISPLAY_GREEN );
}


G_MODULE_EXPORT void on_menuitem_display_window_size_normal_activate ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif
    display_set_window_startup_scale ( DISPLAY_STARTUP_SIZE_NORMAL );
}


G_MODULE_EXPORT void on_menuitem_display_window_size_bigger_activate ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif
    display_set_window_startup_scale ( DISPLAY_STARTUP_SIZE_BIGGER );
}

G_MODULE_EXPORT void on_menuitem_display_window_size_big_activate ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif
    display_set_window_startup_scale ( DISPLAY_STARTUP_SIZE_BIG );
}


G_MODULE_EXPORT void on_menuitem_display_fix_aspect_ratio_by_width_activate ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif
    iface_sdl_fix_window_aspect_ratio ( 'W' );
}


G_MODULE_EXPORT void on_menuitem_display_fix_aspect_ratio_by_height_activate ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif
    iface_sdl_fix_window_aspect_ratio ( 'H' );
}


G_MODULE_EXPORT void on_checkmenuitem_display_forced_redrawing_toggled ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif
    g_display.forced_full_screen_redrawing = gtk_check_menu_item_get_active ( ui_get_check_menu_item ( "checkmenuitem_display_forced_redrawing" ) );

}


void ui_display_update_menu ( void ) {

    LOCK_UICALLBACKS ( );

    switch ( display_get_window_color_schema ( ) ) {
        case DISPLAY_NORMAL:
            gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_display_colors_normal" ), TRUE );
            break;
        case DISPLAY_GRAYSCALE:
            gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_display_colors_grayscale" ), TRUE );
            break;
        case DISPLAY_GREEN:
            gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_display_colors_green" ), TRUE );
            break;
    };

#ifdef WINDOWS
    gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "checkmenuitem_display_window_size_lock_aspect_ratio" ), ( g_display.locked_window_aspect_ratio ) ? TRUE : FALSE );
    gtk_widget_show ( ui_get_widget ( "checkmenuitem_display_window_size_lock_aspect_ratio" ) );
#else
    gtk_widget_hide ( ui_get_widget ( "checkmenuitem_display_window_size_lock_aspect_ratio" ) );
#endif
    gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "checkmenuitem_display_forced_redrawing" ), ( g_display.forced_full_screen_redrawing ) ? TRUE : FALSE );

    UNLOCK_UICALLBACKS ( );
}


G_MODULE_EXPORT void on_checkmenuitem_display_window_size_lock_aspect_ratio_toggled ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif
    g_display.locked_window_aspect_ratio = gtk_check_menu_item_get_active ( ui_get_check_menu_item ( "checkmenuitem_display_window_size_lock_aspect_ratio" ) );

    if ( g_display.locked_window_aspect_ratio ) {
        iface_sdl_set_window_size_by_scale ( 1 );
    };
}

