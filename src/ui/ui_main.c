/* 
 * File:   ui_main.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 3. července 2015, 9:44
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

#define _GNU_SOURCE /* vasprintf */

#ifdef WINDOWS
#include<windows.h>
#endif

/*
#include <locale.h>
#include <glib.h>
#include <glib/gi18n.h>
#define GETTEXT_PACKAGE "gtk30"
 */
#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gprintf.h>

/*
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <locale.h>

#include "main.h"
#include "ui_main.h"
#include "ui_utils.h"
#include "iface_sdl/iface_sdl.h"
#include "ui_display.h"

#include "mz800.h"

#include "cfgmain.h"
#include "build_time.h"


#ifdef MZ800EMU_CFG_DEBUGGER_ENABLED
#include "debugger/debugger.h"
#include "ui/debugger/ui_debugger.h"
#include "ui/debugger/ui_breakpoints.h"
#include "ui/debugger/ui_dbg_memext.h"
#include "debugger/ui_membrowser.h"
#include "debugger/ui_dissassembler.h"
#endif

#include "generic_driver/ui_file_driver.h"
#include "generic_driver/ui_memory_driver.h"

#include "dsk_tool/ui_dsk_tool.h"
#include "ui_joy.h"
#include "cmt/cmt.h"
#include "ui_file_chooser.h"
#include "version_check/version_check.h"
#include "ui_version_check.h"



st_UI g_ui;
static int g_ui_is_initialised = 0;

#define UI_RESOURCES_DIR    "ui_resources/"


GObject* ui_get_object_safely ( gchar *name ) {
    GObject *gobj = gtk_builder_get_object ( g_ui.builder, name );
    if ( gobj == NULL ) {
        ui_show_error ( "Object '%s' not found!\nYou have fresh files in ./%s directory.", name, UI_RESOURCES_DIR );
        gchar *msg = "Do you want kill this application immediatly?";
        GtkWidget *dialog = gtk_message_dialog_new ( NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, msg );
        if ( gtk_dialog_run ( GTK_DIALOG ( dialog ) ) == GTK_RESPONSE_YES ) {
            main_app_quit ( EXIT_FAILURE );
        };
    };
    return gobj;
}


void ui_main_setpos ( st_UIWINPOS *wpos, gint x, gint y ) {
    wpos->x = x;
    wpos->y = y;
}


void ui_main_win_move_to_pos ( GtkWindow *w, st_UIWINPOS *wpos ) {
    if ( ( wpos->x == -1 ) || ( wpos->y == -1 ) ) return;
    gtk_window_move ( w, wpos->x, wpos->y );
}


void ui_main_win_get_pos ( GtkWindow *w, st_UIWINPOS *wpos ) {
    gtk_window_get_position ( w, &wpos->x, &wpos->y );
}


void ui_main_update_menuitem_disabled_hotkeys ( unsigned state ) {
    LOCK_UICALLBACKS ( );
    if ( state ) {
        gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_keyboard_disable_hotkeys" ), TRUE );
    } else {
        gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_keyboard_disable_hotkeys" ), FALSE );
    };
    UNLOCK_UICALLBACKS ( );
}


void ui_disable_hotkeys ( unsigned value ) {
    value &= 1;
    if ( value == g_ui.disable_hotkeys ) return;
    g_ui.disable_hotkeys = value;
    ui_main_update_menuitem_disabled_hotkeys ( value );
    printf ( "INFO: Hotkeys in the main emulator window are %s.\n", ( !value ) ? "ENABLED" : "DISABLED" );
}


void ui_propagatecfg_disable_hotkeys ( void *e, void *data ) {
    ui_disable_hotkeys ( cfgelement_get_bool_value ( (CFGELM *) e ) );
}


void ui_init ( void ) {

    setlocale ( LC_ALL, "" );
    /*
            bindtextdomain ( GETTEXT_PACKAGE, DATADIR "/locale" );
            bind_textdomain_codeset ( GETTEXT_PACKAGE, "UTF-8" );
            textdomain ( GETTEXT_PACKAGE );
     */

    //gtk_init ( &argc, &argv );
    gtk_init ( 0, NULL );


    printf ( "GTK UI Init: %d.%d.%d\n", gtk_get_major_version ( ), gtk_get_minor_version ( ), gtk_get_micro_version ( ) );


    /* prozatim nepotrebujeme */
    /* add_pixmap_directory (PACKAGE_DATA_DIR "/" PACKAGE "/pixmaps");*/


    UNLOCK_UICALLBACKS ( );
    g_ui.builder = NULL;

    g_ui.builder = gtk_builder_new ( );
    GError *err = NULL;
    GString *StrErr = g_string_new ( "" );
    GString *AllStrErr = g_string_new ( "" );

    const char *gladeResources[] = {
                                    UI_RESOURCES_DIR "mz800emu.glade",
                                    UI_RESOURCES_DIR "mz800emu_cmt.glade",
                                    UI_RESOURCES_DIR "dsk_tool.glade",
                                    UI_RESOURCES_DIR "mz800emu_audio_setup.glade",
                                    UI_RESOURCES_DIR "mz800emu_topmenu.glade",
#ifdef MZ800EMU_CFG_DEBUGGER_ENABLED
        /* TODO: prozkoumat GtkCssProvider - nastavit rules hint pro disassembled a stack treeview */
                                    UI_RESOURCES_DIR "mz800emu_debugger.glade",
                                    UI_RESOURCES_DIR "membrowser.glade",
#endif
        NULL
    };

    int i = 0;
    while ( NULL != gladeResources[i] ) {
        if ( 0 == gtk_builder_add_from_file ( g_ui.builder, gladeResources[i++], &err ) ) {
            g_string_printf ( StrErr, "GtkBuilder error: %s\n", err->message );
            g_string_append ( AllStrErr, StrErr->str );
            printf ( StrErr->str );
        };
    };

    g_string_free ( StrErr, TRUE );

    if ( AllStrErr->len ) {
        GtkWidget *w = gtk_window_new ( GTK_WINDOW_TOPLEVEL );
        GtkWidget *dialog = gtk_message_dialog_new ( GTK_WINDOW ( w ), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, AllStrErr->str );
        gtk_dialog_run ( GTK_DIALOG ( dialog ) );
        gtk_widget_destroy ( dialog );
        gtk_widget_destroy ( w );
        g_string_free ( AllStrErr, TRUE );
        main_app_quit ( EXIT_FAILURE );
    };

    g_string_free ( AllStrErr, TRUE );

#ifndef MZ800EMU_CFG_DEBUGGER_ENABLED
    gtk_widget_set_sensitive ( ui_get_widget ( "menuitem_debugger" ), FALSE );
#endif

    gtk_builder_connect_signals ( g_ui.builder, NULL );
    /* jina moznost jak registrovat callbacky:*/
    /* gtk_builder_add_callback_symbol ( builder, "on_circle_clicked",  G_CALLBACK ( on_circle_clicked ) ); */
    /* g_object_unref ( G_OBJECT ( builder ) ); */


    GtkCssProvider *provider;
    GdkDisplay *display;
    GdkScreen *screen;

    provider = gtk_css_provider_new ( );
    display = gdk_display_get_default ( );
    screen = gdk_display_get_default_screen ( display );

    gtk_style_context_add_provider_for_screen ( screen,
                                                GTK_STYLE_PROVIDER ( provider ),
                                                GTK_STYLE_PROVIDER_PRIORITY_APPLICATION );



    gtk_style_context_add_provider_for_screen ( screen,
                                                GTK_STYLE_PROVIDER ( provider ),
                                                GTK_STYLE_PROVIDER_PRIORITY_APPLICATION );

#if 0
    gtk_css_provider_load_from_data ( GTK_CSS_PROVIDER ( provider ),
                                      " GtkWindow {\n"
                                      "   -GtkWidget-focus-line-width: 0;\n"
                                      /* The next 2 lines are not guaranteed to work, they can be can be overridden by the window manager*/
                                      "   -GtkWindow-resize-grip-height: 0;\n"
                                      "   -GtkWindow-resize-grip-width: 0;\n"
                                      /* The next 4 lines are just 4 different ways to make the background blue. Each one overrides the last one.
            Their just different color units: named color units, rgb,  rgba, hexidecimal, and shade*/
                                      "   background-color: blue;\n"
                                      "   background-color: rgb (0, 0, 255);\n"
                                      "   background-color: rgba (0,0,255,1);\n"
                                      "   background-color: #0000FF;\n"
                                      "   background-color: shade(blue, 1.0);\n"
                                      "}\n", -1, NULL );
#endif

    gtk_css_provider_load_from_path ( GTK_CSS_PROVIDER ( provider ), UI_RESOURCES_DIR "mz800emu.css", &err );

    if ( err ) {
        printf ( "provider load error: %s\n", err->message );
    };


    g_object_unref ( provider );



    /*
     *  Konfiguracni udaje pro UI
     */


    CFGMOD *cmod = cfgroot_register_new_module ( g_cfgmain, "UI" );

    CFGELM *elm;
    elm = cfgmodule_register_new_element ( cmod, "disable_hot_keys", CFGENTYPE_BOOL, 0 );

    cfgelement_set_propagate_cb ( elm, ui_propagatecfg_disable_hotkeys, NULL );
    cfgelement_set_handlers ( elm, (void*) &g_ui.disable_hotkeys, (void*) &g_ui.disable_hotkeys );

    cfgmodule_parse ( cmod );
    cfgmodule_propagate ( cmod );

    ui_main_setpos ( &g_ui.filebrowser_pos, -1, -1 );

    g_ui_is_initialised = 1;

    /* display bylo nacteno jeste pred inicializaci ui, proto udelame update_menu nyni */
    ui_display_update_menu ( );

    /* inicializace generickych driveru */
    ui_file_driver_init ( );
    ui_memory_driver_init ( );
    ui_file_chooser_init ( );
}


void ui_exit ( void ) {

    ui_file_chooser_exit ( );

    g_ui_is_initialised = 0;
    /* TODO: je potreba nejak specialne ukoncit i gtk?*/
}


void ui_iteration ( void ) {
    while ( gtk_events_pending ( ) ) {
        gtk_main_iteration ( );
    };
}


#if 0


void ui_short_iteration ( void ) {
    if ( gtk_events_pending ( ) ) {
        gtk_main_iteration ( );
    };
}
#endif


/*
void ui_show_error_dialog ( char *error_message ) {
    GtkWidget *dialog = gtk_message_dialog_new ( NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, error_message );
    gtk_dialog_run ( GTK_DIALOG ( dialog ) );
    gtk_widget_destroy ( dialog );
    ui_iteration ( );
}
 */


#ifdef UI_USE_ERRORLOG


void ui_write_errorlog ( char *lvl, char *msg ) {
    FILE *fp;
    if ( NULL == ( fp = ui_utils_file_open ( UI_ERRORLOG_FILE, "a" ) ) ) {
        fprintf ( stderr, "%s():%d - '%s' - fopen error: %s", __func__, __LINE__, UI_ERRORLOG_FILE, strerror ( errno ) );
    } else {
        fprintf ( fp, "%s %s:\t%s\n", cfgmain_create_timestamp ( ), lvl, msg );
        fclose ( fp );
    };
}
#endif


/* modalni okno s [YES/NO] dialogem */
int ui_show_yesno_dialog ( char *format, ... ) {

    char *msg = NULL;
    va_list args;
    va_start ( args, format );

#if 0
    /* TODO: W32NAT nezna vasprintf - asi by bylo reseni pouzit vsnprintf (pokud jej tam existuje) */
#if W32NAT
    msg = "Unknown Error Message: your build is completed on host without vasprintf()\nCheck console for details.";
    vprintf ( format, args );
#else
    vasprintf ( &msg, format, args );
#endif
#else
    g_vasprintf ( &msg, format, args );
#endif    
    va_end ( args );

    GtkWidget *dialog = NULL;
    if ( g_ui_is_initialised == 1 ) {
        dialog = gtk_message_dialog_new ( ui_get_window ( "main_window" ), GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, msg );
    };

#if 0
#ifndef W32NAT
    char *msg_in_locale;
    msg_in_locale = g_locale_from_utf8 ( msg, -1, NULL, NULL, NULL );
    fprintf ( stderr, "\nUI_ERROR: %s\n", msg_in_locale );
    g_free ( msg_in_locale );
#endif
#endif

    free ( msg );

    gint result = GTK_RESPONSE_NO;

    if ( g_ui_is_initialised == 1 ) {
        result = gtk_dialog_run ( GTK_DIALOG ( dialog ) );
        gtk_widget_destroy ( dialog );
    };

    if ( result == GTK_RESPONSE_YES ) {
        return EXIT_SUCCESS;
    };

    return EXIT_FAILURE;
}


/* modalni okno s chybou */
void ui_show_error ( char *format, ... ) {

    char *msg = NULL;
    va_list args;
    va_start ( args, format );

#if 0
    /* TODO: W32NAT nezna vasprintf - asi by bylo reseni pouzit vsnprintf (pokud jej tam existuje) */
#if W32NAT
    msg = "Unknown Error Message: your build is completed on host without vasprintf()\nCheck console for details.";
    vprintf ( format, args );
#else
    vasprintf ( &msg, format, args );
#endif
#else
    g_vasprintf ( &msg, format, args );
#endif    
    va_end ( args );

    GtkWidget *dialog = NULL;
    if ( g_ui_is_initialised == 1 ) {
        dialog = gtk_message_dialog_new ( ui_get_window ( "main_window" ), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, msg );
    };

#if 0
#ifndef W32NAT
    char *msg_in_locale;
    msg_in_locale = g_locale_from_utf8 ( msg, -1, NULL, NULL, NULL );
    fprintf ( stderr, "\nUI_ERROR: %s\n", msg_in_locale );
    g_free ( msg_in_locale );
#endif
#endif

#ifdef UI_USE_ERRORLOG
    ui_write_errorlog ( "ERROR", msg );
#endif

    free ( msg );

    if ( g_ui_is_initialised == 1 ) {
        gtk_dialog_run ( GTK_DIALOG ( dialog ) );
        gtk_widget_destroy ( dialog );
    };
}


/* nemodalni okno s warningem */
void ui_show_warning ( char *format, ... ) {

    char *msg = NULL;
    va_list args;
    va_start ( args, format );

#if 0
    /* TODO: W32NAT nezna vasprintf  - asi by bylo reseni pouzit vsnprintf (pokud jej tam existuje) */
#if W32NAT
    msg = "Unknown Error Message: your build is completed on host without vasprintf()\nCheck console for details.";
    vprintf ( format, args );
#else
    vasprintf ( &msg, format, args );
#endif
#else
    g_vasprintf ( &msg, format, args );
#endif

    va_end ( args );

    GtkWidget *dialog = NULL;
    if ( g_ui_is_initialised == 1 ) {
        dialog = gtk_message_dialog_new ( ui_get_window ( "main_window" ), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, msg );
    };

#if 0
#ifndef W32NAT
    char *msg_in_locale;
    msg_in_locale = g_locale_from_utf8 ( msg, -1, NULL, NULL, NULL );
    fprintf ( stderr, "\nUI_WARNING: %s\n", msg_in_locale );
    g_free ( msg_in_locale );
#endif
#endif

#ifdef UI_USE_ERRORLOG
    ui_write_errorlog ( "WARNING", msg );
#endif

    free ( msg );

    if ( g_ui_is_initialised == 1 ) {
        g_signal_connect_swapped ( dialog, "response", G_CALLBACK ( gtk_widget_destroy ), dialog );
        gtk_widget_show ( dialog );
    };
}


void ui_hide_main_menu_window ( void ) {
    GtkWidget *main_window = ui_get_widget ( "main_window" );
    gtk_widget_hide ( main_window );
}


#ifdef UI_TOPMENU_IS_WINDOW


void ui_show_hide_main_menu_window ( void ) {
    /* V Linuxu se mi nepodarilo otevrit samotny popup bez okna :( */
    GtkWidget *main_window = ui_get_widget ( "main_window" );

    if ( gtk_widget_get_visible ( main_window ) ) {
        gtk_widget_hide ( main_window );

    } else {
        gtk_window_set_position ( GTK_WINDOW ( main_window ), GTK_WIN_POS_MOUSE );
        gtk_widget_show ( main_window );

#if WINDOWS
        GdkEventButton *evb = g_new0 ( GdkEventButton, 1 );
        evb->type = GDK_BUTTON_PRESS;
        evb->window = gtk_widget_get_window ( main_window );

        gint root_x = 0, root_y = 0;
        gtk_window_get_position ( GTK_WINDOW ( main_window ), &root_x, &root_y );
        evb->x_root = root_x;
        evb->y_root = root_y;
        evb->x = 0;
        evb->y = 0;

        GdkWindow *gdkw = gtk_widget_get_window ( main_window );
        GdkRectangle gdr = { 0, 10, 0, 0 };
        GtkWidget *menu_emulator_setup = ui_get_widget ( "menu_emulator_setup" );
        gtk_menu_popup_at_rect ( GTK_MENU ( menu_emulator_setup ), gdkw, &gdr, 0, 0, (GdkEvent*) evb );
#endif
        
        /*
                GtkWidget *menu_emulator_setup = ui_get_widget ( "menu_emulator_setup" );
                GdkEventButton *evb = g_new0 ( GdkEventButton, 1 );
                evb->type = GDK_BUTTON_PRESS;
                evb->window = gtk_widget_get_window ( GTK_WIDGET ( main_window ) );
                gtk_menu_popup_at_pointer ( GTK_MENU ( menu_emulator_setup ), (GdkEvent*) evb );
         */
        //g_free ( evb );
    };
}


#else


void ui_popup_main_menu ( void ) {
    GtkWidget *menu_emulator_setup = ui_get_widget ( "menu_emulator_setup" );
    gtk_menu_popup ( GTK_MENU ( menu_emulator_setup ), NULL, NULL, NULL, NULL, 0, 0 );

}
#endif


/*
 * 
 * Callbacky main menu
 * 
 */

G_MODULE_EXPORT void on_menu_emulator_setup_hide ( GtkWidget *menu, gpointer data ) {
    (void) menu;
    (void) data;

    ui_hide_main_menu_window ( );
}


G_MODULE_EXPORT void on_reset ( GtkMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

    mz800_reset ( );
}


G_MODULE_EXPORT void on_max_cpu_speed ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

    if ( FALSE == gtk_check_menu_item_get_active ( ui_get_check_menu_item ( "menuitem_max_cpu_speed" ) ) ) {
        mz800_switch_emulation_speed ( MZ800_EMULATION_SPEED_NORMAL );
    } else {
        mz800_switch_emulation_speed ( MZ800_EMULATION_SPEED_MAX );
    };
}


/* 
 * 0 - normal
 * 1 - max
 * 
 */
void ui_main_update_cpu_speed_menu ( unsigned state ) {
    LOCK_UICALLBACKS ( );
    if ( state ) {
        gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_max_cpu_speed" ), TRUE );
    } else {
        gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_max_cpu_speed" ), FALSE );
    };
    UNLOCK_UICALLBACKS ( );
}


G_MODULE_EXPORT void on_pause_emulation ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

    if ( FALSE == gtk_check_menu_item_get_active ( ui_get_check_menu_item ( "menuitem_pause_emulation" ) ) ) {
        mz800_pause_emulation ( 0 );
    } else {
        mz800_pause_emulation ( 1 );
    };
}


/* 
 * 0 - normal state
 * 1 - paused
 * 
 */
void ui_main_update_emulation_state ( unsigned state ) {
    LOCK_UICALLBACKS ( );
    if ( state ) {
        gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_pause_emulation" ), TRUE );
    } else {
        gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_pause_emulation" ), FALSE );
    };
    UNLOCK_UICALLBACKS ( );
}



#ifdef MZ800EMU_CFG_DEBUGGER_ENABLED


G_MODULE_EXPORT void on_open_debugger ( GtkMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif
    debugger_show_hide_main_window ( );
}


G_MODULE_EXPORT void on_menuitem_open_breakpoints_activate ( GtkMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif
    ui_breakpoints_show_hide_window ( );
}


G_MODULE_EXPORT void on_menuitem_open_membrowser_activate ( GtkMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif
    ui_membrowser_show_hide ( );
}


G_MODULE_EXPORT void on_menuitem_open_dissassembler_activate ( GtkMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif
    ui_dissassembler_show_window ( );
}


G_MODULE_EXPORT void on_menuitem_memext_map_settings_activate ( GtkMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif
    ui_dbg_memext_show ( );
}

#else


/* Tohle zde musi byt, aby se nevypisoval warning, ze nebyl nalezen callback */

G_MODULE_EXPORT void on_open_debugger ( GtkMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
}


G_MODULE_EXPORT void on_menuitem_open_breakpoints_activate ( GtkMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
}


G_MODULE_EXPORT void on_menuitem_open_membrowser_activate ( GtkMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
}


G_MODULE_EXPORT void on_menuitem_open_dissassembler_activate ( GtkMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
}


G_MODULE_EXPORT void on_menuitem_memext_map_settings_activate ( GtkMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
}
#endif



#ifdef WINDOWS


/* BUGFIX: ve WINDOWS (WIN32 i WIN64?) nefunguje spravne gtk_show_uri() */
void on_aboutdialog_activate_link_event ( GtkMenuItem *menuitem, gpointer user_data ) {
    GtkWidget *window = ui_get_widget ( "emulator_aboutdialog" );
    ShellExecute ( NULL, "open", gtk_about_dialog_get_website ( GTK_ABOUT_DIALOG ( window ) ), NULL, NULL, SW_SHOWNORMAL );
}
#endif


G_MODULE_EXPORT void on_about_menuitem_activate ( GtkMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif
    GtkWidget *window = ui_get_widget ( "emulator_aboutdialog" );
    static int initialised = 0;
    if ( initialised == 0 ) {

        unsigned version_length = 0;
        char *version_format = "Version: %s\n%s\nBuild: %s (%s)"; /* verze, build time, platform */

        version_length += strlen ( CFGMAIN_EMULATOR_VERSION_TEXT );
        version_length += strlen ( build_time_get ( ) );
        version_length += strlen ( build_time_get_revision_txt ( ) );
        version_length += strlen ( version_format ) + 1;

        gchar *version_txt = malloc ( version_length );
        sprintf ( version_txt, version_format, CFGMAIN_EMULATOR_VERSION_TEXT, build_time_get_revision_txt ( ), build_time_get ( ), CFGMAIN_PLATFORM );

        gtk_about_dialog_set_version ( GTK_ABOUT_DIALOG ( window ), version_txt );

        free ( version_txt );

        g_signal_connect ( GTK_DIALOG ( window ), "response", G_CALLBACK ( gtk_widget_hide ), window );
#ifdef WINDOWS
        g_signal_connect ( GTK_DIALOG ( window ), "activate-link", G_CALLBACK ( on_aboutdialog_activate_link_event ), NULL );
#endif
        initialised = 1;
    };
    gtk_widget_show ( window );
}


G_MODULE_EXPORT gboolean on_emulator_aboutdialog_delete_event ( GtkWidget *widget, GdkEvent *event, gpointer user_data ) {
    GtkWidget *window = ui_get_widget ( "emulator_aboutdialog" );
    gtk_widget_hide ( window );
    return TRUE;
}


void ui_main_update_rear_dip_switch_mz800_mode ( unsigned state ) {
    LOCK_UICALLBACKS ( );
    if ( state ) {
        gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_dip_switch_mz800_mode" ), TRUE );
    } else {
        gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_dip_switch_mz800_mode" ), FALSE );
    };
    UNLOCK_UICALLBACKS ( );
}


G_MODULE_EXPORT void on_menuitem_dip_switch_mz800_mode_toggled ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

    if ( FALSE == gtk_check_menu_item_get_active ( ui_get_check_menu_item ( "menuitem_dip_switch_mz800_mode" ) ) ) {
        mz800_rear_dip_switch_mz800_mode ( 0 );
    } else {
        mz800_rear_dip_switch_mz800_mode ( 1 );
    };

    printf ( "Rear dip switch - Mode: %s\n", ( !g_mz800.mz800_switch ) ? "MZ-700" : "MZ-800" );
}


void ui_main_update_rear_dip_switch_cmt_inverted_polarity ( unsigned state ) {
    LOCK_UICALLBACKS ( );
    if ( state ) {
        gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_dip_switch_cmt_inverted_polarity" ), TRUE );
    } else {
        gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_dip_switch_cmt_inverted_polarity" ), FALSE );
    };
    UNLOCK_UICALLBACKS ( );
}


G_MODULE_EXPORT void on_menuitem_dip_switch_cmt_inverted_polarity_toggled ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

    if ( FALSE == gtk_check_menu_item_get_active ( ui_get_check_menu_item ( "menuitem_dip_switch_cmt_inverted_polarity" ) ) ) {
        cmt_rear_dip_switch_cmt_inverted_polarity ( 0 );
    } else {
        cmt_rear_dip_switch_cmt_inverted_polarity ( 1 );
    };

    printf ( "Rear dip switch - CMT polarity: %s\n", ( !g_cmt.polarity ) ? "Normal" : "Inverted" );
}


void ui_main_update_hwcompatibility_mz700_pal_timing ( unsigned state ) {
    LOCK_UICALLBACKS ( );
    if ( state ) {
        gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_hwcompatibility_mz700_pal_timing" ), TRUE );
    } else {
        gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_hwcompatibility_mz700_pal_timing" ), FALSE );
    };
    UNLOCK_UICALLBACKS ( );
}


void ui_main_update_hwcompatibility_mz700_fixed_e008 ( unsigned state ) {
    LOCK_UICALLBACKS ( );
    if ( state ) {
        gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_hwcompatibility_mz700_fixed_e008" ), TRUE );
    } else {
        gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_hwcompatibility_mz700_fixed_e008" ), FALSE );
    };
    UNLOCK_UICALLBACKS ( );
}


G_MODULE_EXPORT void on_menuitem_hwcompatibility_mz700_pal_timing_toggled ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

    if ( FALSE == gtk_check_menu_item_get_active ( ui_get_check_menu_item ( "menuitem_hwcompatibility_mz700_pal_timing" ) ) ) {
        mz800_hwcompatibility_mz700_pal_timing ( 0 );
    } else {
        mz800_hwcompatibility_mz700_pal_timing ( 1 );
    };

    printf ( "HW compatibility - VRAM timing: %s\n", ( g_mz800.hwcompatibility_mz700_pal_timing == HWCOMPAT_MZ700_PAL_TIMING_YES ) ? "MZ-700 (PAL)" : "MZ-800" );
}


G_MODULE_EXPORT void on_menuitem_hwcompatibility_mz700_fixed_e008_toggled ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

    if ( FALSE == gtk_check_menu_item_get_active ( ui_get_check_menu_item ( "menuitem_hwcompatibility_mz700_fixed_e008" ) ) ) {
        mz800_hwcompatibility_mz700_fixed_e008 ( 0 );
    } else {
        mz800_hwcompatibility_mz700_fixed_e008 ( 1 );
    };

    printf ( "HW compatibility - MZ-700 regDMD at 0xE008 HW bugfix %s\n", ( g_mz800.hwcompatibility_mz700_fixed_e008 == HWCOMPAT_MZ700_FIXED_E008_YES ) ? "ENABLED" : "DISABLED" );
}


G_MODULE_EXPORT void on_menuitem_cmt_cpu_boost_toggled ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

    if ( FALSE == gtk_check_menu_item_get_active ( ui_get_check_menu_item ( "menuitem_cmt_cpu_boost" ) ) ) {
        cmt_cpu_boost_set ( CMT_CPU_BOOST_DISABLED );
    } else {
        cmt_cpu_boost_set ( CMT_CPU_BOOST_ENABLED );
    };
}


G_MODULE_EXPORT void on_menuitem_cmt_mzfsize_check_toggled ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

    if ( FALSE == gtk_check_menu_item_get_active ( ui_get_check_menu_item ( "menuitem_cmt_mzfsize_check" ) ) ) {
        cmt_mzfsize_check_set ( CMT_MZFSIZE_CHECK_DISABLED );
    } else {
        cmt_mzfsize_check_set ( CMT_MZFSIZE_CHECK_ENABLED );
    };
}


G_MODULE_EXPORT void on_menuitem_keyboard_disable_hotkeys_toggled ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

    if ( FALSE == gtk_check_menu_item_get_active ( ui_get_check_menu_item ( "menuitem_keyboard_disable_hotkeys" ) ) ) {
        ui_disable_hotkeys ( 0 );
    } else {
        ui_disable_hotkeys ( 1 );
    };
}


G_MODULE_EXPORT void on_menuitem_dsk_tool_activate ( GtkMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

    ui_dsk_tool_show_window ( );
}


G_MODULE_EXPORT void on_menuitem_joystick_setup_activate ( GtkMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

    ui_joy_show_window ( );
}


G_MODULE_EXPORT void on_menuitem_version_check_setup_activate ( GtkMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

    ui_version_check_show_setup_window ( );
}


G_MODULE_EXPORT void on_menuitem_version_check_now_activate ( GtkMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

    version_check_run_test ( TRUE );
}

#ifdef MZ800EMU_CFG_DEBUGGER_ENABLED


void ui_main_debugger_windows_refresh ( void ) {

    if ( gtk_widget_is_visible ( ui_get_widget ( "debugger_main_window" ) ) ) {
        ui_debugger_update_disassembled ( ui_debugger_dissassembled_get_first_addr ( ), -1 );
        ui_debugger_update_stack ( );
        ui_debugger_update_mmap ( );
    };
}

#endif
