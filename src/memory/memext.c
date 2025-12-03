/* 
 * File:   memext.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 17. července 2018, 20:05
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
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "ui/ui_main.h"
#include "ui/ui_utils.h"
#include "fs_layer.h"
#include "memext.h"
#include "memory.h"
#include "ui/ui_memext.h"

#include "cfgmain.h"


st_MEMEXT g_memext;


void memext_reset ( void ) {
    if ( ( TEST_MEMEXT_TYPE_LUFTNER ) && ( !TEST_MEMEXT_LUFTNER_AUTO_INIT ) ) return;
    int i;
    for ( i = 0; i < MEMEXT_RAW_MAP_SIZE; i++ ) {
        g_memext.map[i] = i;
    };
}


void memext_flash_reload ( void ) {
    if ( !TEST_MEMEXT_CONNECTED_LUFTNER ) return;
    if ( ( g_memext.flash_filepath ) && ( ui_utils_file_access ( g_memext.flash_filepath, F_OK ) != -1 ) ) {
        FILE *fh;
        FS_LAYER_FOPEN ( fh, g_memext.flash_filepath, FS_LAYER_FMODE_RO );
        if ( fh ) {
            uint32_t readlen = 0;
            FS_LAYER_FREAD ( fh, g_memext.FLASH, sizeof (g_memext.FLASH ), &readlen );
            FS_LAYER_FCLOSE ( fh );
        } else {
            char *filepath_locale = ui_utils_file_name_locale_from_utf8 ( g_memext.flash_filepath );
            fprintf ( stderr, "%s():%d - Can't open file '%s'\n", __func__, __LINE__, filepath_locale );
            ui_utils_mem_free ( filepath_locale );
        };
    };
#ifdef MZ800EMU_CFG_DEBUGGER_ENABLED
    ui_main_debugger_windows_refresh ( );
#endif
}


static void memext_init_luftner ( void ) {
    int i;
    for ( i = 0; i < MEMEXT_RAW_MAP_SIZE; i++ ) {
        g_memext.map[i] = rand ( ) % 0xff;
    };

    memset ( g_memext.FLASH, 0xff, sizeof (g_memext.FLASH ) );

    if ( TEST_MEMEXT_CONNECTED ) {
        memext_flash_reload ( );
#ifdef MZ800EMU_CFG_DEBUGGER_ENABLED
        ui_main_debugger_windows_refresh ( );
#endif
    };
}


void memext_connect ( en_MEMEXT_TYPE type ) {
    g_memext.connection = MEMEXT_CONNECTION_YES;
    g_memext.type = type;
    if ( TEST_MEMEXT_TYPE_LUFTNER ) memext_init_luftner ( );
    memext_reset ( );
    memory_reconnect_ram ( );
    ui_memext_menu_update ( );
}


void memext_disconnect ( void ) {
    g_memext.connection = MEMEXT_CONNECTION_NO;
    memory_reconnect_ram ( );
    ui_memext_menu_update ( );
}


void memext_init ( void ) {

    CFGMOD *cmod = cfgroot_register_new_module ( g_cfgmain, "MEMEXT" );
    CFGELM *elm;

    elm = cfgmodule_register_new_element ( cmod, "connected", CFGENTYPE_BOOL, MEMEXT_CONNECTION_NO );
    cfgelement_set_handlers ( elm, (void*) &g_memext.connection, (void*) &g_memext.connection );

    elm = cfgmodule_register_new_element ( cmod, "type", CFGENTYPE_KEYWORD, MEMEXT_TYPE_PEHU,
                                           MEMEXT_TYPE_PEHU, "PEHU",
                                           MEMEXT_TYPE_LUFTNER, "LUFTNER",
                                           -1 );
    cfgelement_set_handlers ( elm, (void*) &g_memext.type, (void*) &g_memext.type );

    elm = cfgmodule_register_new_element ( cmod, "flash_filepath", CFGENTYPE_TEXT, MEMEXT_DEFAULT_FLASH_FNAME );
    cfgelement_set_pointers ( elm, (void*) &g_memext.flash_filepath, (void*) &g_memext.flash_filepath );

    elm = cfgmodule_register_new_element ( cmod, "luftner_force_init", CFGENTYPE_BOOL, MEMEXT_CONNECTION_YES );
    cfgelement_set_handlers ( elm, (void*) &g_memext.init_luftner, (void*) &g_memext.init_luftner );

    elm = cfgmodule_register_new_element ( cmod, "filling_on_init", CFGENTYPE_KEYWORD, MEMEXT_INIT_MEM_NULL,
                                           MEMEXT_INIT_MEM_NULL, "NULL",
                                           MEMEXT_INIT_MEM_RANDOM, "RANDOM",
                                           MEMEXT_INIT_MEM_SHARP, "SHARP",
                                           -1 );
    cfgelement_set_handlers ( elm, (void*) &g_memext.init_mem, (void*) &g_memext.init_mem );

    cfgmodule_parse ( cmod );
    cfgmodule_propagate ( cmod );

    srand ( time ( NULL ) );


    if ( g_memext.init_mem == MEMEXT_INIT_MEM_NULL ) {
        memset ( g_memext.RAM, 0x00, sizeof (g_memext.RAM ) );
    } else if ( g_memext.init_mem == MEMEXT_INIT_MEM_SHARP ) {
        uint32_t i;
        uint16_t *addr;

        for ( i = 0; i < ( MEMEXT_RAM_SIZE - 1 ); i += 4 ) {
            addr = ( uint16_t* ) & g_memext.RAM [ i ];
            *addr++ = 0xffff;
            *addr = 0x0000;
        };
    } else {
        int i;
        for ( i = 0; i < MEMEXT_RAM_SIZE; i++ ) {
            g_memext.RAM[i] = rand ( ) % 0xff;
        };
    };

    if ( TEST_MEMEXT_TYPE_LUFTNER ) memext_init_luftner ( );

    memext_reset ( );

    ui_memext_menu_update ( );
}


void memext_map_pwrite ( int addr_point, uint8_t value ) {

    //printf ( "point: 0x%02x, value: %d\n", addr_point, value );

    if ( TEST_MEMEXT_TYPE_LUFTNER ) {
        g_memext.map[addr_point] = value;
    } else if ( TEST_MEMEXT_TYPE_PEHU ) {
        int ap = addr_point & 0xfe;
        int rawbank = ( value & MEMEXT_PEHU_MASK ) * 2;
        //printf ( "PEHU point: 0x%02x, bank: %d\n", ap, rawbank );
        g_memext.map[ap] = rawbank;
        g_memext.map[( ap + 1 )] = rawbank + 1;
    };

    memory_reconnect_ram ( );
}


static uint8_t* memext_get_ram_read_pointer_by_rawbank ( int rawbank ) {
    if ( rawbank & 0x80 ) {
        return &g_memext.FLASH[( ( rawbank & 0x7f ) * MEMEXT_RAW_BANK_SIZE )];
    };
    return &g_memext.RAM[( rawbank * MEMEXT_RAW_BANK_SIZE )];
}


uint8_t* memext_get_ram_read_pointer_by_addr_point ( int addr_point ) {
    return memext_get_ram_read_pointer_by_rawbank ( g_memext.map[addr_point] );
}


static uint8_t* memext_get_ram_write_pointer_by_rawbank ( int rawbank ) {
    if ( rawbank & 0x80 ) {
        return g_memext.WOM;
    };
    return &g_memext.RAM[( rawbank * MEMEXT_RAW_BANK_SIZE )];
}


uint8_t* memext_get_ram_write_pointer_by_addr_point ( int addr_point ) {
    return memext_get_ram_write_pointer_by_rawbank ( g_memext.map[addr_point] );
}
