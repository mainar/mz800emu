/* 
 * File:   breakpoints.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 30. září 2015, 10:15
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
#include <string.h>

#ifdef MZ800EMU_CFG_DEBUGGER_ENABLED

#include "breakpoints.h"
#include "ui/debugger/ui_breakpoints.h"
#include "mz800.h"
#include "debugger.h"

st_BREAPOINTS g_breakpoints;


void breakpoints_init ( void ) {
    breakpoints_clear_all ( );
    ui_breakpoints_init ( );
}


void breakpoints_clear_all ( void ) {
    memset ( g_breakpoints.bpmap, BREAKPOINT_TYPE_NONE, sizeof ( g_breakpoints.bpmap ) );
    g_breakpoints.temporary_bpt_addr = -1;
}


void breakpoints_reset_temporary_event ( void ) {
    if ( g_breakpoints.temporary_bpt_addr == -1 ) return;
    printf ( "INFO - remove temporary breakpoint on addr: 0x%04x\n", g_breakpoints.temporary_bpt_addr );
    if ( g_breakpoints.bpmap [ g_breakpoints.temporary_bpt_addr ] == BREAKPOINT_TYPE_TEMPORARY ) g_breakpoints.bpmap [ g_breakpoints.temporary_bpt_addr ] = BREAKPOINT_TYPE_NONE;
    g_breakpoints.temporary_bpt_addr = -1;
}


void breakpoints_set_temporary_event ( Z80EX_WORD addr ) {
    if ( g_breakpoints.temporary_bpt_addr != -1 ) breakpoints_reset_temporary_event ( );
    g_breakpoints.temporary_bpt_addr = addr;
    printf ( "INFO - add temporary breakpoint on addr: 0x%04x\n", g_breakpoints.temporary_bpt_addr );
    if ( g_breakpoints.bpmap [ g_breakpoints.temporary_bpt_addr ] == BREAKPOINT_TYPE_NONE ) g_breakpoints.bpmap [ g_breakpoints.temporary_bpt_addr ] = BREAKPOINT_TYPE_TEMPORARY;
}


int breakpoints_event_add ( Z80EX_WORD addr, int id ) {
    if ( id <= BREAKPOINT_TYPE_NONE ) return 0;
    if ( g_breakpoints.bpmap [ addr ] > BREAKPOINT_TYPE_NONE ) return g_breakpoints.bpmap [ addr ];
    g_breakpoints.bpmap [ addr ] = id;
    return 0;
}


int breakpoints_event_clear ( Z80EX_WORD addr, int id ) {
    if ( id <= BREAKPOINT_TYPE_NONE ) return 0;
    if ( g_breakpoints.bpmap [ addr ] != id ) return 0;
    g_breakpoints.bpmap [ addr ] = BREAKPOINT_TYPE_NONE;
    if ( addr == g_breakpoints.temporary_bpt_addr ) g_breakpoints.bpmap [ addr ] = BREAKPOINT_TYPE_TEMPORARY;
    return 1;
}


void breakpoints_event_clear_addr ( Z80EX_WORD addr ) {
    if ( g_breakpoints.bpmap [ addr ] > BREAKPOINT_TYPE_NONE ) g_breakpoints.bpmap [ addr ] = BREAKPOINT_TYPE_NONE;
    if ( addr == g_breakpoints.temporary_bpt_addr ) g_breakpoints.bpmap [ addr ] = BREAKPOINT_TYPE_TEMPORARY;
}


void breakpoints_event_clear_id ( int id ) {
    if ( id <= BREAKPOINT_TYPE_NONE ) return;
    unsigned addr;
    for ( addr = 0; addr <= 0xffff; addr++ ) {
        if ( g_breakpoints.bpmap [ addr ] == id ) {
            g_breakpoints.bpmap [ addr ] = BREAKPOINT_TYPE_NONE;
            if ( addr == g_breakpoints.temporary_bpt_addr ) g_breakpoints.bpmap [ addr ] = BREAKPOINT_TYPE_TEMPORARY;
            return;
        };
    };
}


int breakpoints_event_get_addr_by_id ( int id ) {
    if ( id <= BREAKPOINT_TYPE_NONE ) return -1;
    unsigned addr;
    for ( addr = 0; addr <= 0xffff; addr++ ) {
        if ( g_breakpoints.bpmap [ addr ] == id ) {
            return addr;
        };
    };
    return -1;
}


int breakpoints_event_get_id_by_addr ( Z80EX_WORD addr ) {
    return g_breakpoints.bpmap [ addr ];
}


void breakpoints_activate_event ( void ) {
    int breakpoint_id = g_breakpoints.bpmap [ g_mz800.instruction_addr ];
    if ( breakpoint_id == BREAKPOINT_TYPE_NONE ) return;
    if ( breakpoint_id > BREAKPOINT_TYPE_NONE ) {
        printf ( "INFO - activated breakpoint on addr: 0x%04x\n", g_mz800.instruction_addr );
        mz800_pause_emulation ( 1 );
        debugger_show_main_window ( );
        ui_breakpoints_show_window ( );
        ui_breakpoints_select_id ( breakpoint_id );
    };

    if ( !TEST_EMULATION_PAUSED ) {
        if ( g_mz800.instruction_addr == g_breakpoints.temporary_bpt_addr ) {
            printf ( "INFO - activated temporary breakpoint on addr: 0x%04x\n", g_mz800.instruction_addr );
            mz800_pause_emulation ( 1 );
            debugger_show_main_window ( );
        };
    };
}
#endif
