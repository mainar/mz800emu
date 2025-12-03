/* 
 * File:   qdisk.h
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 11. února 2016, 18:04
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

#ifndef QDISK_H
#define QDISK_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WINDOWS
#define COMPILE_FOR_EMULATOR
#undef COMPILE_FOR_UNICARD
#undef FS_LAYER_FATFS
#elif LINUX
#define COMPILE_FOR_EMULATOR
#undef COMPILE_FOR_UNICARD
#undef FS_LAYER_FATFS
#else
#undef COMPILE_FOR_EMULATOR
#define COMPILE_FOR_UNICARD
#define FS_LAYER_FATFS
#endif

#include "z80ex/include/z80ex.h"

    // tuto hodnotu jseme kdysi v konferenci namerili jako max
    //#define QDISK_FORMAT_SIZE            61454

    // v teto velikosti se ke mne dostal udajne "lehce nafoukly" image od Mikese
#define QDISK_FORMAT_SIZE           82958

    /*
     *  Podle provedenych mereni se povedlo na QD zapsat nad ramec formatu
     * jeste jeden MZF o velikosti 0x1a80
     * 
     */
#define QDISK_IMAGE_MAX_SIZE        ( QDISK_FORMAT_SIZE + 84 + 0x1a80 )

#define QDISK_DISCONNECTED     0
#define QDISK_CONNECTED        1

#define QDSIO_CHANNEL_A        0
#define QDSIO_CHANNEL_B        1


#define QDSTS_NO_DISC       0
#define QDSTS_IMG_READY     1
#define QDSTS_HEAD_HOME     2
#define QDSTS_IMG_SYNC      4
#define QDSTS_IMG_READONLY  8


#define QDISK_TYPE_IMAGE    0
#define QDISK_TYPE_VIRTUAL  1
#define QDISK_TYPE_UNICARD  2

#ifndef FS_LAYER_FATFS
#define QDISKK_FILENAME_LENGTH 1024
#endif


#define QDISK_VIRT_TEMP_FNAME   "qd_temp.tmp"

#define QDISK_MZF_FILENAME_LENGTH   17


    typedef enum en_QDSIO_ADDR {
        QDSIO_ADDR_DATA_A = 0,
        QDSIO_ADDR_DATA_B,
        QDSIO_ADDR_CTRL_A,
        QDSIO_ADDR_CTRL_B
    } en_QDSIO_ADDR;


    typedef enum en_QDSIO_REGADRR {
        QDSIO_REGADDR_0 = 0, /* zakladni prikazy */
        QDSIO_REGADDR_1, /* nastaveni generovani interruptu a signalu Wait/Ready */
        QDSIO_REGADDR_2, /* vektor preruseni (zastoupen jen u kanalu B) */
        QDSIO_REGADDR_3, /* rizeni prijmu */
        QDSIO_REGADDR_4, /* nastaveni vlastnosti prenosu */
        QDSIO_REGADDR_5, /* nastaveni odesilani */
        QDSIO_REGADDR_6, /* sync1 */
        QDSIO_REGADDR_7 /* sync2 */
    } en_QDSIO_REGADRR;


    typedef enum en_QDSIO_WR0CMD {
        QDSIO_WR0CMD_NONE = 0,
        QDSIO_WR0CMD_SDLC_STOP, /* ukoncit vysilani (pouze v rezimu SDLC) */
        QDSIO_WR0CMD_RESET_INTF, /* reset priznaku preruseni */
        QDSIO_WR0CMD_RESET, /* reset kanalu */
        QDSIO_WR0CMD_ENABLE_INT, /* povol generovani interruptu pri dalsim prijatem znaku */
        QDSIO_WR0CMD_RESET_OUTBUF_INT, /* resetuj generovani interruptu pri prazdnem output bufferu */
        QDSIO_WR0CMD_RESET_ERRFL, /* reset  priznaku chyby */
        QDSIO_WR0CMD_RETI /* navrat z preruseni */
    } en_QDSIO_WR0CMD;


    typedef struct st_QDSIO_CHANNEL {
        char name;
        en_QDSIO_REGADRR REG_addr;
        Z80EX_BYTE Wreg [ 8 ];
        Z80EX_BYTE Rreg [ 3 ];
    } st_QDSIO_CHANNEL;


    typedef enum st_QDISK_VRTSTS {
        QDISK_VRTSTS_QDHEADER,
        QDISK_VRTSTS_MZFHEAD,
        QDISK_VRTSTS_MZFBODY,
        QDISK_VRTSTS_FREE_FILEAREA,
        QDISK_VRTSTS_WR_MZFHEAD,
        QDISK_VRTSTS_WR_MZFBODY,
        QDISK_VRTSTS_FORMATING,
    } st_QDISK_VRTSTS;


    /* 0x0000 - 0x0007 ( 8 bajtu ) */
    typedef struct st_QDISK_HEADER {
        Z80EX_BYTE start_sign [ 4 ]; /* 0x00, 0x16, 0x16, 0xa5 */
        Z80EX_BYTE file_blocks_count; /* pocet pouzitych bloku (resp. pocet souboru * 2) */
        Z80EX_BYTE crc [ 3 ]; /* C, R, C */
    } st_QDSTRT_BLOCK;


    /* 0x0008 - 0x00051 ( 10 + 64 = 74 bajtu ) */
    typedef struct st_QDISK_MZF_HEADER {
        Z80EX_BYTE start_sign [ 4 ]; /* 0x00, 0x16, 0x16, 0xa5 */
        Z80EX_BYTE mzf_header_sign; /* 0x00 */
        Z80EX_WORD data_size; /* 0x40, 0x00 - tzn: 64 bajtu */
        Z80EX_BYTE mzf_ftype;
        Z80EX_BYTE mzf_fname [ 16 ];
        Z80EX_BYTE mzf_fname_end; /* 0x0d */
        Z80EX_BYTE unused1 [ 2 ]; /* 0x00 */
        Z80EX_WORD mzf_size;
        Z80EX_WORD mzf_start;
        Z80EX_WORD mzf_exec;
        Z80EX_BYTE mzf_header_description [ 38 ]; /* Prvnich 38 bajtu z mzf description */
        Z80EX_BYTE crc [ 3 ]; /* C, R, C */
    } st_QDISK_MZF_HEADER;


    /* 0x0052 - 0x???? ( 10 + body_size bajtu ) */
    typedef struct st_QDISK_MZF_BODY {
        Z80EX_BYTE start_sign [ 4 ]; /* 0x00, 0x16, 0x16, 0xa5 */
        Z80EX_BYTE mzf_body_sign; /* 0x05 */
        Z80EX_WORD data_size; /* body_size */
        Z80EX_BYTE mzf_body [ 65535 ]; /* body [ body_size ] */
        Z80EX_BYTE crc [ 3 ]; /* C, R, C */
    } st_QDISK_MZF_BODY;


    typedef struct st_QDISK {
        unsigned connected;
        unsigned type;
        unsigned status;

        st_QDSIO_CHANNEL channel [ 2 ];
        FILE *image_fp;
        Z80EX_WORD out_crc16;
        unsigned image_position;

        unsigned virt_status;
        unsigned virt_files_count;
        unsigned virt_file_num;
        char virt_saved_filename [ QDISK_MZF_FILENAME_LENGTH + 4 ];
        Z80EX_WORD virt_mzfbody_size;
        FILE *mzf_fp;
    } st_QDISK;

    extern st_QDISK g_qdisk;

    extern void qdisk_init ( void );
    extern void qdisk_exit ( void );
    extern Z80EX_BYTE qdisk_read_byte ( en_QDSIO_ADDR SIO_addr );
    extern void qdisk_write_byte ( en_QDSIO_ADDR SIO_addr, Z80EX_BYTE value );
    extern void qdisk_close ( void );
    extern void qdisk_open ( void );
    extern void qdisk_mount ( void );
    extern void qdisk_umount ( void );
    extern void qdisk_set_write_protected ( int value );
    extern void qdisk_create_image ( char *filename );
    extern void qdisk_activate_unicard_boot_loader ( void );
    extern void qdisk_deactivate_unicard_boot_loader ( void );


#ifdef __cplusplus
}
#endif

#endif /* QDISK_H */
