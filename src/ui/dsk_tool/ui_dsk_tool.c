/* 
 * File:   ui_dsk_tool.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 12. září 2016, 10:07
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
#include <gtk/gtk.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "ui/ui_main.h"
#include "ui/ui_file_chooser.h"

#include "libs/dsk/dsk.h"
#include "libs/dsk/dsk_tools.h"
#include "ui_dsk_tool.h"
#include "ui/ui_hexeditable.h"

#include "libs/dsk/dsk.h"
#include "libs/generic_driver/generic_driver.h"
#include "src/ui/generic_driver/ui_file_driver.h"
#include "src/ui/generic_driver/ui_memory_driver.h"
#include "ui/ui_utils.h"



#ifndef WINDOWS
#define HAVE_GTK_GRID_REMOVE_ROW    // existuje az ve verzi 3.10
#endif


typedef struct st_UI_DSK_TOOL_PREDEFINED_RULE {
    guint8 from_track;
    guint8 sectors;
    en_DSK_SECTOR_SIZE ssize;
    en_DSK_SECTOR_ORDER_TYPE sector_order;
    guint8 *sector_map;
    guint8 filler;
} st_UI_DSK_TOOL_PREDEFINED_RULE;


typedef struct st_UI_DSK_TOOL_PREDEFINED_ROW {
    char *name;
    gint sides;
    gint tracks;
    st_UI_DSK_TOOL_PREDEFINED_RULE *rules;
} st_UI_DSK_TOOL_PREDEFINED_ROW;


static st_UI_DSK_TOOL_PREDEFINED_RULE g_predef_custom_rules[] = {
    { 0, 16, DSK_SECTOR_SIZE_256, DSK_SEC_ORDER_NORMAL, NULL, 0xff },
    { 0, 0, 0, 0, NULL, 0 },
};


static st_UI_DSK_TOOL_PREDEFINED_RULE g_predef_mzbasic_rules[] = {
    { 0, 16, DSK_SECTOR_SIZE_256, DSK_SEC_ORDER_NORMAL, NULL, 0xff },
    { 0, 0, 0, 0, NULL, 0 },
};


static st_UI_DSK_TOOL_PREDEFINED_RULE g_predef_leccpmSD_rules[] = {
    { 0, 9, DSK_SECTOR_SIZE_512, DSK_SEC_ORDER_INTERLACED_LEC, NULL, 0xe5 },
    { 1, 16, DSK_SECTOR_SIZE_256, DSK_SEC_ORDER_NORMAL, NULL, 0xff },
    { 2, 9, DSK_SECTOR_SIZE_512, DSK_SEC_ORDER_INTERLACED_LEC, NULL, 0xe5 },
    { 0, 0, 0, 0, NULL, 0 },
};


static st_UI_DSK_TOOL_PREDEFINED_RULE g_predef_leccpmHD_rules[] = {
    { 0, 18, DSK_SECTOR_SIZE_512, DSK_SEC_ORDER_INTERLACED_LEC_HD, NULL, 0xe5 },
    { 1, 16, DSK_SECTOR_SIZE_256, DSK_SEC_ORDER_NORMAL, NULL, 0xff },
    { 2, 18, DSK_SECTOR_SIZE_512, DSK_SEC_ORDER_INTERLACED_LEC_HD, NULL, 0xe5 },
    { 0, 0, 0, 0, NULL, 0 },
};

static st_UI_DSK_TOOL_PREDEFINED_RULE g_predef_mrs_rules[] = {
    { 0, 9, DSK_SECTOR_SIZE_512, DSK_SEC_ORDER_INTERLACED_LEC, NULL, 0xe5 },
    { 1, 16, DSK_SECTOR_SIZE_256, DSK_SEC_ORDER_NORMAL, NULL, 0xff },
    { 2, 9, DSK_SECTOR_SIZE_512, DSK_SEC_ORDER_INTERLACED_LEC, NULL, 0xe5 },
    { 0, 0, 0, 0, NULL, 0 },
};

guint8 g_predef_sector_map_lemmings[10] = { 1, 6, 2, 7, 3, 8, 4, 9, 5, 21 };

static st_UI_DSK_TOOL_PREDEFINED_RULE g_predef_lemmings_rules[] = {
    { 0, 9, DSK_SECTOR_SIZE_512, DSK_SEC_ORDER_INTERLACED_LEC, NULL, 0xe5 },
    { 1, 16, DSK_SECTOR_SIZE_256, DSK_SEC_ORDER_NORMAL, NULL, 0xe5 },
    { 2, 9, DSK_SECTOR_SIZE_512, DSK_SEC_ORDER_INTERLACED_LEC, NULL, 0xe5 },
    { 16, 10, DSK_SECTOR_SIZE_512, DSK_SEC_ORDER_CUSTOM, g_predef_sector_map_lemmings, 0xe5 },
    { 17, 9, DSK_SECTOR_SIZE_512, DSK_SEC_ORDER_INTERLACED_LEC, NULL, 0xe5 },
    { 0, 0, 0, 0, NULL, 0 },
};


static const st_UI_DSK_TOOL_PREDEFINED_ROW g_predef_custom = { "Custom format...", 2, 160, g_predef_custom_rules };
static const st_UI_DSK_TOOL_PREDEFINED_ROW g_predef_mzbasic = { "Sharp MZ-BASIC", 2, 160, g_predef_mzbasic_rules };
static const st_UI_DSK_TOOL_PREDEFINED_ROW g_predef_leccpmSD = { "Sharp LEC cp/m", 2, 160, g_predef_leccpmSD_rules };
static const st_UI_DSK_TOOL_PREDEFINED_ROW g_predef_leccpmHD = { "Sharp LEC-HD cp/m", 2, 160, g_predef_leccpmHD_rules };
static const st_UI_DSK_TOOL_PREDEFINED_ROW g_predef_mrs = { "Sharp MRS", 2, 160, g_predef_mrs_rules };
static const st_UI_DSK_TOOL_PREDEFINED_ROW g_predef_lemmings = { "Sharp Lemmings", 2, 160, g_predef_lemmings_rules };


typedef struct st_UI_DSK_TOOL_PREDEFINED {
    const gchar id;
    const st_UI_DSK_TOOL_PREDEFINED_ROW *predef;
} st_UI_DSK_TOOL_PREDEFINED;

static const st_UI_DSK_TOOL_PREDEFINED g_predefined[] = {
    { 0, &g_predef_custom },
    { 1, &g_predef_mzbasic },
    { 2, &g_predef_leccpmSD },
    { 3, &g_predef_leccpmHD },
    { 4, &g_predef_mrs },
    { 5, &g_predef_lemmings },
};


typedef struct st_UI_DSK_TOOL_RULE {
    GtkWidget *label_track;
    GtkAdjustment *spinbutton_track_adj;
    GtkWidget *spinbutton_track;
    GtkWidget *label_sectors;
    GtkAdjustment *spinbutton_sectors_adj;
    GtkWidget *spinbutton_sectors;
    GtkWidget *comboboxtext_ssize;
    GtkWidget *comboboxtext_sector_order;
    GtkWidget *button_edit_map;
    GtkWidget *label_filler;
    GtkWidget *entry_filler;
    GtkWidget *separator;
    GtkWidget *button_remove;
    guint8 sector_map[DSK_MAX_SECTORS];
    int *rule_id;
} st_UI_DSK_TOOL_RULE;


typedef struct st_UI_DSK_TOOL {
    int rules_count;
    gboolean lock_predef;
    GtkWidget *grid_rules;
    st_UI_DSK_TOOL_RULE *rules;
} st_UI_DSK_TOOL;

static st_UI_DSK_TOOL g_dsk_tool;


static inline void ui_dsk_tool_watch_predef ( void ) {
    if ( g_dsk_tool.lock_predef ) return;
    if ( 0 == gtk_combo_box_get_active ( ui_get_combo_box ( "comboboxtext_dsk_new_predefined" ) ) ) return;
    g_dsk_tool.lock_predef = TRUE;
    gtk_combo_box_set_active ( ui_get_combo_box ( "comboboxtext_dsk_new_predefined" ), 0 );
    g_dsk_tool.lock_predef = FALSE;
}


/*
 * 
 * Sector map
 * 
 */

typedef struct st_UI_DSK_TOOL_SMAP_ROW {
    int id;
    GtkWidget *label_sector;
    GtkWidget *label_colon;
    GtkWidget *spinbutton_sector_id;
    GtkAdjustment *adjustment_sector_id;
    GtkWidget *label_hexvalue;
    GtkWidget *entry_sector_id;
} st_UI_DSK_TOOL_SMAP_ROW;


typedef struct st_UI_DSK_TOOL_SMAP {
    int rule_id;
    st_UI_DSK_TOOL_SMAP_ROW rows[DSK_MAX_SECTORS];
    gboolean lock;
} st_UI_DSK_TOOL_SMAP;

static st_UI_DSK_TOOL_SMAP g_smap;


static void on_ui_dsk_tool_sector_map_spinbutton_value_changed ( GtkSpinButton *spin_button, gpointer user_data ) {

    if ( g_smap.lock ) return;

    ui_dsk_tool_watch_predef ( );

    int *id = user_data;
    st_UI_DSK_TOOL_SMAP_ROW *row = &g_smap.rows[*id];

    guint8 sector_id = gtk_adjustment_get_value ( row->adjustment_sector_id );

    char sector_id_txt[3];
    snprintf ( sector_id_txt, sizeof ( sector_id_txt ), "%02X", sector_id );

    g_smap.lock = TRUE;
    gtk_entry_set_text ( GTK_ENTRY ( row->entry_sector_id ), sector_id_txt );
    g_smap.lock = FALSE;
}


static guint8 ui_dsk_tool_a2hex ( char c ) {

    if ( c >= '0' && c <= '9' ) {
        return ( c - '0' );
    };

    if ( c >= 'a' && c <= 'f' ) {
        return ( c - 'a' + 0x0a );
    };

    if ( c >= 'A' && c <= 'F' ) {
        return ( c - 'A' + 0x0a );
    };

    return 0;
}


static void on_ui_dsk_tool_sector_map_entry_changed ( GtkEditable *ed, gpointer user_data ) {

    if ( g_smap.lock ) return;

    ui_dsk_tool_watch_predef ( );

    ui_hexeditable_changed ( ed, user_data );

    int *id = user_data;
    st_UI_DSK_TOOL_SMAP_ROW *row = &g_smap.rows[*id];

    const char *sector_id_txt = gtk_entry_get_text ( GTK_ENTRY ( row->entry_sector_id ) );
    int length = gtk_entry_get_text_length ( GTK_ENTRY ( row->entry_sector_id ) );

    guint8 sector_id = 0;

    if ( length == 1 ) {
        sector_id = ui_dsk_tool_a2hex ( sector_id_txt[0] );
    } else if ( length == 2 ) {
        sector_id = ( ui_dsk_tool_a2hex ( sector_id_txt[0] ) * 0x10 ) + ui_dsk_tool_a2hex ( sector_id_txt[1] );
    };

    g_smap.lock = TRUE;
    gtk_adjustment_set_value ( row->adjustment_sector_id, sector_id );
    g_smap.lock = FALSE;
}


gboolean on_ui_dsk_tool_sector_map_entry_focus_out ( GtkWidget *widget, GdkEvent *event, gpointer user_data ) {
    gint length = gtk_entry_get_text_length ( GTK_ENTRY ( widget ) );
    if ( length == 2 ) return FALSE;
    char filler[3];
    strncpy ( filler, "00", 3 );
    if ( length == 1 ) {
        strncpy ( &filler[1], gtk_entry_get_text ( GTK_ENTRY ( widget ) ), 1 );
    };
    gtk_entry_set_text ( GTK_ENTRY ( widget ), filler );
    return FALSE;
}


static void ui_dsk_tool_sector_map_show_window ( int rule_id ) {

    g_smap.rule_id = rule_id;
    g_smap.lock = FALSE;

    static gboolean is_initialized = FALSE;

    if ( !is_initialized ) {
        is_initialized = TRUE;

        GtkWidget *grid = ui_get_widget ( "grid_sector_map_rows" );

        int i;
        for ( i = 0; i < sizeof ( g_smap.rows ) / sizeof ( st_UI_DSK_TOOL_SMAP_ROW ); i++ ) {

            st_UI_DSK_TOOL_SMAP_ROW *row = &g_smap.rows[i];
            row->id = i;

            char label[11];
            snprintf ( label, sizeof (label ), "%d. sector", i + 1 );

            row->label_sector = gtk_label_new ( label );
            gtk_widget_show ( row->label_sector );
            gtk_grid_attach ( GTK_GRID ( grid ), row->label_sector, 0, i, 1, 1 );

            row->label_colon = gtk_label_new ( ":" );
            gtk_widget_show ( row->label_colon );
            gtk_grid_attach ( GTK_GRID ( grid ), row->label_colon, 1, i, 1, 1 );

            row->adjustment_sector_id = gtk_adjustment_new ( 0, 0x00, 0xff, 1, 10, 0 );

            row->spinbutton_sector_id = gtk_spin_button_new ( row->adjustment_sector_id, 1, 0 );
            gtk_widget_show ( row->spinbutton_sector_id );
            gtk_grid_attach ( GTK_GRID ( grid ), row->spinbutton_sector_id, 2, i, 1, 1 );
            gtk_spin_button_set_numeric ( GTK_SPIN_BUTTON ( row->spinbutton_sector_id ), TRUE );
            gtk_spin_button_set_snap_to_ticks ( GTK_SPIN_BUTTON ( row->spinbutton_sector_id ), TRUE );

            g_signal_connect ( (gpointer) row->spinbutton_sector_id, "value-changed", G_CALLBACK ( on_ui_dsk_tool_sector_map_spinbutton_value_changed ), &row->id );

            row->label_hexvalue = gtk_label_new ( " = 0x" );
            gtk_widget_show ( row->label_hexvalue );
            gtk_grid_attach ( GTK_GRID ( grid ), row->label_hexvalue, 3, i, 1, 1 );

            char *sector_id_txt = "00";

            row->entry_sector_id = gtk_entry_new ( );
            gtk_entry_set_max_length ( GTK_ENTRY ( row->entry_sector_id ), 2 );
            gtk_entry_set_width_chars ( GTK_ENTRY ( row->entry_sector_id ), 3 );
            gtk_entry_set_text ( GTK_ENTRY ( row->entry_sector_id ), sector_id_txt );
            gtk_widget_show ( row->entry_sector_id );
            gtk_grid_attach ( GTK_GRID ( grid ), row->entry_sector_id, 4, i, 1, 1 );

            g_signal_connect ( (gpointer) row->entry_sector_id, "changed", G_CALLBACK ( on_ui_dsk_tool_sector_map_entry_changed ), &row->id );
            g_signal_connect ( (gpointer) row->entry_sector_id, "focus-out-event", G_CALLBACK ( on_ui_dsk_tool_sector_map_entry_focus_out ), &row->id );
        };
    };

    int sectors = gtk_adjustment_get_value ( g_dsk_tool.rules[g_smap.rule_id].spinbutton_sectors_adj );

    int i;
    for ( i = 0; i < sizeof ( g_smap.rows ) / sizeof ( st_UI_DSK_TOOL_SMAP_ROW ); i++ ) {
        st_UI_DSK_TOOL_SMAP_ROW *row = &g_smap.rows[i];
        guint8 sector_id = g_dsk_tool.rules[rule_id].sector_map[i];
        gtk_adjustment_set_value ( row->adjustment_sector_id, sector_id );
        if ( i < sectors ) {
            gtk_widget_show ( row->label_sector );
            gtk_widget_show ( row->label_colon );
            gtk_widget_show ( row->spinbutton_sector_id );
            gtk_widget_show ( row->label_hexvalue );
            gtk_widget_show ( row->entry_sector_id );
        } else {
            gtk_widget_hide ( row->label_sector );
            gtk_widget_hide ( row->label_colon );
            gtk_widget_hide ( row->spinbutton_sector_id );
            gtk_widget_hide ( row->label_hexvalue );
            gtk_widget_hide ( row->entry_sector_id );
        };
    };

    GtkWidget *window = ui_get_widget ( "window_sector_map" );
    gtk_widget_show ( window );
}


void ui_dsk_tool_sector_map_hide_window ( void ) {
    GtkWidget *window = ui_get_widget ( "window_sector_map" );
    gtk_widget_hide ( window );
}


G_MODULE_EXPORT gboolean on_window_sector_map_delete_event ( GtkWidget *widget, GdkEvent *event, gpointer user_data ) {
    ui_dsk_tool_sector_map_hide_window ( );
    return TRUE;
}


G_MODULE_EXPORT gboolean on_window_sector_map_key_press_event ( GtkWidget *widget, GdkEventKey *event, gpointer user_data ) {
    /*
        if ( event->keyval == GDK_KEY_Escape ) {
            ui_dsk_tool_sector_map_hide_window ( );
            return TRUE;
            //} else if ( event->keyval == GDK_KEY_Return ) {
        };
     */
    return FALSE;
}


G_MODULE_EXPORT void on_button_sector_map_close_clicked ( GtkButton *button, gpointer user_data ) {
    ui_dsk_tool_sector_map_hide_window ( );
}


G_MODULE_EXPORT void on_button_sector_map_ok_clicked ( GtkButton *button, gpointer user_data ) {

    guint8 *map = g_dsk_tool.rules[g_smap.rule_id].sector_map;

    int sectors = gtk_adjustment_get_value ( g_dsk_tool.rules[g_smap.rule_id].spinbutton_sectors_adj );

    int i;
    for ( i = 0; i < sectors; i++ ) {
        st_UI_DSK_TOOL_SMAP_ROW *row = &g_smap.rows[i];
        map[i] = gtk_adjustment_get_value ( row->adjustment_sector_id );
    };

    ui_dsk_tool_sector_map_hide_window ( );
}


/*
 * 
 * dsk tool
 * 
 */

static void ui_dsk_tool_set_rule0_button ( int rules_count ) {
    GtkWidget *button = g_dsk_tool.rules[0].button_remove;
    gboolean sensitive = ( rules_count > 1 ) ? TRUE : FALSE;
    gtk_widget_set_sensitive ( button, sensitive );
}


static inline void ui_dsk_tool_refresh_rules_grid ( void ) {

    if ( g_dsk_tool.grid_rules != NULL ) {
        gtk_container_remove ( GTK_CONTAINER ( ui_get_widget ( "viewport_dsk_tool_rules" ) ), g_dsk_tool.grid_rules );
    };

    g_dsk_tool.grid_rules = gtk_grid_new ( );
    gtk_grid_set_column_spacing ( GTK_GRID ( g_dsk_tool.grid_rules ), 5 );
    gtk_grid_set_row_spacing ( GTK_GRID ( g_dsk_tool.grid_rules ), 5 );
    gtk_widget_show ( g_dsk_tool.grid_rules );
    gtk_container_add ( GTK_CONTAINER ( ui_get_widget ( "viewport_dsk_tool_rules" ) ), g_dsk_tool.grid_rules );
}

extern void on_ui_dsk_tool_delete_rule_clicked ( GtkButton *button, gpointer user_data );


static st_DSK_DESCRIPTION* ui_dsk_tool_create_dsk_description ( void ) {
    st_DSK_DESCRIPTION *dskdesc = ui_utils_mem_alloc ( dsk_tools_compute_description_size ( g_dsk_tool.rules_count ) );

    dskdesc->count_rules = g_dsk_tool.rules_count;
    dskdesc->sides = gtk_combo_box_get_active ( ui_get_combo_box ( "comboboxtext_dsk_new_sides" ) ) + 1;
    dskdesc->tracks = gtk_adjustment_get_value ( ui_get_adjustment ( "adjustment_dsk_tool_tracks" ) ) / dskdesc->sides;

    int rule_id;
    for ( rule_id = 0; rule_id < dskdesc->count_rules; rule_id++ ) {

        st_UI_DSK_TOOL_RULE *rule = &g_dsk_tool.rules[rule_id];

        guint8 from_track = gtk_adjustment_get_value ( rule->spinbutton_track_adj );
        guint8 sectors = gtk_adjustment_get_value ( rule->spinbutton_sectors_adj );
        en_DSK_SECTOR_SIZE ssize = gtk_combo_box_get_active ( GTK_COMBO_BOX ( rule->comboboxtext_ssize ) );
        en_DSK_SECTOR_ORDER_TYPE sector_order = gtk_combo_box_get_active ( GTK_COMBO_BOX ( rule->comboboxtext_sector_order ) );

        guint8 *sector_map = NULL;

        if ( sector_order == DSK_SEC_ORDER_CUSTOM ) {
            sector_map = rule->sector_map;
        };

        const char *filler_txt = gtk_entry_get_text ( GTK_ENTRY ( rule->entry_filler ) );
        guint8 filler = ( ui_dsk_tool_a2hex ( filler_txt[0] ) * 0x10 ) + ui_dsk_tool_a2hex ( filler_txt[1] );

        dsk_tools_assign_description ( dskdesc, rule_id, from_track, sectors, ssize, sector_order, sector_map, filler );
    };

    return dskdesc;
}

#ifndef HAVE_GTK_GRID_REMOVE_ROW
extern void ui_dsk_tool_add_rules_row ( void );
#endif


static void ui_dsk_tool_destroy_rules_row ( int rule_id ) {

#ifdef HAVE_GTK_GRID_REMOVE_ROW
    gtk_grid_remove_row ( GTK_GRID ( g_dsk_tool.grid_rules ), rule_id );
#endif

    ui_utils_mem_free ( g_dsk_tool.rules[rule_id].rule_id );

    g_dsk_tool.rules_count--;

    if ( g_dsk_tool.rules_count ) {
        GtkAdjustment *adj_tracks = ui_get_adjustment ( "adjustment_dsk_tool_tracks" );
        gint dsk_total_tracks = gtk_adjustment_get_value ( adj_tracks );
        int i;
        for ( i = rule_id; i < g_dsk_tool.rules_count; i++ ) {
            memcpy ( &g_dsk_tool.rules[i], &g_dsk_tool.rules[i + 1], sizeof ( st_UI_DSK_TOOL_RULE ) );
            *g_dsk_tool.rules[i].rule_id = i;
            gtk_adjustment_set_lower ( GTK_ADJUSTMENT ( g_dsk_tool.rules[i].spinbutton_track_adj ), i );
            gint track_upper_value = dsk_total_tracks - g_dsk_tool.rules_count + i;
            gtk_adjustment_set_upper ( GTK_ADJUSTMENT ( g_dsk_tool.rules[i].spinbutton_track_adj ), track_upper_value );
        };
        g_dsk_tool.rules = ui_utils_mem_realloc ( g_dsk_tool.rules, ( g_dsk_tool.rules_count ) * sizeof ( st_UI_DSK_TOOL_RULE ) );
        ui_dsk_tool_set_rule0_button ( g_dsk_tool.rules_count );

    } else {
        ui_utils_mem_free ( g_dsk_tool.rules );
    };

    gtk_adjustment_set_lower ( GTK_ADJUSTMENT ( g_dsk_tool.rules[0].spinbutton_track_adj ), 0 );
    gtk_adjustment_set_value ( GTK_ADJUSTMENT ( g_dsk_tool.rules[0].spinbutton_track_adj ), 0 );
    gtk_widget_set_sensitive ( g_dsk_tool.rules[0].spinbutton_track, FALSE );

#ifndef HAVE_GTK_GRID_REMOVE_ROW
    st_DSK_DESCRIPTION *dskdesc = ui_dsk_tool_create_dsk_description ( );
    ui_dsk_tool_refresh_rules_grid ( );

    while ( g_dsk_tool.rules_count ) {
        ui_utils_mem_free ( g_dsk_tool.rules[( g_dsk_tool.rules_count - 1 )].rule_id );
        g_dsk_tool.rules_count--;
        g_dsk_tool.rules = ui_utils_mem_realloc ( g_dsk_tool.rules, ( g_dsk_tool.rules_count ) * sizeof ( st_UI_DSK_TOOL_RULE ) );
    };

    st_DSK_DESCRIPTION_RULE *dscrules = ( st_DSK_DESCRIPTION_RULE* ) & dskdesc->rules;

    int i;
    for ( i = 0; i < dskdesc->count_rules; i++ ) {
        ui_dsk_tool_add_rules_row ( );
        st_UI_DSK_TOOL_RULE *rule = &g_dsk_tool.rules[i];
        st_DSK_DESCRIPTION_RULE *dsc = &dscrules[i];


        gtk_adjustment_set_value ( rule->spinbutton_track_adj, dsc->absolute_track );
        gtk_adjustment_set_value ( rule->spinbutton_sectors_adj, dsc->sectors );
        gtk_combo_box_set_active ( GTK_COMBO_BOX ( rule->comboboxtext_ssize ), dsc->ssize );
        gtk_combo_box_set_active ( GTK_COMBO_BOX ( rule->comboboxtext_sector_order ), dsc->sector_order );

        char filler_txt[3];
        snprintf ( filler_txt, 3, "%02X", dsc->filler );
        gtk_entry_set_text ( GTK_ENTRY ( rule->entry_filler ), filler_txt );

        if ( dsc->sector_order == DSK_SEC_ORDER_CUSTOM ) {
            memcpy ( rule->sector_map, dsc->sector_map, sizeof (guint8 ) * dsc->sectors );
        };
    };

    ui_utils_mem_free ( dskdesc );
#endif
}


void on_ui_dsk_tool_delete_rule_clicked ( GtkButton *button, gpointer user_data ) {
    ui_dsk_tool_watch_predef ( );
    int *rule_id = user_data;
    ui_dsk_tool_destroy_rules_row ( *rule_id );
}


static void on_ui_dsk_tool_ssize_changed ( GtkComboBox *combobox, gpointer user_data ) {
    ui_dsk_tool_watch_predef ( );
}


static void on_ui_dsk_tool_sector_order_changed ( GtkComboBox *combobox, gpointer user_data ) {
    ui_dsk_tool_watch_predef ( );
    int *rule_id = user_data;
    en_DSK_SECTOR_ORDER_TYPE sector_order = gtk_combo_box_get_active ( combobox );
    if ( sector_order == DSK_SEC_ORDER_CUSTOM ) {
        gtk_widget_set_sensitive ( g_dsk_tool.rules[*rule_id].button_edit_map, TRUE );
    } else {
        gtk_widget_set_sensitive ( g_dsk_tool.rules[*rule_id].button_edit_map, FALSE );
    };
}


static void on_ui_dsk_tool_edit_map_clicked ( GtkButton *button, gpointer user_data ) {
    int *rule_id = user_data;
    ui_dsk_tool_sector_map_show_window ( *rule_id );
}


static inline void ui_dsk_tool_addbutton_sensitivity ( void ) {
    GtkAdjustment *adj_tracks = ui_get_adjustment ( "adjustment_dsk_tool_tracks" );
    gint dsk_total_abs_tracks = gtk_adjustment_get_value ( adj_tracks );

    gint top_rule_value = gtk_adjustment_get_value ( GTK_ADJUSTMENT ( g_dsk_tool.rules[g_dsk_tool.rules_count - 1].spinbutton_track_adj ) );

    //GtkWidget *add_rule_button = g_ui_dsk_tool_button_add;
    GtkWidget *add_rule_button = ui_get_widget ( "button_dsk_new_add_rule" );

    if ( top_rule_value >= dsk_total_abs_tracks - 1 ) {
        gtk_widget_set_sensitive ( add_rule_button, FALSE );
    } else {
        gtk_widget_set_sensitive ( add_rule_button, TRUE );
    };
}


static void on_ui_dsk_tool_spinbutton_sectors_value_changed ( GtkSpinButton *spin_button, gpointer user_data ) {
    ui_dsk_tool_watch_predef ( );
}


static void on_ui_dsk_tool_spinbutton_track_value_changed ( GtkSpinButton *spin_button, gpointer user_data ) {

    ui_dsk_tool_watch_predef ( );

    int *rule_id = user_data;
    if ( *rule_id == 0 ) return;

    gint value = gtk_adjustment_get_value ( gtk_spin_button_get_adjustment ( spin_button ) );

    if ( *rule_id > 1 ) {
        gint ruledown_value = gtk_adjustment_get_value ( GTK_ADJUSTMENT ( g_dsk_tool.rules[*rule_id - 1].spinbutton_track_adj ) );
        if ( ruledown_value >= value ) {
            gtk_adjustment_set_value ( GTK_ADJUSTMENT ( g_dsk_tool.rules[*rule_id - 1].spinbutton_track_adj ), value - 1 );
        };
    };

    if ( *rule_id < g_dsk_tool.rules_count - 1 ) {
        gint ruleup_value = gtk_adjustment_get_value ( GTK_ADJUSTMENT ( g_dsk_tool.rules[*rule_id + 1].spinbutton_track_adj ) );
        if ( ruleup_value <= value ) {
            gtk_adjustment_set_value ( GTK_ADJUSTMENT ( g_dsk_tool.rules[*rule_id + 1].spinbutton_track_adj ), value + 1 );
        };
    };

    ui_dsk_tool_addbutton_sensitivity ( );
}


static void on_ui_dsk_tool_entry_filler_changed ( GtkEditable *ed, gpointer user_data ) {
    ui_dsk_tool_watch_predef ( );
    ui_hexeditable_changed ( ed, user_data );
}


gboolean on_ui_dsk_tool_entry_filler_focus_out ( GtkWidget *widget, GdkEvent *event, gpointer user_data ) {
    gint length = gtk_entry_get_text_length ( GTK_ENTRY ( widget ) );
    if ( length == 2 ) return FALSE;
    char filler[3];
    strncpy ( filler, "00", 3 );
    if ( length == 1 ) {
        strncpy ( &filler[1], gtk_entry_get_text ( GTK_ENTRY ( widget ) ), 1 );
    };
    gtk_entry_set_text ( GTK_ENTRY ( widget ), filler );
    return FALSE;
}


static void ui_dsk_tool_create_rules_row ( int rule_id ) {

    st_UI_DSK_TOOL_RULE *rule = &g_dsk_tool.rules[rule_id];

    rule->rule_id = ui_utils_mem_alloc ( sizeof ( int ) );
    *rule->rule_id = rule_id;

    gint from_track = 0;
    gint track_lower_value = rule_id;
    gint predefined_sectors = 16;
    gint predefined_ssize = DSK_SECTOR_SIZE_256;
    gint sector_order = DSK_SEC_ORDER_NORMAL;
    char predefined_filler[3] = "00";

    if ( rule_id != 0 ) {
        from_track = gtk_adjustment_get_value ( GTK_ADJUSTMENT ( g_dsk_tool.rules[rule_id - 1].spinbutton_track_adj ) ) + 1;
        predefined_sectors = gtk_adjustment_get_value ( GTK_ADJUSTMENT ( g_dsk_tool.rules[rule_id - 1].spinbutton_sectors_adj ) );
        predefined_ssize = gtk_combo_box_get_active ( GTK_COMBO_BOX ( g_dsk_tool.rules[rule_id - 1].comboboxtext_ssize ) );
        sector_order = gtk_combo_box_get_active ( GTK_COMBO_BOX ( g_dsk_tool.rules[rule_id - 1].comboboxtext_sector_order ) );
        strncpy ( predefined_filler, gtk_entry_get_text ( GTK_ENTRY ( g_dsk_tool.rules[rule_id - 1].entry_filler ) ), sizeof ( predefined_filler ) );
    };

    if ( ( rule_id != 0 ) && ( sector_order == DSK_SEC_ORDER_CUSTOM ) ) {
        memcpy ( rule->sector_map, g_dsk_tool.rules[rule_id - 1].sector_map, sizeof (rule->sector_map ) );
    } else {
        gint8 i;
        for ( i = 0; i < sizeof (rule->sector_map ); i++ ) {
            rule->sector_map[i] = i + 1;
        };
    };

    gint column = 0;

    rule->label_track = gtk_label_new ( "From track: " );
    gtk_widget_show ( rule->label_track );
    gtk_grid_attach ( GTK_GRID ( g_dsk_tool.grid_rules ), rule->label_track, column++, rule_id, 1, 1 );

    GtkAdjustment *adj_tracks = ui_get_adjustment ( "adjustment_dsk_tool_tracks" );
    gint dsk_total_abs_tracks = gtk_adjustment_get_value ( adj_tracks );

    rule->spinbutton_track_adj = gtk_adjustment_new ( from_track, track_lower_value, dsk_total_abs_tracks - 1, 1, 10, 0 );

    rule->spinbutton_track = gtk_spin_button_new ( rule->spinbutton_track_adj, 1, 0 );
    gtk_widget_show ( rule->spinbutton_track );
    gtk_grid_attach ( GTK_GRID ( g_dsk_tool.grid_rules ), rule->spinbutton_track, column++, rule_id, 1, 1 );
    gtk_spin_button_set_numeric ( GTK_SPIN_BUTTON ( rule->spinbutton_track ), TRUE );
    gtk_spin_button_set_snap_to_ticks ( GTK_SPIN_BUTTON ( rule->spinbutton_track ), TRUE );

    if ( rule_id == 0 ) {
        gtk_widget_set_sensitive ( rule->spinbutton_track, FALSE );
    };

    g_signal_connect ( (gpointer) rule->spinbutton_track, "value-changed", G_CALLBACK ( on_ui_dsk_tool_spinbutton_track_value_changed ), rule->rule_id );

    rule->label_sectors = gtk_label_new ( "Sectors: " );
    gtk_widget_show ( rule->label_sectors );
    gtk_grid_attach ( GTK_GRID ( g_dsk_tool.grid_rules ), rule->label_sectors, column++, rule_id, 1, 1 );

    rule->spinbutton_sectors_adj = gtk_adjustment_new ( predefined_sectors, 1, DSK_MAX_SECTORS, 1, 10, 0 );

    rule->spinbutton_sectors = gtk_spin_button_new ( rule->spinbutton_sectors_adj, 1, 0 );
    gtk_widget_show ( rule->spinbutton_sectors );
    gtk_grid_attach ( GTK_GRID ( g_dsk_tool.grid_rules ), rule->spinbutton_sectors, column++, rule_id, 1, 1 );
    gtk_spin_button_set_numeric ( GTK_SPIN_BUTTON ( rule->spinbutton_sectors ), TRUE );
    gtk_spin_button_set_snap_to_ticks ( GTK_SPIN_BUTTON ( rule->spinbutton_sectors ), TRUE );

    g_signal_connect ( (gpointer) rule->spinbutton_sectors, "value-changed", G_CALLBACK ( on_ui_dsk_tool_spinbutton_sectors_value_changed ), rule->rule_id );

    rule->comboboxtext_ssize = gtk_combo_box_text_new ( );
    gtk_widget_show ( rule->comboboxtext_ssize );
    gtk_combo_box_text_append ( GTK_COMBO_BOX_TEXT ( rule->comboboxtext_ssize ), NULL, "ssize 128 B" );
    gtk_combo_box_text_append ( GTK_COMBO_BOX_TEXT ( rule->comboboxtext_ssize ), NULL, "ssize 256 B" );
    gtk_combo_box_text_append ( GTK_COMBO_BOX_TEXT ( rule->comboboxtext_ssize ), NULL, "ssize 512 B" );
    gtk_combo_box_text_append ( GTK_COMBO_BOX_TEXT ( rule->comboboxtext_ssize ), NULL, "ssize 1024 B" );
    gtk_combo_box_set_active ( GTK_COMBO_BOX ( rule->comboboxtext_ssize ), predefined_ssize );
    gtk_grid_attach ( GTK_GRID ( g_dsk_tool.grid_rules ), rule->comboboxtext_ssize, column++, rule_id, 1, 1 );

    g_signal_connect ( (gpointer) rule->comboboxtext_ssize, "changed", G_CALLBACK ( on_ui_dsk_tool_ssize_changed ), rule->rule_id );

    rule->comboboxtext_sector_order = gtk_combo_box_text_new ( );
    gtk_widget_show ( rule->comboboxtext_sector_order );
    gtk_combo_box_text_append ( GTK_COMBO_BOX_TEXT ( rule->comboboxtext_sector_order ), NULL, "Custom map" );
    gtk_combo_box_text_append ( GTK_COMBO_BOX_TEXT ( rule->comboboxtext_sector_order ), NULL, "Normal" );
    gtk_combo_box_text_append ( GTK_COMBO_BOX_TEXT ( rule->comboboxtext_sector_order ), NULL, "Lec interlaced" );
    gtk_combo_box_text_append ( GTK_COMBO_BOX_TEXT ( rule->comboboxtext_sector_order ), NULL, "Lec-HD interlaced" );
    gtk_grid_attach ( GTK_GRID ( g_dsk_tool.grid_rules ), rule->comboboxtext_sector_order, column++, rule_id, 1, 1 );

    rule->button_edit_map = gtk_button_new_with_label ( "Map" );
    gtk_widget_show ( rule->button_edit_map );
    gtk_grid_attach ( GTK_GRID ( g_dsk_tool.grid_rules ), rule->button_edit_map, column++, rule_id, 1, 1 );

    g_signal_connect ( (gpointer) rule->button_edit_map, "clicked", G_CALLBACK ( on_ui_dsk_tool_edit_map_clicked ), rule->rule_id );

    g_signal_connect ( (gpointer) rule->comboboxtext_sector_order, "changed", G_CALLBACK ( on_ui_dsk_tool_sector_order_changed ), rule->rule_id );
    gtk_combo_box_set_active ( GTK_COMBO_BOX ( rule->comboboxtext_sector_order ), sector_order );

    rule->label_filler = gtk_label_new ( "Fill: 0x" );
    gtk_widget_show ( rule->label_filler );
    gtk_grid_attach ( GTK_GRID ( g_dsk_tool.grid_rules ), rule->label_filler, column++, rule_id, 1, 1 );

    rule->entry_filler = gtk_entry_new ( );
    gtk_entry_set_max_length ( GTK_ENTRY ( rule->entry_filler ), 2 );
    gtk_entry_set_width_chars ( GTK_ENTRY ( rule->entry_filler ), 3 );
    gtk_entry_set_text ( GTK_ENTRY ( rule->entry_filler ), predefined_filler );
    gtk_widget_show ( rule->entry_filler );
    gtk_grid_attach ( GTK_GRID ( g_dsk_tool.grid_rules ), rule->entry_filler, column++, rule_id, 1, 1 );

    g_signal_connect ( (gpointer) rule->entry_filler, "changed", G_CALLBACK ( on_ui_dsk_tool_entry_filler_changed ), rule->rule_id );
    g_signal_connect ( (gpointer) rule->entry_filler, "focus-out-event", G_CALLBACK ( on_ui_dsk_tool_entry_filler_focus_out ), rule->rule_id );

    rule->separator = gtk_separator_new ( GTK_ORIENTATION_VERTICAL );
    gtk_widget_show ( rule->separator );
    gtk_grid_attach ( GTK_GRID ( g_dsk_tool.grid_rules ), rule->separator, column++, rule_id, 1, 1 );

    rule->button_remove = gtk_button_new_with_label ( "Remove rule" );
    gtk_widget_show ( rule->button_remove );
    gtk_grid_attach ( GTK_GRID ( g_dsk_tool.grid_rules ), rule->button_remove, column++, rule_id, 1, 1 );

    g_signal_connect ( (gpointer) rule->button_remove, "clicked", G_CALLBACK ( on_ui_dsk_tool_delete_rule_clicked ), rule->rule_id );
}


void ui_dsk_tool_add_rules_row ( void ) {

    if ( g_dsk_tool.rules != NULL ) {
        g_dsk_tool.rules = ui_utils_mem_realloc ( g_dsk_tool.rules, ( g_dsk_tool.rules_count + 1 ) * sizeof ( st_UI_DSK_TOOL_RULE ) );
    } else {
        g_dsk_tool.rules = ui_utils_mem_alloc ( ( g_dsk_tool.rules_count + 1 ) * sizeof ( st_UI_DSK_TOOL_RULE ) );
    };

    ui_dsk_tool_create_rules_row ( g_dsk_tool.rules_count++ );
    ui_dsk_tool_set_rule0_button ( g_dsk_tool.rules_count );

    GtkAdjustment *adj_tracks = ui_get_adjustment ( "adjustment_dsk_tool_tracks" );
    gint dsk_total_abs_tracks = gtk_adjustment_get_value ( adj_tracks );

    int i;
    for ( i = 0; i < g_dsk_tool.rules_count; i++ ) {
        gint track_upper_value = dsk_total_abs_tracks - g_dsk_tool.rules_count + i;
        gtk_adjustment_set_upper ( GTK_ADJUSTMENT ( g_dsk_tool.rules[i].spinbutton_track_adj ), track_upper_value );
    };

    ui_dsk_tool_addbutton_sensitivity ( );
}


static void ui_dsk_tool_set_sides ( gint sides ) {

    static gboolean lock = FALSE;
    if ( lock ) return;
    lock = TRUE;

    gtk_combo_box_set_active ( ui_get_combo_box ( "comboboxtext_dsk_new_sides" ), sides - 1 );

    GtkAdjustment *adj_tracks = ui_get_adjustment ( "adjustment_dsk_tool_tracks" );

    gint min_tracks = sides;
    gint max_tracks = DSK_MAX_TOTAL_TRACKS;

    gtk_adjustment_set_lower ( adj_tracks, min_tracks );
    gtk_adjustment_set_upper ( adj_tracks, max_tracks );
    gtk_adjustment_set_step_increment ( adj_tracks, sides );

    gint dsk_abs_tracks = gtk_adjustment_get_value ( adj_tracks );

    if ( dsk_abs_tracks < min_tracks ) {
        dsk_abs_tracks = min_tracks;
    };

    if ( dsk_abs_tracks > max_tracks ) {
        dsk_abs_tracks = max_tracks;
    };

    if ( ( sides == 2 ) && ( dsk_abs_tracks & 1 ) ) {
        dsk_abs_tracks++;
    };

    gtk_adjustment_set_value ( adj_tracks, dsk_abs_tracks );

    lock = FALSE;
}


static void ui_dsk_tool_remove_unnecessary_rules ( void ) {
    while ( g_dsk_tool.rules_count > 1 ) {
        ui_dsk_tool_destroy_rules_row ( g_dsk_tool.rules_count - 1 );
    };
}


void ui_dsk_tool_show_window ( void ) {

    GtkWidget *window = ui_get_widget ( "window_dsk_tool" );
    if ( gtk_widget_is_visible ( window ) ) return;

    GtkEntry *entry = ui_get_entry ( "entry_dsk_new_filename" );
    gtk_entry_set_text ( entry, UI_DSK_TOOL_NEW_IMAGE_NAME );

    gtk_widget_grab_focus ( GTK_WIDGET ( entry ) );
    gint len = strlen ( UI_DSK_TOOL_NEW_IMAGE_NAME );
    gtk_editable_set_position ( GTK_EDITABLE ( entry ), len );
    gtk_editable_select_region ( GTK_EDITABLE ( entry ), 0, -1 );

    GtkWidget *fcdialog = ui_get_widget ( "filechooserbutton_dsk_new" );
    gtk_file_chooser_set_current_folder ( GTK_FILE_CHOOSER ( fcdialog ), ui_filechooser_get_last_dsk_dir ( ) );

    g_dsk_tool.lock_predef = TRUE;

    //do_list_store ( );

    static gboolean is_initialized = FALSE;
    if ( !is_initialized ) {

        is_initialized = TRUE;
        g_dsk_tool.rules_count = 0;

        g_dsk_tool.grid_rules = NULL;

        ui_dsk_tool_refresh_rules_grid ( );

        GtkWidget *cmbbt_format = ui_get_widget ( "comboboxtext_dsk_new_predefined" );
        //gtk_combo_box_text_remove_all ( GTK_COMBO_BOX_TEXT ( cmbbt_format ) );

        gint predef_count = sizeof (g_predefined ) / sizeof (st_UI_DSK_TOOL_PREDEFINED );

        int i;
        for ( i = 0; i < predef_count; i++ ) {
            gtk_combo_box_text_append ( GTK_COMBO_BOX_TEXT ( cmbbt_format ), &g_predefined[i].id, g_predefined[i].predef->name );
        };

        ui_dsk_tool_add_rules_row ( );
        ui_dsk_tool_set_sides ( 2 );
        gtk_adjustment_set_value ( ui_get_adjustment ( "adjustment_dsk_tool_tracks" ), 160 );
    };

    g_dsk_tool.lock_predef = FALSE;

    gtk_combo_box_set_active ( ui_get_combo_box ( "comboboxtext_dsk_new_predefined" ), 1 );

    ui_dsk_tool_addbutton_sensitivity ( );

    gtk_widget_show ( window );
}


static void ui_dsk_tool_hide_window ( void ) {
    GtkWidget *window = ui_get_widget ( "window_dsk_tool" );
    gtk_widget_hide ( window );
    ui_dsk_tool_remove_unnecessary_rules ( );
}


G_MODULE_EXPORT gboolean on_window_dsk_tool_delete_event ( GtkWidget *widget, GdkEvent *event, gpointer user_data ) {
    ui_dsk_tool_hide_window ( );
    return TRUE;
}


G_MODULE_EXPORT gboolean on_window_dsk_tool_key_press_event ( GtkWidget *widget, GdkEventKey *event, gpointer user_data ) {
    /*
        if ( event->keyval == GDK_KEY_Escape ) {
            ui_dsk_tool_hide_window ( );
            return TRUE;
        } else if ( event->keyval == GDK_KEY_Return ) {
            //gtk_button_clicked ( GTK_BUTTON ( ui_get_widget ( "button_dsk_new_ok" ) ) );

        };
     */
    return FALSE;
}


G_MODULE_EXPORT void on_button_dsk_new_close_clicked ( GtkButton *button, gpointer user_data ) {
    ui_dsk_tool_hide_window ( );
}


static int ui_dsk_tool_create_dsk ( char *path, char *filename, st_DSK_DESCRIPTION *dskdesc ) {

    st_DRIVER *driver = &g_ui_file_driver;

    int path_length = strlen ( path );
    int filename_length = strlen ( filename );

    char *filepath = ui_utils_mem_alloc0 ( path_length + 1 + filename_length + 1 );

    strncpy ( filepath, path, path_length );
    strncat ( filepath, "/", 2 );
    strncat ( filepath, filename, filename_length );

    if ( -1 != ui_utils_file_access ( filepath, F_OK ) ) {
        if ( EXIT_FAILURE == ui_show_yesno_dialog ( "File '%s' already exists. Overwrite it?", filename ) ) {
            ui_utils_mem_free ( filepath );
            return EXIT_FAILURE;
        };
    };

    st_HANDLER *handler = generic_driver_open_file ( NULL, driver, filepath, FILE_DRIVER_OPMODE_W );

    ui_utils_mem_free ( filepath );

    if ( ( !handler ) || ( handler->err ) || ( driver->err ) ) {
        ui_show_error ( "%s() - Can't open file handler '%s': %s\n", __func__, filename, generic_driver_error_message ( handler, driver ) );
        return EXIT_FAILURE;
    };

    printf ( "Creating new FD image '%s'...\n", filename );

    if ( EXIT_SUCCESS != dsk_tools_create_image ( handler, dskdesc ) ) {
        ui_show_error ( "%s() - Can't create DSK image: %s\n", __func__, dsk_error_message ( handler, driver ) );
        return EXIT_FAILURE;
    };

    printf ( "Done.\n" );

    generic_driver_close ( handler );

    ui_utils_mem_free ( handler );

    return EXIT_SUCCESS;
}


G_MODULE_EXPORT void on_button_dsk_new_ok_clicked ( GtkButton *button, gpointer user_data ) {

    int length = gtk_entry_get_text_length ( ui_get_entry ( "entry_dsk_new_filename" ) );

    if ( !length ) {
        ui_show_error ( "Filename of new FD image can't be empty!" );
        return;
    };

    char *filename = ui_utils_mem_alloc0 ( length + 5 );

    strncpy ( filename, gtk_entry_get_text ( ui_get_entry ( "entry_dsk_new_filename" ) ), length + 1 );

    if ( ( length < 4 ) || ( 0 != strcasecmp ( &filename[length - 4], ".dsk" ) ) ) {
        strncat ( filename, ".dsk", 5 );
    };

    char *path = gtk_file_chooser_get_filename ( GTK_FILE_CHOOSER ( ui_get_widget ( "filechooserbutton_dsk_new" ) ) );

    ui_filechooser_set_last_mzq_dir ( path );

    st_DSK_DESCRIPTION *dskdesc = ui_dsk_tool_create_dsk_description ( );

    if ( EXIT_SUCCESS == ui_dsk_tool_create_dsk ( path, filename, dskdesc ) ) {
        ui_dsk_tool_hide_window ( );
    };

    ui_utils_mem_free ( filename );
    ui_utils_mem_free ( dskdesc );
}


G_MODULE_EXPORT void on_button_dsk_new_add_rule_clicked ( GtkButton *button, gpointer user_data ) {
    ui_dsk_tool_watch_predef ( );
    ui_dsk_tool_add_rules_row ( );
}


G_MODULE_EXPORT void on_comboboxtext_dsk_new_sides_changed ( GtkComboBox *combobox, gpointer data ) {
    ui_dsk_tool_watch_predef ( );
    gint sides = gtk_combo_box_get_active ( combobox ) + 1;
    ui_dsk_tool_set_sides ( sides );
}


G_MODULE_EXPORT void on_spinbutton_dsk_new_tracks_value_changed ( GtkSpinButton *spin_button, gpointer data ) {

    ui_dsk_tool_watch_predef ( );

    if ( !g_dsk_tool.rules_count ) return;

    GtkAdjustment *adj_tracks = gtk_spin_button_get_adjustment ( spin_button );
    gint dsk_abs_tracks = gtk_adjustment_get_value ( adj_tracks );

    while ( g_dsk_tool.rules_count > dsk_abs_tracks ) {
        ui_dsk_tool_destroy_rules_row ( g_dsk_tool.rules_count - 1 );
    };

    gint max_abs_track = dsk_abs_tracks - 1;

    GtkAdjustment *last_adj = GTK_ADJUSTMENT ( g_dsk_tool.rules[g_dsk_tool.rules_count - 1].spinbutton_track_adj );
    if ( gtk_adjustment_get_value ( last_adj ) > max_abs_track ) {
        gtk_adjustment_set_value ( last_adj, max_abs_track );
    };

    ui_dsk_tool_addbutton_sensitivity ( );
}


G_MODULE_EXPORT void on_comboboxtext_dsk_new_predefined_changed ( GtkComboBox *combobox, gpointer data ) {

    if ( g_dsk_tool.lock_predef ) return;

    g_dsk_tool.lock_predef = TRUE;

    ui_dsk_tool_remove_unnecessary_rules ( );

    gint active_prdef = gtk_combo_box_get_active ( combobox );

    const st_UI_DSK_TOOL_PREDEFINED_ROW *predef = g_predefined[active_prdef].predef;

    ui_dsk_tool_set_sides ( predef->sides );
    gtk_adjustment_set_value ( ui_get_adjustment ( "adjustment_dsk_tool_tracks" ), predef->tracks );

    int rule_id = 0;

    while ( predef->rules[rule_id].sectors != 0 ) {

        st_UI_DSK_TOOL_PREDEFINED_RULE *predef_rule = &predef->rules[rule_id];

        if ( !( g_dsk_tool.rules_count > rule_id ) ) {
            ui_dsk_tool_add_rules_row ( );
        };

        st_UI_DSK_TOOL_RULE *rule = &g_dsk_tool.rules[rule_id];

        gtk_adjustment_set_value ( rule->spinbutton_track_adj, predef_rule->from_track );
        gtk_adjustment_set_value ( rule->spinbutton_sectors_adj, predef_rule->sectors );
        gtk_combo_box_set_active ( GTK_COMBO_BOX ( rule->comboboxtext_ssize ), predef_rule->ssize );
        gtk_combo_box_set_active ( GTK_COMBO_BOX ( rule->comboboxtext_sector_order ), predef_rule->sector_order );

        if ( predef_rule->sector_map != NULL ) {
            memcpy ( rule->sector_map, predef_rule->sector_map, predef_rule->sectors );
        } else {
            int i;
            for ( i = 0; i < DSK_MAX_SECTORS; i++ ) {
                rule->sector_map[i] = i + 1;
            };
        };

        char filler[3];
        snprintf ( filler, sizeof ( filler ), "%02X", predef_rule->filler );

        gtk_entry_set_text ( GTK_ENTRY ( rule->entry_filler ), filler );

        rule_id++;
    };

    ui_dsk_tool_addbutton_sensitivity ( );

    g_dsk_tool.lock_predef = FALSE;
}
