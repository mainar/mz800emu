/* 
 * File:   ui_audio.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 26. dubna 2023, 21:38
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
#include <gtk-3.0/gtk/gtktogglebutton.h>
#include <gtk-3.0/gtk/gtkwidget.h>
#include <gtk-3.0/gtk/gtkscalebutton.h>

#include "ui_main.h"

#include "audio.h"


typedef struct st_UIAUDIO {
    st_UIWINPOS pos;
} st_UIAUDIO;

static st_UIAUDIO g_uiaudio;


void ui_audio_init ( void ) {
    ui_main_setpos ( &g_uiaudio.pos, -1, -1 );
}


static void ui_audio_volume_setup_changed ( void ) {
    GtkWidget *grid_channels = ui_get_widget ( "grid_volume_per_channel" );
    GtkWidget *per_channel = ui_get_widget ( "radiobutton_volume_per_channel" );
    gboolean per_channel_active = gtk_toggle_button_get_active ( GTK_TOGGLE_BUTTON ( per_channel ) );
    gtk_widget_set_sensitive ( grid_channels, per_channel_active );
}


static void ui_audio_volume_setup_window_show_hide ( void ) {
    GtkWidget *window = ui_get_widget ( "window_mz800emu_audio" );
    if ( gtk_widget_get_visible ( window ) ) {
        ui_main_win_get_pos ( GTK_WINDOW ( window ), &g_uiaudio.pos );
        ui_cmt_tape_window_hide ( );
        gtk_widget_hide ( window );
    } else {

        LOCK_UICALLBACKS ( );

        if ( g_audio.volume_setup == AUDIO_VOLUME_SETUP_ALL ) {
            GtkWidget *btn = ui_get_widget ( "radiobutton_volume_all_in_one" );
            gtk_toggle_button_set_active ( GTK_TOGGLE_BUTTON ( btn ), TRUE );
        } else {
            GtkWidget *btn = ui_get_widget ( "radiobutton_volume_per_channel" );
            gtk_toggle_button_set_active ( GTK_TOGGLE_BUTTON ( btn ), TRUE );
        };

        gtk_scale_button_set_value ( GTK_SCALE_BUTTON ( ui_get_widget ( "volumebutton_master" ) ), (double) g_audio.volume_master / 100 );
        gtk_scale_button_set_value ( GTK_SCALE_BUTTON ( ui_get_widget ( "volumebutton_8253" ) ), (double) g_audio.volume_8253 / 100 );
        gtk_scale_button_set_value ( GTK_SCALE_BUTTON ( ui_get_widget ( "volumebutton_psg0" ) ), (double) g_audio.volume_psg0 / 100 );
        gtk_scale_button_set_value ( GTK_SCALE_BUTTON ( ui_get_widget ( "volumebutton_psg1" ) ), (double) g_audio.volume_psg1 / 100 );
        gtk_scale_button_set_value ( GTK_SCALE_BUTTON ( ui_get_widget ( "volumebutton_psg2" ) ), (double) g_audio.volume_psg2 / 100 );
        gtk_scale_button_set_value ( GTK_SCALE_BUTTON ( ui_get_widget ( "volumebutton_psg3" ) ), (double) g_audio.volume_psg3 / 100 );

        UNLOCK_UICALLBACKS ( );

        ui_audio_volume_setup_changed ( );

        gtk_widget_show ( window );
        ui_main_win_move_to_pos ( GTK_WINDOW ( window ), &g_uiaudio.pos );
        ui_cmt_window_update ( );
    };
}


G_MODULE_EXPORT gboolean on_window_mz800emu_audio_key_press_event ( GtkWidget *widget, GdkEventKey *event, gpointer user_data ) {
    if ( event->keyval == GDK_KEY_Escape ) {
        ui_audio_volume_setup_window_show_hide ( );
        return TRUE;
    };
    return FALSE;
}


G_MODULE_EXPORT gboolean on_window_mz800emu_audio_delete_event ( GtkWidget *widget, GdkEvent *event, gpointer data ) {
    (void) widget;
    (void) event;
    (void) data;

    ui_audio_volume_setup_window_show_hide ( );
    return TRUE;
}


G_MODULE_EXPORT void on_menuitem_audio_volume_setup_activate ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif
    ui_audio_volume_setup_window_show_hide ( );
}


G_MODULE_EXPORT void on_radiobutton_volume_all_in_one_toggled ( GtkToggleButton *button, gpointer data ) {
    (void) button;
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

    if ( gtk_toggle_button_get_active ( button ) ) {
        audio_setup_type ( AUDIO_VOLUME_SETUP_ALL );
    };

    ui_audio_volume_setup_changed ( );
}


G_MODULE_EXPORT void on_radiobutton_volume_per_channel_toggled ( GtkToggleButton *button, gpointer data ) {
    (void) button;
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

    if ( gtk_toggle_button_get_active ( button ) ) {
        audio_setup_type ( AUDIO_VOLUME_SETUP_PER_CHANNEL );
    };

    ui_audio_volume_setup_changed ( );
}


G_MODULE_EXPORT void on_volumebutton_master_value_changed ( GtkScaleButton *button, double value, gpointer data ) {
    (void) button;
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

    audio_set_volume_master ( value * 100 );


    if ( g_audio.volume_setup == AUDIO_VOLUME_SETUP_PER_CHANNEL ) {
        LOCK_UICALLBACKS ( );
        gtk_scale_button_set_value ( GTK_SCALE_BUTTON ( ui_get_widget ( "volumebutton_8253" ) ), (double) g_audio.volume_8253 / 100 );
        gtk_scale_button_set_value ( GTK_SCALE_BUTTON ( ui_get_widget ( "volumebutton_psg0" ) ), (double) g_audio.volume_psg0 / 100 );
        gtk_scale_button_set_value ( GTK_SCALE_BUTTON ( ui_get_widget ( "volumebutton_psg1" ) ), (double) g_audio.volume_psg1 / 100 );
        gtk_scale_button_set_value ( GTK_SCALE_BUTTON ( ui_get_widget ( "volumebutton_psg2" ) ), (double) g_audio.volume_psg2 / 100 );
        gtk_scale_button_set_value ( GTK_SCALE_BUTTON ( ui_get_widget ( "volumebutton_psg3" ) ), (double) g_audio.volume_psg3 / 100 );
        UNLOCK_UICALLBACKS ( );
    };

}


G_MODULE_EXPORT void on_volumebutton_8253_value_changed ( GtkScaleButton *button, double value, gpointer data ) {
    (void) button;
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

    audio_set_volume_8253 ( value * 100 );
}


G_MODULE_EXPORT void on_volumebutton_psg0_value_changed ( GtkScaleButton *button, double value, gpointer data ) {
    (void) button;
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

    audio_set_volume_psg0 ( value * 100 );
}


G_MODULE_EXPORT void on_volumebutton_psg1_value_changed ( GtkScaleButton *button, double value, gpointer data ) {
    (void) button;
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

    audio_set_volume_psg1 ( value * 100 );
}


G_MODULE_EXPORT void on_volumebutton_psg2_value_changed ( GtkScaleButton *button, double value, gpointer data ) {
    (void) button;
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

    audio_set_volume_psg2 ( value * 100 );
}


G_MODULE_EXPORT void on_volumebutton_psg3_value_changed ( GtkScaleButton *button, double value, gpointer data ) {
    (void) button;
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

    audio_set_volume_psg3 ( value * 100 );
}
