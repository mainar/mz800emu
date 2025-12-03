/* 
 * File:   memory.h
 * Author: chaky
 *
 * Created on 15. června 2015, 17:34
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

#ifndef MEMORY_H
#define MEMORY_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef MEMORY_MAKE_STATISTICS

#define MEMORY_STATISTIC_DAT_FILE   "memstats.dat"
#define MEMORY_STATISTIC_TXT_FILE   "memstats.txt"


    typedef struct st_MEMORY_STATS {
        unsigned long long read[16];
        unsigned long long write[16];
    } st_MEMORY_STATS;
#endif

#include "z80ex/include/z80ex.h"
#include "gdg/gdg.h"
#include "memory/rom.h"

    /*
     *
     * Velikost jednotlivych pameti
     * 
     */
#define MEMORY_SIZE_RAM         0x10000

#define MEMORY_SIZE_VRAM_BANK   0x2000
#define MEMORY_SIZE_VRAM        MEMORY_SIZE_VRAM_BANK * 2

#define MEMORY_MEMRAM_POINTS    0x10

    /*
     * 
     *  Flagy mapovani pameti
     *
     */
#define MEMORY_MAP_FLAG_ROM_0000 ( 1 << 0 )
#define MEMORY_MAP_FLAG_ROM_1000 ( 1 << 1 )
#define MEMORY_MAP_FLAG_CGRAM_VRAM ( 1 << 2 )  /* MZ700: 0xc000 - 0xcfff (CGRAM) 
                                                       MZ800: 0x8000 - 0x9fff | 0xbfff (VRAM) */
#define MEMORY_MAP_FLAG_ROM_E000 ( 1 << 3 )  /* + MZ700: 0xd000 - 0xdfff (atributova VRAM) */


    /* Memory map porty pro IORQ - PWRITE */
    typedef enum en_MMAP_PWRITE {
        MMAP_PWRITE_E0 = 0xe0, /* memory unmap ROM 0000 , CGROM */
        MMAP_PWRITE_E1 = 0xe1, /* memory unmap ROM E000, coz v MZ700 znamena i VRAM na D000 */
        MMAP_PWRITE_E2 = 0xe2, /* memory map ROM 0000 */
        MMAP_PWRITE_E3 = 0xe3, /* memory map ROM E000, coz v MZ700 znamena i VRAM na D000 */
        MMAP_PWRITE_E4 = 0xe4, /* memory map ROM 0000, ROM E000, MZ700: unmap CGROM, CGRAM, MZ800: map CGROM, VRAM */
        /* pozustatky z MZ-700 - v MZ-800 ponekud nefunkcni */
        MMAP_PWRITE_E5 = 0xe5, /* map EXROM */
        MMAP_PWRITE_E6 = 0xe6, /* unmap EXROM */
    } en_MMAP_PWRITE;


    /* Memory map porty pro IORQ - PREAD */
    typedef enum en_MMAP_PREAD {
        MMAP_PREAD_E0 = 0xe0, /* memory map CG-ROM, CG-RAM, VRAM - podle mode */
        MMAP_PREAD_E1 = 0xe1, /* memory unmap CG-ROM, CG-RAM, VRAM - podle mode */
    } en_MMAP_PREAD;


    /*
     * 
     *  Fyzicke umisteni vsech pameti
     *
     */
    typedef struct st_MEMORY {
        Z80EX_BYTE map;
        Z80EX_BYTE *memram_read [ MEMORY_MEMRAM_POINTS ];
        Z80EX_BYTE *memram_write [ MEMORY_MEMRAM_POINTS ];
        Z80EX_BYTE ROM [ ROM_SIZE_TOTAL ];
        Z80EX_BYTE RAM [ MEMORY_SIZE_RAM ];
        Z80EX_BYTE VRAM [ MEMORY_SIZE_VRAM ];
        Z80EX_BYTE EXVRAM [ MEMORY_SIZE_VRAM ];
    } st_MEMORY;


    extern st_MEMORY g_memory;


    extern uint8_t *g_memoryVRAM;
    extern uint8_t *g_memoryVRAM_I;
    extern uint8_t *g_memoryVRAM_II;
    extern uint8_t *g_memoryVRAM_III;
    extern uint8_t *g_memoryVRAM_IV;


    /*
     * 
     *  Testy mapovacich stavu
     *
     */
#define MEMORY_MAP_TEST_ROM_0000 ( g_memory.map & MEMORY_MAP_FLAG_ROM_0000 )
#define MEMORY_MAP_TEST_ROM_1000 ( g_memory.map & MEMORY_MAP_FLAG_ROM_1000 )
#define MEMORY_MAP_TEST_ROM_E000 ( g_memory.map & MEMORY_MAP_FLAG_ROM_E000 )
#define MEMORY_MAP_TEST_VRAM  ( g_memory.map & MEMORY_MAP_FLAG_CGRAM_VRAM )
#define MEMORY_MAP_TEST_CGRAM  ( DMD_TEST_MZ700 && MEMORY_MAP_TEST_VRAM )
#define MEMORY_MAP_TEST_VRAM_D000 ( DMD_TEST_MZ700 && MEMORY_MAP_TEST_ROM_E000 )

#define MEMORY_MAP_TEST_VRAM_8000 ( ( ! DMD_TEST_MZ700 ) && MEMORY_MAP_TEST_VRAM )
#define MEMORY_MAP_TEST_VRAM_A000 ( MEMORY_MAP_TEST_VRAM_8000 && DMD_TEST_SCRW640 )


    /*
     * 
     *  Definice IO operace pouzite pro mapovani
     *
     */
    typedef enum {
        MEMORY_MAPRQ_IOOP_IN = 0,
        MEMORY_MAP_IOOP_OUT
    } MEMORY_MAP_IOOP;



    /*
     * 
     *  Obsluzne funkce pameti
     *
     */


    /* Tepla inicializace pameti */
    extern void memory_reset ( void );

    /* Studena inicializace pameti */
    extern void memory_init ( void );

    /* Zmena pripojeni bank memextu */
    extern void memory_reconnect_ram ( void );

    /* Callback pro cteni bajtu */
    extern Z80EX_BYTE memory_read_cb ( Z80EX_CONTEXT *cpu, Z80EX_WORD addr, int m1_state, void *user_data );

    /* Callback pro cteni bajtu + debugovaci informace o vykonavanych bajtech */
    extern Z80EX_BYTE memory_read_with_history_cb ( Z80EX_CONTEXT *cpu, Z80EX_WORD addr, int m1_state, void *user_data );

    /* Callback pro zapis bajtu */
    extern void memory_write_cb ( Z80EX_CONTEXT *cpu, Z80EX_WORD addr, Z80EX_BYTE value, void *user_data );

    /* Nastaveni mapy pameti */
    extern void memory_map_pwrite ( en_MMAP_PWRITE mmap_port );
    extern void memory_map_pread ( en_MMAP_PREAD mmap_port );

    /* Cteni z aktualne mapovane pameti - bez synchronizace */
    extern Z80EX_BYTE memory_read_byte ( Z80EX_WORD addr );

    /* Zapis do aktualne mapovane pameti - bez synchronizace */
    extern void memory_write_byte ( Z80EX_WORD addr, Z80EX_BYTE value );


    typedef enum en_MEMORY_LOAD {
        MEMORY_LOAD_MAPED = 0,
        MEMORY_LOAD_RAMONLY,
    } en_MEMORY_LOAD;

    /* Nacteni datoveho bloku do pameti */
    extern void memory_load_block ( uint8_t *data, Z80EX_WORD addr, Z80EX_WORD size, en_MEMORY_LOAD type );


    static inline Z80EX_BYTE memory_pure_ram_read_byte ( Z80EX_WORD addr ) {
        //return g_memory.RAM [ addr ];
        return ( g_memory.memram_read[( addr >> 12 )] ) [ ( addr & 0x0fff ) ];
    }


    static inline int memory_test_addr_is_vram ( Z80EX_WORD addr ) {
        int a = addr >> 12;
        if (
             ( ( MEMORY_MAP_TEST_CGRAM ) && ( a == 0x0c ) ) ||
             ( ( MEMORY_MAP_TEST_VRAM_D000 ) && ( a == 0x0d ) ) ||
             ( ( MEMORY_MAP_TEST_VRAM_8000 ) && ( a == 0x08 ) ) ||
             ( ( MEMORY_MAP_TEST_VRAM_A000 ) && ( a == 0x0a ) )
             ) {
            return 1;
        };
        return 0;
    }

#ifdef MEMORY_MAKE_STATISTICS
    void memory_write_memory_statistics ( void );
#endif

#ifdef __cplusplus
}
#endif

#endif /* MEMORY_H */

