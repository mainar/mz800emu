/* 
 * File:   ui_version_check.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 3. září 2018, 9:34
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

#include <stdlib.h>
#include <strings.h>

#include "ui_main.h"

#include "cfgmain.h"
#include "build_time.h"

#include "version_check/version_check.h"

#ifdef WINDOWS
#define UI_VERSIONCHECK_URL_OPEN    "start"
#else
#ifdef LINUX
#define UI_VERSIONCHECK_URL_OPEN    "xdg-open"
#else
#warning "Unknown platform!"
#endif
#endif


typedef struct st_UI_VERSION_CHECK_URL {
    char *tag;
    char *url;
} st_UI_VERSION_CHECK_URL;

const st_UI_VERSION_CHECK_URL g_ui_verurl[] = {
    { VERSION_CHECK_MAJOR_BRANCH, "https://sourceforge.net/projects/mz800emu/" },
    { "devel", "https://sourceforge.net/projects/mz800emu/" },
    { "win-snapshot", "https://www.ordoz.com/mz800emu/snapshot/" },
    { NULL, NULL }
};

static const char *g_ui_version_check_url = NULL;
static st_VERSION_BRANCH *g_ui_version_check_ignore_branch = NULL;


static const char* ui_version_check_get_url_by_tag ( char *tag ) {
    const char *ret = NULL;
    int i = 0;
    while ( g_ui_verurl[i].tag ) {
        if ( 0 == strcasecmp ( VERSION_CHECK_MAJOR_BRANCH, g_ui_verurl[i].tag ) ) {
            ret = g_ui_verurl[i].url;
        };
        if ( 0 == strcasecmp ( tag, g_ui_verurl[i].tag ) ) {
            return g_ui_verurl[i].url;
        };
        i++;
    };
    return ret;
}


G_MODULE_EXPORT void on_version_check_proxy_comboboxtext_changed ( GtkComboBox *widget, gpointer data ) {
    (void) widget;
    (void) data;

    int active = gtk_combo_box_get_active ( ui_get_combo_box ( "version_check_proxy_comboboxtext" ) );
    gboolean state = ( active == 2 ) ? TRUE : FALSE;
    if ( state ) {
        gtk_widget_show ( ui_get_widget ( "version_check_proxy_manual_entry" ) );
    } else {
        gtk_widget_hide ( ui_get_widget ( "version_check_proxy_manual_entry" ) );
    };
}


void ui_version_check_show_setup_window ( void ) {
    GtkWidget *window = ui_get_widget ( "version_check_setup_window" );
    if ( gtk_widget_is_visible ( window ) ) return;

    en_VERSION_CHECK_PERIOD period = version_check_get_period ( );

    if ( period == VERSION_CHECK_PERIOD_UNKNOWN ) period = VERSION_CHECK_DEFAULT_PERIOD;

    if ( period == VERSION_CHECK_PERIOD_DAILY ) {
        gtk_combo_box_set_active ( ui_get_combo_box ( "version_check_period_comboboxtext" ), 0 );
    } else if ( period == VERSION_CHECK_PERIOD_WEEKLY ) {
        gtk_combo_box_set_active ( ui_get_combo_box ( "version_check_period_comboboxtext" ), 1 );
    } else if ( period == VERSION_CHECK_PERIOD_MONTHLY ) {
        gtk_combo_box_set_active ( ui_get_combo_box ( "version_check_period_comboboxtext" ), 2 );
    } else if ( period == VERSION_CHECK_PERIOD_SOMETIMES ) {
        gtk_combo_box_set_active ( ui_get_combo_box ( "version_check_period_comboboxtext" ), 3 );
    } else if ( period == VERSION_CHECK_PERIOD_NEVER ) {
        gtk_combo_box_set_active ( ui_get_combo_box ( "version_check_period_comboboxtext" ), 4 );
    } else {
        fprintf ( stderr, "%s():%d - Unknown check period '%d'\n", __func__, __LINE__, period );
        gtk_combo_box_set_active ( ui_get_combo_box ( "version_check_period_comboboxtext" ), 2 );
    };


    en_VERSION_CHECK_PROXY use_proxy = version_check_get_proxy_settings ( );
    if ( use_proxy == VERSION_CHECK_PROXY_NONE ) {
        gtk_combo_box_set_active ( ui_get_combo_box ( "version_check_proxy_comboboxtext" ), 0 );
    } else if ( use_proxy == VERSION_CHECK_PROXY_SYSTEM ) {
        gtk_combo_box_set_active ( ui_get_combo_box ( "version_check_proxy_comboboxtext" ), 1 );
    } else if ( use_proxy == VERSION_CHECK_PROXY_MANUAL ) {
        gtk_combo_box_set_active ( ui_get_combo_box ( "version_check_proxy_comboboxtext" ), 2 );
    } else {
        fprintf ( stderr, "%s():%d - Unknown proxy settings '%d'\n", __func__, __LINE__, use_proxy );
        gtk_combo_box_set_active ( ui_get_combo_box ( "version_check_proxy_comboboxtext" ), 1 );
    };

    on_version_check_proxy_comboboxtext_changed ( NULL, NULL );

    gtk_entry_set_text ( ui_get_entry ( "version_check_proxy_manual_entry" ), version_check_get_proxy_server ( ) );

    gtk_widget_show ( window );
}


G_MODULE_EXPORT void on_version_check_ok_button_clicked ( GtkButton *button, gpointer data ) {
    (void) button;
    (void) data;

    GtkWidget *window = ui_get_widget ( "version_check_setup_window" );
    gtk_widget_hide ( window );
    en_VERSION_CHECK_PERIOD old_period = version_check_get_period ( );

    en_VERSION_CHECK_PERIOD new_period;

    int period_combo_state = gtk_combo_box_get_active ( ui_get_combo_box ( "version_check_period_comboboxtext" ) );
    switch ( period_combo_state ) {
        case 0:
            new_period = VERSION_CHECK_PERIOD_DAILY;
            break;
        case 1:
            new_period = VERSION_CHECK_PERIOD_WEEKLY;
            break;
        case 2:
            new_period = VERSION_CHECK_PERIOD_MONTHLY;
            break;
        case 3:
            new_period = VERSION_CHECK_PERIOD_SOMETIMES;
            break;
        case 4:
            new_period = VERSION_CHECK_PERIOD_NEVER;
            break;
        default:
            fprintf ( stderr, "%s():%d - Unknown check period option '%d'\n", __func__, __LINE__, period_combo_state );
            new_period = VERSION_CHECK_PERIOD_MONTHLY;
            break;
    };

    en_VERSION_CHECK_PROXY new_proxy_settings = VERSION_CHECK_PROXY_NONE;

    int proxy_combo_state = gtk_combo_box_get_active ( ui_get_combo_box ( "version_check_proxy_comboboxtext" ) );
    switch ( proxy_combo_state ) {
        case 0:
            new_proxy_settings = VERSION_CHECK_PROXY_NONE;
            break;
        case 1:
            new_proxy_settings = VERSION_CHECK_PROXY_SYSTEM;
            break;
        case 2:
            new_proxy_settings = VERSION_CHECK_PROXY_MANUAL;
            break;
        default:
            fprintf ( stderr, "%s():%d - Unknown proxy_server option '%d'\n", __func__, __LINE__, proxy_combo_state );
            new_period = VERSION_CHECK_PERIOD_MONTHLY;
            break;
    };

    version_check_set_proxy_settings ( new_proxy_settings );

    version_check_set_proxy_server ( gtk_entry_get_text ( ui_get_entry ( "version_check_proxy_manual_entry" ) ) );

    if ( old_period == new_period ) return;

    version_check_set_period ( new_period );

    if ( new_period > VERSION_CHECK_PERIOD_NEVER ) {
        uint32_t last_check = version_check_get_last_check ( );
        uint32_t unix_days = version_check_get_unix_days ( );
        if ( ( last_check == 0 ) || ( ( unix_days - last_check ) >= new_period ) ) {
            version_check_run_test ( FALSE );
        };
    };
}


void ui_version_check_report_running_version ( void ) {
    gtk_label_set_text ( ui_get_label ( "version_check_report_running_tag_label" ), CFGMAIN_EMULATOR_VERSION_TAG );
    gtk_label_set_markup ( ui_get_label ( "version_check_report_running_version_label" ), CFGMAIN_EMULATOR_VERSION_NUM_STRING );

    uint32_t revision = build_time_get_revision_int ( );
    GString *rev = g_string_sized_new ( 0 );
    g_string_append_printf ( rev, "%d", revision );
    gtk_label_set_markup ( ui_get_label ( "version_check_report_running_revision_label" ), rev->str );
    g_string_free ( rev, TRUE );

    GString *date = g_string_new ( 0 );
    date = g_string_insert_len ( date, 0, build_time_get ( ), 10 );
    gtk_label_set_text ( ui_get_label ( "version_check_report_running_date_label" ), date->str );
    g_string_free ( date, TRUE );
}


void ui_version_check_show_report_window ( st_VERSION_BRANCH *branch ) {

    ui_version_check_report_running_version ( );

    GtkLabel *main_label = ui_get_label ( "version_check_report_main_label" );
    GtkWidget *botom_label = ui_get_widget ( "version_check_report_botom_label" );
    GtkWidget *open_button = ui_get_widget ( "version_check_report_open_button" );
    GtkWidget *ignore_button = ui_get_widget ( "version_check_report_ignore_button" );

    uint32_t version = cfgmain_get_version_uint32 ( );
    uint32_t revision = build_time_get_revision_int ( );

    gboolean new_version_available = ( ( branch->version > version ) || ( branch->revision > revision ) );

    if ( new_version_available ) {
        gtk_label_set_text ( main_label, "A new version of the mz800emu software is available" );
    } else {
        gtk_label_set_text ( main_label, "Your mz800emu software version is up to date" );
    };

    gtk_label_set_text ( ui_get_label ( "version_check_report_tag_label" ), branch->tag->str );

    GString *ver = version_check_create_version_string ( branch->version );
    if ( branch->version > version ) {
        ver = g_string_prepend ( ver, "<b>" );
        ver = g_string_append ( ver, "</b>" );
    };
    gtk_label_set_markup ( ui_get_label ( "version_check_report_version_label" ), ver->str );
    g_string_free ( ver, TRUE );

    GString *rev = g_string_sized_new ( 0 );
    if ( branch->revision > revision ) {
        g_string_append_printf ( rev, "<b>%d</b>", branch->revision );
    } else {
        g_string_append_printf ( rev, "%d", branch->revision );
    };
    gtk_label_set_markup ( ui_get_label ( "version_check_report_revision_label" ), rev->str );
    g_string_free ( rev, TRUE );

    gtk_label_set_text ( ui_get_label ( "version_check_report_date_label" ), branch->date->str );

    g_ui_version_check_url = ui_version_check_get_url_by_tag ( branch->tag->str );
    GtkLabel *url_label = ui_get_label ( "version_check_report_url_label" );

#ifdef UI_VERSIONCHECK_URL_OPEN
    if ( g_ui_version_check_url ) {
        gtk_label_set_text ( url_label, g_ui_version_check_url );
        gtk_widget_show ( open_button );
    } else {
        gtk_label_set_text ( url_label, "" );
        gtk_widget_hide ( open_button );
    };
#else
    gtk_widget_hide ( open_button );
#endif

    if ( new_version_available ) {

        if ( branch->msg ) {
            char *msg = g_markup_escape_text ( branch->msg->str, -1 );
            gtk_label_set_markup ( GTK_LABEL ( botom_label ), msg );
            g_free ( msg );
            gtk_widget_show ( botom_label );
        } else {
            gtk_widget_hide ( botom_label );
        };

        version_check_branch_free ( g_ui_version_check_ignore_branch );
        g_ui_version_check_ignore_branch = version_check_copy_branch ( branch );

        gtk_widget_show ( ignore_button );
    } else {
        gtk_widget_hide ( ignore_button );
        gtk_widget_hide ( botom_label );
    };


    GtkWidget *window = ui_get_widget ( "version_check_report_window" );
    gtk_widget_show ( window );
}


static void ui_version_check_hide_report_window ( void ) {
    GtkWidget *window = ui_get_widget ( "version_check_report_window" );
    gtk_widget_hide ( window );
    version_check_update_last_check ( );
}


G_MODULE_EXPORT void on_version_check_report_close_button_clicked ( GtkButton *button, gpointer data ) {
    (void) button;
    (void) data;

    ui_version_check_hide_report_window ( );
}


G_MODULE_EXPORT void on_version_check_report_ignore_button_clicked ( GtkButton *button, gpointer data ) {
    (void) button;
    (void) data;

    st_VERSION_BRANCH *branch = g_ui_version_check_ignore_branch;

    if ( branch ) {
        version_check_set_ignored ( branch->tag->str, branch->version, branch->revision );
    };

    version_check_branch_free ( g_ui_version_check_ignore_branch );
    g_ui_version_check_ignore_branch = NULL;

    ui_version_check_hide_report_window ( );
}


G_MODULE_EXPORT void on_version_check_report_open_button_clicked ( GtkButton *button, gpointer data ) {
    (void) button;
    (void) data;

#ifdef UI_VERSIONCHECK_URL_OPEN
    if ( g_ui_version_check_url ) {
        printf ( "VERSION_CHECK Redirecting to open %s\n", g_ui_version_check_url );
        GString *openurl = g_string_sized_new ( 0 );
        g_string_append_printf ( openurl, "%s %s", UI_VERSIONCHECK_URL_OPEN, g_ui_version_check_url );
        system ( openurl->str );
        g_string_free ( openurl, TRUE );
    };
#endif
}


G_MODULE_EXPORT void on_version_check_report_setup_button_clicked ( GtkButton *button, gpointer data ) {
    (void) button;
    (void) data;

    ui_version_check_show_setup_window ( );
}
