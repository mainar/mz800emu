/* 
 * File:   cmt_bitstream.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 29. dubna 2018, 13:37
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
#include <errno.h>
#include <string.h>
#include <assert.h>

#include "ui/ui_utils.h"

#include "libs/generic_driver/generic_driver.h"
#include "libs/wav/wav.h"
#include "libs/endianity/endianity.h"
#include "libs/mztape/mztape.h"

#include "cmt_bitstream.h"
#include "cmt_vstream.h"


void cmt_bitstream_destroy ( st_CMT_BITSTREAM *stream ) {
    if ( !stream ) return;
    if ( stream->data ) {
        ui_utils_mem_free ( stream->data );
    }
    ui_utils_mem_free ( stream );
}


st_CMT_BITSTREAM* cmt_bitstream_new ( uint32_t rate, uint32_t blocks, en_CMT_STREAM_POLARITY polarity ) {

    st_CMT_BITSTREAM *stream = ui_utils_mem_alloc0 ( sizeof ( st_CMT_BITSTREAM ) );

    if ( !stream ) {
        fprintf ( stderr, "%s() - %d, Error: can't alocate memory - strerr: %s\n", __func__, __LINE__, strerror ( errno ) );
        return NULL;
    };

    stream->rate = rate;
    stream->scan_time = ( 1 / (double) rate );
    stream->polarity = polarity;

    if ( blocks ) {
        stream->data = ui_utils_mem_alloc0 ( ( CMT_BITSTREAM_BLOCK_SIZE / 8 ) * blocks );
        if ( !stream->data ) {
            fprintf ( stderr, "%s() - %d, Error: can't alocate memory - strerr: %s\n", __func__, __LINE__, strerror ( errno ) );
            return NULL;
        };
        stream->blocks = blocks;
    };

    return stream;
}


st_CMT_BITSTREAM* cmt_bitstream_new_from_vstream ( st_CMT_VSTREAM *vstream, uint32_t dst_rate ) {

    uint32_t rate = cmt_vstream_get_rate ( vstream );
    if ( dst_rate == 0 ) dst_rate = rate;
    double step = (double) rate / dst_rate;

    uint64_t scans = cmt_vstream_get_count_scans ( vstream );
    uint64_t dst_scans = scans / step;
    if ( ( dst_scans * step ) < scans ) dst_scans++;
    uint32_t blocks = cmt_bitstream_compute_required_blocks_from_scans ( dst_scans );

    en_CMT_STREAM_POLARITY polarity = cmt_vstream_get_polarity ( vstream );
    st_CMT_BITSTREAM *stream = cmt_bitstream_new ( dst_rate, blocks, polarity );
    if ( !stream ) return NULL;

    int value = 0;
    uint32_t dst_position = 0;

    double next_scan = step;
    double total_samples = 0;
    uint64_t samples = 0;

    if ( dst_rate == rate ) {
        while ( EXIT_SUCCESS == cmt_vstream_read_pulse ( vstream, &samples, &value ) ) {
            uint64_t i;
            for ( i = 0; i < samples; i++ ) {
                cmt_bitstream_set_value_on_position ( stream, dst_position++, value );
            };
        };
    } else {
        int previous_value = 0;
        double scan_min_limit = step / 2;
        while ( EXIT_SUCCESS == cmt_vstream_read_pulse ( vstream, &samples, &value ) ) {

            double d_samples = samples;

            while ( ( total_samples + d_samples ) >= next_scan ) {

                double new_state_samples = ( next_scan - total_samples );
                total_samples += new_state_samples;
                d_samples -= new_state_samples;
                next_scan += step;

                int sample_value;
                if ( previous_value == value ) {
                    sample_value = value;
                } else {
                    sample_value = ( new_state_samples >= scan_min_limit ) ? value : previous_value;
                };
                cmt_bitstream_set_value_on_position ( stream, dst_position++, sample_value );
                //cmt_bitstream_set_value_on_position ( stream, dst_position++, value );
                
                previous_value = value;
            };

            previous_value = value;
            total_samples += d_samples;
        };
    };

    return stream;
}


uint32_t cmt_bitstream_compute_required_blocks_from_scans ( uint32_t scans ) {
    uint32_t required_blocks = scans / CMT_BITSTREAM_BLOCK_SIZE;
    if ( scans % CMT_BITSTREAM_BLOCK_SIZE ) required_blocks++;
    return required_blocks;
}


st_CMT_BITSTREAM* cmt_bitstream_new_from_wav ( st_HANDLER *wav_handler, en_CMT_STREAM_POLARITY polarity ) {

    st_HANDLER *h = wav_handler;

    st_WAV_SIMPLE_HEADER *sh = wav_simple_header_new_from_handler ( h );

    if ( !sh ) {
        fprintf ( stderr, "%s() - %d: Error - can't create wav_simple_header.\n", __func__, __LINE__ );
        return NULL;
    };

    uint32_t scans = sh->blocks;
    uint32_t blocks = cmt_bitstream_compute_required_blocks_from_scans ( scans );

    st_CMT_BITSTREAM *stream = cmt_bitstream_new ( sh->sample_rate, blocks, polarity );
    if ( !stream ) {
        fprintf ( stderr, "%s() - %d: Error - can't create cmt_bitstream.\n", __func__, __LINE__ );
        wav_simple_header_destroy ( sh );
        return NULL;
    }

    stream->scans = scans;
    stream->stream_length = scans * ( 1 / (double) stream->rate );
    en_WAV_POLARITY wav_polarity = ( polarity == CMT_STREAM_POLARITY_NORMAL ) ? WAV_POLARITY_NORMAL : WAV_POLARITY_INVERTED;

    uint32_t i;
    for ( i = 0; i < scans; i++ ) {
        int bit_value = 0;
        if ( EXIT_FAILURE == wav_get_bit_value_of_sample ( h, sh, i, wav_polarity, &bit_value ) ) {
            fprintf ( stderr, "%s() - %d: Error - can't read sample.\n", __func__, __LINE__ );
            wav_simple_header_destroy ( sh );
            cmt_bitstream_destroy ( stream );
            return NULL;
        };

        cmt_bitstream_set_value_on_position ( stream, i, bit_value );
    };

    wav_simple_header_destroy ( sh );

    return stream;
}


int cmt_bitstream_create_wav ( st_HANDLER *wav_handler, st_CMT_BITSTREAM *stream ) {

    st_HANDLER *h = wav_handler;

    uint32_t wav_size = sizeof ( st_WAV_RIFF_HEADER ) + sizeof ( st_WAV_CHUNK_HEADER ) + sizeof ( st_WAV_FMT16 ) + sizeof ( st_WAV_CHUNK_HEADER ) + stream->scans;

    st_WAV_RIFF_HEADER wavhdr;
    memcpy ( wavhdr.riff_tag, WAV_TAG_RIFF, sizeof ( wavhdr.riff_tag ) );
    memcpy ( wavhdr.wave_tag, WAV_TAG_WAVE, sizeof ( wavhdr.riff_tag ) );
    wavhdr.overall_size = endianity_bswap32_LE ( wav_size - sizeof ( st_WAV_RIFF_HEADER ) );

    uint32_t pos = 0;

    if ( EXIT_SUCCESS != generic_driver_write ( h, pos, &wavhdr, sizeof ( st_WAV_RIFF_HEADER ) ) ) {
        fprintf ( stderr, "%s():%d - Could not write: %s, %s\n", __func__, __LINE__, strerror ( errno ), generic_driver_error_message ( h, h->driver ) );
        return EXIT_FAILURE;
    };

    pos += sizeof ( st_WAV_RIFF_HEADER );

    st_WAV_CHUNK_HEADER fmtchunk;
    memcpy ( fmtchunk.chunk_tag, WAV_TAG_FMT, sizeof ( fmtchunk.chunk_tag ) );
    fmtchunk.chunk_size = endianity_bswap32_LE ( sizeof ( st_WAV_FMT16 ) );

    if ( EXIT_SUCCESS != generic_driver_write ( h, pos, &fmtchunk, sizeof ( st_WAV_CHUNK_HEADER ) ) ) {
        fprintf ( stderr, "%s():%d - Could not write: %s, %s\n", __func__, __LINE__, strerror ( errno ), generic_driver_error_message ( h, h->driver ) );
        return EXIT_FAILURE;
    };

    pos += sizeof ( st_WAV_CHUNK_HEADER );

    st_WAV_FMT16 fmt;
    fmt.format_code = endianity_bswap16_LE ( WAVE_FORMAT_CODE_PCM );
    fmt.channels = endianity_bswap16_LE ( 1 );
    fmt.sample_rate = endianity_bswap32_LE ( stream->rate );
    fmt.bytes_per_sec = endianity_bswap32_LE ( ( stream->rate * 8 * 1 ) / 8 );
    fmt.block_size = endianity_bswap16_LE ( ( 8 * 1 ) / 8 );
    fmt.bits_per_sample = endianity_bswap16_LE ( 8 );

    if ( EXIT_SUCCESS != generic_driver_write ( h, pos, &fmt, sizeof ( st_WAV_FMT16 ) ) ) {
        fprintf ( stderr, "%s():%d - Could not write: %s, %s\n", __func__, __LINE__, strerror ( errno ), generic_driver_error_message ( h, h->driver ) );
        return EXIT_FAILURE;
    };

    pos += sizeof ( st_WAV_FMT16 );

    st_WAV_CHUNK_HEADER datachunk;
    memcpy ( datachunk.chunk_tag, WAV_TAG_DATA, sizeof ( datachunk.chunk_tag ) );
    datachunk.chunk_size = endianity_bswap32_LE ( stream->scans );

    if ( EXIT_SUCCESS != generic_driver_write ( h, pos, &datachunk, sizeof ( st_WAV_CHUNK_HEADER ) ) ) {
        fprintf ( stderr, "%s():%d - Could not write: %s, %s\n", __func__, __LINE__, strerror ( errno ), generic_driver_error_message ( h, h->driver ) );
        return EXIT_FAILURE;
    };

    pos += sizeof ( st_WAV_CHUNK_HEADER );

    uint32_t i;
    for ( i = 0; i < stream->scans; i++ ) {

        int sample = cmt_bitstream_get_value_on_position ( stream, i );

        int8_t buffer = ( sample ) ? MZTAPE_WAV_LEVEL_HIGH : MZTAPE_WAV_LEVEL_LOW;

        if ( EXIT_SUCCESS != generic_driver_write ( h, pos++, &buffer, 1 ) ) {
            fprintf ( stderr, "%s():%d - Could not write: %s, %s\n", __func__, __LINE__, strerror ( errno ), generic_driver_error_message ( h, h->driver ) );
            return EXIT_FAILURE;
        };

    };

    return EXIT_SUCCESS;
}


void cmt_bitstream_change_polarity ( st_CMT_BITSTREAM *stream, en_CMT_STREAM_POLARITY polarity ) {
    if ( stream->polarity == polarity ) return;
    uint32_t i;
    for ( i = 0; i < stream->blocks; i++ ) {
        stream->data[i] = ~stream->data[i];
    };
}


uint32_t cmt_bitstream_get_size ( st_CMT_BITSTREAM *bitstream ) {
    assert ( bitstream != NULL );
    return (bitstream->scans / CMT_BITSTREAM_BLOCK_SIZE );
}


uint32_t cmt_bitstream_get_rate ( st_CMT_BITSTREAM *bitstream ) {
    assert ( bitstream != NULL );
    return bitstream->rate;
}


double cmt_bitstream_get_length ( st_CMT_BITSTREAM *bitstream ) {
    assert ( bitstream != NULL );
    return bitstream->stream_length;
}


uint64_t cmt_bitstream_get_count_scans ( st_CMT_BITSTREAM *bitstream ) {
    assert ( bitstream != NULL );
    return bitstream->scans;
}


double cmt_bitstream_get_scantime ( st_CMT_BITSTREAM *bitstream ) {
    assert ( bitstream != NULL );
    return bitstream->scan_time;
}
