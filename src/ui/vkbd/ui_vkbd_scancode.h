/* 
 * File:   ui_vkbd_scancode.h
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 6. prosince 2017, 14:13
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


#ifndef UI_VKBD_SCANCODE_H
#define UI_VKBD_SCANCODE_H

#ifdef __cplusplus
extern "C" {
#endif


    /* Definice scankodu vychazi z SDL_scancode.h */

    typedef enum en_VKBD_SCANCODE {
        VKBD_SCANCODE_UNKNOWN = 0,

        /**
         *  \name Usage page 0x07
         *
         *  These values are from usage page 0x07 (USB keyboard page).
         */
        /* @{ */

        VKBD_SCANCODE_A = 4,
        VKBD_SCANCODE_B = 5,
        VKBD_SCANCODE_C = 6,
        VKBD_SCANCODE_D = 7,
        VKBD_SCANCODE_E = 8,
        VKBD_SCANCODE_F = 9,
        VKBD_SCANCODE_G = 10,
        VKBD_SCANCODE_H = 11,
        VKBD_SCANCODE_I = 12,
        VKBD_SCANCODE_J = 13,
        VKBD_SCANCODE_K = 14,
        VKBD_SCANCODE_L = 15,
        VKBD_SCANCODE_M = 16,
        VKBD_SCANCODE_N = 17,
        VKBD_SCANCODE_O = 18,
        VKBD_SCANCODE_P = 19,
        VKBD_SCANCODE_Q = 20,
        VKBD_SCANCODE_R = 21,
        VKBD_SCANCODE_S = 22,
        VKBD_SCANCODE_T = 23,
        VKBD_SCANCODE_U = 24,
        VKBD_SCANCODE_V = 25,
        VKBD_SCANCODE_W = 26,
        VKBD_SCANCODE_X = 27,
        VKBD_SCANCODE_Y = 28,
        VKBD_SCANCODE_Z = 29,

        VKBD_SCANCODE_1 = 30,
        VKBD_SCANCODE_2 = 31,
        VKBD_SCANCODE_3 = 32,
        VKBD_SCANCODE_4 = 33,
        VKBD_SCANCODE_5 = 34,
        VKBD_SCANCODE_6 = 35,
        VKBD_SCANCODE_7 = 36,
        VKBD_SCANCODE_8 = 37,
        VKBD_SCANCODE_9 = 38,
        VKBD_SCANCODE_0 = 39,

        VKBD_SCANCODE_RETURN = 40,
        VKBD_SCANCODE_ESCAPE = 41,
        VKBD_SCANCODE_BACKSPACE = 42,
        VKBD_SCANCODE_TAB = 43,
        VKBD_SCANCODE_SPACE = 44,

        VKBD_SCANCODE_MINUS = 45,
        VKBD_SCANCODE_EQUALS = 46,
        VKBD_SCANCODE_LEFTBRACKET = 47,
        VKBD_SCANCODE_RIGHTBRACKET = 48,
        VKBD_SCANCODE_BACKSLASH = 49, /**< Located at the lower left of the return
                                  *   key on ISO keyboards and at the right end
                                  *   of the QWERTY row on ANSI keyboards.
                                  *   Produces REVERSE SOLIDUS (backslash) and
                                  *   VERTICAL LINE in a US layout, REVERSE
                                  *   SOLIDUS and VERTICAL LINE in a UK Mac
                                  *   layout, NUMBER SIGN and TILDE in a UK
                                  *   Windows layout, DOLLAR SIGN and POUND SIGN
                                  *   in a Swiss German layout, NUMBER SIGN and
                                  *   APOSTROPHE in a German layout, GRAVE
                                  *   ACCENT and POUND SIGN in a French Mac
                                  *   layout, and ASTERISK and MICRO SIGN in a
                                  *   French Windows layout.
                                  */
        VKBD_SCANCODE_NONUSHASH = 50, /**< ISO USB keyboards actually use this code
                                  *   instead of 49 for the same key, but all
                                  *   OSes I've seen treat the two codes
                                  *   identically. So, as an implementor, unless
                                  *   your keyboard generates both of those
                                  *   codes and your OS treats them differently,
                                  *   you should generate VKBD_SCANCODE_BACKSLASH
                                  *   instead of this code. As a user, you
                                  *   should not rely on this code because SDL
                                  *   will never generate it with most (all?)
                                  *   keyboards.
                                  */
        VKBD_SCANCODE_SEMICOLON = 51,
        VKBD_SCANCODE_APOSTROPHE = 52,
        VKBD_SCANCODE_GRAVE = 53, /**< Located in the top left corner (on both ANSI
                              *   and ISO keyboards). Produces GRAVE ACCENT and
                              *   TILDE in a US Windows layout and in US and UK
                              *   Mac layouts on ANSI keyboards, GRAVE ACCENT
                              *   and NOT SIGN in a UK Windows layout, SECTION
                              *   SIGN and PLUS-MINUS SIGN in US and UK Mac
                              *   layouts on ISO keyboards, SECTION SIGN and
                              *   DEGREE SIGN in a Swiss German layout (Mac:
                              *   only on ISO keyboards), CIRCUMFLEX ACCENT and
                              *   DEGREE SIGN in a German layout (Mac: only on
                              *   ISO keyboards), SUPERSCRIPT TWO and TILDE in a
                              *   French Windows layout, COMMERCIAL AT and
                              *   NUMBER SIGN in a French Mac layout on ISO
                              *   keyboards, and LESS-THAN SIGN and GREATER-THAN
                              *   SIGN in a Swiss German, German, or French Mac
                              *   layout on ANSI keyboards.
                              */
        VKBD_SCANCODE_COMMA = 54,
        VKBD_SCANCODE_PERIOD = 55,
        VKBD_SCANCODE_SLASH = 56,

        VKBD_SCANCODE_CAPSLOCK = 57,

        VKBD_SCANCODE_F1 = 58,
        VKBD_SCANCODE_F2 = 59,
        VKBD_SCANCODE_F3 = 60,
        VKBD_SCANCODE_F4 = 61,
        VKBD_SCANCODE_F5 = 62,
        VKBD_SCANCODE_F6 = 63,
        VKBD_SCANCODE_F7 = 64,
        VKBD_SCANCODE_F8 = 65,
        VKBD_SCANCODE_F9 = 66,
        VKBD_SCANCODE_F10 = 67,
        VKBD_SCANCODE_F11 = 68,
        VKBD_SCANCODE_F12 = 69,

        VKBD_SCANCODE_PRINTSCREEN = 70,
        VKBD_SCANCODE_SCROLLLOCK = 71,
        VKBD_SCANCODE_PAUSE = 72,
        VKBD_SCANCODE_INSERT = 73, /**< insert on PC, help on some Mac keyboards (but
                                   does send code 73, not 117) */
        VKBD_SCANCODE_HOME = 74,
        VKBD_SCANCODE_PAGEUP = 75,
        VKBD_SCANCODE_DELETE = 76,
        VKBD_SCANCODE_END = 77,
        VKBD_SCANCODE_PAGEDOWN = 78,
        VKBD_SCANCODE_RIGHT = 79,
        VKBD_SCANCODE_LEFT = 80,
        VKBD_SCANCODE_DOWN = 81,
        VKBD_SCANCODE_UP = 82,

        VKBD_SCANCODE_NUMLOCKCLEAR = 83, /**< num lock on PC, clear on Mac keyboards
                                     */
        VKBD_SCANCODE_KP_DIVIDE = 84,
        VKBD_SCANCODE_KP_MULTIPLY = 85,
        VKBD_SCANCODE_KP_MINUS = 86,
        VKBD_SCANCODE_KP_PLUS = 87,
        VKBD_SCANCODE_KP_ENTER = 88,
        VKBD_SCANCODE_KP_1 = 89,
        VKBD_SCANCODE_KP_2 = 90,
        VKBD_SCANCODE_KP_3 = 91,
        VKBD_SCANCODE_KP_4 = 92,
        VKBD_SCANCODE_KP_5 = 93,
        VKBD_SCANCODE_KP_6 = 94,
        VKBD_SCANCODE_KP_7 = 95,
        VKBD_SCANCODE_KP_8 = 96,
        VKBD_SCANCODE_KP_9 = 97,
        VKBD_SCANCODE_KP_0 = 98,
        VKBD_SCANCODE_KP_PERIOD = 99,

        VKBD_SCANCODE_NONUSBACKSLASH = 100, /**< This is the additional key that ISO
                                        *   keyboards have over ANSI ones,
                                        *   located between left shift and Y.
                                        *   Produces GRAVE ACCENT and TILDE in a
                                        *   US or UK Mac layout, REVERSE SOLIDUS
                                        *   (backslash) and VERTICAL LINE in a
                                        *   US or UK Windows layout, and
                                        *   LESS-THAN SIGN and GREATER-THAN SIGN
                                        *   in a Swiss German, German, or French
                                        *   layout. */
        VKBD_SCANCODE_APPLICATION = 101, /**< windows contextual menu, compose */
        VKBD_SCANCODE_POWER = 102, /**< The USB document says this is a status flag,
                               *   not a physical key - but some Mac keyboards
                               *   do have a power key. */
        VKBD_SCANCODE_KP_EQUALS = 103,
        VKBD_SCANCODE_F13 = 104,
        VKBD_SCANCODE_F14 = 105,
        VKBD_SCANCODE_F15 = 106,
        VKBD_SCANCODE_F16 = 107,
        VKBD_SCANCODE_F17 = 108,
        VKBD_SCANCODE_F18 = 109,
        VKBD_SCANCODE_F19 = 110,
        VKBD_SCANCODE_F20 = 111,
        VKBD_SCANCODE_F21 = 112,
        VKBD_SCANCODE_F22 = 113,
        VKBD_SCANCODE_F23 = 114,
        VKBD_SCANCODE_F24 = 115,
        VKBD_SCANCODE_EXECUTE = 116,
        VKBD_SCANCODE_HELP = 117,
        VKBD_SCANCODE_MENU = 118,
        VKBD_SCANCODE_SELECT = 119,
        VKBD_SCANCODE_STOP = 120,
        VKBD_SCANCODE_AGAIN = 121, /**< redo */
        VKBD_SCANCODE_UNDO = 122,
        VKBD_SCANCODE_CUT = 123,
        VKBD_SCANCODE_COPY = 124,
        VKBD_SCANCODE_PASTE = 125,
        VKBD_SCANCODE_FIND = 126,
        VKBD_SCANCODE_MUTE = 127,
        VKBD_SCANCODE_VOLUMEUP = 128,
        VKBD_SCANCODE_VOLUMEDOWN = 129,
        /* not sure whether there's a reason to enable these */
        /*     VKBD_SCANCODE_LOCKINGCAPSLOCK = 130,  */
        /*     VKBD_SCANCODE_LOCKINGNUMLOCK = 131, */
        /*     VKBD_SCANCODE_LOCKINGSCROLLLOCK = 132, */
        VKBD_SCANCODE_KP_COMMA = 133,
        VKBD_SCANCODE_KP_EQUALSAS400 = 134,

        VKBD_SCANCODE_INTERNATIONAL1 = 135, /**< used on Asian keyboards, see
                                            footnotes in USB doc */
        VKBD_SCANCODE_INTERNATIONAL2 = 136,
        VKBD_SCANCODE_INTERNATIONAL3 = 137, /**< Yen */
        VKBD_SCANCODE_INTERNATIONAL4 = 138,
        VKBD_SCANCODE_INTERNATIONAL5 = 139,
        VKBD_SCANCODE_INTERNATIONAL6 = 140,
        VKBD_SCANCODE_INTERNATIONAL7 = 141,
        VKBD_SCANCODE_INTERNATIONAL8 = 142,
        VKBD_SCANCODE_INTERNATIONAL9 = 143,
        VKBD_SCANCODE_LANG1 = 144, /**< Hangul/English toggle */
        VKBD_SCANCODE_LANG2 = 145, /**< Hanja conversion */
        VKBD_SCANCODE_LANG3 = 146, /**< Katakana */
        VKBD_SCANCODE_LANG4 = 147, /**< Hiragana */
        VKBD_SCANCODE_LANG5 = 148, /**< Zenkaku/Hankaku */
        VKBD_SCANCODE_LANG6 = 149, /**< reserved */
        VKBD_SCANCODE_LANG7 = 150, /**< reserved */
        VKBD_SCANCODE_LANG8 = 151, /**< reserved */
        VKBD_SCANCODE_LANG9 = 152, /**< reserved */

        VKBD_SCANCODE_ALTERASE = 153, /**< Erase-Eaze */
        VKBD_SCANCODE_SYSREQ = 154,
        VKBD_SCANCODE_CANCEL = 155,
        VKBD_SCANCODE_CLEAR = 156,
        VKBD_SCANCODE_PRIOR = 157,
        VKBD_SCANCODE_RETURN2 = 158,
        VKBD_SCANCODE_SEPARATOR = 159,
        VKBD_SCANCODE_OUT = 160,
        VKBD_SCANCODE_OPER = 161,
        VKBD_SCANCODE_CLEARAGAIN = 162,
        VKBD_SCANCODE_CRSEL = 163,
        VKBD_SCANCODE_EXSEL = 164,

        VKBD_SCANCODE_KP_00 = 176,
        VKBD_SCANCODE_KP_000 = 177,
        VKBD_SCANCODE_THOUSANDSSEPARATOR = 178,
        VKBD_SCANCODE_DECIMALSEPARATOR = 179,
        VKBD_SCANCODE_CURRENCYUNIT = 180,
        VKBD_SCANCODE_CURRENCYSUBUNIT = 181,
        VKBD_SCANCODE_KP_LEFTPAREN = 182,
        VKBD_SCANCODE_KP_RIGHTPAREN = 183,
        VKBD_SCANCODE_KP_LEFTBRACE = 184,
        VKBD_SCANCODE_KP_RIGHTBRACE = 185,
        VKBD_SCANCODE_KP_TAB = 186,
        VKBD_SCANCODE_KP_BACKSPACE = 187,
        VKBD_SCANCODE_KP_A = 188,
        VKBD_SCANCODE_KP_B = 189,
        VKBD_SCANCODE_KP_C = 190,
        VKBD_SCANCODE_KP_D = 191,
        VKBD_SCANCODE_KP_E = 192,
        VKBD_SCANCODE_KP_F = 193,
        VKBD_SCANCODE_KP_XOR = 194,
        VKBD_SCANCODE_KP_POWER = 195,
        VKBD_SCANCODE_KP_PERCENT = 196,
        VKBD_SCANCODE_KP_LESS = 197,
        VKBD_SCANCODE_KP_GREATER = 198,
        VKBD_SCANCODE_KP_AMPERSAND = 199,
        VKBD_SCANCODE_KP_DBLAMPERSAND = 200,
        VKBD_SCANCODE_KP_VERTICALBAR = 201,
        VKBD_SCANCODE_KP_DBLVERTICALBAR = 202,
        VKBD_SCANCODE_KP_COLON = 203,
        VKBD_SCANCODE_KP_HASH = 204,
        VKBD_SCANCODE_KP_SPACE = 205,
        VKBD_SCANCODE_KP_AT = 206,
        VKBD_SCANCODE_KP_EXCLAM = 207,
        VKBD_SCANCODE_KP_MEMSTORE = 208,
        VKBD_SCANCODE_KP_MEMRECALL = 209,
        VKBD_SCANCODE_KP_MEMCLEAR = 210,
        VKBD_SCANCODE_KP_MEMADD = 211,
        VKBD_SCANCODE_KP_MEMSUBTRACT = 212,
        VKBD_SCANCODE_KP_MEMMULTIPLY = 213,
        VKBD_SCANCODE_KP_MEMDIVIDE = 214,
        VKBD_SCANCODE_KP_PLUSMINUS = 215,
        VKBD_SCANCODE_KP_CLEAR = 216,
        VKBD_SCANCODE_KP_CLEARENTRY = 217,
        VKBD_SCANCODE_KP_BINARY = 218,
        VKBD_SCANCODE_KP_OCTAL = 219,
        VKBD_SCANCODE_KP_DECIMAL = 220,
        VKBD_SCANCODE_KP_HEXADECIMAL = 221,

        VKBD_SCANCODE_LCTRL = 224,
        VKBD_SCANCODE_LSHIFT = 225,
        VKBD_SCANCODE_LALT = 226, /**< alt, option */
        VKBD_SCANCODE_LGUI = 227, /**< windows, command (apple), meta */
        VKBD_SCANCODE_RCTRL = 228,
        VKBD_SCANCODE_RSHIFT = 229,
        VKBD_SCANCODE_RALT = 230, /**< alt gr, option */
        VKBD_SCANCODE_RGUI = 231, /**< windows, command (apple), meta */

        VKBD_SCANCODE_MODE = 257, /**< I'm not sure if this is really not covered
                                 *   by any of the above, but since there's a
                                 *   special KMOD_MODE for it I'm adding it here
                                 */

        /* @} *//* Usage page 0x07 */

        /**
         *  \name Usage page 0x0C
         *
         *  These values are mapped from usage page 0x0C (USB consumer page).
         */
        /* @{ */

        VKBD_SCANCODE_AUDIONEXT = 258,
        VKBD_SCANCODE_AUDIOPREV = 259,
        VKBD_SCANCODE_AUDIOSTOP = 260,
        VKBD_SCANCODE_AUDIOPLAY = 261,
        VKBD_SCANCODE_AUDIOMUTE = 262,
        VKBD_SCANCODE_MEDIASELECT = 263,
        VKBD_SCANCODE_WWW = 264,
        VKBD_SCANCODE_MAIL = 265,
        VKBD_SCANCODE_CALCULATOR = 266,
        VKBD_SCANCODE_COMPUTER = 267,
        VKBD_SCANCODE_AC_SEARCH = 268,
        VKBD_SCANCODE_AC_HOME = 269,
        VKBD_SCANCODE_AC_BACK = 270,
        VKBD_SCANCODE_AC_FORWARD = 271,
        VKBD_SCANCODE_AC_STOP = 272,
        VKBD_SCANCODE_AC_REFRESH = 273,
        VKBD_SCANCODE_AC_BOOKMARKS = 274,

        /* @} *//* Usage page 0x0C */

        /**
         *  \name Walther keys
         *
         *  These are values that Christian Walther added (for mac keyboard?).
         */
        /* @{ */

        VKBD_SCANCODE_BRIGHTNESSDOWN = 275,
        VKBD_SCANCODE_BRIGHTNESSUP = 276,
        VKBD_SCANCODE_DISPLAYSWITCH = 277, /**< display mirroring/dual display
                                           switch, video mode switch */
        VKBD_SCANCODE_KBDILLUMTOGGLE = 278,
        VKBD_SCANCODE_KBDILLUMDOWN = 279,
        VKBD_SCANCODE_KBDILLUMUP = 280,
        VKBD_SCANCODE_EJECT = 281,
        VKBD_SCANCODE_SLEEP = 282,

        VKBD_SCANCODE_APP1 = 283,
        VKBD_SCANCODE_APP2 = 284,

        /* @} *//* Walther keys */

        /**
         *  \name Usage page 0x0C (additional media keys)
         *
         *  These values are mapped from usage page 0x0C (USB consumer page).
         */
        /* @{ */

        VKBD_SCANCODE_AUDIOREWIND = 285,
        VKBD_SCANCODE_AUDIOFASTFORWARD = 286,

        /* @} *//* Usage page 0x0C (additional media keys) */

        /* Add any other keys here. */

        VKBD_NUM_SCANCODES = 512 /**< not a key, just marks the number of scancodes
                                 for array bounds */
    } en_VKBD_SCANCODE;




#ifdef __cplusplus
}
#endif

#endif /* UI_VKBD_SCANCODE_H */

