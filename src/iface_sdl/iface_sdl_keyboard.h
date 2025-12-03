/* 
 * File:   iface_sdl_keyboard.h
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 21. června 2015, 22:46
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

#ifndef IFACE_SDL_KEYBOARD_H
#define IFACE_SDL_KEYBOARD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "iface_sdl.h"

    extern void iface_sdl_full_keyboard_scan ( void );
    extern void iface_sdl_keydown_event ( SDL_Event *event );
    extern void iface_sdl_keyup_event ( SDL_Event *event );
    extern void iface_sdl_pool_keyboard_events ( void );

#ifdef __cplusplus
}
#endif

#endif /* IFACE_SDL_KEYBOARD_H */

