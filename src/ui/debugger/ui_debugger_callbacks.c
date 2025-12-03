/* 
 * File:   ui_debugger_callbacks.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 24. srpna 2015, 11:04
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

#include <stdio.h>
#include <gtk/gtk.h>

#ifdef MZ800EMU_CFG_DEBUGGER_ENABLED

#include <string.h>
#include <strings.h>

#include "ui/ui_main.h"
#include "ui/ui_file_chooser.h"
#include "debugger/debugger.h"
#include "mz800.h"
#include "ui_debugger.h"
#include "ui_debugger_iasm.h"
#include "memory/memory.h"
#include "memory/memext.h"
#include "ui_dbg_memext.h"
#include "ui_breakpoints.h"
#include "debugger/breakpoints.h"
#include "ui_membrowser.h"
#include "ui/ui_hexeditable.h"
#include "pio8255/pio8255.h"
#include "z80ex/include/z80ex_dasm.h"
#include "ui_dissassembler.h"
#include "ui/ui_utils.h"
#include "libs/mzf/mzf.h"
#include "libs/mzf/mzf_tools.h"
#include "ui/ui_utils.h"
#include "libs/generic_driver/generic_driver.h"
#include "ui/generic_driver/ui_memory_driver.h"

#include "gdg/hwscroll.h"

gboolean g_ui_debugger_callbacks_hwscroll_entry_lock = FALSE;


static st_DRIVER *g_driver = &g_ui_memory_driver_static;


static Z80EX_WORD ui_debugger_dissassembled_get_selected_addr ( void ) {
    GtkTreeModel *model = GTK_TREE_MODEL ( ui_get_object ( "dbg_disassembled_liststore" ) );
    GtkTreeSelection *selection = gtk_tree_view_get_selection ( ui_get_tree_view ( "dbg_disassembled_treeview" ) );
    GtkTreeIter iter;
    gtk_tree_selection_get_selected ( selection, &model, &iter );
    GValue gv_addr = G_VALUE_INIT;
    gtk_tree_model_get_value ( model, &iter, DBG_DIS_ADDR, &gv_addr );
    Z80EX_WORD addr = (Z80EX_WORD) g_value_get_uint ( &gv_addr );
    return addr;
}


G_MODULE_EXPORT void on_debugger_main_window_size_allocate ( GtkWidget *widget, GdkRectangle *allocation, gpointer user_data ) {
    if ( g_debugger.animated_updates != DEBUGGER_ANIMATED_UPDATES_DISABLED ) return;
    if ( g_debugger.run_to_temporary_breakpoint ) return;
    ui_debugger_show_spinner_window ( );
}


G_MODULE_EXPORT gboolean on_debugger_main_window_configure_event ( GtkWidget *widget, GdkEventConfigure *event ) {
    if ( g_debugger.animated_updates == DEBUGGER_ANIMATED_UPDATES_DISABLED ) ui_debugger_show_spinner_window ( );
    return FALSE;
}


G_MODULE_EXPORT gboolean on_debugger_main_window_delete ( GtkWidget *widget, GdkEvent *event, gpointer user_data ) {
    g_uidebugger.accelerators_locked = 0;
    debugger_show_hide_main_window ( );
    return TRUE;
}


G_MODULE_EXPORT gboolean on_debugger_main_window_key_press_event ( GtkWidget *widget, GdkEventKey *event, gpointer user_data ) {
    if ( g_uidebugger.accelerators_locked != 0 ) return FALSE;

    if ( event->keyval == GDK_KEY_Escape ) {
        debugger_show_hide_main_window ( );
        return TRUE;
    };

    return FALSE;
}


G_MODULE_EXPORT void on_dbg_mem_load_mzf_menuitem_activate ( GtkCheckMenuItem *menuitem, gpointer user_data ) {
    (void) menuitem;
    (void) user_data;

    /*
        if ( !TEST_EMULATION_PAUSED ) {
            mz800_pause_emulation ( 1 );
        };
     */

    char *filename = ui_file_chooser_open_mzf ( "", (void*) ui_get_window ( "debugger_main_window" ) );
    if ( filename == NULL ) return;

    st_HANDLER *h = generic_driver_open_memory_from_file ( NULL, g_driver, filename );

    if ( !h ) {
        ui_show_error ( "Can't open MZF '%s'\n", filename );
        g_free ( filename );
        return;
    };

    g_free ( filename );

    st_MZF_HEADER mzfhdr;

    if ( EXIT_SUCCESS != mzf_read_header ( h, &mzfhdr ) ) {
        ui_show_error ( "Can't read MZF header\n" );
        generic_driver_close ( h );
        return;
    };

    char ascii_filename[MZF_FNAME_FULL_LENGTH];
    mzf_tools_get_fname ( &mzfhdr, ascii_filename );
    printf ( "\nDebugger load MZF body into RAM\n" );
    printf ( "fname: %s\n", ascii_filename );
    printf ( "ftype: 0x%02x\n", mzfhdr.ftype );
    printf ( "fstrt: 0x%04x\n", mzfhdr.fstrt );
    printf ( "fsize: 0x%04x\n", mzfhdr.fsize );
    printf ( "fexec: 0x%04x\n", mzfhdr.fexec );

    uint8_t data[0x10000];
    if ( EXIT_SUCCESS != mzf_read_body ( h, data, mzfhdr.fsize ) ) {
        ui_show_error ( "Can't read MZF body\n" );
        generic_driver_close ( h );
        return;
    };

    generic_driver_close ( h );

    memory_load_block ( data, mzfhdr.fstrt, mzfhdr.fsize, MEMORY_LOAD_RAMONLY );

    printf ( "\nDebugger load MZF - Done.\n" );
    ui_debugger_update_disassembled ( ui_debugger_dissassembled_get_first_addr ( ), -1 );
    ui_debugger_update_stack ( );
}


G_MODULE_EXPORT void on_dbg_hide_menuitem_activate ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
    if ( g_uidebugger.accelerators_locked != 0 ) return;
    debugger_show_hide_main_window ( );
}


G_MODULE_EXPORT void on_dbg_reset_menuitem_activate ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
    mz800_reset ( );
}


G_MODULE_EXPORT void on_dbg_speed_menuitem_activate ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
    mz800_switch_emulation_speed ( ( ~g_mz800.use_max_emulation_speed ) & 0x01 );
}


G_MODULE_EXPORT void on_dbg_pause_switch_menuitem_activate ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
    mz800_pause_emulation ( ( ~g_mz800.emulation_paused ) & 0x01 );
}


G_MODULE_EXPORT void on_dbg_animated_enabled_without_audio_radiomenuitem_toggled ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;

    if ( g_uidebugger.accelerators_locked != 0 ) return;
    g_debugger.animated_updates = DEBUGGER_ANIMATED_UPDATES_ENABLED_WTHOUT_AUDIO;
    ui_debugger_hide_spinner_window ( );
    debugger_step_call ( 0 );
    printf ( "Debugger animations ENABLED without audio\n" );
}


G_MODULE_EXPORT void on_dbg_animated_enabled_radiomenuitem_toggled ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;

    if ( g_uidebugger.accelerators_locked != 0 ) return;
    g_debugger.animated_updates = DEBUGGER_ANIMATED_UPDATES_ENABLED;
    ui_debugger_hide_spinner_window ( );
    debugger_step_call ( 0 );
    printf ( "Debugger animations ENABLED\n" );
}


G_MODULE_EXPORT void on_dbg_animated_disabled_radiomenuitem_toggled ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;

    if ( g_uidebugger.accelerators_locked != 0 ) return;
    g_debugger.animated_updates = DEBUGGER_ANIMATED_UPDATES_DISABLED;
    ui_debugger_show_spinner_window ( );
    printf ( "Debugger animations DISABLED\n" );
}


G_MODULE_EXPORT void on_dbg_hexeditable_changed ( GtkEditable *ed, gpointer user_data ) {
    ui_hexeditable_changed ( ed, user_data );
}


G_MODULE_EXPORT void on_dbg_reg_edited ( GtkCellRendererText *cell, const gchar *path_string, const gchar *new_text, gpointer data ) {

    g_uidebugger.accelerators_locked = 0;

    if ( !TEST_EMULATION_PAUSED ) return;

    //GtkTreePath *path = gtk_tree_path_new_from_string ( path_string );
    //gint row = gtk_tree_path_get_indices ( path )[0];

    GtkTreeView *treeview = GTK_TREE_VIEW ( data );
    GtkTreeModel *model = gtk_tree_view_get_model ( treeview );
    GtkTreeIter iter;
    gtk_tree_model_get_iter_from_string ( model, &iter, path_string );

    GValue reg_id = G_VALUE_INIT;
    gtk_tree_model_get_value ( model, &iter, DBG_REG_ID, &reg_id );

    debugger_change_z80_register ( g_value_get_uint ( &reg_id ), (Z80EX_WORD) debuger_hextext_to_uint32 ( new_text ) );
}


static void on_dbg_regX_treeview_row_activated ( GtkTreeView *tree_view, GtkTreePath *path, GtkTreeModel *model ) {

    if ( !TEST_EMULATION_PAUSED ) {
        ui_debugger_pause_emulation ( );
        return;
    };

    GtkTreeIter iter;
    gtk_tree_model_get_iter ( model, &iter, path );

    GValue gv_reg = G_VALUE_INIT;
    gtk_tree_model_get_value ( model, &iter, DBG_REG_ID, &gv_reg );
    Z80_REG_T reg = (Z80_REG_T) g_value_get_uint ( &gv_reg );

    if ( ( reg == regR ) || ( reg == regI ) ) return;

    GValue gv_value = G_VALUE_INIT;
    gtk_tree_model_get_value ( model, &iter, DBG_REG_VALUE, &gv_value );
    Z80EX_WORD addr = (Z80EX_WORD) g_value_get_uint ( &gv_value );

    ui_debugger_update_disassembled ( addr, -1 );
}


G_MODULE_EXPORT void on_dbg_reg0_treeview_row_activated ( GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data ) {
    GtkTreeModel *model = GTK_TREE_MODEL ( ui_get_object ( "dbg_reg0_liststore" ) );
    on_dbg_regX_treeview_row_activated ( tree_view, path, model );
}


G_MODULE_EXPORT void on_dbg_reg1_treeview_row_activated ( GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data ) {
    GtkTreeModel *model = GTK_TREE_MODEL ( ui_get_object ( "dbg_reg1_liststore" ) );
    on_dbg_regX_treeview_row_activated ( tree_view, path, model );
}


G_MODULE_EXPORT void on_dbg_stack_treeview_row_activated ( GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data ) {

    if ( !TEST_EMULATION_PAUSED ) {
        ui_debugger_pause_emulation ( );
        return;
    };

    GtkTreeModel *model = GTK_TREE_MODEL ( ui_get_object ( "dbg_stack_liststore" ) );
    GtkTreeIter iter;
    gtk_tree_model_get_iter ( model, &iter, path );

    GValue gv_value = G_VALUE_INIT;
    gtk_tree_model_get_value ( model, &iter, DBG_STACK_VALUE, &gv_value );
    Z80EX_WORD addr = (Z80EX_WORD) g_value_get_uint ( &gv_value );

    ui_debugger_update_disassembled ( addr, -1 );
}


G_MODULE_EXPORT void on_dbg_stack_edited ( GtkCellRendererText *cell, const gchar *path_string, const gchar *new_text, gpointer data ) {

    g_uidebugger.accelerators_locked = 0;

    if ( !TEST_EMULATION_PAUSED ) return;


    //GtkTreePath *path = gtk_tree_path_new_from_string ( path_string );
    //gint row = gtk_tree_path_get_indices ( path )[0];

    GtkTreeModel *model = GTK_TREE_MODEL ( ui_get_object ( "dbg_stack_liststore" ) );
    GtkTreeIter iter;
    gtk_tree_model_get_iter_from_string ( model, &iter, path_string );

    GValue gv = G_VALUE_INIT;
    gtk_tree_model_get_value ( model, &iter, DBG_STACK_ADDR, &gv );

    //    printf ( "addr: 0x%04x, value: 0x%s\n", g_value_get_uint ( &addr ), new_text );

    Z80EX_WORD new_value = (Z80EX_WORD) debuger_hextext_to_uint32 ( new_text );
    Z80EX_WORD addr = (Z80EX_WORD) g_value_get_uint ( &gv );

    debugger_memory_write_byte ( addr, (Z80EX_BYTE) ( new_value & 0xff ) );
    debugger_memory_write_byte ( ( addr + 1 ), (Z80EX_BYTE) ( ( new_value >> 8 ) & 0xff ) );

    if ( g_debugger.screen_refresh_on_edit ) {
        if ( ( memory_test_addr_is_vram ( addr ) ) || ( memory_test_addr_is_vram ( addr + 1 ) ) ) {
            debugger_forced_screen_update ( );
        };
    };

    /* TODO: updatnout jen zmeneny radek a zachovat selection */
    ui_debugger_update_stack ( );

    ui_debugger_update_disassembled ( ui_debugger_dissassembled_get_first_addr ( ), -1 );
}


G_MODULE_EXPORT void on_dbg_cell_editing_canceled ( GtkCellRenderer *cell, GtkCellEditable *editable, const gchar *path_string, gpointer data ) {
    g_uidebugger.accelerators_locked = 0;
}


G_MODULE_EXPORT void on_dbg_cell_editing_started ( GtkCellRenderer *cell, GtkCellEditable *editable, const gchar *path_string, gpointer data ) {


    if ( !TEST_EMULATION_PAUSED ) {
        /* Tohle nefunguje, nicmene pokud po editing_started prijde zmena obsahu, tak se editing rezim stejne sam ukonci, takze OK */
        //gtk_cell_renderer_stop_editing ( GTK_CELL_RENDERER ( cell ), TRUE );
        ui_debugger_pause_emulation ( );
        //return;
    };

    GtkEntry *entry;
    g_uidebugger.accelerators_locked = 1;

    entry = GTK_ENTRY ( editable );
    gchar *text = gtk_editable_get_chars ( GTK_EDITABLE ( editable ), 0, -1 );
    gtk_entry_set_max_length ( entry, strlen ( text ) );
    g_signal_connect ( editable, "changed", G_CALLBACK ( on_dbg_hexeditable_changed ), NULL );
}


G_MODULE_EXPORT gboolean on_dbg_mmap ( GtkWidget *widget, GdkEventButton *event, gpointer user_data ) {
    if ( !TEST_EMULATION_PAUSED ) {
        ui_debugger_pause_emulation ( );
    } else {

        gtk_widget_set_sensitive ( ui_get_widget ( "dbg_mmap_memext_remap" ), ( TEST_MEMEXT_CONNECTED ) );

        // win32 a win64 GTK ve stavajici verzi umi jen gtk_menu_popup, ktery je vsak v novych verzich deprecated
#ifdef WINDOWS
        gtk_menu_popup ( GTK_MENU ( ui_get_widget ( "dbg_mmap_menu" ) ), NULL, NULL, NULL, NULL, 0, 0 );
#else
        gtk_menu_popup_at_pointer ( GTK_MENU ( ui_get_widget ( "dbg_mmap_menu" ) ), (GdkEvent*) event );
#endif
    };
    return FALSE;
}


G_MODULE_EXPORT void on_dbg_mmap0_mount ( GtkMenuItem *menuitem, gpointer user_data ) {
    debugger_mmap_mount ( MEMORY_MAP_FLAG_ROM_0000 );
}


G_MODULE_EXPORT void on_dbg_mmap0_umount ( GtkMenuItem *menuitem, gpointer user_data ) {
    debugger_mmap_umount ( MEMORY_MAP_FLAG_ROM_0000 );
}


G_MODULE_EXPORT void on_dbg_mmap1_mount ( GtkMenuItem *menuitem, gpointer user_data ) {
    debugger_mmap_mount ( MEMORY_MAP_FLAG_ROM_1000 );
}


G_MODULE_EXPORT void on_dbg_mmap1_umount ( GtkMenuItem *menuitem, gpointer user_data ) {
    debugger_mmap_umount ( MEMORY_MAP_FLAG_ROM_1000 );
}


G_MODULE_EXPORT void on_dbg_mmap_vram_mount ( GtkMenuItem *menuitem, gpointer user_data ) {
    debugger_mmap_mount ( MEMORY_MAP_FLAG_CGRAM_VRAM );
}


G_MODULE_EXPORT void on_dbg_mmap_vram_umount ( GtkMenuItem *menuitem, gpointer user_data ) {
    debugger_mmap_umount ( MEMORY_MAP_FLAG_CGRAM_VRAM );
}


G_MODULE_EXPORT void on_dbg_mmape_mount ( GtkMenuItem *menuitem, gpointer user_data ) {
    debugger_mmap_mount ( MEMORY_MAP_FLAG_ROM_E000 );
}


G_MODULE_EXPORT void on_dbg_mmape_umount ( GtkMenuItem *menuitem, gpointer user_data ) {
    debugger_mmap_umount ( MEMORY_MAP_FLAG_ROM_E000 );
}


G_MODULE_EXPORT void on_dbg_mmap_mount_all ( GtkMenuItem *menuitem, gpointer user_data ) {
    debugger_mmap_mount ( MEMORY_MAP_FLAG_ROM_0000 | MEMORY_MAP_FLAG_ROM_1000 | MEMORY_MAP_FLAG_CGRAM_VRAM | MEMORY_MAP_FLAG_ROM_E000 );
}


G_MODULE_EXPORT void on_dbg_mmap_umount_all ( GtkMenuItem *menuitem, gpointer user_data ) {
    debugger_mmap_umount ( MEMORY_MAP_FLAG_ROM_0000 | MEMORY_MAP_FLAG_ROM_1000 | MEMORY_MAP_FLAG_CGRAM_VRAM | MEMORY_MAP_FLAG_ROM_E000 );
}


G_MODULE_EXPORT void on_dbg_mmap_memext_remap_activate ( GtkMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
    ui_dbg_memext_show ( );
}


static void ui_debugger_change_z80_flagbit ( unsigned flagbit, unsigned value ) {
    debugger_change_z80_flagbit ( flagbit, value );
    g_uidebugger.last_flagreg[flagbit] = ( value ) ? TRUE : FALSE;
}


G_MODULE_EXPORT void on_dbg_flagreg_bit0 ( GtkToggleButton *togglebutton, gpointer user_data ) {
    if ( TEST_UICALLBACKS_LOCKED ) return;
    unsigned value = gtk_toggle_button_get_active ( togglebutton ) ? 1 : 0;
    ui_debugger_change_z80_flagbit ( 0, value );
}


G_MODULE_EXPORT void on_dbg_flagreg_bit1 ( GtkToggleButton *togglebutton, gpointer user_data ) {
    if ( TEST_UICALLBACKS_LOCKED ) return;
    unsigned value = gtk_toggle_button_get_active ( togglebutton ) ? 1 : 0;
    ui_debugger_change_z80_flagbit ( 1, value );
}


G_MODULE_EXPORT void on_dbg_flagreg_bit2 ( GtkToggleButton *togglebutton, gpointer user_data ) {
    if ( TEST_UICALLBACKS_LOCKED ) return;
    unsigned value = gtk_toggle_button_get_active ( togglebutton ) ? 1 : 0;
    ui_debugger_change_z80_flagbit ( 2, value );
}


G_MODULE_EXPORT void on_dbg_flagreg_bit3 ( GtkToggleButton *togglebutton, gpointer user_data ) {
    if ( TEST_UICALLBACKS_LOCKED ) return;
    unsigned value = gtk_toggle_button_get_active ( togglebutton ) ? 1 : 0;
    ui_debugger_change_z80_flagbit ( 3, value );
}


G_MODULE_EXPORT void on_dbg_flagreg_bit4 ( GtkToggleButton *togglebutton, gpointer user_data ) {
    if ( TEST_UICALLBACKS_LOCKED ) return;
    unsigned value = gtk_toggle_button_get_active ( togglebutton ) ? 1 : 0;
    ui_debugger_change_z80_flagbit ( 4, value );
}


G_MODULE_EXPORT void on_dbg_flagreg_bit5 ( GtkToggleButton *togglebutton, gpointer user_data ) {
    if ( TEST_UICALLBACKS_LOCKED ) return;
    unsigned value = gtk_toggle_button_get_active ( togglebutton ) ? 1 : 0;
    ui_debugger_change_z80_flagbit ( 5, value );
}


G_MODULE_EXPORT void on_dbg_flagreg_bit6 ( GtkToggleButton *togglebutton, gpointer user_data ) {
    if ( TEST_UICALLBACKS_LOCKED ) return;
    unsigned value = gtk_toggle_button_get_active ( togglebutton ) ? 1 : 0;
    ui_debugger_change_z80_flagbit ( 6, value );
}


G_MODULE_EXPORT void on_dbg_flagreg_bit7 ( GtkToggleButton *togglebutton, gpointer user_data ) {
    if ( TEST_UICALLBACKS_LOCKED ) return;
    unsigned value = gtk_toggle_button_get_active ( togglebutton ) ? 1 : 0;
    ui_debugger_change_z80_flagbit ( 7, value );
}


G_MODULE_EXPORT void on_dbg_im ( GtkComboBox *combobox, gpointer user_data ) {
    if ( TEST_UICALLBACKS_LOCKED ) return;
    int value = gtk_combo_box_get_active ( combobox );
    debugger_change_z80_register ( regIM, value );
    g_uidebugger.last_im = value;
}


G_MODULE_EXPORT void on_dbg_regiff1 ( GtkToggleButton *togglebutton, gpointer user_data ) {
    if ( TEST_UICALLBACKS_LOCKED ) return;
    unsigned value = gtk_toggle_button_get_active ( togglebutton ) ? 1 : 0;
    debugger_change_z80_register ( regIFF1, value );
    g_uidebugger.last_iff1 = value; // last je potreba zmenit i v pripade, ze teprve prechazime do pauzy (v update se nam v UI opet nastavi puvodni hodnota)
}


G_MODULE_EXPORT void on_dbg_regiff2 ( GtkToggleButton *togglebutton, gpointer user_data ) {
    if ( TEST_UICALLBACKS_LOCKED ) return;
    unsigned value = gtk_toggle_button_get_active ( togglebutton ) ? 1 : 0;
    debugger_change_z80_register ( regIFF2, value );
    g_uidebugger.last_iff2 = value; // last je potreba zmenit i v pripade, ze teprve prechazime do pauzy (v update se nam v UI opet nastavi puvodni hodnota)
}


G_MODULE_EXPORT void on_dbg_regdmd ( GtkComboBox *combobox, gpointer user_data ) {
    if ( TEST_UICALLBACKS_LOCKED ) return;
    int value = gtk_combo_box_get_active ( combobox );
    debugger_change_dmd ( value );
    g_uidebugger.last_dmd = value;

    if ( g_debugger.screen_refresh_on_edit ) {
        debugger_forced_screen_update ( );
    };
}


G_MODULE_EXPORT void on_dbg_8255_ctc2_mask_checkbutton_toggled ( GtkToggleButton *togglebutton, gpointer user_data ) {

    if ( TEST_UICALLBACKS_LOCKED ) return;

    unsigned value = gtk_toggle_button_get_active ( togglebutton ) ? 1 : 0;

    if ( !TEST_EMULATION_PAUSED ) {
        /* Toggle button neni potreba nastavovat zpet, protoze po pauze dojde k update */
        ui_debugger_pause_emulation ( );
    } else {
        pio8255_pc2_set ( value );
    };

    g_uidebugger.last_i8255_ctc2_mask = value; // last je potreba zmenit i v pripade, ze teprve prechazime do pauzy (v update se nam v UI opet nastavi puvodni hodnota)
}


G_MODULE_EXPORT void on_dbg_gdg_reg_border_comboboxtext_changed ( GtkComboBox *combobox, gpointer user_data ) {
    if ( TEST_UICALLBACKS_LOCKED ) return;
    int value = gtk_combo_box_get_active ( combobox );
    debugger_change_gdg_reg_border ( value );
    g_uidebugger.last_gdg_reg_border = value;

    if ( g_debugger.screen_refresh_on_edit ) {
        debugger_forced_screen_update ( );
    };
}


G_MODULE_EXPORT void on_dbg_gdg_reg_palgrp_comboboxtext_changed ( GtkComboBox *combobox, gpointer user_data ) {
    if ( TEST_UICALLBACKS_LOCKED ) return;
    int value = gtk_combo_box_get_active ( combobox );
    debugger_change_gdg_reg_palgrp ( value );
    g_uidebugger.last_gdg_reg_palgrp = value;

    if ( g_debugger.screen_refresh_on_edit ) {
        debugger_forced_screen_update ( );
    };
}


G_MODULE_EXPORT void on_dbg_gdg_reg_pal0_comboboxtext_changed ( GtkComboBox *combobox, gpointer user_data ) {
    if ( TEST_UICALLBACKS_LOCKED ) return;
    int value = gtk_combo_box_get_active ( combobox );
    debugger_change_gdg_reg_pal ( 0, value );
    g_uidebugger.last_gdg_reg_pal0 = value;

    if ( g_debugger.screen_refresh_on_edit ) {
        debugger_forced_screen_update ( );
    };
}


G_MODULE_EXPORT void on_dbg_gdg_reg_pal1_comboboxtext_changed ( GtkComboBox *combobox, gpointer user_data ) {
    if ( TEST_UICALLBACKS_LOCKED ) return;
    int value = gtk_combo_box_get_active ( combobox );
    debugger_change_gdg_reg_pal ( 1, value );
    g_uidebugger.last_gdg_reg_pal1 = value;

    if ( g_debugger.screen_refresh_on_edit ) {
        debugger_forced_screen_update ( );
    };
}


G_MODULE_EXPORT void on_dbg_gdg_reg_pal2_comboboxtext_changed ( GtkComboBox *combobox, gpointer user_data ) {
    if ( TEST_UICALLBACKS_LOCKED ) return;
    int value = gtk_combo_box_get_active ( combobox );
    debugger_change_gdg_reg_pal ( 2, value );
    g_uidebugger.last_gdg_reg_pal2 = value;

    if ( g_debugger.screen_refresh_on_edit ) {
        debugger_forced_screen_update ( );
    };
}


G_MODULE_EXPORT void on_dbg_gdg_reg_pal3_comboboxtext_changed ( GtkComboBox *combobox, gpointer user_data ) {
    if ( TEST_UICALLBACKS_LOCKED ) return;
    int value = gtk_combo_box_get_active ( combobox );
    debugger_change_gdg_reg_pal ( 3, value );
    g_uidebugger.last_gdg_reg_pal3 = value;

    if ( g_debugger.screen_refresh_on_edit ) {
        debugger_forced_screen_update ( );
    };
}


static void ui_debugger_change_gdg_rfr ( void ) {
    Z80EX_BYTE reg_value = 0x00;
    reg_value |= gtk_combo_box_get_active ( GTK_COMBO_BOX ( g_uidebugger.gdg_rfr_mode_comboboxtext ) ) << 7;
    reg_value |= gtk_combo_box_get_active ( GTK_COMBO_BOX ( g_uidebugger.gdg_rfr_bank_comboboxtext ) ) << 4;
    reg_value |= gtk_toggle_button_get_active ( GTK_TOGGLE_BUTTON ( g_uidebugger.gdg_rfr_plane1_checkbutton ) ) << 0;
    reg_value |= gtk_toggle_button_get_active ( GTK_TOGGLE_BUTTON ( g_uidebugger.gdg_rfr_plane2_checkbutton ) ) << 1;
    reg_value |= gtk_toggle_button_get_active ( GTK_TOGGLE_BUTTON ( g_uidebugger.gdg_rfr_plane3_checkbutton ) ) << 2;
    reg_value |= gtk_toggle_button_get_active ( GTK_TOGGLE_BUTTON ( g_uidebugger.gdg_rfr_plane4_checkbutton ) ) << 3;
    debugger_change_gdg_rfr ( reg_value );
}


G_MODULE_EXPORT void on_dbg_gdg_rfr_mode_comboboxtext_changed ( GtkComboBox *combobox, gpointer user_data ) {
    if ( TEST_UICALLBACKS_LOCKED ) return;
    int value = gtk_combo_box_get_active ( combobox );
    ui_debugger_change_gdg_rfr ( );
    g_uidebugger.last_gdg_rfr_mode = value;
}


G_MODULE_EXPORT void on_dbg_gdg_rfr_bank_comboboxtext_changed ( GtkComboBox *combobox, gpointer user_data ) {
    if ( TEST_UICALLBACKS_LOCKED ) return;
    int value = gtk_combo_box_get_active ( combobox );
    ui_debugger_change_gdg_rfr ( );
    g_uidebugger.last_gdg_rfr_bank = value;
}


G_MODULE_EXPORT void on_dbg_gdg_rfr_plane1_checkbutton_toggled ( GtkToggleButton *togglebutton, gpointer user_data ) {
    if ( TEST_UICALLBACKS_LOCKED ) return;
    gboolean value = gtk_toggle_button_get_active ( togglebutton );
    ui_debugger_change_gdg_rfr ( );
    g_uidebugger.last_gdg_rfr_plane1 = value;
}


G_MODULE_EXPORT void on_dbg_gdg_rfr_plane2_checkbutton_toggled ( GtkToggleButton *togglebutton, gpointer user_data ) {
    if ( TEST_UICALLBACKS_LOCKED ) return;
    gboolean value = gtk_toggle_button_get_active ( togglebutton );
    ui_debugger_change_gdg_rfr ( );
    g_uidebugger.last_gdg_rfr_plane2 = value;
}


G_MODULE_EXPORT void on_dbg_gdg_rfr_plane3_checkbutton_toggled ( GtkToggleButton *togglebutton, gpointer user_data ) {
    if ( TEST_UICALLBACKS_LOCKED ) return;
    gboolean value = gtk_toggle_button_get_active ( togglebutton );
    ui_debugger_change_gdg_rfr ( );
    g_uidebugger.last_gdg_rfr_plane3 = value;
}


G_MODULE_EXPORT void on_dbg_gdg_rfr_plane4_checkbutton_toggled ( GtkToggleButton *togglebutton, gpointer user_data ) {
    if ( TEST_UICALLBACKS_LOCKED ) return;
    gboolean value = gtk_toggle_button_get_active ( togglebutton );
    ui_debugger_change_gdg_rfr ( );
    g_uidebugger.last_gdg_rfr_plane4 = value;
}


static void ui_debugger_change_gdg_wfr ( void ) {
    Z80EX_BYTE reg_value = 0x00;
    Z80EX_BYTE wf_mode = gtk_combo_box_get_active ( GTK_COMBO_BOX ( g_uidebugger.gdg_wfr_mode_comboboxtext ) );
    if ( wf_mode == 5 ) {
        wf_mode++;
    };
    reg_value |= wf_mode << 5;
    reg_value |= gtk_combo_box_get_active ( GTK_COMBO_BOX ( g_uidebugger.gdg_wfr_bank_comboboxtext ) ) << 4;
    reg_value |= gtk_toggle_button_get_active ( GTK_TOGGLE_BUTTON ( g_uidebugger.gdg_wfr_plane1_checkbutton ) ) << 0;
    reg_value |= gtk_toggle_button_get_active ( GTK_TOGGLE_BUTTON ( g_uidebugger.gdg_wfr_plane2_checkbutton ) ) << 1;
    reg_value |= gtk_toggle_button_get_active ( GTK_TOGGLE_BUTTON ( g_uidebugger.gdg_wfr_plane3_checkbutton ) ) << 2;
    reg_value |= gtk_toggle_button_get_active ( GTK_TOGGLE_BUTTON ( g_uidebugger.gdg_wfr_plane4_checkbutton ) ) << 3;
    debugger_change_gdg_wfr ( reg_value );
}


G_MODULE_EXPORT void on_dbg_gdg_wfr_mode_comboboxtext_changed ( GtkComboBox *combobox, gpointer user_data ) {
    if ( TEST_UICALLBACKS_LOCKED ) return;
    int value = gtk_combo_box_get_active ( combobox );
    ui_debugger_change_gdg_wfr ( );
    g_uidebugger.last_gdg_wfr_mode = value;
}


G_MODULE_EXPORT void on_dbg_gdg_wfr_bank_comboboxtext_changed ( GtkComboBox *combobox, gpointer user_data ) {
    if ( TEST_UICALLBACKS_LOCKED ) return;
    int value = gtk_combo_box_get_active ( combobox );
    ui_debugger_change_gdg_wfr ( );
    g_uidebugger.last_gdg_wfr_bank = value;
}


G_MODULE_EXPORT void on_dbg_gdg_wfr_plane1_checkbutton_toggled ( GtkToggleButton *togglebutton, gpointer user_data ) {
    if ( TEST_UICALLBACKS_LOCKED ) return;
    gboolean value = gtk_toggle_button_get_active ( togglebutton );
    ui_debugger_change_gdg_wfr ( );
    g_uidebugger.last_gdg_wfr_plane1 = value;
}


G_MODULE_EXPORT void on_dbg_gdg_wfr_plane2_checkbutton_toggled ( GtkToggleButton *togglebutton, gpointer user_data ) {
    if ( TEST_UICALLBACKS_LOCKED ) return;
    gboolean value = gtk_toggle_button_get_active ( togglebutton );
    ui_debugger_change_gdg_wfr ( );
    g_uidebugger.last_gdg_wfr_plane2 = value;
}


G_MODULE_EXPORT void on_dbg_gdg_wfr_plane3_checkbutton_toggled ( GtkToggleButton *togglebutton, gpointer user_data ) {
    if ( TEST_UICALLBACKS_LOCKED ) return;
    gboolean value = gtk_toggle_button_get_active ( togglebutton );
    ui_debugger_change_gdg_wfr ( );
    g_uidebugger.last_gdg_wfr_plane3 = value;
}


G_MODULE_EXPORT void on_dbg_gdg_wfr_plane4_checkbutton_toggled ( GtkToggleButton *togglebutton, gpointer user_data ) {
    if ( TEST_UICALLBACKS_LOCKED ) return;
    gboolean value = gtk_toggle_button_get_active ( togglebutton );
    ui_debugger_change_gdg_wfr ( );
    g_uidebugger.last_gdg_wfr_plane4 = value;
}


G_MODULE_EXPORT void on_dbg_cpu_ticks_reset_button_clicked ( GtkButton *button, gpointer user_data ) {
    ui_debugger_cpu_tick_counter_reset ( );
    ui_debugger_update_cpu_ticks ( );
}


G_MODULE_EXPORT void on_dbg_continue_toolbutton_clicked ( GtkToolButton *toolbutton, gpointer user_data ) {
    if ( !TEST_EMULATION_PAUSED ) {
        ui_debugger_pause_emulation ( );
        return;
    };
    debugger_step_call ( 0 );
    mz800_pause_emulation ( 0 );
}


G_MODULE_EXPORT void on_dbg_pause_toolbutton_clicked ( GtkToolButton *toolbutton, gpointer user_data ) {
    mz800_pause_emulation ( 1 );
}


G_MODULE_EXPORT void on_dbg_step_in_toolbutton_clicked ( GtkToolButton *toolbutton, gpointer user_data ) {
    if ( !TEST_EMULATION_PAUSED ) {
        ui_debugger_pause_emulation ( );
        return;
    };
    debugger_step_call ( 1 );
}


G_MODULE_EXPORT void on_dbg_step_over_toolbutton_clicked ( GtkToolButton *toolbutton, gpointer user_data ) {
    if ( !TEST_EMULATION_PAUSED ) {
        // Pokud uzivatel drzi trvale F8, tak by to mohlo zpusobit nezadouci pauzu
        if ( g_debugger.run_to_temporary_breakpoint ) return;
        ui_debugger_pause_emulation ( );
        return;
    };

    Z80EX_WORD addr = z80ex_get_reg ( g_mz800.cpu, regPC );

    char mnemonic [ DEBUGGER_MNEMONIC_MAXLEN ];
    int t_states, t_states2;
    unsigned bytecode_length = z80ex_dasm ( mnemonic, DEBUGGER_MNEMONIC_MAXLEN - 1, 0, &t_states, &t_states2, debugger_dasm_read_cb, addr, NULL );


    if (
         ( 0 == strncasecmp ( mnemonic, "call", 4 ) ) ||
         ( 0 == strncasecmp ( mnemonic, "rst", 3 ) ) ||
         ( 0 == strncasecmp ( mnemonic, "ldir", 4 ) ) ||
         ( 0 == strncasecmp ( mnemonic, "lddr", 4 ) ) ||
         ( 0 == strncasecmp ( mnemonic, "otir", 4 ) ) ||
         ( 0 == strncasecmp ( mnemonic, "otdr", 4 ) ) ||
         ( 0 == strncasecmp ( mnemonic, "inir", 4 ) ) ||
         ( 0 == strncasecmp ( mnemonic, "indr", 4 ) ) ||
         ( 0 == strncasecmp ( mnemonic, "cpir", 4 ) ) ||
         ( 0 == strncasecmp ( mnemonic, "cpdr", 4 ) ) ||
         ( 0 == strncasecmp ( mnemonic, "djnz", 4 ) )
         ) {
        breakpoints_set_temporary_event ( addr + bytecode_length );
        debugger_step_call ( 0 );
        //mz800_pause_emulation ( 0 );
        mz800_run_to_temporary_breakpoint ( );
    } else {
        debugger_step_call ( 1 );
    };
}


G_MODULE_EXPORT void on_dbg_step_out_toolbutton_clicked ( GtkToolButton *toolbutton, gpointer user_data ) {


    printf ( "%s() - not implemented\n", __func__ );
}


G_MODULE_EXPORT void on_dbg_run_to_cursor_toolbutton_clicked ( GtkToolButton *toolbutton, gpointer user_data ) {
    if ( !TEST_EMULATION_PAUSED ) {
        ui_debugger_pause_emulation ( );
        return;
    };

    Z80EX_WORD addr = ui_debugger_dissassembled_get_selected_addr ( );


    if ( addr == z80ex_get_reg ( g_mz800.cpu, regPC ) ) return;

    breakpoints_set_temporary_event ( addr );
    debugger_step_call ( 0 );
    //mz800_pause_emulation ( 0 );
    mz800_run_to_temporary_breakpoint ( );
}


G_MODULE_EXPORT void on_dbg_breakpoints_toolbutton_clicked ( GtkToolButton *toolbutton, gpointer user_data ) {
    ui_breakpoints_show_hide_window ( );
}


G_MODULE_EXPORT void on_dbg_membrowser_toolbutton_clicked ( GtkToolButton *toolbutton, gpointer user_data ) {
    ui_membrowser_show_hide ( );
}


G_MODULE_EXPORT void on_dbg_dissassembler_toolbutton_clicked ( GtkToolButton *toolbutton, gpointer user_data ) {
    ui_dissassembler_show_window ( );
}


G_MODULE_EXPORT gboolean on_dbg_disassembled_treeview_scroll_event ( GtkWidget *widget, GdkEventScroll *event, gpointer user_data ) {

    if ( !TEST_EMULATION_PAUSED ) {
        //ui_debugger_pause_emulation ( );
        return FALSE;
    };

    GdkScrollDirection direction = event->direction;

    //gdk_event_get_scroll_direction ( (GdkEvent) event, &direction );
    if ( direction == GDK_SCROLL_SMOOTH ) {
        gdouble delta_x = 0;
        gdouble delta_y = 0;
        gdk_event_get_scroll_deltas ( (GdkEvent*) event, &delta_x, &delta_y );
        if ( delta_y < 0 ) {
            direction = GDK_SCROLL_UP;
        } else if ( delta_y > 0 ) {
            direction = GDK_SCROLL_DOWN;
        };
    };

    if ( ( direction == GDK_SCROLL_UP ) || ( direction == GDK_SCROLL_DOWN ) ) {

        GtkTreeModel *model = GTK_TREE_MODEL ( ui_get_object ( "dbg_disassembled_liststore" ) );
        GtkTreeSelection *selection = gtk_tree_view_get_selection ( ui_get_tree_view ( "dbg_disassembled_treeview" ) );
        GtkTreeIter iter;
        gtk_tree_selection_get_selected ( selection, &model, &iter );
        GtkTreePath *path = gtk_tree_model_get_path ( model, &iter );

        gint row = gtk_tree_path_get_indices ( path )[0];

        GValue addr_gvalue = G_VALUE_INIT;
        gtk_tree_model_get_value ( model, &iter, DBG_DIS_ADDR, &addr_gvalue );
        Z80EX_WORD addr = (Z80EX_WORD) g_value_get_uint ( &addr_gvalue );

        if ( ( row == 0 ) && ( direction == GDK_SCROLL_UP ) ) {
            addr--;
            ui_debugger_update_disassembled ( addr, -1 );
            return TRUE;
        } else if ( ( row == ( DEBUGGER_DISASSEMBLED_ROWS - 1 ) ) && ( direction == GDK_SCROLL_DOWN ) ) {


            addr++;
            ui_debugger_update_disassembled ( addr, row );
            return TRUE;
        };
    };

    return FALSE;
}


G_MODULE_EXPORT gboolean on_dbg_disassembled_treeview_key_press_event ( GtkWidget *widget, GdkEventKey *event, gpointer user_data ) {

    if ( !TEST_EMULATION_PAUSED ) {
        ui_debugger_pause_emulation ( );
        return FALSE;
    };

    //printf ( "string: 0x%02x\n", event->string[0] );
    /* ridici klavesy maji hodnotu 0x00, 0x0d a 0x20 se pouzivaji pro aktivaci radku */
    //if ( event->string[0] <= 0x20 ) return FALSE;

    if ( ( event->keyval == GDK_KEY_Up ) || ( event->keyval == GDK_KEY_Down ) || ( event->keyval == GDK_KEY_Page_Up ) || ( event->keyval == GDK_KEY_Page_Down ) ) {

        GtkTreeModel *model = GTK_TREE_MODEL ( ui_get_object ( "dbg_disassembled_liststore" ) );
        GtkTreeSelection *selection = gtk_tree_view_get_selection ( ui_get_tree_view ( "dbg_disassembled_treeview" ) );
        GtkTreeIter iter;
        gtk_tree_selection_get_selected ( selection, &model, &iter );
        GtkTreePath *path = gtk_tree_model_get_path ( model, &iter );

        gint row = gtk_tree_path_get_indices ( path )[0];

        GValue addr_gvalue = G_VALUE_INIT;
        gtk_tree_model_get_value ( model, &iter, DBG_DIS_ADDR, &addr_gvalue );
        Z80EX_WORD addr = (Z80EX_WORD) g_value_get_uint ( &addr_gvalue );

        //printf ( "row: %d\n", row );

        if ( row == 0 ) {
            if ( event->keyval == GDK_KEY_Up ) {
                addr--;
                ui_debugger_update_disassembled ( addr, -1 );
                return TRUE;
            };
            if ( event->keyval == GDK_KEY_Page_Up ) {
                addr -= DEBUGGER_DISASSEMBLED_PGSTEP;
                ui_debugger_update_disassembled ( addr, -1 );
                return TRUE;
            };
        } else if ( row == ( DEBUGGER_DISASSEMBLED_ROWS - 1 ) ) {
            if ( event->keyval == GDK_KEY_Down ) {
                addr++;
                ui_debugger_update_disassembled ( addr, row );
                return TRUE;
            };
            if ( event->keyval == GDK_KEY_Page_Down ) {
                addr += DEBUGGER_DISASSEMBLED_PGSTEP;
                ui_debugger_update_disassembled ( addr, row );
                return TRUE;
            };
        };
        return FALSE;
    };

    char c = event->string[0];
    if ( ( ( c >= 'a' ) && ( c <= 'z' ) ) || ( ( c >= 'A' ) && ( c <= 'Z' ) ) ) {


        GtkTreeModel *model = GTK_TREE_MODEL ( ui_get_object ( "dbg_disassembled_liststore" ) );
        GtkTreeSelection *selection = gtk_tree_view_get_selection ( ui_get_tree_view ( "dbg_disassembled_treeview" ) );
        GtkTreeIter iter;
        gtk_tree_selection_get_selected ( selection, &model, &iter );

        GValue addr_txt = G_VALUE_INIT;
        gtk_tree_model_get_value ( model, &iter, DBG_DIS_ADDR_TXT, &addr_txt );

        ui_debugger_iasm_show_main_window ( event->string, 0, (gchar*) g_value_get_string ( &addr_txt ) );
    };

    return FALSE;
}


G_MODULE_EXPORT gboolean on_dbg_disassembled_treeview_button_press_event ( GtkWidget *widget, GdkEventButton *event, gpointer user_data ) {

    if ( !TEST_EMULATION_PAUSED ) {
        ui_debugger_pause_emulation ( );
    } else {
        if ( 3 == event->button ) {
#ifdef WINDOWS
            // win32 a win64 GTK ve stavajici verzi umi jen gtk_menu_popup, ktery je vsak v novych verzich deprecated
            gtk_menu_popup ( GTK_MENU ( ui_get_widget ( "dbg_disassembled_menu" ) ), NULL, NULL, NULL, NULL, 0, 0 );
#else


            gtk_menu_popup_at_pointer ( GTK_MENU ( ui_get_widget ( "dbg_disassembled_menu" ) ), (GdkEvent*) event );
#endif
        };
    };
    return FALSE;
}


G_MODULE_EXPORT gboolean on_dbg_history_treeview_button_press_event ( GtkWidget *widget, GdkEventButton *event, gpointer user_data ) {

    if ( !TEST_EMULATION_PAUSED ) {


        ui_debugger_pause_emulation ( );
    };
    return FALSE;
}


G_MODULE_EXPORT void on_dbg_disassembled_treeview_row_activated ( GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data ) {

    if ( !TEST_EMULATION_PAUSED ) {


        ui_debugger_pause_emulation ( );
        return;
    };

    GtkTreeModel *model = GTK_TREE_MODEL ( ui_get_object ( "dbg_disassembled_liststore" ) );
    GtkTreeIter iter;
    gtk_tree_model_get_iter ( model, &iter, path );

    GValue addr_txt = G_VALUE_INIT;
    gtk_tree_model_get_value ( model, &iter, DBG_DIS_ADDR_TXT, &addr_txt );

    GValue mnemonics = G_VALUE_INIT;
    gtk_tree_model_get_value ( model, &iter, DBG_DIS_MNEMONIC, &mnemonics );

    ui_debugger_iasm_show_main_window ( (gchar*) g_value_get_string ( &mnemonics ), 1, (gchar*) g_value_get_string ( &addr_txt ) );
}


G_MODULE_EXPORT void on_dbg_set_as_breakpoint_menuitem_activate ( GtkMenuItem *menuitem, gpointer user_data ) {
    Z80EX_WORD addr = ui_debugger_dissassembled_get_selected_addr ( );
    ui_breakpoints_show_window ( );
    int id = ui_breakpoints_simple_add_event ( addr );


    if ( id == -1 ) return;
    ui_breakpoints_select_id ( id );
}


G_MODULE_EXPORT void on_dbg_set_as_pc_menuitem_activate ( GtkMenuItem *menuitem, gpointer user_data ) {


    Z80EX_WORD addr = ui_debugger_dissassembled_get_selected_addr ( );
    debugger_change_z80_register ( regPC, addr );
}


static void fill_combo_entry ( GtkWidget *combo ) {
    gtk_combo_box_text_remove_all ( GTK_COMBO_BOX_TEXT ( combo ) );
    int i;
    for ( i = 0; i < g_uidebugger.focus_addr_hist_count; i++ ) {


        gchar addr_txt [ 5 ];
        sprintf ( addr_txt, "%04X", g_uidebugger.focus_addr_history[i] );
        gtk_combo_box_text_append_text ( GTK_COMBO_BOX_TEXT ( combo ), addr_txt );
    };
}


static GtkWidget *g_dbg_focus_to_addr_entry;


G_MODULE_EXPORT void on_dbg_focus_to_menuitem_activate ( GtkMenuItem *menuitem, gpointer user_data ) {

    static GtkWidget *combo = NULL;

    if ( combo == NULL ) {


        combo = gtk_combo_box_text_new_with_entry ( );
        gtk_widget_show ( combo );
        gtk_container_add ( GTK_CONTAINER ( ui_get_widget ( "dbg_focus_to_addr_box" ) ), combo );
        g_dbg_focus_to_addr_entry = gtk_bin_get_child ( GTK_BIN ( combo ) );
        gtk_entry_set_alignment ( GTK_ENTRY ( g_dbg_focus_to_addr_entry ), 1 );
        gtk_entry_set_max_length ( GTK_ENTRY ( g_dbg_focus_to_addr_entry ), 4 );
        g_signal_connect ( g_dbg_focus_to_addr_entry, "changed", G_CALLBACK ( on_dbg_hexeditable_changed ), NULL );
    };

    fill_combo_entry ( combo );

    gchar addr_txt [ 5 ];
    sprintf ( addr_txt, "%04X", g_uidebugger.last_focus_addr );
    gtk_entry_set_text ( GTK_ENTRY ( g_dbg_focus_to_addr_entry ), addr_txt );
    gtk_editable_select_region ( GTK_EDITABLE ( g_dbg_focus_to_addr_entry ), 0, -1 );

    GtkWidget *window = ui_get_widget ( "dbg_focus_to_window" );
    gtk_widget_show ( window );
    gtk_widget_grab_focus ( g_dbg_focus_to_addr_entry );
}


G_MODULE_EXPORT void on_dbg_focus_to_pc_menuitem_activate ( GtkMenuItem *menuitem, gpointer user_data ) {
    ui_debugger_update_disassembled ( z80ex_get_reg ( g_mz800.cpu, regPC ), -1 );
}


G_MODULE_EXPORT void on_dbg_edit_row_menuitem_activate ( GtkMenuItem *menuitem, gpointer user_data ) {


    GtkTreeModel *model = GTK_TREE_MODEL ( ui_get_object ( "dbg_disassembled_liststore" ) );
    GtkTreeSelection *selection = gtk_tree_view_get_selection ( ui_get_tree_view ( "dbg_disassembled_treeview" ) );
    GtkTreeIter iter;
    gtk_tree_selection_get_selected ( selection, &model, &iter );

    GValue addr_txt = G_VALUE_INIT;
    gtk_tree_model_get_value ( model, &iter, DBG_DIS_ADDR_TXT, &addr_txt );

    GValue mnemonics = G_VALUE_INIT;
    gtk_tree_model_get_value ( model, &iter, DBG_DIS_MNEMONIC, &mnemonics );

    ui_debugger_iasm_show_main_window ( (gchar*) g_value_get_string ( &mnemonics ), 1, (gchar*) g_value_get_string ( &addr_txt ) );
}


G_MODULE_EXPORT gboolean on_dbg_focus_to_window_delete_event ( GtkWidget *widget, GdkEvent *event, gpointer user_data ) {


    GtkWidget *window = ui_get_widget ( "dbg_focus_to_window" );
    gtk_widget_hide ( window );
    return TRUE;
}


G_MODULE_EXPORT void on_dbg_focus_to_cancel_button_clicked ( GtkButton *button, gpointer user_data ) {


    GtkWidget *window = ui_get_widget ( "dbg_focus_to_window" );
    gtk_widget_hide ( window );
}


G_MODULE_EXPORT void on_dbg_focus_to_ok_button_clicked ( GtkButton *button, gpointer user_data ) {
    const gchar *addr_txt = gtk_entry_get_text ( GTK_ENTRY ( g_dbg_focus_to_addr_entry ) );
    if ( strlen ( addr_txt ) ) {
        g_uidebugger.last_focus_addr = (Z80EX_WORD) debuger_hextext_to_uint32 ( addr_txt );
        ui_debugger_update_disassembled ( g_uidebugger.last_focus_addr, -1 );
        GtkWidget *window = ui_get_widget ( "dbg_focus_to_window" );
        gtk_widget_hide ( window );

        Z80EX_WORD history[DBG_FOCUS_ADDR_HIST_LENGTH];
        memcpy ( history, g_uidebugger.focus_addr_history, sizeof ( history ) );
        int i = 0;
        g_uidebugger.focus_addr_history[i++] = g_uidebugger.last_focus_addr;

        int j;
        for ( j = 0; j < DBG_FOCUS_ADDR_HIST_LENGTH; j++ ) {
            if ( j >= g_uidebugger.focus_addr_hist_count ) break;
            if ( history[j] != g_uidebugger.last_focus_addr ) {


                g_uidebugger.focus_addr_history[i++] = history[j];
            };
        };

        g_uidebugger.focus_addr_hist_count = ( i > DBG_FOCUS_ADDR_HIST_LENGTH ) ? DBG_FOCUS_ADDR_HIST_LENGTH : i;
    };
}


G_MODULE_EXPORT gboolean on_dbg_spinner_window_press_event ( GtkWidget *widget, GdkEventButton *event, gpointer user_data ) {
    ui_debugger_pause_emulation ( );
    return FALSE;
}


G_MODULE_EXPORT void on_dbg_hwscroll_ssa_entry_changed ( GtkEditable *ed, gpointer user_data ) {
    if ( g_ui_debugger_callbacks_hwscroll_entry_lock ) return;


    if ( !TEST_EMULATION_PAUSED ) {
        ui_debugger_pause_emulation ( );
        g_uidebugger.last_gdg_ssa = -1;
        return;
    };

    ui_digiteditable_changed ( ed, user_data );

    g_ui_debugger_callbacks_hwscroll_entry_lock = TRUE;

    GtkEntry *entry = ui_get_entry ( "dbg_hwscroll_ssa_entry" );

    uint32_t value = 0;

    if ( !gtk_entry_get_text_length ( entry ) ) {
        //gtk_entry_set_text ( entry, "0" );
    } else {
        value = atoi ( gtk_entry_get_text ( entry ) );
        if ( value > 0x7f ) {
            gtk_entry_set_text ( entry, "127" );
        };
    };

    g_ui_debugger_callbacks_hwscroll_entry_lock = FALSE;

    hwscroll_set_ssa ( value );

    gtk_label_set_text ( GTK_LABEL ( g_uidebugger.gdg_hwscroll_enabled_label ), ( TEST_HWSCRL_ENABLED ) ? "1" : "0" );
    g_uidebugger.last_gdg_hwscroll_enabled = TEST_HWSCRL_ENABLED;

    if ( g_debugger.screen_refresh_on_edit ) {
        debugger_forced_screen_update ( );
    };
}


G_MODULE_EXPORT void on_dbg_hwscroll_sea_entry_changed ( GtkEditable *ed, gpointer user_data ) {
    if ( g_ui_debugger_callbacks_hwscroll_entry_lock ) return;


    if ( !TEST_EMULATION_PAUSED ) {
        ui_debugger_pause_emulation ( );
        g_uidebugger.last_gdg_sea = -1;
        return;
    };

    ui_digiteditable_changed ( ed, user_data );

    g_ui_debugger_callbacks_hwscroll_entry_lock = TRUE;

    GtkEntry *entry = ui_get_entry ( "dbg_hwscroll_sea_entry" );

    uint32_t value = 0;

    if ( !gtk_entry_get_text_length ( entry ) ) {
        //gtk_entry_set_text ( entry, "0" );
    } else {
        value = atoi ( gtk_entry_get_text ( entry ) );
        if ( value > 0x7f ) {
            gtk_entry_set_text ( entry, "127" );
            value = 0x7f;
        };
    };

    g_ui_debugger_callbacks_hwscroll_entry_lock = FALSE;

    hwscroll_set_sea ( value );

    gtk_label_set_text ( GTK_LABEL ( g_uidebugger.gdg_hwscroll_enabled_label ), ( TEST_HWSCRL_ENABLED ) ? "1" : "0" );
    g_uidebugger.last_gdg_hwscroll_enabled = TEST_HWSCRL_ENABLED;

    if ( g_debugger.screen_refresh_on_edit ) {
        debugger_forced_screen_update ( );
    };
}


G_MODULE_EXPORT void on_dbg_hwscroll_sw_entry_changed ( GtkEditable *ed, gpointer user_data ) {
    if ( g_ui_debugger_callbacks_hwscroll_entry_lock ) return;

    if ( !TEST_EMULATION_PAUSED ) {
        ui_debugger_pause_emulation ( );
        g_uidebugger.last_gdg_sw = -1;
        return;
    };

    ui_digiteditable_changed ( ed, user_data );

    g_ui_debugger_callbacks_hwscroll_entry_lock = TRUE;

    GtkEntry *entry = ui_get_entry ( "dbg_hwscroll_sw_entry" );

    uint32_t value = 0;

    if ( !gtk_entry_get_text_length ( entry ) ) {
        //gtk_entry_set_text ( entry, "0" );
    } else {
        value = atoi ( gtk_entry_get_text ( entry ) );
        if ( value > 0x7f ) {
            gtk_entry_set_text ( entry, "127" );
            value = 0x7f;
        };
    };

    g_ui_debugger_callbacks_hwscroll_entry_lock = FALSE;

    hwscroll_set_sw ( value );

    gtk_label_set_text ( GTK_LABEL ( g_uidebugger.gdg_hwscroll_enabled_label ), ( TEST_HWSCRL_ENABLED ) ? "1" : "0" );
    g_uidebugger.last_gdg_hwscroll_enabled = TEST_HWSCRL_ENABLED;

    if ( g_debugger.screen_refresh_on_edit ) {
        debugger_forced_screen_update ( );
    };
}


G_MODULE_EXPORT void on_dbg_hwscroll_sof_entry_changed ( GtkEditable *ed, gpointer user_data ) {
    if ( g_ui_debugger_callbacks_hwscroll_entry_lock ) return;

    if ( !TEST_EMULATION_PAUSED ) {
        ui_debugger_pause_emulation ( );
        g_uidebugger.last_gdg_sof = -1;
        return;
    };

    ui_digiteditable_changed ( ed, user_data );

    g_ui_debugger_callbacks_hwscroll_entry_lock = TRUE;

    GtkEntry *entry = ui_get_entry ( "dbg_hwscroll_sof_entry" );

    uint32_t value = 0;

    if ( !gtk_entry_get_text_length ( entry ) ) {
        //gtk_entry_set_text ( entry, "0" );
    } else {
        value = atoi ( gtk_entry_get_text ( entry ) );
        if ( value > 0x3ff ) {
            gtk_entry_set_text ( entry, "1023" );
            value = 0x3ff;
        };
    };

    g_ui_debugger_callbacks_hwscroll_entry_lock = FALSE;

    hwscroll_set_sof ( value );

    gtk_label_set_text ( GTK_LABEL ( g_uidebugger.gdg_hwscroll_enabled_label ), ( TEST_HWSCRL_ENABLED ) ? "1" : "0" );
    g_uidebugger.last_gdg_hwscroll_enabled = TEST_HWSCRL_ENABLED;

    if ( g_debugger.screen_refresh_on_edit ) {
        debugger_forced_screen_update ( );
    };
}


G_MODULE_EXPORT void on_dbg_screen_forced_refresh_on_edit_checkmenuitem_toggled ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
    if ( g_uidebugger.accelerators_locked ) return;
    g_debugger.screen_refresh_on_edit = ( gtk_check_menu_item_get_active ( menuitem ) ) ? 1 : 0;
}


G_MODULE_EXPORT void on_dbg_screen_forced_refresh_at_step_checkmenuitem_toggled ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
    if ( g_uidebugger.accelerators_locked ) return;
    g_debugger.screen_refresh_at_step = ( gtk_check_menu_item_get_active ( menuitem ) ) ? 1 : 0;
}


G_MODULE_EXPORT void on_dbg_screen_manually_refresh_menuitem_activate ( GtkMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
    //if ( !TEST_EMULATION_PAUSED ) return;
    debugger_forced_screen_update ( );
}

#endif
