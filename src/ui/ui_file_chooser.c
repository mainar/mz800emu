/* 
 * File:   ui_file_chooser.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 26. června 2018, 8:11
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

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdint.h>
#include <glib.h>
#include <glib/gstdio.h>


#include "ui_main.h"
#include "ui_utils.h"
#include "libs/mzf/mzf.h"
#include "libs/mzf/mzf_tools.h"
#include "fs_layer.h"

#include "cfgmain.h"
#include "ui_file_chooser.h"


typedef struct st_UI_FILE_CHOOSER {
    char *cmtfile_dir;
    char *mzf_dir;
    char *mzq_dir;
    char *qd_dir;
    char *dsk_dir;
    char *dat_dir;
    char *generic_dir;
    char *last_dir;
} st_UI_FILE_CHOOSER;

static st_UI_FILE_CHOOSER g_ui_filechooser;


void ui_file_chooser_init ( void ) {

    memset ( &g_ui_filechooser, 0x00, sizeof ( g_ui_filechooser ) );

    CFGMOD *cmod = cfgroot_register_new_module ( g_cfgmain, "UI_FILE_CHOOSER" );
    CFGELM *elm;

    elm = cfgmodule_register_new_element ( cmod, "cmtfile_dir", CFGENTYPE_TEXT, "" );
    cfgelement_set_pointers ( elm, (void*) &g_ui_filechooser.cmtfile_dir, (void*) &g_ui_filechooser.cmtfile_dir );

    elm = cfgmodule_register_new_element ( cmod, "mzf_dir", CFGENTYPE_TEXT, "" );
    cfgelement_set_pointers ( elm, (void*) &g_ui_filechooser.mzf_dir, (void*) &g_ui_filechooser.mzf_dir );

    elm = cfgmodule_register_new_element ( cmod, "mzq_dir", CFGENTYPE_TEXT, "" );
    cfgelement_set_pointers ( elm, (void*) &g_ui_filechooser.mzq_dir, (void*) &g_ui_filechooser.mzq_dir );

    elm = cfgmodule_register_new_element ( cmod, "qd_dir", CFGENTYPE_TEXT, "" );
    cfgelement_set_pointers ( elm, (void*) &g_ui_filechooser.qd_dir, (void*) &g_ui_filechooser.qd_dir );

    elm = cfgmodule_register_new_element ( cmod, "dsk_dir", CFGENTYPE_TEXT, "" );
    cfgelement_set_pointers ( elm, (void*) &g_ui_filechooser.dsk_dir, (void*) &g_ui_filechooser.dsk_dir );

    elm = cfgmodule_register_new_element ( cmod, "dat_dir", CFGENTYPE_TEXT, "" );
    cfgelement_set_pointers ( elm, (void*) &g_ui_filechooser.dat_dir, (void*) &g_ui_filechooser.dat_dir );

    elm = cfgmodule_register_new_element ( cmod, "generic_dir", CFGENTYPE_TEXT, "" );
    cfgelement_set_pointers ( elm, (void*) &g_ui_filechooser.generic_dir, (void*) &g_ui_filechooser.generic_dir );

    elm = cfgmodule_register_new_element ( cmod, "last_dir", CFGENTYPE_TEXT, "" );
    cfgelement_set_pointers ( elm, (void*) &g_ui_filechooser.last_dir, (void*) &g_ui_filechooser.last_dir );

    cfgmodule_parse ( cmod );
    cfgmodule_propagate ( cmod );
}


static inline void ui_file_chooser_free_cfg_dir ( char *dirpath ) {
    if ( dirpath ) g_free ( dirpath );
}


void ui_file_chooser_exit ( void ) {
    ui_file_chooser_free_cfg_dir ( g_ui_filechooser.cmtfile_dir );
    ui_file_chooser_free_cfg_dir ( g_ui_filechooser.mzf_dir );
    ui_file_chooser_free_cfg_dir ( g_ui_filechooser.mzq_dir );
    ui_file_chooser_free_cfg_dir ( g_ui_filechooser.qd_dir );
    ui_file_chooser_free_cfg_dir ( g_ui_filechooser.dsk_dir );
    ui_file_chooser_free_cfg_dir ( g_ui_filechooser.dat_dir );
    ui_file_chooser_free_cfg_dir ( g_ui_filechooser.generic_dir );
    ui_file_chooser_free_cfg_dir ( g_ui_filechooser.last_dir );
}


char* ui_file_chooser_get_filename_extension ( char *filename ) {
    int end_pos = strlen ( filename ) - 1;
    int pos = end_pos;
    while ( pos ) {
        if ( ( filename[pos] == '/' ) || ( filename[pos] == '\\' ) ) return NULL;
        if ( filename[pos] == '.' ) {
            break;
        };
        pos--;
    };
    if ( !pos ) return NULL;
    pos++;
    if ( pos == end_pos ) return NULL;
    return &filename[pos];
}


static gboolean ui_file_chooser_create_preview_MZF ( GtkWidget *preview, char *filename ) {
    st_MZF_HEADER hdr;
    FILE *fh = g_fopen ( filename, "rb" );
    if ( !fh ) return FALSE;
    uint32_t read_len = 0;
    int ret = fs_layer_fread ( &fh, &hdr, sizeof ( hdr ), &read_len );
    fclose ( fh );
    if ( ret != FS_LAYER_FR_OK ) return FALSE;
    mzf_header_items_correction ( &hdr );
    char fname[sizeof ( st_MZF_FILENAME )];
    mzf_tools_get_fname ( &hdr, (char*) &fname );
    const char format[] = "<b>NAME:</b> %s\n<b>TYPE:</b> 0x%02X\n<b>SIZE:</b> 0x%04X\n<b>STRT:</b> 0x%04X\n<b>EXEC:</b> 0x%04X";
    char *pango_fname = g_markup_escape_text ( fname, -1 );
    uint32_t length = sizeof ( format ) + sizeof ( pango_fname ) + 2 + 4 + 4 + 4 + 1;
    char *buff = ui_utils_mem_alloc0 ( length );
    g_snprintf ( buff, length, format, pango_fname, hdr.ftype, hdr.fsize, hdr.fstrt, hdr.fexec );
    gtk_label_set_markup ( GTK_LABEL ( preview ), buff );
    g_free ( pango_fname );
    g_free ( buff );
    return TRUE;
}


static gboolean ui_file_chooser_create_preview_by_filename ( GtkWidget *preview, char *filename ) {
    if ( ( !filename ) || ( g_file_test ( filename, G_FILE_TEST_IS_DIR ) ) ) return FALSE;
    char *extension = ui_file_chooser_get_filename_extension ( filename );
    if ( !extension ) return FALSE;
    if ( ( 0 == strcasecmp ( extension, "MZF" ) ) || ( 0 == strcasecmp ( extension, "M12" ) ) ) {
        return ui_file_chooser_create_preview_MZF ( preview, filename );
    } else if (
                ( 0 == strcasecmp ( extension, "WAV" ) ) ||
                ( 0 == strcasecmp ( extension, "MZT" ) ) ||
                ( 0 == strcasecmp ( extension, "TAP" ) )
                ) {
        gtk_label_set_markup ( GTK_LABEL ( preview ), "<b>You can load this file\nover virtual CMT.</b>" );
        return TRUE;
    };
    return FALSE;
}


static void ui_file_chooser_update_preview_cb ( GtkFileChooser *file_chooser, gpointer data ) {
    GtkWidget *preview;
    char *filename;
    gboolean have_preview;

    preview = GTK_WIDGET ( data );
    filename = gtk_file_chooser_get_preview_filename ( file_chooser );

    have_preview = ui_file_chooser_create_preview_by_filename ( preview, filename );
    g_free ( filename );

    gtk_file_chooser_set_preview_widget_active ( file_chooser, have_preview );
}


static void ui_file_chooser_get_lastdir ( char **dirpath, const char **lastdir, const char *predefined, const char *cfglastdir ) {
    *dirpath = NULL;
    *lastdir = NULL;
    if ( ( predefined ) && strlen ( predefined ) ) {
        *dirpath = g_path_get_dirname ( predefined );
    } else if ( ( cfglastdir ) && strlen ( cfglastdir ) ) {
        *lastdir = cfglastdir;
    } else if ( ( g_ui_filechooser.last_dir ) && strlen ( g_ui_filechooser.last_dir ) ) {
        *lastdir = g_ui_filechooser.last_dir;
    } else {
        *lastdir = "./";
    };
}


static void ui_file_chooser_get_lastdir_for_cmtfile ( char **dirpath, const char **lastdir, const char *predefined, const char *cfglastdir1, const char *cfglastdir2 ) {
    *dirpath = NULL;
    *lastdir = NULL;
    if ( ( predefined ) && strlen ( predefined ) ) {
        *dirpath = g_path_get_dirname ( predefined );
    } else if ( ( cfglastdir1 ) && strlen ( cfglastdir1 ) ) {
        *lastdir = cfglastdir1;
    } else if ( ( cfglastdir2 ) && strlen ( cfglastdir2 ) ) {
        *lastdir = cfglastdir2;
    } else if ( ( g_ui_filechooser.last_dir ) && strlen ( g_ui_filechooser.last_dir ) ) {
        *lastdir = g_ui_filechooser.last_dir;
    } else {
        *lastdir = "./";
    };
}


const char* ui_filechooser_get_last_mzf_dir ( void ) {
    char *dirpath = NULL;
    const char *lastdir = NULL;
    ui_file_chooser_get_lastdir ( &dirpath, &lastdir, NULL, g_ui_filechooser.mzf_dir );
    return lastdir;
}


const char* ui_filechooser_get_last_dsk_dir ( void ) {
    char *dirpath = NULL;
    const char *lastdir = NULL;
    ui_file_chooser_get_lastdir ( &dirpath, &lastdir, NULL, g_ui_filechooser.dsk_dir );
    return lastdir;
}


const char* ui_filechooser_get_last_mzq_dir ( void ) {
    char *dirpath = NULL;
    const char *lastdir = NULL;
    ui_file_chooser_get_lastdir ( &dirpath, &lastdir, NULL, g_ui_filechooser.mzq_dir );
    return lastdir;
}


const char* ui_filechooser_get_last_generic_dir ( void ) {
    char *dirpath = NULL;
    const char *lastdir = NULL;
    ui_file_chooser_get_lastdir ( &dirpath, &lastdir, NULL, g_ui_filechooser.generic_dir );
    return lastdir;
}


static void ui_filechooser_set_last_dir_from_filepath ( const char *filepath, char **cfglastdir ) {
    if ( ( filepath ) && ( strlen ( filepath ) ) ) {
        ui_utils_mem_free ( *cfglastdir );
        *cfglastdir = g_path_get_dirname ( filepath );
        ui_utils_mem_free ( g_ui_filechooser.last_dir );
        g_ui_filechooser.last_dir = g_path_get_dirname ( filepath );
    };
}


static void ui_filechooser_set_last_last_dir ( const char *dirpath ) {
    if ( ( dirpath ) && ( strlen ( dirpath ) ) ) {
        int len = strlen ( dirpath ) + 1;
        g_ui_filechooser.last_dir = (char*) ui_utils_mem_realloc ( g_ui_filechooser.last_dir, len );
        strncpy ( g_ui_filechooser.last_dir, dirpath, len );
    };
}


void ui_filechooser_set_last_mzq_dir ( const char *dirpath ) {
    if ( ( dirpath ) && ( strlen ( dirpath ) ) ) {
        int len = strlen ( dirpath ) + 1;
        g_ui_filechooser.mzq_dir = (char*) ui_utils_mem_realloc ( g_ui_filechooser.mzq_dir, len );
        strncpy ( g_ui_filechooser.mzq_dir, dirpath, len );
        ui_filechooser_set_last_last_dir ( dirpath );
    };
}


void ui_filechooser_set_last_dsk_dir ( const char *dirpath ) {
    if ( ( dirpath ) && ( strlen ( dirpath ) ) ) {
        int len = strlen ( dirpath ) + 1;
        g_ui_filechooser.dsk_dir = (char*) ui_utils_mem_realloc ( g_ui_filechooser.dsk_dir, len );
        strncpy ( g_ui_filechooser.dsk_dir, dirpath, len );
        ui_filechooser_set_last_last_dir ( dirpath );
    };
}


void ui_filechooser_set_last_mzf_dir ( const char *dirpath ) {
    if ( ( dirpath ) && ( strlen ( dirpath ) ) ) {
        int len = strlen ( dirpath ) + 1;
        g_ui_filechooser.mzf_dir = (char*) ui_utils_mem_realloc ( g_ui_filechooser.mzf_dir, len );
        strncpy ( g_ui_filechooser.mzf_dir, dirpath, len );
        ui_filechooser_set_last_last_dir ( dirpath );
    };
}


void ui_filechooser_set_last_generic_dir ( const char *dirpath ) {
    if ( ( dirpath ) && ( strlen ( dirpath ) ) ) {
        int len = strlen ( dirpath ) + 1;
        g_ui_filechooser.generic_dir = (char*) ui_utils_mem_realloc ( g_ui_filechooser.generic_dir, len );
        strncpy ( g_ui_filechooser.generic_dir, dirpath, len );
        ui_filechooser_set_last_last_dir ( dirpath );
    };
}


static char* ui_file_chooser_internal_open_file ( const char *predefined_filename, const char *predefined_dirpath, const char *window_title, GtkWindow *parent, GtkFileChooserAction action, st_UI_FCS_FILTERS *filters ) {

    GtkWindow *fcparent = ( parent ) ? parent : ui_get_window ( "main_window" );

    GtkWidget *fcdialog = gtk_file_chooser_dialog_new ( window_title, fcparent,
                                                        action,
                                                        "_Cancel", GTK_RESPONSE_CANCEL,
                                                        "_Open", GTK_RESPONSE_ACCEPT,
                                                        NULL );

    gtk_container_set_border_width ( GTK_CONTAINER ( fcdialog ), 5 );
    g_object_set ( fcdialog, "local-only", FALSE, NULL );
    gtk_window_set_modal ( GTK_WINDOW ( fcdialog ), TRUE );
    gtk_window_set_skip_taskbar_hint ( GTK_WINDOW ( fcdialog ), TRUE );
    gtk_window_set_skip_pager_hint ( GTK_WINDOW ( fcdialog ), TRUE );
    gtk_window_set_type_hint ( GTK_WINDOW ( fcdialog ), GDK_WINDOW_TYPE_HINT_DIALOG );
    gtk_window_set_urgency_hint ( GTK_WINDOW ( fcdialog ), TRUE );

    if ( filters != NULL ) {
        int i;
        for ( i = 0; i < filters->count; i++ ) {
            gtk_file_chooser_add_filter ( GTK_FILE_CHOOSER ( fcdialog ), filters->filter[i] );
        };
    };

    gtk_file_chooser_set_do_overwrite_confirmation ( GTK_FILE_CHOOSER ( fcdialog ), TRUE );

    GtkWidget *preview = gtk_label_new ( "" );
    gtk_widget_set_name ( preview, "ui_file_chooser_preview" );
    GtkStyleContext *context = gtk_widget_get_style_context ( preview );
    gtk_style_context_add_class ( context, "class_file_chooser_preview_label" );

    gtk_file_chooser_set_preview_widget ( GTK_FILE_CHOOSER ( fcdialog ), preview );
    g_signal_connect ( GTK_FILE_CHOOSER ( fcdialog ), "update-preview", G_CALLBACK ( ui_file_chooser_update_preview_cb ), preview );

    if ( ( predefined_filename ) && ( strlen ( predefined_filename ) ) ) {
        gtk_file_chooser_set_filename ( GTK_FILE_CHOOSER ( fcdialog ), predefined_filename );
    };

    if ( ( predefined_dirpath ) && ( strlen ( predefined_dirpath ) ) ) {
        gtk_file_chooser_set_current_folder ( GTK_FILE_CHOOSER ( fcdialog ), predefined_dirpath );
    };

    gtk_widget_show ( fcdialog );
    ui_main_win_move_to_pos ( GTK_WINDOW ( fcdialog ), &g_ui.filebrowser_pos );

    char *filename = NULL;
    if ( gtk_dialog_run ( GTK_DIALOG ( fcdialog ) ) == GTK_RESPONSE_ACCEPT ) {
        filename = gtk_file_chooser_get_filename ( GTK_FILE_CHOOSER ( fcdialog ) );
    };

    ui_main_win_get_pos ( GTK_WINDOW ( fcdialog ), &g_ui.filebrowser_pos );
    gtk_widget_destroy ( fcdialog );
    if ( filters != NULL ) {
        g_free ( filters->filter );
        g_free ( filters );
    };

    return filename;
}


st_UI_FCS_FILTERS* ui_file_chooser_filters_new ( void ) {
    st_UI_FCS_FILTERS *filters = (st_UI_FCS_FILTERS*) ui_utils_mem_alloc ( sizeof ( st_UI_FCS_FILTERS ) );
    filters->count = 0;
    filters->filter = (GtkFileFilter**) ui_utils_mem_alloc0 ( sizeof ( gpointer ) );
    return filters;
}


void ui_file_chooser_filters_add_filter ( st_UI_FCS_FILTERS *filters, GtkFileFilter *filter ) {
    int i = filters->count;
    filters->count++;
    filters->filter = (GtkFileFilter**) ui_utils_mem_realloc ( filters->filter, sizeof ( gpointer ) * filters->count );
    filters->filter[i] = filter;
}


static void ui_file_chooser_filter_add_mzf ( GtkFileFilter *filter ) {
    gtk_file_filter_add_pattern ( filter, "*.mzf" );
    gtk_file_filter_add_pattern ( filter, "*.MZF" );
    gtk_file_filter_add_pattern ( filter, "*.m12" );
    gtk_file_filter_add_pattern ( filter, "*.M12" );
}


GtkFileFilter* ui_file_chooser_create_filter_mzf ( void ) {
    GtkFileFilter *filter = gtk_file_filter_new ( );
    ui_file_chooser_filter_add_mzf ( filter );
    gtk_file_filter_set_name ( filter, "MZ-800 Tape File (MZF, M12)" );
    return filter;
}


static void ui_file_chooser_filter_add_mzt ( GtkFileFilter *filter ) {
    gtk_file_filter_add_pattern ( filter, "*.mzt" );
    gtk_file_filter_add_pattern ( filter, "*.MZT" );
}


static GtkFileFilter* ui_file_chooser_create_filter_mzt ( void ) {
    GtkFileFilter *filter = gtk_file_filter_new ( );
    ui_file_chooser_filter_add_mzt ( filter );
    gtk_file_filter_set_name ( filter, "MZ-800 Tape Archive File (MZT)" );
    return filter;
}


static void ui_file_chooser_filter_add_wav ( GtkFileFilter *filter ) {
    gtk_file_filter_add_pattern ( filter, "*.wav" );
    gtk_file_filter_add_pattern ( filter, "*.WAV" );
    gtk_file_filter_add_pattern ( filter, "*.wave" );
    gtk_file_filter_add_pattern ( filter, "*.WAVE" );
}


static GtkFileFilter* ui_file_chooser_create_filter_wav ( void ) {
    GtkFileFilter *filter = gtk_file_filter_new ( );
    ui_file_chooser_filter_add_wav ( filter );
    gtk_file_filter_set_name ( filter, "Waveform Audio File Format (WAV, WAVE)" );
    return filter;
}


static void ui_file_chooser_filter_add_tap ( GtkFileFilter *filter ) {
    gtk_file_filter_add_pattern ( filter, "*.tap" );
    gtk_file_filter_add_pattern ( filter, "*.TAP" );
}


static GtkFileFilter* ui_file_chooser_create_filter_tap ( void ) {
    GtkFileFilter *filter = gtk_file_filter_new ( );
    ui_file_chooser_filter_add_tap ( filter );
    gtk_file_filter_set_name ( filter, "ZX spectrum simple tape (TAP)" );
    return filter;
}


static GtkFileFilter* ui_file_chooser_create_filter_all_cmt_formats ( void ) {
    GtkFileFilter *filter = gtk_file_filter_new ( );
    ui_file_chooser_filter_add_mzf ( filter );
    ui_file_chooser_filter_add_mzt ( filter );
    ui_file_chooser_filter_add_wav ( filter );
    ui_file_chooser_filter_add_tap ( filter );
    gtk_file_filter_set_name ( filter, "All supported CMT formats" );
    return filter;
}


static void ui_file_chooser_filter_add_mzq ( GtkFileFilter *filter ) {
    gtk_file_filter_add_pattern ( filter, "*.mzq" );
    gtk_file_filter_add_pattern ( filter, "*.MZQ" );
}


static GtkFileFilter* ui_file_chooser_create_filter_mzq ( void ) {
    GtkFileFilter *filter = gtk_file_filter_new ( );
    ui_file_chooser_filter_add_mzq ( filter );
    gtk_file_filter_set_name ( filter, "MZ - Quick Disk Image File (MZQ)" );
    return filter;
}


static void ui_file_chooser_filter_add_dsk ( GtkFileFilter *filter ) {
    gtk_file_filter_add_pattern ( filter, "*.dsk" );
    gtk_file_filter_add_pattern ( filter, "*.DSK" );
}


static GtkFileFilter* ui_file_chooser_create_filter_dsk ( void ) {
    GtkFileFilter *filter = gtk_file_filter_new ( );
    ui_file_chooser_filter_add_dsk ( filter );
    gtk_file_filter_set_name ( filter, "Floppy Disk Image (DSK)" );
    return filter;
}


static void ui_file_chooser_filter_add_dat ( GtkFileFilter *filter ) {
    gtk_file_filter_add_pattern ( filter, "*.dat" );
    gtk_file_filter_add_pattern ( filter, "*.DAT" );
}


static GtkFileFilter* ui_file_chooser_create_filter_ramdisk ( void ) {
    GtkFileFilter *filter = gtk_file_filter_new ( );
    ui_file_chooser_filter_add_dat ( filter );
    gtk_file_filter_set_name ( filter, "RAM / ROM file (DAT)" );
    return filter;
}


static void ui_file_chooser_filter_add_all_files ( GtkFileFilter *filter ) {
    gtk_file_filter_add_pattern ( filter, "*.*" );
}


static GtkFileFilter* ui_file_chooser_create_filter_all_files ( void ) {
    GtkFileFilter *filter = gtk_file_filter_new ( );
    ui_file_chooser_filter_add_all_files ( filter );
    gtk_file_filter_set_name ( filter, "All files (*)" );
    return filter;
}


char* ui_file_chooser_open_cmt_file ( const char *predefined_filename ) {
    st_UI_FCS_FILTERS *filters = ui_file_chooser_filters_new ( );
    ui_file_chooser_filters_add_filter ( filters, ui_file_chooser_create_filter_all_cmt_formats ( ) );
    ui_file_chooser_filters_add_filter ( filters, ui_file_chooser_create_filter_mzf ( ) );
    ui_file_chooser_filters_add_filter ( filters, ui_file_chooser_create_filter_wav ( ) );
    ui_file_chooser_filters_add_filter ( filters, ui_file_chooser_create_filter_mzt ( ) );
    ui_file_chooser_filters_add_filter ( filters, ui_file_chooser_create_filter_tap ( ) );
    char title[] = "Select CMT file to open";
    char *dirpath = NULL;
    const char *lastdir = NULL;
    ui_file_chooser_get_lastdir_for_cmtfile ( &dirpath, &lastdir, predefined_filename, g_ui_filechooser.cmtfile_dir, g_ui_filechooser.mzf_dir );
    GtkWindow *parent = ui_get_window ( "cmt_window" );
    char *filepath = ui_file_chooser_internal_open_file ( predefined_filename, ( dirpath ) ? dirpath : lastdir, title, parent, GTK_FILE_CHOOSER_ACTION_OPEN, filters );
    if ( dirpath ) g_free ( dirpath );
    ui_filechooser_set_last_dir_from_filepath ( filepath, &g_ui_filechooser.cmtfile_dir );
    return filepath;
}


char* ui_file_chooser_open_cmthack_file ( const char *predefined_filename ) {
    st_UI_FCS_FILTERS *filters = ui_file_chooser_filters_new ( );
    ui_file_chooser_filters_add_filter ( filters, ui_file_chooser_create_filter_mzf ( ) );
    ui_file_chooser_filters_add_filter ( filters, ui_file_chooser_create_filter_all_cmt_formats ( ) );
    char title[] = "Select MZF file to load by cmthack";
    char *dirpath = NULL;
    const char *lastdir = NULL;
    ui_file_chooser_get_lastdir_for_cmtfile ( &dirpath, &lastdir, predefined_filename, g_ui_filechooser.mzf_dir, g_ui_filechooser.cmtfile_dir );
    char *filepath = ui_file_chooser_internal_open_file ( predefined_filename, ( dirpath ) ? dirpath : lastdir, title, NULL, GTK_FILE_CHOOSER_ACTION_OPEN, filters );
    if ( dirpath ) g_free ( dirpath );
    ui_filechooser_set_last_dir_from_filepath ( filepath, &g_ui_filechooser.mzf_dir );
    return filepath;
}


char* ui_file_chooser_open_mzf ( const char *predefined_filename, void *parent_window ) {
    GtkWindow *parent = (GtkWindow*) parent_window;
    st_UI_FCS_FILTERS *filters = ui_file_chooser_filters_new ( );
    ui_file_chooser_filters_add_filter ( filters, ui_file_chooser_create_filter_mzf ( ) );
    char title[] = "Select MZF file to open";
    char *dirpath = NULL;
    const char *lastdir = NULL;
    ui_file_chooser_get_lastdir_for_cmtfile ( &dirpath, &lastdir, predefined_filename, g_ui_filechooser.mzf_dir, g_ui_filechooser.cmtfile_dir );
    char *filepath = ui_file_chooser_internal_open_file ( predefined_filename, ( dirpath ) ? dirpath : lastdir, title, parent, GTK_FILE_CHOOSER_ACTION_OPEN, filters );
    if ( dirpath ) g_free ( dirpath );
    ui_filechooser_set_last_dir_from_filepath ( filepath, &g_ui_filechooser.mzf_dir );
    return filepath;
}


char* ui_file_chooser_open_mzf_to_save ( void ) {
    st_UI_FCS_FILTERS *filters = ui_file_chooser_filters_new ( );
    ui_file_chooser_filters_add_filter ( filters, ui_file_chooser_create_filter_mzf ( ) );
    ui_file_chooser_filters_add_filter ( filters, ui_file_chooser_create_filter_mzt ( ) );
    char title[] = "Create / Select MZF, MZT file to save";
    char *dirpath = NULL;
    const char *lastdir = NULL;
    ui_file_chooser_get_lastdir_for_cmtfile ( &dirpath, &lastdir, NULL, g_ui_filechooser.cmtfile_dir, g_ui_filechooser.mzf_dir );
    GtkWindow *parent = ui_get_window ( "cmt_window" );
    char *filepath = ui_file_chooser_internal_open_file ( NULL, ( dirpath ) ? dirpath : lastdir, title, parent, GTK_FILE_CHOOSER_ACTION_SAVE, filters );
    if ( dirpath ) g_free ( dirpath );
    ui_filechooser_set_last_dir_from_filepath ( filepath, &g_ui_filechooser.cmtfile_dir );
    return filepath;
}


char* ui_file_chooser_open_wav_to_record ( void ) {
    st_UI_FCS_FILTERS *filters = ui_file_chooser_filters_new ( );
    ui_file_chooser_filters_add_filter ( filters, ui_file_chooser_create_filter_wav ( ) );
    char title[] = "Create / Select WAV file to record";
    char *dirpath = NULL;
    const char *lastdir = NULL;
    ui_file_chooser_get_lastdir_for_cmtfile ( &dirpath, &lastdir, NULL, g_ui_filechooser.cmtfile_dir, g_ui_filechooser.mzf_dir );
    GtkWindow *parent = ui_get_window ( "cmt_window" );
    char *filepath = ui_file_chooser_internal_open_file ( NULL, ( dirpath ) ? dirpath : lastdir, title, parent, GTK_FILE_CHOOSER_ACTION_SAVE, filters );
    if ( dirpath ) g_free ( dirpath );
    ui_filechooser_set_last_dir_from_filepath ( filepath, &g_ui_filechooser.cmtfile_dir );
    return filepath;
}


char* ui_file_chooser_open_mzq ( const char *predefined_filename ) {
    st_UI_FCS_FILTERS *filters = ui_file_chooser_filters_new ( );
    ui_file_chooser_filters_add_filter ( filters, ui_file_chooser_create_filter_mzq ( ) );
    char title[] = "Select existing MZQ file to open or type new .mzq filename to create image";
    char *dirpath = NULL;
    const char *lastdir = NULL;
    ui_file_chooser_get_lastdir ( &dirpath, &lastdir, predefined_filename, g_ui_filechooser.mzq_dir );
    char *filepath = ui_file_chooser_internal_open_file ( predefined_filename, ( dirpath ) ? dirpath : lastdir, title, NULL, ( GTK_FILE_CHOOSER_ACTION_OPEN | GTK_FILE_CHOOSER_ACTION_SAVE ), filters );
    if ( dirpath ) g_free ( dirpath );
    ui_filechooser_set_last_dir_from_filepath ( filepath, &g_ui_filechooser.mzq_dir );
    return filepath;
}


char* ui_file_chooser_open_dsk ( const char *predefined_filename ) {
    st_UI_FCS_FILTERS *filters = ui_file_chooser_filters_new ( );
    ui_file_chooser_filters_add_filter ( filters, ui_file_chooser_create_filter_dsk ( ) );
    char title[] = "Select DSK file to open";
    char *dirpath = NULL;
    const char *lastdir = NULL;
    ui_file_chooser_get_lastdir ( &dirpath, &lastdir, predefined_filename, g_ui_filechooser.dsk_dir );
    char *filepath = ui_file_chooser_internal_open_file ( predefined_filename, ( dirpath ) ? dirpath : lastdir, title, NULL, GTK_FILE_CHOOSER_ACTION_OPEN, filters );
    if ( dirpath ) g_free ( dirpath );
    ui_filechooser_set_last_dir_from_filepath ( filepath, &g_ui_filechooser.dsk_dir );
    return filepath;
}


char* ui_file_chooser_open_dir ( const char *predefined_filepath, const char *title ) {
    char *dirpath = NULL;
    const char *lastdir = NULL;
    ui_file_chooser_get_lastdir ( &dirpath, &lastdir, predefined_filepath, g_ui_filechooser.qd_dir );
    // Volba | GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER se chova nejak divne
    char *filepath = ui_file_chooser_internal_open_file ( predefined_filepath, ( dirpath ) ? dirpath : lastdir, title, NULL, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, NULL );
    if ( dirpath ) g_free ( dirpath );
    ui_filechooser_set_last_dir_from_filepath ( filepath, &g_ui_filechooser.qd_dir );
    return filepath;
}


char* ui_file_chooser_open_qddir ( const char *predefined_filepath ) {
    char title[] = "Select directory to mount as virtual Quick Disk";
    char *dirpath = NULL;
    const char *lastdir = NULL;
    ui_file_chooser_get_lastdir ( &dirpath, &lastdir, predefined_filepath, g_ui_filechooser.qd_dir );
    // Volba | GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER se chova nejak divne
    char *filepath = ui_file_chooser_internal_open_file ( predefined_filepath, ( dirpath ) ? dirpath : lastdir, title, NULL, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, NULL );
    if ( dirpath ) g_free ( dirpath );
    ui_filechooser_set_last_dir_from_filepath ( filepath, &g_ui_filechooser.qd_dir );
    return filepath;
}


char* ui_file_chooser_open_file ( const char *predefined_filepath, const char *predefined_dirpath, const char *title, void *parent_window, en_FC_MODE fcmode, st_UI_FCS_FILTERS *filters ) {
    GtkWindow *parent = (GtkWindow*) parent_window;
    char *dirpath = NULL;
    const char *lastdir = NULL;
    gboolean use_generic_dir = FALSE;
    if ( ( predefined_dirpath ) && ( strlen ( predefined_dirpath ) ) ) {
        lastdir = predefined_dirpath;
    } else {
        ui_file_chooser_get_lastdir ( &dirpath, &lastdir, predefined_filepath, g_ui_filechooser.generic_dir );
        use_generic_dir = TRUE;
    };

    GtkFileChooserAction action;
    switch ( fcmode ) {
        case FC_MODE_OPEN:
            action = GTK_FILE_CHOOSER_ACTION_OPEN;
            break;

        case FC_MODE_SAVE:
            action = GTK_FILE_CHOOSER_ACTION_SAVE;
            break;

        case FC_MODE_OPEN_OR_NEW:
            action = GTK_FILE_CHOOSER_ACTION_OPEN | GTK_FILE_CHOOSER_ACTION_SAVE;
            break;

        default:
            fprintf ( stderr, "%s():%d - Unknown fcmode = %d\n", __func__, __LINE__, fcmode );
            action = GTK_FILE_CHOOSER_ACTION_OPEN;
    };

    char *filepath = ui_file_chooser_internal_open_file ( predefined_filepath, ( dirpath ) ? dirpath : lastdir, title, parent, action, filters );
    if ( dirpath ) g_free ( dirpath );
    if ( use_generic_dir ) ui_filechooser_set_last_dir_from_filepath ( filepath, &g_ui_filechooser.generic_dir );
    return filepath;
}


char* ui_file_chooser_open_dat ( const char *predefined_filepath, const char *title, void *parent_window, en_FC_MODE fcmode ) {
    st_UI_FCS_FILTERS *filters = ui_file_chooser_filters_new ( );
    ui_file_chooser_filters_add_filter ( filters, ui_file_chooser_create_filter_ramdisk ( ) );
    ui_file_chooser_filters_add_filter ( filters, ui_file_chooser_create_filter_all_files ( ) );
    char *dirpath = NULL;
    const char *lastdir = NULL;
    ui_file_chooser_get_lastdir ( &dirpath, &lastdir, predefined_filepath, g_ui_filechooser.dat_dir );
    char *filepath = ui_file_chooser_open_file ( predefined_filepath, ( dirpath ) ? dirpath : lastdir, title, parent_window, fcmode, filters );
    if ( dirpath ) g_free ( dirpath );
    ui_filechooser_set_last_dir_from_filepath ( filepath, &g_ui_filechooser.dat_dir );
    return filepath;
}
