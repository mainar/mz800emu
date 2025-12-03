/* 
 * File:   inline_asm.h
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 16. září 2015, 8:46
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

#ifndef INLINE_ASM_H
#define	INLINE_ASM_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "z80ex/include/z80ex.h"

    /* asi nejdelsi instrukce: LD A,SET 7,(IY+%11111111) */
#define IASM_MNEMONICS_MAXLEN 25 /* +1 pro terminator 0x00 ! */
    
#define IASM_MAX_INS_BYTES 4
    
    typedef struct st_IASMBIN {
        unsigned length; /* pocet bajtu v instrukci */
        Z80EX_BYTE byte [ IASM_MAX_INS_BYTES ];
    } st_IASMBIN;
    
    extern unsigned debugger_iasm_assemble_line ( Z80EX_WORD addr, const char *assemble_txt, st_IASMBIN *compiled_output );


#ifdef	__cplusplus
}
#endif

#endif	/* INLINE_ASM_H */

