/* 
 * File:   breakpoints.h
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 30. září 2015, 10:17
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

#ifndef BREAKPOINTS_H
#define BREAKPOINTS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "z80ex/include/z80ex.h"


    typedef enum en_BREAKPOINT_TYPE {
        BREAKPOINT_TYPE_NONE = -1, // na adrese neni zadny breakpoint
        BREAKPOINT_TYPE_TEMPORARY = -2, // na adrese se nachazi docasny breakpoint (run to cursor)
    } en_BREAKPOINT_TYPE;


    typedef struct st_BREAPOINTS {
        int temporary_bpt_addr; // hodnota 0x0000 - 0xffff odkazuje na adresu docasneho breakpointu (smi byt jen jeden)
        int bpmap [ 0x10000 ]; // hodnota >= 0 odkazuje na ID nastaveneho breakpointu
    } st_BREAPOINTS;

    extern st_BREAPOINTS g_breakpoints;

    extern void breakpoints_init ( void );
    extern void breakpoints_clear_all ( void );
    extern int breakpoints_event_add ( Z80EX_WORD addr, int id );
    extern int breakpoints_event_clear ( Z80EX_WORD addr, int id );
    extern void breakpoints_event_clear_addr ( Z80EX_WORD addr );
    extern void breakpoints_event_clear_id ( int id );
    extern int breakpoints_event_get_addr_by_id ( int id );
    extern int breakpoints_event_get_id_by_addr ( Z80EX_WORD addr );
    extern void breakpoints_reset_temporary_event ( void );
    extern void breakpoints_set_temporary_event ( Z80EX_WORD addr );
    extern void breakpoints_activate_event ( void );
#ifdef __cplusplus
}
#endif

#endif /* BREAKPOINTS_H */

