/* 
 * File:   cmt_vstream.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 2. května 2018, 20:20
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
#include <stdlib.h>
#include <math.h>

#include "ui/ui_utils.h"
#include "libs/generic_driver/generic_driver.h"
#include "libs/wav/wav.h"

#include "gdg/gdg.h"

#include "cmt_vstream.h"
#include "cmt_stream.h"


void cmt_vstream_destroy ( st_CMT_VSTREAM *stream ) {
    if ( !stream ) return;
    if ( stream->data ) ui_utils_mem_free ( stream->data );
    ui_utils_mem_free ( stream );
}


void cmt_vstream_read_reset ( st_CMT_VSTREAM *stream ) {
    stream->last_read_position = 0;
    stream->last_read_position_sample = 0;
    stream->last_read_value = stream->start_value;
}


st_CMT_VSTREAM* cmt_vstream_new ( uint32_t rate, en_CMT_VSTREAM_BYTELENGTH min_byte_length, int value, en_CMT_STREAM_POLARITY polarity ) {
    if ( !( ( min_byte_length == 1 ) || ( min_byte_length == 2 ) || ( min_byte_length == 4 ) ) ) {
        fprintf ( stderr, "%s():%d - Error - invalid min_byte_length (%d).\n", __func__, __LINE__, min_byte_length );
        return NULL;
    };
    if ( ( !rate ) || ( rate > GDGCLK_BASE ) ) {
        fprintf ( stderr, "%s(): %d: Error - bad sample rate (%u).\n", __func__, __LINE__, rate );
    };
    st_CMT_VSTREAM *stream = ui_utils_mem_alloc0 ( sizeof ( st_CMT_VSTREAM ) );
    if ( !stream ) {
        fprintf ( stderr, "%s():%d - Error - can't alocate memory (%u).\n", __func__, __LINE__, (int) sizeof ( st_CMT_VSTREAM ) );
        return NULL;
    };
    stream->rate = rate;
    stream->scan_time = (double) 1 / rate;
    stream->msticks = round ( 0.001 / ( (double) 1 / rate ) );
    stream->scan_gdgticks = round ( (double) GDGCLK_BASE / rate );
    stream->start_value = value & 1;
    stream->polarity = polarity;

    stream->data = ui_utils_mem_alloc0 ( min_byte_length );
    if ( !stream->data ) {
        fprintf ( stderr, "%s() - %d: Error - can't alocate memory (%u).\n", __func__, __LINE__, min_byte_length );
        cmt_vstream_destroy ( stream );
        return NULL;
    };

    stream->stream_length = 0;
    stream->min_byte_length = min_byte_length;
    stream->last_set_value = value & 1;
    //cmt_vstream->data[0] = 0;
    stream->size = min_byte_length;
    stream->last_event_byte_length = min_byte_length;

    cmt_vstream_read_reset ( stream );

    return stream;
}


static inline uint32_t cmt_vstream_get_last_event ( st_CMT_VSTREAM *cmt_vstream ) {
    if ( sizeof ( uint8_t ) == cmt_vstream->last_event_byte_length ) {
        return cmt_vstream->data[( cmt_vstream->size - cmt_vstream->last_event_byte_length )];
    } else if ( sizeof ( uint16_t ) == cmt_vstream->last_event_byte_length ) {
        return * ( uint16_t* ) & cmt_vstream->data[( cmt_vstream->size - cmt_vstream->last_event_byte_length )];
    };
    return * ( uint32_t* ) & cmt_vstream->data[( cmt_vstream->size - cmt_vstream->last_event_byte_length )];
}


static inline int cmt_vstream_set_last_event ( st_CMT_VSTREAM *cmt_vstream, uint32_t count_samples ) {

    if ( sizeof ( uint8_t ) == cmt_vstream->last_event_byte_length ) {

        if ( count_samples < 0xff ) {
            cmt_vstream->data[( cmt_vstream->size - cmt_vstream->last_event_byte_length )] = (uint8_t) count_samples;
            return EXIT_SUCCESS;
        } else {
            cmt_vstream->data[( cmt_vstream->size - cmt_vstream->last_event_byte_length )] = 0xff;
            cmt_vstream->data = ui_utils_mem_realloc ( cmt_vstream->data, cmt_vstream->size + sizeof ( uint16_t ) );
            if ( !cmt_vstream->data ) {
                fprintf ( stderr, "%s() - %d: Error - can't alocate memory (%u).\n", __func__, __LINE__, cmt_vstream->size + ( uint32_t ) sizeof ( uint16_t ) );
                return EXIT_FAILURE;
            };
            cmt_vstream->last_event_byte_length = sizeof ( uint16_t );
            cmt_vstream->size += sizeof ( uint16_t );
            return cmt_vstream_set_last_event ( cmt_vstream, count_samples );
        };

    } else if ( sizeof ( uint16_t ) == cmt_vstream->last_event_byte_length ) {

        uint16_t *pos = ( uint16_t* ) & cmt_vstream->data[( cmt_vstream->size - cmt_vstream->last_event_byte_length )];
        if ( count_samples < 0xffff ) {
            *pos = (uint16_t) count_samples;
            return EXIT_SUCCESS;
        } else {
            *pos = (uint16_t) 0xffff;
            cmt_vstream->data = ui_utils_mem_realloc ( cmt_vstream->data, cmt_vstream->size + sizeof ( uint32_t ) );
            if ( !cmt_vstream->data ) {
                fprintf ( stderr, "%s() - %d: Error - can't alocate memory (%u).\n", __func__, __LINE__, cmt_vstream->size + ( uint32_t ) sizeof ( uint32_t ) );
                return EXIT_FAILURE;
            };
            cmt_vstream->last_event_byte_length = sizeof ( uint32_t );
            cmt_vstream->size += sizeof ( uint32_t );
            return cmt_vstream_set_last_event ( cmt_vstream, count_samples );
        };

    };

    uint32_t *pos = ( uint32_t* ) & cmt_vstream->data[( cmt_vstream->size - cmt_vstream->last_event_byte_length )];
    *pos = (uint32_t) count_samples;
    return EXIT_SUCCESS;
}


int cmt_vstream_add_value ( st_CMT_VSTREAM *cmt_vstream, int value, uint32_t count_samples ) {

    if ( count_samples == 0 ) {
        fprintf ( stderr, "%s() - %d: Error - count samples can't be '0'.\n", __func__, __LINE__ );
        return EXIT_FAILURE;
    };

    if ( ( value & 1 ) == cmt_vstream->last_set_value ) {

        uint32_t last_event = cmt_vstream_get_last_event ( cmt_vstream );

        if ( count_samples >= ( 0xffffffff - last_event ) ) {
            fprintf ( stderr, "%s() - %d: Error - last event is bigger as '0xffffffff'.\n", __func__, __LINE__ );
            return EXIT_FAILURE;
        };

        cmt_vstream->stream_length += ( (double) count_samples * cmt_vstream->scan_time );
        cmt_vstream->scans += count_samples;

        return cmt_vstream_set_last_event ( cmt_vstream, ( last_event + count_samples ) );

    };

    cmt_vstream->data = ui_utils_mem_realloc ( cmt_vstream->data, cmt_vstream->size + cmt_vstream->min_byte_length );
    if ( !cmt_vstream->data ) {
        fprintf ( stderr, "%s() - %d: Error - can't alocate memory (%u).\n", __func__, __LINE__, cmt_vstream->size + (uint32_t) cmt_vstream->min_byte_length );
        return EXIT_FAILURE;
    };

    cmt_vstream->last_set_value = value & 1;

    cmt_vstream->last_event_byte_length = cmt_vstream->min_byte_length;
    cmt_vstream->size += cmt_vstream->min_byte_length;

    cmt_vstream->stream_length += ( (double) count_samples * cmt_vstream->scan_time );
    cmt_vstream->scans += count_samples;

    return cmt_vstream_set_last_event ( cmt_vstream, ( count_samples ) );
}


void cmt_vstream_change_polarity ( st_CMT_VSTREAM *stream, en_CMT_STREAM_POLARITY polarity ) {
    if ( stream->polarity == polarity ) return;
    stream->start_value = ~stream->start_value & 0x01;
}


uint32_t cmt_vstream_get_size ( st_CMT_VSTREAM *vstream ) {
    assert ( vstream != NULL );
    return vstream->size;
}


uint32_t cmt_vstream_get_rate ( st_CMT_VSTREAM *vstream ) {
    assert ( vstream != NULL );
    return vstream->rate;
}


double cmt_vstream_get_length ( st_CMT_VSTREAM *vstream ) {
    assert ( vstream != NULL );
    return vstream->stream_length;
}


uint64_t cmt_vstream_get_count_scans ( st_CMT_VSTREAM *vstream ) {
    assert ( vstream != NULL );
    return vstream->scans;
}


double cmt_vstream_get_scantime ( st_CMT_VSTREAM *vstream ) {
    assert ( vstream != NULL );
    return vstream->scan_time;
}


en_CMT_STREAM_POLARITY cmt_vstream_get_polarity ( st_CMT_VSTREAM *vstream ) {
    assert ( vstream != NULL );
    return vstream->polarity;
}


st_CMT_VSTREAM* cmt_vstream_new_from_wav ( st_HANDLER *h, en_CMT_STREAM_POLARITY polarity ) {

    st_WAV_SIMPLE_HEADER *sh = wav_simple_header_new_from_handler ( h );

    if ( !sh ) {
        fprintf ( stderr, "%s() - %d: Error - can't create wav_simple_header.\n", __func__, __LINE__ );
        return NULL;
    };

    if ( !sh->blocks ) {
        fprintf ( stderr, "%s() - %d: Error - wav is empty\n", __func__, __LINE__ );
        wav_simple_header_destroy ( sh );
        return NULL;
    };

    en_WAV_POLARITY wav_polarity = ( polarity == CMT_STREAM_POLARITY_NORMAL ) ? WAV_POLARITY_NORMAL : WAV_POLARITY_INVERTED;

    int bit_value = 0;
    int last_bit_value = 0;

    if ( EXIT_FAILURE == wav_get_bit_value_of_sample ( h, sh, 0, wav_polarity, &bit_value ) ) {
        fprintf ( stderr, "%s() - %d: Error - can't read sample.\n", __func__, __LINE__ );
        wav_simple_header_destroy ( sh );
        return NULL;
    };
    last_bit_value = bit_value;

    st_CMT_VSTREAM* vstream = cmt_vstream_new ( sh->sample_rate, CMT_VSTREAM_BYTELENGTH8, bit_value, polarity );
    if ( !vstream ) {
        fprintf ( stderr, "%s():%d - Could create cmt vstream\n", __func__, __LINE__ );
        return NULL;
    };

    uint32_t count_scans = 0;
    uint32_t i = 0;
    uint32_t last_saved_scan = 0;
    for ( i = 0; i < sh->blocks; i++ ) {
        if ( EXIT_FAILURE == wav_get_bit_value_of_sample ( h, sh, i, wav_polarity, &bit_value ) ) {
            fprintf ( stderr, "%s() - %d: Error - can't read sample.\n", __func__, __LINE__ );
            wav_simple_header_destroy ( sh );
            cmt_vstream_destroy ( vstream );
            return NULL;
        };
        count_scans++;

        if ( last_bit_value == bit_value ) continue;

        cmt_vstream_add_value ( vstream, bit_value, count_scans );
        last_bit_value = bit_value;
        count_scans = 0;
        last_saved_scan = i;
    };


    if ( last_saved_scan != ( i - 1 ) ) {
        if ( !count_scans ) count_scans++;
        cmt_vstream_add_value ( vstream, bit_value, count_scans );
    };

    wav_simple_header_destroy ( sh );

    return vstream;
}
