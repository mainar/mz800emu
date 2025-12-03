/* 
 * File:   cmt_bitstream.h
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 29. dubna 2018, 13:32
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


#ifndef CMT_BITSTREAM_H
#define CMT_BITSTREAM_H

#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>
#include <assert.h>

#include "libs/generic_driver/generic_driver.h"
#include "libs/wav/wav.h"
#include "cmt_stream_all.h"
#include "cmt_vstream.h"


    /*
     * Kondenzovany WAV. 1 bit odpovida jednomu scanu.
     */
    typedef struct st_CMT_BITSTREAM {
        uint32_t rate; // vzorkovani 1/nnn
        double scan_time; // ( 1 / rate )
        double stream_length; // ( 1 / rate ) * scans NASTAVUJE SE POUZE V cmt_bitstream_new_from_wav() !!!
        uint32_t blocks; // pocet alokovanych datovych bloku
        uint32_t scans; // pocet obsazenych bitu
        en_CMT_STREAM_POLARITY polarity;
        uint32_t *data; // datove bloky
    } st_CMT_BITSTREAM;

#define CMT_BITSTREAM_BLOCK_SIZE ( sizeof ( uint32_t ) * 8 ) // pocet bitu na datovy blok (32)

    extern st_CMT_BITSTREAM* cmt_bitstream_new ( uint32_t rate, uint32_t blocks, en_CMT_STREAM_POLARITY polarity );
    extern st_CMT_BITSTREAM* cmt_bitstream_new_from_wav ( st_HANDLER *wav_handler, en_CMT_STREAM_POLARITY polarity );
    extern st_CMT_BITSTREAM* cmt_bitstream_new_from_vstream ( st_CMT_VSTREAM *vstream, uint32_t dst_rate );
    extern void cmt_bitstream_destroy ( st_CMT_BITSTREAM *stream );
    extern uint32_t cmt_bitstream_compute_required_blocks_from_scans ( uint32_t scans );
    extern int cmt_bitstream_create_wav ( st_HANDLER *wav_handler, st_CMT_BITSTREAM *stream );
    extern void cmt_bitstream_change_polarity ( st_CMT_BITSTREAM *stream, en_CMT_STREAM_POLARITY polarity );

    extern uint32_t cmt_bitstream_get_size ( st_CMT_BITSTREAM *bitstream );
    extern uint32_t cmt_bitstream_get_rate ( st_CMT_BITSTREAM *bitstream );
    extern double cmt_bitstream_get_length ( st_CMT_BITSTREAM *bitstream );
    extern uint64_t cmt_bitstream_get_count_scans ( st_CMT_BITSTREAM *bitstream );
    extern double cmt_bitstream_get_scantime ( st_CMT_BITSTREAM *bitstream );


    /**
     * 
     */
    static inline uint32_t cmt_bitstream_get_position_by_time ( st_CMT_BITSTREAM *stream, double scan_time ) {
        uint32_t position = scan_time / stream->scan_time;
        return position;
    }


    /**
     * 
     */
    static inline int cmt_bitstream_get_value_on_position ( st_CMT_BITSTREAM *stream, uint32_t position ) {
        assert ( position < stream->scans );
        assert ( stream->data != NULL );
        uint32_t block = position / CMT_BITSTREAM_BLOCK_SIZE;
        assert ( block < stream->blocks );
        return ( stream->data[block] >> ( position % CMT_BITSTREAM_BLOCK_SIZE ) ) & 1;
    }


    /**
     * 
     */
    static inline int cmt_bitstream_get_value_on_time ( st_CMT_BITSTREAM *stream, double scan_time ) {
        uint32_t position = cmt_bitstream_get_position_by_time ( stream, scan_time );
        return cmt_bitstream_get_value_on_position ( stream, position );
    }


    /**
     * 
     */
    static inline void cmt_bitstream_set_value_on_position ( st_CMT_BITSTREAM *stream, uint32_t position, int value ) {
        uint32_t block = position / CMT_BITSTREAM_BLOCK_SIZE;
        assert ( block < stream->blocks );
        assert ( stream->data != NULL );

        if ( value ) {
            stream->data[block] |= (uint32_t) 1 << ( position % CMT_BITSTREAM_BLOCK_SIZE );
        } else {
            stream->data[block] &= ~( (uint32_t) 1 << ( position % CMT_BITSTREAM_BLOCK_SIZE ) );
        };


        if ( !( position < stream->scans ) ) {
            stream->scans = position + 1;
            stream->stream_length = stream->scan_time * stream->scans;
        };
    }


    static inline void cmt_bitstream_set_value_on_time ( st_CMT_BITSTREAM *stream, double scan_time, int value ) {
        uint32_t position = cmt_bitstream_get_position_by_time ( stream, scan_time );
        cmt_bitstream_set_value_on_position ( stream, position, value );
    }


#ifdef __cplusplus
}
#endif

#endif /* CMT_BITSTREAM_H */

