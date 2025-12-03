/* 
 * File:   hwscroll.h
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 18. června 2015, 19:45
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

#ifndef HWSCROLL_H
#define HWSCROLL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "z80ex/include/z80ex.h"


    typedef struct st_HWSCROLL {
        int regSSA;
        int regSEA;
        int regSW;
        int regSOF;
        int enabled;

    } st_HWSCROLL;

    extern st_HWSCROLL g_hwscroll;

    extern void hwscroll_init ( void );
    extern void hwscroll_reset ( void );
    extern void hwscroll_set_reg ( int addr, Z80EX_BYTE value );

    extern int hwscroll_get_ssa ( void );
    extern int hwscroll_get_sea ( void );
    extern int hwscroll_get_sw ( void );
    extern int hwscroll_get_sof ( void );

    extern void hwscroll_set_ssa ( int value );
    extern void hwscroll_set_sea ( int value );
    extern void hwscroll_set_sw ( int value );
    extern void hwscroll_set_sof ( int value );

#if 1
#define TEST_HWSCRL_ENABLED (g_hwscroll.enabled)
#define TEST_HWSCRL_ADDR_IN_SCRL_AREA(addr) ( ( addr >= g_hwscroll.regSSA ) && ( addr < g_hwscroll.regSEA ) )
#define TEST_HWSCRL_ADDR_IN_SCRL_AREA_UPDOWN(addr) ( addr >= ( g_hwscroll.regSEA - g_hwscroll.regSOF ) )
#define HWSCRL_SHIFT_UP(addr) ( addr + g_hwscroll.regSOF )
#define HWSCRL_SHIFT_DOWN(addr) ( addr + g_hwscroll.regSOF - g_hwscroll.regSW )
#define HWSCRL_SHIFT_UPDOWN(addr) ( ( TEST_HWSCRL_ADDR_IN_SCRL_AREA_UPDOWN(addr) ) ? HWSCRL_SHIFT_DOWN(addr) : HWSCRL_SHIFT_UP(addr) )

    //#define hwscroll_shift_addr_new(addr) ( ( TEST_HWSCRL_ENABLED && TEST_HWSCRL_ADDR_IN_SCRL_AREA(addr) ) ? HWSCRL_SHIFT_UPDOWN(addr) : addr )
#endif

#if 1


    static inline unsigned hwscroll_shift_addr ( unsigned addr ) {

        if ( TEST_HWSCRL_ENABLED ) {

            /* nachazime se v oblasti, ktera ma byt scrollovana? */
            if ( TEST_HWSCRL_ADDR_IN_SCRL_AREA ( addr ) ) {

                if ( TEST_HWSCRL_ADDR_IN_SCRL_AREA_UPDOWN ( addr ) ) {
                    return HWSCRL_SHIFT_DOWN ( addr );
                };
                return HWSCRL_SHIFT_UP ( addr );
            };
        };

        return addr;
    }
#endif

#if 0
    extern unsigned hwscroll_shift_addr ( unsigned addr );
#endif


#ifdef __cplusplus
}
#endif

#endif /* HWSCROLL_H */

