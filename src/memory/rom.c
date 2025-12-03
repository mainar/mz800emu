/* 
 * File:   rom.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 22. února 2016, 18:30
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
#include <string.h>
#include <unistd.h>


#include "z80ex/include/z80ex.h"

#include "rom.h"
#include "memory.h"
#include "cmt/cmthack.h"

#include "cfgmain.h"

#include "ui/ui_rom.h"
#include "ui/ui_utils.h"
#include "fs_layer.h"

st_ROM g_rom;


static void rom_install_predefined ( Z80EX_BYTE *mz700rom, Z80EX_BYTE *cgrom, Z80EX_BYTE *mz800rom ) {
    Z80EX_BYTE *ROM;

    ROM = g_memory.ROM;
    memcpy ( ROM, mz700rom, ROM_SIZE_MZ700 );
    g_rom.mz700rom = mz700rom;

    ROM += sizeof (Z80EX_BYTE ) * ROM_SIZE_CGROM;
    memcpy ( ROM, cgrom, ROM_SIZE_CGROM );
    g_rom.cgrom = cgrom;

    ROM += sizeof (Z80EX_BYTE ) * ROM_SIZE_CGROM;
    memcpy ( ROM, mz800rom, ROM_SIZE_MZ800 );
    g_rom.mz800rom = mz800rom;
}


static int rom_user_defined_load_file ( void *dst, char *filepath, uint32_t size ) {
    if ( ( !filepath ) || ( strlen ( filepath ) == 0 ) ) {
        fprintf ( stderr, "%s():%d - Empty ROM file name\n", __func__, __LINE__ );
        return EXIT_FAILURE;
    };
    FILE *fh;
    FS_LAYER_FOPEN ( fh, filepath, FS_LAYER_FMODE_RO );
    if ( !fh ) {
        fprintf ( stderr, "%s():%d - Cant open file '%s'\n", __func__, __LINE__, filepath );
        return EXIT_FAILURE;
    };
    int ret = EXIT_SUCCESS;
    uint32_t readlen = 0;
    if ( FS_LAYER_FR_OK != FS_LAYER_FREAD ( fh, dst, size, &readlen ) ) {
        fprintf ( stderr, "%s():%d - Can't read %d bytes from '%s'\n", __func__, __LINE__, size, filepath );
        ret = EXIT_FAILURE;
    };
    FS_LAYER_FCLOSE ( fh );
    return ret;
}


int rom_user_defined_rom_area_load ( st_ROM_AREA *dst, en_ROM_BOOL allinone, char *allinone_fp, char *mz700_fp, char *cgrom_fp, char *mz800_fp ) {
    if ( allinone == ROM_BOOL_YES ) return rom_user_defined_load_file ( dst, allinone_fp, ROM_SIZE_TOTAL );
    if ( EXIT_SUCCESS != rom_user_defined_load_file ( &dst->mz700rom, mz700_fp, ROM_SIZE_MZ700 ) ) return EXIT_FAILURE;
    if ( EXIT_SUCCESS != rom_user_defined_load_file ( &dst->cgrom, cgrom_fp, ROM_SIZE_CGROM ) ) return EXIT_FAILURE;
    return rom_user_defined_load_file ( &dst->mz800rom, mz800_fp, ROM_SIZE_MZ800 );
}


static void rom_cmthack_is_not_compatibile ( void ) {
    if ( TEST_CMTHACK_INSTALLED ) {
        printf ( "CMTHACK is not compatibile with selected ROM - CMTHACK DISABLED\n" );
        cmthack_load_rom_patch ( 0 );
    };
}


static void rom_install ( en_ROMTYPE romtype ) {

    en_ROM_BOOL custom_rom_error = FALSE;

    switch ( romtype ) {

        case ROMTYPE_STANDARD:
            rom_install_predefined ( (Z80EX_BYTE*) c_ROM_MZ700, c_ROM_CGROM, c_ROM_MZ800 );
            break;

        case ROMTYPE_JSS103:
            rom_install_predefined ( c_ROM_JSS103_MZ700, c_ROM_JSS103_CGROM, c_ROM_JSS103_MZ800 );
            break;

        case ROMTYPE_JSS105C:
            rom_install_predefined ( c_ROM_JSS105C_MZ700, c_ROM_JSS105C_CGROM, c_ROM_JSS105C_MZ800 );
            break;

        case ROMTYPE_JSS106A:
            rom_install_predefined ( c_ROM_JSS106A_MZ700, c_ROM_JSS106A_CGROM, c_ROM_JSS106A_MZ800 );
            break;

        case ROMTYPE_JSS108C:
            rom_install_predefined ( c_ROM_JSS108C_MZ700, c_ROM_JSS108C_CGROM, c_ROM_JSS108C_MZ800 );
            break;

        case ROMTYPE_WILLY_EN:
            rom_cmthack_is_not_compatibile ( );
            rom_install_predefined ( c_ROM_WILLY_MZ700, c_ROM_WILLY_en_CGROM, c_ROM_WILLY_en_MZ800 );
            break;

        case ROMTYPE_WILLY_GE:
            rom_cmthack_is_not_compatibile ( );
            rom_install_predefined ( c_ROM_WILLY_MZ700, c_ROM_WILLY_ge_CGROM, c_ROM_WILLY_ge_MZ800 );
            break;

        case ROMTYPE_WILLY_JAP:
            rom_cmthack_is_not_compatibile ( );
            rom_install_predefined ( c_ROM_WILLY_MZ700, c_ROM_WILLY_jap_CGROM, c_ROM_WILLY_jap_MZ800 );
            break;

        case ROMTYPE_USER_DEFINED:
            if ( g_rom.user_defined_rom_loaded != ROM_BOOL_YES ) {
                st_ROM_AREA rom;
                st_ROM_AREA rom_cmthack;

                if ( EXIT_SUCCESS != rom_user_defined_rom_area_load ( &rom, g_rom.user_defined_allinone, g_rom.user_defined_allinone_fp, g_rom.user_defined_mz700_fp, g_rom.user_defined_cgrom_fp, g_rom.user_defined_mz800_fp ) ) {
                    printf ( "Can't install user defined ROM!\n" );
                    custom_rom_error = TRUE;
                    break;
                };

                if ( g_rom.user_defined_cmthack_type == ROM_CMTHACK_CUSTOM ) {
                    if ( EXIT_SUCCESS != rom_user_defined_rom_area_load ( &rom_cmthack, g_rom.user_defined_cmthack_allinone, g_rom.user_defined_cmthack_allinone_fp, g_rom.user_defined_cmthack_mz700_fp, g_rom.user_defined_cmthack_cgrom_fp, g_rom.user_defined_cmthack_mz800_fp ) ) {
                        printf ( "Can't install user defined cmthack ROM!\n" );
                        custom_rom_error = TRUE;
                        break;
                    };
                };

                g_rom.user_defined_rom_loaded = ROM_BOOL_YES;
                memcpy ( &g_rom.rom_user_defined, &rom, sizeof ( st_ROM_AREA ) );
                memcpy ( &g_rom.rom_user_defined_cmthack, &rom_cmthack, sizeof ( st_ROM_AREA ) );
            };

            if ( g_rom.user_defined_cmthack_type == ROM_CMTHACK_DISABLED ) {
                rom_cmthack_is_not_compatibile ( );
            };

            if ( g_cmthack.load_patch_installed ) {
                rom_install_predefined ( g_rom.rom_user_defined_cmthack.mz700rom, g_rom.rom_user_defined_cmthack.cgrom, g_rom.rom_user_defined_cmthack.mz800rom );
            } else {
                rom_install_predefined ( g_rom.rom_user_defined.mz700rom, g_rom.rom_user_defined.cgrom, g_rom.rom_user_defined.mz800rom );
            };
            break;

        default:
            printf ( "Unsupported ROM type! %d\n", romtype );
            rom_install_predefined ( c_ROM_MZ700, c_ROM_CGROM, c_ROM_MZ800 );
            romtype = ROMTYPE_STANDARD;
            break;
    };

    if ( custom_rom_error ) {
        rom_install_predefined ( c_ROM_MZ700, c_ROM_CGROM, c_ROM_MZ800 );
        g_rom.type = ROMTYPE_STANDARD;
        ui_rom_menu_update ( );
        ui_rom_settings_open_window ( );
    } else {
        g_rom.type = romtype;
        ui_rom_menu_update ( );
    };
}


void rom_reinstall ( en_ROMTYPE romtype ) {
    rom_install ( romtype );
    cmthack_reinstall_rom_patch ( );
}


int rom_user_defined_check_filepath ( char **filepath, en_ROM_BOOL clear ) {
    if ( ui_utils_file_access ( *filepath, F_OK ) != -1 ) return EXIT_SUCCESS;
    if ( ROM_BOOL_YES == clear ) {
        *filepath = (char*) ui_utils_mem_realloc ( *filepath, 1 );
        *filepath[0] = 0x00;
    };
    return EXIT_FAILURE;
}


int rom_user_defined_check_size ( char **filepath, uint32_t size, en_ROM_BOOL clear ) {
    FILE *fh;
    FS_LAYER_FOPEN ( fh, *filepath, FS_LAYER_FMODE_RO );
    if ( !fh ) {
        if ( ROM_BOOL_YES == clear ) {
            *filepath = (char*) ui_utils_mem_realloc ( *filepath, 1 );
            *filepath[0] = 0x00;
        };
        return EXIT_FAILURE;
    };
    FS_LAYER_FSEEK_END ( fh, 0 );
    uint32_t file_size = FS_LAYER_FTELL ( fh );
    FS_LAYER_FCLOSE ( fh );
    if ( file_size < size ) {
        if ( ROM_BOOL_YES == clear ) {
            *filepath = (char*) ui_utils_mem_realloc ( *filepath, 1 );
            *filepath[0] = 0x00;
        };
        return EXIT_FAILURE;
    };
    return EXIT_SUCCESS;
}


void rom_set_user_defined_filepath ( char **dst, char *src ) {
    int len = 1;
    if ( src ) {
        len = strlen ( src ) + 1;
    };
    *dst = ui_utils_mem_realloc ( *dst, len );
    if ( len == 1 ) {
        *dst[0] = 0x00;
    } else {
        strncpy ( *dst, src, len );
    };
}


void rom_init ( void ) {

    g_rom.user_defined_rom_loaded = ROM_BOOL_NO;

    CFGMOD *cmod = cfgroot_register_new_module ( g_cfgmain, "ROM" );

    CFGELM *elm_rom_type;

    if ( g_mz800.development_mode == DEVELMODE_YES ) {
        elm_rom_type = cfgmodule_register_new_element ( cmod, "rom_type", CFGENTYPE_KEYWORD, ROMTYPE_STANDARD,
                                                        ROMTYPE_STANDARD, "STANDARD",
                                                        ROMTYPE_JSS103, "JSS103",
                                                        ROMTYPE_JSS105C, "JSS105C",
                                                        ROMTYPE_JSS106A, "JSS106A",
                                                        ROMTYPE_JSS108C, "JSS108C",
                                                        ROMTYPE_WILLY_EN, "WILLY_EN",
                                                        ROMTYPE_WILLY_GE, "WILLY_GE",
                                                        ROMTYPE_WILLY_JAP, "WILLY_JAP",
                                                        ROMTYPE_USER_DEFINED, "USER_DEFINED",
                                                        -1 );
    } else {
        elm_rom_type = cfgmodule_register_new_element ( cmod, "rom_type", CFGENTYPE_KEYWORD, ROMTYPE_STANDARD,
                                                        ROMTYPE_STANDARD, "STANDARD",
                                                        ROMTYPE_JSS106A, "JSS106A",
                                                        ROMTYPE_JSS108C, "JSS108C",
                                                        ROMTYPE_WILLY_EN, "WILLY_EN",
                                                        ROMTYPE_WILLY_GE, "WILLY_GE",
                                                        ROMTYPE_WILLY_JAP, "WILLY_JAP",
                                                        ROMTYPE_USER_DEFINED, "USER_DEFINED",
                                                        -1 );
    };

    cfgelement_set_handlers ( elm_rom_type, (void*) &g_rom.type, (void*) &g_rom.type );

    CFGELM *elm;

    elm = cfgmodule_register_new_element ( cmod, "user_defined_allinone", CFGENTYPE_BOOL, ROM_BOOL_YES );
    cfgelement_set_handlers ( elm, (void*) &g_rom.user_defined_allinone, (void*) &g_rom.user_defined_allinone );

    elm = cfgmodule_register_new_element ( cmod, "user_defined_allinone_filepath", CFGENTYPE_TEXT, "" );
    cfgelement_set_pointers ( elm, (void*) &g_rom.user_defined_allinone_fp, (void*) &g_rom.user_defined_allinone_fp );

    elm = cfgmodule_register_new_element ( cmod, "user_defined_mz700_filepath", CFGENTYPE_TEXT, "" );
    cfgelement_set_pointers ( elm, (void*) &g_rom.user_defined_mz700_fp, (void*) &g_rom.user_defined_mz700_fp );

    elm = cfgmodule_register_new_element ( cmod, "user_defined_cgrom_filepath", CFGENTYPE_TEXT, "" );
    cfgelement_set_pointers ( elm, (void*) &g_rom.user_defined_cgrom_fp, (void*) &g_rom.user_defined_cgrom_fp );

    elm = cfgmodule_register_new_element ( cmod, "user_defined_mz800_filepath", CFGENTYPE_TEXT, "" );
    cfgelement_set_pointers ( elm, (void*) &g_rom.user_defined_mz800_fp, (void*) &g_rom.user_defined_mz800_fp );


    elm = cfgmodule_register_new_element ( cmod, "user_defined_cmthack", CFGENTYPE_KEYWORD, ROM_CMTHACK_DISABLED,
                                           ROM_CMTHACK_DISABLED, "DISABLED",
                                           ROM_CMTHACK_DEFAULT, "DEFAULT",
                                           ROM_CMTHACK_CUSTOM, "CUSTOM",
                                           -1 );
    cfgelement_set_handlers ( elm, (void*) &g_rom.user_defined_cmthack_type, (void*) &g_rom.user_defined_cmthack_type );

    elm = cfgmodule_register_new_element ( cmod, "user_defined_cmthack_allinone", CFGENTYPE_BOOL, ROM_BOOL_YES );
    cfgelement_set_handlers ( elm, (void*) &g_rom.user_defined_cmthack_allinone, (void*) &g_rom.user_defined_cmthack_allinone );

    elm = cfgmodule_register_new_element ( cmod, "user_defined_cmthack_allinone_filepath", CFGENTYPE_TEXT, "" );
    cfgelement_set_pointers ( elm, (void*) &g_rom.user_defined_cmthack_allinone_fp, (void*) &g_rom.user_defined_cmthack_allinone_fp );

    elm = cfgmodule_register_new_element ( cmod, "user_defined_cmthack_mz700_filepath", CFGENTYPE_TEXT, "" );
    cfgelement_set_pointers ( elm, (void*) &g_rom.user_defined_cmthack_mz700_fp, (void*) &g_rom.user_defined_cmthack_mz700_fp );

    elm = cfgmodule_register_new_element ( cmod, "user_defined_cmthack_cgrom_filepath", CFGENTYPE_TEXT, "" );
    cfgelement_set_pointers ( elm, (void*) &g_rom.user_defined_cmthack_cgrom_fp, (void*) &g_rom.user_defined_cmthack_cgrom_fp );

    elm = cfgmodule_register_new_element ( cmod, "user_defined_cmthack_mz800_filepath", CFGENTYPE_TEXT, "" );
    cfgelement_set_pointers ( elm, (void*) &g_rom.user_defined_cmthack_mz800_fp, (void*) &g_rom.user_defined_cmthack_mz800_fp );

    cfgmodule_parse ( cmod );
    cfgmodule_propagate ( cmod );

    if ( strlen ( g_rom.user_defined_allinone_fp ) ) rom_user_defined_check_filepath ( &g_rom.user_defined_allinone_fp, ROM_BOOL_YES );
    if ( strlen ( g_rom.user_defined_mz700_fp ) ) rom_user_defined_check_filepath ( &g_rom.user_defined_mz700_fp, ROM_BOOL_YES );
    if ( strlen ( g_rom.user_defined_cgrom_fp ) ) rom_user_defined_check_filepath ( &g_rom.user_defined_cgrom_fp, ROM_BOOL_YES );
    if ( strlen ( g_rom.user_defined_mz800_fp ) ) rom_user_defined_check_filepath ( &g_rom.user_defined_mz800_fp, ROM_BOOL_YES );
    if ( strlen ( g_rom.user_defined_cmthack_allinone_fp ) ) rom_user_defined_check_filepath ( &g_rom.user_defined_cmthack_allinone_fp, ROM_BOOL_YES );
    if ( strlen ( g_rom.user_defined_cmthack_mz700_fp ) ) rom_user_defined_check_filepath ( &g_rom.user_defined_cmthack_mz700_fp, ROM_BOOL_YES );
    if ( strlen ( g_rom.user_defined_cmthack_cgrom_fp ) ) rom_user_defined_check_filepath ( &g_rom.user_defined_cmthack_cgrom_fp, ROM_BOOL_YES );
    if ( strlen ( g_rom.user_defined_cmthack_mz800_fp ) ) rom_user_defined_check_filepath ( &g_rom.user_defined_cmthack_mz800_fp, ROM_BOOL_YES );

    rom_install ( (en_ROMTYPE) cfgelement_get_keyword_value ( elm_rom_type ) );
}
