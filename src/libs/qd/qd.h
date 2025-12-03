/* 
 * File:   qd.h
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 2. října 2016, 12:00
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


#ifndef QD_H
#define QD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define QD_MZF_FNAME_LENGTH 16  /* o 1 bajt kratsi, nez v MZF, protoze na 17 pozici vzdy musi byt 0x0d !!! */
#define QD_MZF_CMNT_LENGTH 38 /* o 66 bajtu kratsi, nez v MZF !!! */

#define QD_PREFIX_LENGTH    4
#define QD_PREFIX "\x00\x16\x16\xa5"

#define QD_SURFIX_LENGTH    4
#define QD_SURFIX "CRC" /* nemame zadny duvod pocitat skutecne CRC */

#define QD_FNAME_TERMINATOR 0x0d


    typedef enum en_QD_BLOCK_TYPE {
        QD_BLOCK_TYPE_HEADER = 0x00,
        QD_BLOCK_TYPE_DATA = 0x05,
    } en_QD_BLOCK_TYPE;


    typedef struct st_QD_IMAGE_HEADER {
        uint8_t prefix [ QD_PREFIX_LENGTH ];
        uint8_t blocks;
        uint8_t surfix [ QD_SURFIX_LENGTH ];
    } st_QD_IMAGE_HEADER;


    typedef struct st_QD_MZF_HEADER {
        uint8_t ftype;
        uint8_t fname [ QD_MZF_FNAME_LENGTH ];
        uint8_t fname_terminator;
        uint8_t unused[2];
        uint16_t fsize;
        uint16_t fstrt;
        uint16_t fexec;
        uint8_t cmnt [ QD_MZF_CMNT_LENGTH ];
    } st_QD_MZF_HEADER;


    typedef struct st_QD_BLOCK_START {
        uint8_t prefix [ QD_PREFIX_LENGTH ];
        en_QD_BLOCK_TYPE type;
        uint16_t size;
    } st_QD_BLOCK_START;


    typedef struct st_QD_BLOCK_END {
        uint8_t surfix [ QD_SURFIX_LENGTH ];
    } st_QD_BLOCK_END;


    typedef struct st_QD_BLOCK_MZF_HEADER {
        st_QD_BLOCK_START bs;
        st_QD_MZF_HEADER header;
        st_QD_BLOCK_END be;
    } st_QD_BLOCK_MZF_HEADER;

#ifdef __cplusplus
}
#endif

#endif /* QD_H */

