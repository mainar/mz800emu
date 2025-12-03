/* 
 * File:   cmthack.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 2. července 2015, 20:49
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
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <strings.h>

#include "cmthack.h"
#include "cmtext.h"

#include "z80ex/include/z80ex.h"
#include "memory/memory.h"
#include "memory/rom.h"
#include "mz800.h"
#include "gdg/vramctrl.h"
#include "cmt.h"

#include "ui/ui_main.h"
#include "ui/ui_file_chooser.h"
#include "ui/ui_cmt.h"

#include "cfgmain.h"

#include "ui/ui_utils.h"
#include "libs/generic_driver/generic_driver.h"
#include "ui/generic_driver/ui_memory_driver.h"
#include "libs/mzf/mzf.h"
#include "libs/mzf/mzf_tools.h"


#include "ui/vkbd/ui_vkbd.h"

st_CMTHACK g_cmthack;


static st_DRIVER *g_driver = &g_ui_memory_driver_static;
#define CMT_HACK_DEFAULT_HANDLER_TYPE HANDLER_TYPE_MEMORY


typedef enum en_LOADRET {
    LOADRET_OK = 0,
    LOADRET_ERROR,
    LOADRET_BREAK
} en_LOADRET;


void cmthack_install_rom_patch ( void ) {

    /* ROM hack */

    /* 
     * Precteni hlavicky z CMT
     *
     *	RHEAD:
     *
     *	Vystup: CY = 1  doslo k chybe
     *			A = 1  chyba kontrolniho souctu
     *			A = 2  detekovano BREAK
     *		CY = 0  O.K.
     *
     */
    g_memory.ROM [ 0x04d8 ] = 0xe5; /* push HL */
    g_memory.ROM [ 0x04d9 ] = 0x21; /* ld HL, 0x10f0 */
    g_memory.ROM [ 0x04da ] = 0xf0;
    g_memory.ROM [ 0x04db ] = 0x10;
    g_memory.ROM [ 0x04dc ] = 0xd3; /* out (0x01), a */
    g_memory.ROM [ 0x04dd ] = 0x01;
    g_memory.ROM [ 0x04de ] = 0xe1; /* pop HL */
    g_memory.ROM [ 0x04df ] = 0xc9; /* ret */


    /*
     *	RDATA:
     *
     * Precte program z CMT podle informaci
     *
     *	vystup:  CY s vyznamem jako u RHEAD
     *
     */
    g_memory.ROM [ 0x04f8 ] = 0xe5; /* push HL */
    g_memory.ROM [ 0x04f9 ] = 0xc5; /* push BC */
    g_memory.ROM [ 0x04fa ] = 0x2a; /* ld HL, (0x1104) */
    g_memory.ROM [ 0x04fb ] = 0x04;
    g_memory.ROM [ 0x04fc ] = 0x11;
    g_memory.ROM [ 0x04fd ] = 0xed; /* ld BC, (0x1102) */
    g_memory.ROM [ 0x04fe ] = 0x4b;
    g_memory.ROM [ 0x04ff ] = 0x02;
    g_memory.ROM [ 0x0500 ] = 0x11;
    g_memory.ROM [ 0x0501 ] = 0xd3; /* out (0x02), A */
    g_memory.ROM [ 0x0502 ] = 0x02;
    g_memory.ROM [ 0x0503 ] = 0xc1; /* pop BC */
    g_memory.ROM [ 0x0504 ] = 0xe1; /* pop BC */
    g_memory.ROM [ 0x0505 ] = 0xc9; /* ret */


    /*
     * 
     * JSS 1.3 a 1.6A na adrese 0xf3bb (0xf3b8) provadi kontrolu horni a dolni ROM:
     * 
     * xor a, nasledne xor 0x0000 - 0x0xfff, 0xe010 - 0xffff
     * 
     * obsah porovna s tim co je na adrese 0xe840 ( == 0xed)
     * 
     * Bohuzel nikdo uz asi nikdy nezjisti na ktere adrese dela JSS korekci, aby 
     * byl vysledny XOR = 0xed
     * 
     */

    g_memory.ROM [ 0x0506 ] = 0x59; /* pokud zde dame 0x00, tak XOR vychazi 0xb4, proto: 0xb4 ^ 0xed = 0x59 */

    cmt_stop ( );
}


void cmthack_reinstall_rom_patch ( void ) {
    if ( !TEST_ROM_WILLY ) {
        if ( ( g_rom.type != ROMTYPE_USER_DEFINED ) || ( g_rom.user_defined_cmthack_type == ROM_CMTHACK_DEFAULT ) ) {
            if ( g_cmthack.load_patch_installed ) {
                cmthack_install_rom_patch ( );
            };
        };
    };
    ui_cmt_hack_menu_update ( );
}


void cmthack_load_rom_patch ( unsigned enabled ) {

    if ( !TEST_ROM_WILLY ) {
        if ( ( !TEST_ROM_USER_DEFINED ) || ( g_rom.user_defined_cmthack_type == ROM_CMTHACK_DEFAULT ) ) {
            if ( enabled ) {
                cmthack_install_rom_patch ( );
            } else {
                memcpy ( &g_memory.ROM [ 0x04d8 ], &g_rom.mz700rom [ 0x04d8 ], 8 );
                memcpy ( &g_memory.ROM [ 0x04f8 ], &g_rom.mz700rom [ 0x04f8 ], 15 );
            };
            g_cmthack.load_patch_installed = enabled & 1;
        } else if ( g_rom.user_defined_cmthack_type == ROM_CMTHACK_CUSTOM ) {
            g_cmthack.load_patch_installed = enabled & 1;
            rom_reinstall ( ROMTYPE_USER_DEFINED );
        };
    };

    ui_cmt_hack_menu_update ( );
}


void cmthack_reset ( void ) {
    /* pokud jsme rozecteny MZF soubor, tak ho zavrem */
    generic_driver_close ( &g_cmthack.mzf_handler );
}


void cmthack_exit ( void ) {
    /* pokud jsme rozecteny MZF soubor, tak ho zavrem */
    generic_driver_close ( &g_cmthack.mzf_handler );
    if ( g_cmthack.last_filename ) {
        ui_utils_mem_free ( g_cmthack.last_filename );
    }
}


#define DEFAULT_CMT_HACK_ENABLE   1
#define DEFAULT_CMT_HACK_FIX_TERMINATOR   1


void cmthack_propagatecfg_load_rom_patch ( void *e, void *data ) {
    cmthack_load_rom_patch ( cfgelement_get_bool_value ( (CFGELM *) e ) );
}


void cmthack_init ( void ) {

    generic_driver_register_handler ( &g_cmthack.mzf_handler, CMT_HACK_DEFAULT_HANDLER_TYPE );
    generic_driver_set_handler_readonly_status ( &g_cmthack.mzf_handler, 1 );
    g_cmthack.last_filename = ui_utils_mem_alloc0 ( 1 );
    g_cmthack.load_patch_installed = 0;
    g_cmthack.fix_fname_terminator = 0;

    CFGMOD *cmod = cfgroot_register_new_module ( g_cfgmain, "CMTHACK" );

    CFGELM *elm;
    elm = cfgmodule_register_new_element ( cmod, "enable", CFGENTYPE_BOOL, DEFAULT_CMT_HACK_ENABLE );
    cfgelement_set_propagate_cb ( elm, cmthack_propagatecfg_load_rom_patch, NULL );
    cfgelement_set_handlers ( elm, NULL, (void*) &g_cmthack.load_patch_installed );

    elm = cfgmodule_register_new_element ( cmod, "fix_fname_terminator", CFGENTYPE_BOOL, DEFAULT_CMT_HACK_FIX_TERMINATOR );
    cfgelement_set_handlers ( elm, (void*) &g_cmthack.fix_fname_terminator, (void*) &g_cmthack.fix_fname_terminator );

    cfgmodule_parse ( cmod );
    cfgmodule_propagate ( cmod );

    ui_cmt_hack_menu_update ( );
}


void cmthack_result ( en_LOADRET result ) {

    Z80EX_WORD reg_af = z80ex_get_reg ( g_mz800.cpu, regAF );

    if ( result == LOADRET_OK ) {
        reg_af &= ~0x01; /* reset CARRY flag */
    } else {
        reg_af |= 0x01; /* set CARRY flag */
        reg_af |= ( (Z80EX_WORD) result << 8 ); /* result do regA */
        reg_af &= ( (Z80EX_WORD) result << 8 ) | 0xff;
    };
    z80ex_set_reg ( g_mz800.cpu, regAF, reg_af );
}


/* 
 * 
 * Pozadavek na precteni headeru z CMT
 * 
 */
void cmthack_load_file ( void ) {

    /* BUGFIX: po otevreni file selektoru se zakousne stav vkbd */
    ui_vkbd_reset_keyboard_state ( );

    mz800_flush_full_screen ( );

    char *filename = ui_file_chooser_open_cmthack_file ( g_cmthack.last_filename );

    if ( filename == NULL ) {
        /* Zruseno: nastavit Err + Break */
        cmthack_result ( LOADRET_BREAK );
        return;
    };

    const char *file_ext = cmtext_get_filename_extension ( filename );

    if ( ( 0 == strcasecmp ( file_ext, "mzf" ) ) || ( 0 == strcasecmp ( file_ext, "m12" ) ) ) {
        cmthack_load_mzf_filename ( filename );
    } else {
        ui_show_error ( "This file can't be open in CMTHACK.\nPlease, load this file by virtual CMT." );
        cmthack_result ( LOADRET_BREAK );
    };

    ui_utils_mem_free ( filename );
}


void cmthack_load_mzf_filename ( char *filename ) {

    unsigned reg_hl;

    /* pokud jsme rozecteny MZF soubor, tak ho zavrem */
    generic_driver_close ( &g_cmthack.mzf_handler );

    if ( ( NULL == generic_driver_open_memory_from_file ( &g_cmthack.mzf_handler, g_driver, filename ) ) || ( g_cmthack.mzf_handler.err ) || ( g_driver->err ) ) {
        /* Nejde otevrit soubor, nebo inicializovat handler: nastavit Err + Break */
        ui_show_error ( "CMT HACK can't open MZF file '%s': %s, gdriver_err: %s\n", filename, strerror ( errno ), generic_driver_error_message ( &g_cmthack.mzf_handler, g_cmthack.mzf_handler.driver ) );
        cmthack_result ( LOADRET_BREAK );
        return;
    };

    g_cmthack.last_filename = ui_utils_mem_realloc ( g_cmthack.last_filename, strlen ( filename ) + 1 );
    strncpy ( g_cmthack.last_filename, filename, strlen ( filename ) + 1 );

    /* precteme prvnich 128 bajtu z MZF souboru a ulozime je do RAM na adresu z regHL */

    reg_hl = z80ex_get_reg ( g_mz800.cpu, regHL );

    uint8_t buff[sizeof ( st_MZF_HEADER )];
    if ( EXIT_SUCCESS != generic_driver_read ( &g_cmthack.mzf_handler, 0, &buff, sizeof ( buff ) ) ) {
        /* Vadny soubor: nastavit Err + Checksum */
        ui_show_error ( "CMT HACK can't load MZF header '%s': %s, gdriver_err: %s\n", filename, strerror ( errno ), generic_driver_error_message ( &g_cmthack.mzf_handler, g_cmthack.mzf_handler.driver ) );
        generic_driver_close ( &g_cmthack.mzf_handler );
        cmthack_result ( LOADRET_ERROR );
        return;
    };
    memory_load_block ( buff, reg_hl, sizeof ( st_MZF_HEADER ), MEMORY_LOAD_MAPED );

    int test_fname_term = mzf_header_test_fname_terminator ( &g_cmthack.mzf_handler );

    st_MZF_HEADER mzfhdr;

    if ( EXIT_SUCCESS == mzf_read_header ( &g_cmthack.mzf_handler, &mzfhdr ) ) {
        char ascii_filename[MZF_FNAME_FULL_LENGTH];
        mzf_tools_get_fname ( &mzfhdr, ascii_filename );
        printf ( "\nCMT hack: load MZF header on 0x%04x.\n\n", reg_hl );
        printf ( "fname: %s", ascii_filename );
        if ( test_fname_term == EXIT_FAILURE ) {
            uint8_t buff = MZF_FNAME_TERMINATOR;
            if ( g_cmthack.fix_fname_terminator ) {
                memory_load_block ( &buff, ( reg_hl + sizeof ( st_MZF_FILENAME ) - 2 ), 1, MEMORY_LOAD_MAPED );
            }
            printf ( " (!mising 0x0d%s)", ( g_cmthack.fix_fname_terminator ) ? " - fixed" : "" );
        }
        printf ( "\n" );
        printf ( "ftype: 0x%02x\n", mzfhdr.ftype );
        printf ( "fstrt: 0x%04x\n", mzfhdr.fstrt );
        printf ( "fsize: 0x%04x\n", mzfhdr.fsize );
        printf ( "fexec: 0x%04x\n", mzfhdr.fexec );

        if ( TEST_CMT_MZFSIZE_CHECK_ENABLED ) {
            int mzf_size = mzfhdr.fsize + sizeof ( st_MZF_HEADER );
            ui_cmt_check_mzf_filesize ( filename, mzf_size );
        };

    } else {
        ui_show_error ( "CMT HACK can't read MZF header '%s': %s, gdriver_err: %s\n", filename, strerror ( errno ), mzf_error_message ( &g_cmthack.mzf_handler, g_cmthack.mzf_handler.driver ) );
        generic_driver_close ( &g_cmthack.mzf_handler );
        cmthack_result ( LOADRET_ERROR );
        return;
    };

    /* Nacteno OK */
    cmthack_result ( LOADRET_OK );
}


/* 
 * 
 * Pozadavek na precteni tela souboru
 * 
 */
void cmthack_read_mzf_body ( void ) {

    unsigned reg_hl;
    unsigned reg_bc;

    /* Mame otevren nejaky MZF? */
    if ( !( g_cmthack.mzf_handler.status & HANDLER_STATUS_READY ) ) {
        /* Zruseno: nastavit Err + Break */
        cmthack_result ( LOADRET_BREAK );
        return;
    };

    /* precteme prvnich regBC bajtu z MZF souboru a ulozime je do RAM na adresu z regHL */
    reg_hl = z80ex_get_reg ( g_mz800.cpu, regHL );
    reg_bc = z80ex_get_reg ( g_mz800.cpu, regBC );

    /* precteme prvnich regBC bajtu z MZF body a ulozime je do RAM na adresu z regHL */
    uint8_t data[0x10000];
    if ( EXIT_SUCCESS != mzf_read_body ( &g_cmthack.mzf_handler, data, reg_bc ) ) {
        /* Vadny soubor: nastavit Err + Checksum */
        ui_show_error ( "CMT HACK can't load MZF header '%s': %s, gdriver_err: %s\n", g_cmthack.last_filename, strerror ( errno ), mzf_error_message ( &g_cmthack.mzf_handler, g_cmthack.mzf_handler.driver ) );
        generic_driver_close ( &g_cmthack.mzf_handler );
        cmthack_result ( LOADRET_ERROR );
        return;
    };

    memory_load_block ( data, reg_hl, reg_bc, MEMORY_LOAD_MAPED );

    printf ( "\nCMT hack: load 0x%04x bytes from MZF body to addr 0x%04x.\n", reg_bc, reg_hl );

    /* Nacteno OK */
    generic_driver_close ( &g_cmthack.mzf_handler );
    cmthack_result ( LOADRET_OK );
}

