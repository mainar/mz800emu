/* 
 * File:   qdisk.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 11. února 2016, 18:03
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


//#define DBGLEVEL        ( DBGNON /* | DBGERR | DBGWAR | DBGINF */ )
//#define DBGLEVEL        ( DBGNON | DBGERR | DBGWAR | DBGINF )
#include "debug.h"

#include "../fs_layer.h"

#include "z80ex/include/z80ex.h"

#include "main.h"

#include "mz800.h"
#include "qdisk.h"
#include "sharpmz_ascii.h"


#ifdef COMPILE_FOR_UNICARD
#include "hal.h"
#include "monitor.h"
#include "mzint.h"
#else
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include "ui/ui_main.h"
#include "ui/ui_file_chooser.h"
#include "ui/ui_utils.h"
#include "ui/ui_qdisk.h"
#include "cfgmain.h"
#include "unicard/unicard.h"
#endif


st_QDISK g_qdisk;

CFGELM *g_elm_std_fp;
CFGELM *g_elm_virt_fp;
CFGELM *g_elm_wrprt;


uint8_t qdisk_virt_scan_directory ( void ) {

    char *dirpath = cfgelement_get_text_value ( g_elm_virt_fp );

    if ( strlen ( dirpath ) == 0 ) {
        return 0x00;
    };

    FS_LAYER_DIR_HANDLE *dh = NULL;
    FS_LAYER_DIR_ITEM *ditem;
    unsigned mzf_files_count = 0;


    dh = FS_LAYER_DIR_OPEN ( dirpath );
    if ( dh == NULL ) {
#ifdef COMPILE_FOR_EMULATOR
        ui_show_error ( "%s() - Can't open dir '%s': %s", __func__, dirpath, FS_LAYER_GET_ERROR_MESSAGE ( ) );
#endif
        return 0x00;
    };

    while ( ( ditem = FS_LAYER_DIR_READ ( dh ) ) != NULL ) {
        if ( FS_LAYER_DITEM_IS_FILE ( dirpath, ditem ) ) {

            const char *fname = FS_LAYER_DITEM_GET_NAME ( ditem );
            unsigned len = strlen ( fname );

            if ( len > 4 ) {
                if ( 0 == strcasecmp ( &fname [ len - 4 ], ".mzf" ) ) {
                    //printf ( "QDISK VIRT FOUND: %d. [%s]\n", mzf_files_count, fname );
                    mzf_files_count++;
                    if ( mzf_files_count == 127 ) break; /* max pocet souboru (254 bloku) */
                };
            };
        };
    };
    FS_LAYER_DIR_CLOSE ( dh );

    return mzf_files_count;
}


void qdisk_virt_close_mzf ( ) {
    if ( g_qdisk.mzf_fp != NULL ) {
        FS_LAYER_FSYNC ( g_qdisk.mzf_fp );
        FS_LAYER_FCLOSE ( g_qdisk.mzf_fp );
        g_qdisk.mzf_fp = NULL;
    };
}


void qdisk_virt_open_mzf_for_read ( const char *dirpath, const char *filename ) {
    const char *filepath = ui_utils_build_filepath ( dirpath, filename );
    if ( FS_LAYER_FR_OK != FS_LAYER_FOPEN ( g_qdisk.mzf_fp, (char*) filepath, FS_LAYER_FMODE_RW ) ) {
        DBGPRINTF ( DBGERR, "fopen()\n" );
#ifdef COMPILE_FOR_EMULATOR
        ui_show_error ( "%s() - Can't create open file '%s': %s", __func__, filepath, strerror ( errno ) );
#endif
    };
    ui_utils_free_filepath ( filepath );
}


void qdisk_virt_prepare_mzf_head ( void ) {

    qdisk_virt_close_mzf ( );

    char *dirpath = cfgelement_get_text_value ( g_elm_virt_fp );

    FS_LAYER_DIR_HANDLE *dh = NULL;
    FS_LAYER_DIR_ITEM *ditem;
    unsigned mzf_files_count = 0;

    dh = FS_LAYER_DIR_OPEN ( dirpath );
    if ( dh == NULL ) {
#ifdef COMPILE_FOR_EMULATOR
        ui_show_error ( "%s() - Can't open dir '%s': %s", __func__, dirpath, FS_LAYER_GET_ERROR_MESSAGE ( ) );
#endif
        return;
    };

    while ( ( ditem = FS_LAYER_DIR_READ ( dh ) ) != NULL ) {
        if ( FS_LAYER_DITEM_IS_FILE ( dirpath, ditem ) ) {

            const char *fname = FS_LAYER_DITEM_GET_NAME ( ditem );
            unsigned len = strlen ( fname );

            if ( 0 == strcasecmp ( &fname [ len - 4 ], ".mzf" ) ) {

                if ( g_qdisk.virt_saved_filename[0] == 0x00 ) {

                    if ( mzf_files_count == g_qdisk.virt_file_num ) {
                        qdisk_virt_open_mzf_for_read ( (const char*) dirpath, FS_LAYER_DITEM_GET_NAME ( ditem ) );
                        break;
                    };

                } else if ( mzf_files_count == ( g_qdisk.virt_files_count - 1 ) ) {
                    /* po tom co jsme zapsali soubor, provadime kontrolu ctenim - zapsany soubor tedy potrebujeme cist jako posledni */
                    qdisk_virt_open_mzf_for_read ( (const char*) dirpath, g_qdisk.virt_saved_filename );
                    break;

                } else if ( mzf_files_count == g_qdisk.virt_file_num ) {
                    qdisk_virt_open_mzf_for_read ( (const char*) dirpath, FS_LAYER_DITEM_GET_NAME ( ditem ) );
                    break;
                };

                mzf_files_count++;
                if ( mzf_files_count == 127 ) break; /* max pocet souboru (254 bloku) */
            };
        };
    };
    FS_LAYER_DIR_CLOSE ( dh );

    if ( g_qdisk.mzf_fp == NULL ) {
#ifdef COMPILE_FOR_EMULATOR
        ui_show_error ( "%s() - Can't prepare MZF file num '%d'", __func__, g_qdisk.virt_file_num );
#endif        
    }
}


void qdisk_virt_prepare_mzf_body ( void ) {

    g_qdisk.virt_mzfbody_size = 0;

    if ( g_qdisk.mzf_fp == NULL ) {
        return;
    };

    /* precteme fsize z mzf */
    if ( FS_LAYER_FR_OK != FS_LAYER_FSEEK ( g_qdisk.mzf_fp, 18 ) ) {
        DBGPRINTF ( DBGERR, "fseek() error\n" );
        return;
    };

    unsigned int readlen;
    Z80EX_BYTE rbuf;

    FS_LAYER_FREAD ( g_qdisk.mzf_fp, &rbuf, 1, &readlen );
    if ( 1 != readlen ) {
        DBGPRINTF ( DBGERR, "fread() error\n" );
        g_qdisk.virt_mzfbody_size = 0;
        return;
    };

    g_qdisk.virt_mzfbody_size = rbuf;

    FS_LAYER_FREAD ( g_qdisk.mzf_fp, &rbuf, 1, &readlen );
    if ( 1 != readlen ) {
        DBGPRINTF ( DBGERR, "fread() error\n" );
        g_qdisk.virt_mzfbody_size = 0;
        return;
    };

    g_qdisk.virt_mzfbody_size |= rbuf << 8;

    /* nastavime se na pozici mzf body */
    if ( FS_LAYER_FR_OK != FS_LAYER_FSEEK ( g_qdisk.mzf_fp, 0x80 ) ) {
        DBGPRINTF ( DBGERR, "fseek() error\n" );
        g_qdisk.virt_mzfbody_size = 0;
        return;
    };
}


void qdisk_drive_reset ( void ) {
    g_qdisk.image_position = 0;
    g_qdisk.status |= QDSTS_HEAD_HOME;
    if ( g_qdisk.type == QDISK_TYPE_VIRTUAL ) {
        qdisk_virt_close_mzf ( );
        g_qdisk.virt_status = QDISK_VRTSTS_QDHEADER;
        g_qdisk.virt_files_count = 0;
        g_qdisk.virt_file_num = 0;
    };
}


void qdisk_close ( void ) {
    if ( g_qdisk.connected == QDISK_CONNECTED ) {
        if ( g_qdisk.status & QDSTS_IMG_READY ) {

            if ( ( g_qdisk.type == QDISK_TYPE_IMAGE ) || ( g_qdisk.type == QDISK_TYPE_UNICARD ) ) {
                /* standard image */

                if ( FS_LAYER_FR_OK != FS_LAYER_FSYNC ( g_qdisk.image_fp ) ) {
                    DBGPRINTF ( DBGERR, "fsync()\n" );
#ifdef COMPILE_FOR_EMULATOR
                    ui_show_error ( "%s():%d - fsync error: %s", __func__, __LINE__, strerror ( errno ) );
#endif
                };
                FS_LAYER_FCLOSE ( g_qdisk.image_fp );

            } else {
                /* virtual qdisk */
                qdisk_virt_close_mzf ( );
            };

            g_qdisk.status &= ~QDSTS_IMG_READY;
        };
    };
}

#define QDISK_CREATE_BLOCK_SIZE 100


void qdisk_create_image ( char *filename ) {

    FILE *fp;

    printf ( "\nQuick Disk: create new QD image '%s'\n", filename );

    if ( FS_LAYER_FR_OK != FS_LAYER_FOPEN ( fp, filename, FS_LAYER_FMODE_W ) ) {
        DBGPRINTF ( DBGERR, "fopen()\n" );
#ifdef COMPILE_FOR_EMULATOR
        ui_show_error ( "%s() - Can't create file '%s': %s", __func__, filename, strerror ( errno ) );
#endif
        return;
    };

    uint8_t block [ QDISK_CREATE_BLOCK_SIZE ];
    memset ( &block, 0x00, sizeof ( block ) );

    unsigned i;
    unsigned len;

    for ( i = QDISK_FORMAT_SIZE; i >= QDISK_CREATE_BLOCK_SIZE; i -= QDISK_CREATE_BLOCK_SIZE ) {
        if ( FS_LAYER_FR_OK != FS_LAYER_FWRITE ( fp, &block, sizeof ( block ), &len ) ) {
            DBGPRINTF ( DBGERR, "fwrite() error\n" );
        };
    };

    if ( i ) {
        if ( FS_LAYER_FR_OK != FS_LAYER_FWRITE ( fp, &block, i, &len ) ) {
            DBGPRINTF ( DBGERR, "fwrite() error\n" );
        };
    };

    if ( FS_LAYER_FR_OK != FS_LAYER_FSYNC ( fp ) ) {
        DBGPRINTF ( DBGERR, "fsync()\n" );
#ifdef COMPILE_FOR_EMULATOR
        ui_show_error ( "%s() - Can't sync file '%s': %s", __func__, filename, strerror ( errno ) );
#endif        
    };

    FS_LAYER_FCLOSE ( fp );
}


void qdisk_virt_open_directory ( char *dirpath ) {


    if ( g_qdisk.connected == QDISK_CONNECTED ) {

        g_qdisk.status = QDSTS_NO_DISC;

        qdisk_drive_reset ( );
        qdisk_close ( );

        if ( strlen ( dirpath ) != 0 ) {

            unsigned read_only_flag;
            if ( 0 != cfgelement_get_bool_value ( g_elm_wrprt ) ) {
                read_only_flag = QDSTS_IMG_READONLY;
            } else {
                read_only_flag = 0;
            };

            FS_LAYER_DIR_HANDLE *dh = FS_LAYER_DIR_OPEN ( dirpath );

            if ( dh != NULL ) {
                FS_LAYER_DIR_CLOSE ( dh );
                g_qdisk.status = QDSTS_IMG_READY | QDSTS_HEAD_HOME | read_only_flag;
                cfgelement_set_text_value ( g_elm_virt_fp, dirpath );

            } else {
                cfgelement_set_text_value ( g_elm_virt_fp, "" );
#ifdef COMPILE_FOR_EMULATOR
                ui_show_error ( "%s() - Can't open dir '%s': %s", __func__, dirpath, FS_LAYER_GET_ERROR_MESSAGE ( ) );
#endif
            };
        } else {
#ifdef COMPILE_FOR_EMULATOR
            cfgelement_set_text_value ( g_elm_virt_fp, "" );
#endif
        };

    } else {
        g_qdisk.status = QDSTS_NO_DISC;
    };
}


void qdisk_open_image ( char *filepath ) {

    if ( g_qdisk.connected == QDISK_CONNECTED ) {

        g_qdisk.status = QDSTS_NO_DISC;

        qdisk_drive_reset ( );
        qdisk_close ( );

        if ( strlen ( filepath ) != 0 ) {

            char *open_file_mode;
            unsigned read_only_flag;

            if ( ( 0 != cfgelement_get_bool_value ( g_elm_wrprt ) ) || ( ui_utils_file_access ( filepath, W_OK ) == -1 ) ) {
                open_file_mode = FS_LAYER_FMODE_RO;
                read_only_flag = QDSTS_IMG_READONLY;
            } else {
                open_file_mode = FS_LAYER_FMODE_RW;
                read_only_flag = 0;
            };

            if ( FS_LAYER_FR_OK == FS_LAYER_FOPEN ( g_qdisk.image_fp, filepath, open_file_mode ) ) {
                g_qdisk.status = QDSTS_IMG_READY | QDSTS_HEAD_HOME | read_only_flag;

#ifdef COMPILE_FOR_EMULATOR
                cfgelement_set_text_value ( g_elm_std_fp, filepath );
#endif
            } else {
#ifdef COMPILE_FOR_EMULATOR
                ui_show_error ( "%s() - Can't open file '%s': %s", __func__, filepath, strerror ( errno ) );
                cfgelement_set_text_value ( g_elm_std_fp, "" );
#endif
            };
        } else {
#ifdef COMPILE_FOR_EMULATOR
            cfgelement_set_text_value ( g_elm_std_fp, "" );
#endif
        };

    } else {
        g_qdisk.status = QDSTS_NO_DISC;
    };
}


void qdisk_open ( void ) {
    char *filepath;
    if ( g_qdisk.type == QDISK_TYPE_IMAGE ) {
        filepath = cfgelement_get_text_value ( g_elm_std_fp );
        qdisk_open_image ( filepath );
#ifdef COMPILE_FOR_EMULATOR
        filepath = cfgelement_get_text_value ( g_elm_std_fp );
        ui_qdisk_set_path ( filepath );
    } else if ( g_qdisk.type == QDISK_TYPE_UNICARD ) {
        if ( TEST_UNICARD_CONNECTED ) {
            filepath = unicard_get_mzfloader_image_filepath ( );
            qdisk_open_image ( filepath );
            ui_utils_mem_free ( filepath );
        };
#endif
    } else {
        /* TODO: */
        filepath = cfgelement_get_text_value ( g_elm_virt_fp );
        qdisk_virt_open_directory ( filepath );
#ifdef COMPILE_FOR_EMULATOR
        filepath = cfgelement_get_text_value ( g_elm_virt_fp );
        ui_qdisk_set_path ( filepath );
#endif
    };
    g_qdisk.virt_saved_filename[0] = 0x00;
}


void qdisk_mount ( void ) {

    if ( g_qdisk.type == QDISK_TYPE_IMAGE ) {

        char *filename = ui_file_chooser_open_mzq ( cfgelement_get_text_value ( g_elm_std_fp ) );
        if ( !filename ) return;

        if ( ui_utils_file_access ( filename, F_OK ) == -1 ) {
            /* soubor neexistuje - vyrobime novy */
            //printf ( "create new: '%s'\n", filename );
            qdisk_create_image ( filename );
        };

        //printf ( "open MZQ: '%s'\n", filename );
        qdisk_open_image ( filename );
        ui_utils_mem_free ( filename );
        ui_qdisk_set_path ( cfgelement_get_text_value ( g_elm_std_fp ) );

    } else {
        /* virtual QDISK */
        char *dirpath = ui_file_chooser_open_qddir ( cfgelement_get_text_value ( g_elm_virt_fp ) );
        if ( !dirpath ) return;
        qdisk_virt_open_directory ( dirpath );
        ui_utils_mem_free ( dirpath );
        ui_qdisk_set_path ( cfgelement_get_text_value ( g_elm_virt_fp ) );
    };
}


void qdisk_umount ( void ) {
    qdisk_close ( );
    if ( ( g_qdisk.type == QDISK_TYPE_IMAGE ) || ( g_qdisk.type == QDISK_TYPE_UNICARD ) ) {
        cfgelement_set_text_value ( g_elm_std_fp, "" );
    } else {
        cfgelement_set_text_value ( g_elm_virt_fp, "" );
    };
    ui_qdisk_menu_update ( );
    ui_qdisk_set_path ( "" );
}


void qdisk_set_write_protected ( int value ) {
    cfgelement_set_bool_value ( g_elm_wrprt, value );
    if ( g_qdisk.status & QDSTS_IMG_READY ) {
        qdisk_close ( );
        qdisk_open ( );
    };
}


/* Pri ukonceni je potreba uzavrit vsechny otevrene soubory */
void qdisk_exit ( void ) {
    qdisk_close ( );
}


void qdisk_init ( void ) {

    memset ( &g_qdisk, 0x00, sizeof ( st_QDISK ) );
    g_qdisk.channel[0].name = 'A';
    g_qdisk.channel[1].name = 'B';
    qdisk_drive_reset ( );
    g_qdisk.mzf_fp = NULL;

    CFGMOD *cmod = cfgroot_register_new_module ( g_cfgmain, "QDISK" );

    CFGELM *elm;
    elm = cfgmodule_register_new_element ( cmod, "mz1f11_connected", CFGENTYPE_BOOL, QDISK_CONNECTED );
    cfgelement_set_handlers ( elm, (void*) &g_qdisk.connected, (void*) &g_qdisk.connected );

    elm = cfgmodule_register_new_element ( cmod, "mz1f11_type", CFGENTYPE_KEYWORD, QDISK_TYPE_IMAGE,
                                           QDISK_TYPE_IMAGE, "STANDARD",
                                           QDISK_TYPE_VIRTUAL, "VIRTUAL",
                                           QDISK_TYPE_UNICARD, "UNICARD",
                                           -1 );
    cfgelement_set_handlers ( elm, (void*) &g_qdisk.type, (void*) &g_qdisk.type );

    g_elm_std_fp = cfgmodule_register_new_element ( cmod, "mz1f11_std_filepath", CFGENTYPE_TEXT, "" );
    g_elm_virt_fp = cfgmodule_register_new_element ( cmod, "mz1f11_virtual_filepath", CFGENTYPE_TEXT, "" );
    g_elm_wrprt = cfgmodule_register_new_element ( cmod, "mz1f11_write_protected", CFGENTYPE_BOOL, 0 );

    cfgmodule_parse ( cmod );
    cfgmodule_propagate ( cmod );

    qdisk_open ( );

    ui_qdisk_menu_update ( );
}


Z80EX_BYTE qdisk_read_byte_from_drive ( void ) {
    unsigned int readlen;
    Z80EX_BYTE retval;

    /* pokud neni pripojen img - jdeme pryc */
    if ( 0 == ( g_qdisk.status & QDSTS_IMG_READY ) ) {
        return 0xff;
    };

    /* pokud nebezi motor - jdeme pryc */
    if ( ( g_qdisk.channel[ QDSIO_CHANNEL_B ].Wreg[ QDSIO_REGADDR_5 ] & 0x80 ) == 0x00 ) {
        return 0xff;
    };


    if ( QDISK_IMAGE_MAX_SIZE <= g_qdisk.image_position ) {
        if ( QDISK_IMAGE_MAX_SIZE == g_qdisk.image_position ) g_qdisk.image_position++;
        return 0xff;
    };


    if ( ( g_qdisk.type == QDISK_TYPE_IMAGE ) || ( g_qdisk.type == QDISK_TYPE_UNICARD ) ) {

        FS_LAYER_FREAD ( g_qdisk.image_fp, &retval, 1, &readlen );

        if ( 1 != readlen ) {
            DBGPRINTF ( DBGERR, "fread() error\n" );
        };

        g_qdisk.image_position++;

    } else {

        retval = 0xff;

        static char CRC[] = "CRC";
        static char *crc_ptr = CRC;

        if ( g_qdisk.image_position < 3 ) {
            printf ( "Err: read in sync area? (%d)\n", g_qdisk.image_position );

        } else if ( g_qdisk.image_position == 3 ) {
            /* konec synchronizacni znacky */
            retval = 0xa5;
            g_qdisk.image_position++;

        } else if ( g_qdisk.virt_status == QDISK_VRTSTS_QDHEADER ) {

            if ( g_qdisk.image_position == 4 ) {
                /* probiha normalni cteni - razeni souboru bude takove, jak jsou ulozeny na disku (saved_filename nas nezajima) */
                g_qdisk.virt_saved_filename[0] = 0x00;
                g_qdisk.virt_files_count = qdisk_virt_scan_directory ( );
                //printf ( "QDISK VIRT: scan ( 0x%02x)\n", g_qdisk.virt_files_count );
                retval = g_qdisk.virt_files_count << 1;
                g_qdisk.image_position++;
                crc_ptr = CRC;

            } else if ( g_qdisk.image_position <= 7 ) {
                g_qdisk.image_position++;
                retval = *crc_ptr++;
            };
            /* disk header je precten */

        } else if ( g_qdisk.virt_status == QDISK_VRTSTS_FREE_FILEAREA ) {
            if ( g_qdisk.image_position & 1 ) {
                retval = 0xaa;
                g_qdisk.image_position = 4;
            } else {
                retval = 0x55;
                g_qdisk.image_position = 5;
            };

        } else if ( g_qdisk.virt_status == QDISK_VRTSTS_MZFHEAD ) {

            if ( 4 == g_qdisk.image_position ) {
                /* mzf header sign */
                retval = 0x00;
                g_qdisk.image_position++;

            } else if ( 6 >= g_qdisk.image_position ) {
                /* mzf header size 0x0040 */
                retval = 0x40 * ( 6 - g_qdisk.image_position );
                g_qdisk.image_position++;

            } else if ( 24 >= g_qdisk.image_position ) {
                /* mzf ftype, fname[16], 0x0d */
                FS_LAYER_FREAD ( g_qdisk.mzf_fp, &retval, 1, &readlen );

                if ( 1 != readlen ) {
                    DBGPRINTF ( DBGERR, "fread() error\n" );
                };

                g_qdisk.image_position++;

            } else if ( 26 >= g_qdisk.image_position ) {
                /* 2 bajty unused */
                retval = 0x00;
                g_qdisk.image_position++;

            } else if ( 70 >= g_qdisk.image_position ) {
                /* size, start, exec, comment[38] */
                FS_LAYER_FREAD ( g_qdisk.mzf_fp, &retval, 1, &readlen );

                if ( 1 != readlen ) {
                    DBGPRINTF ( DBGERR, "fread() error\n" );
                };
                g_qdisk.image_position++;
                crc_ptr = CRC;

            } else if ( 73 >= g_qdisk.image_position ) {
                g_qdisk.image_position++;
                retval = *crc_ptr++;
            };
            /* hlavicka je prectena */

        } else if ( g_qdisk.virt_status == QDISK_VRTSTS_MZFBODY ) {

            if ( 4 == g_qdisk.image_position ) {
                /* mzf body sign */
                retval = 0x05;
                g_qdisk.image_position++;

            } else if ( 5 == g_qdisk.image_position ) {
                /* mzf body size + 10 */
                retval = g_qdisk.virt_mzfbody_size & 0xff;
                g_qdisk.image_position++;

            } else if ( 6 == g_qdisk.image_position ) {
                /* mzf body size + 10 */
                retval = ( g_qdisk.virt_mzfbody_size >> 8 ) & 0xff;
                g_qdisk.image_position++;

            } else if ( 7 == g_qdisk.image_position ) {

                FS_LAYER_FREAD ( g_qdisk.mzf_fp, &retval, 1, &readlen );

                if ( 1 != readlen ) {
                    DBGPRINTF ( DBGERR, "fread() error\n" );
                };

                g_qdisk.virt_mzfbody_size--;
                if ( 0 == g_qdisk.virt_mzfbody_size ) {
                    g_qdisk.image_position++;
                    qdisk_virt_close_mzf ( );
                    crc_ptr = CRC;
                };

            } else if ( 10 >= g_qdisk.image_position ) {

                retval = *crc_ptr++;
                g_qdisk.image_position++;
            };
            /* telo precteno */
        };
    };

    //printf ( "\tread: ( %d, %d) = 0x%02x\n", g_qdisk.virt_status, g_qdisk.image_position - 1, retval );
    return retval;
}


int qdisk_test_disk_is_writeable ( void ) {

    /* pokud neni pripojen img - jdeme pryc */
    if ( 0 == ( g_qdisk.status & QDSTS_IMG_READY ) ) {
        return 0;
    };

    /* pokud je img write protected - jdeme pryc */
    if ( g_qdisk.status & QDSTS_IMG_READONLY ) {
        return 0;
    };

    /* pokud nebezi motor - jdeme pryc */
    if ( ( g_qdisk.channel[ QDSIO_CHANNEL_B ].Wreg[ QDSIO_REGADDR_5 ] & 0x80 ) == 0x00 ) {
        return 0;
    };

    /* pokud neni nastaven output mode - jdeme pryc */
    if ( ( g_qdisk.channel[ QDSIO_CHANNEL_A ].Wreg[ QDSIO_REGADDR_5 ] & 0x08 ) == 0x00 ) {
        return 0;
    };

    return 1;
}


void qdisk_write_byte_into_drive ( Z80EX_BYTE value ) {
    unsigned len;

    if ( 0 == qdisk_test_disk_is_writeable ( ) ) return;

    if ( ( g_qdisk.type == QDISK_TYPE_IMAGE ) || ( g_qdisk.type == QDISK_TYPE_UNICARD ) ) {

        if ( QDISK_IMAGE_MAX_SIZE <= g_qdisk.image_position ) {
            if ( QDISK_IMAGE_MAX_SIZE == g_qdisk.image_position ) g_qdisk.image_position++;
            return;
        };

        if ( FS_LAYER_FR_OK != FS_LAYER_FWRITE ( g_qdisk.image_fp, &value, 1, &len ) ) {
            DBGPRINTF ( DBGERR, "fwrite() error\n" );
#ifdef COMPILE_FOR_EMULATOR
            ui_show_error ( "%s():%d - fwrite error: %s", __func__, __LINE__, strerror ( errno ) );
#endif
        };

        g_qdisk.image_position++;

    } else {

        //printf ( "\twrite: ( %d, %d) = 0x%02x\n", g_qdisk.virt_status, g_qdisk.image_position, value );

        if ( g_qdisk.virt_status == QDISK_VRTSTS_QDHEADER ) {
            if ( g_qdisk.image_position == 4 ) {
                if ( value == 0 ) {
                    //printf ( "Formating...\n" );
                    g_qdisk.virt_status = QDISK_VRTSTS_FORMATING;
                } else {
                    /* bude probihat kontrolni cteni - saved_filename musi byt precten vzdy jako posledni! (nenulujeme jej) */
                    //printf ( "Write file done...\n" );
                    g_qdisk.virt_files_count = qdisk_virt_scan_directory ( );
                    //printf ( "QDISK VIRT: scan ( 0x%02x)\n", g_qdisk.virt_files_count );
                };
            };
            g_qdisk.image_position++;

        } else if ( g_qdisk.virt_status == QDISK_VRTSTS_WR_MZFHEAD ) {

            if ( ( ( g_qdisk.image_position >= 7 ) && ( g_qdisk.image_position < 25 ) ) || ( ( g_qdisk.image_position >= 27 ) && ( g_qdisk.image_position < 71 ) ) ) {

                if ( FS_LAYER_FR_OK != FS_LAYER_FWRITE ( g_qdisk.mzf_fp, &value, 1, &len ) ) {
                    DBGPRINTF ( DBGERR, "fwrite() error\n" );
                };

            };
            g_qdisk.image_position++;

        } else if ( g_qdisk.virt_status == QDISK_VRTSTS_WR_MZFBODY ) {

            if ( g_qdisk.image_position <= 6 ) {

                if ( g_qdisk.image_position == 5 ) {
                    g_qdisk.virt_mzfbody_size = value;

                } else if ( g_qdisk.image_position == 6 ) {
                    g_qdisk.virt_mzfbody_size |= value << 8;
                };

                g_qdisk.image_position++;

            } else {

                g_qdisk.virt_mzfbody_size--;

                if ( FS_LAYER_FR_OK != FS_LAYER_FWRITE ( g_qdisk.mzf_fp, &value, 1, &len ) ) {
                    DBGPRINTF ( DBGERR, "fwrite() error\n" );
                };

                if ( g_qdisk.virt_mzfbody_size == 0 ) {

                    if ( FS_LAYER_FR_OK != FS_LAYER_FSEEK ( g_qdisk.mzf_fp, 1 ) ) {
                        DBGPRINTF ( DBGERR, "fseek() error\n" );
                    };

                    unsigned int readlen;
                    uint8_t mzf_fname [ QDISK_MZF_FILENAME_LENGTH ];

                    FS_LAYER_FREAD ( g_qdisk.mzf_fp, mzf_fname, sizeof ( mzf_fname ), &readlen );
                    if ( sizeof ( mzf_fname ) != readlen ) {
                        DBGPRINTF ( DBGERR, "fread() error\n" );
                    };

                    qdisk_virt_close_mzf ( );



                    int i;
                    for ( i = 0; i < sizeof ( g_qdisk.virt_saved_filename ) - 4; i++ ) {
                        if ( mzf_fname [ i ] < 0x20 ) break;
                        g_qdisk.virt_saved_filename [ i ] = sharpmz_cnv_from ( mzf_fname [ i ] );
                    };
                    g_qdisk.virt_saved_filename [ i ] = 0x00;
                    strcat ( g_qdisk.virt_saved_filename, ".mzf" );

                    char *dirpath = cfgelement_get_text_value ( g_elm_virt_fp );
                    if ( 0 != ui_utils_file_rename ( dirpath, QDISK_VIRT_TEMP_FNAME, g_qdisk.virt_saved_filename ) ) {
                        DBGPRINTF ( DBGERR, "rename() error\n" );
                    };

                    g_qdisk.virt_status = QDISK_VRTSTS_FREE_FILEAREA;
                };
            }
        };

    };
}


Z80EX_BYTE qdisk_read_byte ( en_QDSIO_ADDR SIO_addr ) {


    Z80EX_BYTE retval = 0x00;

    st_QDSIO_CHANNEL *channel = &g_qdisk.channel[ SIO_addr & 0x01 ];

    switch ( SIO_addr ) {

        case QDSIO_ADDR_CTRL_A:

            /* hunt phase */
            if ( ( channel->Wreg [ QDSIO_REGADDR_3 ] & 0x11 ) == 0x11 ) {

                channel->Rreg [ QDSIO_REGADDR_0 ] |= 0x10;
                g_qdisk.status &= ~QDSTS_IMG_SYNC;

                if ( ( g_qdisk.type == QDISK_TYPE_IMAGE ) || ( g_qdisk.type == QDISK_TYPE_UNICARD ) ) {

                    Z80EX_BYTE sync1, sync2;

                    sync1 = qdisk_read_byte_from_drive ( );

                    int i;
                    for ( i = 0; i < 8; i++ ) {

                        sync2 = qdisk_read_byte_from_drive ( );

                        if ( ( sync1 == channel->Wreg [ QDSIO_REGADDR_6 ] ) && ( sync2 == channel->Wreg [ QDSIO_REGADDR_7 ] ) ) {
                            channel->Rreg [ QDSIO_REGADDR_0 ] &= 0xef; // inverze huntphase bitu a konec
                            g_qdisk.status |= QDSTS_IMG_SYNC;
                            break;
                        }

                        sync1 = sync2;
                    };

                } else {

                    //printf ( "OK: hunt phase ( %d, %d)\n", g_qdisk.virt_status, g_qdisk.image_position );

                    if ( g_qdisk.image_position == 0 ) {
                        /* automaticky predpokladame g_qdisk.virt_status == QDISK_VRTSTS_MZFHEAD */

                        g_qdisk.image_position = 3;
                        channel->Rreg [ QDSIO_REGADDR_0 ] &= 0xef; // inverze huntphase bitu a konec
                        g_qdisk.status |= QDSTS_IMG_SYNC;

                    } else {

                        if ( g_qdisk.virt_status != QDISK_VRTSTS_FREE_FILEAREA ) {

                            if ( g_qdisk.virt_status == QDISK_VRTSTS_MZFHEAD ) {
                                qdisk_virt_prepare_mzf_body ( );
                                g_qdisk.virt_status = QDISK_VRTSTS_MZFBODY;

                            } else {

                                if ( g_qdisk.virt_status == QDISK_VRTSTS_QDHEADER ) {

                                    if ( ( g_qdisk.image_position > 4 ) && ( g_qdisk.virt_files_count != 0 ) ) {

                                        /* otevrit mzf a nastavit se na zacatek headeru */
                                        qdisk_virt_prepare_mzf_head ( );
                                        g_qdisk.virt_status = QDISK_VRTSTS_MZFHEAD;

                                    } else {
                                        /* zadne soubory na disku nemame */
                                        g_qdisk.virt_status = QDISK_VRTSTS_FREE_FILEAREA;
                                    };


                                } else if ( g_qdisk.virt_status == QDISK_VRTSTS_MZFBODY ) {

                                    if ( ( g_qdisk.virt_files_count - 1 ) > g_qdisk.virt_file_num ) {

                                        /* otevrit dalsi mzf a nastavit se na zacatek headeru */
                                        g_qdisk.virt_file_num++;
                                        qdisk_virt_prepare_mzf_head ( );
                                        g_qdisk.virt_status = QDISK_VRTSTS_MZFHEAD;

                                    } else {
                                        /* zadne dalsi soubory na disku nemame */
                                        g_qdisk.virt_status = QDISK_VRTSTS_FREE_FILEAREA;
                                    };
                                };
                            };

                            g_qdisk.image_position = 3;
                            channel->Rreg [ QDSIO_REGADDR_0 ] &= 0xef; // inverze huntphase bitu a konec
                            g_qdisk.status |= QDSTS_IMG_SYNC;

                        };
                    };
                };
            };

            channel->Rreg [ QDSIO_REGADDR_0 ] |= 0x01; /* v prijimacim bufferu je alespon jeden bajt */
            channel->Rreg [ QDSIO_REGADDR_0 ] |= 0x04; /* output buffer je prazdny */

            if ( g_qdisk.status & QDSTS_IMG_READY ) {
                channel->Rreg [ QDSIO_REGADDR_0 ] |= 0x08; /* DCD 1: disk je pritomen */
            } else {
                channel->Rreg [ QDSIO_REGADDR_0 ] &= ~0x08;
            };

            if ( g_qdisk.status & QDSTS_IMG_READONLY ) {
                channel->Rreg [ QDSIO_REGADDR_0 ] &= ~0x20; /* CTS 0: disk chranen proti zapisu */
            } else {
                channel->Rreg [ QDSIO_REGADDR_0 ] |= 0x20;
            };

            /* pokud jsme prekrocili velikost media, tk zahlasime CRC error */
            if ( QDISK_IMAGE_MAX_SIZE < g_qdisk.image_position ) {
                channel->Rreg [ QDSIO_REGADDR_1 ] |= 0x40; /* CTS 1: CRC error */
            } else {
                channel->Rreg [ QDSIO_REGADDR_1 ] &= ~0x40;
            };

            retval = channel->Rreg [ channel->REG_addr & 0x03 ];

            //printf ( "%s(): channel: '%c', port: 0x%02x, retval: 0x%02x, (PC = 0x%04x)\n", __func__, channel->name, SIO_addr + 0xf4, retval, z80ex_get_reg ( g_mz800.cpu, regPC ) );

            break;

        case QDSIO_ADDR_CTRL_B:

            if ( g_qdisk.status & QDSTS_HEAD_HOME ) {
                channel->Rreg [ QDSIO_REGADDR_0 ] = 0x08;
            } else {
                channel->Rreg [ QDSIO_REGADDR_0 ] = 0x00;
            };

            if ( ( g_qdisk.channel[ QDSIO_CHANNEL_A ].Wreg[ QDSIO_REGADDR_5 ] & 0x1a ) == 0x0a ) {
                if ( g_qdisk.out_crc16 != 0 ) {
                    qdisk_write_byte_into_drive ( 'C' );
                    qdisk_write_byte_into_drive ( 'R' );
                    qdisk_write_byte_into_drive ( 'C' );
                };
            };

            if ( QDSIO_REGADDR_0 == channel->REG_addr ) {
                retval = 0xff;
            } else {
                retval = channel->Rreg [ channel->REG_addr & 0x03 ];
            };

            break;

        case QDSIO_ADDR_DATA_A:

            g_qdisk.status &= ~QDSTS_HEAD_HOME;

            if ( g_qdisk.status & QDSTS_IMG_READY ) {
                retval = qdisk_read_byte_from_drive ( );
            };
            break;

        case QDSIO_ADDR_DATA_B:
            retval = 0xff;
            break;
    };

    channel->REG_addr = QDSIO_REGADDR_0;

    return retval;
}


void qdisk_write_byte ( en_QDSIO_ADDR SIO_addr, Z80EX_BYTE value ) {

    st_QDSIO_CHANNEL *channel = &g_qdisk.channel[ SIO_addr & 0x01 ];


    /* zapis na CTRL [ A / B ] */
    if ( SIO_addr & 0x02 ) {

        channel->Wreg[ channel->REG_addr ] = value;

        if ( QDSIO_REGADDR_0 == channel->REG_addr ) {

            channel->REG_addr = value & 0x07;
            en_QDSIO_WR0CMD wr0cmd = ( value >> 3 ) & 0x07;

            /* reset vypoctu CRC odchozich dat */
            if ( ( value & 0xc0 ) == 0x80 ) {
                g_qdisk.out_crc16 = 0;
            };

            switch ( wr0cmd ) {

                case QDSIO_WR0CMD_RESET:
                    memset ( &channel->Wreg, 0x00, sizeof ( channel->Wreg ) );
                    break;

                case QDSIO_WR0CMD_NONE:
                case QDSIO_WR0CMD_RESET_INTF:
                case QDSIO_WR0CMD_SDLC_STOP:
                case QDSIO_WR0CMD_ENABLE_INT:
                case QDSIO_WR0CMD_RESET_OUTBUF_INT:
                case QDSIO_WR0CMD_RESET_ERRFL:
                case QDSIO_WR0CMD_RETI:
                    break;
            };

        } else {

            switch ( channel->REG_addr ) {


                case QDSIO_REGADDR_2:

                    /* nastaveni interrupt vectoru ( lze jen u kanalu B ) */
                    if ( channel->name == 'B' ) {
                        channel->Rreg [ channel->REG_addr ] = value;
                    }
                    break;

                    /* Rx CTRL*/
                case QDSIO_REGADDR_3:
                    if ( channel->Wreg [ QDSIO_REGADDR_3 ] & 0x10 ) {
                        /* vstup do rezimu Hunt */
                        channel->Rreg [ QDSIO_REGADDR_0 ] |= 0x10;
                    };
                    break;

                    /* Tx CTRL */
                case QDSIO_REGADDR_5:

                    if ( channel->name == 'B' ) {

                        if ( ( channel->Wreg[ QDSIO_REGADDR_5 ] & 0x80 ) == 0x00 ) {
                            /* QD motor nenaktivni */

                            if ( g_qdisk.status & QDSTS_IMG_READY ) {

                                if ( ( g_qdisk.type == QDISK_TYPE_IMAGE ) || ( g_qdisk.type == QDISK_TYPE_UNICARD ) ) {

                                    if ( FS_LAYER_FR_OK != FS_LAYER_FSEEK ( g_qdisk.image_fp, 0 ) ) {
                                        DBGPRINTF ( DBGERR, "fseek() error\n" );
                                    };

                                    FS_LAYER_FSYNC ( g_qdisk.image_fp );
                                };
                            };

                            qdisk_drive_reset ( );

                        };

                    } else {

                        if ( ( channel->Wreg[ QDSIO_REGADDR_5 ] & 0x18 ) == 0x18 ) {

                            /* signal preruseni vysilani + povoleno odesilani dat */

                            if ( ( g_qdisk.type == QDISK_TYPE_IMAGE ) || ( g_qdisk.type == QDISK_TYPE_UNICARD ) ) {
                                qdisk_write_byte_into_drive ( 0x00 );
                            };

                        } else if ( ( channel->Wreg[ QDSIO_REGADDR_5 ] & 0x1a ) == 0x0a ) {
                            /* signal preruseni vysilani + povoleno odesilani dat + signal RTS */
                            /* zapis synchronizacni znacky */

                            if ( ( g_qdisk.type == QDISK_TYPE_IMAGE ) || ( g_qdisk.type == QDISK_TYPE_UNICARD ) ) {
                                qdisk_write_byte_into_drive ( channel->Wreg[ QDSIO_REGADDR_6 ] );
                                qdisk_write_byte_into_drive ( channel->Wreg[ QDSIO_REGADDR_7 ] );
                            } else {
                                //printf ( "WR SYNC: ( %d, %d)\n", g_qdisk.virt_status, g_qdisk.image_position );

                                if ( ( ( g_qdisk.virt_status == QDISK_VRTSTS_QDHEADER ) && ( g_qdisk.image_position == 0 ) ) || ( g_qdisk.virt_status == QDISK_VRTSTS_FORMATING ) ) {

                                    g_qdisk.image_position = 3;
                                } else {

                                    g_qdisk.image_position = 3;

                                    if ( 0 != qdisk_test_disk_is_writeable ( ) ) {

                                        if ( g_qdisk.virt_status != QDISK_VRTSTS_WR_MZFHEAD ) {

                                            qdisk_virt_close_mzf ( );

                                            char *dirpath = cfgelement_get_text_value ( g_elm_virt_fp );
                                            const char *filepath = ui_utils_build_filepath ( dirpath, QDISK_VIRT_TEMP_FNAME );

                                            if ( FS_LAYER_FR_OK != FS_LAYER_FOPEN ( g_qdisk.mzf_fp, (char*) filepath, FS_LAYER_FMODE_W ) ) {
                                                DBGPRINTF ( DBGERR, "fopen()\n" );
#ifdef COMPILE_FOR_EMULATOR
                                                ui_show_error ( "%s() - Can't create open file '%s': %s", __func__, filepath, strerror ( errno ) );
#endif
                                            } else {
                                                ui_utils_free_filepath ( filepath );
                                                g_qdisk.virt_status = QDISK_VRTSTS_WR_MZFHEAD;
                                            };

                                        } else {
                                            g_qdisk.virt_status = QDISK_VRTSTS_WR_MZFBODY;

                                            unsigned len;
                                            uint8_t mzf_cmnt[ 104 - 38 ];

                                            memset ( mzf_cmnt, 0x00, sizeof ( mzf_cmnt ) );
                                            if ( FS_LAYER_FR_OK != FS_LAYER_FWRITE ( g_qdisk.mzf_fp, mzf_cmnt, sizeof ( mzf_cmnt ), &len ) ) {
                                                DBGPRINTF ( DBGERR, "fwrite() error\n" );
                                            };
                                        };

                                    };

                                };
                            }
                        };

                    };
                    break;

                case QDSIO_REGADDR_0:
                case QDSIO_REGADDR_1:
                case QDSIO_REGADDR_4:
                case QDSIO_REGADDR_6:
                case QDSIO_REGADDR_7:
                    break;

            };

            channel->REG_addr = QDSIO_REGADDR_0;
        };

    } else {
        if ( channel->name == 'A' ) {
            g_qdisk.out_crc16 ^= value;
            qdisk_write_byte_into_drive ( value );
        };
    };

}


void qdisk_deactivate_unicard_boot_loader ( void ) {
    if ( ( g_qdisk.connected == QDISK_CONNECTED ) && ( g_qdisk.type == QDISK_TYPE_UNICARD ) ) {
        qdisk_set_write_protected ( 0 );
        qdisk_close ( );
        g_qdisk.type = QDISK_TYPE_IMAGE;
        qdisk_open_image ( "" );
        qdisk_open ( );
        ui_qdisk_menu_update ( );
    };
}


void qdisk_activate_unicard_boot_loader ( void ) {
    if ( g_qdisk.connected == QDISK_CONNECTED ) {
        qdisk_close ( );
    };
    g_qdisk.connected = QDISK_CONNECTED;
    g_qdisk.type = QDISK_TYPE_UNICARD;
    qdisk_set_write_protected ( 1 );
    qdisk_open ( );
    ui_qdisk_menu_update ( );
}
