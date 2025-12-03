/* 
 * File:   pio8255.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 21. června 2015, 23:13
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
#include <string.h>
#include <glib.h>

#include "z80ex/include/z80ex.h"

#include "pio8255.h"
#include "iface_sdl/iface_sdl_keyboard.h"
#include "mz800.h"
#include "gdg/gdg.h"
#include "audio.h"
#include "ctc8253/ctc8253.h"
#include "cmt/cmt.h"
#include "cmt/cmthack.h"

//#define DBGLEVEL (DBGNON /* | DBGERR | DBGWAR | DBGINF*/)
//#define DBGLEVEL (DBGNON | DBGERR | DBGWAR | DBGINF )
#include "debug.h"
#include "ui/vkbd/windows_virtual_key_codes.h"
#include "ui/vkbd/ui_vkbd_autotype.h"


/*
 * Emulace PIO8255
 *
 *
 */


#define DEF_PIO8255_PORTA 0x00
#define DEF_PIO8255_PORTB 0x01
#define DEF_PIO8255_PORTC 0x02
#define DEF_PIO8255_MASTER 0x03


st_PIO8255 g_pio8255;


void pio8255_keyboard_matrix_reset ( void ) {
    memset ( &g_pio8255.keyboard_matrix, 0xff, sizeof ( g_pio8255.keyboard_matrix ) );
}


void pio8255_vkbd_matrix_reset ( void ) {
    memset ( &g_pio8255.vkbd_matrix, 0xff, sizeof ( g_pio8255.vkbd_matrix ) );
}


void pio8255_autotype_set_key_down_ms ( double ms ) {
    g_pio8255.vkbd_autotype_kd_ms = ms;
    g_pio8255.vkbd_autotype_kd_ticks = ms * ( GDGCLK_BASE / 1000 );
}


void pio8255_autotype_set_key_up_ms ( double ms ) {
    g_pio8255.vkbd_autotype_ku_ms = ms;
    g_pio8255.vkbd_autotype_ku_ticks = ms * ( GDGCLK_BASE / 1000 );
}


void pio8255_set_autotype ( char *txt ) {
    g_pio8255.vkbd_autotype = txt;
    g_pio8255.vkbd_autotype_col = -1;
}


void pio8255_init ( void ) {
    PIO8255_KEYBOARD_MATRIX_RESET ( );
    memset ( &g_pio8255.vkbd_matrix, 0xff, sizeof ( g_pio8255.vkbd_matrix ) );
    g_pio8255.signal_PA = 0x00;
    g_pio8255.signal_PA_keybord_column = 0x00;
    g_pio8255.signal_PC = 0x00;
    pio8255_autotype_set_key_down_ms ( 20 );
    pio8255_autotype_set_key_up_ms ( 20 );
}

//static unsigned long bdcounter = 0;


/* data do CMT */
void static inline pio8255_set_pc01 ( uint8_t new_pc01 ) {
    if ( new_pc01 != g_pio8255.signal_pc01 ) {
        g_pio8255.signal_pc01 = new_pc01;
        cmt_write_data ( new_pc01 );
        //printf("bdc=%lu\n", bdcounter++);
#if 0
        static uint32_t last = 0;
#ifdef LINUX
        printf ( "CMT: %d - %lu, %lu\n", new_pc01, gdg_get_total_ticks ( ) - last, ( gdg_get_total_ticks ( ) - last ) / 8 );
#else
        printf ( "CMT: %d - %llu, %llu\n", new_pc01, gdg_get_total_ticks ( ) - last, ( gdg_get_total_ticks ( ) - last ) / 8 );
#endif
        last = gdg_get_total_ticks ( );
#endif
    };
}


void pio8255_write ( int addr, Z80EX_BYTE value ) {

    DBGPRINTF ( DBGINF, "addr = 0x%02x, value = 0x%02x, PC = 0x%04x\n", addr, value, g_mz800.instruction_addr );

    int bit_setres;



    switch ( addr ) {

        case DEF_PIO8255_PORTA:
            DBGPRINTF ( DBGINF, "addr = %d (PORT_A), value = 0x%02x )\n", addr, value );
            g_pio8255.signal_PA = value;

            // TODO: overit na skutecnem HW

            // 0. - 3. bit vzorkovani klavesnice - aktivni pri H
            int pahalf = value & 0x0f;
            g_pio8255.signal_PA_keybord_column = ( pahalf <= 9 ) ? pahalf : 10;

            // 4. bit Joystick-1 strobe - aktivni pri L
            g_pio8255.signal_PA_joy1_enabled = ( !( value & 0x20 ) ) ? 1 : 0;

            // 5. bit Joystick-2 strobe - aktivni pri L
            g_pio8255.signal_PA_joy2_enabled = ( !( value & 0x40 ) ) ? 1 : 0;

            // 7. bit cursor timer reset - aktivni pri L
            if ( !( value & 0x80 ) ) {
                mz800_cursor_timer_reset ( );
            };

            break;

        case DEF_PIO8255_PORTC:

            DBGPRINTF ( DBGINF, "addr = %d (PORT_C), value = 0x%02x), PC = 0x%04x\n", addr, value, g_mz800.instruction_addr );
            //printf ( "WR addr = %d (PORT_C), value = 0x%02x )\n", addr, value );

            /* blokovani CTC0 - zvukovy vystup */
            g_pio8255.signal_pc00 = ( value >> 0 ) & 0x01;
            DBGPRINTF ( DBGINF, "audio ctc0 mask (pc00): %d\n", g_pio8255.signal_pc00 );
            audio_ctc0_changed ( ( CTC8253_OUT ( 0 ) & CTC_AUDIO_MASK ), gdg_get_insigeop_ticks ( ) );

            /* data do CMT */
            pio8255_set_pc01 ( ( value >> 1 ) & 0x01 );

            /* blokovani CTC2 - preruseni z CTC */
            g_pio8255.signal_pc02 = ( value >> 2 ) & 0x01;
            DBGPRINTF ( DBGINF, "interrupt ctc2 mask (pc02): %d\n", g_pio8255.signal_pc02 );
            mz800_interrupt_manager ( );

            /* rizeni motoru CMT - nabezna hrana provede zmenu */
            unsigned old_pc03_state = g_pio8255.signal_pc03;
            g_pio8255.signal_pc03 = ( value >> 3 ) & 0x01;
            DBGPRINTF ( DBGINF, "cmt motor driver (pc03): %d\n", g_pio8255.signal_pc03 );
            if ( ( old_pc03_state == 0 ) && ( g_pio8255.signal_pc03 == 1 ) ) {
                if ( TEST_CMTHACK_INSTALLED ) {
                    g_pio8255.signal_pc04 = ( ~g_pio8255.signal_pc04 ) & 1;
                    DBGPRINTF ( DBGINF, "set CMT motor for CMT hack (pc04 - input): %d\n", g_pio8255.signal_pc04 );
                } else if ( !TEST_CMT_STOP ) {
                    if ( TEST_CMT_PAUSED ) {
                        cmt_pause ( 0 );
                    } else {
                        cmt_pause ( 1 );
                    };
                    DBGPRINTF ( DBGINF, "set CMT motor (virtual CMT): %d\n", ( ( TEST_CMT_STOP ) || ( TEST_CMT_PAUSED ) ) ? 0 : 1 );
                } else {
                    DBGPRINTF ( DBGINF, "set CMT motor (virtual CMT): ignored\n" );
                };
            };
            break;

        case DEF_PIO8255_MASTER:

            DBGPRINTF ( DBGINF, "addr = %d (MASTER_PORT), value = 0x%02x, PC = 0x%04x\n", addr, value, g_mz800.instruction_addr );

            if ( value & 0x80 ) {
                if ( value == 0x8a ) {
                    /* nastaveni MODE0, PortA: out, PortC_up: in, PortB: in, PortC_down: out */
                    /* vystupy jsou nastaveny na 0 */
                    g_pio8255.signal_PA = 0x00;
                    g_pio8255.signal_PA_keybord_column = 0x00;
                    g_pio8255.signal_pc00 = 0;
                    pio8255_set_pc01 ( 0 );
                    g_pio8255.signal_pc02 = 0;
                    DBGPRINTF ( DBGINF, "reset - pc00 - pc03 = 0x00\n" );

                    audio_ctc0_changed ( ( CTC8253_OUT ( 0 ) & CTC_AUDIO_MASK ), gdg_get_insigeop_ticks ( ) );
                    mz800_interrupt_manager ( );
#if ( DBGLEVEL & DBGWAR )
                } else {
                    DBGPRINTF ( DBGWAR, "addr = %d, value = 0x%02x - UNSUPORTED MODE! PC: 0x%04x\n", addr, value, z80ex_get_reg ( g_mz800.cpu, regPC ) );
#endif
                };
            } else {
                bit_setres = ( value >> 1 ) & 0x07;

                if ( bit_setres == 0 ) {
                    g_pio8255.signal_pc00 = value & 0x01;
                    DBGPRINTF ( DBGINF, "audio ctc0 mask (pc00): %d\n", g_pio8255.signal_pc00 );
                    audio_ctc0_changed ( ( CTC8253_OUT ( 0 ) & CTC_AUDIO_MASK ), gdg_get_insigeop_ticks ( ) );

                } else if ( bit_setres == 1 ) {
                    pio8255_set_pc01 ( value & 0x01 );

                } else if ( bit_setres == 2 ) {
                    g_pio8255.signal_pc02 = value & 0x01;
                    DBGPRINTF ( DBGINF, "interrupt ctc2 mask (pc02): %d\n", g_pio8255.signal_pc02 );
                    mz800_interrupt_manager ( );

                } else if ( bit_setres == 3 ) {
                    /* rizeni motoru CMT - nabezna hrana provede zmenu */
                    unsigned old_pc03_state = g_pio8255.signal_pc03;
                    g_pio8255.signal_pc03 = value & 0x01;
                    DBGPRINTF ( DBGINF, "cmt motor driver (pc03): %d\n", g_pio8255.signal_pc03 );
                    if ( ( old_pc03_state == 0 ) && ( g_pio8255.signal_pc03 == 1 ) ) {
                        if ( TEST_CMTHACK_INSTALLED ) {
                            g_pio8255.signal_pc04 = ( ~g_pio8255.signal_pc04 ) & 1;
                            DBGPRINTF ( DBGINF, "set CMT motor for CMT hack (pc04 - input): %d\n", g_pio8255.signal_pc04 );
                        } else if ( !TEST_CMT_STOP ) {
                            if ( TEST_CMT_PAUSED ) {
                                cmt_pause ( 0 );
                            } else {
                                cmt_pause ( 1 );
                            };
                            DBGPRINTF ( DBGINF, "set CMT motor (virtual CMT): %d\n", ( ( TEST_CMT_STOP ) || ( TEST_CMT_PAUSED ) ) ? 0 : 1 );
                        } else {
                            DBGPRINTF ( DBGINF, "set CMT motor (virtual CMT): ignored\n" );
                        };
                    };

                } else {
                    /* TODO: ostatni bity nas budou zajimat pozdeji:) */
                    printf ( "PIO8255 NOT IMPL. - bit: %d, value: %d\n", bit_setres, value & 1 );
                };
            };
            break;

    };

#if 0
    static unsigned int last_pc01_change = 0;
    static int last_pc01_state = 0;

    if ( g_pio8255.signal_pc01 != last_pc01_state ) {
        printf ( "CMT: %d, %d\n", last_pc01_state, g_timer.counter_gdg - last_pc01_change );
        last_pc01_state = g_pio8255.signal_pc01;
        last_pc01_change = g_timer.counter_gdg;
    };
#endif
}


int pio8255_autotype_get_matrix ( char c, Z80EX_BYTE *ret, gboolean *ret_shift ) {


    struct {
        char c;
        int col;
        int row;
        int shift;
    } key[] = {
               // col0
        { '_', 0, 7, 0 }, // Blank key
               // 6 => [Graph]
               //{ ???, 0, 5, 0 }, // Sipka dolu
        { PIO8255_VKBDAUTO_LIBRA, 0, 5, 1 }, // Libra + shift
               // 4 => [Alpha]
        { '\t', 0, 3, 0 },
        { ';', 0, 2, 0 },
        { '+', 0, 2, 1 },
        { ':', 0, 1, 0 },
        { '*', 0, 1, 1 },
        { '\n', 0, 0, 0 },

               // col1
        { 'Y', 1, 7, 0 },
        { 'y', 1, 7, 1 },
        { 'Z', 1, 6, 0 },
        { 'z', 1, 6, 1 },
        { '@', 1, 5, 0 },
        { '`', 1, 5, 1 },
        { '[', 1, 4, 0 },
        { '{', 1, 4, 1 },
        { ']', 1, 3, 0 },
        { '}', 1, 3, 1 },
        { ']', 1, 2, 0 },
        { '}', 1, 2, 1 },

               // col2
        { 'Q', 2, 7, 0 },
        { 'q', 2, 7, 1 },
        { 'R', 2, 6, 0 },
        { 'r', 2, 6, 1 },
        { 'S', 2, 5, 0 },
        { 's', 2, 5, 1 },
        { 'T', 2, 4, 0 },
        { 't', 2, 4, 1 },
        { 'U', 2, 3, 0 },
        { 'u', 2, 3, 1 },
        { 'V', 2, 2, 0 },
        { 'v', 2, 2, 1 },
        { 'W', 2, 1, 0 },
        { 'w', 2, 1, 1 },
        { 'X', 2, 0, 0 },
        { 'x', 2, 0, 1 },

               // col3
        { 'I', 3, 7, 0 },
        { 'i', 3, 7, 1 },
        { 'J', 3, 6, 0 },
        { 'j', 3, 6, 1 },
        { 'K', 3, 5, 0 },
        { 'k', 3, 5, 1 },
        { 'L', 3, 4, 0 },
        { 'l', 3, 4, 1 },
        { 'M', 3, 3, 0 },
        { 'm', 3, 3, 1 },
        { 'N', 3, 2, 0 },
        { 'n', 3, 2, 1 },
        { 'O', 3, 1, 0 },
        { 'o', 3, 1, 1 },
        { 'P', 3, 0, 0 },
        { 'p', 3, 0, 1 },

               // col4
        { 'A', 4, 7, 0 },
        { 'a', 4, 7, 1 },
        { 'B', 4, 6, 0 },
        { 'b', 4, 6, 1 },
        { 'C', 4, 5, 0 },
        { 'c', 4, 5, 1 },
        { 'D', 4, 4, 0 },
        { 'd', 4, 4, 1 },
        { 'E', 4, 3, 0 },
        { 'e', 4, 3, 1 },
        { 'F', 4, 2, 0 },
        { 'f', 4, 2, 1 },
        { 'G', 4, 1, 0 },
        { 'g', 4, 1, 1 },
        { 'H', 4, 0, 0 },
        { 'h', 4, 0, 1 },

               // col5
        { '1', 5, 7, 0 },
        { '!', 5, 7, 1 },
        { '2', 5, 6, 0 },
        { '"', 5, 6, 1 },
        { '3', 5, 5, 0 },
        { '#', 5, 5, 1 },
        { '4', 5, 4, 0 },
        { '$', 5, 4, 1 },
        { '5', 5, 3, 0 },
        { '%', 5, 3, 1 },
        { '6', 5, 2, 0 },
        { '&', 5, 2, 1 },
        { '7', 5, 1, 0 },
        { '\'', 5, 1, 1 },
        { '8', 5, 0, 0 },
        { '(', 5, 0, 1 },

               // col6
        { '\\', 6, 7, 0 },
        { '|', 6, 7, 1 },
        { '^', 6, 6, 0 },
        { '~', 6, 6, 1 },
        { '-', 6, 5, 0 },
        { '=', 6, 5, 1 },
        { ' ', 6, 4, 0 },
               //{ ' ', 6, 4, 1 },
        { '0', 6, 3, 0 },
        { PIO8255_VKBDAUTO_PI, 6, 3, 1 }, // PI
        { '9', 6, 2, 0 },
        { ')', 6, 2, 1 },
        { ',', 6, 1, 0 },
        { '<', 6, 1, 1 },
        { '.', 6, 0, 0 },
        { '>', 6, 0, 1 },

               // col7
        { '?', 7, 1, 0 },
               //{ '?', 6, 1, 1 },
        { '/', 7, 0, 0 },
               //{ '/', 6, 0, 1 },

        { 0, 0, 0, 0 },
    };

    int i = 0;
    while ( 1 ) {
        if ( ( key[i].c == c ) || ( key[i].c == 0 ) ) break;
        i++;
    };

    if ( !key[i].c ) return -1;

    *ret = 0xff & ~( 1 << key[i].row );
    *ret_shift = ( key[i].shift ) ? TRUE : FALSE;
    return key[i].col;
}


Z80EX_BYTE pio8255_read ( int addr ) {

    DBGPRINTF ( DBGINF, "addr = 0x%02x, PC = 0x%04x\n", addr, g_mz800.instruction_addr );

    switch ( addr ) {

        case DEF_PIO8255_PORTA:
            if ( g_pio8255.signal_PA_keybord_column < 10 ) return g_pio8255.signal_PA;
            return 0xff;
            break;

        case DEF_PIO8255_PORTB:

            if ( g_pio8255.vkbd_autotype ) {
                if ( g_pio8255.vkbd_autotype_col == -1 ) {
                    while ( g_pio8255.vkbd_autotype[0] != 0x00 ) {
                        g_pio8255.vkbd_autotype_col = pio8255_autotype_get_matrix ( g_pio8255.vkbd_autotype[0], &g_pio8255.vkbd_autotype_ret, &g_pio8255.vkbd_autotype_ret_shift );
                        g_pio8255.vkbd_autotype++;
                        if ( g_pio8255.vkbd_autotype_col != -1 ) break;
                    };
                    g_pio8255.vkbd_autotype_start_ticks = gdg_get_total_ticks ( );
                    g_pio8255.vkbd_autotype_keydown = ( g_pio8255.vkbd_autotype_ret_shift ) ? 0 : 1;
                };

                if ( g_pio8255.vkbd_autotype_col == -1 ) {
                    ui_vkbd_autotype_deactivate ( );
                } else {

                    Z80EX_BYTE ret = 0xff;

                    if ( g_pio8255.vkbd_autotype_keydown == 0 ) {
                        if ( ( g_pio8255.vkbd_autotype_ret_shift ) && ( g_pio8255.signal_PA_keybord_column == 8 ) ) {
                            ret = 0xfe;
                        };
                    } else if ( g_pio8255.vkbd_autotype_keydown == 1 ) {
                        if ( ( g_pio8255.vkbd_autotype_ret_shift ) && ( g_pio8255.signal_PA_keybord_column == 8 ) ) {
                            ret = 0xfe;
                        } else if ( g_pio8255.signal_PA_keybord_column == g_pio8255.vkbd_autotype_col ) {
                            ret = g_pio8255.vkbd_autotype_ret;
                        };
                    };

                    uint64_t now_ticks = gdg_get_total_ticks ( );

                    if ( g_pio8255.vkbd_autotype_keydown <= 1 ) {
                        if ( now_ticks >= ( g_pio8255.vkbd_autotype_start_ticks + g_pio8255.vkbd_autotype_kd_ticks ) ) {
                            g_pio8255.vkbd_autotype_keydown++;
                            g_pio8255.vkbd_autotype_start_ticks = now_ticks;
                        };
                    } else {
                        if ( now_ticks >= ( g_pio8255.vkbd_autotype_start_ticks + g_pio8255.vkbd_autotype_ku_ticks ) ) {
                            g_pio8255.vkbd_autotype_col = -1;
                        };
                    };

                    return ret;
                };
            };

            iface_sdl_pool_keyboard_events ( );
            //g_pio8255.keyboard_matrix [ 2 ] &= 0xdf;
            Z80EX_BYTE retval = g_pio8255.keyboard_matrix [ g_pio8255.signal_PA_keybord_column ] & g_pio8255.vkbd_matrix [ g_pio8255.signal_PA_keybord_column ];
            DBGPRINTF ( DBGINF, "addr = 0x%02x, keyboard_matrix[%d] = 0x%02x, PC = 0x%04x\n", addr, g_pio8255.signal_PA_keybord_column, retval, g_mz800.instruction_addr );
            return retval;

            /* TODO: prozatim mame jen ty nejpodstatnejsi bity */
        case DEF_PIO8255_PORTC:
            /*
             * 0. blokovani CTC0_OUT
             * 1. data do CMT
             * 2. zakaz preruseni z CTC
             * 3. rizeni motoru CMT 0 - 1 - 0
             * 4. test stavu motoru CMT
             * 5. data z CMT
             * 6. cursor timer
             * 7. VBLNK
             *
             */

            retval = 0x00;
            retval |= SIGNAL_GDG_VBLNK ? 1 << 7 : 0;
            retval |= mz800_get_cursor_timer_state ( ) << 6;
#if 1
            retval |= ( cmt_read_data ( ) & 1 ) << 5;
#else
            int cmtdata = ( cmt_read_data ( ) & 1 );
            retval |= ( cmtdata ) << 5;
            static uint32_t gdg_last_time = 0;
            printf ( "IORQ - %d, %d\n", cmtdata, gdg_get_total_ticks ( ) - gdg_last_time );
            gdg_last_time = gdg_get_total_ticks ( );
#endif
            if ( TEST_CMTHACK_INSTALLED ) {
                retval |= g_pio8255.signal_pc04 << 4;
            } else {
                retval |= ( ( TEST_CMT_STOP ) || ( TEST_CMT_PAUSED ) ) ? 0 : ( 1 << 4 );
            };
            //retval |= SIGNAL_GDG_VBLNK << 4; /* simulujeme promenlivy stav motoru (jinak neni mozne spustit hru "24" - musel by se manualne menit stav motoru ) */

            retval |= g_pio8255.signal_pc02 << 2;
            retval |= g_pio8255.signal_pc00;

            //DBGPRINTF ( "PIO8255_PORTC read - retval = 0x%02x\n", retval );
            //printf ( "PIO8255_PORTC read - retval = 0x%02x, PC = 0x%04x\n", retval & 0x10, z80ex_get_reg ( g_mz800.cpu, regPC ) );

            //printf ( "8255 PortB_5: %d, %lu\n", ( retval >> 5 ) & 1, gdg_get_total_ticks ( ) );

            return retval;
            break;

        default:
            DBGPRINTF ( DBGWAR, "addr = %d - unsupported addr\n", addr );
            break;
    };
    return 0x00;
}


void pio8255_pc2_set ( int value ) {
    g_pio8255.signal_pc02 = value & 1;
}


int pio8255_pc1_get ( void ) {
    return g_pio8255.signal_pc01;
}


int pio8255_pc2_get ( void ) {
    return g_pio8255.signal_pc02;
}


int pio8255_pc4_get ( void ) {
    if ( TEST_CMTHACK_INSTALLED ) {
        return g_pio8255.signal_pc04;
    };
    return ( ( TEST_CMT_STOP ) || ( TEST_CMT_PAUSED ) ) ? 0 : 1;
}


int pio8255_pa4_get ( void ) {
    return g_pio8255.signal_PA_joy1_enabled;
}


int pio8255_pa5_get ( void ) {
    return g_pio8255.signal_PA_joy2_enabled;
}


int pio8255_pa0_3_get ( void ) {
    return g_pio8255.signal_PA_keybord_column;
}
