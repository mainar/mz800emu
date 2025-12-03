/* 
 * File:   ui_vkbd_linux_x11.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 6. prosince 2017, 14:08
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

#ifdef LINUX

#include <X11/keysym.h>

#include "ui_vkbd_linux_x11.h"


/*
 * Preklad keycode na scancode podle SDL ./src/video/x11/SDL_x11keyboard.c
 * 
 */
static const struct {
    guint keysym;
    en_VKBD_SCANCODE scancode;
} KeySymToVKBDScancode[] = {
    { XK_Return, VKBD_SCANCODE_RETURN },
    { XK_Escape, VKBD_SCANCODE_ESCAPE },
    { XK_BackSpace, VKBD_SCANCODE_BACKSPACE },
    { XK_Tab, VKBD_SCANCODE_TAB },
    { XK_Caps_Lock, VKBD_SCANCODE_CAPSLOCK },
    { XK_F1, VKBD_SCANCODE_F1 },
    { XK_F2, VKBD_SCANCODE_F2 },
    { XK_F3, VKBD_SCANCODE_F3 },
    { XK_F4, VKBD_SCANCODE_F4 },
    { XK_F5, VKBD_SCANCODE_F5 },
    { XK_F6, VKBD_SCANCODE_F6 },
    { XK_F7, VKBD_SCANCODE_F7 },
    { XK_F8, VKBD_SCANCODE_F8 },
    { XK_F9, VKBD_SCANCODE_F9 },
    { XK_F10, VKBD_SCANCODE_F10 },
    { XK_F11, VKBD_SCANCODE_F11 },
    { XK_F12, VKBD_SCANCODE_F12 },
    { XK_Print, VKBD_SCANCODE_PRINTSCREEN },
    { XK_Scroll_Lock, VKBD_SCANCODE_SCROLLLOCK },
    { XK_Pause, VKBD_SCANCODE_PAUSE },
    { XK_Insert, VKBD_SCANCODE_INSERT },
    { XK_Home, VKBD_SCANCODE_HOME },
    { XK_Prior, VKBD_SCANCODE_PAGEUP },
    { XK_Delete, VKBD_SCANCODE_DELETE },
    { XK_End, VKBD_SCANCODE_END },
    { XK_Next, VKBD_SCANCODE_PAGEDOWN },
    { XK_Right, VKBD_SCANCODE_RIGHT },
    { XK_Left, VKBD_SCANCODE_LEFT },
    { XK_Down, VKBD_SCANCODE_DOWN },
    { XK_Up, VKBD_SCANCODE_UP },
    { XK_Num_Lock, VKBD_SCANCODE_NUMLOCKCLEAR },
    { XK_KP_Divide, VKBD_SCANCODE_KP_DIVIDE },
    { XK_KP_Multiply, VKBD_SCANCODE_KP_MULTIPLY },
    { XK_KP_Subtract, VKBD_SCANCODE_KP_MINUS },
    { XK_KP_Add, VKBD_SCANCODE_KP_PLUS },
    { XK_KP_Enter, VKBD_SCANCODE_KP_ENTER },
    { XK_KP_Delete, VKBD_SCANCODE_KP_PERIOD },
    { XK_KP_End, VKBD_SCANCODE_KP_1 },
    { XK_KP_Down, VKBD_SCANCODE_KP_2 },
    { XK_KP_Next, VKBD_SCANCODE_KP_3 },
    { XK_KP_Left, VKBD_SCANCODE_KP_4 },
    { XK_KP_Begin, VKBD_SCANCODE_KP_5 },
    { XK_KP_Right, VKBD_SCANCODE_KP_6 },
    { XK_KP_Home, VKBD_SCANCODE_KP_7 },
    { XK_KP_Up, VKBD_SCANCODE_KP_8 },
    { XK_KP_Prior, VKBD_SCANCODE_KP_9 },
    { XK_KP_Insert, VKBD_SCANCODE_KP_0 },
    { XK_KP_Decimal, VKBD_SCANCODE_KP_PERIOD },
    { XK_KP_1, VKBD_SCANCODE_KP_1 },
    { XK_KP_2, VKBD_SCANCODE_KP_2 },
    { XK_KP_3, VKBD_SCANCODE_KP_3 },
    { XK_KP_4, VKBD_SCANCODE_KP_4 },
    { XK_KP_5, VKBD_SCANCODE_KP_5 },
    { XK_KP_6, VKBD_SCANCODE_KP_6 },
    { XK_KP_7, VKBD_SCANCODE_KP_7 },
    { XK_KP_8, VKBD_SCANCODE_KP_8 },
    { XK_KP_9, VKBD_SCANCODE_KP_9 },
    { XK_KP_0, VKBD_SCANCODE_KP_0 },
    { XK_KP_Decimal, VKBD_SCANCODE_KP_PERIOD },
    { XK_Hyper_R, VKBD_SCANCODE_APPLICATION },
    { XK_KP_Equal, VKBD_SCANCODE_KP_EQUALS },
    { XK_F13, VKBD_SCANCODE_F13 },
    { XK_F14, VKBD_SCANCODE_F14 },
    { XK_F15, VKBD_SCANCODE_F15 },
    { XK_F16, VKBD_SCANCODE_F16 },
    { XK_F17, VKBD_SCANCODE_F17 },
    { XK_F18, VKBD_SCANCODE_F18 },
    { XK_F19, VKBD_SCANCODE_F19 },
    { XK_F20, VKBD_SCANCODE_F20 },
    { XK_F21, VKBD_SCANCODE_F21 },
    { XK_F22, VKBD_SCANCODE_F22 },
    { XK_F23, VKBD_SCANCODE_F23 },
    { XK_F24, VKBD_SCANCODE_F24 },
    { XK_Execute, VKBD_SCANCODE_EXECUTE },
    { XK_Help, VKBD_SCANCODE_HELP },
    { XK_Menu, VKBD_SCANCODE_MENU },
    { XK_Select, VKBD_SCANCODE_SELECT },
    { XK_Cancel, VKBD_SCANCODE_STOP },
    { XK_Redo, VKBD_SCANCODE_AGAIN },
    { XK_Undo, VKBD_SCANCODE_UNDO },
    { XK_Find, VKBD_SCANCODE_FIND },
    { XK_KP_Separator, VKBD_SCANCODE_KP_COMMA },
    { XK_Sys_Req, VKBD_SCANCODE_SYSREQ },
    { XK_Control_L, VKBD_SCANCODE_LCTRL },
    { XK_Shift_L, VKBD_SCANCODE_LSHIFT },
    { XK_Alt_L, VKBD_SCANCODE_LALT },
    { XK_Meta_L, VKBD_SCANCODE_LGUI },
    { XK_Super_L, VKBD_SCANCODE_LGUI },
    { XK_Control_R, VKBD_SCANCODE_RCTRL },
    { XK_Shift_R, VKBD_SCANCODE_RSHIFT },
    { XK_Alt_R, VKBD_SCANCODE_RALT },
    { XK_ISO_Level3_Shift, VKBD_SCANCODE_RALT },
    { XK_Meta_R, VKBD_SCANCODE_RGUI },
    { XK_Super_R, VKBD_SCANCODE_RGUI },
    { XK_Mode_switch, VKBD_SCANCODE_MODE },
    { XK_period, VKBD_SCANCODE_PERIOD },
    { XK_comma, VKBD_SCANCODE_COMMA },
    { XK_slash, VKBD_SCANCODE_SLASH },
    { XK_backslash, VKBD_SCANCODE_BACKSLASH },
    { XK_minus, VKBD_SCANCODE_MINUS },
    { XK_equal, VKBD_SCANCODE_EQUALS },
    { XK_space, VKBD_SCANCODE_SPACE },
    { XK_grave, VKBD_SCANCODE_GRAVE },
    { XK_apostrophe, VKBD_SCANCODE_APOSTROPHE },
    { XK_bracketleft, VKBD_SCANCODE_LEFTBRACKET },
    { XK_bracketright, VKBD_SCANCODE_RIGHTBRACKET },
    { XK_semicolon, VKBD_SCANCODE_SEMICOLON },
};


en_VKBD_SCANCODE ui_vkbd_linux_x11_KeysymToScancode ( guint keysym ) {
    int i;

    if ( keysym == 0 ) {
        return VKBD_SCANCODE_UNKNOWN;
    };

    if ( keysym >= XK_a && keysym <= XK_z ) {
        return VKBD_SCANCODE_A + ( keysym - XK_a );
    };

    if ( keysym >= XK_A && keysym <= XK_Z ) {
        return VKBD_SCANCODE_A + ( keysym - XK_A );
    };

    if ( keysym == XK_0 ) {
        return VKBD_SCANCODE_0;
    };

    if ( keysym >= XK_1 && keysym <= XK_9 ) {
        return VKBD_SCANCODE_1 + ( keysym - XK_1 );
    };

    for ( i = 0; i < ( sizeof ( KeySymToVKBDScancode ) / sizeof ( KeySymToVKBDScancode[0] ) ); ++i ) {
        if ( keysym == KeySymToVKBDScancode[i].keysym ) {
            return KeySymToVKBDScancode[i].scancode;
        };
    };

    return VKBD_SCANCODE_UNKNOWN;
}

#endif
