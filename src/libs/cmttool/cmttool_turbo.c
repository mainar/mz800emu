/* 
 * File:   cmttool_turbo.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 26. září 2018, 13:06
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
#include <stdint.h>
#include <string.h>

#include "libs/mzf/mzf.h"
#include "cmttool_turbo.h"


/*
 * TurboCopy 1.0
 */

#define CMTTOOL_TURBO_V10_LOADER_SIZE 60

#define CMTTOOL_TURBO_V10_FTYPE  0x01
#define CMTTOOL_TURBO_V10_FSIZE  0x0050
#define CMTTOOL_TURBO_V10_FSTRT  0xd400
#define CMTTOOL_TURBO_V10_FEXEC  0xd400

#define CMTTOOL_TURBO_V10_BLK_READPOINT  0x003c
#define CMTTOOL_TURBO_V10_BLK_BLCOUNT  0x003d
#define CMTTOOL_TURBO_V10_BLK_FSIZE  0x003e
#define CMTTOOL_TURBO_V10_BLK_FSTRT  0x0040
#define CMTTOOL_TURBO_V10_BLK_FEXEC  0x0042

/*
 * Obsah umisteny v body. Za timto blokem nasleduje:
 * 
 * 0xd43c - prodleva pro readpoint
 * 0xd43d - pocet opakovani bloku
 * 0xd43e - size, start, exec
 * 0xd444 - nejaky bordel ...
 * 
 */
const uint8_t cmttool_turbo_v10_loader[] = {
                                            0x3e, 0x08, 0xd3, 0xce, 0x21, 0x00, 0x00, 0xd3,
                                            0xe4, 0x7e, 0xd3, 0xe0, 0x77, 0x23, 0x7c, 0xfe,
                                            0x10, 0x20, 0xf4, 0x3a, 0x3c, 0xd4, 0x32, 0x4b,
                                            0x0a, 0x3a, 0x3d, 0xd4, 0x32, 0x12, 0x05, 0x21,
                                            0x3e, 0xd4, 0x11, 0x02, 0x11, 0x01, 0x06, 0x00,
                                            0xed, 0xb0, 0x2a, 0x04, 0x11, 0xd9, 0x21, 0x00,
                                            0x12, 0x22, 0x04, 0x11, 0xcd, 0x2a, 0x00, 0xd3,
                                            0xe4, 0xc3, 0x9a, 0xe9,
};


/*
 * TurboCopy 1.2, 1.21, 1.22
 */

#define CMTTOOL_TURBO_V12_LOADER_SIZE 75
#define CMTTOOL_TURBO_V12_HDRCMNT_SIZE 7

#define CMTTOOL_TURBO_V12_FTYPE  0x01
#define CMTTOOL_TURBO_V12_FSIZE  0x005a
#define CMTTOOL_TURBO_V12_FSTRT  0xd400
#define CMTTOOL_TURBO_V12_FEXEC  0xd400

#define CMTTOOL_TURBO_V12_BLK_READPOINT  0x004b
#define CMTTOOL_TURBO_V12_BLK_BLCOUNT  0x004c
#define CMTTOOL_TURBO_V12_BLK_FSIZE  0x004d
#define CMTTOOL_TURBO_V12_BLK_FSTRT  0x004f
#define CMTTOOL_TURBO_V12_BLK_FEXEC  0x0051
#define CMTTOOL_TURBO_V12_BLK_HDRCMNT  0x0053

/*
 * Obsah umisteny v commentu headeru
 */
const uint8_t cmttool_turbo_v12_hdr_comment[] = {
                                                 0x5b, 0x96, 0xa5, 0x9d, 0x9a, 0xb7, 0x5d
};

/*
 * Obsah umisteny v body. Za timto blokem nasleduje:
 * 
 * 0xd44b - prodleva pro readpoint (ROM = 0x52)
 * 0xd44c - pocet opakovani bloku
 * 0xd44d - size, start, exec
 * 0xd453 - 7 prvnich bajtu z puvodniho commentu
 * 
 */
const uint8_t cmttool_turbo_v12_loader[] = {
                                            0x3e, 0x08, 0xd3, 0xce, 0xe5, 0x21, 0x00, 0x00,
                                            0xd3, 0xe4, 0x7e, 0xd3, 0xe0, 0x77, 0x23, 0x7c,
                                            0xfe, 0x10, 0x20, 0xf4, 0x3a, 0x4b, 0xd4, 0x32,
                                            0x4b, 0x0a, 0x3a, 0x4c, 0xd4, 0x32, 0x12, 0x05,
                                            0x21, 0x4d, 0xd4, 0x11, 0x02, 0x11, 0x01, 0x0d,
                                            0x00, 0xed, 0xb0, 0xe1, 0x7c, 0xfe, 0xd4, 0x28,
                                            0x12, 0x2a, 0x04, 0x11, 0xd9, 0x21, 0x00, 0x12,
                                            0x22, 0x04, 0x11, 0xcd, 0x2a, 0x00, 0xd3, 0xe4,
                                            0xc3, 0x9a, 0xe9, 0xcd, 0x2a, 0x00, 0xd3, 0xe4,
                                            0xc3, 0x24, 0x01
};


static int cmttool_turbo10_test_loader ( st_MZF_HEADER *hdr, uint8_t *body ) {
    if ( !( ( hdr->ftype == CMTTOOL_TURBO_V10_FTYPE ) && ( hdr->fsize == CMTTOOL_TURBO_V10_FSIZE ) && ( hdr->fstrt == CMTTOOL_TURBO_V10_FSTRT ) && ( hdr->fexec == CMTTOOL_TURBO_V10_FEXEC ) ) ) return EXIT_FAILURE;
    if ( 0 != memcmp ( body, cmttool_turbo_v10_loader, CMTTOOL_TURBO_V10_LOADER_SIZE ) ) return EXIT_FAILURE;
    return EXIT_SUCCESS;
}


static int cmttool_turbo12_test_loader ( st_MZF_HEADER *hdr, uint8_t *body ) {
    if ( !( ( hdr->ftype == CMTTOOL_TURBO_V12_FTYPE ) && ( hdr->fsize == CMTTOOL_TURBO_V12_FSIZE ) && ( hdr->fstrt == CMTTOOL_TURBO_V12_FSTRT ) && ( hdr->fexec == CMTTOOL_TURBO_V12_FEXEC ) ) ) return EXIT_FAILURE;
    if ( 0 != memcmp ( hdr->cmnt, cmttool_turbo_v12_hdr_comment, CMTTOOL_TURBO_V12_HDRCMNT_SIZE ) ) return EXIT_FAILURE;
    if ( 0 != memcmp ( body, cmttool_turbo_v12_loader, CMTTOOL_TURBO_V12_LOADER_SIZE ) ) return EXIT_FAILURE;
    return EXIT_SUCCESS;
}


en_CMTTOOL_TURBO_VERSION cmttool_turbo_test_loader ( st_MZF_HEADER *hdr, uint8_t *body ) {
    if ( EXIT_SUCCESS == cmttool_turbo10_test_loader ( hdr, body ) ) return CMTTOOL_TURBO_VERSION_10;
    if ( EXIT_SUCCESS == cmttool_turbo12_test_loader ( hdr, body ) ) return CMTTOOL_TURBO_VERSION_12;
    return CMTTOOL_TURBO_VERSION_NONE;
}


uint8_t cmttool_turbo_get_readpoint ( uint8_t *body, en_CMTTOOL_TURBO_VERSION version ) {
    if ( version == CMTTOOL_TURBO_VERSION_10 ) return body[CMTTOOL_TURBO_V10_BLK_READPOINT];
    return body[CMTTOOL_TURBO_V12_BLK_READPOINT];
}


uint8_t cmttool_turbo_get_blcount ( uint8_t *body, en_CMTTOOL_TURBO_VERSION version ) {
    if ( version == CMTTOOL_TURBO_VERSION_10 ) return body[CMTTOOL_TURBO_V10_BLK_BLCOUNT];
    return body[CMTTOOL_TURBO_V12_BLK_BLCOUNT];
}


uint16_t cmttool_turbo_get_fsize ( uint8_t *body, en_CMTTOOL_TURBO_VERSION version ) {
    if ( version == CMTTOOL_TURBO_VERSION_10 ) return body[CMTTOOL_TURBO_V10_BLK_FSIZE] | ( body[CMTTOOL_TURBO_V10_BLK_FSIZE + 1] << 8 );
    return body[CMTTOOL_TURBO_V12_BLK_FSIZE] | ( body[CMTTOOL_TURBO_V12_BLK_FSIZE + 1] << 8 );
}


uint16_t cmttool_turbo_get_fstrt ( uint8_t *body, en_CMTTOOL_TURBO_VERSION version ) {
    if ( version == CMTTOOL_TURBO_VERSION_10 ) return body[CMTTOOL_TURBO_V10_BLK_FSTRT] | ( body[CMTTOOL_TURBO_V10_BLK_FSTRT + 1] << 8 );
    return body[CMTTOOL_TURBO_V12_BLK_FSTRT] | ( body[CMTTOOL_TURBO_V12_BLK_FSTRT + 1] << 8 );
}


uint16_t cmttool_turbo_get_fexec ( uint8_t *body, en_CMTTOOL_TURBO_VERSION version ) {
    if ( version == CMTTOOL_TURBO_VERSION_10 ) return body[CMTTOOL_TURBO_V10_BLK_FEXEC] | ( body[CMTTOOL_TURBO_V10_BLK_FEXEC + 1] << 8 );
    return body[CMTTOOL_TURBO_V12_BLK_FEXEC] | ( body[CMTTOOL_TURBO_V12_BLK_FEXEC + 1] << 8 );
}


const char* cmttool_turbo_get_version_txt ( en_CMTTOOL_TURBO_VERSION version ) {
    if ( version == CMTTOOL_TURBO_VERSION_10 ) return "v1.0";
    return "v1.2x";
}


void cmttool_turbo12_fix_hdrcmnt ( st_MZF_HEADER *hdr, uint8_t *body ) {
    memcpy ( hdr->cmnt, &body[CMTTOOL_TURBO_V12_BLK_HDRCMNT], CMTTOOL_TURBO_V12_HDRCMNT_SIZE );
}
