/* 
 * File:   ui_ramdisk.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 10. srpna 2015, 20:44
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
#include "ui_file_chooser.h"
#include "ui_ramdisk.h"
#include "ui_fcbutton.h"

#include "ramdisk/ramdisk.h"
#include "ui_utils.h"


static st_UI_FCBUTTON *fcb_e8 = NULL;
static st_UI_FCBUTTON *fcb_68 = NULL;


char* ui_ramdisk_pezik_settings_get_surfix_by_type ( unsigned pezik_type ) {
    if ( pezik_type == RAMDISK_PEZIK_E8 ) {
        return "e8";
    };
    return "68";
}


void ui_ramdisk_pezik_settings_update_name_surfix ( char *name, char *surfix ) {
    strncpy ( &name [ strlen ( name ) - 2 ], surfix, 2 );
}


void ui_ramdisk_pezik_settings_connection_changed ( gboolean enabled, unsigned pezik_type ) {

    char combo_name[] = "comboboxtext_pezik_settings_e8";
    char label_name[] = "label_pezik_settings_e8";
    char grid_name[] = "grid_pezik_settings_e8";
    char backup_check_name[] = "checkbutton_pezik_settings_backuped_e8";
    char fcb_box_name[] = "box_pezik_settings_fcbutton_e8";

    char *pezik_port_name = ui_ramdisk_pezik_settings_get_surfix_by_type ( pezik_type );

    ui_ramdisk_pezik_settings_update_name_surfix ( combo_name, pezik_port_name );
    ui_ramdisk_pezik_settings_update_name_surfix ( label_name, pezik_port_name );
    ui_ramdisk_pezik_settings_update_name_surfix ( grid_name, pezik_port_name );
    ui_ramdisk_pezik_settings_update_name_surfix ( backup_check_name, pezik_port_name );
    ui_ramdisk_pezik_settings_update_name_surfix ( fcb_box_name, pezik_port_name );

    gtk_widget_set_sensitive ( ui_get_widget ( combo_name ), enabled );
    gtk_widget_set_sensitive ( ui_get_widget ( label_name ), enabled );
    gtk_widget_set_sensitive ( ui_get_widget ( grid_name ), enabled );
    gtk_widget_set_sensitive ( ui_get_widget ( backup_check_name ), enabled );
    gtk_widget_set_sensitive ( ui_get_widget ( fcb_box_name ), enabled );
}


void ui_ramdisk_pezik_settings_backuped_changed ( gboolean enabled, unsigned pezik_type ) {

    st_UI_FCBUTTON *fcb;

    if ( pezik_type == RAMDISK_PEZIK_E8 ) {
        fcb = fcb_e8;
    } else {
        fcb = fcb_68;
    };

    gtk_widget_set_sensitive ( fcb->button, enabled );
}


void ui_ramdisk_pezik_settings_update_combo ( gint pezik_type, gint pezik_portmask ) {

    gint combo_state;

    char *pezik_port_name = ui_ramdisk_pezik_settings_get_surfix_by_type ( pezik_type );

    char combo_name[] = "comboboxtext_pezik_settings_e8";
    ui_ramdisk_pezik_settings_update_name_surfix ( combo_name, pezik_port_name );


    if ( 0xff == pezik_portmask ) {
        combo_state = 0;
    } else if ( 0xf0 == pezik_portmask ) {
        combo_state = 1;
    } else if ( 0x0f == pezik_portmask ) {
        combo_state = 2;
    } else {
        combo_state = 3;
    };

    LOCK_UICALLBACKS ( );
    gtk_combo_box_set_active ( ui_get_combo_box ( combo_name ), combo_state );
    UNLOCK_UICALLBACKS ( );
}


void ui_ramdisk_pezik_settings_update_size ( gint pezik_type, gint pezik_portmask ) {

    char *pezik_port_name = ui_ramdisk_pezik_settings_get_surfix_by_type ( pezik_type );

    char label_size[] = "label_pezik_settings_size_e8";
    ui_ramdisk_pezik_settings_update_name_surfix ( label_size, pezik_port_name );

    int i;
    int pezik_size = 0;
    for ( i = 0; i < 8; i++ ) {
        if ( pezik_portmask & ( 1 << i ) ) {
            pezik_size += 64;
        };
    };

    char pezik_size_txt [ 4 ];
    sprintf ( pezik_size_txt, "%d", pezik_size );
    gtk_label_set_text ( ui_get_label ( label_size ), pezik_size_txt );
}


void ui_ramdisk_pezik_settings_update_banks ( gint pezik_type, gint pezik_portmask ) {

    char *pezik_port_name = ui_ramdisk_pezik_settings_get_surfix_by_type ( pezik_type );

    char bank_check[] = "pezik_settings_port_e8";
    ui_ramdisk_pezik_settings_update_name_surfix ( bank_check, pezik_port_name );

    LOCK_UICALLBACKS ( );

    int i;
    int pezik_size = 0;
    for ( i = 0; i < 8; i++ ) {

        gboolean bank_connected = FALSE;

        if ( pezik_portmask & ( 1 << i ) ) {
            bank_connected = TRUE;
            pezik_size += 64;
        };

        char *c = &bank_check [ strlen ( bank_check ) - 1 ];
        sprintf ( c, "%x", i + 0x08 );

        gtk_toggle_button_set_active ( ui_get_toggle ( bank_check ), bank_connected );
    };

    UNLOCK_UICALLBACKS ( );
}


gint ui_ramdisk_pezik_settings_get_portmap ( gint pezik_type ) {

    gint pezik_portmask = 0x00;
    char *pezik_port_name = ui_ramdisk_pezik_settings_get_surfix_by_type ( pezik_type );

    char bank_check[] = "pezik_settings_port_e8";
    ui_ramdisk_pezik_settings_update_name_surfix ( bank_check, pezik_port_name );

    int i;
    for ( i = 0; i < 8; i++ ) {

        char *c = &bank_check [ strlen ( bank_check ) - 1 ];
        sprintf ( c, "%x", i + 0x08 );

        if ( gtk_toggle_button_get_active ( ui_get_toggle ( bank_check ) ) ) {
            pezik_portmask |= ( 1 << i );
        };
    };

    return pezik_portmask;
}


void on_fcbutton_pezik_settings_clicked ( GtkButton *button, st_UI_FCBUTTON *fcb ) {
    char title[] = "Select Backup File for Pezik";
    char *filename = ui_file_chooser_open_dat ( fcb->filepath, title, (void*) ui_get_window ( "window_pezik_settings" ), FC_MODE_SAVE );
    if ( !filename ) return;
    ui_fcbutton_set_filepath ( fcb, filename );
    ui_utils_mem_free ( filename );
}


void ui_ramdisk_pezik_settings_init ( unsigned pezik_type ) {

    char *pezik_port_name = ui_ramdisk_pezik_settings_get_surfix_by_type ( pezik_type );

    st_UI_FCBUTTON **fcb;

    if ( pezik_type == RAMDISK_PEZIK_E8 ) {
        fcb = &fcb_e8;
    } else {
        fcb = &fcb_68;
    };

    if ( NULL == *fcb ) {

        char box_name[] = "box_pezik_settings_fcbutton_e8";
        ui_ramdisk_pezik_settings_update_name_surfix ( box_name, pezik_port_name );
        GtkGrid *box = (GtkGrid*) ui_get_widget ( box_name );

        *fcb = ui_fcbutton_new ( );
        st_UI_FCBUTTON *fcb1 = *fcb;
        g_signal_connect ( (gpointer) fcb1->button, "clicked", G_CALLBACK ( on_fcbutton_pezik_settings_clicked ), *fcb );
        gtk_box_pack_start ( GTK_BOX ( box ), fcb1->button, TRUE, TRUE, 0 );
    };
    ui_fcbutton_set_filepath ( *fcb, g_ramdisk.pezik [ pezik_type ].filepath );

    char check_name[] = "checkbutton_pezik_settings_connected_e8";
    ui_ramdisk_pezik_settings_update_name_surfix ( check_name, pezik_port_name );

    char backuped_check_name[] = "checkbutton_pezik_settings_backuped_e8";
    ui_ramdisk_pezik_settings_update_name_surfix ( backuped_check_name, pezik_port_name );

    gboolean pezik_connected;

    if ( g_ramdisk.pezik [ pezik_type ].connected ) {
        pezik_connected = TRUE;
    } else {
        pezik_connected = FALSE;
    };

    LOCK_UICALLBACKS ( );
    gtk_toggle_button_set_active ( ui_get_toggle ( check_name ), pezik_connected );
    UNLOCK_UICALLBACKS ( );

    ui_ramdisk_pezik_settings_connection_changed ( pezik_connected, pezik_type );

    ui_ramdisk_pezik_settings_update_combo ( pezik_type, g_ramdisk.pezik [ pezik_type ].portmask );
    ui_ramdisk_pezik_settings_update_size ( pezik_type, g_ramdisk.pezik [ pezik_type ].portmask );
    ui_ramdisk_pezik_settings_update_banks ( pezik_type, g_ramdisk.pezik [ pezik_type ].portmask );

    gboolean pezik_backuped;

    if ( PEZIK_BACKUPED_YES == g_ramdisk.pezik [ pezik_type ].backuped ) {
        pezik_backuped = TRUE;
    } else {
        pezik_backuped = FALSE;
    };

    LOCK_UICALLBACKS ( );
    gtk_toggle_button_set_active ( ui_get_toggle ( backuped_check_name ), pezik_backuped );
    UNLOCK_UICALLBACKS ( );

    ui_ramdisk_pezik_settings_backuped_changed ( pezik_backuped, pezik_type );
}


void ui_ramdisk_show_pezik_settings ( void ) {

    GtkWidget *window = ui_get_widget ( "window_pezik_settings" );
    gtk_widget_show ( window );

    ui_ramdisk_pezik_settings_init ( RAMDISK_PEZIK_E8 );
    ui_ramdisk_pezik_settings_init ( RAMDISK_PEZIK_68 );

}


void ui_ramdisk_pezik_settings_hide_window ( void ) {
    GtkWidget *window = ui_get_widget ( "window_pezik_settings" );
    gtk_widget_hide ( window );
}


void ui_ramdisk_pezik_settings_combobox_changed ( gint combo_state, gint pezik_type ) {

    gint portmask;

    if ( 0 == combo_state ) {
        portmask = 0xff;
    } else if ( 1 == combo_state ) {
        portmask = 0xf0;
    } else if ( 2 == combo_state ) {
        portmask = 0x0f;
    } else {
        return;
    };

    ui_ramdisk_pezik_settings_update_size ( pezik_type, portmask );
    ui_ramdisk_pezik_settings_update_banks ( pezik_type, portmask );
}


typedef struct st_UIRAMPEZSET {
    gboolean connected;
    guint portmask;
    gboolean backuped;
    const gchar *filepath;
} st_UIRAMPEZSET;


void ui_ramdisk_pezik_settings_get_pezik_settings ( unsigned pezik_type, st_UIRAMPEZSET *pezset ) {

    char *pezik_port_name = ui_ramdisk_pezik_settings_get_surfix_by_type ( pezik_type );

    char check_name[] = "checkbutton_pezik_settings_connected_e8";
    ui_ramdisk_pezik_settings_update_name_surfix ( check_name, pezik_port_name );
    pezset->connected = gtk_toggle_button_get_active ( ui_get_toggle ( check_name ) );

    pezset->portmask = ui_ramdisk_pezik_settings_get_portmap ( pezik_type );

    char backuped_check_name[] = "checkbutton_pezik_settings_backuped_e8";
    ui_ramdisk_pezik_settings_update_name_surfix ( backuped_check_name, pezik_port_name );
    pezset->backuped = gtk_toggle_button_get_active ( ui_get_toggle ( backuped_check_name ) );

    st_UI_FCBUTTON *fcb;

    if ( pezik_type == RAMDISK_PEZIK_E8 ) {
        fcb = fcb_e8;
    } else {
        fcb = fcb_68;
    };

    pezset->filepath = ui_fcbutton_get_filepath ( fcb );
}


G_MODULE_EXPORT gboolean on_window_pezik_settings_delete_event ( GtkWidget *widget, GdkEvent *event, gpointer user_data ) {
    ui_ramdisk_pezik_settings_hide_window ( );
    return TRUE;
}


G_MODULE_EXPORT void on_button_pezik_settings_close_clicked ( GtkButton *button, gpointer user_data ) {
    ui_ramdisk_pezik_settings_hide_window ( );
}


G_MODULE_EXPORT void on_checkbutton_pezik_settings_e8_connected_toggled ( GtkToggleButton *togglebutton, gpointer user_data ) {
    if ( TEST_UICALLBACKS_LOCKED ) return;
    ui_ramdisk_pezik_settings_connection_changed ( gtk_toggle_button_get_active ( togglebutton ), RAMDISK_PEZIK_E8 );
}


G_MODULE_EXPORT void on_checkbutton_pezik_settings_68_connected_toggled ( GtkToggleButton *togglebutton, gpointer user_data ) {
    if ( TEST_UICALLBACKS_LOCKED ) return;
    ui_ramdisk_pezik_settings_connection_changed ( gtk_toggle_button_get_active ( togglebutton ), RAMDISK_PEZIK_68 );
}


G_MODULE_EXPORT void on_comboboxtext_pezik_settings_e8_changed ( GtkComboBox *combobox, gpointer data ) {
    (void) data;
    if ( TEST_UICALLBACKS_LOCKED ) return;
    ui_ramdisk_pezik_settings_combobox_changed ( gtk_combo_box_get_active ( combobox ), RAMDISK_PEZIK_E8 );
}


G_MODULE_EXPORT void on_comboboxtext_pezik_settings_68_changed ( GtkComboBox *combobox, gpointer data ) {
    (void) data;
    if ( TEST_UICALLBACKS_LOCKED ) return;
    ui_ramdisk_pezik_settings_combobox_changed ( gtk_combo_box_get_active ( combobox ), RAMDISK_PEZIK_68 );
}


G_MODULE_EXPORT void on_pezik_settings_e8_port_changed ( GtkToggleButton *togglebutton, gpointer user_data ) {
    if ( TEST_UICALLBACKS_LOCKED ) return;

    gint portmask = ui_ramdisk_pezik_settings_get_portmap ( RAMDISK_PEZIK_E8 );
    if ( 0x00 == portmask ) {
        /* nepovoleny stav */
        LOCK_UICALLBACKS ( );
        gtk_toggle_button_set_active ( togglebutton, TRUE );
        UNLOCK_UICALLBACKS ( );
        return;
    };

    ui_ramdisk_pezik_settings_update_size ( RAMDISK_PEZIK_E8, portmask );
    ui_ramdisk_pezik_settings_update_combo ( RAMDISK_PEZIK_E8, portmask );
}


G_MODULE_EXPORT void on_pezik_settings_68_port_changed ( GtkToggleButton *togglebutton, gpointer user_data ) {
    if ( TEST_UICALLBACKS_LOCKED ) return;

    gint portmask = ui_ramdisk_pezik_settings_get_portmap ( RAMDISK_PEZIK_68 );
    if ( 0x00 == portmask ) {
        /* nepovoleny stav */
        LOCK_UICALLBACKS ( );
        gtk_toggle_button_set_active ( togglebutton, TRUE );
        UNLOCK_UICALLBACKS ( );
        return;
    };

    ui_ramdisk_pezik_settings_update_size ( RAMDISK_PEZIK_68, portmask );
    ui_ramdisk_pezik_settings_update_combo ( RAMDISK_PEZIK_68, portmask );
}


G_MODULE_EXPORT void on_checkbutton_pezik_settings_backuped_e8_toggled ( GtkToggleButton *togglebutton, gpointer user_data ) {
    if ( TEST_UICALLBACKS_LOCKED ) return;
    ui_ramdisk_pezik_settings_backuped_changed ( gtk_toggle_button_get_active ( togglebutton ), RAMDISK_PEZIK_E8 );
}


G_MODULE_EXPORT void on_checkbutton_pezik_settings_backuped_68_toggled ( GtkToggleButton *togglebutton, gpointer user_data ) {
    if ( TEST_UICALLBACKS_LOCKED ) return;
    ui_ramdisk_pezik_settings_backuped_changed ( gtk_toggle_button_get_active ( togglebutton ), RAMDISK_PEZIK_68 );
}


G_MODULE_EXPORT void on_button_pezik_settings_ok_clicked ( GtkButton *button, gpointer user_data ) {

    st_UIRAMPEZSET pezset;

    ui_ramdisk_pezik_settings_get_pezik_settings ( RAMDISK_PEZIK_E8, &pezset );

    /* nepovolena kombinace */
    if ( ( TRUE == pezset.connected ) && ( RAMDISK_CONNECTED == g_ramdisk.std.connected ) ) {
        ramdisk_std_disconnect ( );
    };

    ramdisk_pezik_init ( RAMDISK_PEZIK_E8, pezset.connected, pezset.portmask, pezset.backuped, (char *) pezset.filepath );

    ui_ramdisk_pezik_settings_get_pezik_settings ( RAMDISK_PEZIK_68, &pezset );
    ramdisk_pezik_init ( RAMDISK_PEZIK_68, pezset.connected, pezset.portmask, pezset.backuped, (char *) pezset.filepath );

    ui_ramdisk_pezik_settings_hide_window ( );
    ui_ramdisk_update_menu ( );
}


/* 
 * 
 * 
 * 
 * main menu ramdisk events 
 *
 * 
 * 
 */

G_MODULE_EXPORT void on_ramdisk_not_connected ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

    ramdisk_pezik_disconnect ( RAMDISK_PEZIK_E8 );
    ramdisk_pezik_disconnect ( RAMDISK_PEZIK_68 );
    ramdisk_std_disconnect ( );

    ui_ramdisk_update_menu ( );
}


G_MODULE_EXPORT void on_ramdisk_pezik_e8 ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

    if ( gtk_check_menu_item_get_active ( ui_get_check_menu_item ( "menuitem_ramdisk_pezik_e8" ) ) ) {
        ramdisk_pezik_connect ( RAMDISK_PEZIK_E8 );
    } else {
        ramdisk_pezik_disconnect ( RAMDISK_PEZIK_E8 );
    };

    ui_ramdisk_update_menu ( );
}


G_MODULE_EXPORT void on_ramdisk_pezik_68 ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

    if ( gtk_check_menu_item_get_active ( ui_get_check_menu_item ( "menuitem_ramdisk_pezik_68" ) ) ) {
        ramdisk_pezik_connect ( RAMDISK_PEZIK_68 );
    } else {
        ramdisk_pezik_disconnect ( RAMDISK_PEZIK_68 );
    };

    ui_ramdisk_update_menu ( );
}


G_MODULE_EXPORT void on_ramdisk_std ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

    if ( gtk_check_menu_item_get_active ( ui_get_check_menu_item ( "menuitem_ramdisk_std" ) ) ) {
        ramdisk_std_init ( RAMDISK_CONNECTED, g_ramdisk.std.type, g_ramdisk.std.size, g_ramdisk.std.filepath );
    } else {
        ramdisk_std_disconnect ( );
    };

    ui_ramdisk_update_menu ( );
}


G_MODULE_EXPORT void on_menuitem_pezik_settings_activate ( GtkMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

    ui_ramdisk_show_pezik_settings ( );
}


G_MODULE_EXPORT void on_ramdisk_size_64 ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

    ramdisk_std_init ( RAMDISK_CONNECTED, g_ramdisk.std.type, RAMDISK_SIZE_64, g_ramdisk.std.filepath );
    ui_ramdisk_update_menu ( );
}


G_MODULE_EXPORT void on_ramdisk_size_256 ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

    ramdisk_std_init ( RAMDISK_CONNECTED, g_ramdisk.std.type, RAMDISK_SIZE_256, g_ramdisk.std.filepath );
    ui_ramdisk_update_menu ( );
}


G_MODULE_EXPORT void on_ramdisk_size_512 ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

    ramdisk_std_init ( RAMDISK_CONNECTED, g_ramdisk.std.type, RAMDISK_SIZE_512, g_ramdisk.std.filepath );
    ui_ramdisk_update_menu ( );
}


G_MODULE_EXPORT void on_ramdisk_size_1mb ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

    ramdisk_std_init ( RAMDISK_CONNECTED, g_ramdisk.std.type, RAMDISK_SIZE_1M, g_ramdisk.std.filepath );
    ui_ramdisk_update_menu ( );
}


G_MODULE_EXPORT void on_ramdisk_size_16mb ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

    ramdisk_std_init ( RAMDISK_CONNECTED, g_ramdisk.std.type, RAMDISK_SIZE_16M, g_ramdisk.std.filepath );
    ui_ramdisk_update_menu ( );
}


G_MODULE_EXPORT void on_ramdisk_type_std ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

    ramdisk_std_init ( RAMDISK_CONNECTED, RAMDISK_TYPE_STD, g_ramdisk.std.size, g_ramdisk.std.filepath );
    ui_ramdisk_update_menu ( );
}


G_MODULE_EXPORT void on_ramdisk_type_sram ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

    ramdisk_std_init ( RAMDISK_CONNECTED, RAMDISK_TYPE_SRAM, g_ramdisk.std.size, g_ramdisk.std.filepath );
    ui_ramdisk_update_menu ( );
}


G_MODULE_EXPORT void on_ramdisk_type_rom ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

    ramdisk_std_init ( RAMDISK_CONNECTED, RAMDISK_TYPE_ROM, g_ramdisk.std.size, g_ramdisk.std.filepath );
    ui_ramdisk_update_menu ( );
}


G_MODULE_EXPORT void on_ramdisk_file ( GtkMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

    ramdisk_std_open_file ( );
    ui_ramdisk_update_menu ( );
}


void ui_ramdisk_update_menu ( void ) {

    LOCK_UICALLBACKS ( );

    unsigned ramdisk_connected = 0;

    gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_ramdisk_not_connected" ), TRUE );

    gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_ramdisk_pezik_e8" ), FALSE );
    gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_ramdisk_pezik_68" ), FALSE );
    gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_ramdisk_std" ), FALSE );

    gtk_widget_set_sensitive ( ui_get_widget ( "menuitem_ramdisk_pezik_e8" ), TRUE );
    gtk_widget_set_sensitive ( ui_get_widget ( "menuitem_ramdisk_pezik_68" ), TRUE );
    gtk_widget_set_sensitive ( ui_get_widget ( "menuitem_ramdisk_std" ), TRUE );

    gtk_widget_set_sensitive ( ui_get_widget ( "menuitem_ramdisk_size" ), FALSE );
    gtk_widget_set_sensitive ( ui_get_widget ( "menuitem_ramdisk_type" ), FALSE );
    gtk_widget_set_sensitive ( ui_get_widget ( "menuitem_ramdisk_file" ), FALSE );


    if ( g_ramdisk.pezik[RAMDISK_PEZIK_E8].connected ) {
        ramdisk_connected = 1;
        gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_ramdisk_pezik_e8" ), TRUE );

        gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_ramdisk_std" ), FALSE );
        gtk_widget_set_sensitive ( ui_get_widget ( "menuitem_ramdisk_std" ), FALSE );
    };

    if ( g_ramdisk.pezik[RAMDISK_PEZIK_68].connected ) {
        ramdisk_connected = 1;
        gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_ramdisk_pezik_68" ), TRUE );
    };

    if ( g_ramdisk.std.connected ) {
        ramdisk_connected = 1;
        gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_ramdisk_std" ), TRUE );

        gtk_widget_set_sensitive ( ui_get_widget ( "menuitem_ramdisk_pezik_e8" ), FALSE );

        gtk_widget_set_sensitive ( ui_get_widget ( "menuitem_ramdisk_size" ), TRUE );
        gtk_widget_set_sensitive ( ui_get_widget ( "menuitem_ramdisk_type" ), TRUE );
        gtk_widget_set_sensitive ( ui_get_widget ( "menuitem_ramdisk_file" ), TRUE );

        switch ( g_ramdisk.std.size ) {
            case RAMDISK_SIZE_64:
                gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_ramdisk_size_64" ), TRUE );
                break;
            case RAMDISK_SIZE_256:
                gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_ramdisk_size_256" ), TRUE );
                break;
            case RAMDISK_SIZE_512:
                gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_ramdisk_size_512" ), TRUE );
                break;
            case RAMDISK_SIZE_1M:
                gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_ramdisk_size_1mb" ), TRUE );
                break;
            case RAMDISK_SIZE_16M:
                gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_ramdisk_size_16mb" ), TRUE );
                break;
        };

        switch ( g_ramdisk.std.type ) {
            case RAMDISK_TYPE_STD:
                gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_ramdisk_type_std" ), TRUE );
                gtk_widget_set_sensitive ( ui_get_widget ( "menuitem_ramdisk_file" ), FALSE );
                break;
            case RAMDISK_TYPE_SRAM:
                gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_ramdisk_type_sram" ), TRUE );
                break;
            case RAMDISK_TYPE_ROM:
                gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_ramdisk_type_rom" ), TRUE );
                break;
        };
    };

    if ( ramdisk_connected ) {
        gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_ramdisk_not_connected" ), FALSE );
    }

    UNLOCK_UICALLBACKS ( );
}
