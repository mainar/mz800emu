/* 
 * File:   ui_vkbd.h
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 24. února 2017, 14:08
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


#ifndef UI_VKBD_H
#define UI_VKBD_H

#ifdef __cplusplus
extern "C" {
#endif

    extern void ui_vkbd_show_hide ( void );
    extern void ui_vkbd_reset_keyboard_state ( void );

#ifdef __cplusplus
}
#endif

#endif /* UI_VKBD_H */

