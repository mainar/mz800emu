/* 
 * File:   ui_memload.h
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 12. června 2018, 12:37
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


#ifndef UI_MEMLOAD_H
#define UI_MEMLOAD_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef MZ800EMU_CFG_DEBUGGER_ENABLED

#include <stdint.h>

    typedef void (*ui_memload_cb )( uint32_t addr, uint8_t *data, uint32_t size, void *user_data );

    extern void ui_memload_select_file ( uint8_t *dst, ui_memload_cb cb, uint32_t dst_size, void *user_data );

#endif

#ifdef __cplusplus
}
#endif

#endif /* UI_MEMLOAD_H */

