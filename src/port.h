/* 
 * File:   iorq.h
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 18. června 2015, 12:34
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

#ifndef PORT_H
#define	PORT_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "z80ex/include/z80ex.h"
    
    extern Z80EX_BYTE port_read_cb ( Z80EX_CONTEXT *cpu, Z80EX_WORD port, void *user_data );
    extern void port_write_cb ( Z80EX_CONTEXT *cpu, Z80EX_WORD port, Z80EX_BYTE value, void *user_data );


#ifdef	__cplusplus
}
#endif

#endif	/* PORT_H */

