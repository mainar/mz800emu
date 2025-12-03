/* 
 * File:   cmt_vstream.h
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 2. května 2018, 20:21
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


#ifndef CMT_VSTREAM_H
#define CMT_VSTREAM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "libs/generic_driver/generic_driver.h"
#include "libs/wav/wav.h"
#include "cmt_stream_all.h"


    typedef enum en_CMT_VSTREAM_BYTELENGTH {
        CMT_VSTREAM_BYTELENGTH8 = sizeof (uint8_t ),
        CMT_VSTREAM_BYTELENGTH16 = sizeof (uint16_t ),
        CMT_VSTREAM_BYTELENGTH32 = sizeof (uint32_t ),
    } en_CMT_VSTREAM_BYTELENGTH;


    typedef struct st_CMT_VSTREAM {
        uint32_t rate; //samplovaci frequence
        uint32_t scan_gdgticks;
        uint32_t msticks; // pocet scanu v 1 ms 
        double scan_time; // delka jednoho scanu = ( 1 / rate )
        double stream_length; // celkova doba streamu
        en_CMT_VSTREAM_BYTELENGTH min_byte_length; // nejmensi velikost jednoho eventu v bajtech [1|2|4]]
        uint32_t size; // pocet bajtu datove oblasti
        uint64_t scans;
        int start_value; // pocatecni hodnota [0|1]
        int last_set_value; // posledni hodnota [0|1]
        int last_event_byte_length; // pocet bajtu popisujicich posledni event [1|2|4]
        uint32_t last_read_position; // posledni bajt ze ktereho jsme cetli
        uint64_t last_read_position_sample; // pocet samplu, ktere predchazely poslednimu bajtu ze ktereho jsme cetli
        int last_read_value; // posledni hodnota [0|1]
        en_CMT_STREAM_POLARITY polarity;
        uint8_t *data;
    } st_CMT_VSTREAM;

    /*
     * Struktura dat:
     * 
     * uint8_t  - pocet samples po ktere byl signal v setrvalem stavu
     *          - pokud byl pocet samples vetsi jak 0xfe, tak uint8_t obsahuje 0xff a nasleduje uint16_t
     *          - pokud byl pocet samples vetsi jak 0xfffe, tak uint16_t obsahuje 0xffff a nasleduje uint32_t
     * 
     */

    extern st_CMT_VSTREAM* cmt_vstream_new ( uint32_t rate, en_CMT_VSTREAM_BYTELENGTH min_byte_length, int value, en_CMT_STREAM_POLARITY polarity );
    extern void cmt_vstream_destroy ( st_CMT_VSTREAM *cmt_vstream );
    extern void cmt_vstream_read_reset ( st_CMT_VSTREAM *cmt_vstream );

    extern int cmt_vstream_add_value ( st_CMT_VSTREAM *cmt_vstream, int value, uint32_t count_samples );

    extern void cmt_vstream_change_polarity ( st_CMT_VSTREAM *stream, en_CMT_STREAM_POLARITY polarity );

    extern uint32_t cmt_vstream_get_size ( st_CMT_VSTREAM *vstream );
    extern uint32_t cmt_vstream_get_rate ( st_CMT_VSTREAM *vstream );
    extern double cmt_vstream_get_length ( st_CMT_VSTREAM *vstream );
    extern uint64_t cmt_vstream_get_count_scans ( st_CMT_VSTREAM *vstream );
    double cmt_vstream_get_scantime ( st_CMT_VSTREAM *vstream );
    extern en_CMT_STREAM_POLARITY cmt_vstream_get_polarity ( st_CMT_VSTREAM *vstream );

    extern st_CMT_VSTREAM* cmt_vstream_new_from_wav ( st_HANDLER *h, en_CMT_STREAM_POLARITY polarity );


    static inline uint32_t cmt_vstream_get_last_read_event ( st_CMT_VSTREAM *cmt_vstream, int *event_byte_length ) {
        if ( cmt_vstream->min_byte_length == CMT_VSTREAM_BYTELENGTH8 ) {
            uint8_t value8 = cmt_vstream->data[cmt_vstream->last_read_position];
            *event_byte_length = 1;
            if ( value8 < 0xff ) return value8;
            uint16_t value16 = *( uint16_t* ) & cmt_vstream->data[( cmt_vstream->last_read_position + 1 )];
            *event_byte_length = 1 + 2;
            if ( value16 < 0xffff ) return value16;
            uint32_t value32 = *( uint32_t* ) & cmt_vstream->data[( cmt_vstream->last_read_position + 1 + 2 )];
            *event_byte_length = 1 + 2 + 4;
            return value32;
        } else if ( cmt_vstream->min_byte_length == CMT_VSTREAM_BYTELENGTH16 ) {
            uint16_t value16 = *( uint16_t* ) & cmt_vstream->data[cmt_vstream->last_read_position];
            *event_byte_length = 2;
            if ( value16 < 0xffff ) return value16;
            uint32_t value32 = *( uint32_t* ) & cmt_vstream->data[( cmt_vstream->last_read_position + 2 )];
            *event_byte_length = 2 + 4;
            return value32;
        };
        uint32_t value32 = *( uint32_t* ) & cmt_vstream->data[cmt_vstream->last_read_position];
        *event_byte_length = 4;
        return value32;
    }


    static inline int cmt_vstream_read_pulse ( st_CMT_VSTREAM *cmt_vstream, uint64_t *samples, int *value ) {
        if ( cmt_vstream->last_read_position >= cmt_vstream->size ) return EXIT_FAILURE;
        int event_byte_length = 0;
        uint64_t event_samples = cmt_vstream_get_last_read_event ( cmt_vstream, &event_byte_length );
        uint64_t read_samples = event_samples + cmt_vstream->last_read_position_sample;
        *value = cmt_vstream->last_read_value;
        *samples = event_samples;
        cmt_vstream->last_read_value = ~cmt_vstream->last_read_value & 1;
        cmt_vstream->last_read_position += event_byte_length;
        cmt_vstream->last_read_position_sample = read_samples;
        return EXIT_SUCCESS;
    }


    static inline int cmt_vstream_get_value ( st_CMT_VSTREAM *cmt_vstream, uint64_t samples, int *value ) {
        int event_byte_length = 0;
        while ( cmt_vstream->last_read_position < cmt_vstream->size ) {
            uint64_t event_samples = cmt_vstream_get_last_read_event ( cmt_vstream, &event_byte_length );
            uint64_t read_samples = event_samples + cmt_vstream->last_read_position_sample;
            if ( samples < read_samples ) {
                *value = cmt_vstream->last_read_value;
                return EXIT_SUCCESS;
            }
            cmt_vstream->last_read_value = ~cmt_vstream->last_read_value & 1;
            cmt_vstream->last_read_position += event_byte_length;
            cmt_vstream->last_read_position_sample = read_samples;
        };
        *value = cmt_vstream->last_read_value;
        return EXIT_FAILURE;
    }


#ifdef __cplusplus
}
#endif

#endif /* CMT_VSTREAM_H */

