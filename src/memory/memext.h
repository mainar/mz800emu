/* 
 * File:   memext.h
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 17. července 2018, 20:02
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


#ifndef MEMEXT_H
#define MEMEXT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define MEMEXT_RAM_SIZE             0x80000
#define MEMEXT_FLASH_SIZE           0x80000

#define MEMEXT_LUFTNER_BANKS        0x80
#define MEMEXT_LUFTNER_BANK_MASK    0xff
#define MEMEXT_LUFTNER_BANK_SIZE    0x1000
#define MEMEXT_LUFTNER_ADDR_MASK    0x0fff

#define MEMEXT_PEHU_BANKS           0x40
#define MEMEXT_PEHU_MASK            0x3f
#define MEMEXT_PEHU_BANK_SIZE       0x2000
#define MEMEXT_PEHU_ADDR_MASK       0x1fff

#define MEMEXT_RAW_MAP_SIZE         0x10
#define MEMEXT_RAW_BANK_SIZE        0x1000

#define MEMEXT_DEFAULT_FLASH_FNAME  "flash.dat"


    typedef enum en_MEMEXT_CONNECTION {
        MEMEXT_CONNECTION_NO = 0,
        MEMEXT_CONNECTION_YES = 1,
    } en_MEMEXT_CONNECTION;


    typedef enum en_MEMEXT_TYPE {
        MEMEXT_TYPE_LUFTNER = 0,
        MEMEXT_TYPE_PEHU,
    } en_MEMEXT_TYPE;


    typedef enum en_MEMEXT_INIT_MEM {
        MEMEXT_INIT_MEM_NULL = 0,
        MEMEXT_INIT_MEM_RANDOM,
        MEMEXT_INIT_MEM_SHARP,
    } en_MEMEXT_INIT_MEM;


    typedef enum en_MEMEXT_INIT_LUFTNER {
        MEMEXT_INIT_LUFTNER_NONE = 0,
        MEMEXT_INIT_LUFTNER_RESET,
    } en_MEMEXT_INIT_LUFTNER;


    typedef struct st_MEMEXT {
        en_MEMEXT_CONNECTION connection;
        en_MEMEXT_TYPE type;
        en_MEMEXT_INIT_MEM init_mem;
        en_MEMEXT_INIT_LUFTNER init_luftner;
        char *flash_filepath;
        uint32_t map [ MEMEXT_RAW_MAP_SIZE ];
        uint16_t addr_mask;
        uint8_t RAM [ MEMEXT_RAM_SIZE ];
        uint8_t FLASH [ MEMEXT_FLASH_SIZE ];
        uint8_t WOM [ MEMEXT_RAW_BANK_SIZE ]; // Write Only Memory :-)
    } st_MEMEXT;

    extern st_MEMEXT g_memext;

    extern void memext_init ( void );
    extern void memext_reset ( void );

    extern void memext_connect ( en_MEMEXT_TYPE type );
    extern void memext_disconnect ( void );

    extern void memext_flash_reload ( void );

    extern void memext_map_pwrite ( int addr_point, uint8_t value );

    extern uint8_t* memext_get_ram_read_pointer_by_addr_point ( int addr_point );
    extern uint8_t* memext_get_ram_write_pointer_by_addr_point ( int addr_point );

#define TEST_MEMEXT_TYPE_LUFTNER   ( g_memext.type == MEMEXT_TYPE_LUFTNER )
#define TEST_MEMEXT_TYPE_PEHU   ( g_memext.type == MEMEXT_TYPE_PEHU )

#define TEST_MEMEXT_CONNECTED   ( g_memext.connection == MEMEXT_CONNECTION_YES )
#define TEST_MEMEXT_CONNECTED_PEHU  ( TEST_MEMEXT_CONNECTED &&  TEST_MEMEXT_TYPE_PEHU )
#define TEST_MEMEXT_CONNECTED_LUFTNER  ( TEST_MEMEXT_CONNECTED &&  TEST_MEMEXT_TYPE_LUFTNER )

#define TEST_MEMEXT_LUFTNER_AUTO_INIT   ( g_memext.init_luftner == MEMEXT_INIT_LUFTNER_RESET )

#ifdef __cplusplus
}
#endif

#endif /* MEMEXT_H */

