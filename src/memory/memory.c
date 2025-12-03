/* 
 * File:   memory.c
 * Author: chaky
 *
 * Created on 15. června 2015, 17:33
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

#include "z80ex/include/z80ex.h"

#include "mz800.h"
#include "memext.h"
#include "memory.h"
#include "rom.h"
#include "gdg/gdg.h"
#include "gdg/vramctrl.h"
#include "ctc8253/ctc8253.h"
#include "pio8255/pio8255.h"
#include "typedefs.h"

#ifdef MZ800EMU_CFG_DEBUGGER_ENABLED
#include "debugger/debugger.h"
#include "ui/ui_main.h"
#endif

#define DBGLEVEL (DBGNON /* | DBGERR | DBGWAR | DBGINF*/)
//#define DBGLEVEL (DBGNON | DBGERR | DBGWAR | DBGINF )
#include "debug.h"




st_MEMORY g_memory;

#ifdef MEMORY_MAKE_STATISTICS
st_MEMORY_STATS g_memory_statistics;
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "ui/ui_utils.h"
#endif

Z80EX_BYTE *g_memoryVRAM = g_memory.VRAM;
Z80EX_BYTE *g_memoryVRAM_I = g_memory.VRAM;
Z80EX_BYTE *g_memoryVRAM_II = &g_memory.VRAM [ MEMORY_SIZE_VRAM_BANK ];
Z80EX_BYTE *g_memoryVRAM_III = g_memory.EXVRAM;
Z80EX_BYTE *g_memoryVRAM_IV = &g_memory.EXVRAM [ MEMORY_SIZE_VRAM_BANK ];


#define MEMORY_ROM_READ_BYTE                g_memory.ROM [ addr & 0x3fff ]

//#define MEMORY_RAM_READ_BYTE                g_memory.RAM [ addr ]
//#define MEMORY_RAM_WRITE_BYTE               g_memory.RAM [ addr ] = value;
#define MEMORY_RAM_READ_BYTE                ( g_memory.memram_read[( addr >> 12 )] ) [ (addr & 0x0fff) ]
#define MEMORY_RAM_WRITE_BYTE               ( g_memory.memram_write[( addr >> 12 )] ) [ (addr & 0x0fff) ] = value;

#define MEMORY_VRAM_MZ700_READ_BYTE_SYNC    vramctrl_mz700_memop_read_byte_sync ( addr & ~0xe000 )
#define MEMORY_VRAM_MZ700_WRITE_BYTE_SYNC   vramctrl_mz700_memop_write_byte_sync ( addr & ~0xe000, value )

#define MEMORY_VRAM_MZ700_READ_BYTE         vramctrl_mz700_memop_read_byte ( addr & ~0xe000 )
#define MEMORY_VRAM_MZ700_WRITE_BYTE        vramctrl_mz700_memop_write_byte ( addr & ~0xe000, value )

#define MEMORY_VRAM_MZ800_READ_BYTE_SYNC         vramctrl_mz800_memop_read_byte_sync ( addr & 0x3fff )
#define MEMORY_VRAM_MZ800_WRITE_BYTE        vramctrl_mz800_memop_write_byte ( addr & 0x3fff, value )

#define MEMORY_VRAM_MZ800_READ_BYTE         vramctrl_mz800_memop_read_byte ( addr & 0x3fff )


/*******************************************************************************
 *
 * Mapovani pameti ROM_0000, ROM_E000, CG_ROM, CG_RAM, VRAM a RAM
 * 
 ******************************************************************************/


/**
 * OUT 0xE0 - memory unmap ROM 0000 , CGROM
 */
static inline void memory_mmap_rom_bottom_off ( void ) {
    DBGPRINTF ( DBGINF, "pwrite = 0xe0, PC = 0x%04x\n", g_mz800.instruction_addr );
    g_memory.map &= ~( MEMORY_MAP_FLAG_ROM_0000 | MEMORY_MAP_FLAG_ROM_1000 );
}


/**
 * OUT 0xE1 - memory unmap ROM E000, coz v MZ700 znamena i VRAM na D000
 */
static inline void memory_mmap_rom_upper_off ( void ) {
    DBGPRINTF ( DBGINF, "pwrite = 0xe1, PC = 0x%04x\n", g_mz800.instruction_addr );
    g_memory.map &= ~( MEMORY_MAP_FLAG_ROM_E000 );
}


/**
 * OUT 0xE2 - memory map ROM 0000
 */
static inline void memory_mmap_rom_0000_on ( void ) {
    DBGPRINTF ( DBGINF, "pwrite = 0xe2, PC = 0x%04x\n", g_mz800.instruction_addr );
    g_memory.map |= MEMORY_MAP_FLAG_ROM_0000;
}


/**
 * OUT 0xE3 - memory map ROM E000, coz v MZ700 znamena i VRAM na D000
 */
static inline void memory_mmap_rom_upper_on ( void ) {
    DBGPRINTF ( DBGINF, "pwrite = 0xe3, PC = 0x%04x\n", g_mz800.instruction_addr );
    g_memory.map |= MEMORY_MAP_FLAG_ROM_E000;
}


/**
 * OUT 0xE4 - memory map ROM 0000, ROM E000
 * MZ700: unmap CGROM, CGRAM
 * MZ800: map CGROM, VRAM
 */
static inline void memory_mmap_all_on ( void ) {
#if DBGLEVEL
    const char *dbg_mode;
    if ( DMD_TEST_MZ700 ) {
        dbg_mode = "MZ-700";
    } else {
        dbg_mode = "MZ-800";
    };
#endif
    DBGPRINTF ( DBGINF, "pwrite = 0xe4, mode = %s, PC = 0x%04x\n", dbg_mode, g_mz800.instruction_addr );
    g_memory.map |= ( MEMORY_MAP_FLAG_ROM_0000 | MEMORY_MAP_FLAG_ROM_1000 | MEMORY_MAP_FLAG_CGRAM_VRAM | MEMORY_MAP_FLAG_ROM_E000 );
    if ( DMD_TEST_MZ700 ) {
        g_memory.map &= ~( MEMORY_MAP_FLAG_ROM_1000 | MEMORY_MAP_FLAG_CGRAM_VRAM );
    };
}


/**
 * IN 0xE0 - memory map CG-ROM, CG-RAM, VRAM - podle mode
 * 
 * MZ-700: CG-ROM, CG-RAM
 * MZ-800: CG-ROM, VRAM
 */
static inline void memory_mmap_vram_on ( void ) {
    DBGPRINTF ( DBGINF, "pread = 0xe0, PC = 0x%04x\n", g_mz800.instruction_addr );
    g_memory.map |= MEMORY_MAP_FLAG_ROM_1000 | MEMORY_MAP_FLAG_CGRAM_VRAM;
}


/**
 * IN 0xE1 - memory umap CG-ROM, CG-RAM, VRAM - podle mode
 * 
 * MZ-700: CG-ROM, CG-RAM
 * MZ-800: CG-ROM, VRAM
 */
static inline void memory_mmap_vram_off ( void ) {
    DBGPRINTF ( DBGINF, "pread = 0xe1, PC = 0x%04x\n", g_mz800.instruction_addr );
    g_memory.map &= ~( MEMORY_MAP_FLAG_ROM_1000 | MEMORY_MAP_FLAG_CGRAM_VRAM );
}


/**
 * Mapovani pameti pres IORQ - pwrite.
 * 
 * @param mmap_port
 */
void memory_map_pwrite ( en_MMAP_PWRITE mmap_port ) {

    switch ( mmap_port ) {
        case MMAP_PWRITE_E0:
            memory_mmap_rom_bottom_off ( );
            break;

        case MMAP_PWRITE_E1:
            memory_mmap_rom_upper_off ( );
            break;

        case MMAP_PWRITE_E2:
            memory_mmap_rom_0000_on ( );
            break;

        case MMAP_PWRITE_E3:
            memory_mmap_rom_upper_on ( );
            break;

        case MMAP_PWRITE_E4:
            memory_mmap_all_on ( );
            break;

        case MMAP_PWRITE_E5:
            DBGPRINTF ( DBGINF, "pwrite = 0xe5, EXROM map ON: ignorred!, PC = 0x%04x\n", g_mz800.instruction_addr );
            break;

        case MMAP_PWRITE_E6:
            DBGPRINTF ( DBGINF, "pwrite = 0xe6, EXROM map OFF: ignorred!, PC = 0x%04x\n", g_mz800.instruction_addr );
            break;
    };
}


/**
 * Mapovani pameti pres IORQ - pread
 * 
 * @param mmap_port
 */
void memory_map_pread ( en_MMAP_PREAD mmap_port ) {
    switch ( mmap_port ) {
        case MMAP_PREAD_E0:
            memory_mmap_vram_on ( );
            break;

        case MMAP_PREAD_E1:
            memory_mmap_vram_off ( );
            break;
    };
}


/*******************************************************************************
 *
 * Inicializace pameti RAM, MEMEXT, VRAM a EXVRAM
 * 
 ******************************************************************************/

void memory_reconnect_ram ( void ) {
    if ( TEST_MEMEXT_CONNECTED ) {
        int i;
        for ( i = 0; i < MEMORY_MEMRAM_POINTS; i++ ) {
            g_memory.memram_read[i] = memext_get_ram_read_pointer_by_addr_point ( i );
            g_memory.memram_write[i] = memext_get_ram_write_pointer_by_addr_point ( i );
        };
    } else {
        int i;
        for ( i = 0; i < MEMORY_MEMRAM_POINTS; i++ ) {
            g_memory.memram_read[i] = &g_memory.RAM[( i << 12 )];
            g_memory.memram_write[i] = &g_memory.RAM[( i << 12 )];
        };
    };

#ifdef MZ800EMU_CFG_DEBUGGER_ENABLED
    if ( TEST_EMULATION_PAUSED ) {
        ui_main_debugger_windows_refresh ( );
    };
#endif
}


/**
 * Inicializace obsahu DRAM podle toho jak jsem ji odpozoroval na svem MZ-800:
 * 
 * RAM:
 * FF FF 00 00 FF FF 00 00 FF FF 00 00 FF FF 00 00
 * 
 * VRAM:
 * FF 00 FF 00 FF 00 FF 00 FF 00 FF 00 FF 00 FF 00
 * 
 * EXVRAM:
 * FF FF 00 00 FF FF 00 00 FF FF 00 00 FF FF 00 00 
 * FF FF 00 00 FF FF 00 00 FF FF 00 00 FF FF 00 00 
 * FF FF 00 00 FF FF 00 00 FF FF 00 00 FF FF 00 00 
 * FF FF 00 00 FF FF 00 00 FF FF 00 00 FF FF 00 00 
 * FF FF 00 00 FF FF 00 00 FF FF 00 00 FF FF 00 00 
 * FF FF 00 00 FF FF 00 00 FF FF 00 00 FF FF 00 00 
 * FF FF 00 00 FF FF 00 00 FF FF 00 00 FF FF 00 00 
 * FF FF 00 00 FF FF 00 00 FF FF 00 00 FF FF 00 00 
 * 00 00 FF FF 00 00 FF FF 00 00 FF FF 00 00 FF FF
 * 00 00 FF FF 00 00 FF FF 00 00 FF FF 00 00 FF FF
 * 00 00 FF FF 00 00 FF FF 00 00 FF FF 00 00 FF FF
 * 00 00 FF FF 00 00 FF FF 00 00 FF FF 00 00 FF FF
 * 00 00 FF FF 00 00 FF FF 00 00 FF FF 00 00 FF FF
 * 00 00 FF FF 00 00 FF FF 00 00 FF FF 00 00 FF FF
 * 00 00 FF FF 00 00 FF FF 00 00 FF FF 00 00 FF FF
 * 00 00 FF FF 00 00 FF FF 00 00 FF FF 00 00 FF FF
 * 
 */
void memory_init ( void ) {

    DBGPRINTF ( DBGINF, "\n" );

    uint32_t i;
    uint16_t *addr;

    for ( i = 0; i < 0xffff; i += 4 ) {
        addr = ( uint16_t* ) & g_memory.RAM [ i ];
        *addr++ = 0xffff;
        *addr = 0x0000;
    };

    for ( i = 0; i < MEMORY_SIZE_VRAM; i += 2 ) {
        addr = ( uint16_t* ) & g_memory.VRAM [ i ];
        *addr = 0x00ff;
    };

    for ( i = 0; i < MEMORY_SIZE_VRAM; i += 4 ) {
        addr = ( uint16_t* ) & g_memory.EXVRAM [ i ];
        if ( i & 0x80 ) {
            *addr++ = 0x0000;
            *addr = 0xffff;
        } else {
            *addr++ = 0xffff;
            *addr = 0x0000;
        };
    };

    rom_init ( );

    g_memory.map = 0;

    memext_init ( );
    memory_reconnect_ram ( );

#ifdef MEMORY_MAKE_STATISTICS
    memset ( &g_memory_statistics, 0x00, sizeof (g_memory_statistics ) );

    FILE *fp;

    if ( ui_utils_file_access ( MEMORY_STATISTIC_DAT_FILE, F_OK ) != -1 ) {
        if ( ( fp = ui_utils_file_open ( MEMORY_STATISTIC_DAT_FILE, "rb" ) ) ) {
            ui_utils_file_read ( &g_memory_statistics, 1, sizeof (g_memory_statistics ), fp );
        } else {
            ui_show_error ( "%s() - Can't open file '%s': %s", __func__, MEMORY_STATISTIC_DAT_FILE, strerror ( errno ) );
        };
        fclose ( fp );
    } else {
        memory_write_memory_statistics ( );
    };

    printf ( "Actual memory statistics (from file %s):\n", MEMORY_STATISTIC_DAT_FILE );
    int j;
    for ( j = 0; j <= 0x0f; j++ ) {
        printf ( "READ 0x%02x: %llu, WRITE 0x%02x: %llu\n", j, g_memory_statistics.read[j], j, g_memory_statistics.write[j] );
    };
    printf ( "\n\n" );


#endif
}


#if 0

#define MEMORY_DUMP_VRAM_FILE "vram.dat"


void memory_write_vram ( void ) {
    FILE *fp;

    if ( !( fp = ui_utils_file_open ( MEMORY_DUMP_VRAM_FILE, "wb" ) ) ) {
        ui_show_error ( "Can't open file '%s': %s\n", MEMORY_DUMP_VRAM_FILE, strerror ( errno ) );
        return;
    };

    if ( sizeof (g_memory.VRAM ) != ui_utils_file_write ( &g_memory.VRAM, 1, sizeof (g_memory.VRAM ), fp ) ) {
        ui_show_error ( "Can't write to file '%s': %s\n", MEMORY_DUMP_VRAM_FILE, strerror ( errno ) );
    };

    if ( sizeof (g_memory.EXVRAM ) != ui_utils_file_write ( &g_memory.EXVRAM, 1, sizeof (g_memory.VRAM ), fp ) ) {
        ui_show_error ( "Can't write to file '%s': %s\n", MEMORY_DUMP_VRAM_FILE, strerror ( errno ) );
    };

    fclose ( fp );
}

#endif


#ifdef MEMORY_MAKE_STATISTICS


void memory_write_memory_statistics ( void ) {
    FILE *fp;

    if ( !( fp = ui_utils_file_open ( MEMORY_STATISTIC_DAT_FILE, "wb" ) ) ) {
        ui_show_error ( "Can't open file '%s': %s\n", MEMORY_STATISTIC_DAT_FILE, strerror ( errno ) );
        return;
    };

    if ( sizeof (g_memory_statistics ) != ui_utils_file_write ( &g_memory_statistics, 1, sizeof (g_memory_statistics ), fp ) ) {
        ui_show_error ( "Can't write to file '%s': %s\n", MEMORY_STATISTIC_DAT_FILE, strerror ( errno ) );
    };

    fclose ( fp );

    if ( !( fp = ui_utils_file_open ( MEMORY_STATISTIC_TXT_FILE, "wt" ) ) ) {
        ui_show_error ( "Can't open file '%s': %s\n", MEMORY_STATISTIC_TXT_FILE, strerror ( errno ) );
        return;
    };

    fprintf ( fp, "Actual memory statistics (from file %s):\r\n\r\n", MEMORY_STATISTIC_DAT_FILE );
    int j;
    for ( j = 0; j <= 0x0f; j++ ) {
        fprintf ( fp, "READ 0x%02x: %llu, WRITE 0x%02x: %llu\r\n", j, g_memory_statistics.read[j], j, g_memory_statistics.write[j] );
    };
    fprintf ( fp, "\r\n\r\n" );

    fclose ( fp );

    //memory_write_vram ( );
}
#endif


/**
 * implicitni mapovani pameti po resetu
 */
void memory_reset ( void ) {
    DBGPRINTF ( DBGINF, "\n" );
    g_memory.map = MEMORY_MAP_FLAG_ROM_0000 | MEMORY_MAP_FLAG_ROM_1000 | MEMORY_MAP_FLAG_ROM_E000;

    memext_reset ( );
    memory_reconnect_ram ( );
}


/*******************************************************************************
 *
 * Cteni z aktualne mapovane pameti
 * 
 ******************************************************************************/

/**
 * Makra pro cteni z prislusne casti pameti a nasledny return - v zavislosti na mapovani.
 * 
 * Pokud je precteno, tak makro provede return pri kterem vrati prectenou hodnotu, 
 * jinak se pokracuje a nasleduje dalsi radek z volajici funkce.
 * 
 * a = addr >> 12
 * 
 * @param addr
 * @return
 */
#define memory_internal_read_0000_0fff(a) { if ( 0x00 == a ) { if ( MEMORY_MAP_TEST_ROM_0000 ) return MEMORY_ROM_READ_BYTE; return MEMORY_RAM_READ_BYTE; } }
#define memory_internal_read_1000_1fff(a) { if ( 0x01 == a ) { if ( MEMORY_MAP_TEST_ROM_1000 ) return MEMORY_ROM_READ_BYTE; return MEMORY_RAM_READ_BYTE; } }
#define memory_internal_read_8000_9fff_sync(a) { if ( ( 0x08 == a) || ( 0x09 == a) ) { if ( MEMORY_MAP_TEST_VRAM_8000 ) return MEMORY_VRAM_MZ800_READ_BYTE_SYNC; return MEMORY_RAM_READ_BYTE; } }
#define memory_internal_read_8000_9fff(a) { if ( ( 0x08 == a) || ( 0x09 == a) ) { if ( MEMORY_MAP_TEST_VRAM_8000 ) return MEMORY_VRAM_MZ800_READ_BYTE; return MEMORY_RAM_READ_BYTE; } }
#define memory_internal_read_a000_bfff_sync(a) { if ( ( 0x0a == a) || ( 0x0b == a) ) { if ( MEMORY_MAP_TEST_VRAM_A000 ) return MEMORY_VRAM_MZ800_READ_BYTE_SYNC; return MEMORY_RAM_READ_BYTE; } }
#define memory_internal_read_a000_bfff(a) { if ( ( 0x0a == a) || ( 0x0b == a) ) { if ( MEMORY_MAP_TEST_VRAM_A000 ) return MEMORY_VRAM_MZ800_READ_BYTE; return MEMORY_RAM_READ_BYTE; } }
#define memory_internal_read_c000_cfff_sync(a) { if ( 0x0c == a ) { if ( MEMORY_MAP_TEST_CGRAM ) return MEMORY_VRAM_MZ700_READ_BYTE_SYNC; return MEMORY_RAM_READ_BYTE; } }
#define memory_internal_read_c000_cfff(a) { if ( 0x0c == a ) { if ( MEMORY_MAP_TEST_CGRAM ) return MEMORY_VRAM_MZ700_READ_BYTE; return MEMORY_RAM_READ_BYTE; } }
#define memory_internal_read_d000_dfff_sync(a) { if ( 0x0d == a ) { if ( MEMORY_MAP_TEST_VRAM_D000 ) return MEMORY_VRAM_MZ700_READ_BYTE_SYNC; return MEMORY_RAM_READ_BYTE; } }
#define memory_internal_read_d000_dfff(a) { if ( 0x0d == a ) { if ( MEMORY_MAP_TEST_VRAM_D000 ) return MEMORY_VRAM_MZ700_READ_BYTE; return MEMORY_RAM_READ_BYTE; } }
#define memory_internal_read_e000_efff_sync(a) { if ( 0x0e == a ) { if ( MEMORY_MAP_TEST_ROM_E000 ) return memory_internal_read_rom_e000_efff_sync ( addr ); return MEMORY_RAM_READ_BYTE; } }
#define memory_internal_read_e000_efff(a) { if ( 0x0e == a ) { if ( MEMORY_MAP_TEST_ROM_E000 ) return memory_internal_read_rom_e000_efff ( addr ); return MEMORY_RAM_READ_BYTE; } }
#define memory_internal_read_f000_ffff(a) { if ( 0x0f == a ) { if ( MEMORY_MAP_TEST_ROM_E000 ) return MEMORY_ROM_READ_BYTE; return MEMORY_RAM_READ_BYTE; } }


/**
 * Synchronizovane cteni z 0xe000 - 0xefff
 * 
 * @param addr
 * @return 
 */
static inline Z80EX_BYTE memory_internal_read_rom_e000_efff_sync ( Z80EX_WORD addr ) {

    unsigned addr_low = addr & 0x0fff;

    /* cteni z horni rom */
    if ( addr_low > 0x0f ) return MEMORY_ROM_READ_BYTE;

    /* cteni z E009 - E00F */
    if ( addr_low > 0x08 ) return g_mz800.regDBUS_latch;

    /* cteni z E008 ( regDMD ) */
    if ( 0x08 == addr_low ) {
        mz800_sync_insideop_mreq_e00x ( );
        return gdg_read_dmd_status_memop ( );
    };

    /* cteni z CTC8253 */
    if ( addr_low & 0x04 ) {

        /* Kontrol registr cist nelze */
        if ( 0x07 == addr_low ) {
            return g_mz800.regDBUS_latch;
        };
        mz800_sync_insideop_mreq_e00x ( );
        return ctc8253_read_byte ( addr_low & 0x03 );
    };

    /* cteni z PIO8255 */
    mz800_sync_insideop_mreq_e00x ( );
    return pio8255_read ( addr & 0x03 );
}


/**
 * Cteni z 0xe000 - 0xefff - bez synchronizace
 * 
 * @param addr
 * @return 
 */
static inline Z80EX_BYTE memory_internal_read_rom_e000_efff ( Z80EX_WORD addr ) {
    unsigned addr_low = addr & 0x0fff;

    /* cteni z horni rom */
    if ( addr_low > 0x0f ) return MEMORY_ROM_READ_BYTE;

    /* cteni z E009 - E00F */
    if ( addr_low > 0x08 ) return g_mz800.regDBUS_latch;

    /* cteni z E008 ( regDMD ) */
    if ( 0x08 == addr_low ) return gdg_read_dmd_status_memop ( );


    /* cteni z CTC8253 */
    if ( addr_low & 0x04 ) {

        /* Kontrol registr cist nelze */
        if ( 0x07 == addr_low ) {
            return g_mz800.regDBUS_latch;
        };
        // TODO: prozatim vracime 0x00
        return 0x00;
    };

    /* cteni z PIO8255 */
    return pio8255_read ( addr & 0x03 );
}


Z80EX_BYTE memory_internal_read_sync ( Z80EX_WORD addr ) {

    unsigned addr_high = addr >> 12;

#ifdef MEMORY_MAKE_STATISTICS
    g_memory_statistics.read[addr_high]++;
#endif

    memory_internal_read_0000_0fff ( addr_high );
    memory_internal_read_1000_1fff ( addr_high );
    memory_internal_read_8000_9fff_sync ( addr_high );
    memory_internal_read_a000_bfff_sync ( addr_high );
    memory_internal_read_c000_cfff_sync ( addr_high );
    memory_internal_read_d000_dfff_sync ( addr_high );
    memory_internal_read_e000_efff_sync ( addr_high );
    memory_internal_read_f000_ffff ( addr_high );
    return MEMORY_RAM_READ_BYTE;
}


/**
 * Cteni z aktualne mapovane pameti se zachovanim synchronizace u VRAM a 0xe00x periferii.
 * 
 * @param cpu
 * @param addr
 * @param m1_state
 * @param user_data
 * @return 
 */
Z80EX_BYTE memory_read_cb ( Z80EX_CONTEXT *cpu, Z80EX_WORD addr, int m1_state, void *user_data ) {
    Z80EX_BYTE retval = memory_internal_read_sync ( addr );
    g_mz800.regDBUS_latch = retval;
    return retval;
}



#ifdef MZ800EMU_CFG_DEBUGGER_ENABLED


/**
 * Cteni z aktualne mapovane pameti se zachovanim synchronizace u VRAM a 0xe00x periferii.
 * + debugging = ukladani historie poslednich vykonanych bajtu
 * 
 * @param cpu
 * @param addr
 * @param m1_state
 * @param user_data
 * @return 
 */
Z80EX_BYTE memory_read_with_history_cb ( Z80EX_CONTEXT *cpu, Z80EX_WORD addr, int m1_state, void *user_data ) {
    Z80EX_BYTE retval = memory_internal_read_sync ( addr );
    g_mz800.regDBUS_latch = retval;

    if ( ( m1_state ) && ( g_mz800.cpu->prefix == 0x00 ) ) {
        g_debugger_history.position++;
        int position = debugger_history_position ( g_debugger_history.position );
        g_debugger_history.row[position].addr = addr;
        g_debugger_history.row[position].byte[0] = retval;
        g_debugger_history.byte_position = 1;
    } else if ( g_debugger_history.byte_position < DEBUGGER_MAX_INSTR_BYTES ) {
        int position = debugger_history_position ( g_debugger_history.position );
        g_debugger_history.row[position].byte[g_debugger_history.byte_position++] = retval;
    };

    return retval;
}
#endif


/**
 * Cteni z aktualne mapovane pameti - bez synchronizace.
 * 
 * @param addr
 * @return 
 */
Z80EX_BYTE memory_read_byte ( Z80EX_WORD addr ) {
    unsigned addr_high = addr >> 12;
    memory_internal_read_0000_0fff ( addr_high );
    memory_internal_read_1000_1fff ( addr_high );
    memory_internal_read_8000_9fff ( addr_high );
    memory_internal_read_a000_bfff ( addr_high );
    memory_internal_read_c000_cfff ( addr_high );
    memory_internal_read_d000_dfff ( addr_high );
    memory_internal_read_e000_efff ( addr_high );
    memory_internal_read_f000_ffff ( addr_high );
    return MEMORY_RAM_READ_BYTE;
}



/*******************************************************************************
 *
 * Zapis do aktualne mapovane pameti
 * 
 ******************************************************************************/


/**
 * Makra pro zapis do prislusne casti pameti a nasledny return - v zavislosti na mapovani.
 * 
 * Pokud je zapsano, tak makro provede return,
 * jinak se pokracuje a nasleduje dalsi radek z volajici funkce.
 * 
 * a = addr >> 12
 * 
 * @param addr
 * @param value
 */
#define memory_internal_write_0000_0fff(a) { if ( 0x00 == a ) { if ( ! MEMORY_MAP_TEST_ROM_0000 ) MEMORY_RAM_WRITE_BYTE; return; } }
#define memory_internal_write_1000_1fff(a) { if ( 0x01 == a ) { if ( ! MEMORY_MAP_TEST_ROM_1000 ) MEMORY_RAM_WRITE_BYTE; return; } }
#define memory_internal_write_2000_7fff(a) { if ( ( 0x02 <= a ) && ( 0x07 >= a ) ) { MEMORY_RAM_WRITE_BYTE; return; } }
#define memory_internal_write_8000_9fff(a) { if ( ( 0x08 == a ) || ( 0x09 == a ) ) { if ( MEMORY_MAP_TEST_VRAM_8000 ) { MEMORY_VRAM_MZ800_WRITE_BYTE; } else { MEMORY_RAM_WRITE_BYTE; }; return; } }
#define memory_internal_write_a000_bfff(a) { if ( ( 0x0a == a ) || ( 0x0b == a ) ) { if ( MEMORY_MAP_TEST_VRAM_A000 ) { MEMORY_VRAM_MZ800_WRITE_BYTE; } else { MEMORY_RAM_WRITE_BYTE; }; return; } }
#define memory_internal_write_c000_cfff_sync(a) { if ( 0x0c == a ) { if ( MEMORY_MAP_TEST_CGRAM ) { MEMORY_VRAM_MZ700_WRITE_BYTE_SYNC; } else { MEMORY_RAM_WRITE_BYTE; } return; } }
#define memory_internal_write_c000_cfff(a) { if ( 0x0c == a ) { if ( MEMORY_MAP_TEST_CGRAM ) { MEMORY_VRAM_MZ700_WRITE_BYTE; } else { MEMORY_RAM_WRITE_BYTE; } return; } }
#define memory_internal_write_d000_dfff_sync(a) { if ( 0x0d == a ) { if ( MEMORY_MAP_TEST_VRAM_D000 ) { MEMORY_VRAM_MZ700_WRITE_BYTE_SYNC; } else { MEMORY_RAM_WRITE_BYTE; } return; } }
#define memory_internal_write_d000_dfff(a) { if ( 0x0d == a ) { if ( MEMORY_MAP_TEST_VRAM_D000 ) { MEMORY_VRAM_MZ700_WRITE_BYTE; } else { MEMORY_RAM_WRITE_BYTE; } return; } }
#define memory_internal_write_e000_efff_sync(a) { if ( 0x0e == a ) { if ( MEMORY_MAP_TEST_ROM_E000 ) { memory_internal_write_rom_e000_sync ( addr, value ); } else { MEMORY_RAM_WRITE_BYTE; } return; } }
#define memory_internal_write_e000_efff(a) { if ( 0x0e == a ) { if ( MEMORY_MAP_TEST_ROM_E000 ) { memory_internal_write_rom_e000 ( addr, value ); } else { MEMORY_RAM_WRITE_BYTE; } return; } }
#define memory_internal_write_f000_ffff(a) { if ( 0x0f == a ) { if ( ! MEMORY_MAP_TEST_ROM_E000 ) MEMORY_RAM_WRITE_BYTE; return; } }


static inline void memory_internal_write_rom_e000_sync ( Z80EX_WORD addr, Z80EX_BYTE value ) {

    if ( addr > 0xe008 ) return;

    mz800_sync_insideop_mreq_e00x ( );

    if ( addr == 0xe008 ) {
        gdg_write_byte ( addr, value );
    } else if ( addr & 0x04 ) {
        ctc8253_write_byte ( addr & 0x03, value );
    } else {
        pio8255_write ( addr & 0x03, value );
    };
}


static inline void memory_internal_write_rom_e000 ( Z80EX_WORD addr, Z80EX_BYTE value ) {

    if ( addr > 0xe008 ) return;

    if ( addr == 0xe008 ) {
        gdg_write_byte ( addr, value );
    } else if ( addr & 0x04 ) {
        ctc8253_write_byte ( addr & 0x03, value );
    } else {
        pio8255_write ( addr & 0x03, value );
    };
}


/**
 * Zapis do aktualne mapovane pameti s ohledem na synchronizaci.
 * 
 * @param cpu
 * @param addr
 * @param value
 * @param user_data
 */
void memory_write_cb ( Z80EX_CONTEXT *cpu, Z80EX_WORD addr, Z80EX_BYTE value, void *user_data ) {
    unsigned addr_high = addr >> 12;

#ifdef MEMORY_MAKE_STATISTICS
    g_memory_statistics.write[addr_high]++;
#endif

    memory_internal_write_0000_0fff ( addr_high );
    memory_internal_write_1000_1fff ( addr_high );
    memory_internal_write_2000_7fff ( addr_high );
    memory_internal_write_8000_9fff ( addr_high );
    memory_internal_write_a000_bfff ( addr_high );
    memory_internal_write_c000_cfff_sync ( addr_high );
    memory_internal_write_d000_dfff_sync ( addr_high );
    memory_internal_write_e000_efff_sync ( addr_high );
    memory_internal_write_f000_ffff ( addr_high );
}


/**
 * Zapis do aktualne mapovane pameti - bez synchronizace.
 * 
 * @param addr
 * @param value
 */
void memory_write_byte ( Z80EX_WORD addr, Z80EX_BYTE value ) {
    unsigned addr_high = addr >> 12;
    memory_internal_write_0000_0fff ( addr_high );
    memory_internal_write_1000_1fff ( addr_high );
    memory_internal_write_2000_7fff ( addr_high );
    memory_internal_write_8000_9fff ( addr_high );
    memory_internal_write_a000_bfff ( addr_high );
    memory_internal_write_c000_cfff ( addr_high );
    memory_internal_write_d000_dfff ( addr_high );
    memory_internal_write_e000_efff ( addr_high );
    memory_internal_write_f000_ffff ( addr_high );
}


/**
 * Nacteni datoveho bloku do pameti.
 * 
 * @param data
 * @param addr
 * @param size
 * @param type - MEMORY_LOAD_MAPPED, MEMORY_LOAD_RAMONLY
 */
void memory_load_block ( uint8_t *data, Z80EX_WORD addr, Z80EX_WORD size, en_MEMORY_LOAD type ) {

    Z80EX_WORD src_addr = 0;

    while ( size ) {
        //uint8_t *dst = &g_memory.RAM[addr];
        uint8_t *dst = g_memory.memram_write[( addr ) >> 12] + ( addr & 0x0fff );
        uint8_t *src = &data[src_addr];
        uint32_t total_size = addr + size;
        uint32_t load_size;

        if ( type == MEMORY_LOAD_RAMONLY ) {
            uint32_t limit = ( ( addr >> 12 ) + 1 ) << 12;
            load_size = ( total_size < limit ) ? size : ( limit - addr );
            memcpy ( dst, src, load_size );
        } else {
            if ( addr < 0x1000 ) {
                load_size = ( total_size < 0x1000 ) ? size : ( 0x1000 - addr );
                if ( !MEMORY_MAP_TEST_ROM_0000 ) {
                    memcpy ( dst, src, load_size );
                };
            } else if ( addr < 0x2000 ) {
                load_size = ( total_size < 0x2000 ) ? size : ( 0x2000 - addr );
                if ( !MEMORY_MAP_TEST_ROM_1000 ) {
                    memcpy ( dst, src, load_size );
                };
            } else if ( addr < 0x8000 ) {
                uint32_t limit = ( ( addr >> 12 ) + 1 ) << 12;
                load_size = ( total_size < limit ) ? size : ( limit - addr );
                memcpy ( dst, src, load_size );
            } else if ( addr < 0xa000 ) {
                load_size = ( total_size < 0xa000 ) ? size : ( 0xa000 - addr );
                if ( !MEMORY_MAP_TEST_VRAM_8000 ) {
                    uint32_t limit = ( ( addr >> 12 ) + 1 ) << 12;
                    load_size = ( total_size < limit ) ? size : ( limit - addr );
                    memcpy ( dst, src, load_size );
                } else {
                    int i;
                    for ( i = 0; i < load_size; i++ ) {
                        vramctrl_mz800_memop_write_byte ( ( addr + i ) & 0x3fff, data[( src_addr + i )] );
                    };
                };
            } else if ( addr < 0xc000 ) {
                load_size = ( total_size < 0xc000 ) ? size : ( 0xc000 - addr );
                if ( !MEMORY_MAP_TEST_VRAM_A000 ) {
                    uint32_t limit = ( ( addr >> 12 ) + 1 ) << 12;
                    load_size = ( total_size < limit ) ? size : ( limit - addr );
                    memcpy ( dst, src, load_size );
                } else {
                    int i;
                    for ( i = 0; i < load_size; i++ ) {
                        vramctrl_mz800_memop_write_byte ( ( addr + i ) & 0x3fff, data[( src_addr + i )] );
                    };
                };
            } else if ( addr < 0xd000 ) {
                load_size = ( total_size < 0xd000 ) ? size : ( 0xd000 - addr );
                if ( MEMORY_MAP_TEST_CGRAM ) {
                    dst = &g_memoryVRAM_I[( addr & 0x0fff )];
                };
                memcpy ( dst, src, load_size );
            } else if ( addr < 0xe000 ) {
                load_size = ( total_size < 0xe000 ) ? size : ( 0xe000 - addr );
                if ( MEMORY_MAP_TEST_VRAM_D000 ) {
                    dst = &g_memoryVRAM_I[( 0x1000 | ( addr & 0x0fff ) )];
                };
                memcpy ( dst, src, load_size );
            } else {
                uint32_t limit = ( ( addr >> 12 ) + 1 ) << 12;
                load_size = ( total_size < limit ) ? size : ( limit - addr );
                if ( !MEMORY_MAP_TEST_ROM_E000 ) {
                    memcpy ( dst, src, load_size );
                };
            };
        };

        size -= load_size;
        src_addr += load_size;
        addr += load_size;
    };
}
