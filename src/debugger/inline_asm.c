/* 
 * File:   iasm.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 16. září 2015, 8:43
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

#include <stdio.h>
#include <string.h>

#ifdef MZ800EMU_CFG_DEBUGGER_ENABLED


#define DBG_PRINT 0

#include "inline_asm.h"
#include "inline_asm_opcodes.h"

#include "z80ex/include/z80ex.h"
#include "ui/ui_main.h"

typedef enum en_IASMSTATE {
    INLASM_CMD = 0,
    INLASM_PAR1,
    INLASM_VAR1,
    INLASM_PAR2,
    INLASM_VAR2,
    INLASM_PAR3,
    INLASM_COUNT
} en_IASMSTATE;


static void iasm_trim_lspaces ( char *text ) {
    while ( text [ 0 ] == ' ' ) {
        strncpy ( text, &text [ 1 ], IASM_MNEMONICS_MAXLEN + 1 );
    };
}


static void iasm_trim_rspaces ( char *text ) {
    unsigned i = strlen ( text );
    if ( i ) {
        while ( text [ i - 1 ] == ' ' ) {
            text [ i - 1 ] = 0x00;
            i--;
            if ( !i ) {
                return;
            };
        };
    };
}


/*
 * Variables:
 *
 *	nnn		- desitkove cislo
 *	n[a-f]		- hex
 *	0xnnnn		- hex
 *	#nnnn		- hex
 *	%nnnnnnnn	- binarni cislo
 */

static Z80EX_BYTE iasm_char2hex ( char c ) {

    Z80EX_BYTE retval = 0;

    if ( ( c >= '0' ) && ( c <= '9' ) ) {
        retval = c - '0';
    } else {
        retval = c - 'A' + 0x0a;
    };
    return retval;
}


static int iasm_decode_hex_txt ( char *c, Z80EX_WORD *value ) {
    int i = 0;
    int k = 0;
    int length;

    *value = 0;

    while ( ( c [ i ] != 0x00 ) && (
            ( ( c [ i ] >= '0' ) && ( c [ i ] <= '9' ) ) ||
            ( ( c [ i ] >= 'a' ) && ( c [ i ] <= 'f' ) ) ||
            ( ( c [ i ] >= 'A' ) && ( c [ i ] <= 'F' ) ) ) ) {
        if ( ( c [ i ] >= 'a' ) && ( c [ i ] <= 'f' ) ) {
            c [ i ] -= 0x20;
        };
        i++;
    };

    if ( i == 0 ) {
        return 0;
    };
    length = i;
    i--;

    while ( i >= 0 ) {
        *value += iasm_char2hex ( c [ i ] ) << k;
        k += 4;
        i--;

    };

    return length;
}


static int iasm_decode_dec_txt ( char *c, Z80EX_WORD *value ) {
    int i = 0;
    int j;
    int k = 1;
    int length;

    *value = 0;

    while ( ( c [ i ] != 0x00 ) && ( ( c [ i ] >= '0' ) && ( c [ i ] <= '9' ) ) ) {
        i++;
    };
    if ( ( ( c [ i ] >= 'a' ) && ( c [ i ] <= 'f' ) ) || ( ( c [ i ] >= 'A' ) && ( c [ i ] <= 'F' ) ) ) {
        return iasm_decode_hex_txt ( c, value );
    };
    if ( i == 0 ) {
        return 0;
    };
    length = i;
    i--;

    while ( i >= 0 ) {
        j = c [ i ] - '0';
        *value += j * k;
        k = k * 10;
        i--;
    };

    return length;
}


static int iasm_decode_bin_txt ( char *c, Z80EX_WORD *value ) {

    int i = 0;
    int j = 1;
    int length;

    *value = 0;

    while ( ( c [ i ] != 0x00 ) && ( ( c [ i ] == '0' ) || ( c [ i ] == '1' ) ) ) {
        i++;
    };

    if ( i == 0 ) {
        return 0;
    };
    length = i;
    i--;

    while ( i >= 0 ) {
        if ( c [ i-- ] == '1' ) {
            *value += j;
        };
        j = j << 1;
    };

    return length;
}


unsigned debugger_iasm_assemble_line ( Z80EX_WORD addr, const char *assemble_txt, st_IASMBIN *compiled_output ) {

    char parsed_line [ IASM_MNEMONICS_MAXLEN + 1 ];

    int j, k, legal_space, is_not_variable, sign, rel_addr;
    int length [ INLASM_COUNT ], var_length, var_prefix_length;
    char *pos [ INLASM_COUNT ], *c;
    en_IASMSTATE inlasm_state;
    Z80EX_WORD var [ 2 ], value;
    int variables_count;

    const inline_asm_commands_t *asm_commands;
    const inline_asm_opts_t *asm_opts;
    const char *cmd_opts;


    strncpy ( parsed_line, assemble_txt, sizeof ( parsed_line ) );
    parsed_line [ IASM_MNEMONICS_MAXLEN ] = 0x00;


    /* korekce vstupnich dat */
    iasm_trim_lspaces ( parsed_line );
    iasm_trim_rspaces ( parsed_line );

    memset ( &length, 0x00, sizeof ( length ) );
    memset ( &pos, 0x00, sizeof ( pos ) );

    int flag_cmd_is_ld = 0;
    int flag_cmd_is_rst = 0;
    int flag_ld_set_res = 0;

    variables_count = 0;
    sign = 0;

    inlasm_state = INLASM_CMD;
    pos [ inlasm_state ] = &parsed_line[0];

    int i = 0;
    while ( ( parsed_line [ i ] != 0x00 ) ) {
        if ( ( parsed_line [ i ] >= 'a' ) && ( parsed_line [ i ] <= 'z' ) ) {
            parsed_line [ i ] -= 0x20;
        };

        if ( inlasm_state == INLASM_CMD ) {
            if ( parsed_line [ i ] == ' ' ) {
                length [ INLASM_CMD ] = i;
                inlasm_state++;
                pos [ inlasm_state ] = &parsed_line [ i + 1 ];
                if ( 0 == strncmp ( pos [ INLASM_CMD ], "LD ", 3 ) ) {
                    flag_cmd_is_ld = 1;
                } else if ( 0 == strncmp ( pos [ INLASM_CMD ], "RST ", 4 ) ) {
                    flag_cmd_is_rst = 1;
                };
            };


        } else {

            /* odstranime nepovolene mezery */
            if ( parsed_line [ i ] == ' ' ) {

                legal_space = 0;

                if ( flag_cmd_is_ld ) {
                    if ( inlasm_state == INLASM_PAR1 ) {
                        c = pos [ inlasm_state ];
                        if ( ( c [ 0 ] == 'A' ) || ( c [ 0 ] == 'B' ) || ( c [ 0 ] == 'C' ) || ( c [ 0 ] == 'D' ) || ( c [ 0 ] == 'E' ) || ( c [ 0 ] == 'H' ) || ( c [ 0 ] == 'L' ) ) {
                            if ( i == 7 ) {
                                if ( ( 0 == strncmp ( &c [ 1 ], ",RL", 3 ) ) || ( 0 == strncmp ( &c [ 1 ], ",RR", 3 ) ) ) {
                                    legal_space = 1;
                                };
                            } else if ( i == 8 ) {
                                if ( ( 0 == strncmp ( &c [ 1 ], ",RES", 4 ) ) || ( 0 == strncmp ( &c [ 1 ], ",SET", 4 ) ) ) {
                                    legal_space = 1;
                                    flag_ld_set_res = 1;

                                } else if ( ( 0 == strncmp ( &c [ 1 ], ",RLC", 4 ) ) || ( 0 == strncmp ( &c [ 1 ], ",RRC", 4 ) ) || ( 0 == strncmp ( &c [ 1 ], ",SLA", 4 ) ) || ( 0 == strncmp ( &c [ 1 ], ",SLL", 4 ) ) || ( 0 == strncmp ( &c [ 1 ], ",SRA", 4 ) ) || ( 0 == strncmp ( &c [ 1 ], ",SRL", 4 ) ) ) {
                                    legal_space = 1;
                                };
                            };
                        };
                    };
                };


                if ( !legal_space ) {
                    iasm_trim_lspaces ( &parsed_line [ i ] );
                    if ( ( parsed_line [ i ] >= 'a' ) && ( parsed_line [ i ] <= 'z' ) ) {
                        parsed_line [ i ] -= 0x20;
                    };
                };
            };


            if ( inlasm_state != INLASM_PAR3 ) {

                if ( ( sign == 0 ) && ( ( parsed_line [ i ] == '+' ) || ( parsed_line [ i ] == '-' ) ) ) {

                    if ( ( 0 == strncmp ( &parsed_line [ i - 3 ], "(IX", 3 ) ) || ( 0 == strncmp ( &parsed_line [ i - 3 ], "(IY", 3 ) ) ) {
                        if ( parsed_line [ i ] == '+' ) {
                            sign = 1;
                        } else {
                            sign = -1;
                        };
                        parsed_line [ i ] = '+';
                    };

                } else {

                    var_length = 0;

                    if ( ( parsed_line [ i ] >= '0' ) && ( parsed_line [ i ] <= '9' ) ) {

                        is_not_variable = 0;
                        /* 0 neni variable - je soucasti par1 */
                        if ( inlasm_state == INLASM_PAR1 ) {
                            c = pos [ INLASM_CMD ];
                            if ( ( ( flag_ld_set_res ) && ( i == 9 ) ) ||
                                    ( ( i == 4 ) && ( ( 0 == strncmp ( c, "BIT", length [ INLASM_CMD ] ) ) || ( 0 == strncmp ( c, "RES", length [ INLASM_CMD ] ) ) || ( 0 == strncmp ( c, "SET", length [ INLASM_CMD ] ) ) ) ) ||
                                    ( ( i == 3 ) && ( 0 == strncmp ( c, "IM", length [ INLASM_CMD ] ) ) ) ||
                                    ( ( i == 8 ) && ( 0 == strncmp ( parsed_line, "OUT (C),", 8 ) ) ) ) {
                                is_not_variable = 1;
                            };
                        };


                        if ( !is_not_variable ) {
                            if ( ( parsed_line [ i ] == '0' ) && ( ( parsed_line [ i + 1 ] == 'x' ) || ( parsed_line [ i + 1 ] == 'X' ) ) ) {
                                var_prefix_length = 2;
                                var_length = iasm_decode_hex_txt ( &parsed_line [ i + var_prefix_length ], &value );
                            } else {
                                var_prefix_length = 0;
                                var_length = iasm_decode_dec_txt ( &parsed_line [ i ], &value );
                            };
                        };

                    } else if ( parsed_line [ i ] == '#' ) {
                        var_prefix_length = 1;
                        var_length = iasm_decode_hex_txt ( &parsed_line [ i + var_prefix_length ], &value );

                    } else if ( parsed_line [ i ] == '%' ) {
                        var_prefix_length = 1;
                        var_length = iasm_decode_bin_txt ( &parsed_line [ i + var_prefix_length ], &value );
                    };


                    /* nasli jsme platny variable? */
                    if ( var_length ) {

                        if ( sign != 0 ) {
                            if ( value <= 0xff ) { /* pokud je hodnota vetsi, tak na znamenko nesahame - chyba se ohlasi jinde */
                                if ( ( sign == -1 ) && ( value < 128 ) && ( value > 0 ) ) { /* pokud bylo vlozeno (IX-nn), kde nn < 128, tak sahneme na znamenko */
                                    value = 0x100 - value;
                                };
                            };
                        };

                        var [ variables_count++ ] = value;
                        length [ inlasm_state ] = ( &parsed_line [ i ] - pos [ inlasm_state ] ) / sizeof ( char );
                        inlasm_state++;
                        pos [ inlasm_state ] = &parsed_line [ i ];
                        var_length += var_prefix_length;
                        length [ inlasm_state ] = var_length;
                        inlasm_state++;
                        pos [ inlasm_state ] = &parsed_line [ i + var_length ];
                        i += var_length - 1;
                    };

                    sign = 0;
                };
            };
        };

        i++;
    };
    if ( !length [ INLASM_CMD ] ) {
        length [ INLASM_CMD ] = i;
    };

    if ( !length [ inlasm_state ] ) {
        c = pos [ inlasm_state ];
        if ( c [ 0 ] != 0x00 ) {
            length [ inlasm_state ] = ( &parsed_line [ i ] - pos [ inlasm_state ] ) / sizeof ( char );
        };
    };

    c = pos [ INLASM_VAR1 ];
    if ( c != NULL ) {
        c [ 0 ] = 0x00;
    };

    c = pos [ INLASM_VAR2 ];
    if ( c != NULL ) {
        c [ 0 ] = 0x00;
    };

    c = pos [ INLASM_CMD ];
    c [ length [ INLASM_CMD ] ] = 0x00;


#if DBG_PRINT
    printf ( "str: '%s'\n", assemble_txt );
    printf ( "cmd: '%s', len: %d\n", c, length [ INLASM_CMD ] );

    if ( length [ INLASM_PAR1 ] ) {
        c = pos [ INLASM_PAR1 ];
        printf ( "par1: '%s', len: %d\n", c, length [ INLASM_PAR1 ] );
    };

    if ( length [ INLASM_VAR1 ] ) {
        printf ( "var1: '0x%04x', len: %d\n", var [ 0 ], length [ INLASM_VAR1 ] );
    };

    if ( length [ INLASM_PAR2 ] ) {
        c = pos [ INLASM_PAR2 ];
        printf ( "par2: '%s', len: %d\n", c, length [ INLASM_PAR2 ] );
    };

    if ( length [ INLASM_VAR2 ] ) {
        printf ( "var2: '0x%04x', len: %d\n", var [ 1 ], length [ INLASM_VAR2 ] );
    };

    if ( length [ INLASM_PAR3 ] ) {
        c = pos [ INLASM_PAR3 ];
        printf ( "par3: '%s', len: %d\n", c, length [ INLASM_PAR3 ] );
    };
    printf ( "\n" );

    c = pos [ INLASM_CMD ];
#endif


    /* usnadneni pro instrukce RST nn */
    if ( ( flag_cmd_is_rst ) && ( variables_count == 1 ) && ( ( length [ INLASM_PAR1 ] | length [ INLASM_PAR2 ] ) == 0 ) ) {
        switch ( var [ 0 ] ) {
            case 10:
                var [ 0 ] = 0x10;
                break;
            case 18:
                var [ 0 ] = 0x18;
                break;
            case 20:
                var [ 0 ] = 0x20;
                break;
            case 28:
                var [ 0 ] = 0x28;
                break;
            case 30:
                var [ 0 ] = 0x30;
                break;
            case 38:
                var [ 0 ] = 0x38;
                break;
        };
        sprintf ( pos [ INLASM_PAR1 ], "0x%02X", var [ 0 ] );
        length [ INLASM_PAR1 ] = strlen ( pos [ INLASM_PAR1 ] );
        variables_count = 0;
    };

    asm_commands = NULL;
    asm_opts = NULL;
    cmd_opts = NULL;

    /* hledame sadu instrukci podle prvniho pismene */
    i = 0;
    while ( inline_asm_chars [ i ] . c != 0x00 ) {
        if ( inline_asm_chars [ i ] . c == c [ 0 ] ) {
            asm_commands = inline_asm_chars [ i ] . commands;
            break;
        };
        i++;
    };

    j = 0;
    if ( asm_commands != NULL ) {
        while ( asm_commands [ j ] . command != NULL ) {
            if ( 0 == strcmp ( c, asm_commands [ j ] . command ) ) {
#if DBG_PRINT
                printf ( "Found command: %s\n", asm_commands [ j ] . command );
#endif
                asm_opts = asm_commands [ j ] . opts;
                break;
            };
            j++;
        };

        k = 0;
        if ( asm_opts != NULL ) {
            while ( asm_opts [ k ] . optocode != NULL ) {
                /* pokud souhlasi pocet variables, pocet znaku pred 1. variable a pocet za nim, tak hledame dal */
                if ( ( asm_opts [ k ] . variables == variables_count ) && ( asm_opts [ k ] . llen == length [ INLASM_PAR1 ] ) && ( asm_opts [ k ] . rlen == length [ INLASM_PAR2 ] ) ) {
                    if ( 0 == variables_count ) {
                        if ( !length [ INLASM_PAR1 ] ) {
                            /* NALEZENO: instrukce nema zadne parametry */
#if DBG_PRINT
                            printf ( "Found - instruction not have params or variables\n" );
#endif
                            cmd_opts = asm_opts [ k ] . optocode;
                            break;
                        };
                        if ( 0 == strcmp ( pos [ INLASM_PAR1 ], asm_opts [ k ] . param ) ) {
                            /* NALEZENO: instrukce ma shodny parametr a nema zadne variables */
#if DBG_PRINT
                            printf ( "Found param: %s\n", asm_opts [ k ] . param );
#endif
                            cmd_opts = asm_opts [ k ] . optocode;
                            break;
                        };

                    } else if ( length [ INLASM_PAR1 ] == 0 ) {
                        /* NALEZENO: instrukce nema zadny parametr, ale ma jeden variables */
#if DBG_PRINT
                        printf ( "Found - instruction not have params, but have variable\n" );
#endif
                        cmd_opts = asm_opts [ k ] . optocode;
                        break;
                    } else {
                        if ( 0 == strncmp ( pos [ INLASM_PAR1 ], asm_opts [ k ] . param, length [ INLASM_PAR1 ] ) ) {

                            /* souhlasi parametr pred variable */

                            if ( length [ INLASM_PAR2 ] == 0 ) {
                                /* NALEZENO: za variable uz neni zadny parametr a instrukce je ukoncena variablem */
#if DBG_PRINT
                                printf ( "Found param: %s\n", asm_opts [ k ] . param );
#endif
                                cmd_opts = asm_opts [ k ] . optocode;
                                break;

                            } else {
                                if ( variables_count == 1 ) {
                                    if ( 0 == strcmp ( pos [ INLASM_PAR2 ], &asm_opts [ k ] . param [ length [ INLASM_PAR1 ] + 1 ] ) ) {
                                        /* NALEZENO: souhlasi parametr pred i za variable */
#if DBG_PRINT
                                        printf ( "Found param: %s\n", asm_opts [ k ] . param );
#endif
                                        cmd_opts = asm_opts [ k ] . optocode;
                                        break;
                                    };
                                } else {
                                    /* 2 variables maji jen tyto instrukce: LD (IX+$),# */
                                    if ( ( 0 == strcmp ( pos [ INLASM_PAR2 ], ")," ) ) && ( length [ INLASM_PAR3 ] == 0 ) ) {
                                        /* NALEZENO: */
#if DBG_PRINT
                                        printf ( "Found param: %s\n", asm_opts [ k ] . param );
#endif
                                        cmd_opts = asm_opts [ k ] . optocode;
                                        break;
                                    };
                                };
                            };
                        };
                    };
                };
                k++;
            };
        };
    };



    /* vygenerujeme optocode */

    int flag_variable_is_out_of_range = 0;
    st_IASMBIN compiled;

    if ( cmd_opts != NULL ) {

        memset ( &compiled, 0x00, sizeof ( compiled ) );

        int optocode_position = 0;
        int compiled_position = 0;
        int flag_lowhi_byte = 0; /* 0.bit je ukazatel pozice uvnitr word bajtu L/H */
        int variables_count = 0;


        while ( ( cmd_opts [ optocode_position ] != 0x00 ) && ( !flag_variable_is_out_of_range ) ) {
            switch ( cmd_opts [ optocode_position ] ) {
                case '@':
                    compiled.byte[ compiled_position++ ] = var [ variables_count ] & 0xff;
                    compiled.byte[ compiled_position++ ] = ( var [ variables_count ] >> 8 ) & 0xff;
                    variables_count++;
                    break;
                case '#':
                case '$':
                    if ( var [ variables_count ] <= 0xff ) {
                        compiled.byte[ compiled_position++ ] = var [ variables_count++ ];
                    } else {
                        flag_variable_is_out_of_range = 1;
                    };
                    break;
                case '%':
                    rel_addr = var [ variables_count ] - ( addr + 2 ); /* Vsechny instrukce s relativni hodnotou maji 2 bajty */
                    if ( ( rel_addr >= -128 ) && ( ( rel_addr <= 127 ) ) ) {
                        compiled.byte[ compiled_position++ ] = (Z80EX_BYTE) rel_addr;
                    } else {
                        flag_variable_is_out_of_range = 1;
                    };
                    break;
                default:
                    compiled.byte[ compiled_position ] |= iasm_char2hex ( cmd_opts [ optocode_position ] ) << ( ( ( ~flag_lowhi_byte ) & 1 ) * 4 );
                    if ( flag_lowhi_byte & 1 ) {
                        compiled_position++;
                    };
                    flag_lowhi_byte++;
                    break;
            };
            optocode_position++;
        };

        compiled.length = compiled_position;
    };


    /* Doslo k chybe pri kompilaci? */
    if ( ( cmd_opts == NULL ) || ( flag_variable_is_out_of_range ) ) {
        char *errmsg;
        if ( cmd_opts == NULL ) {
            errmsg = "Invalid instruction optocode or parameter!";
        } else {
            errmsg = "Value is out of range!";
        };
        ui_show_error ( "Inline Assembler Error - %s\n\nAddr = 0x%04x\nInstruction = '%s'\n", errmsg, addr, assemble_txt );
        return 0;
    };

    memcpy ( compiled_output, &compiled, sizeof ( compiled ) );

#if DBG_PRINT
    for ( i = 0; i < compiled.length; i++ ) {
        printf ( "compiled output: 0x%04x - 0x%02x\n", addr + i, compiled.byte[ i ] );
    };
#endif       
    return compiled.length; /* vratime pocet zkompilovanych bajtu */

#if 0    
    g_mz800.dasm_memop = 1;
    j--;
    while ( j >= 0 ) {
        /*
                printf ( "write: 0x%04x - 0x%02x\n", addr + j, optocode_bin [ j ] );
         */
        memory_write_cb ( g_mz800.cpu, addr + j, optocode_bin [ j ], NULL );
        j--;
    };
    g_mz800.dasm_memop = 0;


    debugger_update_disassembled ( addr, 1, debugger_disassembled_get_selected_row ( ) );

    debugger_full_update_stack ( );

    debugger_hide_inline_asm_window ( );
#endif

}


#endif

