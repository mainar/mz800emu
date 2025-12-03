/* 
 * File:   ui_debugger.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 23. srpna 2015, 16:20
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

#include <string.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#ifdef MZ800EMU_CFG_DEBUGGER_ENABLED


#include "ui/ui_main.h"
#include "ui_debugger.h"
#include "debugger/debugger.h"

#include "main.h"

#include "mz800.h"
#include "z80ex/include/z80ex.h"
#include "z80ex/include/z80ex_dasm.h"
#include "gdg/gdg.h"
#include "gdg/video.h"
#include "memory/memory.h"
#include "memory/memext.h"
#include "cmt/cmt.h"
#include "pio8255/pio8255.h"
#include "ctc8253/ctc8253.h"

#include "src/ui/tools/ui_tool_pixbuf.h"

#include "cfgmain.h"
#include "ui/ui_utils.h"
#include "cfgfile/cfgtools.h"
#include "pio8255/pio8255.h"
#include "pioz80/pioz80.h"
#include "ctc8253/ctc8253.h"
#include "gdg/vramctrl.h"
#include "gdg/hwscroll.h"
#include "ui_debugger_callbacks.h"


static const uint32_t g_mmap_color[MMBSTATE_COUNT] = {
                                                      0x008000, /* RAM - green */
                                                      0xff0000, /* ROM - red */
                                                      0x000000, /* CGROM - black */
                                                      0xffffff, /* CGRAM - white */
                                                      0xffc0cb, /* VRAM - ping */
                                                      0x0000ff, /* PORTS - blue */
};


struct st_UIDEBUGGER g_uidebugger;

/* Glade momentalne neumi spravne vyrobit funkcni scale, takze jej produkujeme cely zde */
static GtkWidget *dbg_disassembled_addr_vscale = NULL;


void ui_debugger_update_flag_reg ( void ) {

    uint8_t flag_reg = z80ex_get_reg ( g_mz800.cpu, regAF ) & 0xff;

    LOCK_UICALLBACKS ( );

    int i;
    for ( i = 0; i < 8; i++ ) {
        gboolean state = ( flag_reg & 1 ) ? TRUE : FALSE;
        if ( g_uidebugger.last_flagreg[i] != state ) {
            gtk_toggle_button_set_active ( GTK_TOGGLE_BUTTON ( g_uidebugger.flagreg_checkbbutton[i] ), state );
            g_uidebugger.last_flagreg[i] = state;
        };
        flag_reg = flag_reg >> 1;
    };

    UNLOCK_UICALLBACKS ( );
}


gboolean ui_debugger_update_register ( GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data ) {

    gint row = gtk_tree_path_get_indices ( path )[0];

    GValue gv_reg = G_VALUE_INIT;
    gtk_tree_model_get_value ( model, iter, DBG_REG_ID, &gv_reg );
    Z80_REG_T reg = g_value_get_uint ( &gv_reg );

    GValue gv_value = G_VALUE_INIT;
    gtk_tree_model_get_value ( model, iter, DBG_REG_VALUE, &gv_value );
    Z80EX_WORD last_value = (Z80EX_WORD) g_value_get_uint ( &gv_value );

    char *format;
    Z80EX_WORD value = 0;

    if ( row != 6 ) {
        value = z80ex_get_reg ( g_mz800.cpu, reg );
        format = "%04X";
    } else {
        if ( regR == reg ) {
            value = z80ex_get_reg ( g_mz800.cpu, regR ) & 0x7f;
            value |= z80ex_get_reg ( g_mz800.cpu, regR7 ) & 0x80;
        } else {
            value = z80ex_get_reg ( g_mz800.cpu, reg ) & 0xff;
        };
        format = "%02X";
    };

    if ( last_value == value ) return FALSE;

    char value_txt [5];
    snprintf ( value_txt, sizeof ( value_txt ), format, value );

    gtk_list_store_set ( (GtkListStore*) model, iter, DBG_REG_VALUE_TXT, value_txt, DBG_REG_VALUE, value, -1 );
    return FALSE;
}


void ui_debugger_update_registers ( void ) {
    gtk_tree_model_foreach ( GTK_TREE_MODEL ( ui_get_object ( "dbg_reg0_liststore" ) ), ui_debugger_update_register, NULL );
    gtk_tree_model_foreach ( GTK_TREE_MODEL ( ui_get_object ( "dbg_reg1_liststore" ) ), ui_debugger_update_register, NULL );
}


void ui_debugger_update_cpu_ticks ( void ) {
    char cpu_ticks_buff[20];
    snprintf ( cpu_ticks_buff, sizeof ( cpu_ticks_buff ), "%u", (uint32_t) ( ( gdg_get_total_ticks ( ) - g_uidebugger.cpu_ticks_start ) / GDGCLK2CPU_DIVIDER ) );
    gtk_entry_set_text ( GTK_ENTRY ( g_uidebugger.cpu_ticks_entry ), cpu_ticks_buff );
}


void ui_debugger_update_internals ( void ) {

    LOCK_UICALLBACKS ( );

    /*
     * Z80 internals 
     */

    /* internals: IM */
    int im = z80ex_get_reg ( g_mz800.cpu, regIM );
    if ( im != g_uidebugger.last_im ) {
        gtk_combo_box_set_active ( GTK_COMBO_BOX ( g_uidebugger.im_comboboxtext ), im );
        g_uidebugger.last_im = im;
    };

    /* internals: IFF1 */
    gboolean iff1 = ( z80ex_get_reg ( g_mz800.cpu, regIFF1 ) & 1 ) ? TRUE : FALSE;
    if ( iff1 != g_uidebugger.last_iff1 ) {
        gtk_toggle_button_set_active ( GTK_TOGGLE_BUTTON ( g_uidebugger.iff1_checkbutton ), iff1 );
        g_uidebugger.last_iff1 = iff1;
    };

    /* internals: IFF2 */
    gboolean iff2 = ( z80ex_get_reg ( g_mz800.cpu, regIFF2 ) & 1 ) ? TRUE : FALSE;
    if ( iff2 != g_uidebugger.last_iff2 ) {
        gtk_toggle_button_set_active ( GTK_TOGGLE_BUTTON ( g_uidebugger.iff2_checkbutton ), iff2 );
        g_uidebugger.last_iff2 = iff2;
    };


    /*
     * GDG registry
     */


    /* regDMD */
    if ( g_uidebugger.last_dmd != g_gdg.regDMD ) {
        gtk_combo_box_set_active ( GTK_COMBO_BOX ( g_uidebugger.dmd_comboboxtext ), g_gdg.regDMD );
        g_uidebugger.last_dmd = g_gdg.regDMD;
    };

    /* regBORDER */
    if ( g_uidebugger.last_gdg_reg_border != g_gdg.regBOR ) {
        gtk_combo_box_set_active ( GTK_COMBO_BOX ( g_uidebugger.gdg_reg_border_comboboxtext ), g_gdg.regBOR );
        g_uidebugger.last_gdg_reg_border = g_gdg.regBOR;
    };

    /* regPALGRP */
    if ( g_uidebugger.last_gdg_reg_palgrp != g_gdg.regPALGRP ) {
        gtk_combo_box_set_active ( GTK_COMBO_BOX ( g_uidebugger.gdg_reg_palgrp_comboboxtext ), g_gdg.regPALGRP );
        g_uidebugger.last_gdg_reg_palgrp = g_gdg.regPALGRP;
    };

    /* regPAL0 */
    if ( g_uidebugger.last_gdg_reg_pal0 != g_gdg.regPAL0 ) {
        gtk_combo_box_set_active ( GTK_COMBO_BOX ( g_uidebugger.gdg_reg_pal0_comboboxtext ), g_gdg.regPAL0 );
        g_uidebugger.last_gdg_reg_pal0 = g_gdg.regPAL0;
    };

    /* regPAL1 */
    if ( g_uidebugger.last_gdg_reg_pal1 != g_gdg.regPAL1 ) {
        gtk_combo_box_set_active ( GTK_COMBO_BOX ( g_uidebugger.gdg_reg_pal1_comboboxtext ), g_gdg.regPAL1 );
        g_uidebugger.last_gdg_reg_pal1 = g_gdg.regPAL1;
    };

    /* regPAL2 */
    if ( g_uidebugger.last_gdg_reg_pal2 != g_gdg.regPAL2 ) {
        gtk_combo_box_set_active ( GTK_COMBO_BOX ( g_uidebugger.gdg_reg_pal2_comboboxtext ), g_gdg.regPAL2 );
        g_uidebugger.last_gdg_reg_pal2 = g_gdg.regPAL2;
    };

    /* regPAL3 */
    if ( g_uidebugger.last_gdg_reg_pal3 != g_gdg.regPAL3 ) {
        gtk_combo_box_set_active ( GTK_COMBO_BOX ( g_uidebugger.gdg_reg_pal3_comboboxtext ), g_gdg.regPAL3 );
        g_uidebugger.last_gdg_reg_pal3 = g_gdg.regPAL3;
    };

    /* regRF mode */
    if ( g_uidebugger.last_gdg_rfr_mode != g_vramctrl.regRF_SEARCH ) {
        gtk_combo_box_set_active ( GTK_COMBO_BOX ( g_uidebugger.gdg_rfr_mode_comboboxtext ), g_vramctrl.regRF_SEARCH );
        g_uidebugger.last_gdg_rfr_mode = g_vramctrl.regRF_SEARCH;
    };

    /* regRF bank */
    if ( g_uidebugger.last_gdg_rfr_bank != g_vramctrl.regWFRF_VBANK ) {
        gtk_combo_box_set_active ( GTK_COMBO_BOX ( g_uidebugger.gdg_rfr_bank_comboboxtext ), g_vramctrl.regWFRF_VBANK );
        g_uidebugger.last_gdg_rfr_bank = g_vramctrl.regWFRF_VBANK;
    };

    /* regRF plane1 */
    gboolean rfr_plane1 = ( g_vramctrl.regRF_PLANE & 0x01 );
    if ( g_uidebugger.last_gdg_rfr_plane1 != rfr_plane1 ) {
        gtk_toggle_button_set_active ( GTK_TOGGLE_BUTTON ( g_uidebugger.gdg_rfr_plane1_checkbutton ), rfr_plane1 );
        g_uidebugger.last_gdg_rfr_plane1 = rfr_plane1;
    };

    /* regRF plane2 */
    gboolean rfr_plane2 = ( g_vramctrl.regRF_PLANE & 0x02 );
    if ( g_uidebugger.last_gdg_rfr_plane2 != rfr_plane2 ) {
        gtk_toggle_button_set_active ( GTK_TOGGLE_BUTTON ( g_uidebugger.gdg_rfr_plane2_checkbutton ), rfr_plane2 );
        g_uidebugger.last_gdg_rfr_plane2 = rfr_plane2;
    };

    /* regRF plane3 */
    gboolean rfr_plane3 = ( g_vramctrl.regRF_PLANE & 0x04 );
    if ( g_uidebugger.last_gdg_rfr_plane3 != rfr_plane3 ) {
        gtk_toggle_button_set_active ( GTK_TOGGLE_BUTTON ( g_uidebugger.gdg_rfr_plane3_checkbutton ), rfr_plane3 );
        g_uidebugger.last_gdg_rfr_plane3 = rfr_plane3;
    };

    /* regRF plane4 */
    gboolean rfr_plane4 = ( g_vramctrl.regRF_PLANE & 0x08 );
    if ( g_uidebugger.last_gdg_rfr_plane4 != rfr_plane4 ) {
        gtk_toggle_button_set_active ( GTK_TOGGLE_BUTTON ( g_uidebugger.gdg_rfr_plane4_checkbutton ), rfr_plane4 );
        g_uidebugger.last_gdg_rfr_plane1 = rfr_plane4;
    };

    /* regWF mode */
    if ( g_uidebugger.last_gdg_wfr_mode != g_vramctrl.regWF_MODE ) {
        int wf_combo_mode = ( g_vramctrl.regWF_MODE <= GDG_WF_MODE_REPLACE ) ? g_vramctrl.regWF_MODE : GDG_WF_MODE_PSET;
        gtk_combo_box_set_active ( GTK_COMBO_BOX ( g_uidebugger.gdg_wfr_mode_comboboxtext ), wf_combo_mode );
        g_uidebugger.last_gdg_wfr_mode = g_vramctrl.regWF_MODE;
    };

    /* regWF bank */
    if ( g_uidebugger.last_gdg_wfr_bank != g_vramctrl.regWFRF_VBANK ) {
        gtk_combo_box_set_active ( GTK_COMBO_BOX ( g_uidebugger.gdg_wfr_bank_comboboxtext ), g_vramctrl.regWFRF_VBANK );
        g_uidebugger.last_gdg_wfr_bank = g_vramctrl.regWFRF_VBANK;
    };

    /* regWF plane1 */
    gboolean wfr_plane1 = ( g_vramctrl.regWF_PLANE & 0x01 );
    if ( g_uidebugger.last_gdg_wfr_plane1 != wfr_plane1 ) {
        gtk_toggle_button_set_active ( GTK_TOGGLE_BUTTON ( g_uidebugger.gdg_wfr_plane1_checkbutton ), wfr_plane1 );
        g_uidebugger.last_gdg_wfr_plane1 = wfr_plane1;
    };

    /* regWF plane2 */
    gboolean wfr_plane2 = ( g_vramctrl.regWF_PLANE & 0x02 );
    if ( g_uidebugger.last_gdg_wfr_plane2 != wfr_plane2 ) {
        gtk_toggle_button_set_active ( GTK_TOGGLE_BUTTON ( g_uidebugger.gdg_wfr_plane2_checkbutton ), wfr_plane2 );
        g_uidebugger.last_gdg_wfr_plane2 = wfr_plane2;
    };

    /* regWF plane3 */
    gboolean wfr_plane3 = ( g_vramctrl.regWF_PLANE & 0x04 );
    if ( g_uidebugger.last_gdg_wfr_plane3 != wfr_plane3 ) {
        gtk_toggle_button_set_active ( GTK_TOGGLE_BUTTON ( g_uidebugger.gdg_wfr_plane3_checkbutton ), wfr_plane3 );
        g_uidebugger.last_gdg_wfr_plane3 = wfr_plane3;
    };

    /* regWF plane4 */
    gboolean wfr_plane4 = ( g_vramctrl.regWF_PLANE & 0x08 );
    if ( g_uidebugger.last_gdg_wfr_plane4 != wfr_plane4 ) {
        gtk_toggle_button_set_active ( GTK_TOGGLE_BUTTON ( g_uidebugger.gdg_wfr_plane4_checkbutton ), wfr_plane4 );
        g_uidebugger.last_gdg_wfr_plane1 = wfr_plane4;
    };


    /*
     * GDG signaly
     */

    /* GDG hbln */
    if ( g_gdg.hbln != g_uidebugger.last_gdg_hbln ) {
        gtk_label_set_text ( GTK_LABEL ( g_uidebugger.gdg_hbln_label ), ( g_gdg.hbln ) ? "1" : "0" );
        g_uidebugger.last_gdg_hbln = g_gdg.hbln;
    };

    /* GDG vbln */
    if ( g_gdg.vbln != g_uidebugger.last_gdg_vbln ) {
        gtk_label_set_text ( GTK_LABEL ( g_uidebugger.gdg_vbln_label ), ( g_gdg.vbln ) ? "1" : "0" );
        g_uidebugger.last_gdg_vbln = g_gdg.vbln;
    };

    /* GDG hsync */
    if ( g_gdg.sts_hsync != g_uidebugger.last_gdg_hsync ) {
        gtk_label_set_text ( GTK_LABEL ( g_uidebugger.gdg_hsync_label ), ( g_gdg.sts_hsync ) ? "1" : "0" );
        g_uidebugger.last_gdg_hsync = g_gdg.sts_hsync;
    };

    /* GDG vsync */
    if ( g_gdg.sts_vsync != g_uidebugger.last_gdg_vsync ) {
        gtk_label_set_text ( GTK_LABEL ( g_uidebugger.gdg_vsync_label ), ( g_gdg.sts_vsync ) ? "1" : "0" );
        g_uidebugger.last_gdg_vsync = g_gdg.sts_vsync;
    };

    /* GDG xpos */
    int gdg_xpos = VIDEO_GET_SCREEN_COL ( g_gdg.total_elapsed.ticks );
    char gdg_xpos_buff[5];
    snprintf ( gdg_xpos_buff, sizeof ( gdg_xpos_buff ), "%d", gdg_xpos );
    gtk_label_set_text ( GTK_LABEL ( g_uidebugger.gdg_xpos_label ), gdg_xpos_buff );

    /* GDG ypos */
    if ( g_gdg.beam_row != g_uidebugger.last_gdg_ypos ) {
        char buff[4];
        snprintf ( buff, sizeof ( buff ), "%d", g_gdg.beam_row );
        gtk_label_set_text ( GTK_LABEL ( g_uidebugger.gdg_ypos_label ), buff );
        g_uidebugger.last_gdg_ypos = g_gdg.beam_row;
    };

    /* GDG tempo */
    if ( g_gdg.tempo != g_uidebugger.last_gdg_tempo ) {
        gtk_label_set_text ( GTK_LABEL ( g_uidebugger.gdg_tempo_label ), ( g_gdg.tempo ) ? "1" : "0" );
        g_uidebugger.last_gdg_tempo = g_gdg.tempo;
    };

    /* GDG cnt */
    char gdg_cnt_buff[20];
    snprintf ( gdg_cnt_buff, sizeof ( gdg_cnt_buff ), "%u", g_gdg.total_elapsed.ticks );
    gtk_label_set_text ( GTK_LABEL ( g_uidebugger.gdg_cnt_label ), gdg_cnt_buff );


    /* GDG cnt */
    const char *gdg_beam_state = "UNKNOWN STATE";

    const char *bsts_vertical_retrace = "VERTICAL RETRACE";
    const char *bsts_horizontal_retrace = "HORIZONTAL RETRACE";
    const char *bsts_draw_top_border = "TOP BORDER";
    const char *bsts_draw_bottom_border = "BOTTOM BORDER";
    const char *bsts_draw_left_border = "LEFT BORDER";
    const char *bsts_draw_right_border = "RIGHT BORDER";
    const char *bsts_draw_screen = "SCREEN";

    if ( g_gdg.beam_row > VIDEO_BEAM_BORDER_BOTOM_LAST_ROW ) {
        gdg_beam_state = bsts_vertical_retrace;
    } else {
        if ( g_gdg.beam_row < VIDEO_BEAM_CANVAS_FIRST_ROW ) {
            gdg_beam_state = bsts_draw_top_border;
        } else if ( g_gdg.beam_row > VIDEO_BEAM_CANVAS_LAST_ROW ) {
            gdg_beam_state = bsts_draw_bottom_border;
        } else {
            if ( gdg_xpos < VIDEO_BEAM_CANVAS_FIRST_COLUMN ) {
                gdg_beam_state = bsts_draw_left_border;
            } else if ( gdg_xpos < VIDEO_BEAM_BORDER_RIGHT_FIRST_COLUMN ) {
                gdg_beam_state = bsts_draw_screen;
            } else if ( gdg_xpos <= VIDEO_BEAM_BORDER_RIGHT_LAST_COLUMN ) {
                gdg_beam_state = bsts_draw_right_border;
            };
        };

        if ( gdg_xpos > VIDEO_BEAM_BORDER_RIGHT_LAST_COLUMN ) {
            gdg_beam_state = bsts_horizontal_retrace;
        };
    };

    if ( g_uidebugger.last_gdg_beam_state != gdg_beam_state ) {
        gtk_label_set_text ( GTK_LABEL ( g_uidebugger.gdg_beam_label ), gdg_beam_state );
        g_uidebugger.last_gdg_beam_state = gdg_beam_state;
    };

    /*
     * GDG HW Scroll
     */

    /* HW Scroll SSA */
    int ssa = hwscroll_get_ssa ( );
    if ( ssa != g_uidebugger.last_gdg_ssa ) {
        char buff[4];
        snprintf ( buff, sizeof ( buff ), "%d", ssa );
        g_ui_debugger_callbacks_hwscroll_entry_lock = TRUE;
        gtk_entry_set_text ( GTK_ENTRY ( g_uidebugger.gdg_ssa_entry ), buff );
        g_ui_debugger_callbacks_hwscroll_entry_lock = FALSE;
        g_uidebugger.last_gdg_ssa = ssa;
    };

    /* HW Scroll SEA */
    int sea = hwscroll_get_sea ( );
    if ( sea != g_uidebugger.last_gdg_sea ) {
        char buff[4];
        snprintf ( buff, sizeof ( buff ), "%d", sea );
        g_ui_debugger_callbacks_hwscroll_entry_lock = TRUE;
        gtk_entry_set_text ( GTK_ENTRY ( g_uidebugger.gdg_sea_entry ), buff );
        g_ui_debugger_callbacks_hwscroll_entry_lock = FALSE;
        g_uidebugger.last_gdg_sea = sea;
    };

    /* HW Scroll SW */
    int sw = hwscroll_get_sw ( );
    if ( sw != g_uidebugger.last_gdg_sw ) {
        char buff[4];
        snprintf ( buff, sizeof ( buff ), "%d", sw );
        g_ui_debugger_callbacks_hwscroll_entry_lock = TRUE;
        gtk_entry_set_text ( GTK_ENTRY ( g_uidebugger.gdg_sw_entry ), buff );
        g_ui_debugger_callbacks_hwscroll_entry_lock = FALSE;
        g_uidebugger.last_gdg_sw = sw;
    };

    /* HW Scroll SOF */
    int sof = hwscroll_get_sof ( );
    if ( sof != g_uidebugger.last_gdg_sof ) {
        char buff[5];
        snprintf ( buff, sizeof ( buff ), "%d", sof );
        g_ui_debugger_callbacks_hwscroll_entry_lock = TRUE;
        gtk_entry_set_text ( GTK_ENTRY ( g_uidebugger.gdg_sof_entry ), buff );
        g_ui_debugger_callbacks_hwscroll_entry_lock = FALSE;
        g_uidebugger.last_gdg_sof = sof;
    };

    /* HW Scroll Enabled */
    if ( TEST_HWSCRL_ENABLED != g_uidebugger.last_gdg_hwscroll_enabled ) {
        gtk_label_set_text ( GTK_LABEL ( g_uidebugger.gdg_hwscroll_enabled_label ), ( TEST_HWSCRL_ENABLED ) ? "1" : "0" );
        g_uidebugger.last_gdg_hwscroll_enabled = TEST_HWSCRL_ENABLED;
    };

    /*
     * i8255
     */


    /* i8255 cmt in */
    int cmtin = cmt_read_data ( ) & 1;
    if ( cmtin != g_uidebugger.last_cmtin ) {
        gtk_label_set_text ( GTK_LABEL ( g_uidebugger.i8255_cmt_in_label ), ( cmtin ) ? "1" : "0" );
        g_uidebugger.last_cmtin = cmtin;
    };

    /* i8255 cmt out */
    int cmtout = pio8255_pc1_get ( );
    if ( cmtout != g_uidebugger.last_cmtout ) {
        gtk_label_set_text ( GTK_LABEL ( g_uidebugger.i8255_cmt_out_label ), ( cmtout ) ? "1" : "0" );
        g_uidebugger.last_cmtout = cmtout;
    };

    /* i8255 cmt motor */
    int cmtmotor = pio8255_pc4_get ( );
    if ( cmtmotor != g_uidebugger.last_cmtmotor ) {
        gtk_label_set_text ( GTK_LABEL ( g_uidebugger.i8255_cmt_motor_label ), ( cmtmotor ) ? "1" : "0" );
        g_uidebugger.last_cmtmotor = cmtmotor;
    };

    /* i8255 joy1 strobe */
    int joy1 = pio8255_pa4_get ( );
    if ( joy1 != g_uidebugger.last_joy1 ) {
        gtk_label_set_text ( GTK_LABEL ( g_uidebugger.i8255_joy1_strobe_label ), ( joy1 ) ? "1" : "0" );
        g_uidebugger.last_joy1 = joy1;
    };

    /* i8255 joy2 strobe */
    int joy2 = pio8255_pa5_get ( );
    if ( joy2 != g_uidebugger.last_joy2 ) {
        gtk_label_set_text ( GTK_LABEL ( g_uidebugger.i8255_joy2_strobe_label ), ( joy2 ) ? "1" : "0" );
        g_uidebugger.last_joy2 = joy2;
    };

    /* i8255 keyboard strobe */
    int keyboard = pio8255_pa0_3_get ( );
    if ( keyboard != g_uidebugger.last_keyboard ) {
        char buff[2];
        snprintf ( buff, sizeof ( buff ), "%d", keyboard );
        gtk_label_set_text ( GTK_LABEL ( g_uidebugger.i8255_keyboard_strobe_label ), buff );
        g_uidebugger.last_keyboard = keyboard;
    };

    /* i8255 cursor timer */
    int cursor_timer = mz800_get_cursor_timer_state ( );
    if ( cursor_timer != g_uidebugger.last_cursor_timer ) {
        gtk_label_set_text ( GTK_LABEL ( g_uidebugger.i8255_cursor_timer_label ), ( cursor_timer ) ? "1" : "0" );
        g_uidebugger.last_cursor_timer = cursor_timer;
    };

    /* i8255 CTC2 mask */
    gboolean ctc2_mask = ( pio8255_pc2_get ( ) ) ? TRUE : FALSE;
    if ( ctc2_mask != g_uidebugger.last_i8255_ctc2_mask ) {
        gtk_toggle_button_set_active ( GTK_TOGGLE_BUTTON ( g_uidebugger.i8255_ctc2_mask_checkbutton ), ctc2_mask );
        g_uidebugger.last_i8255_ctc2_mask = ctc2_mask;
    };


    /*
     * Interrupts
     */


    /* interrupt pioz80 */
    gboolean int_pioz80 = ( g_mz800.interrupt & MZ800_INTERRUPT_PIOZ80 );
    if ( int_pioz80 != g_uidebugger.last_int_pioz80 ) {
        gtk_label_set_text ( GTK_LABEL ( g_uidebugger.int_pioz80_label ), ( int_pioz80 ) ? "1" : "0" );
        g_uidebugger.last_int_pioz80 = int_pioz80;
    };

    /* interrupt ctc2 */
    gboolean int_ctc2 = ( g_mz800.interrupt & MZ800_INTERRUPT_CTC2 );
    if ( int_ctc2 != g_uidebugger.last_int_ctc2 ) {
        gtk_label_set_text ( GTK_LABEL ( g_uidebugger.int_ctc2_label ), ( int_ctc2 ) ? "1" : "0" );
        g_uidebugger.last_int_ctc2 = int_ctc2;
    };

    /* interrupt fdc */
    gboolean int_fdc = ( g_mz800.interrupt & MZ800_INTERRUPT_FDC );
    if ( int_fdc != g_uidebugger.last_int_fdc ) {
        gtk_label_set_text ( GTK_LABEL ( g_uidebugger.int_fdc_label ), ( int_fdc ) ? "1" : "0" );
        g_uidebugger.last_int_fdc = int_fdc;
    };


    /*
     * PIOZ80 PortA
     */



    /* pioz80 pa icena */
    if ( g_pioz80.port[PIOZ80_PORT_A].icena != g_uidebugger.last_pioz80_pa_icena ) {
        gtk_label_set_text ( GTK_LABEL ( g_uidebugger.pioz80_pa_icena_label ), ( g_pioz80.port[PIOZ80_PORT_A].icena == PIOZ80_ICENA_DISABLED ) ? "disabled" : "enabled" );
        g_uidebugger.last_pioz80_pa_icena = g_pioz80.port[PIOZ80_PORT_A].icena;
    };

    /* pioz80 pa io_mask */
    if ( g_pioz80.port[PIOZ80_PORT_A].io_mask != g_uidebugger.last_pioz80_pa_io_mask ) {
        char buff[3];
        snprintf ( buff, sizeof ( buff ), "%02X", g_pioz80.port[PIOZ80_PORT_A].io_mask );
        gtk_label_set_text ( GTK_LABEL ( g_uidebugger.pioz80_pa_io_mask_label ), buff );
        g_uidebugger.last_pioz80_pa_io_mask = g_pioz80.port[PIOZ80_PORT_A].io_mask;
    };

    /* pioz80 pa icmask */
    if ( g_pioz80.port[PIOZ80_PORT_A].icmask != g_uidebugger.last_pioz80_pa_icmask ) {
        char buff[3];
        snprintf ( buff, sizeof ( buff ), "%02X", g_pioz80.port[PIOZ80_PORT_A].icmask );
        gtk_label_set_text ( GTK_LABEL ( g_uidebugger.pioz80_pa_icmask_label ), buff );
        g_uidebugger.last_pioz80_pa_icmask = g_pioz80.port[PIOZ80_PORT_A].icmask;
    };

    /* pioz80 pa icfnc */
    if ( g_pioz80.port[PIOZ80_PORT_A].icfnc != g_uidebugger.last_pioz80_pa_icfnc ) {
        gtk_label_set_text ( GTK_LABEL ( g_uidebugger.pioz80_pa_icfnc_label ), ( g_pioz80.port[PIOZ80_PORT_A].icfnc == PIOZ80_ICFNC_OR ) ? "Or" : "And" );
        g_uidebugger.last_pioz80_pa_icfnc = g_pioz80.port[PIOZ80_PORT_A].icfnc;
    };

    /* pioz80 pa iclvl */
    if ( g_pioz80.port[PIOZ80_PORT_A].iclvl != g_uidebugger.last_pioz80_pa_iclvl ) {
        gtk_label_set_text ( GTK_LABEL ( g_uidebugger.pioz80_pa_iclvl_label ), ( g_pioz80.port[PIOZ80_PORT_A].iclvl == PIOZ80_ICLVL_LOW ) ? "Low" : "Hi" );
        g_uidebugger.last_pioz80_pa_iclvl = g_pioz80.port[PIOZ80_PORT_A].iclvl;
    };

    /* pioz80 pa vector */
    Z80EX_WORD pa_vector = ( z80ex_get_reg ( g_mz800.cpu, regI ) << 8 ) | g_pioz80.port[PIOZ80_PORT_A].interrupt_vector;
    if ( pa_vector != g_uidebugger.last_pioz80_pa_vector ) {
        char buff[5];
        snprintf ( buff, sizeof ( buff ), "%04X", pa_vector );
        gtk_label_set_text ( GTK_LABEL ( g_uidebugger.pioz80_pa_vector_label ), buff );
        g_uidebugger.last_pioz80_pa_icmask = pa_vector;
    };


    /*
     * i8253
     */

#ifdef MZ800EMU_CFG_CLK1M1_FAST
    ctc8253_sync_ctc0 ( );
#endif

    int i;
    for ( i = 0; i < 3; i++ ) {

        /* i8253 mode */
        if ( g_ctc8253[i].mode != g_uidebugger.i8253_ctc[i].last_mode ) {
            char buff[2];
            snprintf ( buff, sizeof ( buff ), "%d", g_ctc8253[i].mode );
            gtk_label_set_text ( GTK_LABEL ( g_uidebugger.i8253_ctc[i].mode_label ), buff );
            g_uidebugger.i8253_ctc[i].last_mode = g_ctc8253[i].mode;
        };

        /* i8253 input */
        int ctc_input;
        if ( i == 0 ) {
            ctc_input = ( g_gdg.total_elapsed.ticks & 0x08 ) ? 1 : 0;
        } else if ( i == 1 ) {
            ctc_input = g_gdg.sts_hsync;
        } else {
            ctc_input = g_ctc8253[1].out;
        };
        if ( ctc_input != g_uidebugger.i8253_ctc[i].last_input ) {
            gtk_label_set_text ( GTK_LABEL ( g_uidebugger.i8253_ctc[i].input_label ), ( ctc_input ) ? "1" : "0" );
            g_uidebugger.i8253_ctc[i].last_input = ctc_input;
        };

        /* i8253 output */
        if ( g_ctc8253[i].out != g_uidebugger.i8253_ctc[i].last_output ) {
            gtk_label_set_text ( GTK_LABEL ( g_uidebugger.i8253_ctc[i].output_label ), ( g_ctc8253[i].out ) ? "1" : "0" );
            g_uidebugger.i8253_ctc[i].last_output = g_ctc8253[i].out;
        };

        /* i8253 preset */
        if ( g_ctc8253[i].preset_value != g_uidebugger.i8253_ctc[i].last_preset ) {
            char buff[5];
            snprintf ( buff, sizeof ( buff ), "%04X", g_ctc8253[i].preset_value );
            gtk_label_set_text ( GTK_LABEL ( g_uidebugger.i8253_ctc[i].preset_label ), buff );
            g_uidebugger.i8253_ctc[i].last_preset = g_ctc8253[i].preset_value;
        };

        /* i8253 value */
        if ( g_ctc8253[i].value != g_uidebugger.i8253_ctc[i].last_value ) {
            char buff[5];
            snprintf ( buff, sizeof ( buff ), "%04X", g_ctc8253[i].value );
            gtk_label_set_text ( GTK_LABEL ( g_uidebugger.i8253_ctc[i].value_label ), buff );
            g_uidebugger.i8253_ctc[i].last_value = g_ctc8253[i].value;
        };

        /* i8253 gate */
        if ( i == 0 ) {
            if ( g_ctc8253[i].gate != g_uidebugger.i8253_ctc[i].last_gate ) {
                gtk_label_set_text ( GTK_LABEL ( g_uidebugger.i8253_ctc[i].gate_label ), ( g_ctc8253[i].gate ) ? "1" : "0" );
                g_uidebugger.i8253_ctc[i].last_gate = g_ctc8253[i].gate;
            };
        };

    };

    /*
     * CPU ticks 
     */

    ui_debugger_update_cpu_ticks ( );

    UNLOCK_UICALLBACKS ( );
}


void ui_debugger_set_mmap_forced ( st_UIMMAPBANK *mmbank, en_UI_MMBSTATE state ) {
    ui_tool_pixbuf_fill ( mmbank->pixbuf, g_mmap_color[state] );
    mmbank->state = state;
    gtk_widget_queue_draw ( mmbank->drawing_area );
}


static inline void ui_debugger_set_mmap ( st_UIMMAPBANK *mmbank, en_UI_MMBSTATE state ) {
    if ( mmbank->state != state ) {
        ui_debugger_set_mmap_forced ( mmbank, state );
    };
}


void ui_debugger_update_mmap ( void ) {

    /* MemExt */

    int memext_state = ( TEST_MEMEXT_CONNECTED_PEHU ) | ( TEST_MEMEXT_CONNECTED_LUFTNER << 1 );

    if ( memext_state == 0 ) {
        if ( g_uidebugger.last_memext != memext_state ) {
            int i;
            for ( i = 0; i < MEMEXT_RAW_MAP_SIZE; i++ ) {
                char label_name[14];
                snprintf ( label_name, sizeof ( label_name ), "mmap_memext_%X", i );
                gtk_label_set_text ( ui_get_label ( label_name ), "--" );
                g_uidebugger.last_memext_map[i] = -1;
            };
            g_uidebugger.last_memext = memext_state;
        };
    } else {

        if ( g_uidebugger.last_memext != memext_state ) {
            int i;
            for ( i = 0; i < MEMEXT_RAW_MAP_SIZE; i++ ) {
                char label_name[14];
                snprintf ( label_name, sizeof ( label_name ), "mmap_memext_%X", i );
                g_uidebugger.last_memext_map[i] = g_memext.map[i];
                if ( ( !( i & 1 ) ) || ( TEST_MEMEXT_TYPE_LUFTNER ) ) {
                    char buff[3];
                    snprintf ( buff, sizeof ( buff ), "%02X", g_memext.map[i] >> ( TEST_MEMEXT_TYPE_PEHU ) );
                    gtk_label_set_text ( ui_get_label ( label_name ), buff );
                } else {
                    gtk_label_set_text ( ui_get_label ( label_name ), "--" );
                };
            };
            g_uidebugger.last_memext = memext_state;
        } else {
            int i;
            for ( i = 0; i < MEMEXT_RAW_MAP_SIZE; i++ ) {
                if ( g_uidebugger.last_memext_map[i] != g_memext.map[i] ) {
                    char label_name[14];
                    snprintf ( label_name, sizeof ( label_name ), "mmap_memext_%X", i );
                    g_uidebugger.last_memext_map[i] = g_memext.map[i];
                    if ( ( !( i & 1 ) ) || ( TEST_MEMEXT_TYPE_LUFTNER ) ) {
                        char buff[3];
                        snprintf ( buff, sizeof ( buff ), "%02X", g_memext.map[i] >> ( TEST_MEMEXT_TYPE_PEHU ) );
                        gtk_label_set_text ( ui_get_label ( label_name ), buff );
                    };
                };
            };
        };
    };


    /* Standardni mapovani */

    if ( ( g_gdg.regDMD == g_uidebugger.last_mmap_dmd ) && ( g_memory.map == g_uidebugger.last_map ) ) {
        return;
    };

    g_uidebugger.last_map = g_memory.map;
    g_uidebugger.last_mmap_dmd = g_gdg.regDMD;

    if ( MEMORY_MAP_TEST_ROM_0000 ) {
        ui_debugger_set_mmap ( &g_uidebugger.mmapbank[MMBANK_0], MMBSTATE_ROM );
    } else {
        ui_debugger_set_mmap ( &g_uidebugger.mmapbank[MMBANK_0], MMBSTATE_RAM );
    };

    if ( MEMORY_MAP_TEST_ROM_1000 ) {
        ui_debugger_set_mmap ( &g_uidebugger.mmapbank[MMBANK_1], MMBSTATE_CGROM );
    } else {
        ui_debugger_set_mmap ( &g_uidebugger.mmapbank[MMBANK_1], MMBSTATE_RAM );
    };

    if ( MEMORY_MAP_TEST_VRAM_8000 ) {
        ui_debugger_set_mmap ( &g_uidebugger.mmapbank[MMBANK_8], MMBSTATE_VRAM );
        ui_debugger_set_mmap ( &g_uidebugger.mmapbank[MMBANK_9], MMBSTATE_VRAM );
    } else {
        ui_debugger_set_mmap ( &g_uidebugger.mmapbank[MMBANK_8], MMBSTATE_RAM );
        ui_debugger_set_mmap ( &g_uidebugger.mmapbank[MMBANK_9], MMBSTATE_RAM );
    };

    if ( MEMORY_MAP_TEST_VRAM_A000 ) {
        ui_debugger_set_mmap ( &g_uidebugger.mmapbank[MMBANK_A], MMBSTATE_VRAM );
        ui_debugger_set_mmap ( &g_uidebugger.mmapbank[MMBANK_B], MMBSTATE_VRAM );
    } else {
        ui_debugger_set_mmap ( &g_uidebugger.mmapbank[MMBANK_A], MMBSTATE_RAM );
        ui_debugger_set_mmap ( &g_uidebugger.mmapbank[MMBANK_B], MMBSTATE_RAM );
    };

    if ( MEMORY_MAP_TEST_CGRAM ) {
        ui_debugger_set_mmap ( &g_uidebugger.mmapbank[MMBANK_C], MMBSTATE_CGRAM );
    } else {
        ui_debugger_set_mmap ( &g_uidebugger.mmapbank[MMBANK_C], MMBSTATE_RAM );
    };

    if ( DMD_TEST_MZ700 ) {
        if ( MEMORY_MAP_TEST_ROM_E000 ) {
            ui_debugger_set_mmap ( &g_uidebugger.mmapbank[MMBANK_D], MMBSTATE_VRAM );
            ui_debugger_set_mmap ( &g_uidebugger.mmapbank[MMBANK_E_PORTS], MMBSTATE_PORTS );
            ui_debugger_set_mmap ( &g_uidebugger.mmapbank[MMBANK_E], MMBSTATE_ROM );
            ui_debugger_set_mmap ( &g_uidebugger.mmapbank[MMBANK_F], MMBSTATE_ROM );
        } else {
            ui_debugger_set_mmap ( &g_uidebugger.mmapbank[MMBANK_D], MMBSTATE_RAM );
            ui_debugger_set_mmap ( &g_uidebugger.mmapbank[MMBANK_E_PORTS], MMBSTATE_RAM );
            ui_debugger_set_mmap ( &g_uidebugger.mmapbank[MMBANK_E], MMBSTATE_RAM );
            ui_debugger_set_mmap ( &g_uidebugger.mmapbank[MMBANK_F], MMBSTATE_RAM );
        };
    } else {

        ui_debugger_set_mmap ( &g_uidebugger.mmapbank[MMBANK_D], MMBSTATE_RAM );

        if ( MEMORY_MAP_TEST_ROM_E000 ) {
            ui_debugger_set_mmap ( &g_uidebugger.mmapbank[MMBANK_E_PORTS], MMBSTATE_ROM );
            ui_debugger_set_mmap ( &g_uidebugger.mmapbank[MMBANK_E], MMBSTATE_ROM );
            ui_debugger_set_mmap ( &g_uidebugger.mmapbank[MMBANK_F], MMBSTATE_ROM );
        } else {
            ui_debugger_set_mmap ( &g_uidebugger.mmapbank[MMBANK_E_PORTS], MMBSTATE_RAM );
            ui_debugger_set_mmap ( &g_uidebugger.mmapbank[MMBANK_E], MMBSTATE_RAM );
            ui_debugger_set_mmap ( &g_uidebugger.mmapbank[MMBANK_F], MMBSTATE_RAM );
        };
    };
}


void ui_debugger_init_stack ( GtkTreeModel *model ) {
    unsigned i;
    for ( i = 0; i < DEBUGGER_STACK_ROWS; i++ ) {
        GtkTreeIter iter;
        gtk_list_store_append ( GTK_LIST_STORE ( model ), &iter );
        gtk_list_store_set ( GTK_LIST_STORE ( model ), &iter,
                             DBG_STACK_ADDR, 0,
                             DBG_STACK_ADDR_TXT, "",
                             DBG_STACK_VALUE_TXT, "",
                             -1 );
    };
}


void ui_debugger_update_stack ( void ) {

    Z80EX_WORD addr = z80ex_get_reg ( g_mz800.cpu, regSP ) - DEBUGGER_STACK_ROWS;

    GtkTreeModel *model = GTK_TREE_MODEL ( ui_get_object ( "dbg_stack_liststore" ) );

    if ( 0 == gtk_tree_model_iter_n_children ( model, NULL ) ) ui_debugger_init_stack ( model );

    GtkTreeIter iter;
    gtk_tree_model_get_iter_first ( model, &iter );

    unsigned i;
    for ( i = 0; i < DEBUGGER_STACK_ROWS; i++ ) {

        if ( i == ( DEBUGGER_STACK_ROWS / 2 ) ) {
            GtkTreeSelection *selection = gtk_tree_view_get_selection ( ui_get_tree_view ( "dbg_stack_treeview" ) );
            gtk_tree_selection_select_iter ( selection, &iter );
        } else if ( i == ( DEBUGGER_STACK_ROWS / 2 ) - 3 ) {
            GtkTreePath *path = gtk_tree_model_get_path ( model, &iter );
            gtk_tree_view_scroll_to_cell ( ui_get_tree_view ( "dbg_stack_treeview" ), path, NULL, FALSE, 0.0, 0.0 );
        };

        Z80EX_WORD value = debugger_memory_read_byte ( addr );
        value |= debugger_memory_read_byte ( addr + 1 ) << 8;

        GValue gv_addr = G_VALUE_INIT;
        gtk_tree_model_get_value ( model, &iter, DBG_STACK_ADDR, &gv_addr );
        Z80EX_WORD last_addr = (Z80EX_WORD) g_value_get_uint ( &gv_addr );

        GValue gv_value = G_VALUE_INIT;
        gtk_tree_model_get_value ( model, &iter, DBG_STACK_VALUE, &gv_value );
        Z80EX_WORD last_value = (Z80EX_WORD) g_value_get_uint ( &gv_value );

        if ( ( last_addr != addr ) || ( last_value != value ) ) {
            char addr_txt [ 6 ];
            char value_txt [ 5 ];

            snprintf ( addr_txt, sizeof ( addr_txt ), "%04X:", addr );
            snprintf ( value_txt, sizeof ( value_txt ), "%04X", value );

            gtk_list_store_set ( GTK_LIST_STORE ( model ), &iter,
                                 DBG_STACK_ADDR, addr,
                                 DBG_STACK_ADDR_TXT, addr_txt,
                                 DBG_STACK_VALUE_TXT, value_txt,
                                 DBG_STACK_VALUE, value,
                                 -1 );
        };
        addr += 2;
        gtk_tree_model_iter_next ( model, &iter );
    };

}


void ui_debugger_init_disassembled ( GtkTreeModel *model, unsigned start_row, unsigned count ) {
    unsigned i;
    unsigned end_row = start_row + count;

    /* TODO: casem nejaky assert */
    if ( !( start_row < end_row ) ) return;

    for ( i = start_row; i != end_row; i++ ) {
        GtkTreeIter iter;
        gtk_list_store_append ( GTK_LIST_STORE ( model ), &iter );
        gtk_list_store_set ( GTK_LIST_STORE ( model ), &iter,
                             DBG_DIS_ADDR, 0,
                             DBG_DIS_ADDR_TXT, "",
                             DBG_DIS_BYTE0, "",
                             DBG_DIS_BYTE1, "",
                             DBG_DIS_BYTE2, "",
                             DBG_DIS_BYTE3, "",
                             DBG_DIS_MNEMONIC, "",
                             -1 );
    };
}


static inline void ui_debugger_init_history ( GtkTreeModel *model, unsigned start_row, unsigned count ) {
    ui_debugger_init_disassembled ( model, start_row, count );
}


void ui_debugger_update_history ( void ) {

    GtkTreeModel *model = GTK_TREE_MODEL ( ui_get_object ( "dbg_history_liststore" ) );
    GtkTreeIter iter;

    if ( 0 == gtk_tree_model_iter_n_children ( model, NULL ) ) ui_debugger_init_history ( model, 0, DEBUGGER_HISTORY_ROWS );

    int row = 0;
    gtk_tree_model_get_iter_first ( model, &iter );

    unsigned i;
    for ( i = row; i < DEBUGGER_HISTORY_ROWS; i++ ) {

        uint8_t position = debugger_history_position ( g_debugger_history.position - DEBUGGER_HISTORY_ROWS + 1 + i );

        Z80EX_WORD addr = g_debugger_history.row[position].addr;

        char addr_txt [ 6 ];
        char byte0 [ 3 ];
        char byte1 [ 3 ];
        char byte2 [ 3 ];
        char byte3 [ 3 ];
        char mnemonic [ DEBUGGER_MNEMONIC_MAXLEN ];
        int t_states, t_states2;

        sprintf ( addr_txt, "%04X:", addr );
        Z80EX_WORD addr_row = addr;

        unsigned bytecode_length = z80ex_dasm ( mnemonic, DEBUGGER_MNEMONIC_MAXLEN - 1, 0, &t_states, &t_states2, debugger_dasm_history_read_cb, addr, &position );

        sprintf ( byte0, "%02X", g_debugger_history.row[position].byte[0] );

        if ( 1 < bytecode_length ) {
            sprintf ( byte1, "%02X", g_debugger_history.row[position].byte[1] );
        } else {
            byte1 [ 0 ] = 0x00;
        };

        if ( 2 < bytecode_length ) {
            sprintf ( byte2, "%02X", g_debugger_history.row[position].byte[2] );
        } else {
            byte2 [ 0 ] = 0x00;
        };

        if ( 3 < bytecode_length ) {
            sprintf ( byte3, "%02X", g_debugger_history.row[position].byte[3] );
        } else {
            byte3 [ 0 ] = 0x00;
        };

        /* mnemonics se mi ctou lepe, kdyz jsou napsany malym pismem */
        char *c = mnemonic;
        while ( *c != 0x00 ) {
            if ( ( *c >= 'A' ) && ( *c <= 'Z' ) ) {
                *c += 0x20;
            };
            c++;
        };

        gtk_list_store_set ( GTK_LIST_STORE ( model ), &iter,
                             DBG_DIS_ADDR, addr_row,
                             DBG_DIS_BYTES, bytecode_length,
                             DBG_DIS_ADDR_TXT, addr_txt,
                             DBG_DIS_BYTE0, byte0,
                             DBG_DIS_BYTE1, byte1,
                             DBG_DIS_BYTE2, byte2,
                             DBG_DIS_BYTE3, byte3,
                             DBG_DIS_MNEMONIC, mnemonic,
                             -1 );

        if ( i == ( DEBUGGER_HISTORY_ROWS - 1 ) ) {
            GtkTreePath *path = gtk_tree_model_get_path ( model, &iter );
            gtk_tree_view_scroll_to_cell ( ui_get_tree_view ( "dbg_history_treeview" ), path, NULL, FALSE, 0.0, 0.0 );
        };

        gtk_tree_model_iter_next ( model, &iter );
    };

}


/*
 * 
 * Vzdy je potreba nastavit addr.
 * row:
 *      -1 = update noveho seznamu
 *      0 - xx = update po editaci radku
 * 
 */
void ui_debugger_update_disassembled ( Z80EX_WORD addr, int row ) {

    GtkTreeModel *model = GTK_TREE_MODEL ( ui_get_object ( "dbg_disassembled_liststore" ) );

    if ( 0 == gtk_tree_model_iter_n_children ( model, NULL ) ) ui_debugger_init_disassembled ( model, 0, DEBUGGER_DISASSEMBLED_ROWS );

    GtkTreeIter iter;
    unsigned select_row = 0;

    if ( row == -1 ) {
        /* Provedeme kompletni update celeho seznamu a selectujeme prvni radek. */
        row = 0;
        gtk_tree_model_get_iter_first ( model, &iter );

    } else {
        /* Provedeme update jen od konkretniho radku a selectujeme nasledujici radek. */

        if ( row == ( DEBUGGER_DISASSEMBLED_ROWS - 1 ) ) {
            /* Jsme na konci seznamu. Prvni radek odstranime a za posledni jeden pridame */
            gtk_tree_model_get_iter_first ( model, &iter );
            gtk_list_store_remove ( GTK_LIST_STORE ( model ), &iter );
            ui_debugger_init_disassembled ( model, row, 1 );
            row--;
        };

        select_row = row + 1;

        char path_str [ 5 ];
        snprintf ( path_str, sizeof ( path_str ), "%d", row );
        gtk_tree_model_get_iter_from_string ( model, &iter, path_str );
    };

    //gtk_range_set_value ( ui_get_range ( "dbg_disassembled_addr_vscale" ), addr );
    gtk_range_set_value ( GTK_RANGE ( dbg_disassembled_addr_vscale ), addr );

    unsigned i;
    for ( i = row; i < DEBUGGER_DISASSEMBLED_ROWS; i++ ) {

        char addr_txt [ 6 ];
        char byte0 [ 3 ];
        char byte1 [ 3 ];
        char byte2 [ 3 ];
        char byte3 [ 3 ];
        char mnemonic [ DEBUGGER_MNEMONIC_MAXLEN ];
        int t_states, t_states2;

        sprintf ( addr_txt, "%04X:", addr );
        Z80EX_WORD addr_row = addr;

        unsigned bytecode_length = z80ex_dasm ( mnemonic, DEBUGGER_MNEMONIC_MAXLEN - 1, 0, &t_states, &t_states2, debugger_dasm_read_cb, addr, NULL );

        sprintf ( byte0, "%02X", debugger_memory_read_byte ( addr++ ) );

        if ( 1 < bytecode_length ) {
            sprintf ( byte1, "%02X", debugger_memory_read_byte ( addr++ ) );
        } else {
            byte1 [ 0 ] = 0x00;
        };

        if ( 2 < bytecode_length ) {
            sprintf ( byte2, "%02X", debugger_memory_read_byte ( addr++ ) );
        } else {
            byte2 [ 0 ] = 0x00;
        };

        if ( 3 < bytecode_length ) {
            sprintf ( byte3, "%02X", debugger_memory_read_byte ( addr++ ) );
        } else {
            byte3 [ 0 ] = 0x00;
        };

        /* mnemonics se mi ctou lepe, kdyz jsou napsany malym pismem */
        char *c = mnemonic;
        while ( *c != 0x00 ) {
            if ( ( *c >= 'A' ) && ( *c <= 'Z' ) ) {
                *c += 0x20;
            };
            c++;
        };

        gtk_list_store_set ( GTK_LIST_STORE ( model ), &iter,
                             DBG_DIS_ADDR, addr_row,
                             DBG_DIS_BYTES, bytecode_length,
                             DBG_DIS_ADDR_TXT, addr_txt,
                             DBG_DIS_BYTE0, byte0,
                             DBG_DIS_BYTE1, byte1,
                             DBG_DIS_BYTE2, byte2,
                             DBG_DIS_BYTE3, byte3,
                             DBG_DIS_MNEMONIC, mnemonic,
                             -1 );

        if ( i == select_row ) {
            GtkTreeSelection *selection = gtk_tree_view_get_selection ( ui_get_tree_view ( "dbg_disassembled_treeview" ) );
            gtk_tree_selection_select_iter ( selection, &iter );


            /* 
             * TODO: tady je nejaka chybka
             *      1) provedu editaci 15. radku na nejake vysoke adrese a ulozim jej na adresu 0x0000
             *      2) udela se full update, na 1. radku je moje nova instrukce, 2. radek je oznacen - OK
             * 
             *      CHYBA:
             *         klepnu do enter a edituju aktualni radek 15 !!!
             *         pripadne klepnu do cursor down a jsem na radku 16
             */
            GtkTreePath *path = gtk_tree_model_get_path ( model, &iter );
            gtk_tree_view_scroll_to_cell ( ui_get_tree_view ( "dbg_disassembled_treeview" ), path, NULL, FALSE, 0.0, 0.0 );

        };

        gtk_tree_model_iter_next ( model, &iter );
    };
}


void ui_debugger_update_all ( void ) {
    ui_debugger_update_flag_reg ( );
    ui_debugger_update_registers ( );
    ui_debugger_update_internals ( );
    ui_debugger_update_mmap ( );
    ui_debugger_update_stack ( );
    ui_debugger_update_disassembled ( z80ex_get_reg ( g_mz800.cpu, regPC ), -1 );
    ui_debugger_update_history ( );
}


void ui_debugger_update_animated ( void ) {

    static unsigned animate_call = 0;
    static unsigned animate_phase = 0;

    if ( animate_call++ < 2 ) return;

    switch ( animate_phase ) {

        case 0:
            ui_debugger_update_flag_reg ( );
            ui_debugger_update_registers ( );
            ui_debugger_update_internals ( );
            break;

        case 1:
            ui_debugger_update_mmap ( );
            break;

        case 2:
            ui_debugger_update_stack ( );
            break;

        case 3:
            ui_debugger_update_disassembled ( z80ex_get_reg ( g_mz800.cpu, regPC ), -1 );
            ui_debugger_update_history ( );

        case 4:
            animate_phase = 0;
            animate_call = 0;
            return;

    };
    animate_phase++;
}


/* Tyto callbacky jsou zde proto, ze glade momentalne neumi spravne vyrobit scale, takze jej produkujeme cely zde */
void on_dbg_disassembled_addr_vscale_adjust_bounds ( GtkRange *range, gdouble value, gpointer user_data ) {
    //printf ( "new addr: 0x%04X\n", (unsigned int) gtk_range_get_value ( GTK_RANGE ( dbg_disassembled_addr_vscale ) ) );
    ui_debugger_update_disassembled ( (Z80EX_WORD) gtk_range_get_value ( GTK_RANGE ( dbg_disassembled_addr_vscale ) ), -1 );
}


gboolean on_dbg_disassembled_addr_vscale_button_press_event ( GtkWidget *widget, GdkEventButton *event, gpointer user_data ) {
    if ( !TEST_EMULATION_PAUSED ) {
        ui_debugger_pause_emulation ( );
        return TRUE;
    };
    return FALSE;
}


void ui_debugger_show_spinner_window ( void ) {

    if ( g_debugger.animated_updates != DEBUGGER_ANIMATED_UPDATES_DISABLED ) return;
    if ( TEST_EMULATION_PAUSED ) return;

    GtkWidget *w_main = ui_get_widget ( "debugger_main_window" );
    if ( !gtk_widget_get_visible ( w_main ) ) return;

    gint wx, wy;
    gint x, y;
    gint wox, woy;

    gint width;
    gint height;

    GtkWidget *disassembled = ui_get_widget ( "dbg_disassembled_frame" );
    gtk_window_get_position ( GTK_WINDOW ( w_main ), &wx, &wy );
    gtk_widget_translate_coordinates ( disassembled, w_main, 0, 0, &x, &y );

    //gtk_window_get_size ( GTK_WINDOW ( w_main ), &with, &height );

    if ( GDK_IS_WINDOW ( gtk_widget_get_window ( disassembled ) ) ) {
        gdk_window_get_origin ( gtk_widget_get_window ( disassembled ), &wox, &woy );
    } else {
        wox = 0;
        woy = 0;
    };


    width = gtk_widget_get_allocated_width ( ui_get_widget ( "dbg_grid" ) );
    height = gtk_widget_get_allocated_height ( disassembled );

    GtkWidget *spinner_window = ui_get_widget ( "dbg_spinner_window" );
    gtk_window_move ( GTK_WINDOW ( spinner_window ), wox + x, woy + y );
    gtk_window_resize ( GTK_WINDOW ( spinner_window ), width, height );


    if ( gtk_widget_get_visible ( spinner_window ) ) return;

    gtk_widget_show_all ( spinner_window );
    gtk_widget_set_opacity ( spinner_window, 0.60 );

    gtk_spinner_start ( GTK_SPINNER ( ui_get_widget ( "dbg_spinner" ) ) );
    //gtk_widget_grab_focus ( w_main );
    gtk_window_set_transient_for ( GTK_WINDOW ( spinner_window ), GTK_WINDOW ( w_main ) );

    //#ifdef LINUX
    //gtk_window_set_keep_above ( GTK_WINDOW ( w ), TRUE );
    gtk_widget_set_sensitive ( disassembled, FALSE );
    gtk_widget_set_sensitive ( ui_get_widget ( "dbg_right_grid" ), FALSE );
    //#endif
}


void ui_debugger_hide_spinner_window ( void ) {
    gtk_spinner_stop ( GTK_SPINNER ( ui_get_widget ( "dbg_spinner" ) ) );
    gtk_widget_hide ( ui_get_widget ( "dbg_spinner_window" ) );
    //#ifdef LINUX
    gtk_widget_set_sensitive ( ui_get_widget ( "dbg_disassembled_frame" ), TRUE );
    gtk_widget_set_sensitive ( ui_get_widget ( "dbg_right_grid" ), TRUE );
    //#endif
}


static gboolean on_mmap_drawing_area_draw ( GtkWidget *widget, cairo_t *cr, gpointer user_data ) {
    st_UIMMAPBANK *mmbank = (st_UIMMAPBANK *) user_data;
    gdk_cairo_set_source_pixbuf ( cr, mmbank->pixbuf, 0, 0 );
    cairo_paint ( cr );
    return TRUE;
}


void ui_debugger_create_mmap_pixbuf ( GtkWidget *widget, st_UIMMAPBANK *mmbank ) {

    mmbank->drawing_area = widget;

    GtkAllocation *alloc = g_new ( GtkAllocation, 1 );
    gtk_widget_get_allocation ( mmbank->drawing_area, alloc );

    mmbank->pixbuf = gdk_pixbuf_new ( GDK_COLORSPACE_RGB, FALSE, 8, alloc->width, alloc->height );

    g_signal_connect ( G_OBJECT ( mmbank->drawing_area ), "draw",
                       G_CALLBACK ( on_mmap_drawing_area_draw ), mmbank );

    ui_debugger_set_mmap_forced ( mmbank, MMBSTATE_RAM );

    g_free ( alloc );
}


void ui_debugger_initialize_mmap ( void ) {

    g_uidebugger.last_map = -1;
    g_uidebugger.last_mmap_dmd = -1;
    g_uidebugger.last_memext = -1;
    int i;
    for ( i = 0; i < MEMEXT_RAW_MAP_SIZE; i++ ) {
        char label_name[14];
        snprintf ( label_name, sizeof ( label_name ), "mmap_memext_%X", i );
        gtk_label_set_text ( ui_get_label ( label_name ), "--" );
        g_uidebugger.last_memext_map[i] = -1;
    };


    char *g_mmap_names[MMBANK_COUNT] = {
                                        "dbg_mmap_drawingarea0",
                                        "dbg_mmap_drawingarea1",
                                        "dbg_mmap_drawingarea2",
                                        "dbg_mmap_drawingarea3",
                                        "dbg_mmap_drawingarea4",
                                        "dbg_mmap_drawingarea5",
                                        "dbg_mmap_drawingarea6",
                                        "dbg_mmap_drawingarea7",
                                        "dbg_mmap_drawingarea8",
                                        "dbg_mmap_drawingarea9",
                                        "dbg_mmap_drawingareaA",
                                        "dbg_mmap_drawingareaB",
                                        "dbg_mmap_drawingareaC",
                                        "dbg_mmap_drawingareaD",
                                        "dbg_mmap_drawingareaE_ports",
                                        "dbg_mmap_drawingareaE",
                                        "dbg_mmap_drawingareaF",
    };

    for ( i = 0; i < MMBANK_COUNT; i++ ) {
        GtkWidget *widget = ui_get_widget ( g_mmap_names[i] );
        ui_debugger_create_mmap_pixbuf ( widget, &g_uidebugger.mmapbank[i] );
    };
}


static void ui_debugger_create_tbutton_icon ( char *tbname, const char *filename ) {

    GtkWidget *img = gtk_image_new_from_file ( filename );

    if ( GTK_IMAGE_PIXBUF != gtk_image_get_storage_type ( GTK_IMAGE ( img ) ) ) {
        ui_show_error ( "%s() - can't create pixbuf from %s\n", __func__, filename );
        main_app_quit ( EXIT_FAILURE );
    };
    gtk_widget_show ( img );

    GtkToolButton *button = GTK_TOOL_BUTTON ( ui_get_widget ( tbname ) );
    gtk_tool_button_set_icon_widget ( button, img );
}


static void ui_debugger_create_toolbar_icons ( void ) {

    ui_debugger_create_tbutton_icon ( "dbg_continue_toolbutton", "ui_resources/icons/debugger/Continue24.gif" );
    ui_debugger_create_tbutton_icon ( "dbg_pause_toolbutton", "ui_resources/icons/debugger/Pause24.gif" );

#if 1
    ui_debugger_create_tbutton_icon ( "dbg_step_over_toolbutton", "ui_resources/icons/debugger/StepOver24.gif" );
    ui_debugger_create_tbutton_icon ( "dbg_step_in_toolbutton", "ui_resources/icons/debugger/StepInto24.gif" );
    ui_debugger_create_tbutton_icon ( "dbg_step_out_toolbutton", "ui_resources/icons/debugger/StepOut24.gif" );
    ui_debugger_create_tbutton_icon ( "dbg_run_to_cursor_toolbutton", "ui_resources/icons/debugger/Run_To_Cursor.gif" );
#else
    ui_debugger_create_tbutton_icon ( "dbg_step_over_toolbutton", "ui_resources/icons/debugger/step_over_instruction24.png" );
    ui_debugger_create_tbutton_icon ( "dbg_step_in_toolbutton", "ui_resources/icons/debugger/step_into_instruction24.png" );
    ui_debugger_create_tbutton_icon ( "dbg_step_out_toolbutton", "ui_resources/icons/debugger/step_out_instruction24.png" );
    ui_debugger_create_tbutton_icon ( "dbg_run_to_cursor_toolbutton", "ui_resources/icons/debugger/run_to_cursor_instruction24.png" );
#endif

    ui_debugger_create_tbutton_icon ( "dbg_breakpoints_toolbutton", "ui_resources/icons/debugger/Breakpoints24.png" );
    ui_debugger_create_tbutton_icon ( "dbg_membrowser_toolbutton", "ui_resources/icons/debugger/memory_browser24.png" );
    ui_debugger_create_tbutton_icon ( "dbg_dissassembler_toolbutton", "ui_resources/icons/debugger/dissassembler24.png" );
}


void ui_debugger_cpu_tick_counter_reset ( void ) {
    g_uidebugger.cpu_ticks_start = gdg_get_total_ticks ( );
}


void ui_debugger_show_main_window ( void ) {

    GtkWidget *window = ui_get_widget ( "debugger_main_window" );
    if ( gtk_widget_is_visible ( window ) ) {
        ui_debugger_update_all ( );
        return;
    };

    // zapneme ukladani historie
    z80ex_set_memread_callback ( g_mz800.cpu, memory_read_with_history_cb, NULL );
    debugger_reset_history ( );


    ui_main_win_move_to_pos ( GTK_WINDOW ( window ), &g_uidebugger.pos );
    gtk_widget_show ( window );
    gtk_widget_grab_focus ( window );

    /* inicializace prvku, ktere neumime udelat pres glade */
    static gboolean initialised = FALSE;
    if ( !initialised ) {
        initialised = TRUE;

        /* Tyto vlastnosti se mi nepovedlo nastavit pomoci glade */
        g_object_set ( ui_get_object ( "dbg_reg0_value_cellrenderertext" ), "editable", TRUE, "xalign", 1.0, NULL );
        g_object_set ( ui_get_object ( "dbg_reg1_value_cellrenderertext" ), "editable", TRUE, "xalign", 1.0, NULL );
        g_object_set ( ui_get_object ( "dbg_stack_value_cellrenderertext" ), "editable", TRUE, "xalign", 1.0, NULL );

        /* scale vyrobeny pres glade nefunguje spravne */
        dbg_disassembled_addr_vscale = gtk_scale_new ( GTK_ORIENTATION_VERTICAL, GTK_ADJUSTMENT ( gtk_adjustment_new ( 0, 0, 65535, 1, 256, 0 ) ) );
        gtk_widget_set_name ( dbg_disassembled_addr_vscale, "dbg_disassembled_addr_vscale" );
        gtk_widget_show ( dbg_disassembled_addr_vscale );
        gtk_box_pack_start ( GTK_BOX ( ui_get_object ( "dbg_disassembled_hbox" ) ), dbg_disassembled_addr_vscale, FALSE, TRUE, 2 );
        gtk_scale_set_draw_value ( GTK_SCALE ( dbg_disassembled_addr_vscale ), FALSE );

        g_signal_connect ( (gpointer) dbg_disassembled_addr_vscale, "adjust_bounds",
                           G_CALLBACK ( on_dbg_disassembled_addr_vscale_adjust_bounds ),
                           NULL );

        g_signal_connect ( (gpointer) dbg_disassembled_addr_vscale, "button_press_event",
                           G_CALLBACK ( on_dbg_disassembled_addr_vscale_button_press_event ),
                           NULL );


        ui_main_setpos ( &g_uidebugger.pos, -1, -1 );

        GtkTreeSelection *selection = gtk_tree_view_get_selection ( GTK_TREE_VIEW ( ui_get_object ( "dbg_history_treeview" ) ) );
        gtk_tree_selection_set_mode ( selection, GTK_SELECTION_NONE );

        ui_debugger_create_toolbar_icons ( );

        ui_debugger_initialize_mmap ( );

        // flagreg
        char checkbutton_name[] = "dbg_flagreg_bit0_checkbutton";
        int i;
        for ( i = 0; i < 8; i++ ) {
            checkbutton_name [ 15 ] = '0' + i;
            g_uidebugger.flagreg_checkbbutton[i] = ui_get_widget ( checkbutton_name );
            g_uidebugger.last_flagreg[i] = FALSE;
        };

        // internals
        g_uidebugger.im_comboboxtext = ui_get_widget ( "dbg_im_comboboxtext" );
        g_uidebugger.iff1_checkbutton = ui_get_widget ( "dbg_regiff1_checkbutton" );
        g_uidebugger.iff2_checkbutton = ui_get_widget ( "dbg_regiff2_checkbutton" );
        g_uidebugger.last_im = -1;
        g_uidebugger.last_iff1 = FALSE;
        g_uidebugger.last_iff2 = FALSE;

        // regDMD
        g_uidebugger.dmd_comboboxtext = ui_get_widget ( "dbg_regdmd_comboboxtext" );
        g_uidebugger.last_dmd = -1;

        // i8255
        g_uidebugger.i8255_cmt_in_label = ui_get_widget ( "dbg_8255_cmt_in_label" );
        g_uidebugger.i8255_cmt_out_label = ui_get_widget ( "dbg_8255_cmt_out_label" );
        g_uidebugger.i8255_cmt_motor_label = ui_get_widget ( "dbg_8255_cmt_motor_label" );
        g_uidebugger.i8255_joy1_strobe_label = ui_get_widget ( "dbg_8255_joy1_strobe_label" );
        g_uidebugger.i8255_joy2_strobe_label = ui_get_widget ( "dbg_8255_joy2_strobe_label" );
        g_uidebugger.i8255_keyboard_strobe_label = ui_get_widget ( "dbg_8255_keyboard_strobe_label" );
        g_uidebugger.i8255_cursor_timer_label = ui_get_widget ( "dbg_8255_cursor_label" );
        g_uidebugger.i8255_ctc2_mask_checkbutton = ui_get_widget ( "dbg_8255_ctc2_mask_checkbutton" );
        g_uidebugger.last_cmtin = -1;
        g_uidebugger.last_cmtout = -1;
        g_uidebugger.last_cursor_timer = -1;
        g_uidebugger.last_i8255_ctc2_mask = FALSE;

        // interrupts
        g_uidebugger.int_pioz80_label = ui_get_widget ( "dbg_interrupt_pioz80_label" );
        g_uidebugger.int_ctc2_label = ui_get_widget ( "dbg_interrupt_ctc2_label" );
        g_uidebugger.int_fdc_label = ui_get_widget ( "dbg_interrupt_fdc_label" );
        g_uidebugger.last_int_pioz80 = FALSE;
        g_uidebugger.last_int_ctc2 = FALSE;
        g_uidebugger.last_int_fdc = FALSE;

        // pioz80 pa
        g_uidebugger.pioz80_pa_icena_label = ui_get_widget ( "dbg_pioz80_pa_icena_label" );
        g_uidebugger.pioz80_pa_io_mask_label = ui_get_widget ( "dbg_pioz80_pa_io_mask_label" );
        g_uidebugger.pioz80_pa_icmask_label = ui_get_widget ( "dbg_pioz80_pa_icmask_label" );
        g_uidebugger.pioz80_pa_icfnc_label = ui_get_widget ( "dbg_pioz80_pa_icfnc_label" );
        g_uidebugger.pioz80_pa_iclvl_label = ui_get_widget ( "dbg_pioz80_pa_iclvl_label" );
        g_uidebugger.pioz80_pa_vector_label = ui_get_widget ( "dbg_pioz80_pa_vector_label" );
        g_uidebugger.last_pioz80_pa_icena = PIOZ80_ICENA_DISABLED;
        g_uidebugger.last_pioz80_pa_io_mask = 0x00;
        g_uidebugger.last_pioz80_pa_icmask = 0x00;
        g_uidebugger.last_pioz80_pa_icfnc = PIOZ80_ICFNC_OR;
        g_uidebugger.last_pioz80_pa_iclvl = PIOZ80_ICLVL_LOW;
        g_uidebugger.last_pioz80_pa_vector = 0x0000;

        // i8253
        for ( i = 0; i < 3; i++ ) {
            char buff[100];

            snprintf ( buff, sizeof ( buff ), "dbg_8253_ctc%c_mode_label", ( '0' + i ) );
            g_uidebugger.i8253_ctc[i].mode_label = ui_get_widget ( buff );

            snprintf ( buff, sizeof ( buff ), "dbg_8253_ctc%c_input_label", ( '0' + i ) );
            g_uidebugger.i8253_ctc[i].input_label = ui_get_widget ( buff );

            snprintf ( buff, sizeof ( buff ), "dbg_8253_ctc%c_output_label", ( '0' + i ) );
            g_uidebugger.i8253_ctc[i].output_label = ui_get_widget ( buff );

            snprintf ( buff, sizeof ( buff ), "dbg_8253_ctc%c_preset_label", ( '0' + i ) );
            g_uidebugger.i8253_ctc[i].preset_label = ui_get_widget ( buff );

            snprintf ( buff, sizeof ( buff ), "dbg_8253_ctc%c_value_label", ( '0' + i ) );
            g_uidebugger.i8253_ctc[i].value_label = ui_get_widget ( buff );

            snprintf ( buff, sizeof ( buff ), "dbg_8253_ctc%c_gate_label", ( '0' + i ) );
            g_uidebugger.i8253_ctc[i].gate_label = ui_get_widget ( buff );

            g_uidebugger.i8253_ctc[i].last_mode = CTC_MODE0;
            g_uidebugger.i8253_ctc[i].last_input = 0;
            g_uidebugger.i8253_ctc[i].last_output = 0;
            g_uidebugger.i8253_ctc[i].last_preset = 0x0000;
            g_uidebugger.i8253_ctc[i].last_value = 0x0000;
            g_uidebugger.i8253_ctc[i].last_gate = ( i == 0 ) ? 0 : 1;
        };

        // GDG color registry
        g_uidebugger.gdg_reg_border_comboboxtext = ui_get_widget ( "dbg_gdg_reg_border_comboboxtext" );
        g_uidebugger.last_gdg_reg_border = -1;
        g_uidebugger.gdg_reg_palgrp_comboboxtext = ui_get_widget ( "dbg_gdg_reg_palgrp_comboboxtext" );
        g_uidebugger.last_gdg_reg_palgrp = -1;
        g_uidebugger.gdg_reg_pal0_comboboxtext = ui_get_widget ( "dbg_gdg_reg_pal0_comboboxtext" );
        g_uidebugger.last_gdg_reg_pal0 = -1;
        g_uidebugger.gdg_reg_pal1_comboboxtext = ui_get_widget ( "dbg_gdg_reg_pal1_comboboxtext" );
        g_uidebugger.last_gdg_reg_pal1 = -1;
        g_uidebugger.gdg_reg_pal2_comboboxtext = ui_get_widget ( "dbg_gdg_reg_pal2_comboboxtext" );
        g_uidebugger.last_gdg_reg_pal2 = -1;
        g_uidebugger.gdg_reg_pal3_comboboxtext = ui_get_widget ( "dbg_gdg_reg_pal3_comboboxtext" );
        g_uidebugger.last_gdg_reg_pal3 = -1;

        // GDG regRF
        g_uidebugger.gdg_rfr_mode_comboboxtext = ui_get_widget ( "dbg_gdg_rfr_mode_comboboxtext" );
        g_uidebugger.gdg_rfr_bank_comboboxtext = ui_get_widget ( "dbg_gdg_rfr_bank_comboboxtext" );
        g_uidebugger.gdg_rfr_plane1_checkbutton = ui_get_widget ( "dbg_gdg_rfr_plane1_checkbutton" );
        g_uidebugger.gdg_rfr_plane2_checkbutton = ui_get_widget ( "dbg_gdg_rfr_plane2_checkbutton" );
        g_uidebugger.gdg_rfr_plane3_checkbutton = ui_get_widget ( "dbg_gdg_rfr_plane3_checkbutton" );
        g_uidebugger.gdg_rfr_plane4_checkbutton = ui_get_widget ( "dbg_gdg_rfr_plane4_checkbutton" );
        g_uidebugger.last_gdg_rfr_mode = -1;
        g_uidebugger.last_gdg_rfr_bank = -1;
        g_uidebugger.last_gdg_rfr_plane1 = FALSE;
        g_uidebugger.last_gdg_rfr_plane2 = FALSE;
        g_uidebugger.last_gdg_rfr_plane3 = FALSE;
        g_uidebugger.last_gdg_rfr_plane4 = FALSE;

        // GDG regRF
        g_uidebugger.gdg_wfr_mode_comboboxtext = ui_get_widget ( "dbg_gdg_wfr_mode_comboboxtext" );
        g_uidebugger.gdg_wfr_bank_comboboxtext = ui_get_widget ( "dbg_gdg_wfr_bank_comboboxtext" );
        g_uidebugger.gdg_wfr_plane1_checkbutton = ui_get_widget ( "dbg_gdg_wfr_plane1_checkbutton" );
        g_uidebugger.gdg_wfr_plane2_checkbutton = ui_get_widget ( "dbg_gdg_wfr_plane2_checkbutton" );
        g_uidebugger.gdg_wfr_plane3_checkbutton = ui_get_widget ( "dbg_gdg_wfr_plane3_checkbutton" );
        g_uidebugger.gdg_wfr_plane4_checkbutton = ui_get_widget ( "dbg_gdg_wfr_plane4_checkbutton" );
        g_uidebugger.last_gdg_wfr_mode = -1;
        g_uidebugger.last_gdg_wfr_bank = -1;
        g_uidebugger.last_gdg_wfr_plane1 = FALSE;
        g_uidebugger.last_gdg_wfr_plane2 = FALSE;
        g_uidebugger.last_gdg_wfr_plane3 = FALSE;
        g_uidebugger.last_gdg_wfr_plane4 = FALSE;

        // GDG signaly
        g_uidebugger.gdg_hbln_label = ui_get_widget ( "dbg_gdg_hbln_label" );
        g_uidebugger.gdg_vbln_label = ui_get_widget ( "dbg_gdg_vbln_label" );
        g_uidebugger.gdg_hsync_label = ui_get_widget ( "dbg_gdg_hsync_label" );
        g_uidebugger.gdg_vsync_label = ui_get_widget ( "dbg_gdg_vsync_label" );
        g_uidebugger.gdg_xpos_label = ui_get_widget ( "dbg_gdg_xpos_label" );
        g_uidebugger.gdg_ypos_label = ui_get_widget ( "dbg_gdg_ypos_label" );
        g_uidebugger.gdg_tempo_label = ui_get_widget ( "dbg_gdg_tempo_label" );
        g_uidebugger.gdg_cnt_label = ui_get_widget ( "dbg_gdg_cnt_label" );
        g_uidebugger.gdg_beam_label = ui_get_widget ( "dbg_gdg_beam_label" );
        g_uidebugger.last_gdg_hbln = 0;
        g_uidebugger.last_gdg_vbln = 0;
        g_uidebugger.last_gdg_hsync = 0;
        g_uidebugger.last_gdg_vsync = 0;
        g_uidebugger.last_gdg_ypos = 0;
        g_uidebugger.last_gdg_tempo = 0;
        g_uidebugger.last_gdg_beam_state = NULL;

        // GDG HW Scroll
        g_uidebugger.gdg_ssa_entry = ui_get_widget ( "dbg_hwscroll_ssa_entry" );
        g_uidebugger.gdg_sea_entry = ui_get_widget ( "dbg_hwscroll_sea_entry" );
        g_uidebugger.gdg_sw_entry = ui_get_widget ( "dbg_hwscroll_sw_entry" );
        g_uidebugger.gdg_sof_entry = ui_get_widget ( "dbg_hwscroll_sof_entry" );
        g_uidebugger.gdg_hwscroll_enabled_label = ui_get_widget ( "dbg_hwscroll_enabled_label" );
        g_uidebugger.last_gdg_ssa = -1;
        g_uidebugger.last_gdg_sea = -1;
        g_uidebugger.last_gdg_sw = -1;
        g_uidebugger.last_gdg_sof = -1;
        g_uidebugger.last_gdg_hwscroll_enabled = -1;

        // CPU ticks
        g_uidebugger.cpu_ticks_entry = ui_get_widget ( "dbg_cpu_ticks_entry" );
        ui_debugger_cpu_tick_counter_reset ( );
    };

    g_uidebugger.accelerators_locked = 1;

    if ( g_debugger.animated_updates == DEBUGGER_ANIMATED_UPDATES_DISABLED ) {
        gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "dbg_animated_disabled_radiomenuitem" ), TRUE );
        ui_debugger_show_spinner_window ( );
        printf ( "Debugger activated - animations DISABLED\n" );
    } else if ( g_debugger.animated_updates == DEBUGGER_ANIMATED_UPDATES_ENABLED ) {
        gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "dbg_animated_enabled_radiomenuitem" ), TRUE );
        ui_debugger_hide_spinner_window ( );
        printf ( "Debugger activated - animations ENABLED\n" );
    } else if ( g_debugger.animated_updates == DEBUGGER_ANIMATED_UPDATES_ENABLED_WTHOUT_AUDIO ) {
        gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "dbg_animated_enabled_without_audio_radiomenuitem" ), TRUE );
        ui_debugger_hide_spinner_window ( );
        printf ( "Debugger activated - animations ENABLED without audio\n" );
    } else {
        fprintf ( stderr, "%s():%d - Unknown animation state '%d'\n", __func__, __LINE__, g_debugger.animated_updates );
    };

    gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "dbg_screen_forced_refresh_on_edit_checkmenuitem" ), ( g_debugger.screen_refresh_on_edit ) ? TRUE : FALSE );
    gtk_check_menu_item_set_active ( ui_get_check_menu_item ( "dbg_screen_forced_refresh_at_step_checkmenuitem" ), ( g_debugger.screen_refresh_at_step ) ? TRUE : FALSE );

    g_uidebugger.accelerators_locked = 0;


    ui_debugger_update_all ( );
}


void ui_debugger_hide_main_window ( void ) {

    // vypneme ukladani historie
    z80ex_set_memread_callback ( g_mz800.cpu, memory_read_cb, NULL );

    ui_debugger_hide_spinner_window ( );
    GtkWidget *window = ui_get_widget ( "debugger_main_window" );
    ui_main_win_get_pos ( GTK_WINDOW ( window ), &g_uidebugger.pos );
    gtk_widget_hide ( window );
}


void ui_debugger_pause_emulation ( void ) {

    gchar msg[] = "Emulation was paused. Now you can make any changes...";
    mz800_pause_emulation ( 1 );
    GtkWidget *dialog = gtk_message_dialog_new ( ui_get_window ( "debugger_main_window" ), GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, msg );
    gtk_window_set_position ( GTK_WINDOW ( dialog ), GTK_WIN_POS_MOUSE );
    gtk_dialog_run ( GTK_DIALOG ( dialog ) );
    gtk_widget_destroy ( dialog );
    //ui_iteration ( );
}


void ui_debugger_focus_to_addr_history_propagatecfg_cb ( void *e, void *data ) {
    char *encoded_txt = cfgelement_get_text_value ( (CFGELM *) e );

    int ret = EXIT_FAILURE;
    long int li_array[DBG_FOCUS_ADDR_HIST_LENGTH];
    int length = cfgtools_strtol_array ( encoded_txt, li_array, DBG_FOCUS_ADDR_HIST_LENGTH, NULL, &ret );

    if ( ( length == 0 ) || ( ret != EXIT_SUCCESS ) ) {
        g_uidebugger.focus_addr_hist_count = 1;
        g_uidebugger.focus_addr_history[0] = 0x0000;
    } else {
        int i;
        for ( i = 0; i < length; i++ ) {
            g_uidebugger.focus_addr_history[i] = li_array[i];
        };
        g_uidebugger.focus_addr_hist_count = length;
    };

    g_uidebugger.last_focus_addr = g_uidebugger.focus_addr_history[0];
}


void ui_debugger_focus_to_addr_history_save_cb ( void *e, void *data ) {

    const char *separator = ", ";
    int separator_length = strlen ( separator );
    int value_length = 6;
    int count = g_uidebugger.focus_addr_hist_count;

    char *encoded_txt = ui_utils_mem_alloc0 ( ( count * value_length ) + ( ( count - 1 ) * separator_length ) + 1 );
    char *value_txt = ui_utils_mem_alloc0 ( value_length + 1 );

    int i;
    for ( i = 0; i < count; i++ ) {
        if ( i != 0 ) strncat ( encoded_txt, separator, separator_length + 1 );
        sprintf ( value_txt, "0x%04x", g_uidebugger.focus_addr_history[i] );
        strncat ( encoded_txt, value_txt, value_length + 1 );
    };

    cfgelement_set_text_value ( (CFGELM *) e, encoded_txt );

    ui_utils_mem_free ( encoded_txt );
    ui_utils_mem_free ( value_txt );
}


Z80EX_WORD ui_debugger_dissassembled_get_first_addr ( void ) {
    GtkTreeModel *model = GTK_TREE_MODEL ( ui_get_object ( "dbg_disassembled_liststore" ) );
    GtkTreeIter iter;
    gtk_tree_model_get_iter_first ( model, &iter );
    GValue gv_addr = G_VALUE_INIT;
    gtk_tree_model_get_value ( model, &iter, DBG_DIS_ADDR, &gv_addr );
    Z80EX_WORD addr = (Z80EX_WORD) g_value_get_uint ( &gv_addr );
    return addr;
}

#endif
