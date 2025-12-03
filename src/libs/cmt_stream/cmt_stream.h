/* 
 * File:   cmt_stream.h
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 7. května 2018, 10:23
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


#ifndef CMT_STREAM_H
#define CMT_STREAM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "libs/generic_driver/generic_driver.h"

#include "cmt_stream_all.h"

#define CMTSTREAM_DEFAULT_RATE 44100


    typedef enum en_CMT_STREAM_TYPE {
        CMT_STREAM_TYPE_BITSTREAM = 0,
        CMT_STREAM_TYPE_VSTREAM,
    } en_CMT_STREAM_TYPE;


#include "cmt_bitstream.h"
#include "cmt_vstream.h"


    typedef union un_CMT_STREAM {
        st_CMT_BITSTREAM *bitstream;
        st_CMT_VSTREAM *vstream;
    } un_CMT_STREAM;


    typedef struct st_CMT_STREAM {
        en_CMT_STREAM_TYPE stream_type;
        un_CMT_STREAM str;
    } st_CMT_STREAM;

    extern void cmt_stream_destroy ( st_CMT_STREAM *stream );
    extern st_CMT_STREAM* cmt_stream_new ( en_CMT_STREAM_TYPE type );
    extern st_CMT_STREAM* cmt_stream_new_from_wav ( st_HANDLER *h, en_CMT_STREAM_POLARITY polarity, en_CMT_STREAM_TYPE type );

    extern en_CMT_STREAM_TYPE cmt_stream_get_stream_type ( st_CMT_STREAM *stream );
    extern const char* cmt_stream_get_stream_type_txt ( st_CMT_STREAM *stream );
    extern uint32_t cmt_stream_get_size ( st_CMT_STREAM *stream );
    extern uint32_t cmt_stream_get_rate ( st_CMT_STREAM *stream );
    extern double cmt_stream_get_length ( st_CMT_STREAM *stream );
    extern uint64_t cmt_stream_get_count_scans ( st_CMT_STREAM *stream );
    extern double cmt_stream_get_scantime ( st_CMT_STREAM *stream );
    extern void cmt_stream_set_polarity ( st_CMT_STREAM *stream, en_CMT_STREAM_POLARITY polarity );
    extern int cmt_stream_save_wav ( st_CMT_STREAM *stream, uint32_t rate, char *filename );

#ifdef __cplusplus
}
#endif

#endif /* CMT_STREAM_H */

