/* 
 * File:   ui_cmt.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 11. srpna 2015, 19:40
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
#include <math.h>
#include <string.h>
#include <strings.h>
#include <gtk-3.0/gtk/gtktypes.h>

#include "ui_main.h"
#include "ui_cmt.h"

#include "cmt/cmt.h"
#include "cmt/cmthack.h"
#include "cmt/cmtext.h"
#include "cmt/cmtext_block.h"
#include "cmt/cmtext_container.h"
#include "cmt/cmt_mzf.h"

#include "libs/mzf/mzf.h"
#include "libs/mzf/mzf_tools.h"

#include "libs/mztape/cmtspeed.h"
#include "cmt/cmt_tap.h"
#include "memory/rom.h"
#include "gdg/gdg.h"
#include "ui_utils.h"
#include "ui_file_chooser.h"
#include "ui_cmt_tape.h"


typedef struct st_UICMT {
    st_UIWINPOS pos;
    en_UICMT_SHOW show_time;
    int last_show_stream_type; // -2 not initialised, -1 "--", 0...
    int last_show_stream_rate; // -2 not initialised, -1 "--", 0...
    int32_t last_show_stream_size; // -2 not initialised, -1 "--", 0...
} st_UICMT;

static st_UICMT g_uicmt;


static void ui_cmt_show_time_changed ( void ) {
    if ( g_uicmt.show_time == UICMT_SHOW_REMAINING_TIME ) {
        gtk_label_set_text ( ui_get_label ( "cmt_time_info_label" ), "Remaining time:" );
    } else {
        gtk_label_set_text ( ui_get_label ( "cmt_time_info_label" ), "Play time:" );
    };
    ui_cmt_update_player ( );
}


void ui_cmt_set_show_time ( en_UICMT_SHOW show_time ) {
    g_uicmt.show_time = show_time;
    ui_cmt_show_time_changed ( );
}


void ui_cmt_init ( void ) {
    ui_main_setpos ( &g_uicmt.pos, -1, -1 );
    g_uicmt.last_show_stream_type = -2;
    g_uicmt.last_show_stream_rate = -2;
    g_uicmt.last_show_stream_size = -2;
    ui_cmt_set_show_time ( UICMT_SHOW_REMAINING_TIME );

    GtkWidget *statusbar = ui_get_widget ( "cmt_statusbar" );
    guint id = gtk_statusbar_get_context_id ( GTK_STATUSBAR ( statusbar ), "info" );
    gtk_statusbar_push ( GTK_STATUSBAR ( statusbar ), id, "CMT is empty..." );
    //gtk_statusbar_pop ( GTK_STATUSBAR ( statusbar ), id );
}


G_MODULE_EXPORT gboolean on_cmt_window_key_press_event ( GtkWidget *widget, GdkEventKey *event, gpointer user_data ) {
    if ( event->keyval == GDK_KEY_Escape ) {
        ui_cmt_window_show_hide ( );
        return TRUE;
    };
    return FALSE;
}


G_MODULE_EXPORT void on_menuitem_cmt_hack ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif
    if ( gtk_check_menu_item_get_active ( ui_get_check_menu_item ( "menuitem_cmt_hack" ) ) ) {
        cmthack_load_rom_patch ( 1 );
    } else {
        cmthack_load_rom_patch ( 0 );
    };
}


G_MODULE_EXPORT void on_menuitem_cmt_hack_fix_fname_terminator_toggled ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif
    if ( gtk_check_menu_item_get_active ( ui_get_check_menu_item ( "menuitem_cmt_hack_fix_fname_terminator" ) ) ) {
        g_cmthack.fix_fname_terminator = 1;
    } else {
        g_cmthack.fix_fname_terminator = 0;
    };
}


G_MODULE_EXPORT void on_menuitem_cmt_speed_1_1_toggled ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

    if ( gtk_check_menu_item_get_active ( menuitem ) ) {
        LOCK_UICALLBACKS ( );
        cmt_change_speed ( CMTSPEED_1_1 );
        UNLOCK_UICALLBACKS ( );
    };
}


G_MODULE_EXPORT void on_menuitem_cmt_speed_2_1_toggled ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

    if ( gtk_check_menu_item_get_active ( menuitem ) ) {
        LOCK_UICALLBACKS ( );
        cmt_change_speed ( CMTSPEED_2_1 );
        UNLOCK_UICALLBACKS ( );
    };
}


G_MODULE_EXPORT void on_menuitem_cmt_speed_2_1_cpm_toggled ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

    if ( gtk_check_menu_item_get_active ( menuitem ) ) {
        LOCK_UICALLBACKS ( );
        cmt_change_speed ( CMTSPEED_2_1_CPM );
        UNLOCK_UICALLBACKS ( );
    };
}


G_MODULE_EXPORT void on_menuitem_cmt_speed_7_3_toggled ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

    if ( gtk_check_menu_item_get_active ( menuitem ) ) {
        LOCK_UICALLBACKS ( );
        cmt_change_speed ( CMTSPEED_7_3 );
        UNLOCK_UICALLBACKS ( );
    };
}


G_MODULE_EXPORT void on_menuitem_cmt_speed_8_3_toggled ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

    if ( gtk_check_menu_item_get_active ( menuitem ) ) {
        LOCK_UICALLBACKS ( );
        cmt_change_speed ( CMTSPEED_8_3 );
        UNLOCK_UICALLBACKS ( );
    };
}


G_MODULE_EXPORT void on_menuitem_cmt_speed_3_1_toggled ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

    if ( gtk_check_menu_item_get_active ( menuitem ) ) {
        LOCK_UICALLBACKS ( );
        cmt_change_speed ( CMTSPEED_3_1 );
        UNLOCK_UICALLBACKS ( );
    };
}


G_MODULE_EXPORT void on_menuitem_cmt_open_and_play ( GtkMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif

    if ( EXIT_SUCCESS == cmt_open ( ) ) {
        cmt_play ( );
    };
}


void ui_cmt_window_show_hide ( void ) {
    GtkWidget *window = ui_get_widget ( "cmt_window" );
    if ( gtk_widget_get_visible ( window ) ) {
        ui_main_win_get_pos ( GTK_WINDOW ( window ), &g_uicmt.pos );
        ui_cmt_tape_window_hide ( );
        gtk_widget_hide ( window );
    } else {
        gtk_widget_show ( window );
        ui_main_win_move_to_pos ( GTK_WINDOW ( window ), &g_uicmt.pos );
        ui_cmt_window_update ( );
    };
}


G_MODULE_EXPORT void on_menuitem_cmt_show_window ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif
    ui_cmt_window_show_hide ( );
}


void ui_cmt_hack_menu_update ( void ) {
    LOCK_UICALLBACKS ( );
    if ( TEST_CMTHACK_INSTALLED ) {
        ui_cmt_tape_window_hide ( );

        gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_cmt_hack" ), TRUE );
        gtk_widget_set_sensitive ( ui_get_widget ( "menuitem_cmthack_options" ), TRUE );

        gtk_widget_hide ( ui_get_widget ( "vbox_cmt_ready" ) );
        gtk_widget_show ( ui_get_widget ( "box_cmt_not_available" ) );
    } else {
        gtk_widget_set_sensitive ( ui_get_widget ( "menuitem_cmthack_options" ), FALSE );
        gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_cmt_hack" ), FALSE );

        gtk_widget_show ( ui_get_widget ( "vbox_cmt_ready" ) );
        gtk_widget_hide ( ui_get_widget ( "box_cmt_not_available" ) );
    };
    if ( g_cmthack.fix_fname_terminator ) {
        gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_cmt_hack_fix_fname_terminator" ), TRUE );
    } else {
        gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_cmt_hack_fix_fname_terminator" ), FALSE );
    };
    UNLOCK_UICALLBACKS ( );
    gboolean cmth_sensitive = ( TEST_ROM_WILLY ) ? FALSE : TRUE;
    if ( TEST_ROM_USER_DEFINED ) {
        cmth_sensitive = ( g_rom.user_defined_cmthack_type == ROM_CMTHACK_DISABLED ) ? FALSE : TRUE;
    };
    gtk_widget_set_sensitive ( ui_get_widget ( "menuitem_cmt_hack" ), cmth_sensitive );
}


void ui_cmt_cpu_boost_menu_update ( void ) {
    LOCK_UICALLBACKS ( );
    gboolean cpu_boost_state = ( g_cmt.cpu_boost ) ? TRUE : FALSE;
    gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_cmt_cpu_boost" ), cpu_boost_state );
    gtk_toggle_button_set_active ( GTK_TOGGLE_BUTTON ( ui_get_widget ( "cmt_cpu_boost_checkbutton" ) ), cpu_boost_state );
    UNLOCK_UICALLBACKS ( );
}


void ui_cmt_mzfsize_check_menu_update ( void ) {
    LOCK_UICALLBACKS ( );
    gboolean mzfsize_check_state = ( g_cmt.mzfsize_check ) ? TRUE : FALSE;
    gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_cmt_mzfsize_check" ), mzfsize_check_state );
    gtk_toggle_button_set_active ( GTK_TOGGLE_BUTTON ( ui_get_widget ( "cmt_fix_mzfsize_mzfsize_check_enabled_checkbutton" ) ), mzfsize_check_state );
    UNLOCK_UICALLBACKS ( );
}


void ui_cmt_speed_menu_update ( void ) {

    LOCK_UICALLBACKS ( );

    if ( !TEST_CMT_STOP ) {
        gtk_widget_set_sensitive ( ui_get_widget ( "menuitem_cmt_speed" ), FALSE );
    } else {
        gtk_widget_set_sensitive ( ui_get_widget ( "menuitem_cmt_speed" ), TRUE );
    };

    switch ( g_cmt.mz_cmtspeed ) {
        case CMTSPEED_1_1:
            gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_cmt_speed_1_1" ), TRUE );
            break;
        case CMTSPEED_2_1:
            gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_cmt_speed_2_1" ), TRUE );
            break;
        case CMTSPEED_2_1_CPM:
            gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_cmt_speed_2_1_cpm" ), TRUE );
            break;
        case CMTSPEED_7_3:
            gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_cmt_speed_7_3" ), TRUE );
            break;
        case CMTSPEED_8_3:
            gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_cmt_speed_8_3" ), TRUE );
            break;
        case CMTSPEED_3_1:
            gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "menuitem_cmt_speed_3_1" ), TRUE );
            break;
        case CMTSPEED_NONE:
        case CMTSPEED_3_2:
        case CMTSPEED_9_7:
        case CMTSPEED_25_14:
            fprintf ( stderr, "%s():%d - Unsoported mz_cmtspeed (%d)\n", __func__, __LINE__, g_cmt.mz_cmtspeed );
    };

    UNLOCK_UICALLBACKS ( );
}


void ui_cmt_polarity_menu_update ( void ) {
    if ( !TEST_CMT_STOP ) {
        gtk_widget_set_sensitive ( ui_get_widget ( "menuitem_dip_switch_cmt_inverted_polarity" ), FALSE );
    } else {
        gtk_widget_set_sensitive ( ui_get_widget ( "menuitem_dip_switch_cmt_inverted_polarity" ), TRUE );
    };
}


G_MODULE_EXPORT gboolean on_cmt_window_delete_event ( GtkWidget *widget, GdkEvent *event, gpointer data ) {
    (void) widget;
    (void) event;
    (void) data;

    ui_cmt_window_show_hide ( );
    return TRUE;
}


G_MODULE_EXPORT void on_cmt_hide_button_clicked ( GtkButton *button, gpointer data ) {
    (void) button;
    (void) data;

    ui_cmt_window_show_hide ( );
}


G_MODULE_EXPORT void on_cmt_stop_button_clicked ( GtkButton *button, gpointer data ) {
    (void) button;
    (void) data;

    if ( TEST_CMT_RECORD ) {
        st_CMTEXT_BLOCK *block = cmtext_get_block ( g_cmt.ext );
        uint32_t size = cmtext_block_get_size ( block );
        if ( !size ) {
            cmt_eject ( );
        } else {
#if 1
            st_CMTEXT_CONTAINER *container = cmtext_get_container ( g_cmt.ext );
            const char *fpath = cmtext_container_get_filepath ( container );
            int len = strlen ( fpath ) + 1;
            char *filepath = ui_utils_mem_alloc0 ( len );
            strncpy ( filepath, fpath, len );
            cmt_eject ( );
            cmt_open_file_by_extension ( filepath );
            ui_utils_mem_free ( filepath );
#else
            g_cmt.ext->info->type = CMTEXT_TYPE_PLAYABLE;
            cmt_stop ( );
#endif
        };
    } else {
        cmt_stop ( );
    };
}


G_MODULE_EXPORT void on_cmt_record_togglebutton_toggled ( GtkToggleButton *togglebutton, gpointer data ) {
    (void) togglebutton;
    (void) data;

    if ( ( gtk_toggle_button_get_active ( togglebutton ) ) && ( TEST_CMT_STOP ) ) {
        cmt_record ( );
    };
}


G_MODULE_EXPORT void on_cmt_play_togglebutton_toggled ( GtkToggleButton *togglebutton, gpointer data ) {
    (void) togglebutton;
    (void) data;

    if ( ( gtk_toggle_button_get_active ( togglebutton ) ) && ( TEST_CMT_STOP ) ) {
        cmt_play ( );
    };
}


G_MODULE_EXPORT void on_cmt_pause_togglebutton_toggled ( GtkToggleButton *togglebutton, gpointer data ) {
    (void) togglebutton;
    (void) data;

    cmt_pause ( gtk_toggle_button_get_active ( togglebutton ) );
}


G_MODULE_EXPORT void on_cmt_open_button_clicked ( GtkButton *button, gpointer data ) {
    (void) button;
    (void) data;

    cmt_open ( );
}


G_MODULE_EXPORT void on_cmt_eject_button_clicked ( GtkButton *button, gpointer data ) {
    (void) button;
    (void) data;

    cmt_eject ( );
}


G_MODULE_EXPORT void on_cmt_previous_button_clicked ( GtkButton *button, gpointer data ) {
    (void) button;
    (void) data;

    assert ( g_cmt.ext->container->cb_previous_block );
    if ( !g_cmt.ext->container->cb_previous_block ) return;

    gboolean fl_play = TEST_CMT_PLAY;
    gboolean fl_paused = TEST_CMT_PAUSED;
    if ( fl_play ) cmt_stop ( );

    if ( EXIT_FAILURE == g_cmt.ext->container->cb_previous_block ( ) ) return;

    if ( fl_play ) {
        if ( fl_paused ) {
            cmt_play_paused ( );
        } else {
            cmt_play ( );
        };
    } else {
        ui_cmt_window_update ( );
    };
}


G_MODULE_EXPORT void on_cmt_next_button_clicked ( GtkButton *button, gpointer data ) {
    (void) button;
    (void) data;

    assert ( g_cmt.ext->container->cb_next_block );
    if ( !g_cmt.ext->container->cb_next_block ) return;

    gboolean fl_play = TEST_CMT_PLAY;
    gboolean fl_paused = TEST_CMT_PAUSED;
    if ( fl_play ) cmt_stop ( );

    if ( EXIT_FAILURE == g_cmt.ext->container->cb_next_block ( ) ) return;

    if ( fl_play ) {
        if ( fl_paused ) {
            cmt_play_paused ( );
        } else {
            cmt_play ( );
        };
    } else {
        ui_cmt_window_update ( );
    };
}


G_MODULE_EXPORT void on_cmt_rewind_button_clicked ( GtkButton *button, gpointer data ) {
    (void) button;
    (void) data;

    printf ( "%s()\n", __func__ );
}


G_MODULE_EXPORT void on_cmt_forward_button_clicked ( GtkButton *button, gpointer data ) {
    (void) button;
    (void) data;

    printf ( "%s()\n", __func__ );
}


G_MODULE_EXPORT void on_cmt_speed_comboboxtext_changed ( GtkComboBox *combobox, gpointer data ) {
    //(void) combobox;
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

    int mztape_speed_id = gtk_combo_box_get_active ( combobox );
    LOCK_UICALLBACKS ( );
    cmt_change_speed ( g_mztape_speed[mztape_speed_id] );
    UNLOCK_UICALLBACKS ( );
}


static void ui_cmt_set_stream_info_labels_none ( void ) {
    if ( g_uicmt.last_show_stream_type != -1 ) gtk_label_set_text ( ui_get_label ( "cmt_stream_source_label" ), "--" );
    if ( g_uicmt.last_show_stream_rate != -1 ) gtk_label_set_text ( ui_get_label ( "cmt_stream_rate_label" ), "--" );
    if ( g_uicmt.last_show_stream_size != -1 ) gtk_label_set_text ( ui_get_label ( "cmt_stream_size_label" ), "--" );
    g_uicmt.last_show_stream_type = -1;
    g_uicmt.last_show_stream_rate = -1;
    g_uicmt.last_show_stream_size = -1;
}


static void ui_cmt_set_stream_info_labels ( st_CMTEXT_BLOCK *block ) {

    char buff [ 100 ];

    if ( !block ) {
        ui_cmt_set_stream_info_labels_none ( );
        return;
    } else {
        en_CMTEXT_BLOCK_TYPE block_type = cmtext_block_get_type ( g_cmt.ext->block );

        if ( ( block_type != CMTEXT_BLOCK_TYPE_WAV ) || ( !cmtext_block_get_stream ( block ) ) ) {
            ui_cmt_set_stream_info_labels_none ( );
            return;
        } else {
            en_CMT_STREAM_TYPE stream_type = cmtext_block_get_stream_type ( block );
            if ( stream_type == CMT_STREAM_TYPE_BITSTREAM ) {
                if ( g_uicmt.last_show_stream_type != stream_type ) gtk_label_set_text ( ui_get_label ( "cmt_stream_source_label" ), "bitstream" );
            } else if ( stream_type == CMT_STREAM_TYPE_VSTREAM ) {
                if ( g_uicmt.last_show_stream_type != stream_type ) gtk_label_set_text ( ui_get_label ( "cmt_stream_source_label" ), "vstream" );
            } else {
                fprintf ( stderr, "%s():%d - Unknown cmt stream type '%d'\n", __func__, __LINE__, stream_type );
            };
            g_uicmt.last_show_stream_type = stream_type;
        };
    };

    uint32_t rate = cmtext_block_get_rate ( block );
    if ( rate != g_uicmt.last_show_stream_rate ) {
        if ( rate < 1000000 ) {
            snprintf ( buff, sizeof (buff ), "%0.2f kHz", ( (float) rate / 1000 ) );
        } else {
            snprintf ( buff, sizeof (buff ), "%0.2f MHz", ( (float) rate / 1000000 ) );
        };
        gtk_label_set_text ( ui_get_label ( "cmt_stream_rate_label" ), buff );
        g_uicmt.last_show_stream_rate = rate;
    };

    uint32_t size = cmtext_block_get_size ( block );

    if ( size != g_uicmt.last_show_stream_size ) {
        if ( !size ) {
            snprintf ( buff, sizeof (buff ), "--" );
        } else if ( size < 1024 ) {
            snprintf ( buff, sizeof (buff ), "%d B", size );
        } else if ( size < ( 1024 * 1024 ) ) {
            snprintf ( buff, sizeof (buff ), "%0.2f kB", ( (float) size / 1024 ) );
        } else {
            snprintf ( buff, sizeof (buff ), "%0.2f MB", ( (float) size / ( 1024 * 1024 ) ) );
        };
        gtk_label_set_text ( ui_get_label ( "cmt_stream_size_label" ), buff );
        g_uicmt.last_show_stream_size = size;
    };
}


void ui_cmt_window_update ( void ) {

    GtkWidget *window = ui_get_widget ( "cmt_window" );
    if ( !gtk_widget_get_visible ( window ) ) {
        ui_cmt_speed_menu_update ( );
        ui_cmt_polarity_menu_update ( );
        return;
    };

    char buff [ 100 ];

    int total_blocks = 0;
    int play_block = 0;
    st_CMTEXT_CONTAINER *container = NULL;

    if ( TEST_CMT_FILLED ) {
        container = cmtext_get_container ( g_cmt.ext );
        total_blocks = cmtext_container_get_count_blocks ( container );
        play_block = cmtext_block_get_block_id ( g_cmt.ext->block ) + 1;
    };

    // Tape info
    if ( total_blocks == 0 ) {
        gtk_label_set_text ( ui_get_label ( "cmt_play_block_label" ), "--" );
        gtk_label_set_text ( ui_get_label ( "cmt_total_blocks_label" ), "--" );
    } else {
        snprintf ( buff, sizeof (buff ), "%02d", play_block );
        gtk_label_set_text ( ui_get_label ( "cmt_play_block_label" ), buff );
        snprintf ( buff, sizeof (buff ), "%02d", total_blocks );
        gtk_label_set_text ( ui_get_label ( "cmt_total_blocks_label" ), buff );
    };


    gtk_label_set_text ( ui_get_label ( "cmt_filetype_label" ), "--" );
    gtk_label_set_text ( ui_get_label ( "cmt_filename_label" ), "--" );
    gtk_label_set_text ( ui_get_label ( "cmt_fsize_label" ), "--" );
    gtk_label_set_text ( ui_get_label ( "cmt_fexec_label" ), "--" );
    gtk_label_set_text ( ui_get_label ( "cmt_fstart_label" ), "--" );
    gtk_label_set_text ( ui_get_label ( "cmt_file_speed_label" ), "--" );
    //gtk_label_set_text ( ui_get_label ( "cmt_stream_source_label" ), "--" );
    //gtk_label_set_text ( ui_get_label ( "cmt_stream_rate_label" ), "--" );
    //gtk_label_set_text ( ui_get_label ( "cmt_stream_size_label" ), "--" );
    ui_cmt_set_stream_info_labels ( NULL );

    if ( TEST_CMT_FILLED ) {
        if ( ( EXIT_SUCCESS == cmtext_is_playable ( g_cmt.ext ) ) && ( g_cmt.playsts != CMTEXT_BLOCK_PLAYSTS_PAUSE ) ) {


            uint16_t file_bdspeed = 0;
            st_MZF_HEADER *mzfhdr = NULL;
            st_CMTEXT_TAPE_ITEM_TAPHDR *taphdr = NULL;
            st_CMTEXT_TAPE_ITEM_TAPDATA *tapdata = NULL;

            switch ( cmtext_block_get_type ( g_cmt.ext->block ) ) {
                case CMTEXT_BLOCK_TYPE_MZF:
                    file_bdspeed = ( !g_cmt.ext->block->cb_get_bdspeed ) ? 0 : g_cmt.ext->block->cb_get_bdspeed ( g_cmt.ext );
                    mzfhdr = cmtmzf_block_get_spec_mzfheader ( g_cmt.ext->block );
                    g_assert ( mzfhdr != NULL );

                    snprintf ( buff, sizeof (buff ), "0x%02x", mzfhdr->ftype );
                    gtk_label_set_text ( ui_get_label ( "cmt_filetype_label" ), buff );

                    mzf_tools_get_fname ( mzfhdr, (char*) &buff );
                    gtk_label_set_text ( ui_get_label ( "cmt_filename_label" ), buff );

                    snprintf ( buff, sizeof (buff ), "0x%04x", mzfhdr->fsize );
                    gtk_label_set_text ( ui_get_label ( "cmt_fsize_label" ), buff );

                    snprintf ( buff, sizeof (buff ), "0x%04x", mzfhdr->fexec );
                    gtk_label_set_text ( ui_get_label ( "cmt_fexec_label" ), buff );

                    snprintf ( buff, sizeof (buff ), "0x%04x", mzfhdr->fstrt );
                    gtk_label_set_text ( ui_get_label ( "cmt_fstart_label" ), buff );

                    snprintf ( buff, sizeof (buff ), "%d Bd", file_bdspeed );
                    gtk_label_set_text ( ui_get_label ( "cmt_file_speed_label" ), buff );
                    break;

                case CMTEXT_BLOCK_TYPE_WAV:
                    gtk_label_set_text ( ui_get_label ( "cmt_filetype_label" ), "WAV" );
                    break;

                case CMTEXT_BLOCK_TYPE_TAPHEADER:
                    file_bdspeed = ( !g_cmt.ext->block->cb_get_bdspeed ) ? 0 : g_cmt.ext->block->cb_get_bdspeed ( g_cmt.ext );
                    taphdr = cmttap_block_get_spec_tapheader ( g_cmt.ext->block );
                    g_assert ( taphdr != NULL );

                    snprintf ( buff, sizeof (buff ), "0x%02x", taphdr->code );
                    gtk_label_set_text ( ui_get_label ( "cmt_filetype_label" ), buff );

                    gtk_label_set_text ( ui_get_label ( "cmt_filename_label" ), taphdr->fname );

                    snprintf ( buff, sizeof (buff ), "%d Bd", file_bdspeed );
                    gtk_label_set_text ( ui_get_label ( "cmt_file_speed_label" ), buff );
                    break;

                case CMTEXT_BLOCK_TYPE_TAPDATA:
                    file_bdspeed = ( !g_cmt.ext->block->cb_get_bdspeed ) ? 0 : g_cmt.ext->block->cb_get_bdspeed ( g_cmt.ext );
                    tapdata = cmttap_block_get_spec_tapdata ( g_cmt.ext->block );
                    g_assert ( tapdata != NULL );

                    snprintf ( buff, sizeof (buff ), "0x%04x", tapdata->size - 2 );
                    gtk_label_set_text ( ui_get_label ( "cmt_fsize_label" ), buff );

                    snprintf ( buff, sizeof (buff ), "%d Bd", file_bdspeed );
                    gtk_label_set_text ( ui_get_label ( "cmt_file_speed_label" ), buff );
                    break;

                default:
                    fprintf ( stderr, "%s():%d - Unknown cmtext block type '%d'\n", __func__, __LINE__, cmtext_block_get_type ( g_cmt.ext->block ) );
            };

            // stream info
            ui_cmt_set_stream_info_labels ( g_cmt.ext->block );

        } else if ( EXIT_SUCCESS == cmtext_is_recordable ( g_cmt.ext ) ) {
            gtk_label_set_text ( ui_get_label ( "cmt_filetype_label" ), "SAVE" );
            // stream info
            ui_cmt_set_stream_info_labels ( g_cmt.ext->block );
        };
    };


    // progress bar
    if ( !TEST_CMT_FILLED ) {
        gtk_progress_bar_set_text ( ui_get_progress_bar ( "cmt_progressbar" ), "*** Empty ***" );
    } else {
        if ( g_cmt.playsts == CMTEXT_BLOCK_PLAYSTS_PAUSE ) {
            snprintf ( buff, sizeof (buff ), "*** Playing a gap space of %d ms ***", cmtext_block_get_pause_after ( g_cmt.ext->block ) );
            gtk_progress_bar_set_text ( ui_get_progress_bar ( "cmt_progressbar" ), buff );
        } else {
            if ( g_cmt.ext->block->cb_get_playname ) {
                gtk_progress_bar_set_text ( ui_get_progress_bar ( "cmt_progressbar" ), g_cmt.ext->block->cb_get_playname ( g_cmt.ext ) );
            } else {
                gtk_progress_bar_set_text ( ui_get_progress_bar ( "cmt_progressbar" ), cmtext_container_get_name ( container ) );
            };
        };
    };


    // Ovladaci prvky    
    if ( !TEST_CMT_FILLED ) {
        gtk_widget_set_sensitive ( ui_get_widget ( "cmt_filelist_button" ), FALSE );

        gtk_widget_show ( ui_get_widget ( "cmt_speed_label" ) );
        gtk_widget_show ( ui_get_widget ( "cmt_speed_comboboxtext" ) );

        gtk_widget_set_sensitive ( ui_get_widget ( "cmt_open_button" ), TRUE );
        gtk_widget_set_sensitive ( ui_get_widget ( "cmt_eject_button" ), FALSE );

        gtk_widget_set_sensitive ( ui_get_widget ( "cmt_play_togglebutton" ), FALSE );
        gtk_widget_set_sensitive ( ui_get_widget ( "cmt_record_togglebutton" ), TRUE );
        gtk_widget_set_sensitive ( ui_get_widget ( "cmt_stop_button" ), FALSE );
        gtk_widget_set_sensitive ( ui_get_widget ( "cmt_pause_togglebutton" ), FALSE );
        gtk_widget_set_sensitive ( ui_get_widget ( "cmt_speed_comboboxtext" ), TRUE );

        gtk_widget_set_sensitive ( ui_get_widget ( "cmt_previous_button" ), FALSE );
        gtk_widget_set_sensitive ( ui_get_widget ( "cmt_next_button" ), FALSE );
        gtk_widget_set_sensitive ( ui_get_widget ( "cmt_rewind_button" ), FALSE );
        gtk_widget_set_sensitive ( ui_get_widget ( "cmt_forward_button" ), FALSE );

        LOCK_UICALLBACKS ( );
        gtk_toggle_button_set_active ( GTK_TOGGLE_BUTTON ( ui_get_widget ( "cmt_play_togglebutton" ) ), FALSE );
        gtk_toggle_button_set_active ( GTK_TOGGLE_BUTTON ( ui_get_widget ( "cmt_record_togglebutton" ) ), FALSE );
        UNLOCK_UICALLBACKS ( );


        GtkWidget *statusbar = ui_get_widget ( "cmt_statusbar" );
        guint id = gtk_statusbar_get_context_id ( GTK_STATUSBAR ( statusbar ), "info" );
        gtk_statusbar_pop ( GTK_STATUSBAR ( statusbar ), id );
        gtk_statusbar_push ( GTK_STATUSBAR ( statusbar ), id, "CMT is empty..." );
    } else {

        if ( CMTEXT_CONTAINER_TYPE_SIMPLE_TAPE == cmtext_container_get_type ( container ) ) {
            gtk_widget_set_sensitive ( ui_get_widget ( "cmt_filelist_button" ), TRUE );
        } else {
            gtk_widget_set_sensitive ( ui_get_widget ( "cmt_filelist_button" ), FALSE );
        };

        if ( cmtext_block_get_block_speed ( g_cmt.ext->block ) == CMTEXT_BLOCK_SPEED_DEFAULT ) {
            gtk_widget_show ( ui_get_widget ( "cmt_speed_label" ) );
            gtk_widget_show ( ui_get_widget ( "cmt_speed_comboboxtext" ) );
        } else {
            gtk_widget_hide ( ui_get_widget ( "cmt_speed_label" ) );
            gtk_widget_hide ( ui_get_widget ( "cmt_speed_comboboxtext" ) );
        };


        if ( play_block == 1 ) {
            gtk_widget_set_sensitive ( ui_get_widget ( "cmt_previous_button" ), FALSE );
        } else {
            gtk_widget_set_sensitive ( ui_get_widget ( "cmt_previous_button" ), TRUE );
        };

        if ( play_block < total_blocks ) {
            gtk_widget_set_sensitive ( ui_get_widget ( "cmt_next_button" ), TRUE );
        } else {
            gtk_widget_set_sensitive ( ui_get_widget ( "cmt_next_button" ), FALSE );
        };


        gtk_widget_set_sensitive ( ui_get_widget ( "cmt_rewind_button" ), FALSE );
        gtk_widget_set_sensitive ( ui_get_widget ( "cmt_forward_button" ), FALSE );

        gtk_widget_set_sensitive ( ui_get_widget ( "cmt_pause_togglebutton" ), TRUE );

        if ( TEST_CMT_STOP ) {
            gtk_widget_set_sensitive ( ui_get_widget ( "cmt_open_button" ), TRUE );
            gtk_widget_set_sensitive ( ui_get_widget ( "cmt_eject_button" ), TRUE );

            gtk_widget_set_sensitive ( ui_get_widget ( "cmt_play_togglebutton" ), TRUE );
            gtk_widget_set_sensitive ( ui_get_widget ( "cmt_record_togglebutton" ), FALSE );

            gtk_widget_set_sensitive ( ui_get_widget ( "cmt_stop_button" ), FALSE );

            gtk_widget_set_sensitive ( ui_get_widget ( "cmt_speed_comboboxtext" ), TRUE );

            LOCK_UICALLBACKS ( );
            gtk_toggle_button_set_active ( GTK_TOGGLE_BUTTON ( ui_get_widget ( "cmt_play_togglebutton" ) ), FALSE );
            gtk_toggle_button_set_active ( GTK_TOGGLE_BUTTON ( ui_get_widget ( "cmt_record_togglebutton" ) ), FALSE );
            gtk_toggle_button_set_active ( GTK_TOGGLE_BUTTON ( ui_get_widget ( "cmt_pause_togglebutton" ) ), FALSE );
            UNLOCK_UICALLBACKS ( );

            GtkWidget *statusbar = ui_get_widget ( "cmt_statusbar" );
            guint id = gtk_statusbar_get_context_id ( GTK_STATUSBAR ( statusbar ), "info" );
            gtk_statusbar_pop ( GTK_STATUSBAR ( statusbar ), id );
            gtk_statusbar_push ( GTK_STATUSBAR ( statusbar ), id, "CMT is ready to play..." );
        } else {
            gtk_widget_set_sensitive ( ui_get_widget ( "cmt_open_button" ), FALSE );
            gtk_widget_set_sensitive ( ui_get_widget ( "cmt_eject_button" ), TRUE );

            gtk_widget_set_sensitive ( ui_get_widget ( "cmt_play_togglebutton" ), FALSE );
            gtk_widget_set_sensitive ( ui_get_widget ( "cmt_record_togglebutton" ), FALSE );

            gtk_widget_set_sensitive ( ui_get_widget ( "cmt_stop_button" ), TRUE );

            gtk_widget_set_sensitive ( ui_get_widget ( "cmt_speed_comboboxtext" ), FALSE );

            LOCK_UICALLBACKS ( );
            if ( TEST_CMT_PLAY ) {
                gtk_toggle_button_set_active ( GTK_TOGGLE_BUTTON ( ui_get_widget ( "cmt_play_togglebutton" ) ), TRUE );
            } else if ( TEST_CMT_RECORD ) {
                gtk_toggle_button_set_active ( GTK_TOGGLE_BUTTON ( ui_get_widget ( "cmt_record_togglebutton" ) ), TRUE );
            };

            if ( TEST_CMT_PAUSED ) {
                gtk_toggle_button_set_active ( GTK_TOGGLE_BUTTON ( ui_get_widget ( "cmt_pause_togglebutton" ) ), TRUE );
            } else {
                gtk_toggle_button_set_active ( GTK_TOGGLE_BUTTON ( ui_get_widget ( "cmt_pause_togglebutton" ) ), FALSE );
            };
            UNLOCK_UICALLBACKS ( );

            GtkWidget *statusbar = ui_get_widget ( "cmt_statusbar" );
            guint id = gtk_statusbar_get_context_id ( GTK_STATUSBAR ( statusbar ), "info" );
            gtk_statusbar_pop ( GTK_STATUSBAR ( statusbar ), id );

            if ( TEST_CMT_PLAY ) {
                if ( TEST_CMT_PAUSED ) {
                    gtk_statusbar_push ( GTK_STATUSBAR ( statusbar ), id, "CMT play is paused..." );
                } else {
                    gtk_statusbar_push ( GTK_STATUSBAR ( statusbar ), id, "CMT is playing..." );
                };
            } else {// if ( TEST_CMT_RECORD ) {
                if ( TEST_CMT_PAUSED ) {
                    gtk_statusbar_push ( GTK_STATUSBAR ( statusbar ), id, "CMT record is paused..." );
                } else {
                    gtk_statusbar_push ( GTK_STATUSBAR ( statusbar ), id, "CMT is recording..." );
                };
            };

        };
    };


    ui_cmt_update_player ( );

    int mztape_speed_id = 0;
    while ( g_mztape_speed[mztape_speed_id] != CMTSPEED_NONE ) {
        if ( g_mztape_speed[mztape_speed_id] == g_cmt.mz_cmtspeed ) break;
        mztape_speed_id++;
    }

    LOCK_UICALLBACKS ( );
    gtk_combo_box_set_active ( ui_get_combo_box ( "cmt_speed_comboboxtext" ), mztape_speed_id );
    UNLOCK_UICALLBACKS ( );

    ui_cmt_speed_menu_update ( );
    ui_cmt_polarity_menu_update ( );
    ui_cmt_tape_update_filelist ( );
}


void ui_cmt_update_player ( void ) {
    GtkWidget *window = ui_get_widget ( "cmt_window" );
    if ( !gtk_widget_get_visible ( window ) ) return;

    gdouble total_time = 0;
    gdouble play_time = 0;
    gdouble fraction = 0;
    guint32 print_time;

    if ( TEST_CMT_FILLED ) {
        if ( TEST_CMT_RECORD ) {
            ui_cmt_set_stream_info_labels ( g_cmt.ext->block );

            total_time = UI_CMT_RECORDING_MAX_TIME_IN_SEC;
            play_time = cmt_get_playtime ( );
            fraction = ( play_time / total_time );
        } else {
            double scan_time = cmtext_block_get_scantime ( g_cmt.ext->block );
            uint64_t scans = cmtext_block_get_count_scans ( g_cmt.ext->block );
            gdouble body_total_time = ( scan_time * scans );
            if ( TEST_CMT_PLAY ) {
                if ( g_cmt.playsts == CMTEXT_BLOCK_PLAYSTS_PAUSE ) {
                    total_time = 0.001 * cmtext_block_get_pause_after ( g_cmt.ext->block );
                    play_time = cmt_get_playtime ( ) - body_total_time;
                } else {
                    total_time = body_total_time;
                    play_time = cmt_get_playtime ( );
                };
                fraction = ( play_time / total_time );
            } else {
                total_time = body_total_time;
            };
        };
    };

    char *minus = "";

    if ( g_uicmt.show_time == UICMT_SHOW_REMAINING_TIME ) {
        print_time = ( total_time - play_time );
        if ( ( total_time - play_time ) > print_time ) {
            print_time++;
        };
        minus = "-";
    } else {
        print_time = play_time;
    };

    char buff [ 100 ];

    snprintf ( buff, sizeof ( buff ), "<b><span font='50'>%s%02d:%02d</span></b>", minus, ( print_time / 60 ), ( print_time % 60 ) );
    gtk_label_set_markup ( ui_get_label ( "cmt_time_label" ), buff );

    gtk_progress_bar_set_fraction ( ui_get_progress_bar ( "cmt_progressbar" ), fraction );
}


void ui_cmt_set_filename ( char *filename ) {
    gtk_progress_bar_set_text ( ui_get_progress_bar ( "cmt_progressbar" ), filename );
}


static void ui_cmt_switch_show_time ( void ) {
    if ( g_uicmt.show_time == UICMT_SHOW_REMAINING_TIME ) {
        ui_cmt_set_show_time ( UICMT_SHOW_PLAY_TIME );
    } else {
        ui_cmt_set_show_time ( UICMT_SHOW_REMAINING_TIME );
    };
}


G_MODULE_EXPORT gboolean on_cmt_time_info_eventbox_button_press_event ( GtkWidget *widget, GdkEvent *event, gpointer user_data ) {
    ui_cmt_switch_show_time ( );
    return FALSE;
}


G_MODULE_EXPORT gboolean on_cmt_time_eventbox_button_press_event ( GtkWidget *widget, GdkEvent *event, gpointer user_data ) {
    ui_cmt_switch_show_time ( );
    return FALSE;
}


G_MODULE_EXPORT void on_cmt_filelist_button_clicked ( GtkButton *button, gpointer data ) {
    (void) button;
    (void) data;
    ui_cmt_tape_window_show ( );
}


G_MODULE_EXPORT void on_cmt_cpu_boost_checkbutton_toggled ( GtkToggleButton *togglebutton, gpointer data ) {
    (void) data;

    if ( TEST_UICALLBACKS_LOCKED ) return;

    if ( FALSE == gtk_toggle_button_get_active ( togglebutton ) ) {
        cmt_cpu_boost_set ( CMT_CPU_BOOST_DISABLED );
    } else {
        cmt_cpu_boost_set ( CMT_CPU_BOOST_ENABLED );
    };
}


void ui_cmt_main_window_set_sensitive ( gboolean sensitive ) {
    gtk_widget_set_sensitive ( ui_get_widget ( "cmt_display_vbox" ), sensitive );
    gtk_widget_set_sensitive ( ui_get_widget ( "cmt_buttons_vbox" ), sensitive );
}


/*
 * 
 * MZF file size check
 * 
 */

static GString *s_filename = NULL;
static int s_fix_valid_size = 0;


static void ui_cmt_fix_mzfsize_window_hide ( gboolean save_option ) {
    GtkWidget *window = ui_get_widget ( "cmt_fix_mzfsize_window" );
    gtk_widget_hide ( window );

    if ( save_option ) {
        gboolean mzfsize_check_state = gtk_toggle_button_get_active ( GTK_TOGGLE_BUTTON ( ui_get_widget ( "cmt_fix_mzfsize_mzfsize_check_enabled_checkbutton" ) ) );
        cmt_mzfsize_check_set ( ( mzfsize_check_state ) ? CMT_MZFSIZE_CHECK_ENABLED : CMT_MZFSIZE_CHECK_DISABLED );
    } else {
        ui_cmt_mzfsize_check_menu_update ( );
    };

    g_string_free ( s_filename, TRUE );
}


static void ui_cmt_fix_mzfsize_window_show ( char *filename, int valid_size, int file_size ) {

    s_filename = g_string_new ( filename );
    s_fix_valid_size = valid_size;

    gtk_label_set_text ( ui_get_label ( "cmt_fix_mzfsize_filename_label" ), filename );

    GString *gs = g_string_sized_new ( 0 );
    g_string_sprintf ( gs, "%d", ( file_size - valid_size ) );
    gtk_label_set_text ( ui_get_label ( "cmt_fix_mzfsize_exceed_label" ), gs->str );
    g_string_free ( gs, TRUE );

    GtkWidget *window = ui_get_widget ( "cmt_fix_mzfsize_window" );
    gtk_widget_show ( window );
    gtk_widget_grab_focus ( window );
}


void ui_cmt_check_mzf_filesize ( char *filename, int valid_size ) {
    FILE *fh = ui_utils_file_open ( filename, "rb" );
    fseek ( fh, 0, SEEK_END );
    int file_size = ftell ( fh );
    fclose ( fh );
    if ( file_size > valid_size ) {
        ui_cmt_fix_mzfsize_window_show ( filename, valid_size, file_size );
    };
    return;
}


G_MODULE_EXPORT gboolean on_cmt_fix_mzfsize_window_delete_event ( GtkWidget *widget, GdkEvent *event, gpointer data ) {
    (void) widget;
    (void) event;
    (void) data;

    ui_cmt_fix_mzfsize_window_hide ( FALSE );
    return TRUE;
}


G_MODULE_EXPORT gboolean on_cmt_fix_mzfsize_window_key_press_event ( GtkWidget *widget, GdkEventKey *event, gpointer user_data ) {
    if ( event->keyval == GDK_KEY_Escape ) {
        ui_cmt_fix_mzfsize_window_hide ( FALSE );
        return TRUE;
    };
    return FALSE;
}


G_MODULE_EXPORT void on_cmt_fix_mzfsize_ignore_button_clicked ( GtkButton *button, gpointer data ) {
    (void) button;
    (void) data;

    ui_cmt_fix_mzfsize_window_hide ( TRUE );
}


G_MODULE_EXPORT void on_cmt_fix_mzfsize_truncate_button_clicked ( GtkButton *button, gpointer data ) {
    (void) button;
    (void) data;

    if ( 0 != ui_utils_file_truncate ( s_filename->str, s_fix_valid_size ) ) {
        printf ( "%s(%d) - Error: %s\n", __func__, __LINE__, ui_utils_strerror ( ) );
    } else {
        printf ( "MZF file truncated." );
    };

    ui_cmt_fix_mzfsize_window_hide ( TRUE );
}


G_MODULE_EXPORT void on_cmt_fix_mzfsize_saveas_button_clicked ( GtkButton *button, gpointer data ) {
    (void) button;
    (void) data;

    char *new_filepath = ui_file_chooser_open_mzf_to_save ( );
    if ( new_filepath ) {

        char *fileext = ui_file_chooser_get_filename_extension ( new_filepath );
        if ( fileext != NULL ) {
            if ( ( 0 != strcasecmp ( fileext, "mzf" ) ) && ( 0 != strcasecmp ( fileext, "m12" ) ) && ( 0 != strcasecmp ( fileext, "mzt" ) ) ) {
                fileext = NULL;
            };
        };

        if ( fileext == NULL ) {
            GString *gs = g_string_new ( 0 );
            int i = 0;

            FILE *tst_fh = NULL;

            do {
                if ( i > 100 ) break;
                g_string_sprintf ( gs, "%s", new_filepath );
                if ( i != 0 ) {
                    g_string_sprintfa ( gs, "_%d", i );
                };
                i++;
                g_string_append ( gs, ".mzf" );
                tst_fh = ui_utils_file_open ( gs->str, "rb+" );
            } while ( tst_fh );

            if ( tst_fh ) {
                g_string_free ( gs, TRUE );
                ui_utils_file_close ( tst_fh );
                printf ( "Can't save to file: %s.mzf\n", new_filepath );
                free ( new_filepath );
                new_filepath = NULL;
            } else {
                free ( new_filepath );
                new_filepath = g_string_free ( gs, FALSE );
            };
        };

        if ( new_filepath ) {
            FILE *fh_src = ui_utils_file_open ( s_filename->str, "rb" );
            if ( fh_src ) {
                uint8_t *buffer = g_new ( uint8_t, s_fix_valid_size );
                if ( s_fix_valid_size == ui_utils_file_read ( buffer, 1, s_fix_valid_size, fh_src ) ) {
                    FILE *fh_dst = ui_utils_file_open ( new_filepath, "wb" );
                    if ( fh_dst ) {
                        if ( s_fix_valid_size != ui_utils_file_write ( buffer, 1, s_fix_valid_size, fh_dst ) ) {
                            printf ( "%s(%d) - Error: %s\n", __func__, __LINE__, ui_utils_strerror ( ) );
                        } else {
                            printf ( "Fixed MZF file saved as: %s\n", new_filepath );
                        };
                        ui_utils_file_close ( fh_dst );
                    } else {
                        printf ( "%s(%d) - Error: %s\n", __func__, __LINE__, ui_utils_strerror ( ) );
                    };
                } else {
                    printf ( "%s(%d) - Error: %s\n", __func__, __LINE__, ui_utils_strerror ( ) );
                };

                ui_utils_file_close ( fh_src );
                free ( buffer );
            } else {
                printf ( "%s(%d) - Error: %s\n", __func__, __LINE__, ui_utils_strerror ( ) );
            };

            free ( new_filepath );
        };
    };

    ui_cmt_fix_mzfsize_window_hide ( TRUE );
}
