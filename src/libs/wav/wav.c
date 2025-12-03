/* 
 * File:   wav.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 18. února 2017, 8:50
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
#include <string.h>
#include <assert.h>
#include <errno.h>

#include "ui/ui_utils.h"

#include "libs/endianity/endianity.h"
#include "libs/generic_driver/generic_driver.h"

#include "wav.h"


static int wav_check_riff_header ( st_HANDLER *h ) {

    uint8_t work_buffer [ sizeof ( st_WAV_RIFF_HEADER ) ];
    st_WAV_RIFF_HEADER *riff_hdr = NULL;

    if ( EXIT_SUCCESS != generic_driver_direct_read ( h, 0, (void**) &riff_hdr, &work_buffer, sizeof ( work_buffer ) ) ) return EXIT_FAILURE;

    if ( 0 != strncmp ( (char*) riff_hdr->riff_tag, WAV_TAG_RIFF, sizeof ( riff_hdr->riff_tag ) ) ) return EXIT_FAILURE;
    if ( 0 != strncmp ( (char*) riff_hdr->wave_tag, WAV_TAG_WAVE, sizeof ( riff_hdr->wave_tag ) ) ) return EXIT_FAILURE;

    return EXIT_SUCCESS;
}


static int wav_check_chunk_header ( st_HANDLER *h, uint32_t offset, const char *chunk_tag, uint32_t *chunk_size ) {

    uint8_t work_buffer [ sizeof ( st_WAV_CHUNK_HEADER ) ];
    st_WAV_CHUNK_HEADER *chunk_header = NULL;

    if ( EXIT_SUCCESS != generic_driver_direct_read ( h, offset, (void**) &chunk_header, &work_buffer, sizeof ( work_buffer ) ) ) return EXIT_FAILURE;

    if ( 0 != strncmp ( (char*) chunk_header->chunk_tag, chunk_tag, sizeof ( chunk_header->chunk_tag ) ) ) return EXIT_FAILURE;

    *chunk_size = endianity_bswap32_LE ( chunk_header->chunk_size );

    return EXIT_SUCCESS;
}


static int wav_read_simple_header ( st_HANDLER *h, st_WAV_SIMPLE_HEADER *sh ) {

    memset ( sh, 0x00, sizeof ( st_WAV_SIMPLE_HEADER ) );

    if ( EXIT_SUCCESS != wav_check_riff_header ( h ) ) {
        fprintf ( stderr, "%s() - %d Error: bad WAV format\n", __func__, __LINE__ );
        return EXIT_FAILURE;
    };

    uint32_t fmt_size = 0;
    uint32_t offset = sizeof ( st_WAV_RIFF_HEADER );
    if ( EXIT_SUCCESS != wav_check_chunk_header ( h, offset, WAV_TAG_FMT, &fmt_size ) ) {
        fprintf ( stderr, "%s() - %d Error: bad WAV format\n", __func__, __LINE__ );
        return EXIT_FAILURE;
    };

    offset += sizeof ( st_WAV_CHUNK_HEADER );

    uint8_t work_buffer [ sizeof ( st_WAV_FMT16 ) ];
    st_WAV_FMT16 *fmt = NULL;

    if ( EXIT_SUCCESS != generic_driver_direct_read ( h, offset, (void**) &fmt, &work_buffer, sizeof ( work_buffer ) ) ) return EXIT_FAILURE;

    if ( fmt->format_code != WAVE_FORMAT_CODE_PCM ) {
        fprintf ( stderr, "%s() - %d Error: only PCM format is supported, format_code = 0x%02x\n", __func__, __LINE__, fmt->format_code );
        return EXIT_FAILURE;
    };

    if ( !( ( fmt->bits_per_sample == 8 ) || ( fmt->bits_per_sample == 16 ) || ( fmt->bits_per_sample == 32 ) || ( fmt->bits_per_sample == 64 ) ) ) {
        fprintf ( stderr, "%s() - %d Error: unsupported bits per sample %d\n", __func__, __LINE__, fmt->bits_per_sample );
        return EXIT_FAILURE;
    };

    uint32_t data_size = 0;
    offset += fmt_size;
    if ( EXIT_SUCCESS != wav_check_chunk_header ( h, offset, WAV_TAG_DATA, &data_size ) ) {
        fprintf ( stderr, "%s() - %d Error: daata chunk not found\n", __func__, __LINE__ );
        return EXIT_FAILURE;
    };

    sh->format_code = endianity_bswap16_LE ( fmt->format_code );
    sh->channels = endianity_bswap16_LE ( fmt->channels );
    sh->sample_rate = endianity_bswap32_LE ( fmt->sample_rate );
    sh->bytes_per_sec = endianity_bswap32_LE ( fmt->bytes_per_sec );
    sh->block_size = endianity_bswap16_LE ( fmt->block_size );
    sh->bits_per_sample = endianity_bswap16_LE ( fmt->bits_per_sample );
    sh->real_data_offset = offset + sizeof ( st_WAV_CHUNK_HEADER );
    sh->data_size = data_size;
    sh->blocks = data_size / sh->block_size;
    sh->count_sec = ( (float) sh->data_size / sh->block_size ) / sh->sample_rate;


    return EXIT_SUCCESS;
}


void wav_simple_header_destroy ( st_WAV_SIMPLE_HEADER *simple_header ) {
    if ( !simple_header ) return;
    ui_utils_mem_free ( simple_header );
}


st_WAV_SIMPLE_HEADER* wav_simple_header_new_from_handler ( st_HANDLER *wav_handler ) {
    st_WAV_SIMPLE_HEADER *sh = ui_utils_mem_alloc0 ( sizeof ( st_WAV_SIMPLE_HEADER ) );
    if ( EXIT_SUCCESS != wav_read_simple_header ( wav_handler, sh ) ) {
        return NULL;
    };
    return sh;
}


int wav_get_bit_value_of_sample ( st_HANDLER *wav_handler, st_WAV_SIMPLE_HEADER *simple_header, uint32_t sample_position, en_WAV_POLARITY polarity, int *bit_value ) {

    st_HANDLER *h = wav_handler;
    st_WAV_SIMPLE_HEADER *sh = simple_header;

    int8_t buffer[( WAV_MAX_BITS_PER_SAMPLE / 8 )];
    void *buf = &buffer;
    int sample_bytes = ( sh->bits_per_sample / 8 );

    uint32_t offset = ( sample_position * sh->block_size ) + sh->real_data_offset;

    if ( EXIT_SUCCESS != generic_driver_read ( h, offset, buffer, sample_bytes ) ) {
        fprintf ( stderr, "%s():%d - Could not read sample: %s, %s\n", __func__, __LINE__, strerror ( errno ), generic_driver_error_message ( h, h->driver ) );
        return EXIT_FAILURE;
    };

    switch ( sh->bits_per_sample ) {

        case 8:
        {
            int8_t sample = buffer[0];
            if ( polarity == WAV_POLARITY_NORMAL ) {
                *bit_value = ( sample >= 0 ) ? 0 : 1;
            } else {
                *bit_value = ( sample > 0 ) ? 1 : 0;
            };
            break;
        }

        case 16:
        {
            int16_t sample = endianity_bswap16_LE ( * (int16_t*) buf );
            if ( polarity == WAV_POLARITY_NORMAL ) {
                *bit_value = ( sample >= 0 ) ? 0 : 1;
            } else {
                *bit_value = ( sample > 0 ) ? 1 : 0;
            };
            break;
        }

        case 32:
        {
            int32_t sample = endianity_bswap32_LE ( * (int32_t*) buf );
            if ( polarity == WAV_POLARITY_NORMAL ) {
                *bit_value = ( sample >= 0 ) ? 0 : 1;
            } else {
                *bit_value = ( sample > 0 ) ? 1 : 0;
            };
            break;
        }

        case 64:
        {
            int64_t sample = endianity_bswap64_LE ( * (int64_t*) buf );
            if ( polarity == WAV_POLARITY_NORMAL ) {
                *bit_value = ( sample >= 0 ) ? 0 : 1;
            } else {
                *bit_value = ( sample > 0 ) ? 1 : 0;
            };
            break;
        }

        default:
            fprintf ( stderr, "%s():%d - Unsupported bits_per_sample '%d'\n", __func__, __LINE__, sh->bits_per_sample );
            return EXIT_FAILURE;
    };

    return EXIT_SUCCESS;
}
