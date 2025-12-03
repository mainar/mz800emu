/* 
 * File:   ui_cmt_tape.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 11. února 2022, 8:17
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

#include "ui_main.h"
#include "ui_cmt.h"

#include "cmt/cmt.h"
#include "cmt/cmt_tap.h"
#include "libs/mztape/mztape.h"
#include "libs/zxtape/zxtape.h"


typedef enum en_CMT_FILELIST {
    CMT_FILELIST_FILE_POSITION, // uint
    CMT_FILELIST_FNAME,
    CMT_FILELIST_FTYPE_TXT,
    CMT_FILELIST_FSIZE_TXT,
    CMT_FILELIST_FSTRT_TXT,
    CMT_FILELIST_FEXEC_TXT,
    CMT_FILELIST_BLOCK_TYPE, // uint
    CMT_FILELIST_BLOCK_SPEED, // uint
    CMT_FILELIST_PLAY_ICON_PIXBUF,
    CMT_FILELIST_PLAY_ICON_VISIBLE, // bool
    CMT_FILELIST_CMTSPEED, // uint
    CMT_FILELIST_CMTSPEED_TXT,
    CMT_FILELIST_CMTSPEED_SENSITIVE, // bool
    CMT_FILELIST_CMTSPEED_MZ_VISIBLE, // bool
    CMT_FILELIST_CMTSPEED_ZX_VISIBLE, // bool
    CMT_FILELIST_CMTSPEED_PENCIL_VISIBLE, // bool
    CMT_FILELIST_LENGTH_TXT,
    CMT_FILELIST_COUNT_ITEMS
} en_CMT_FILELIST;


typedef struct st_UI_CMT_FILELIST {
    st_UIWINPOS main_pos;
    GdkPixbuf *play_pixbuf;
    GdkPixbuf *pause_pixbuf;
} st_UI_CMT_FILELIST;

static st_UI_CMT_FILELIST g_cmt_filelist;


void ui_cmt_tape_window_hide ( void ) {
    ui_cmt_main_window_set_sensitive ( TRUE );
    GtkWidget *window = ui_get_widget ( "cmt_tape_window" );
    if ( !gtk_widget_is_visible ( window ) ) return;
    ui_main_win_get_pos ( GTK_WINDOW ( window ), &g_cmt_filelist.main_pos );
    gtk_widget_hide ( window );
}


static void ui_cmt_tape_get_speed_txt ( char *buff, uint32_t buff_size, en_CMTEXT_BLOCK_TYPE bltype, en_CMTEXT_BLOCK_SPEED blspeed, en_CMTSPEED cmtspeed, gboolean rateio ) {

    uint16_t base_bdspeed;

    if ( blspeed == CMTEXT_BLOCK_SPEED_NONE ) {
        buff[0] = 0x00;
        return;
    };

    if ( blspeed == CMTEXT_BLOCK_SPEED_DEFAULT ) {
        snprintf ( buff, buff_size, "Default" );
        return;
    };

    if ( bltype == CMTEXT_BLOCK_TYPE_MZF ) {
        base_bdspeed = MZTAPE_DEFAULT_BDSPEED;
    } else if ( ( bltype == CMTEXT_BLOCK_TYPE_TAPHEADER ) || ( bltype == CMTEXT_BLOCK_TYPE_TAPDATA ) ) {
        base_bdspeed = ZXTAPE_DEFAULT_BDSPEED;
    } else {
        snprintf ( buff, buff_size, "UNKNOWN" );
        return;
    };

    if ( rateio ) {
        cmtspeed_get_ratiospeedtxt ( buff, buff_size, cmtspeed, base_bdspeed );
    } else {
        cmtspeed_get_speedtxt ( buff, buff_size, cmtspeed, base_bdspeed );
    };
}


static int ui_cmt_tape_get_length ( en_CMTEXT_BLOCK_TYPE bltype, en_CMTEXT_BLOCK_SPEED blspeed, en_CMTSPEED cmtspeed, int fsize ) {

    uint16_t base_bdspeed;

    if ( blspeed == CMTEXT_BLOCK_SPEED_NONE ) {
        return -1;
    };

    if ( bltype == CMTEXT_BLOCK_TYPE_MZF ) {
        base_bdspeed = MZTAPE_DEFAULT_BDSPEED;
    } else if ( ( bltype == CMTEXT_BLOCK_TYPE_TAPHEADER ) || ( bltype == CMTEXT_BLOCK_TYPE_TAPDATA ) ) {
        base_bdspeed = ZXTAPE_DEFAULT_BDSPEED;
    } else {
        // UNKNOWN
        return -1;
    };

    if ( cmtspeed == CMTSPEED_NONE ) {
        cmtspeed = CMTSPEED_1_1;
    };

    uint16_t bdspeed = cmtspeed_get_bdspeed ( cmtspeed, base_bdspeed );

    return ( fsize * 9 ) / bdspeed;
}


static void ui_cmt_tape_prepare_cmtspeed_mz_model ( void ) {

    GtkListStore *store = GTK_LIST_STORE ( ui_get_object ( "cmtspeed_mz_combo_liststore" ) );
    gtk_list_store_clear ( store );
    GtkTreeIter iter;

    gtk_list_store_append ( store, &iter );
    char buff[20];
    ui_cmt_tape_get_speed_txt ( buff, sizeof ( buff ), CMTEXT_BLOCK_TYPE_MZF, CMTEXT_BLOCK_SPEED_DEFAULT, CMTSPEED_NONE, TRUE );
    gtk_list_store_set ( store, &iter, 0, 0, 1, buff, -1 );

    int i = 0;
    while ( g_mztape_speed[i] != CMTSPEED_NONE ) {
        gtk_list_store_append ( store, &iter );
        ui_cmt_tape_get_speed_txt ( buff, sizeof ( buff ), CMTEXT_BLOCK_TYPE_MZF, CMTEXT_BLOCK_SPEED_SET, g_mztape_speed[i], TRUE );
        gtk_list_store_set ( store, &iter, 0, g_mztape_speed[i], 1, buff, -1 );
        i++;
    };

}


static void ui_cmt_tape_prepare_cmtspeed_zx_model ( void ) {

    GtkListStore *store = GTK_LIST_STORE ( ui_get_object ( "cmtspeed_zx_combo_liststore" ) );
    gtk_list_store_clear ( store );
    GtkTreeIter iter;

    int i = 0;
    while ( g_zxtape_speed[i] != CMTSPEED_NONE ) {
        gtk_list_store_append ( store, &iter );
        char buff[20];
        ui_cmt_tape_get_speed_txt ( buff, sizeof ( buff ), CMTEXT_BLOCK_TYPE_TAPHEADER, CMTEXT_BLOCK_SPEED_SET, g_zxtape_speed[i], TRUE );
        gtk_list_store_set ( store, &iter, 0, g_zxtape_speed[i], 1, buff, -1 );
        i++;
    };

}


void ui_cmt_tape_update_filelist ( void ) {
    GtkWidget *window = ui_get_widget ( "cmt_tape_window" );
    if ( !gtk_widget_is_visible ( window ) ) return;

    if ( !TEST_CMT_FILLED ) {
        ui_cmt_tape_window_hide ( );
        return;
    };

    st_CMTEXT_CONTAINER *container = cmtext_get_container ( g_cmt.ext );

    if ( cmtext_container_get_type ( container ) != CMTEXT_CONTAINER_TYPE_SIMPLE_TAPE ) {
        ui_cmt_tape_window_hide ( );
        return;
    };

    GtkWidget *treeview = ui_get_widget ( "cmt_tape_treeview" );
    GtkListStore *store = GTK_LIST_STORE ( gtk_tree_view_get_model ( GTK_TREE_VIEW ( treeview ) ) );
    gtk_list_store_clear ( store );

    int count_blocks = cmtext_container_get_count_blocks ( container );

    int play_block = cmtext_block_get_block_id ( g_cmt.ext->block );

    int i;
    for ( i = 0; i < count_blocks; i++ ) {

        gboolean played = ( ( i == play_block ) && ( TEST_CMT_PLAY ) ) ? TRUE : FALSE;
        gboolean paused = ( played && TEST_CMT_PAUSED ) ? TRUE : FALSE;

        en_CMTEXT_BLOCK_TYPE bltype = cmtext_container_get_block_type ( container, i );

        const char *ftype_txt;
        int ftype = cmtext_container_get_block_ftype ( container, i );
        if ( ( bltype == CMTEXT_BLOCK_TYPE_TAPHEADER ) || ( bltype == CMTEXT_BLOCK_TYPE_TAPDATA ) ) {
            ftype_txt = cmttap_get_block_code_txt ( ftype );
        } else {
            char ftype_num_txt[5];
            ftype_num_txt[0] = 0x00;
            if ( ftype != -1 ) {
                snprintf ( ftype_num_txt, sizeof ( ftype_num_txt ), "0x%02X", ftype );
            };
            ftype_txt = ftype_num_txt;
        };

        int fsize = cmtext_container_get_block_fsize ( container, i );
        char fsize_txt[7];
        fsize_txt[0] = 0x00;
        if ( fsize != -1 ) {
            snprintf ( fsize_txt, sizeof ( fsize_txt ), "0x%04X", fsize );
        };

        int fstrt = cmtext_container_get_block_fstrt ( container, i );
        char fstrt_txt[7];
        fstrt_txt[0] = 0x00;
        if ( fstrt != -1 ) {
            snprintf ( fstrt_txt, sizeof ( fstrt_txt ), "0x%04X", fstrt );
        };

        int fexec = cmtext_container_get_block_fexec ( container, i );
        char fexec_txt[7];
        fexec_txt[0] = 0x00;
        if ( fexec != -1 ) {
            snprintf ( fexec_txt, sizeof ( fexec_txt ), "0x%04X", fexec );
        };

        en_CMTEXT_BLOCK_SPEED blspeed = cmtext_container_get_block_speed ( container, i );

        gboolean cmtspeed_mz_visible = ( ( bltype == CMTEXT_BLOCK_TYPE_MZF ) && ( blspeed != CMTEXT_BLOCK_SPEED_NONE ) ) ? TRUE : FALSE;
        gboolean cmtspeed_zx_visible = ( ( ( bltype == CMTEXT_BLOCK_TYPE_TAPHEADER ) || ( bltype == CMTEXT_BLOCK_TYPE_TAPDATA ) ) && ( blspeed != CMTEXT_BLOCK_SPEED_NONE ) ) ? TRUE : FALSE;
        gboolean cmtspeed_sensitive = ( played ) ? FALSE : TRUE;

        en_CMTSPEED cmtspeed = cmtext_container_get_block_cmt_speed ( container, i );

        char cmtspeed_txt[20];
        ui_cmt_tape_get_speed_txt ( cmtspeed_txt, sizeof ( cmtspeed_txt ), bltype, blspeed, cmtspeed, FALSE );

        GString *gs = g_string_new ( 0 );
        int length = ui_cmt_tape_get_length ( bltype, blspeed, cmtspeed, fsize );
        if ( length > 0 ) {
            g_string_sprintf ( gs, "%02d:%02d", ( length / 60 ), ( length % 60 ) );
        };

        GdkPixbuf *play_icon_pixbuf = NULL;

        if ( paused ) {
            play_icon_pixbuf = g_cmt_filelist.pause_pixbuf;
        } else if ( played ) {
            play_icon_pixbuf = g_cmt_filelist.play_pixbuf;
        };

        GtkTreeIter iter;
        gtk_list_store_append ( store, &iter );
        gtk_list_store_set ( store, &iter,
                             CMT_FILELIST_FILE_POSITION, ( i + 1 ),
                             CMT_FILELIST_BLOCK_TYPE, bltype,
                             CMT_FILELIST_BLOCK_SPEED, blspeed,
                             CMT_FILELIST_FNAME, cmtext_container_get_block_fname ( container, i ),
                             CMT_FILELIST_FTYPE_TXT, ftype_txt,
                             CMT_FILELIST_FSIZE_TXT, fsize_txt,
                             CMT_FILELIST_FSTRT_TXT, fstrt_txt,
                             CMT_FILELIST_FEXEC_TXT, fexec_txt,
                             CMT_FILELIST_PLAY_ICON_PIXBUF, play_icon_pixbuf,
                             CMT_FILELIST_PLAY_ICON_VISIBLE, ( played | paused ),
                             CMT_FILELIST_CMTSPEED, cmtspeed,
                             CMT_FILELIST_CMTSPEED_TXT, cmtspeed_txt,
                             CMT_FILELIST_CMTSPEED_SENSITIVE, cmtspeed_sensitive,
                             CMT_FILELIST_CMTSPEED_MZ_VISIBLE, cmtspeed_mz_visible,
                             CMT_FILELIST_CMTSPEED_ZX_VISIBLE, cmtspeed_zx_visible,
                             CMT_FILELIST_CMTSPEED_PENCIL_VISIBLE, ( cmtspeed_mz_visible | cmtspeed_zx_visible ),
                             CMT_FILELIST_LENGTH_TXT, gs->str,
                             -1 );

        g_string_free ( gs, TRUE );
    };
}


void ui_cmt_tape_window_show ( void ) {
    ui_cmt_main_window_set_sensitive ( FALSE );
    static gboolean initialised = FALSE;
    if ( !initialised ) {
        ui_main_setpos ( &g_cmt_filelist.main_pos, -1, -1 );

        GError *error = NULL;
        g_cmt_filelist.play_pixbuf = gdk_pixbuf_new_from_file ( "ui_resources/icons/cmt/play_icon.png", &error );
        if ( error ) {
            fprintf ( stderr, "%s():%d - Unable to read icon: %s\n", __func__, __LINE__, error->message );
        };

        g_cmt_filelist.pause_pixbuf = gdk_pixbuf_new_from_file ( "ui_resources/icons/cmt/pause_icon.png", &error );
        if ( error ) {
            fprintf ( stderr, "%s():%d - Unable to read icon: %s\n", __func__, __LINE__, error->message );
        };

        ui_cmt_tape_prepare_cmtspeed_mz_model ( );
        ui_cmt_tape_prepare_cmtspeed_zx_model ( );

        initialised = TRUE;
    };
    GtkWidget *window = ui_get_widget ( "cmt_tape_window" );
    ui_main_win_move_to_pos ( GTK_WINDOW ( window ), &g_cmt_filelist.main_pos );


    gtk_widget_show ( window );
    ui_cmt_tape_update_filelist ( );
    gtk_widget_grab_focus ( window );
}


G_MODULE_EXPORT gboolean on_cmt_tape_window_key_press_event ( GtkWidget *widget, GdkEventKey *event, gpointer user_data ) {
    if ( event->keyval == GDK_KEY_Escape ) {
        ui_cmt_tape_window_hide ( );
        return TRUE;
    };
    return FALSE;
}


G_MODULE_EXPORT gboolean on_cmt_tape_window_delete_event ( GtkWidget *widget, GdkEvent *event, gpointer data ) {
    (void) widget;
    (void) event;
    (void) data;
    ui_cmt_tape_window_hide ( );
    return TRUE;
}


G_MODULE_EXPORT void on_cmt_tape_treeview_row_activated ( GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data ) {

    if ( (GObject*) column == ui_get_object ( "cmt_tape_play_icon_treeviewcolumn" ) ) return;

    gint *indices = gtk_tree_path_get_indices ( path );
    int new_block = indices[0];
    int old_block = cmtext_block_get_block_id ( g_cmt.ext->block );

    gboolean fl_play = TEST_CMT_PLAY;
    gboolean fl_paused = TEST_CMT_PAUSED;
    if ( fl_play ) cmt_stop ( );

    if ( old_block != new_block ) {
        if ( EXIT_SUCCESS == g_cmt.ext->container->cb_open_block ( new_block ) ) {
            if ( fl_paused ) {
                cmt_play_paused ( );
            } else {
                cmt_play ( );
            };
        };
    } else if ( !fl_play ) {
        if ( fl_paused ) {
            cmt_play_paused ( );
        } else {
            cmt_play ( );
        };
    };

    ui_cmt_window_update ( );
}


G_MODULE_EXPORT void on_cmtspeed_mz_cellrenderercombo_changed ( GtkCellRendererCombo *combo, gchar *path_string, GtkTreeIter *new_iter, gpointer user_data ) {

    GValue gv = G_VALUE_INIT;
    gtk_tree_model_get_value ( GTK_TREE_MODEL ( ui_get_object ( "cmtspeed_mz_combo_liststore" ) ), new_iter, 0, &gv );
    gint value = g_value_get_uint ( &gv );

    GtkTreeModel *model = gtk_tree_view_get_model ( GTK_TREE_VIEW ( ui_get_widget ( "cmt_tape_treeview" ) ) );
    GtkListStore *store = GTK_LIST_STORE ( model );
    GtkTreeIter iter;
    gtk_tree_model_get_iter_from_string ( model, &iter, path_string );

    en_CMTEXT_BLOCK_SPEED blspeed;
    en_CMTSPEED cmtspeed;

    if ( value == 0 ) {
        blspeed = CMTEXT_BLOCK_SPEED_DEFAULT;
        cmtspeed = CMTSPEED_NONE;
    } else {
        blspeed = CMTEXT_BLOCK_SPEED_SET;
        cmtspeed = value;
    };

    char cmtspeed_txt[20];
    ui_cmt_tape_get_speed_txt ( cmtspeed_txt, sizeof ( cmtspeed_txt ), CMTEXT_BLOCK_TYPE_MZF, blspeed, cmtspeed, FALSE );

    GtkTreePath *path = gtk_tree_path_new_from_string ( path_string );
    gint *indices = gtk_tree_path_get_indices ( path );
    int block_id = indices[0];

    if ( ( block_id == cmtext_block_get_block_id ( g_cmt.ext->block ) ) && ( TEST_CMT_PLAY ) ) {
        ui_cmt_tape_update_filelist ( );
    } else {
        st_CMTEXT_CONTAINER *container = cmtext_get_container ( g_cmt.ext );
        cmtext_container_set_block_speed ( container, block_id, blspeed );
        cmtext_container_set_block_cmt_speed ( container, block_id, cmtspeed );

        gtk_list_store_set ( store, &iter,
                             CMT_FILELIST_BLOCK_SPEED, blspeed,
                             CMT_FILELIST_CMTSPEED, cmtspeed,
                             CMT_FILELIST_CMTSPEED_TXT, cmtspeed_txt,
                             -1 );

        if ( block_id == cmtext_block_get_block_id ( g_cmt.ext->block ) ) {
            g_cmt.ext->container->cb_open_block ( block_id );
            ui_cmt_window_update ( );
        } else {
            ui_cmt_tape_update_filelist ( );
        };
    };
    gtk_tree_path_free ( path );

}


G_MODULE_EXPORT void on_cmtspeed_zx_cellrenderercombo_changed ( GtkCellRendererCombo *combo, gchar *path_string, GtkTreeIter *new_iter, gpointer user_data ) {

    GValue gv = G_VALUE_INIT;
    gtk_tree_model_get_value ( GTK_TREE_MODEL ( ui_get_object ( "cmtspeed_zx_combo_liststore" ) ), new_iter, 0, &gv );
    gint value = g_value_get_uint ( &gv );

    GtkTreeModel *model = gtk_tree_view_get_model ( GTK_TREE_VIEW ( ui_get_widget ( "cmt_tape_treeview" ) ) );
    GtkListStore *store = GTK_LIST_STORE ( model );
    GtkTreeIter iter;
    gtk_tree_model_get_iter_from_string ( model, &iter, path_string );

    en_CMTSPEED cmtspeed = value;

    char cmtspeed_txt[20];
    ui_cmt_tape_get_speed_txt ( cmtspeed_txt, sizeof ( cmtspeed_txt ), CMTEXT_BLOCK_TYPE_TAPHEADER, CMTEXT_BLOCK_SPEED_SET, cmtspeed, FALSE );

    GtkTreePath *path = gtk_tree_path_new_from_string ( path_string );
    gint *indices = gtk_tree_path_get_indices ( path );
    int block_id = indices[0];

    if ( ( block_id == cmtext_block_get_block_id ( g_cmt.ext->block ) ) && ( TEST_CMT_PLAY ) ) {
        ui_cmt_tape_update_filelist ( );
    } else {
        st_CMTEXT_CONTAINER *container = cmtext_get_container ( g_cmt.ext );
        cmtext_container_set_block_cmt_speed ( container, block_id, cmtspeed );

        gtk_list_store_set ( store, &iter,
                             CMT_FILELIST_CMTSPEED, cmtspeed,
                             CMT_FILELIST_CMTSPEED_TXT, cmtspeed_txt,
                             -1 );

        if ( block_id == cmtext_block_get_block_id ( g_cmt.ext->block ) ) {
            g_cmt.ext->container->cb_open_block ( block_id );
            ui_cmt_window_update ( );
        } else {
            ui_cmt_tape_update_filelist ( );
        };
    };
    gtk_tree_path_free ( path );
}


G_MODULE_EXPORT gboolean on_cmt_tape_treeview_button_press_event ( GtkWidget* self, GdkEventButton *event, gpointer user_data ) {

    GtkWidget *treeview = ui_get_widget ( "cmt_tape_treeview" );
    GtkTreePath *path;
    GtkTreeViewColumn *column;

    if ( !gtk_tree_view_get_path_at_pos ( GTK_TREE_VIEW ( treeview ), event->x, event->y, &path, &column, NULL, NULL ) ) return FALSE;
    if ( (GObject*) column != ui_get_object ( "cmt_tape_play_icon_treeviewcolumn" ) ) return FALSE;

    GtkTreeModel *model = gtk_tree_view_get_model ( GTK_TREE_VIEW ( treeview ) );

    GtkTreeIter iter;
    if ( gtk_tree_model_get_iter ( model, &iter, path ) ) {
        guint file_position;
        gtk_tree_model_get ( model, &iter, CMT_FILELIST_FILE_POSITION, &file_position, -1 );
        int selected_block_id = file_position - 1;

        int play_block = cmtext_block_get_block_id ( g_cmt.ext->block );
        if ( ( TEST_CMT_STOP ) || ( play_block != selected_block_id ) ) {
            if ( !TEST_CMT_STOP ) cmt_stop ( );
            if ( EXIT_FAILURE == g_cmt.ext->container->cb_open_block ( selected_block_id ) ) return FALSE;
            cmt_play_paused ( );
        } else {
            if ( TEST_CMT_PAUSED ) {
                cmt_pause ( 0 );
            } else {
                cmt_pause ( 1 );
            };
        };
    };

    return FALSE;
}
