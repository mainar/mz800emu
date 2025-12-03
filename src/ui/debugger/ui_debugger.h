/* 
 * File:   ui_debugger.h
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 23. srpna 2015, 16:19
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

#ifndef UI_DEBUGGER_H
#define UI_DEBUGGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "z80ex/include/z80ex.h"
#include "ui/ui_main.h"
#include "pioz80/pioz80.h"
#include "ctc8253/ctc8253.h"
#include "memory/memext.h"


#define DEBUGGER_STACK_ROWS             20
#define DEBUGGER_DISASSEMBLED_ROWS      32
#define DEBUGGER_DISASSEMBLED_PGSTEP    32
#define DEBUGGER_MNEMONIC_MAXLEN        20
#define DEBUGGER_HISTORY_ROWS           DEBUGGER_HISTORY_LENGTH


    typedef enum en_DBGREGLSTORE {
        DBG_REG_ID = 0,
        DBG_REG_NAME,
        DBG_REG_SEP,
        DBG_REG_VALUE_TXT,
        DBG_REG_VALUE,
        DBG_REG_COUNT
    } en_DBGREGLSTORE;


    typedef enum en_DBGSTACKLSTORE {
        DBG_STACK_ADDR = 0,
        DBG_STACK_ADDR_TXT,
        DBG_STACK_VALUE_TXT,
        DBG_STACK_VALUE,
        DBG_STACK_COUNT
    } en_DBGSTACKLSTORE;


    typedef enum en_DBGDISLSTORE {
        DBG_DIS_ADDR = 0,
        DBG_DIS_BYTES,
        DBG_DIS_ADDR_TXT,
        DBG_DIS_BYTE0,
        DBG_DIS_BYTE1,
        DBG_DIS_BYTE2,
        DBG_DIS_BYTE3,
        DBG_DIS_MNEMONIC,
        DBG_DIS_COUNT
    } en_DBGDISLSTORE;


    typedef enum en_UI_MMBSTATE {
        MMBSTATE_RAM = 0,
        MMBSTATE_ROM,
        MMBSTATE_CGROM,
        MMBSTATE_CGRAM,
        MMBSTATE_VRAM,
        MMBSTATE_PORTS,
        MMBSTATE_COUNT
    } en_UI_MMBSTATE;


    typedef struct st_UIMMAPBANK {
        GtkWidget *drawing_area;
        GdkPixbuf *pixbuf;
        en_UI_MMBSTATE state;
    } st_UIMMAPBANK;


    typedef enum en_MMBANK {
        MMBANK_0 = 0,
        MMBANK_1,
        MMBANK_2,
        MMBANK_3,
        MMBANK_4,
        MMBANK_5,
        MMBANK_6,
        MMBANK_7,
        MMBANK_8,
        MMBANK_9,
        MMBANK_A,
        MMBANK_B,
        MMBANK_C,
        MMBANK_D,
        MMBANK_E_PORTS,
        MMBANK_E,
        MMBANK_F,
        MMBANK_COUNT
    } en_MMBANK;


#define DBG_FOCUS_ADDR_HIST_LENGTH  10


    typedef struct st_UIDEBUGGER_I8253_CTC {
        GtkWidget *mode_label;
        GtkWidget *input_label;
        GtkWidget *output_label;
        GtkWidget *preset_label;
        GtkWidget *value_label;
        GtkWidget *gate_label;
        en_CTC_MODE last_mode;
        int last_input;
        int last_output;
        Z80EX_WORD last_preset;
        Z80EX_WORD last_value;
        int last_gate;
    } st_UIDEBUGGER_I8253_CTC;


    typedef struct st_UIDEBUGGER {
        unsigned accelerators_locked; /* pri editaci zasobniku a registru blokujeme nektere akceleratory, protoze jinak by ESC zavrel cele okno */

        Z80EX_WORD last_focus_addr; // inicializuje se v ui_debugger_focus_to_addr_history_propagatecfg_cb()
        int focus_addr_hist_count;
        Z80EX_WORD focus_addr_history[DBG_FOCUS_ADDR_HIST_LENGTH];

        st_UIWINPOS pos;
        st_UIMMAPBANK mmapbank[MMBANK_COUNT];

        // flag reg
        GtkWidget *flagreg_checkbbutton[8];
        gboolean last_flagreg[8];

        // internals
        GtkWidget *im_comboboxtext;
        GtkWidget *iff1_checkbutton;
        GtkWidget *iff2_checkbutton;
        int last_im;
        gboolean last_iff1;
        gboolean last_iff2;

        // regDMD
        GtkWidget *dmd_comboboxtext;
        int last_dmd;

        // memory map
        int last_map;
        int last_mmap_dmd;
        // memext
        int last_memext;
        int last_memext_map [ MEMEXT_RAW_MAP_SIZE ];

        // 8255
        GtkWidget *i8255_cmt_in_label;
        GtkWidget *i8255_cmt_out_label;
        GtkWidget *i8255_cmt_motor_label;
        GtkWidget *i8255_joy1_strobe_label;
        GtkWidget *i8255_joy2_strobe_label;
        GtkWidget *i8255_keyboard_strobe_label;
        GtkWidget *i8255_cursor_timer_label;
        GtkWidget *i8255_ctc2_mask_checkbutton;
        int last_cmtin;
        int last_cmtout;
        int last_cmtmotor;
        int last_joy1;
        int last_joy2;
        int last_keyboard;
        int last_cursor_timer;
        gboolean last_i8255_ctc2_mask;

        // interrupts
        GtkWidget *int_pioz80_label;
        GtkWidget *int_ctc2_label;
        GtkWidget *int_fdc_label;
        gboolean last_int_pioz80;
        gboolean last_int_ctc2;
        gboolean last_int_fdc;

        // pioz80
        GtkWidget *pioz80_pa_icena_label;
        GtkWidget *pioz80_pa_io_mask_label;
        GtkWidget *pioz80_pa_icmask_label;
        GtkWidget *pioz80_pa_icfnc_label;
        GtkWidget *pioz80_pa_iclvl_label;
        GtkWidget *pioz80_pa_vector_label;
        en_PIOZ80_ICENA last_pioz80_pa_icena;
        Z80EX_BYTE last_pioz80_pa_io_mask;
        Z80EX_BYTE last_pioz80_pa_icmask;
        en_PIOZ80_ICFNC last_pioz80_pa_icfnc;
        en_PIOZ80_ICLVL last_pioz80_pa_iclvl;
        Z80EX_WORD last_pioz80_pa_vector;

        // i8253
        st_UIDEBUGGER_I8253_CTC i8253_ctc[3];

        // GDG regBORDER
        GtkWidget *gdg_reg_border_comboboxtext;
        int last_gdg_reg_border;

        // GDG regPALGRP
        GtkWidget *gdg_reg_palgrp_comboboxtext;
        int last_gdg_reg_palgrp;

        // GDG regPAL[0-3]
        GtkWidget *gdg_reg_pal0_comboboxtext;
        int last_gdg_reg_pal0;
        GtkWidget *gdg_reg_pal1_comboboxtext;
        int last_gdg_reg_pal1;
        GtkWidget *gdg_reg_pal2_comboboxtext;
        int last_gdg_reg_pal2;
        GtkWidget *gdg_reg_pal3_comboboxtext;
        int last_gdg_reg_pal3;

        // GDG regRF
        GtkWidget *gdg_rfr_mode_comboboxtext;
        GtkWidget *gdg_rfr_bank_comboboxtext;
        GtkWidget *gdg_rfr_plane1_checkbutton;
        GtkWidget *gdg_rfr_plane2_checkbutton;
        GtkWidget *gdg_rfr_plane3_checkbutton;
        GtkWidget *gdg_rfr_plane4_checkbutton;
        int last_gdg_rfr_mode;
        int last_gdg_rfr_bank;
        gboolean last_gdg_rfr_plane1;
        gboolean last_gdg_rfr_plane2;
        gboolean last_gdg_rfr_plane3;
        gboolean last_gdg_rfr_plane4;

        // GDG regWF
        GtkWidget *gdg_wfr_mode_comboboxtext;
        GtkWidget *gdg_wfr_bank_comboboxtext;
        GtkWidget *gdg_wfr_plane1_checkbutton;
        GtkWidget *gdg_wfr_plane2_checkbutton;
        GtkWidget *gdg_wfr_plane3_checkbutton;
        GtkWidget *gdg_wfr_plane4_checkbutton;
        int last_gdg_wfr_mode;
        int last_gdg_wfr_bank;
        gboolean last_gdg_wfr_plane1;
        gboolean last_gdg_wfr_plane2;
        gboolean last_gdg_wfr_plane3;
        gboolean last_gdg_wfr_plane4;

        // GDG signaly
        GtkWidget *gdg_hbln_label;
        GtkWidget *gdg_vbln_label;
        GtkWidget *gdg_hsync_label;
        GtkWidget *gdg_vsync_label;
        GtkWidget *gdg_xpos_label;
        GtkWidget *gdg_ypos_label;
        GtkWidget *gdg_tempo_label;
        GtkWidget *gdg_cnt_label;
        GtkWidget *gdg_beam_label;
        int last_gdg_hbln;
        int last_gdg_vbln;
        int last_gdg_hsync;
        int last_gdg_vsync;
        //int last_gdg_xpos; // nema smysl - pokazde bude jiny
        int last_gdg_ypos;
        int last_gdg_tempo;
        //int last_gdg_cnt; // nema smysl - pokazde bude jiny
        const char *last_gdg_beam_state;

        // GDG HW Scroll
        GtkWidget *gdg_ssa_entry;
        GtkWidget *gdg_sea_entry;
        GtkWidget *gdg_sw_entry;
        GtkWidget *gdg_sof_entry;
        GtkWidget *gdg_hwscroll_enabled_label;
        int last_gdg_ssa;
        int last_gdg_sea;
        int last_gdg_sw;
        int last_gdg_sof;
        int last_gdg_hwscroll_enabled;

        // CPU ticks
        GtkWidget *cpu_ticks_entry;
        uint64_t cpu_ticks_start;

    } st_UIDEBUGGER;

    extern struct st_UIDEBUGGER g_uidebugger;

    extern void ui_debugger_show_main_window ( void );
    extern void ui_debugger_hide_main_window ( void );
    extern void ui_debugger_pause_emulation ( void );

    extern void ui_debugger_show_spinner_window ( void );
    extern void ui_debugger_hide_spinner_window ( void );

    extern void ui_debugger_update_all ( void );
    extern void ui_debugger_update_animated ( void );
    extern void ui_debugger_update_mmap ( void );
    extern void ui_debugger_update_flag_reg ( void );
    extern void ui_debugger_update_registers ( void );
    extern void ui_debugger_update_stack ( void );
    extern void ui_debugger_update_cpu_ticks ( void );
    extern void ui_debugger_update_disassembled ( Z80EX_WORD addr, int row );

    extern void ui_debugger_focus_to_addr_history_propagatecfg_cb ( void *e, void *data );
    extern void ui_debugger_focus_to_addr_history_save_cb ( void *e, void *data );

    extern void ui_debugger_cpu_tick_counter_reset ( void );

    extern Z80EX_WORD ui_debugger_dissassembled_get_first_addr ( void );
#ifdef __cplusplus
}
#endif

#endif /* UI_DEBUGGER_H */

