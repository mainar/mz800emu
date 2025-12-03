/* 
 * File:   ui_vkbd_windows.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 7. prosince 2017, 10:02
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

#include <stdio.h>

#ifdef WINDOWS

/*
#include <winuser.h>
 */

#include "ui_vkbd_windows.h"

#include "windows_virtual_key_codes.h"


static const struct {
    guint16 keysym;
    en_VKBD_SCANCODE scancode;
} KeySymToVKBDScancode[] = {
    { VK_SPACE, VKBD_SCANCODE_SPACE },
    { VK_OEM_3, VKBD_SCANCODE_GRAVE },
    { VK_ESCAPE, VKBD_SCANCODE_ESCAPE },
    { VK_TAB, VKBD_SCANCODE_TAB },
    { VK_CAPITAL, VKBD_SCANCODE_CAPSLOCK },
    { VK_SHIFT, VKBD_SCANCODE_LSHIFT },
    { VK_CONTROL, VKBD_SCANCODE_LCTRL },
    { VK_OEM_5, VKBD_SCANCODE_BACKSLASH },
    { VK_BACK, VKBD_SCANCODE_BACKSPACE },
    { VK_RETURN, VKBD_SCANCODE_RETURN },
    { VK_RSHIFT, VKBD_SCANCODE_RSHIFT },
    { VK_INSERT, VKBD_SCANCODE_INSERT },
    { VK_DELETE, VKBD_SCANCODE_DELETE },
    { VK_END, VKBD_SCANCODE_END },
    { VK_LEFT, VKBD_SCANCODE_LEFT },
    { VK_UP, VKBD_SCANCODE_UP },
    { VK_RIGHT, VKBD_SCANCODE_RIGHT },
    { VK_DOWN, VKBD_SCANCODE_DOWN },
    { VK_OEM_COMMA, VKBD_SCANCODE_COMMA },
    { VK_OEM_PERIOD, VKBD_SCANCODE_PERIOD },
    { VK_OEM_2, VKBD_SCANCODE_SLASH },
    { VK_OEM_MINUS, VKBD_SCANCODE_MINUS },
    { VK_OEM_PLUS, VKBD_SCANCODE_EQUALS },
    { VK_OEM_4, VKBD_SCANCODE_LEFTBRACKET },
    { VK_OEM_6, VKBD_SCANCODE_RIGHTBRACKET },
    { VK_OEM_1, VKBD_SCANCODE_SEMICOLON },
    { VK_OEM_7, VKBD_SCANCODE_APOSTROPHE },

};


en_VKBD_SCANCODE ui_vkbd_windows_KeycodeToScancode ( guint16 hardware_keycode ) {

    if ( hardware_keycode == 0 ) {
        return VKBD_SCANCODE_UNKNOWN;
    };

    if ( hardware_keycode >= VK_KEY_A && hardware_keycode <= VK_KEY_Z ) {
        return VKBD_SCANCODE_A + ( hardware_keycode - VK_KEY_A );
    };

    if ( hardware_keycode == VK_KEY_0 ) {
        return VKBD_SCANCODE_0;
    };

    if ( hardware_keycode >= VK_KEY_1 && hardware_keycode <= VK_KEY_9 ) {
        return VKBD_SCANCODE_1 + ( hardware_keycode - VK_KEY_1 );
    };

    if ( hardware_keycode >= VK_F1 && hardware_keycode <= VK_F12 ) {
        return VKBD_SCANCODE_F1 + ( hardware_keycode - VK_F1 );
    };

    int i;
    for ( i = 0; i < ( sizeof ( KeySymToVKBDScancode ) / sizeof ( KeySymToVKBDScancode[0] ) ); ++i ) {
        if ( hardware_keycode == KeySymToVKBDScancode[i].keysym ) {
            return KeySymToVKBDScancode[i].scancode;
        };
    };

    //printf ( "Unknown win hw keycode: 0x%04x\n", hardware_keycode );
    return VKBD_SCANCODE_UNKNOWN;
}

#endif
