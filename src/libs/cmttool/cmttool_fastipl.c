/* 
 * File:   cmttool_fastipl.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 26. září 2018, 16:28
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
#include "cmttool_fastipl.h"

/*
 * InterCopy v2
 */
#define CMTTOOL_FASTIPL_V02_LOADER_SIZE 96

#define CMTTOOL_FASTIPL_V02_FTYPE  0xbb
#define CMTTOOL_FASTIPL_V02_FSIZE  0x0000
#define CMTTOOL_FASTIPL_V02_FSTRT  0x1200
#define CMTTOOL_FASTIPL_V02_FEXEC  0x1110

#define CMTTOOL_FASTIPL_V02_BLK_BLCOUNT  0x0018
#define CMTTOOL_FASTIPL_V02_BLK_READPOINT  0x0019
#define CMTTOOL_FASTIPL_V02_BLK_FSIZE  0x001a
#define CMTTOOL_FASTIPL_V02_BLK_FSTRT  0x001c
#define CMTTOOL_FASTIPL_V02_BLK_FEXEC  0x001e
#define CMTTOOL_FASTIPL_V02_BLK_LOADER  0x0020


/*
 * Obsah umisteny v komentari mzf headeru. Pred timto blokem je:
 * 
 * 0x1108 - pocet opakovani bloku
 * 0x1109 - prodleva pro readpoint
 * 0x110a - size, start, exec
 * 0x1110 -
 */
const uint8_t cmttool_fastipl_v02_loader[] = {
                                              0x3e, 0x08, 0xd3, 0xce, 0xcd, 0x3e, 0x07, 0x97,
                                              0x57, 0x5f, 0xcd, 0x08, 0x03, 0xcd, 0xbe, 0x02,
                                              0xd3, 0xe2, 0x1a, 0xd3, 0xe0, 0x12, 0x13, 0xcb,
                                              0x62, 0x28, 0xf5, 0x3e, 0xc3, 0x32, 0x1f, 0x06,
                                              0x21, 0x5c, 0x11, 0x22, 0x20, 0x06, 0x2a, 0x08,
                                              0x11, 0x7d, 0x32, 0x12, 0x05, 0x7c, 0x32, 0x4b,
                                              0x0a, 0x2a, 0x0a, 0x11, 0x22, 0x02, 0x11, 0xcd,
                                              0xf8, 0x04, 0xf5, 0x01, 0xcf, 0x06, 0xed, 0x59,
                                              0xf1, 0xd3, 0xe2, 0xda, 0xaa, 0xe9, 0x21, 0x0a,
                                              0x11, 0xc3, 0x08, 0xed, 0xc5, 0x3a, 0x10, 0x11,
                                              0xee, 0x0c, 0x32, 0x10, 0x11, 0x01, 0xcf, 0x06,
                                              0xed, 0x79, 0xc1, 0xc9, 0x31, 0x39, 0x38, 0x37,
};


/*
 * InterCopy v7, v7.2, v8, v8.2, v10.1, v10.2
 */
const uint8_t cmttool_fastipl_v07_loader[] = {
                                              0x3e, 0x08, 0xd3, 0xce, 0xcd, 0x3e, 0x07, 0x36,
                                              0x01, 0x97, 0x57, 0x5f, 0xcd, 0x08, 0x03, 0xcd,
                                              0xbe, 0x02, 0xd3, 0xe2, 0x1a, 0xd3, 0xe0, 0x12,
                                              0x13, 0xcb, 0x62, 0x28, 0xf5, 0x3e, 0xc3, 0x32,
                                              0x1f, 0x06, 0x21, 0x5c, 0x11, 0x22, 0x20, 0x06,
                                              0x2a, 0x08, 0x11, 0x7d, 0x32, 0x12, 0x05, 0x7c,
                                              0x32, 0x4b, 0x0a, 0x2a, 0x0a, 0x11, 0x22, 0x02,
                                              0x11, 0xcd, 0xf8, 0x04, 0x01, 0xcf, 0x06, 0xed,
                                              0x71, 0xd3, 0xe2, 0xda, 0xaa, 0xe9, 0x21, 0x0a,
                                              0x11, 0xc3, 0x08, 0xed, 0xc5, 0x3a, 0x10, 0x11,
                                              0xee, 0x0c, 0x32, 0x10, 0x11, 0x01, 0xcf, 0x06,
                                              0xed, 0x79, 0xc1, 0xc9, 0x31, 0x39, 0x38, 0x37
};


en_CMTTOOL_FASTIPL_VERSION cmttool_fastipl_test_loader ( st_MZF_HEADER *hdr ) {
    if ( !( ( hdr->ftype == CMTTOOL_FASTIPL_V02_FTYPE ) && ( hdr->fsize == CMTTOOL_FASTIPL_V02_FSIZE ) && ( hdr->fstrt == CMTTOOL_FASTIPL_V02_FSTRT ) && ( hdr->fexec == CMTTOOL_FASTIPL_V02_FEXEC ) ) ) return CMTTOOL_FASTIPL_VERSION_NONE;
    uint8_t *hdrdata = (uint8_t*) hdr;
    if ( 0 == memcmp ( &hdrdata[CMTTOOL_FASTIPL_V02_BLK_LOADER], cmttool_fastipl_v02_loader, CMTTOOL_FASTIPL_V02_LOADER_SIZE ) ) return CMTTOOL_FASTIPL_VERSION_02;
    if ( 0 == memcmp ( &hdrdata[CMTTOOL_FASTIPL_V02_BLK_LOADER], cmttool_fastipl_v07_loader, CMTTOOL_FASTIPL_V02_LOADER_SIZE ) ) return CMTTOOL_FASTIPL_VERSION_07;
    return CMTTOOL_FASTIPL_VERSION_NONE;
}


uint8_t cmttool_fastipl_get_readpoint ( st_MZF_HEADER *hdr ) {
    uint8_t *hdrdata = (uint8_t*) hdr;
    return hdrdata[CMTTOOL_FASTIPL_V02_BLK_READPOINT];
}


uint8_t cmttool_fastipl_get_blcount ( st_MZF_HEADER *hdr ) {
    uint8_t *hdrdata = (uint8_t*) hdr;
    return hdrdata[CMTTOOL_FASTIPL_V02_BLK_BLCOUNT];
}


uint16_t cmttool_fastipl_get_fsize ( st_MZF_HEADER *hdr ) {
    uint8_t *hdrdata = (uint8_t*) hdr;
    return hdrdata[CMTTOOL_FASTIPL_V02_BLK_FSIZE] | ( hdrdata[CMTTOOL_FASTIPL_V02_BLK_FSIZE + 1] << 8 );
}


uint16_t cmttool_fastipl_get_fstrt ( st_MZF_HEADER *hdr ) {
    uint8_t *hdrdata = (uint8_t*) hdr;
    return hdrdata[CMTTOOL_FASTIPL_V02_BLK_FSTRT] | ( hdrdata[CMTTOOL_FASTIPL_V02_BLK_FSTRT + 1] << 8 );
}


uint16_t cmttool_fastipl_get_fexec ( st_MZF_HEADER *hdr ) {
    uint8_t *hdrdata = (uint8_t*) hdr;
    return hdrdata[CMTTOOL_FASTIPL_V02_BLK_FEXEC] | ( hdrdata[CMTTOOL_FASTIPL_V02_BLK_FEXEC + 1] << 8 );
}


const char* cmttool_fastipl_get_version_txt ( en_CMTTOOL_FASTIPL_VERSION version ) {
    if ( version == CMTTOOL_FASTIPL_VERSION_02 ) return "v2";
    return "v7";
}
