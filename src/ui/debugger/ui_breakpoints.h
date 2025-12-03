/* 
 * File:   ui_breakpoints.h
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 1. října 2015, 11:42
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

#ifndef UI_BREAKPOINTS_H
#define	UI_BREAKPOINTS_H

#ifdef	__cplusplus
extern "C" {
#endif

#define BREAKPOINTS_INI_FILENAME    "breakpoints.ini"

    extern void ui_breakpoints_init ( void );
    extern void ui_breakpoints_show_window ( void );
    extern void ui_breakpoints_show_hide_window ( void );
    extern int ui_breakpoints_simple_add_event ( unsigned addr );
    extern void ui_breakpoints_select_id ( int id );
    extern void ui_breakpoints_save ( void );

#ifdef	__cplusplus
}
#endif

#endif	/* UI_BREAKPOINTS_H */

