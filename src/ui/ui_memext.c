/* 
 * File:   ui_memext.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 18. července 2018, 14:59
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
#include <string.h>

#include "ui_main.h"
#include "ui_file_chooser.h"
#include "debugger/ui_memsave.h"
#include "debugger/ui_memload.h"

#include "memory/memext.h"
#include "ui_utils.h"


void ui_memext_menu_update ( void ) {

    LOCK_UICALLBACKS ( );

#ifdef MZ800EMU_CFG_DEBUGGER_ENABLED
    gtk_widget_set_visible ( ui_get_widget ( "menuitem_memext_mem_content" ), TRUE );
    // menu Debugger -> MemExt Settings...
    gtk_widget_set_sensitive ( ui_get_widget ( "menuitem_memext_map_settings" ), ( TEST_MEMEXT_CONNECTED ) );
#else
    gtk_widget_set_visible ( ui_get_widget ( "menuitem_memext_mem_content" ), FALSE );
#endif

    gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "radiomenuitem_memext_notconnected" ), ( !TEST_MEMEXT_CONNECTED ) );
    gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "radiomenuitem_memext_pehu" ), ( TEST_MEMEXT_CONNECTED_PEHU ) );
    gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "radiomenuitem_memext_luftner" ), ( TEST_MEMEXT_CONNECTED_LUFTNER ) );

    gtk_widget_set_sensitive ( ui_get_widget ( "menuitem_memext_init_mem" ), ( TEST_MEMEXT_CONNECTED ) );
    gtk_widget_set_sensitive ( ui_get_widget ( "menuitem_memext_mem_content" ), ( TEST_MEMEXT_CONNECTED ) );

    gtk_widget_set_sensitive ( ui_get_widget ( "menuitem_memext_luftner_flash_settings" ), ( TEST_MEMEXT_TYPE_LUFTNER ) );
    gtk_widget_set_sensitive ( ui_get_widget ( "checkmenuitem_memext_luftner_init_reset" ), ( TEST_MEMEXT_TYPE_LUFTNER ) );
    gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "checkmenuitem_memext_luftner_init_reset" ), ( TEST_MEMEXT_LUFTNER_AUTO_INIT ) );
    gtk_widget_set_sensitive ( ui_get_widget ( "menuitem_memext_flash_load" ), ( TEST_MEMEXT_TYPE_LUFTNER ) );
    gtk_widget_set_sensitive ( ui_get_widget ( "menuitem_memext_flash_save" ), ( TEST_MEMEXT_TYPE_LUFTNER ) );

    switch ( g_memext.init_mem ) {
        case MEMEXT_INIT_MEM_NULL:
            gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "radiomenuitem_memext_init_mem_null" ), TRUE );
            break;
        case MEMEXT_INIT_MEM_RANDOM:
            gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "radiomenuitem_memext_init_mem_random" ), TRUE );
            break;
        case MEMEXT_INIT_MEM_SHARP:
            gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "radiomenuitem_memext_init_mem_sharp" ), TRUE );
            break;
    };

    UNLOCK_UICALLBACKS ( );
}


G_MODULE_EXPORT void on_memext_connection_changed ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
    if ( TEST_UICALLBACKS_LOCKED ) return;

#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

    if ( gtk_check_menu_item_get_active ( ui_get_check_menu_item ( "radiomenuitem_memext_notconnected" ) ) ) {
        memext_disconnect ( );
    } else if ( gtk_check_menu_item_get_active ( ui_get_check_menu_item ( "radiomenuitem_memext_pehu" ) ) ) {
        memext_connect ( MEMEXT_TYPE_PEHU );
    } else {
        memext_connect ( MEMEXT_TYPE_LUFTNER );
    };
}


G_MODULE_EXPORT void on_memext_init_mem_changed ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
    if ( TEST_UICALLBACKS_LOCKED ) return;

#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

    if ( gtk_check_menu_item_get_active ( ui_get_check_menu_item ( "radiomenuitem_memext_init_mem_null" ) ) ) {
        g_memext.init_mem = MEMEXT_INIT_MEM_NULL;
    } else if ( gtk_check_menu_item_get_active ( ui_get_check_menu_item ( "radiomenuitem_memext_init_mem_random" ) ) ) {
        g_memext.init_mem = MEMEXT_INIT_MEM_RANDOM;
    } else {
        g_memext.init_mem = MEMEXT_INIT_MEM_SHARP;
    };

    ui_memext_menu_update ( );
}


G_MODULE_EXPORT void on_checkmenuitem_memext_luftner_init_reset_toggled ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

    gboolean state = gtk_check_menu_item_get_active ( ui_get_check_menu_item ( "checkmenuitem_memext_luftner_init_reset" ) );
    g_memext.init_luftner = ( state ) ? MEMEXT_INIT_LUFTNER_RESET : MEMEXT_INIT_LUFTNER_NONE;

    ui_memext_menu_update ( );
}


G_MODULE_EXPORT void on_menuitem_memext_luftner_flash_settings_activate ( GtkMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

    char title[] = "Select FLASH Autoload File for Luftner's MemExt";
    char *filepath = ui_file_chooser_open_dat ( g_memext.flash_filepath, title, NULL, FC_MODE_OPEN );
    if ( !filepath ) return;
    int len = strlen ( filepath ) + 1;
    g_memext.flash_filepath = (char*) ui_utils_mem_realloc ( g_memext.flash_filepath, len );
    strncpy ( g_memext.flash_filepath, filepath, len );
    g_free ( filepath );
    memext_flash_reload ( );
}


G_MODULE_EXPORT void on_menuitem_memext_ram_load_activate ( GtkMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

#ifdef MZ800EMU_CFG_DEBUGGER_ENABLED
    ui_memload_select_file ( g_memext.RAM, NULL, MEMEXT_RAM_SIZE, NULL );
#endif
}


G_MODULE_EXPORT void on_menuitem_memext_ram_save_activate ( GtkMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

#ifdef MZ800EMU_CFG_DEBUGGER_ENABLED
    ui_memsave_window_show ( g_memext.RAM, MEMEXT_RAM_SIZE, 0 );
#endif
}


G_MODULE_EXPORT void on_menuitem_memext_flash_load_activate ( GtkMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

#ifdef MZ800EMU_CFG_DEBUGGER_ENABLED
    ui_memload_select_file ( g_memext.RAM, NULL, MEMEXT_FLASH_SIZE, NULL );
#endif
}


G_MODULE_EXPORT void on_menuitem_memext_flash_save_activate ( GtkMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

#ifdef MZ800EMU_CFG_DEBUGGER_ENABLED
    ui_memsave_window_show ( g_memext.FLASH, MEMEXT_FLASH_SIZE, 0 );
#endif
}
