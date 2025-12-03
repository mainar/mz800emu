/* 
 * File:   zxtape.h
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 22. května 2018, 0:22
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


#ifndef ZXTAPE_H
#define ZXTAPE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "libs/cmt_stream/cmt_stream.h"
#include "libs/mztape/cmtspeed.h"

#define ZXTAPE_DEFAULT_BDSPEED 1400

    extern const en_CMTSPEED g_zxtape_speed[];


    typedef enum en_ZXTAPE_BLOCK_FLAG {
        ZXTAPE_BLOCK_FLAG_HEADER = 0x00,
        ZXTAPE_BLOCK_FLAG_DATA = 0xff,
    } en_ZXTAPE_BLOCK_FLAG;


    extern st_CMT_BITSTREAM* zxtape_create_cmt_bitstream_from_tapblock ( en_ZXTAPE_BLOCK_FLAG flag, uint8_t *data, uint16_t data_size, en_CMTSPEED cmtspeed, uint32_t rate );
    extern st_CMT_VSTREAM* zxtape_create_cmt_vstream_from_tapblock ( en_ZXTAPE_BLOCK_FLAG flag, uint8_t *data, uint16_t data_size, en_CMTSPEED cmtspeed, uint32_t rate );
    extern st_CMT_STREAM* zxtape_create_stream_from_tapblock ( en_ZXTAPE_BLOCK_FLAG flag, uint8_t *data, uint16_t data_size, en_CMTSPEED cmtspeed, uint32_t rate, en_CMT_STREAM_TYPE type );


#ifdef __cplusplus
}
#endif

#endif /* ZXTAPE_H */

