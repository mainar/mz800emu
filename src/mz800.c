/* 
 * File:   mz800.c
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

#include "mz800emu_cfg.h"

#include <stdio.h>
#include <stdlib.h>

#include "main.h"
#include "cfgmain.h"

#include "z80ex/include/z80ex.h"

#include "mz800.h"
#include "gdg/gdg.h"
#include "gdg/video.h"
#include "gdg/framebuffer.h"
#include "gdg/vramctrl.h"

#include "memory/memory.h"
#include "port.h"
#include "ctc8253/ctc8253.h"
#include "pio8255/pio8255.h"
#include "pioz80/pioz80.h"
#include "psg/psg.h"
#include "audio.h"

#include "fdc/fdc.h"
#include "ramdisk/ramdisk.h"
#include "cmt/cmt.h"
#include "cmt/cmthack.h"
#include "qdisk/qdisk.h"
#include "joy/joy.h"
#include "unicard/unicard.h"
#include "ide8/ide8.h"

#include "src/iface_sdl/iface_sdl.h"
#include "iface_sdl/iface_sdl_audio.h"

#include "libs/mztape/mztape.h"

// ve Win32 neni ???
//#include <SDL2/SDL_assert.h>

#include "ui/ui_main.h"
#include "typedefs.h"

#include "version_check/version_check.h"

#ifdef MZ800EMU_CFG_DEBUGGER_ENABLED
#include "debugger/debugger.h"
#include "debugger/breakpoints.h"
#include "ui/debugger/ui_breakpoints.h"
#endif


#define DBGLEVEL (DBGNON /* | DBGERR | DBGWAR | DBGINF*/)
//#define DBGLEVEL (DBGNON | DBGERR | DBGWAR | DBGINF )
#include "debug.h"
#include "ui/debugger/ui_debugger.h"


struct st_mz800 g_mz800;


/**
 * Vyhledame a nastavime nejblizsi HW event, podle nasledujici priority:
 * 
 * CTC0
 * CMT
 * PIOZ80
 * 
 * GDG
 * SPEED_SYNC
 * 
 */
static inline void mz800_set_proximate_hw_event ( void ) {

    st_EVENT *proximate_event;

#ifdef MZ800EMU_CFG_CLK1M1_FAST
    if ( g_pioz80.icena_event.ticks <= g_ctc8253[CTC_CS0].clk1m1_event.ticks ) {
        proximate_event = &g_pioz80.icena_event;
    } else {
        proximate_event = &g_ctc8253[CTC_CS0].clk1m1_event;
    };

    if ( g_gdg.event.ticks <= proximate_event->ticks ) {
        proximate_event = &g_gdg.event;
    };
#else
    if ( g_gdg.event.ticks <= g_pioz80.icena_event.ticks ) {
        proximate_event = &g_gdg.event;
    } else {
        proximate_event = &g_pioz80.icena_event;
    };
#endif

#ifdef MZ800EMU_CFG_VARIABLE_SPEED
    if ( g_mz800.speed_sync_event.ticks <= proximate_event->ticks ) {
        proximate_event = &g_mz800.speed_sync_event;
    };
#endif

    g_mz800.event.event_name = proximate_event->event_name;
    g_mz800.event.ticks = proximate_event->ticks;
}


/**
 * Nastav nasledujici GDG event
 * 
 */
static inline void mz800_gdg_event_set_next ( void ) {

    if ( g_gdg.event.event_name == EVENT_GDG_SCREEN_ROW_END ) {
        g_gdg.event.event_name = 0;
    } else {
        g_gdg.event.event_name++;
    };

    while ( 1 ) {
        if ( ( g_gdgevent[ g_gdg.event.event_name ].start_row <= g_gdg.beam_row ) &&
             ( ( g_gdgevent [ g_gdg.event.event_name ].start_row + g_gdgevent[ g_gdg.event.event_name ].num_rows - 1 ) >= g_gdg.beam_row ) ) {

            g_gdg.event.ticks = g_gdg.beam_row * VIDEO_SCREEN_WIDTH + g_gdgevent[ g_gdg.event.event_name ].event_column;

            break;
        };
        g_gdg.event.event_name++;
    };
}


void mz800_reset ( void ) {

    printf ( "\nMZ800 Reset!\n" );
    if ( TEST_EMULATION_PAUSED ) {
        printf ( "Emulation is still PAUSED!\n" );
    };
    printf ( "\n" );

    // TODO: tohle se nedeje
    g_mz800.interrupt = 0;

    g_mz800.instruction_addr = 0x0000;

    memory_reset ( );
    gdg_reset ( );

    z80ex_reset ( g_mz800.cpu );
    pioz80_reset ( );
    fdc_reset ( );
    unicard_reset ( );
    ide8_reset ( );

    cmthack_reset ( );

#ifdef MZ800EMU_CFG_DEBUGGER_ENABLED
    debugger_update_all ( );
#endif
}


void mz800_exit ( void ) {
    fdc_exit ( );
    qdisk_exit ( );
    ramdisc_exit ( );
    cmt_exit ( );
    unicard_exit ( );
    ide8_exit ( );
#ifdef MZ800EMU_CFG_DEBUGGER_ENABLED
    debugger_exit ( );
#endif
#ifdef MEMORY_MAKE_STATISTICS
    memory_write_memory_statistics ( );
#endif
}


/* tady resime rozdily chovani HW mezi MZ rezimy */
void mz800_set_display_mode ( Z80EX_BYTE dmd_mode, unsigned event_ticks ) {

    /* TODO: doplnit ovladani citace kurzoru */

    if ( dmd_mode & REGISTER_DMD_FLAG_MZ700 ) {
        /* rezim MZ-700 */
        ctc8253_gate ( 0, g_gdg.regct53g7, event_ticks );

    } else {
        /* rezim MZ-800 */
        ctc8253_gate ( 0, 1, event_ticks );
        g_vramctrl.mz700_wr_latch_is_used = 0;
    };

    g_gdg.regDMD = dmd_mode;
}




/* TODO: nejak rozumne pouklizet tyhle globalni promenne */
unsigned flag_make_picture_time = 1;
unsigned flag_update_status_time = 1;
#ifdef MZ800EMU_CFG_DEBUGGER_ENABLED
unsigned flag_update_debugger_time = 1;
#endif

#define INTERRUPT_TIMER_MS          20
#define INTERRUPT_MAKEPIC_PER_SEC   25
#define INTERRUPT_POOL_EVENTS_PER_SEC   20


uint32_t screens_counter_flush ( uint32_t interval, void* param ) {
    static unsigned make_pic_counter = 0;
    static unsigned call_counter = 0;

    if ( make_pic_counter == 0 ) {
        //printf ("newpic\n");
        flag_make_picture_time++;
        make_pic_counter = ( 1000 / INTERRUPT_TIMER_MS / INTERRUPT_MAKEPIC_PER_SEC ) - 1;
    } else {
        make_pic_counter--;
    };

#ifdef MZ800EMU_CFG_DEBUGGER_ENABLED
    static unsigned debugger_update_counter = 0;

    if ( !TEST_EMULATION_PAUSED ) {
        if ( TEST_DEBUGGER_ACTIVE ) {
            if ( flag_update_debugger_time != 1 ) {
                if ( debugger_update_counter++ >= 2 ) {
                    debugger_update_counter = 0;
                    flag_update_debugger_time = 1;
                    SET_MZ800_EVENT ( EVENT_BREAK_EMULATION_PAUSED, 0 );
                } else {
                    debugger_update_counter++;
                };
            };
        };
    };
#endif

    if ( call_counter == 0 ) {
        call_counter = ( ( 1000 / INTERRUPT_TIMER_MS ) ) - 1;
        static unsigned last = 0;
        unsigned i = g_gdg.total_elapsed.screens;
        unsigned j = i - last;
        last = i;
        g_mz800.status_emulation_speed = (float) j / 0.5;
        flag_update_status_time = 1;
    } else {
        call_counter--;
    };

    return interval;
}


static inline void mz800_event_speed_synchronisation ( unsigned event_ticks ) {

#ifdef AUDIO_FILLBUFF_v1
    audio_fill_buffer_v1 ( event_ticks );
    g_audio.last_update = 0;
    g_audio.buffer_position = 0;
#endif

#ifdef AUDIO_FILLBUFF_v2
    audio_fill_buffer_v2 ( gdg_compute_total_ticks ( event_ticks ) );
#endif

#ifndef MZ800EMU_CFG_AUDIO_DISABLED
    if ( !TEST_EMULATION_PAUSED ) {
        iface_sdl_audio_sync_20ms_cycle ( );
    };
#endif

#ifdef MZ800EMU_CFG_VARIABLE_SPEED
    g_mz800.speed_sync_event.ticks += g_mz800.speed_frame_width;

#endif
}


static inline void mz800_event_screen_done ( void ) {

    static unsigned last_make_picture_time;

#ifdef MZ800EMU_CFG_VARIABLE_SPEED
    if ( ( ( g_mz800.use_max_emulation_speed == 0 ) && ( g_mz800.speed_in_percentage == 100 ) ) || ( last_make_picture_time != flag_make_picture_time ) ) {
#else
    if ( ( g_mz800.use_max_emulation_speed == 0 ) || ( last_make_picture_time != flag_make_picture_time ) ) {
#endif
        last_make_picture_time = flag_make_picture_time;

#ifdef MZ800EMU_CFG_DEBUGGER_ENABLED
        if ( g_debugger.animated_updates != DEBUGGER_ANIMATED_UPDATES_DISABLED ) debugger_animation ( );
#endif

        if ( flag_update_status_time ) {
            flag_update_status_time = 0;
            iface_sdl_render_status_line ( );
        };

        iface_sdl_pool_all_events ( );

        if ( TEST_VERSION_CHECK_THREAD_DONE ) {
            version_check_parse_thread_response ( );
        };

        ui_iteration ( );

        if ( g_gdg.framebuffer_state || g_iface_sdl.redraw_full_screen_request ) {
            iface_sdl_update_window ( );
            g_gdg.framebuffer_state = FB_STATE_NOT_CHANGED;
            g_gdg.screen_is_already_rendered_at_beam_pos = g_mz800.event.ticks;
        };

#ifndef MZ800EMU_CFG_VARIABLE_SPEED
        mz800_event_speed_synchronisation ( g_mz800.event.ticks );
#endif
    };

#ifdef MZ800EMU_CFG_CLK1M1_FAST
    ctc8253_on_screen_done_event ( );
#else
    g_gdg.ctc0clk++;
#endif

    pioz80_on_screen_done_event ( );

#ifdef MZ800EMU_CFG_VARIABLE_SPEED
    g_mz800.speed_sync_event.ticks -= VIDEO_SCREEN_TICKS;
#endif

    cmt_on_screen_done_event ( );

    g_gdg.total_elapsed.screens++;
    g_mz800.cursor_timer++;

#ifdef MZ800EMU_CFG_SPEED_TEST
    if ( g_gdg.total_elapsed.screens > 1000 ) main_app_quit ( EXIT_SUCCESS );
#endif

    // proboha, proc -1 ??? :)
    //     g_gdg.total_elapsed.ticks -= ( VIDEO_SCREEN_TICKS - 1 );
    g_gdg.total_elapsed.ticks -= VIDEO_SCREEN_TICKS;
    g_gdg.beam_row = 0;
}


static inline void mz800_sync ( void ) {

    while ( g_gdg.total_elapsed.ticks >= g_mz800.event.ticks ) {

        // Nejprve odbavime eventy, ktere nepochazeji z GDG
        while ( g_mz800.event.event_name >= EVENT_NO_GDG ) {

#ifdef MZ800EMU_CFG_CLK1M1_FAST
            if ( g_ctc8253[CTC_CS0].clk1m1_event.ticks <= g_mz800.event.ticks ) {
                ctc8253_ctc1m1_event ( g_mz800.event.ticks );
            };
#endif
            if ( g_pioz80.icena_event.ticks <= g_mz800.event.ticks ) {
                pioz80_icena_event ( );
            };

#ifdef MZ800EMU_CFG_VARIABLE_SPEED
            if ( g_mz800.speed_sync_event.ticks <= g_mz800.event.ticks ) {
                mz800_event_speed_synchronisation ( g_mz800.event.ticks );
            };
#endif
            mz800_set_proximate_hw_event ( );

            if ( !( g_gdg.total_elapsed.ticks >= g_mz800.event.ticks ) ) return;
        };


        switch ( g_mz800.event.event_name ) {

            case EVENT_GDG_HBLN_END:
                g_gdg.hbln = HBLN_OFF;

                g_gdg.screen_is_already_rendered_at_beam_pos = g_mz800.event.ticks;

                /* V rezimu MZ-700 aktualizujeme screen framebuffer jakmile skonci HBLN. */
                if ( ( DMD_TEST_MZ700 ) && ( ( g_gdg.beam_row >= VIDEO_BEAM_CANVAS_FIRST_ROW ) && ( g_gdg.beam_row <= VIDEO_BEAM_CANVAS_LAST_ROW ) ) ) {
                    if ( g_gdg.screen_changes ) {
                        framebuffer_update_MZ700_current_screen_row ( );
                    };

                    /* Pokud jsme na poslednim pixelu screen area */
                    if ( g_gdg.beam_row == VIDEO_BEAM_CANVAS_LAST_ROW ) {
                        if ( g_gdg.screen_changes ) {
                            g_gdg.screen_changes--;
                            g_gdg.framebuffer_state |= FB_STATE_SCREEN_CHANGED;
                        };
                    };
                };
                break;


            case EVENT_GDG_HBLN_START:
                g_gdg.hbln = HBLN_ACTIVE;
                g_vramctrl.mz700_wr_latch_is_used = 0;

                unsigned last_vbln_state = g_gdg.vbln;

                if ( g_gdg.beam_row == VIDEO_BEAM_CANVAS_LAST_ROW ) {
                    g_gdg.vbln = VBLN_ACTIVE;
                } else if ( g_gdg.beam_row == VIDEO_BEAM_CANVAS_FIRST_ROW - 1 ) {
                    g_gdg.vbln = VBLN_OFF;
                };

                if ( last_vbln_state != g_gdg.vbln ) {
                    /* udalost pro PIO-Z80 - VBLN */
                    pioz80_port_id_event ( PIOZ80_PORT_A, PIOZ80_PORT_EVENT_PA5_VBLN, g_gdg.vbln );
                };
                break;


            case EVENT_GDG_STS_VSYNC_END:
                g_gdg.sts_vsync = VSYN_OFF;
                break;


            case EVENT_GDG_STS_VSYNC_START:
                g_gdg.sts_vsync = VSYN_ACTIVE;
                break;


            case EVENT_GDG_AFTER_LAST_SCREEN_PIXEL:
                /* V rezimu MZ-800 aktualizujeme screen framebuffer az po dokoncenem radku. */
                if ( !DMD_TEST_MZ700 ) {
                    if ( g_gdg.screen_changes ) {
                        framebuffer_MZ800_current_screen_row_fill ( VIDEO_CANVAS_WIDTH );
                    };

                    /* Pokud jsme na poslednim pixelu screen area */
                    if ( g_gdg.beam_row == VIDEO_BEAM_CANVAS_LAST_ROW ) {
                        if ( g_gdg.screen_changes ) {
                            g_gdg.screen_changes--;
                            g_gdg.framebuffer_state |= FB_STATE_SCREEN_CHANGED;
                        };
                    };
                };
                break;

#if 0
            case EVENT_GDG_STS_HSYNC_START:
                g_gdg.sts_hsync = 0;
                break;
#endif

            case EVENT_GDG_AFTER_LAST_VISIBLE_PIXEL:
                g_gdg.sts_hsync = HSYN_ACTIVE; /* aktivni o par ticku drive */
                /* Jsme skutecne jeste ve viditelne casti obrazu? */
                if ( g_gdg.beam_row < VIDEO_DISPLAY_HEIGHT ) {
                    if ( g_gdg.border_changes ) {
                        framebuffer_border_current_row_fill ( );
                    };
                    g_gdg.last_updated_border_pixel = VIDEO_BEAM_DISPLAY_LAST_COLUMN + 1;

                    /* Pokud jsme na poslednim pixelu visible area */
                    if ( g_gdg.beam_row == VIDEO_BEAM_DISPLAY_LAST_ROW ) {
                        if ( g_gdg.border_changes ) {
                            g_gdg.border_changes--;
                            g_gdg.framebuffer_state |= FB_STATE_BORDER_CHANGED;
                        };
                    };
                };
                break;


            case EVENT_GDG_REAL_HSYNC_START:
                /* tady zacina skutecny HSYNC, ktery je na RGBI a jeho sestupna hrana je CTC1_CLK */
                ctc8253_clkfall ( CTC_CS1, g_mz800.event.ticks );
                break;


            case EVENT_GDG_SCREEN_ROW_END:
                g_gdg.sts_hsync = HSYN_OFF;
                g_gdg.tempo_divider++;
                /* Na mem MZ800 ma TEMPO pravdepodobne cca 34 Hz - tedy od oka :) */
                if ( g_gdg.tempo_divider == 229 ) {
                    g_gdg.tempo_divider = 0;
                    g_gdg.tempo++;
                };

                /* Muzeme vynulovat update pozici borderu ve framebufferu? */
                /* (Pokud ma jinou hodnotu, tak to znamena, ze pres OUT doslo ke zmene radku, ktery teprve nastane.) */
                if ( g_gdg.last_updated_border_pixel == VIDEO_BEAM_DISPLAY_LAST_COLUMN + 1 ) {
                    g_gdg.last_updated_border_pixel = 0;
                };

                g_gdg.screen_need_update_from = 0;

                if ( g_gdg.beam_row < VIDEO_SCREEN_HEIGHT - 1 ) {
                    g_gdg.beam_row++;
                } else {
                    mz800_event_screen_done ( );
                };
                break;

                /* tyto eventy neni potreba zde resit */
            case EVENT_NO_GDG:
            case EVENT_BREAK:
            case EVENT_BREAK_MZ800_INTERRUPT:
            case EVENT_BREAK_EMULATION_PAUSED:
            case EVENT_PIOZ80:
#ifdef MZ800EMU_CFG_CLK1M1_FAST
            case EVENT_CTC0:
#endif

#ifdef MZ800EMU_CFG_VARIABLE_SPEED
            case EVENT_SPEED_SYNC:
#endif
                break;
        };

        mz800_gdg_event_set_next ( );
        mz800_set_proximate_hw_event ( );
    };
}

#ifdef MZ800EMU_CFG_CLK1M1_SLOW


static inline void mz800_sync_ctc0_and_cmt ( unsigned instruction_ticks ) {

    g_gdg.total_elapsed.ticks -= g_gdg.ctc0clk;
    instruction_ticks += g_gdg.ctc0clk;

    while ( instruction_ticks > GDGCLK_1M1_DIVIDER - 1 ) {

        g_gdg.total_elapsed.ticks += GDGCLK_1M1_DIVIDER;
        instruction_ticks -= GDGCLK_1M1_DIVIDER;

        ctc8253_clkfall ( CTC_CS0, g_gdg.total_elapsed.ticks );

        // uz neexistuje
#if 0
        /* TODO: prozatim si sem povesime i pomaly cmt_step() */
        if ( TEST_CMT_PLAYING ) {
            cmt_step ( );
        };
#endif
    };

    g_gdg.ctc0clk = instruction_ticks;
    g_gdg.total_elapsed.ticks += instruction_ticks;
}

#endif


static inline void mz800_sync_insideop ( const en_INSIDEOP insideop ) {

    unsigned tstates = 0;
    unsigned instruction_ticks = 0;
    unsigned ticks_to_sync = 0;
    unsigned tstates_to_psg_sync = 0;

    if ( g_mz800.event.event_name >= EVENT_BREAK ) {
        mz800_set_proximate_hw_event ( );
    };

#if 0
    if ( insideop == INSIDEOP_IORQ ) {
        // dbg
        Z80EX_BYTE byte = memory_read_byte ( g_mz800.instruction_addr );
        if ( byte != 0xd3 ) {
            printf ( "0x%04x: 0x%02x, 0x%02x\n", g_mz800.instruction_addr, byte, memory_read_byte ( g_mz800.instruction_addr ) );
            byte = 0;
        };
    };
#endif

    switch ( insideop ) {
        case INSIDEOP_MREQ_MZ700_VRAMCTRL:
            /* V tuto chvili pocitame s tim, ze uz mame synchronizovano po g_gdg.total_elapsed.ticks */
            ticks_to_sync = VIDEO_BEAM_HBLN_FIRST_COLUMN - VIDEO_GET_SCREEN_COL ( g_gdg.total_elapsed.ticks );
            ticks_to_sync += GDGCLK2CPU_DIVIDER - ( ticks_to_sync % GDGCLK2CPU_DIVIDER );
            z80ex_w_states ( g_mz800.cpu, ( ticks_to_sync / GDGCLK2CPU_DIVIDER ) );
            instruction_ticks = g_mz800.instruction_insideop_sync_ticks + ticks_to_sync;
            break;

        case INSIDEOP_IORQ_PSG_WRITE:
            tstates_to_psg_sync = 16 - ( gdg_get_total_ticks ( ) / GDGCLK2CPU_DIVIDER ) % 16;
            // tohle je muj odhad, kdyz jsem to testoval s analyzerem, tak 1 cpu takt nestacil, zatimco 5 uz bylo OK
            if ( tstates_to_psg_sync < 4 ) {
                tstates_to_psg_sync += 16;
            };
            tstates_to_psg_sync += 16;
            z80ex_w_states ( g_mz800.cpu, tstates_to_psg_sync );
            ticks_to_sync = tstates_to_psg_sync * GDGCLK2CPU_DIVIDER;
            instruction_ticks = g_mz800.instruction_insideop_sync_ticks + ticks_to_sync;
            break;

        default:
            tstates = g_mz800.instruction_tstates + z80ex_op_tstate ( g_mz800.cpu );
            instruction_ticks = tstates * GDGCLK2CPU_DIVIDER;
            ticks_to_sync = instruction_ticks - g_mz800.instruction_insideop_sync_ticks;
            break;
    };


#ifdef MZ800EMU_CFG_CLK1M1_SLOW
    mz800_sync_ctc0_and_cmt ( ticks_to_sync );
#else
    g_gdg.total_elapsed.ticks += ticks_to_sync;
#endif

    g_mz800.instruction_insideop_sync_ticks = instruction_ticks;

    mz800_sync ( );

    if ( g_mz800.interrupt ) {
        SET_MZ800_EVENT ( EVENT_BREAK_MZ800_INTERRUPT, 0 );
#ifdef MZ800EMU_CFG_DEBUGGER_ENABLED
    } else if ( ( TEST_EMULATION_PAUSED ) || ( TEST_DEBUGGER_STEP_CALL ) ) {
#else
    } else if ( TEST_EMULATION_PAUSED ) {
#endif
        SET_MZ800_EVENT ( EVENT_BREAK_EMULATION_PAUSED, 0 );
    };
}


void mz800_sync_insideop_iorq ( void ) {
    mz800_sync_insideop ( INSIDEOP_IORQ );
}


void mz800_sync_insideop_mreq ( void ) {
    mz800_sync_insideop ( INSIDEOP_MREQ );
}


void mz800_sync_insideop_mreq_e00x ( void ) {
    mz800_sync_insideop ( INSIDEOP_MREQ_E00x );
}


void mz800_sync_insideop_mreq_mz700_vramctrl ( void ) {
    mz800_sync_insideop ( INSIDEOP_MREQ_MZ700_VRAMCTRL );
}


void mz800_sync_insideop_iorq_psg_write ( void ) {
    mz800_sync_insideop ( INSIDEOP_IORQ_PSG_WRITE );
}


static inline void mz800_do_emulation_paused ( void ) {

#ifdef MZ800EMU_CFG_DEBUGGER_ENABLED

    debugger_update_all ( );
    debugger_step_call ( 0 );

    iface_sdl_render_status_line ( );

    framebuffer_border_changed ( );
    if ( !DMD_TEST_MZ700 ) {
        framebuffer_MZ800_screen_changed ( );
    };
    unsigned screen_elapsed_ticks = g_gdg.total_elapsed.ticks;
    if ( g_debugger.screen_refresh_at_step ) {
        debugger_forced_screen_update ( );
    } else {
        iface_sdl_update_window_in_beam_interval ( g_gdg.screen_is_already_rendered_at_beam_pos, screen_elapsed_ticks );
    };
    g_gdg.screen_is_already_rendered_at_beam_pos = screen_elapsed_ticks;

    while ( TEST_EMULATION_PAUSED && ( !TEST_DEBUGGER_STEP_CALL ) ) {
        iface_sdl_pool_all_events ( );
        ui_iteration ( );

        if ( g_iface_sdl.redraw_full_screen_request ) {
            iface_sdl_update_window ( );
        };

        g_usleep ( 20 * 1000 );
    };

    if ( TEST_DEBUGGER_STEP_CALL ) {
        SET_MZ800_EVENT ( EVENT_BREAK_EMULATION_PAUSED, 0 );
    } else {
        mz800_set_proximate_hw_event ( );
    };

    cmt_update_output ( );

#else // MZ800_DEBUGGER neni povolen

    iface_sdl_render_status_line ( );
    iface_sdl_update_window_in_beam_interval ( g_gdg.screen_is_already_rendered_at_beam_pos, g_gdg.total_elapsed.ticks );


    while ( TEST_EMULATION_PAUSED ) {
        iface_sdl_pool_all_events ( );
        ui_iteration ( );

        if ( g_iface_sdl.redraw_full_screen_request ) {
            iface_sdl_update_window ( );
        };

        mz800_set_proximate_hw_event ( );
    };
#endif
}

/*******************************************************************************
 *
 * 
 * Obsluha preruseni
 * 
 * 
 ******************************************************************************/


/**
 * Kontrola preruseni od CTC2 - je maskovano z i8255 PC02
 */
static inline void mz800_interrupt_check_ctc2 ( void ) {
    /* k interruptu dojde jen pokud CTC2: 1 a PC02: 1 */
    if ( CTC8253_OUT ( 2 ) & g_pio8255.signal_pc02 ) {
        g_mz800.interrupt |= MZ800_INTERRUPT_CTC2;
    } else {
        g_mz800.interrupt &= ~MZ800_INTERRUPT_CTC2;
    };
}


/**
 * Kontrola preruseni od PIOZ80
 */
static inline void mz800_interrupt_check_pioz80 ( void ) {
    if ( g_pioz80.interrupt & PIOZ80_INTERRUPT_INT_BIT ) {
        g_mz800.interrupt |= MZ800_INTERRUPT_PIOZ80;
    } else {
        g_mz800.interrupt &= ~MZ800_INTERRUPT_PIOZ80;
    };
}


/**
 * Kontrola preruseni od FDC
 */
static inline void mz800_interrupt_check_fdc ( void ) {
    if ( fdc_get_interrupt_state ( ) ) {
        g_mz800.interrupt |= MZ800_INTERRUPT_FDC;
    } else {
        g_mz800.interrupt &= ~MZ800_INTERRUPT_FDC;
    };
}


/**
 * Provede kontrolu vsech zarizeni, ktere jsou schopny vygenerovat interrupt.
 * 
 * @param event_ticks
 */
void mz800_interrupt_manager ( void ) {
    mz800_interrupt_check_fdc ( );
    mz800_interrupt_check_ctc2 ( );
    mz800_interrupt_check_pioz80 ( );
}


/**
 * CPU preslo do rezimu, kdy je opet povoleno ruseni - zkontrolujeme, zda tu neceka nejaky interrupt
 * 
 * @param cpu
 * @param user_data
 */
void mz800_ei_cb ( Z80EX_CONTEXT *cpu, void *user_data ) {
    //printf("mz800_ei_cb()\n");
    if ( g_mz800.interrupt ) {
        SET_MZ800_EVENT ( EVENT_BREAK_MZ800_INTERRUPT, 0 );
    };
}


/**
 * Mezi instrukcnimi takty testujeme, zda na nas neceka nejaky interrupt.
 * Pokud ano, tak jej zkusime poslat do Z80.
 */
static inline void mz800_interrupt_service ( void ) {

    if ( !g_mz800.interrupt ) return;

    unsigned interrupt_tstates = z80ex_int ( g_mz800.cpu );

    if ( interrupt_tstates ) { /* interrupt byl prijat */

        // Pokud nejsme v IM2, tak musime informovat PIOZ80 o INT_ACK
        pioz80_interrupt_ack ( );

        unsigned innterrupt_ticks = interrupt_tstates * GDGCLK2CPU_DIVIDER;

#ifdef MZ800EMU_CFG_CLK1M1_SLOW
        mz800_sync_ctc0_and_cmt ( innterrupt_ticks );
#else
        g_gdg.total_elapsed.ticks += innterrupt_ticks;
#endif

        if ( g_gdg.total_elapsed.ticks >= g_mz800.event.ticks ) {
            mz800_sync ( );
        };

    } else { /* interrupt nebyl prijat */

        // Pokud je ruseni zakazano, tak se o obnoveni eventu postara EICB.
        // Jinak to zkusime v dalsim instrukcnim cyklu.
        if ( z80ex_nmi_possible ( g_mz800.cpu ) != 0 ) {
            /* OK, ted to nejde, protoze jsme uprostred instrukce */
            SET_MZ800_EVENT ( EVENT_BREAK_MZ800_INTERRUPT, 0 );
        };
    };
}


void mz800_main ( void ) {

    mz800_reset ( );

    SDL_AddTimer ( INTERRUPT_TIMER_MS, screens_counter_flush, NULL );

#if 0
    g_memory.map = MEMORY_MAP_FLAG_ROM_0000 | MEMORY_MAP_FLAG_ROM_E000;
    z80ex_set_reg ( g_mz800.cpu, regHL, 0x10f0 );
    cmthack_load_mzf_filename ( "./madonna2.mzf" );
    z80ex_set_reg ( g_mz800.cpu, regHL, 0x3000 );
    z80ex_set_reg ( g_mz800.cpu, regBC, 0x074f );
    cmthack_read_mzf_body ( );
    z80ex_set_reg ( g_mz800.cpu, regSP, 0x10f0 );
    z80ex_set_reg ( g_mz800.cpu, regPC, 0x3304 );
#endif

#if 0
    g_memory.map = MEMORY_MAP_FLAG_ROM_0000 | MEMORY_MAP_FLAG_ROM_E000;
    z80ex_set_reg ( g_mz800.cpu, regHL, 0x10f0 );
    cmthack_load_mzf_filename ( "./interkarate/interkarate_plus.mzf" );
    z80ex_set_reg ( g_mz800.cpu, regHL, 0x1200 );
    z80ex_set_reg ( g_mz800.cpu, regBC, 0x0382 );
    cmthack_read_mzf_body ( );
    z80ex_set_reg ( g_mz800.cpu, regSP, 0x10f0 );
    z80ex_set_reg ( g_mz800.cpu, regPC, 0x14f3 );

    g_cmt.mz_cmtspeed = CMTSPEED_1_1;
    cmt_open_file_by_extension ( "./interkarate/inter.scr$.mzf" );
    cmt_play ( );
#endif

    while ( 1 ) {

        g_mz800.instruction_addr = z80ex_get_reg ( g_mz800.cpu, regPC );

#if 0
        static int dbg = 0;
        if ( g_mz800.instruction_addr == 0x3304 ) {
            dbg = 1;
        }

        if ( dbg ) {
            char mnemonic [ 200 ];
            int t_states, t_states2;
            unsigned bytecode_length = z80ex_dasm ( mnemonic, 200 - 1, 0, &t_states, &t_states2, debugger_dasm_read_cb, g_mz800.instruction_addr, NULL );
            printf ( "0x%04x\t%s\n", g_mz800.instruction_addr, mnemonic );
        }
#endif

#ifdef MZ800EMU_CFG_DEBUGGER_ENABLED
        /*
         * 
         * Implementace zakladnich breakpointu
         * 
         */

        if ( g_breakpoints.bpmap [ g_mz800.instruction_addr ] != BREAKPOINT_TYPE_NONE ) {
            breakpoints_activate_event ( );

            /* jsme v pauze? */
            if ( TEST_EMULATION_PAUSED ) {
                mz800_do_emulation_paused ( );
            }
        };
#endif

        do {
            int tstates = z80ex_step ( g_mz800.cpu );
            g_mz800.instruction_tstates += tstates;
        } while ( z80ex_last_op_type ( g_mz800.cpu ) != 0 );


#ifdef MZ800EMU_CFG_CLK1M1_SLOW
        mz800_sync_ctc0_and_cmt ( ( ( g_mz800.instruction_tstates * GDGCLK2CPU_DIVIDER ) - g_mz800.instruction_insideop_sync_ticks ) );
#else
        g_gdg.total_elapsed.ticks += ( ( g_mz800.instruction_tstates * GDGCLK2CPU_DIVIDER ) - g_mz800.instruction_insideop_sync_ticks );
#endif

        g_mz800.instruction_tstates = 0;
        g_mz800.instruction_insideop_sync_ticks = 0;

        if ( g_gdg.total_elapsed.ticks >= g_mz800.event.ticks ) {

            mz800_sync ( );

            /* Ceka na nas nejaky interrupt? */
            mz800_interrupt_service ( );

            /* jsme v pauze? */
            if ( TEST_EMULATION_PAUSED ) {
                mz800_do_emulation_paused ( );
            };
        };

    };
}


/*
 * 
 * Sem si odskocime, pokud jsme zastavili emulator a potrebujeme vykreslit kompletni screeen.
 * 
 */
void mz800_flush_full_screen ( void ) {

    if ( g_gdg.screen_changes | g_gdg.border_changes ) {

        unsigned beam_row = g_gdg.beam_row;

        do {

            if ( g_gdg.screen_changes ) {
                if ( ( g_gdg.beam_row >= VIDEO_BEAM_CANVAS_FIRST_ROW ) && ( g_gdg.beam_row <= VIDEO_BEAM_CANVAS_LAST_ROW ) ) {
                    if ( !DMD_TEST_MZ700 ) {
                        framebuffer_MZ800_current_screen_row_fill ( VIDEO_CANVAS_WIDTH );
                    } else {
                        framebuffer_update_MZ700_current_screen_row ( );
                    };
                };
                g_gdg.screen_need_update_from = 0;
            };

            if ( g_gdg.border_changes ) {
                if ( g_gdg.beam_row <= VIDEO_BEAM_DISPLAY_LAST_ROW ) {
                    framebuffer_border_current_row_fill ( );
                    g_gdg.last_updated_border_pixel = 0;
                };
            };

            if ( g_gdg.beam_row < VIDEO_SCREEN_HEIGHT - 1 ) {
                g_gdg.beam_row++;
            } else {
                g_gdg.beam_row = 0;
            };

        } while ( g_gdg.beam_row != beam_row );

        if ( g_gdg.screen_changes ) {
            g_gdg.framebuffer_state |= FB_STATE_SCREEN_CHANGED;
        };

        if ( g_gdg.border_changes ) {
            g_gdg.framebuffer_state |= FB_STATE_BORDER_CHANGED;
        };
    };


    if ( g_gdg.framebuffer_state || g_iface_sdl.redraw_full_screen_request ) {
        iface_sdl_update_window ( );
        g_gdg.framebuffer_state = FB_STATE_NOT_CHANGED;
    };
    flag_make_picture_time = 0;
}


void mz800_switch_emulation_speed ( unsigned value ) {

    value &= 1;
    if ( g_mz800.use_max_emulation_speed == value ) return;

    g_mz800.use_max_emulation_speed = value;
    if ( g_mz800.use_max_emulation_speed ) {
        printf ( "Fast emulation speed.\n" );
    } else {
        printf ( "Normal emulation speed.\n" );
    };

    iface_sdl_audio_update_buffer_state ( );

    ui_main_update_cpu_speed_menu ( g_mz800.use_max_emulation_speed );
}


void mz800_pause_emulation ( unsigned value ) {

    value &= 1;
    if ( value == g_mz800.emulation_paused ) return;

    if ( value ) {
        SET_MZ800_EVENT ( EVENT_BREAK_EMULATION_PAUSED, 0 );
    };

    g_mz800.emulation_paused = value;
    iface_sdl_audio_pause_emulation ( value );
    ui_main_update_emulation_state ( g_mz800.emulation_paused );

#ifdef MZ800EMU_CFG_DEBUGGER_ENABLED
    if ( TEST_DEBUGGER_ACTIVE ) {
        if ( value ) {
            // zastavili jsme
            ui_debugger_hide_spinner_window ( );
            breakpoints_reset_temporary_event ( );
        } else {
            ui_debugger_show_spinner_window ( );
            iface_sdl_set_main_window_focus ( );
        };
    };
    if ( !value ) {
        g_debugger.run_to_temporary_breakpoint = 0;
    };
#endif
}

#ifdef MZ800EMU_CFG_DEBUGGER_ENABLED


/**
 *  Pri Run to Cursor a Step Over nechceme, aby se nam prepinal focus na main window
 */
void mz800_run_to_temporary_breakpoint ( void ) {

    g_mz800.emulation_paused = 0;
    iface_sdl_audio_pause_emulation ( 0 );
    ui_main_update_emulation_state ( g_mz800.emulation_paused );

    // zkusime to bez spinner window
    g_debugger.run_to_temporary_breakpoint = 1;
#if 0
    if ( TEST_DEBUGGER_ACTIVE ) { // tady je to zrejme zbytecna podminka
        ui_debugger_show_spinner_window ( );
    };
#endif
}

#endif


void mz800_rear_dip_switch_mz800_mode ( unsigned value ) {

    value &= 1;

    if ( value == g_mz800.mz800_switch ) return;

    g_mz800.mz800_switch = value;
    ui_main_update_rear_dip_switch_mz800_mode ( g_mz800.mz800_switch );
}


void mz800_hwcompatibility_mz700_pal_timing ( unsigned value ) {

    value &= 1;

    if ( value == g_mz800.hwcompatibility_mz700_pal_timing ) return;

    g_mz800.hwcompatibility_mz700_pal_timing = value;
    ui_main_update_hwcompatibility_mz700_pal_timing ( g_mz800.hwcompatibility_mz700_pal_timing );
}


void mz800_hwcompatibility_mz700_fixed_e008 ( unsigned value ) {

    value &= 1;

    if ( value == g_mz800.hwcompatibility_mz700_fixed_e008 ) return;

    g_mz800.hwcompatibility_mz700_fixed_e008 = value;
    ui_main_update_hwcompatibility_mz700_fixed_e008 ( g_mz800.hwcompatibility_mz700_fixed_e008 );
}


/*******************************************************************************
 *
 * Inicializace MZ-800
 * 
 *******************************************************************************/

void mz800_propagatecfg_mz800_switch ( void *e, void *data ) {
    ui_main_update_rear_dip_switch_mz800_mode ( cfgelement_get_bool_value ( (CFGELM *) e ) );
}


void mz800_propagatecfg_mz700_pal_timing ( void *e, void *data ) {
    ui_main_update_hwcompatibility_mz700_pal_timing ( cfgelement_get_bool_value ( (CFGELM *) e ) );
}


void mz800_propagatecfg_mz700_fixed_e008 ( void *e, void *data ) {
    ui_main_update_hwcompatibility_mz700_fixed_e008 ( cfgelement_get_bool_value ( (CFGELM *) e ) );
}


void mz800_init ( void ) {

    CFGMOD *cmod = cfgroot_register_new_module ( g_cfgmain, "MZ800" );
    CFGELM *elm;

    elm = cfgmodule_register_new_element ( cmod, "development_mode", CFGENTYPE_KEYWORD, DEVELMODE_NO,
                                           DEVELMODE_NO, "NO",
                                           DEVELMODE_YES, "YES",
                                           -1 );

    cfgelement_set_handlers ( elm, (void*) &g_mz800.development_mode, (void*) &g_mz800.development_mode );


    elm = cfgmodule_register_new_element ( cmod, "mz800_switch", CFGENTYPE_BOOL, MZ800SWITCH_OFF );

    cfgelement_set_propagate_cb ( elm, mz800_propagatecfg_mz800_switch, NULL );
    cfgelement_set_handlers ( elm, (void*) &g_mz800.mz800_switch, (void*) &g_mz800.mz800_switch );

    elm = cfgmodule_register_new_element ( cmod, "hw_compatibility_mz700_pal_timing", CFGENTYPE_BOOL, HWCOMPAT_MZ700_PAL_TIMING_NO );

    cfgelement_set_propagate_cb ( elm, mz800_propagatecfg_mz700_pal_timing, NULL );
    cfgelement_set_handlers ( elm, (void*) &g_mz800.hwcompatibility_mz700_pal_timing, (void*) &g_mz800.hwcompatibility_mz700_pal_timing );

    elm = cfgmodule_register_new_element ( cmod, "hw_compatibility_mz700_fixed_e008", CFGENTYPE_BOOL, HWCOMPAT_MZ700_FIXED_E008_NO );

    cfgelement_set_propagate_cb ( elm, mz800_propagatecfg_mz700_fixed_e008, NULL );
    cfgelement_set_handlers ( elm, (void*) &g_mz800.hwcompatibility_mz700_fixed_e008, (void*) &g_mz800.hwcompatibility_mz700_fixed_e008 );

    cfgmodule_parse ( cmod );
    cfgmodule_propagate ( cmod );

    mz800_switch_emulation_speed ( MZ800_EMULATION_SPEED_NORMAL );
    //mz800_set_cpu_speed ( MZ800_EMULATION_SPEED_MAX );

    g_mz800.emulation_paused = 0;

    mz800_pause_emulation ( 0 );

    g_mz800.cpu = z80ex_create (
                                 memory_read_cb, NULL,
                                 memory_write_cb, NULL,
                                 port_read_cb, NULL,
                                 port_write_cb, NULL,
                                 pioz80_interrupt_ack_im2_cb, NULL
                                 );

    /*
          z80ex_set_tstate_callback ( g_mz800.cpu, mz800_tstate_cb, NULL );
     */

    z80ex_set_reti_callback ( g_mz800.cpu, pioz80_interrupt_reti_cb, NULL );

    z80ex_set_ei_callback ( g_mz800.cpu, mz800_ei_cb, NULL );

    g_mz800.regDBUS_latch = 0;
    g_mz800.status_changed = 0;
    g_mz800.interrupt = 0;

    g_mz800.instruction_addr = 0x0000;
    g_mz800.instruction_tstates = 0;
    g_mz800.instruction_insideop_sync_ticks = 0;

    mz800_cursor_timer_reset ( );

#ifdef MZ800EMU_CFG_VARIABLE_SPEED
    g_mz800.speed_in_percentage = 100;
    g_mz800.speed_frame_width = VIDEO_SCREEN_TICKS * (float) g_mz800.speed_in_percentage / 100;
    g_mz800.speed_sync_event.event_name = EVENT_SPEED_SYNC;
    g_mz800.speed_sync_event.ticks = g_mz800.speed_frame_width;
#endif

    gdg_init ( ); // GDG by se mel inicializovat uplne jako prvni
    memory_init ( );
    ctc8253_init ( ); // CTC by se mel inicializovat drive, nez PIO-Z80
    pio8255_init ( );
    pioz80_init ( );

    psg_init ( );
    audio_init ( );

    fdc_init ( );
    ramdisk_init ( );
    cmt_init ( );
    qdisk_init ( );
    joy_init ( );
    unicard_init ( );
    ide8_init ( );

#ifdef MZ800EMU_CFG_DEBUGGER_ENABLED
    debugger_init ( );
#endif

    mz800_set_proximate_hw_event ( );

    printf ( "\nRear dip switch - " );
    printf ( "Mode: %s, ", ( !g_mz800.mz800_switch ) ? "MZ-700" : "MZ-800" );
    printf ( "CMT polarity: %s\n", ( !g_cmt.polarity ) ? "Normal" : "Inverted" );

    if ( g_mz800.hwcompatibility_mz700_pal_timing == HWCOMPAT_MZ700_PAL_TIMING_YES ) {
        printf ( "HW compatibility - VRAM timing: %s\n", ( g_mz800.hwcompatibility_mz700_pal_timing == HWCOMPAT_MZ700_PAL_TIMING_YES ) ? "MZ-700 (PAL)" : "MZ-800" );
    };
    
    if ( g_mz800.hwcompatibility_mz700_fixed_e008 == HWCOMPAT_MZ700_FIXED_E008_YES ) {
        printf ( "HW compatibility - MZ-700 regDMD at 0xE008 HW bugfix %s\n", ( g_mz800.hwcompatibility_mz700_fixed_e008 == HWCOMPAT_MZ700_FIXED_E008_YES ) ? "ENABLED" : "DISABLED" );
    }
}
