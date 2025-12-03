/*
 * File:   ui_debugger_iasm.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 30. srpna 2015, 11:17
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

#ifdef MZ800EMU_CFG_DEBUGGER_ENABLED

#include "ui_debugger.h"
#include "ui/ui_main.h"
#include "debugger/debugger.h"
#include "debugger/inline_asm.h"
#include "debugger/inline_asm_opcodes.h"

#include "memory/memory.h"

typedef struct st_UIIASM {
    //st_UIWINPOS main_pos;
    st_UIWINPOS help_pos;
} st_UIIASM;

static st_UIIASM g_uiiasm;


void ui_debugger_iasm_show_main_window ( gchar *instruction, unsigned select_text, char *addr_txt ) {

    GtkEntry *entry_addr = ui_get_entry ( "dbg_inline_asm_addr-entry" );
    gtk_entry_set_text ( entry_addr, addr_txt );

    GtkEntry *entry_instruction = ui_get_entry ( "dbg_inline_asm_instruction-entry" );
    //GTK_ENTRY ( gtk_bin_get_child ( GTK_BIN ( ui_get_widget ( "inline_asm_comboboxentry" ) ) ) );
    gtk_entry_set_max_length ( entry_instruction, IASM_MNEMONICS_MAXLEN );
    gtk_entry_set_text ( entry_instruction, instruction );

    GtkWidget *window = ui_get_widget ( "dbg_inline_assembler_window" );
    gtk_widget_show ( window );
    gtk_widget_grab_focus ( GTK_WIDGET ( entry_instruction ) );
    gtk_editable_set_position ( GTK_EDITABLE ( entry_instruction ), strlen ( instruction ) );
    if ( select_text ) {
        gtk_editable_select_region ( GTK_EDITABLE ( entry_instruction ), 0, -1 );
    };

    static int initialised = 0;
    if ( !initialised ) {
        initialised = 1;
        //ui_main_setpos ( &g_uiiasm.main_pos, -1, -1 );
        ui_main_setpos ( &g_uiiasm.help_pos, -1, -1 );
    };
    //ui_main_win_move_to_pos ( GTK_WINDOW ( window ), &g_uiiasm.main_pos );

}


void ui_debugger_iasm_hide_main_window ( void ) {
    GtkWidget *w = ui_get_widget ( "dbg_inline_assembler_window" );
    //ui_main_win_get_pos ( GTK_WINDOW ( w ), &g_uiiasm.main_pos );
    gtk_widget_hide ( w );
}


G_MODULE_EXPORT gboolean on_dbg_inline_assembler_window_delete_event ( GtkWidget *widget, GdkEvent *event, gpointer user_data ) {
    ui_debugger_iasm_hide_main_window ( );
    return TRUE;
}


G_MODULE_EXPORT void dbg_inline_assembler_cancel ( GtkButton *button, gpointer user_data ) {
    ui_debugger_iasm_hide_main_window ( );
}


G_MODULE_EXPORT void dbg_inline_assembler_ok ( GtkButton *button, gpointer user_data ) {

    GtkEntry *entry_addr = ui_get_entry ( "dbg_inline_asm_addr-entry" );
    const gchar *addr_txt = gtk_entry_get_text ( entry_addr );

    /* Pokud je policko s adresou prazdne, tak jej znovu nastavime a oznacime k pripadne editaci */
    if ( 0 == strlen ( addr_txt ) ) {

        GtkTreeModel *model = GTK_TREE_MODEL ( ui_get_object ( "dbg_disassembled_liststore" ) );
        GtkTreeSelection *selection = gtk_tree_view_get_selection ( ui_get_tree_view ( "dbg_disassembled_treeview" ) );
        GtkTreeIter iter;
        gtk_tree_selection_get_selected ( selection, &model, &iter );

        GValue addr_txt = G_VALUE_INIT;
        gtk_tree_model_get_value ( model, &iter, DBG_DIS_ADDR_TXT, &addr_txt );

        gtk_entry_set_text ( entry_addr, (gchar*) g_value_get_string ( &addr_txt ) );
        gtk_editable_select_region ( GTK_EDITABLE ( entry_addr ), 0, -1 );
        gtk_widget_grab_focus ( GTK_WIDGET ( entry_addr ) );

        return;
    };

    Z80EX_WORD addr = (Z80EX_WORD) debuger_hextext_to_uint32 ( addr_txt );

    GtkEntry *entry_instruction = ui_get_entry ( "dbg_inline_asm_instruction-entry" );
    const gchar *instruction = gtk_entry_get_text ( entry_instruction );


    st_IASMBIN compiled;
    if ( 0 == debugger_iasm_assemble_line ( addr, instruction, &compiled ) ) {
        /* pokud se koimpilace nezdarila, tak oznacime pole s instrukci k editaci */
        gtk_widget_grab_focus ( GTK_WIDGET ( entry_instruction ) );
        gtk_editable_set_position ( GTK_EDITABLE ( entry_instruction ), strlen ( instruction ) );
        gtk_editable_select_region ( GTK_EDITABLE ( entry_instruction ), 0, -1 );
        return;
    };

    int vram_changed = 0;

    /* ulozime kompilat do pameti */
    int i;
    for ( i = 0; i < compiled.length; i++ ) {
        Z80EX_WORD wr_addr = addr + i;
        //printf ( "compiled output: 0x%04x - 0x%02x\n", wr_addr, compiled.byte[ i ] );
        debugger_memory_write_byte ( wr_addr, compiled.byte[ i ] );
        vram_changed += memory_test_addr_is_vram ( wr_addr );
    };

    if ( ( g_debugger.screen_refresh_on_edit ) && ( vram_changed ) ) {
        debugger_forced_screen_update ( );
    };

    /* Update debuggeru */
    ui_debugger_update_stack ( );


    GtkTreeModel *model = GTK_TREE_MODEL ( ui_get_object ( "dbg_disassembled_liststore" ) );
    GtkTreeSelection *selection = gtk_tree_view_get_selection ( ui_get_tree_view ( "dbg_disassembled_treeview" ) );
    GtkTreeIter iter;
    gtk_tree_selection_get_selected ( selection, &model, &iter );

    GValue addr_gv = G_VALUE_INIT;
    gtk_tree_model_get_value ( model, &iter, DBG_DIS_ADDR, &addr_gv );

    if ( addr == (Z80EX_WORD) g_value_get_uint ( &addr_gv ) ) {
        /* adresa se nezmenila */
        GtkTreePath *path = gtk_tree_model_get_path ( model, &iter );
        gint row = gtk_tree_path_get_indices ( path )[0];
        ui_debugger_update_disassembled ( addr, row );
    } else {
        /* zmenili jsme jinou adresu v pameti - udelame full update disassembled */
        ui_debugger_update_disassembled ( addr, 0 );
    };

    ui_debugger_iasm_hide_main_window ( );
}


void ui_debugger_iasm_hide_help_window ( void ) {
    GtkWidget *w = ui_get_widget ( "debugger_iasm_help_window" );
    ui_main_win_get_pos ( GTK_WINDOW ( w ), &g_uiiasm.help_pos );
    gtk_widget_hide ( w );
}


G_MODULE_EXPORT gboolean on_debugger_iasm_help_window_delete_event ( GtkWidget *widget, GdkEvent *event, gpointer user_data ) {
    ui_debugger_iasm_hide_help_window ( );
    return TRUE;
}


G_MODULE_EXPORT void dbg_inline_assembler_help ( GtkButton *button, gpointer user_data ) {

    GtkWidget *help_window = ui_get_widget ( "debugger_iasm_help_window" );
    if ( gtk_widget_get_visible ( help_window ) ) return;

    /* asm window chceme mit modalni, takze jej radeji zavreme, aby bylo mozne pouzivat napovedu */
    GtkWidget *iasm_window;
    iasm_window = ui_get_widget ( "dbg_inline_assembler_window" );
    gtk_widget_hide ( iasm_window );


    GtkWidget *view;
    GtkTextBuffer *buffer;
    /*
        GtkTextIter start, end;
     */
    GtkTextIter start;
    //PangoFontDescription *font_desc;

    GtkTextTag *tag;
    int i, j, k;
    gchar textrow [ IASM_MNEMONICS_MAXLEN + 1 ];
    const inline_asm_commands_t *asm_commands;
    const inline_asm_opts_t *asm_opts;



    view = ui_get_widget ( "iasm_help_textview" );
    //gtk_widget_set_name ( view, "iasm_help_textview" );

    /*    
        gtk_text_buffer_set_text ( gtk_text_view_get_buffer ( GTK_TEXT_VIEW ( view ) ), "\n\ninLine Assembler", -1 );
     */


    buffer = gtk_text_view_get_buffer ( GTK_TEXT_VIEW ( view ) );

    /*
        gtk_text_buffer_set_text ( buffer, "inLine Assembler\n", -1 );
     */
    /* Change default font throughout the widget */


    //    font_desc = pango_font_description_from_string ( "Arial 16" );
    //    gtk_widget_override_font ( view, font_desc );
    //    pango_font_description_free ( font_desc );
    //
    //
    //    GdkRGBA rgba_color;
    //    gdk_rgba_parse ( &rgba_color, "black" );
    //    gtk_widget_override_color ( view, GTK_STATE_NORMAL, &rgba_color );


    //    GtkCssProvider *provider = gtk_css_provider_new ();
    //    gtk_css_provider_load_from_data ( provider )


    /* Change left margin throughout the widget */
    gtk_text_view_set_left_margin ( GTK_TEXT_VIEW ( view ), 30 );

    /* Use a tag to change the color for just one part of the widget */
    /*
        tag = gtk_text_buffer_create_tag ( buffer, "top_section", "justification", GTK_JUSTIFY_CENTER, "scale", PANGO_SCALE_XX_LARGE, "underline", PANGO_UNDERLINE_SINGLE, NULL );    
        gtk_text_buffer_get_iter_at_offset ( buffer, &start, 0 );
        gtk_text_buffer_get_iter_at_offset ( buffer, &end,  16);
        gtk_text_buffer_apply_tag ( buffer, tag, &start, &end );
     */

    tag = gtk_text_buffer_create_tag ( buffer, "top_section", "justification", GTK_JUSTIFY_CENTER, "scale", PANGO_SCALE_XX_LARGE, "underline", PANGO_UNDERLINE_SINGLE, NULL );

    gtk_text_buffer_get_iter_at_line ( buffer, &start, gtk_text_buffer_get_line_count ( buffer ) );
    gtk_text_buffer_insert_with_tags ( buffer, &start, "\nLine Assembler\n\n", -1, tag, NULL );

    gtk_text_buffer_get_iter_at_line ( buffer, &start, gtk_text_buffer_get_line_count ( buffer ) );
    gtk_text_buffer_insert ( buffer, &start, "Line assembler accepts several ways to enter numeric values\nin the different numerical bases designated by the prefix:\n\n", -1 );

    gtk_text_buffer_get_iter_at_line ( buffer, &start, gtk_text_buffer_get_line_count ( buffer ) );
    gtk_text_buffer_insert ( buffer, &start, "\t\t0x...\t- hex values ( 0x0, 0x1234, 0xDEad )\n\t\t#...\t- hex values ( #0, #12aB, #beeF )\n\t\t%...\t- bin values ( %0, %1010, %11111111 )\n\n", -1 );

    gtk_text_buffer_get_iter_at_line ( buffer, &start, gtk_text_buffer_get_line_count ( buffer ) );
    gtk_text_buffer_insert ( buffer, &start, "Variables without a prefix are automatically considered as a decimal\nnumbers, however if immediately after the number is followed\nanother character of 'A' to 'F', so variable is understood\nas a hexadecimal number.\n", -1 );

    tag = gtk_text_buffer_create_tag ( buffer, "sub_section", "justification", GTK_JUSTIFY_CENTER, "scale", PANGO_SCALE_X_LARGE, "underline", PANGO_UNDERLINE_SINGLE, NULL );

    gtk_text_buffer_get_iter_at_line ( buffer, &start, gtk_text_buffer_get_line_count ( buffer ) );
    gtk_text_buffer_insert_with_tags ( buffer, &start, "\n\nSupported Z80 instructions\n\n", -1, tag, NULL );

    gtk_text_buffer_get_iter_at_line ( buffer, &start, gtk_text_buffer_get_line_count ( buffer ) );
    gtk_text_buffer_insert ( buffer, &start, "If any instruction requires some variables, so these are\nlabeled in the optocode by a special character:\n\n", -1 );

    gtk_text_buffer_get_iter_at_line ( buffer, &start, gtk_text_buffer_get_line_count ( buffer ) );
    gtk_text_buffer_insert ( buffer, &start, "\t\t# - variable is 8 bit value 0 ... 255\n\t\t@ - variable is 16 bit value 0 ... 65535\n\t\t$ - variable is 8 bit signed value -128 ... +127\n\t\t% - variable is a 16 bit address, that the assembler\n\t\t   converts to relative from the current row\n", -1 );


    tag = gtk_text_buffer_create_tag ( buffer, "ins_section", "scale", PANGO_SCALE_LARGE, "underline", PANGO_UNDERLINE_SINGLE, NULL );


    i = 0;
    while ( inline_asm_chars [ i ] . c != 0x00 ) {
        asm_commands = inline_asm_chars [ i ] . commands;

        sprintf ( textrow, "\n%c:\n", inline_asm_chars [ i ] . c );
        gtk_text_buffer_get_iter_at_line ( buffer, &start, gtk_text_buffer_get_line_count ( buffer ) );
        gtk_text_buffer_insert_with_tags ( buffer, &start, textrow, -1, tag, NULL );

        j = 0;
        while ( asm_commands [ j ] . command != NULL ) {
            asm_opts = asm_commands [ j ] . opts;

            k = 0;
            while ( asm_opts [ k ] . optocode != NULL ) {
                if ( asm_opts [ k ] . param != NULL ) {
                    sprintf ( textrow, "\t%s %s\n", asm_commands [ j ] . command, asm_opts [ k ] . param );
                } else {
                    sprintf ( textrow, "\t%s\n", asm_commands [ j ] . command );
                };
                gtk_text_buffer_get_iter_at_line ( buffer, &start, gtk_text_buffer_get_line_count ( buffer ) );
                gtk_text_buffer_insert ( buffer, &start, textrow, -1 );
                k++;
            };

            j++;
        };
        i++;
    };

    gtk_widget_show ( help_window );
    ui_main_win_move_to_pos ( GTK_WINDOW ( help_window ), &g_uiiasm.help_pos );
}


#endif
