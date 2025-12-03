/* 
 * File:   framebuffer.h
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 5. července 2015, 8:43
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

#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#ifdef __cplusplus
extern "C" {
#endif


    extern void framebuffer_MZ800_current_screen_row_fill ( unsigned last_pixel );
    extern void framebuffer_MZ800_all_screen_rows_fill ( void );
    extern void framebuffer_update_MZ700_current_screen_row ( void );
    extern void framebuffer_update_MZ700_all_rows ( void );
    extern void framebuffer_MZ800_screen_changed ( void );

    extern void framebuffer_border_current_row_fill ( void );
    extern void framebuffer_border_all_rows_fill ( void );
    extern void framebuffer_border_changed ( void );

#ifdef __cplusplus
}
#endif

#endif /* FRAMEBUFFER_H */

