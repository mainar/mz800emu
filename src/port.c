/* 
 * File:   iorq.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 18. června 2015, 12:33
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


#include "z80ex/include/z80ex.h"

#include "port.h"
#include "mz800.h"
#include "memory/memory.h"
#include "memory/memext.h"
#include "gdg/gdg.h"
#include "ctc8253/ctc8253.h"
#include "pio8255/pio8255.h"
#include "cmt/cmthack.h"
#include "pioz80/pioz80.h"
#include "typedefs.h"
#include "psg/psg.h"
#include "fdc/fdc.h"
#include "ramdisk/ramdisk.h"
#include "qdisk/qdisk.h"
#include "joy/joy.h"
#include "unicard/unicard.h"
#include "ide8/ide8.h"




#define DBGLEVEL (DBGNON /* | DBGERR | DBGWAR | DBGINF*/)
//#define DBGLEVEL (DBGNON | DBGERR | DBGWAR | DBGINF )
#include "debug.h"


Z80EX_BYTE port_read_cb ( Z80EX_CONTEXT *cpu, Z80EX_WORD port, void *user_data ) {

    Z80EX_BYTE port_lsb = port & 0xff;

    mz800_sync_insideop_iorq ( );

    Z80EX_BYTE retval;

    switch ( port_lsb ) {

        case 0x50:
        case 0x51:
            /* cteme Unicard */
            if ( TEST_UNICARD_CONNECTED ) {
                retval = unicard_read_byte ( port );
            } else {
                retval = g_mz800.regDBUS_latch;
            };
            break;

        case 0x68:
        case 0x69:
        case 0x6a:
        case 0x6b:
        case 0x6c:
        case 0x6d:
        case 0x6e:
        case 0x6f:
            /* cteme posunuty Pezik: 0x68 - 0x6f */
            if ( g_ramdisk.pezik [ RAMDISK_PEZIK_68 ].connected ) {
                retval = ramdisk_pezik_read_byte ( port );
            } else {
                retval = g_mz800.regDBUS_latch;
            };
            break;

        case 0x78:
        case 0x79:
        case 0x7a:
        case 0x7b:
        case 0x7c:
        case 0x7d:
        case 0x7e:
        case 0x7f:
            /* cteme IDE8: 0x78 - 0x7f */
            if ( TEST_IDE8_CONNECTED ) {
                retval = ide8_read_byte ( port_lsb & 0x07 );
            } else {
                retval = g_mz800.regDBUS_latch;
            };
            break;

        case 0xce:
            /* cteme DMD status: 0xce */
            //            printf ("sts: 0x%02x\n", z80ex_get_reg ( g_mz800.cpu, regPC) );
            // nyni uz mame overeno, ze cteni z 0xce se chova stejne v 700 i 800 modu
            retval = gdg_read_dmd_status_ioop ( );
            break;


        case 0xd0:
        case 0xd1:
        case 0xd2:
        case 0xd3:
            /* cteme z PIO8255: 0xd0 - 0xd3  */
            if ( !DMD_TEST_MZ700 ) {
                retval = pio8255_read ( port_lsb & 0x03 );
            } else {
                retval = g_mz800.regDBUS_latch;
            };
            break;


        case 0xd4:
        case 0xd5:
        case 0xd6:
            /* cteme z CTC8253: 0xd4 - 0xd6, 0xd7 cist nelze */
            if ( !DMD_TEST_MZ700 ) {
                retval = ctc8253_read_byte ( port_lsb & 0x03 );
            } else {
                retval = g_mz800.regDBUS_latch;
            };
            break;

        case 0xd8:
        case 0xd9:
        case 0xda:
        case 0xdb:
            /* cteme do WDC: 0xd8 - 0xdf */
            if ( g_fdc.connected ) {
                fdc_read_byte ( port_lsb, &retval );
            };
            break;

        case 0xe0:
        case 0xe1:
            /* cteme memory mapper: 0xe0 - 0xe1 */
            memory_map_pread ( port_lsb );
            retval = g_mz800.regDBUS_latch;
            break;

        case 0xe8:
        case 0xe9:
        case 0xeb:
        case 0xec:
        case 0xed:
        case 0xee:
        case 0xef:
            /* cteme Pezik: 0xe8 - 0xef */
            if ( g_ramdisk.pezik [ RAMDISK_PEZIK_E8 ].connected ) {
                retval = ramdisk_pezik_read_byte ( port );
            } else {
                retval = g_mz800.regDBUS_latch;
            };
            break;

        case 0xea:
            /* pezik, nebo std ramdisk */
            if ( g_ramdisk.pezik [ RAMDISK_PEZIK_E8 ].connected ) {
                retval = ramdisk_pezik_read_byte ( port );
            } else if ( g_ramdisk.std.connected ) {
                retval = ramdisk_std_read_byte ( port_lsb );
            } else {
                retval = g_mz800.regDBUS_latch;
            };
            break;

        case 0xf4:
        case 0xf5:
        case 0xf6:
        case 0xf7:
            /* cteme QDISC: 0xf4 - 0xf7 */
            if ( g_qdisk.connected ) {
                retval = qdisk_read_byte ( port_lsb & 0x03 );
            } else {
                retval = g_mz800.regDBUS_latch;
            };
            break;

        case 0xf8:
        case 0xf9:
            /* cteme standardni ramdisk */
            if ( g_ramdisk.std.connected ) {
                retval = ramdisk_std_read_byte ( port_lsb );
            } else {
                retval = g_mz800.regDBUS_latch;
            };
            break;

        case 0xf0:
            /* cteme JOY1 */
            if ( g_pio8255.signal_PA_joy1_enabled ) {
                retval = joy_read_byte ( JOY_DEVID_0 );
            } else {
                retval = 0xff;
            };
            break;

        case 0xf1:
            /* cteme JOY2 */
            if ( g_pio8255.signal_PA_joy2_enabled ) {
                retval = joy_read_byte ( JOY_DEVID_1 );
            } else {
                retval = 0xff;
            };
            break;

        case 0xfc:
        case 0xfd:
        case 0xfe:
        case 0xff:
            /* cteme z PIOZ80: 0xfc - 0xff */
            retval = pioz80_read_byte ( port_lsb & 0x03 );
            break;

        default:
            // pri cteni neobsazeneho portu vracime posledni byte, ktery byl na sbernici
            retval = g_mz800.regDBUS_latch;
            break;
    };

    return retval;
}



/*write <value> to <port> -- called when WR & IORQ goes active*/


/* z80ex_pwrite_cb */
void port_write_cb ( Z80EX_CONTEXT *cpu, Z80EX_WORD port, Z80EX_BYTE value, void *user_data ) {

    Z80EX_BYTE port_lsb = port & 0xff;

    mz800_sync_insideop_iorq ( );

    switch ( port_lsb ) {

        case 0x01:
            cmthack_load_file ( );
            break;

        case 0x02:
            cmthack_read_mzf_body ( );
            break;

        case 0x50:
        case 0x51:
            /* zapisujeme do Unicard */
            if ( TEST_UNICARD_CONNECTED ) {
                unicard_write_byte ( port, value );
            };
            break;

        case 0x68:
        case 0x69:
        case 0x6a:
        case 0x6b:
        case 0x6c:
        case 0x6d:
        case 0x6e:
        case 0x6f:
            /* zapisujeme na posunuty Pezik: 0x68 - 0x6f */
            if ( g_ramdisk.pezik [ RAMDISK_PEZIK_68 ].connected ) {
                ramdisk_pezik_write_byte ( port, value );
            };
            break;

        case 0x78:
        case 0x79:
        case 0x7a:
        case 0x7b:
        case 0x7c:
        case 0x7d:
        case 0x7e:
        case 0x7f:
            /* zapisujeme na IDE8: 0x78 - 0x7f */
            if ( TEST_IDE8_CONNECTED ) {
                ide8_write_byte ( port_lsb & 0x07, value );
            };
            break;

        case 0xcc:
        case 0xcd:
        case 0xce:
        case 0xcf:
        case 0xf0:
            /* zapisujeme do GDG: 0xcc - 0xcf, 0xf0 */
            gdg_write_byte ( port, value );
            break;


        case 0xd0:
        case 0xd1:
        case 0xd2:
        case 0xd3:
            /* zapisujeme do PIO8255: 0xd0 - 0xd3 */
            if ( !DMD_TEST_MZ700 ) {
                pio8255_write ( port_lsb & 0x03, value );
            };
            break;


        case 0xd4:
        case 0xd5:
        case 0xd6:
        case 0xd7:
            /* zapisujeme do CTC8253: 0xd4 - 0xd7 */
            if ( !DMD_TEST_MZ700 ) {
                ctc8253_write_byte ( port_lsb & 0x03, value );
            };
            break;

        case 0xd8:
        case 0xd9:
        case 0xda:
        case 0xdb:
        case 0xdc:
        case 0xdd:
        case 0xde:
        case 0xdf:
            /* zapisujeme do WDC: 0xd8 - 0xdf */
            if ( g_fdc.connected ) {
                fdc_write_byte ( port_lsb, &value );
            };
            break;

        case 0xe0:
        case 0xe1:
        case 0xe2:
        case 0xe3:
        case 0xe4:
        case 0xe5:
        case 0xe6:
            /* zapisujeme na memory mapper: 0xe0 - 0xe6 */
            memory_map_pwrite ( port_lsb );
            break;

        case 0xe7:
            /* zapisujeme na memext mapper: 0xe7 */
            if ( TEST_MEMEXT_CONNECTED ) {
                memext_map_pwrite ( ( ( port >> 12 ) & 0x0f ), value );
            };
            break;


        case 0xe8:
        case 0xec:
        case 0xed:
        case 0xee:
        case 0xef:
            /* zapisujeme na Pezik: 0xe8, 0xec - 0xef */
            if ( g_ramdisk.pezik [ RAMDISK_PEZIK_E8 ].connected ) {
                ramdisk_pezik_write_byte ( port, value );
            };
            break;

        case 0xf2:
            psg_write_byte ( value );
            break;

        case 0xf4:
        case 0xf5:
        case 0xf6:
        case 0xf7:
            /* zapisujeme do QDISC: 0xf4 - 0xf7 */
            if ( g_qdisk.connected ) {
                qdisk_write_byte ( port_lsb & 0x03, value );
            };
            break;

        case 0xfc:
        case 0xfd:
        case 0xfe:
        case 0xff:
            /* zapisujeme do PIOZ80: 0xfc - 0xff */
            pioz80_write_byte ( port_lsb & 0x03, value );
            break;

        case 0xe9:
        case 0xea:
        case 0xeb:
            if ( g_ramdisk.pezik [ RAMDISK_PEZIK_E8 ].connected ) {
                /* zapisujeme na Pezik: 0xe9 - 0xeb */
                ramdisk_pezik_write_byte ( port, value );

            } else if ( g_ramdisk.std.connected ) {
                /* zapisujeme na Pezik: 0xe9 - 0xeb */
                ramdisk_std_write_byte ( port, value );
            };
            break;

        case 0xfa:
            /* zapisujeme na std ramdisk: 0xfa */
            if ( g_ramdisk.std.connected ) {
                ramdisk_std_write_byte ( port, value );
            };
            break;
    };
}
