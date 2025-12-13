/* 
 * File:   mz800.h
 * Author: chaky
 *
 * Created on 14. června 2015, 16:16
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

#ifndef MZ800_H
#define MZ800_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mz800emu_cfg.h"

#include "z80ex/include/z80ex.h"


    typedef enum en_MZ800SWITCH {
        MZ800SWITCH_OFF = 0,
        MZ800SWITCH_ON,
    } en_MZ800SWITCH;

#define MZ800_INTERRUPT_CTC2    ( 1 << 0 )
#define MZ800_INTERRUPT_PIOZ80  ( 1 << 1 )
#define MZ800_INTERRUPT_FDC     ( 1 << 2 )


    typedef enum en_MZEVENT {
        //        EVENT_GDG_STS_HSYNC_END,
        EVENT_GDG_HBLN_END,
        EVENT_GDG_HBLN_START,
        EVENT_GDG_STS_VSYNC_END,
        EVENT_GDG_STS_VSYNC_START,
        EVENT_GDG_AFTER_LAST_SCREEN_PIXEL,
        //        EVENT_GDG_STS_HSYNC_START,
        EVENT_GDG_AFTER_LAST_VISIBLE_PIXEL,
        EVENT_GDG_REAL_HSYNC_START,
        EVENT_GDG_SCREEN_ROW_END,

        // jine, nez GDG eventy
        EVENT_NO_GDG, /* pouze hranicni hodnota - neni skutecny event */

        EVENT_PIOZ80,

#ifdef MZ800EMU_CFG_CLK1M1_FAST
        EVENT_CTC0,
#endif

#ifdef MZ800EMU_CFG_VARIABLE_SPEED
        EVENT_SPEED_SYNC,
#endif

        EVENT_BREAK, /* pouze hranicni hodnota - neni skutecny event */

        // V prubehu zpracovani instrukce vznikl MZ800 interrupt.
        // Tento event slouzi k tomu, aby jsme vybehli z instrukcni smycky a pokusili se jej prevzit.
        EVENT_BREAK_MZ800_INTERRUPT,

        EVENT_BREAK_EMULATION_PAUSED,
    } en_MZEVENT;


    typedef struct st_EVENT {
        en_MZEVENT event_name;
        unsigned ticks;
    } st_EVENT;

#define MZ800_EMULATION_SPEED_NORMAL    0
#define MZ800_EMULATION_SPEED_MAX       1


    typedef enum en_DEVELMODE {
        DEVELMODE_NO = 0,
        DEVELMODE_YES,
    } en_DEVELMODE;

    typedef enum en_HWCOMPAT_MZ700_PAL_TIMING {
        HWCOMPAT_MZ700_PAL_TIMING_NO = 0,
        HWCOMPAT_MZ700_PAL_TIMING_YES,
    } en_HWCOMPAT_MZ700_PAL_TIMING;

    typedef enum en_HWCOMPAT_MZ700_FIXED_E008 {
        HWCOMPAT_MZ700_FIXED_E008_NO = 0,
        HWCOMPAT_MZ700_FIXED_E008_YES,
    } en_HWCOMPAT_MZ700_FIXED_E008;

    typedef struct st_mz800 {
        Z80EX_CONTEXT *cpu; /* Model Z80ex */

        unsigned cursor_timer;

        Z80EX_WORD instruction_addr; /* posledni adresa na ktere se nabirala instrukce */

        int instruction_tstates; /* citac tstates vykonanych v instrukcnim cyklu */
        int instruction_insideop_sync_ticks; /* citac GDG ticks vykonanych v instrukcnim cyklu, ktere uz jsme v ramci sync vykonali */

        Z80EX_BYTE regDBUS_latch; /* Obsahuje esoterickeho ducha hodnoty posledniho bajtu, ktery byl precten na datove sbernici */

        int pio8255_ct53g7; /* rizeni CTC0 v rezimu MZ700 */

        unsigned use_max_emulation_speed; /* MZ800_EMULATION_SPEED_NORMAL = 0, MZ800_EMULATION_SPEED_MAX = 1 */
        unsigned emulation_paused;

        st_EVENT event;

        unsigned status_changed;
        unsigned status_emulation_speed;

        unsigned interrupt;

        en_DEVELMODE development_mode;
        en_HWCOMPAT_MZ700_PAL_TIMING hwcompatibility_mz700_pal_timing;
        en_HWCOMPAT_MZ700_FIXED_E008 hwcompatibility_mz700_fixed_e008;

        en_MZ800SWITCH mz800_switch;

#ifdef MZ800EMU_CFG_VARIABLE_SPEED        
        st_EVENT speed_sync_event;
        unsigned speed_in_percentage; /* pozadovana rychlost v procentech 1 - 500, default 100 % */
        unsigned speed_frame_width;
#endif
    } st_mz800;

    extern struct st_mz800 g_mz800;

#define TEST_EMULATION_PAUSED       ( g_mz800.emulation_paused != 0 )
#define TEST_MAX_EMULATION_SPEED    ( g_mz800.use_max_emulation_speed == 1 )

#define SET_MZ800_EVENT(e,t) {\
    g_mz800.event.event_name = e;\
    g_mz800.event.ticks = t;\
}


    typedef enum en_INSIDEOP {
        // IORQ, MREQ a MREQ_E00x se od sebe nicim nelisi - jsoui rozdeleny jen pro lepsi debugovani
        INSIDEOP_IORQ = 0, /* operace PREAD, PWRITE */
        INSIDEOP_MREQ, /* obecna operace MREQ */
        INSIDEOP_MREQ_E00x, /* MREQ pro mapovane porty */
        // Synchronizace MZ700 VRAM MREQ pri neaktivnim VBLN
        INSIDEOP_MREQ_MZ700_VRAMCTRL, /* MZ700 VRAM MREQ */
        INSIDEOP_IORQ_PSG_WRITE, /* IORQ wite byte do PSG */
    } en_INSIDEOP;

    extern void mz800_interrupt_manager ( void );

    extern void mz800_reset ( void );
    extern void mz800_init ( void );
    extern void mz800_main ( const char *mzf_file_to_load );
    extern void mz800_exit ( void );
    extern void mz800_set_display_mode ( Z80EX_BYTE dmd_mode, unsigned event_ticks );

    extern void mz800_sync_insideop_iorq ( void );
    extern void mz800_sync_insideop_mreq ( void );
    extern void mz800_sync_insideop_mreq_e00x ( void );
    extern void mz800_sync_insideop_mreq_mz700_vramctrl ( void );
    extern void mz800_sync_insideop_iorq_psg_write ( void );

#include "gdg/gdg.h"

#define mz800_sync_insideop_mreq_mz800_vramctrl() {\
    int ticks_to_sync = 16 - ( gdg_get_total_ticks ( ) % 16 );\
    if ( ticks_to_sync <= 7 ) {\
        ticks_to_sync += 8;\
    };\
    ticks_to_sync += 16;\
    g_mz800.instruction_insideop_sync_ticks -= ticks_to_sync;\
}

    extern void mz800_flush_full_screen ( void );

    extern void mz800_switch_emulation_speed ( unsigned emulation_speed );
    extern void mz800_pause_emulation ( unsigned value );

#ifdef MZ800EMU_CFG_DEBUGGER_ENABLED
    extern void mz800_run_to_temporary_breakpoint ( void );
#endif

    extern void mz800_rear_dip_switch_mz800_mode ( unsigned value );
    
    extern void mz800_hwcompatibility_mz700_pal_timing ( unsigned value );
    extern void mz800_hwcompatibility_mz700_fixed_e008 ( unsigned value );

#define mz800_get_instruction_start_ticks() ( g_gdg.total_elapsed.ticks - g_mz800.instruction_insideop_sync_ticks ) /* muze legalne vracet i zaporne cislo! */

#define mz800_cursor_timer_reset() { g_mz800.cursor_timer = 0; }
#define mz800_get_cursor_timer_state() (  ( g_mz800.cursor_timer / 25 ) & 1 )

#ifdef __cplusplus
}
#endif

#endif /* MZ800_H */

