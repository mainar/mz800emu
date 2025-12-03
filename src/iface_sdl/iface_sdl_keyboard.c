/* 
 * File:   iface_sdl_keyboard.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 21. června 2015, 22:45
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

#include "mz800emu_cfg.h"

#include "iface_sdl.h"
#include "iface_sdl_joy.h"

#include "ui/ui_main.h"
#include "ui/ui_cmt.h"
#include "ui/vkbd/ui_vkbd.h"
#include "fdc/fdc.h"

#include "pio8255/pio8255.h"
#include "joy/joy.h"

#include "mz800.h"

#ifdef MZ800EMU_CFG_DEBUGGER_ENABLED
#include "debugger/debugger.h"
#include "ui/debugger/ui_breakpoints.h"
#include "ui/debugger/ui_membrowser.h"
#include "ui/debugger/ui_dissassembler.h"
#endif

static int g_iface_alt_key = 0;


static inline void iface_sdl_keyboard_scan_col0 ( const Uint8 *state ) {
    /* BLANK GRAPH LIBRA ALPHA TAB ; : CR */

    if ( state [ SDL_SCANCODE_GRAVE ] ) {
        PIO8255_MZKEYBIT_RESET ( 0, 7 ); /* BLANK */
    };
    if ( state [ SDL_SCANCODE_CAPSLOCK ] ) {
        PIO8255_MZKEYBIT_RESET ( 0, 6 ); /* GRAPH */
    };
    if ( state [ SDL_SCANCODE_F9 ] ) {
        PIO8255_MZKEYBIT_RESET ( 0, 5 ); /* LIBRA */
    };
    if ( state [ SDL_SCANCODE_BACKSLASH ] ) {
        PIO8255_MZKEYBIT_RESET ( 0, 4 ); /* ALPHA */
    };
    if ( state [ SDL_SCANCODE_TAB ] ) {
        PIO8255_MZKEYBIT_RESET ( 0, 3 ); /* TAB */
    };
    if ( ( state [ SDL_SCANCODE_SEMICOLON ] ) || ( state [ SDL_SCANCODE_KP_PLUS ] ) ) {
        PIO8255_MZKEYBIT_RESET ( 0, 2 ); /* ; */
    };
    if ( ( state [ SDL_SCANCODE_APOSTROPHE ] ) || ( state [ SDL_SCANCODE_KP_MULTIPLY ] ) ) {
        PIO8255_MZKEYBIT_RESET ( 0, 1 ); /* : */
    };
    /* samotny RETURN udajne zlobi na nejakem notebooku s winXP pri aktivnim NumLock */
    if ( ( state [ SDL_SCANCODE_RETURN ] ) ||
         ( state [ SDL_SCANCODE_RETURN2 ] ) ||
         ( state [ SDL_SCANCODE_KP_ENTER ] ) ) {
        PIO8255_MZKEYBIT_RESET ( 0, 0 ); /* CR */
    };
}


static inline void iface_sdl_keyboard_scan_col1 ( const Uint8 *state ) {
    /* Y Z @ [ ] */

    if ( state [ SDL_SCANCODE_Y ] ) {
        PIO8255_MZKEYBIT_RESET ( 1, 7 ); /* Y */
    };
    if ( state [ SDL_SCANCODE_Z ] ) {
        PIO8255_MZKEYBIT_RESET ( 1, 6 ); /* Z */
    };
    if ( state [ SDL_SCANCODE_F6 ] ) {
        PIO8255_MZKEYBIT_RESET ( 1, 5 ); /* @ */
    };
    if ( state [ SDL_SCANCODE_LEFTBRACKET ] ) {
        PIO8255_MZKEYBIT_RESET ( 1, 4 ); /* [ */
    };
    if ( state [ SDL_SCANCODE_RIGHTBRACKET ] ) {
        PIO8255_MZKEYBIT_RESET ( 1, 3 ); /* ] */
    };
}


static inline void iface_sdl_keyboard_scan_col2 ( const Uint8 *state ) {
    /* Q R S T U V W X */

    if ( state [ SDL_SCANCODE_Q ] ) {
        PIO8255_MZKEYBIT_RESET ( 2, 7 ); /* Q */
    };
    if ( state [ SDL_SCANCODE_R ] ) {
        PIO8255_MZKEYBIT_RESET ( 2, 6 ); /* R */
    };
    if ( state [ SDL_SCANCODE_S ] ) {
        PIO8255_MZKEYBIT_RESET ( 2, 5 ); /* S */
    };
    if ( state [ SDL_SCANCODE_T ] ) {
        PIO8255_MZKEYBIT_RESET ( 2, 4 ); /* T */
    };
    if ( state [ SDL_SCANCODE_U ] ) {
        PIO8255_MZKEYBIT_RESET ( 2, 3 ); /* U */
    };
    if ( state [ SDL_SCANCODE_V ] ) {
        PIO8255_MZKEYBIT_RESET ( 2, 2 ); /* V */
    };
    if ( state [ SDL_SCANCODE_W ] ) {
        PIO8255_MZKEYBIT_RESET ( 2, 1 ); /* W */
    };
    if ( state [ SDL_SCANCODE_X ] ) {
        PIO8255_MZKEYBIT_RESET ( 2, 0 ); /* X */
    };
}


static inline void iface_sdl_keyboard_scan_col3 ( const Uint8 *state ) {
    /* I J K L M N O P */

    if ( state [ SDL_SCANCODE_I ] ) {
        PIO8255_MZKEYBIT_RESET ( 3, 7 ); /* I */
    };
    if ( state [ SDL_SCANCODE_J ] ) {
        PIO8255_MZKEYBIT_RESET ( 3, 6 ); /* J */
    };
    if ( state [ SDL_SCANCODE_K ] ) {
        PIO8255_MZKEYBIT_RESET ( 3, 5 ); /* K */
    };
    if ( state [ SDL_SCANCODE_L ] ) {
        PIO8255_MZKEYBIT_RESET ( 3, 4 ); /* L */
    };
    if ( state [ SDL_SCANCODE_M ] ) {
        PIO8255_MZKEYBIT_RESET ( 3, 3 ); /* M */
    };
    if ( state [ SDL_SCANCODE_N ] ) {
        PIO8255_MZKEYBIT_RESET ( 3, 2 ); /* N */
    };
    if ( state [ SDL_SCANCODE_O ] ) {
        PIO8255_MZKEYBIT_RESET ( 3, 1 ); /* O */
    };
    if ( state [ SDL_SCANCODE_P ] ) {
        PIO8255_MZKEYBIT_RESET ( 3, 0 ); /* P */
    };
}


static inline void iface_sdl_keyboard_scan_col4 ( const Uint8 *state ) {
    /* A B C D E F G H  */

    if ( state [ SDL_SCANCODE_A ] ) {
        PIO8255_MZKEYBIT_RESET ( 4, 7 ); /* A */
    };
    if ( state [ SDL_SCANCODE_B ] ) {
        PIO8255_MZKEYBIT_RESET ( 4, 6 ); /* B */
    };
    if ( state [ SDL_SCANCODE_C ] ) {
        PIO8255_MZKEYBIT_RESET ( 4, 5 ); /* C */
    };
    if ( state [ SDL_SCANCODE_D ] ) {
        PIO8255_MZKEYBIT_RESET ( 4, 4 ); /* D */
    };
    if ( state [ SDL_SCANCODE_E ] ) {
        PIO8255_MZKEYBIT_RESET ( 4, 3 ); /* E */
    };
    if ( state [ SDL_SCANCODE_F ] ) {
        PIO8255_MZKEYBIT_RESET ( 4, 2 ); /* F */
    };
    if ( state [ SDL_SCANCODE_G ] ) {
        PIO8255_MZKEYBIT_RESET ( 4, 1 ); /* G */
    };
    if ( state [ SDL_SCANCODE_H ] ) {
        PIO8255_MZKEYBIT_RESET ( 4, 0 ); /* H */
    };
}


static inline void iface_sdl_keyboard_scan_col5 ( const Uint8 *state, SDL_Keymod kmod ) {
    /* 1 2 3 4 5 6 7 8 */

    if ( ( state [ SDL_SCANCODE_1 ] ) || ( ( kmod & KMOD_NUM ) && ( state[SDL_SCANCODE_KP_1] ) ) ) {
        PIO8255_MZKEYBIT_RESET ( 5, 7 ); /* 1 */
    };
    if ( ( state [ SDL_SCANCODE_2 ] ) || ( ( kmod & KMOD_NUM ) && ( state[SDL_SCANCODE_KP_2] ) ) ) {
        PIO8255_MZKEYBIT_RESET ( 5, 6 ); /* 2 */
    };
    if ( ( state [ SDL_SCANCODE_3 ] ) || ( ( kmod & KMOD_NUM ) && ( state[SDL_SCANCODE_KP_3] ) ) ) {
        PIO8255_MZKEYBIT_RESET ( 5, 5 ); /* 3 */
    };
    if ( ( state [ SDL_SCANCODE_4 ] ) || ( ( kmod & KMOD_NUM ) && ( state[SDL_SCANCODE_KP_4] ) ) ) {
        PIO8255_MZKEYBIT_RESET ( 5, 4 ); /* 4 */
    };
    if ( ( state [ SDL_SCANCODE_5 ] ) || ( ( kmod & KMOD_NUM ) && ( state[SDL_SCANCODE_KP_5] ) ) ) {
        PIO8255_MZKEYBIT_RESET ( 5, 3 ); /* 5 */
    };
    if ( ( state [ SDL_SCANCODE_6 ] ) || ( ( kmod & KMOD_NUM ) && ( state[SDL_SCANCODE_KP_6] ) ) ) {
        PIO8255_MZKEYBIT_RESET ( 5, 2 ); /* 6 */
    };
    if ( ( state [ SDL_SCANCODE_7 ] ) || ( ( kmod & KMOD_NUM ) && ( state[SDL_SCANCODE_KP_7] ) ) ) {
        PIO8255_MZKEYBIT_RESET ( 5, 1 ); /* 7 */
    };
    if ( ( state [ SDL_SCANCODE_8 ] ) || ( ( kmod & KMOD_NUM ) && ( state[SDL_SCANCODE_KP_8] ) ) ) {
        PIO8255_MZKEYBIT_RESET ( 5, 0 ); /* 8 */
    };
}


static inline void iface_sdl_keyboard_scan_col6 ( const Uint8 *state, SDL_Keymod kmod ) {
    /* \ ~ - SPACE 0 9 , . */

    if ( state [ SDL_SCANCODE_F7 ] ) {
        PIO8255_MZKEYBIT_RESET ( 6, 7 ); /* \ */
    };
    if ( state [ SDL_SCANCODE_EQUALS ] ) {
        PIO8255_MZKEYBIT_RESET ( 6, 6 ); /* ~ */
    };
    if ( ( state [ SDL_SCANCODE_MINUS ] ) || ( state[SDL_SCANCODE_KP_MINUS] ) ) {
        PIO8255_MZKEYBIT_RESET ( 6, 5 ); /* - */
    };
    if ( state [ SDL_SCANCODE_SPACE ] ) {
        PIO8255_MZKEYBIT_RESET ( 6, 4 ); /* SPACE */
    };
    if ( ( state [ SDL_SCANCODE_0 ] ) || ( ( kmod & KMOD_NUM ) && ( state[SDL_SCANCODE_KP_0] ) ) ) {
        PIO8255_MZKEYBIT_RESET ( 6, 3 ); /* 0 */
    };
    if ( ( state [ SDL_SCANCODE_9 ] ) || ( ( kmod & KMOD_NUM ) && ( state[SDL_SCANCODE_KP_9] ) ) ) {
        PIO8255_MZKEYBIT_RESET ( 6, 2 ); /* 9 */
    };
    if ( state [ SDL_SCANCODE_COMMA ] ) {
        PIO8255_MZKEYBIT_RESET ( 6, 1 ); /* , */
    };
    if ( ( state [ SDL_SCANCODE_PERIOD ] ) || ( ( kmod & KMOD_NUM ) && ( state[SDL_SCANCODE_KP_PERIOD] ) ) ) {
        PIO8255_MZKEYBIT_RESET ( 6, 0 ); /* . */
    };
}


static inline void iface_sdl_keyboard_scan_col7 ( const Uint8 *state, SDL_Keymod kmod ) {
    /* INST DEL UP DOWN RIGHT LEFT ? / */

    if ( ( state [ SDL_SCANCODE_INSERT ] ) || ( ( !( kmod & KMOD_NUM ) ) && ( state[SDL_SCANCODE_KP_0] ) ) ) {
        PIO8255_MZKEYBIT_RESET ( 7, 7 ); /* INSERT */
    };
    if ( ( state [ SDL_SCANCODE_DELETE ] ) || ( ( !( kmod & KMOD_NUM ) ) && ( state[SDL_SCANCODE_KP_PERIOD] ) ) ) {
        PIO8255_MZKEYBIT_RESET ( 7, 6 ); /* DELETE */
    };
    if ( state [ SDL_SCANCODE_BACKSPACE ] ) {
        PIO8255_MZKEYBIT_RESET ( 7, 6 ); /* DELETE */
    };
    if ( ( state [ SDL_SCANCODE_UP ] ) || ( ( !( kmod & KMOD_NUM ) ) && ( state[SDL_SCANCODE_KP_8] ) ) ) {
        PIO8255_MZKEYBIT_RESET ( 7, 5 ); /* UP */
    };
    if ( ( state [ SDL_SCANCODE_DOWN ] ) || ( ( !( kmod & KMOD_NUM ) ) && ( state[SDL_SCANCODE_KP_2] ) ) ) {
        PIO8255_MZKEYBIT_RESET ( 7, 4 ); /* DOWN */
    };
    if ( ( state [ SDL_SCANCODE_RIGHT ] ) || ( ( !( kmod & KMOD_NUM ) ) && ( state[SDL_SCANCODE_KP_6] ) ) ) {
        PIO8255_MZKEYBIT_RESET ( 7, 3 ); /* RIGHT */
    };
    if ( ( state [ SDL_SCANCODE_LEFT ] ) || ( ( !( kmod & KMOD_NUM ) ) && ( state[SDL_SCANCODE_KP_4] ) ) ) {
        PIO8255_MZKEYBIT_RESET ( 7, 2 ); /* LEFT */
    };
    if ( state [ SDL_SCANCODE_F8 ] ) {
        PIO8255_MZKEYBIT_RESET ( 7, 1 ); /* ? */
    };
    if ( ( state [ SDL_SCANCODE_SLASH ] ) || ( state[SDL_SCANCODE_KP_DIVIDE] ) ) {
        PIO8255_MZKEYBIT_RESET ( 7, 0 ); /* / */
    };
}


static inline void iface_sdl_keyboard_scan_col8 ( const Uint8 *state ) {
    /* ESC CTRL SHIFT */

    if ( state [ SDL_SCANCODE_ESCAPE ] ) {
        PIO8255_MZKEYBIT_RESET ( 8, 7 ); /* ESC */
    };
    if ( state [ SDL_SCANCODE_END ] ) {
        PIO8255_MZKEYBIT_RESET ( 8, 7 ); /* END */
    };
    if ( state [ SDL_SCANCODE_LCTRL ] ) {
        PIO8255_MZKEYBIT_RESET ( 8, 6 ); /* CTRL */
    };
    if ( state [ SDL_SCANCODE_LSHIFT ] ) {
        PIO8255_MZKEYBIT_RESET ( 8, 0 ); /* SHIFT */
    };
    if ( state [ SDL_SCANCODE_RSHIFT ] ) {
        PIO8255_MZKEYBIT_RESET ( 8, 0 ); /* SHIFT */
    };
    if ( state [ SDL_SCANCODE_KP_PLUS ] ) {
        PIO8255_MZKEYBIT_RESET ( 8, 0 ); /* SHIFT + ; = PLUS */
    };
    if ( state [ SDL_SCANCODE_KP_MULTIPLY ] ) {
        PIO8255_MZKEYBIT_RESET ( 8, 0 ); /* SHIFT + : = * */
    };
}


static inline void iface_sdl_keyboard_scan_col9 ( const Uint8 *state ) {
    /* F1 F2 F3 F4 F5 */

    if ( state [ SDL_SCANCODE_F1 ] ) {
        PIO8255_MZKEYBIT_RESET ( 9, 7 ); /* F1 */
    };
    if ( state [ SDL_SCANCODE_F2 ] ) {
        PIO8255_MZKEYBIT_RESET ( 9, 6 ); /* F2 */
    };
    if ( state [ SDL_SCANCODE_F3 ] ) {
        PIO8255_MZKEYBIT_RESET ( 9, 5 ); /* F3 */
    };
    if ( state [ SDL_SCANCODE_F4 ] ) {
        PIO8255_MZKEYBIT_RESET ( 9, 4 ); /* F4 */
    };
    if ( state [ SDL_SCANCODE_F5 ] ) {
        PIO8255_MZKEYBIT_RESET ( 9, 3 ); /* F5 */
    };
}


static inline void iface_sdl_joy_num_keypad_scan ( const Uint8 *state, Z80EX_BYTE *joystate ) {
    if ( state [ SDL_SCANCODE_KP_8 ] ) {
        JOY_STATEBIT_RESET ( *joystate, JOY_STATEBIT_UP );
    };
    if ( state [ SDL_SCANCODE_KP_2 ] ) {
        JOY_STATEBIT_RESET ( *joystate, JOY_STATEBIT_DOWN );
    };
    if ( state [ SDL_SCANCODE_KP_4 ] ) {
        JOY_STATEBIT_RESET ( *joystate, JOY_STATEBIT_LEFT );
    };
    if ( state [ SDL_SCANCODE_KP_6 ] ) {
        JOY_STATEBIT_RESET ( *joystate, JOY_STATEBIT_RIGHT );
    };
    if ( state [ SDL_SCANCODE_KP_7 ] ) {
        JOY_STATEBIT_RESET ( *joystate, JOY_STATEBIT_UP );
        JOY_STATEBIT_RESET ( *joystate, JOY_STATEBIT_LEFT );
    };
    if ( state [ SDL_SCANCODE_KP_9 ] ) {
        JOY_STATEBIT_RESET ( *joystate, JOY_STATEBIT_UP );
        JOY_STATEBIT_RESET ( *joystate, JOY_STATEBIT_RIGHT );
    };
    if ( state [ SDL_SCANCODE_KP_1 ] ) {
        JOY_STATEBIT_RESET ( *joystate, JOY_STATEBIT_DOWN );
        JOY_STATEBIT_RESET ( *joystate, JOY_STATEBIT_LEFT );
    };
    if ( state [ SDL_SCANCODE_KP_3 ] ) {
        JOY_STATEBIT_RESET ( *joystate, JOY_STATEBIT_DOWN );
        JOY_STATEBIT_RESET ( *joystate, JOY_STATEBIT_RIGHT );
    };
    if ( state [ SDL_SCANCODE_KP_5 ] ) {
        JOY_STATEBIT_RESET ( *joystate, JOY_STATEBIT_TRIG1 );
    };
    if ( state [ SDL_SCANCODE_KP_0 ] ) {
        JOY_STATEBIT_RESET ( *joystate, JOY_STATEBIT_TRIG2 );
    };
}


void iface_sdl_full_keyboard_scan ( void ) {
    const Uint8 *state = SDL_GetKeyboardState ( NULL );
    SDL_Keymod kmod = SDL_GetModState ( );
    if ( state[SDL_SCANCODE_LALT] ) return;
    pio8255_keyboard_matrix_reset ( );
    iface_sdl_keyboard_scan_col0 ( state );
    iface_sdl_keyboard_scan_col1 ( state );
    iface_sdl_keyboard_scan_col2 ( state );
    iface_sdl_keyboard_scan_col3 ( state );
    iface_sdl_keyboard_scan_col4 ( state );
    iface_sdl_keyboard_scan_col5 ( state, kmod );
    iface_sdl_keyboard_scan_col6 ( state, kmod );
    iface_sdl_keyboard_scan_col7 ( state, kmod );
    iface_sdl_keyboard_scan_col8 ( state );
    iface_sdl_keyboard_scan_col9 ( state );

    int device;
    for ( device = 0; device < JOY_DEVID_COUNT; device++ ) {
        if ( g_joy.dev[device].type == JOY_TYPE_NUM_KEYPAD ) {
            joy_reset_dev_state ( device );
            iface_sdl_joy_num_keypad_scan ( state, &g_joy.dev[device].state );
            break;
        };
    };
}


static inline int iface_sdl_keydown_in_development_mode ( SDL_Event *event ) {
#if 0
    if ( event->key.keysym.scancode == SDL_SCANCODE_F10 ) {
        printf ( "F10 - INTERRUPT\n" );
        unsigned interrupt_ticks = z80ex_int ( g_mz800.cpu );
        if ( interrupt_ticks ) {
            /* interrupt byl prijat */
            printf ( "Interrupt received!\n" );
        } else {
            printf ( "Interrupt NOT received!\n" );
        }
        return 1;
    };
#endif
#if 0
    if ( event.key.keysym.scancode == SDL_SCANCODE_F11 ) {
        if ( g_mz800.debug_pc == 0 ) {
            printf ( "Turn ON debug PC\n" );
        } else {
            printf ( "Turn OFF debug PC\n" );
        };
        g_mz800.debug_pc = ~g_mz800.debug_pc & 1;
        return 1;
    };
#endif
    return 0;
}


static inline void iface_sdl_keydown_hotkeys ( SDL_Event *event ) {
    /*
     * 
     *  Obsluha klavesovych zkratek ALT+xx
     * 
     */
    if ( g_iface_alt_key ) {

        if ( event->key.keysym.scancode == SDL_SCANCODE_C ) {
            /*
             * Virtual CMT: Alt + C
             */
            ui_cmt_window_show_hide ( );

        } else if ( event->key.keysym.scancode == SDL_SCANCODE_M ) {
            /*
             * Max speed: Alt + M
             */
            mz800_switch_emulation_speed ( ( ~g_mz800.use_max_emulation_speed ) & 0x01 );

        } else if ( event->key.keysym.scancode == SDL_SCANCODE_P ) {
            /*
             * Pause emulation: Alt + P
             */
            mz800_pause_emulation ( ( ~g_mz800.emulation_paused ) & 0x01 );

        } else if ( event->key.keysym.scancode == SDL_SCANCODE_K ) {
            /*
             * Virtual keyboard: Alt + K
             */
            ui_vkbd_show_hide ( );

        } else if ( event->key.keysym.scancode == SDL_SCANCODE_W ) {
            /*
             * Virtual keyboard: Alt + W
             */
            iface_sdl_fix_window_aspect_ratio ( 'W' );

        } else if ( event->key.keysym.scancode == SDL_SCANCODE_H ) {
            /*
             * Virtual keyboard: Alt + H
             */
            iface_sdl_fix_window_aspect_ratio ( 'H' );
#if 0
        } else if ( event->key.keysym.scancode == SDL_SCANCODE_1 ) {
            /*
             * Mount FD0 DSK image: Alt + 1
             */
            fdc_mount ( 0 );

        } else if ( event->key.keysym.scancode == SDL_SCANCODE_2 ) {
            /*
             * Mount FD1 DSK image: Alt + 2
             */
            fdc_mount ( 1 );

        } else if ( event->key.keysym.scancode == SDL_SCANCODE_3 ) {
            /*
             * Mount FD2 DSK image: Alt + 3
             */
            fdc_mount ( 2 );

        } else if ( event->key.keysym.scancode == SDL_SCANCODE_4 ) {
            /*
             * Mount FD3 DSK image: Alt + 4
             */
            fdc_mount ( 3 );
#endif
#ifdef MZ800EMU_CFG_DEBUGGER_ENABLED                    
        } else if ( event->key.keysym.scancode == SDL_SCANCODE_D ) {
            /*
             * Debugger window: Alt + D
             */
            debugger_show_hide_main_window ( );

        } else if ( event->key.keysym.scancode == SDL_SCANCODE_B ) {
            /*
             * Breakpoints window: Alt + B
             */
            ui_breakpoints_show_hide_window ( );

        } else if ( event->key.keysym.scancode == SDL_SCANCODE_E ) {
            /*
             * Memory browser window: Alt + E
             */
            ui_membrowser_show_hide ( );

        } else if ( event->key.keysym.scancode == SDL_SCANCODE_I ) {
            /*
             * Dissassembler window: Alt + I
             */
            ui_dissassembler_show_window ( );
#endif
        };
    };
}


void iface_sdl_keydown_event ( SDL_Event *event ) {

    if ( event->key.keysym.scancode == SDL_SCANCODE_F12 ) {
        mz800_reset ( );
    } else if ( event->key.keysym.scancode == SDL_SCANCODE_F11 ) {
        iface_sdl_joy_get_calibration ( );
    } else if ( event->key.keysym.scancode == SDL_SCANCODE_LALT ) {
        g_iface_alt_key = 1;
    } else {
        if ( ( g_mz800.development_mode ) && ( iface_sdl_keydown_in_development_mode ( event ) ) ) {
            return;
        } else if ( !g_ui.disable_hotkeys ) {
            iface_sdl_keydown_hotkeys ( event );
        };
    };
}


void iface_sdl_keyup_event ( SDL_Event * event ) {
    if ( event->key.keysym.scancode == SDL_SCANCODE_LALT ) {
        g_iface_alt_key = 0;
    };
}


void iface_sdl_pool_keyboard_events ( void ) {

    SDL_PumpEvents ( );

    SDL_Event events [ 20 ];

    int keyboard_events = SDL_PeepEvents ( events, 20, SDL_GETEVENT, SDL_KEYDOWN, SDL_KEYUP );

    int i;
    for ( i = 0; i < keyboard_events; i++ ) {
        SDL_Event *event = &events [ i ];

        if ( event->type == SDL_KEYDOWN ) {
            iface_sdl_keydown_event ( event );
        } else {
            iface_sdl_keyup_event ( event );
        };
    };

    if ( keyboard_events != 0 ) {
        iface_sdl_full_keyboard_scan ( );
    }

}
