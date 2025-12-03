/* 
 * File:   rom.h
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

#ifndef ROM_H
#define ROM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "z80ex/include/z80ex.h"

#define ROM_SIZE_MZ700 0x1000
#define ROM_SIZE_CGROM 0x1000
#define ROM_SIZE_MZ800 0x2000

#define ROM_SIZE_TOTAL  ROM_SIZE_MZ700 + ROM_SIZE_CGROM + ROM_SIZE_MZ800


    extern Z80EX_BYTE c_ROM_MZ700 [];
    extern Z80EX_BYTE c_ROM_CGROM [];
    extern Z80EX_BYTE c_ROM_MZ800 [];


    extern Z80EX_BYTE c_ROM_JSS103_MZ700 [];
    extern Z80EX_BYTE c_ROM_JSS103_CGROM [];
    extern Z80EX_BYTE c_ROM_JSS103_MZ800 [];


    extern Z80EX_BYTE c_ROM_JSS105C_MZ700 [];
    extern Z80EX_BYTE c_ROM_JSS105C_CGROM [];
    extern Z80EX_BYTE c_ROM_JSS105C_MZ800 [];


    extern Z80EX_BYTE c_ROM_JSS106A_MZ700 [];
    extern Z80EX_BYTE c_ROM_JSS106A_CGROM [];
    extern Z80EX_BYTE c_ROM_JSS106A_MZ800 [];

    extern Z80EX_BYTE c_ROM_JSS108C_MZ700 [];
    extern Z80EX_BYTE c_ROM_JSS108C_CGROM [];
    extern Z80EX_BYTE c_ROM_JSS108C_MZ800 [];

    extern Z80EX_BYTE c_ROM_WILLY_MZ700 [];

    extern Z80EX_BYTE c_ROM_WILLY_en_CGROM [];
    extern Z80EX_BYTE c_ROM_WILLY_en_MZ800 [];

    extern Z80EX_BYTE c_ROM_WILLY_ge_CGROM [];
    extern Z80EX_BYTE c_ROM_WILLY_ge_MZ800 [];

    extern Z80EX_BYTE c_ROM_WILLY_jap_CGROM [];
    extern Z80EX_BYTE c_ROM_WILLY_jap_MZ800 [];


    typedef enum en_ROMTYPE {
        ROMTYPE_STANDARD = 0,
        ROMTYPE_JSS103,
        ROMTYPE_JSS105C,
        ROMTYPE_JSS106A,
        ROMTYPE_JSS108C,
        ROMTYPE_WILLY_EN,
        ROMTYPE_WILLY_GE,
        ROMTYPE_WILLY_JAP,
        ROMTYPE_USER_DEFINED,
    } en_ROMTYPE;


    typedef enum en_ROM_BOOL {
        ROM_BOOL_NO = 0,
        ROM_BOOL_YES = 1
    } en_ROM_BOOL;


    typedef enum en_ROM_CMTHACK {
        ROM_CMTHACK_DISABLED = 0,
        ROM_CMTHACK_DEFAULT,
        ROM_CMTHACK_CUSTOM
    } en_ROM_CMTHACK;


    typedef struct st_ROM_AREA {
        Z80EX_BYTE mz700rom[ROM_SIZE_MZ700];
        Z80EX_BYTE cgrom[ROM_SIZE_CGROM];
        Z80EX_BYTE mz800rom[ROM_SIZE_MZ800];
    } st_ROM_AREA;


    typedef struct st_ROM {
        en_ROMTYPE type;
        Z80EX_BYTE *mz700rom;
        Z80EX_BYTE *cgrom;
        Z80EX_BYTE *mz800rom;
        en_ROM_BOOL user_defined_allinone;
        char *user_defined_allinone_fp;
        char *user_defined_mz700_fp;
        char *user_defined_cgrom_fp;
        char *user_defined_mz800_fp;
        en_ROM_CMTHACK user_defined_cmthack_type;
        en_ROM_BOOL user_defined_cmthack_allinone;
        char *user_defined_cmthack_allinone_fp;
        char *user_defined_cmthack_mz700_fp;
        char *user_defined_cmthack_cgrom_fp;
        char *user_defined_cmthack_mz800_fp;
        st_ROM_AREA rom_user_defined;
        st_ROM_AREA rom_user_defined_cmthack;
        en_ROM_BOOL user_defined_rom_loaded;
    } st_ROM;

    extern st_ROM g_rom;

    extern void rom_init ( void );
    extern void rom_reinstall ( en_ROMTYPE romtype );
    extern int rom_user_defined_check_filepath ( char **filepath, en_ROM_BOOL clear );
    extern int rom_user_defined_check_size ( char **filepath, uint32_t size, en_ROM_BOOL clear );
    extern void rom_set_user_defined_filepath ( char **dst, char *src );
    extern int rom_user_defined_rom_area_load ( st_ROM_AREA *dst, en_ROM_BOOL allinone, char *allinone_fp, char *mz700_fp, char *cgrom_fp, char *mz800_fp );

#define TEST_ROM_WILLY ( ( g_rom.type >= ROMTYPE_WILLY_EN ) && ( g_rom.type <= ROMTYPE_WILLY_JAP ) )
#define TEST_ROM_USER_DEFINED ( g_rom.type == ROMTYPE_USER_DEFINED )

#ifdef __cplusplus
}
#endif

#endif /* ROM_H */

