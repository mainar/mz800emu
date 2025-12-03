/* 
 * File:   wav.h
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 18. února 2017, 8:48
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


#ifndef WAV_H
#define WAV_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "libs/generic_driver/generic_driver.h"


#define WAV_TAG_RIFF    "RIFF"
#define WAV_TAG_WAVE    "WAVE"
#define WAV_TAG_FMT     "fmt "
#define WAV_TAG_DATA    "data"


    typedef struct st_WAV_RIFF_HEADER {
        uint8_t riff_tag[4]; // RIFF
        uint32_t overall_size; // n - 8
        uint8_t wave_tag[4]; // WAVE
    }
    st_WAV_RIFF_HEADER;


    typedef struct st_WAV_CHUNK_HEADER {
        uint8_t chunk_tag[4]; // "fmt |data|fact"
        uint32_t chunk_size; // n - 8
    }
    st_WAV_CHUNK_HEADER;


    typedef enum en_WAVE_FORMAT_CODE {
        WAVE_FORMAT_CODE_PCM = 0x0001, // PCM - pulzne kodovana modulace
        WAVE_FORMAT_CODE_IEEE_FLOAT = 0x0003, // IEEE float
        WAVE_FORMAT_CODE_ALAW = 0x0006, // 8-bit ITU-T G.711 A-law
        WAVE_FORMAT_CODE_MULAW = 0x0007, // 8-bit ITU-T G.711 µ-law
        WAVE_FORMAT_CODE_EXTENSIBLE = 0xfffe, // Determined by SubFormat
    } en_WAVE_FORMAT_CODE;


    typedef struct st_WAV_FMT16 {
        uint16_t format_code; // podporujeme jen PCM
        uint16_t channels;
        uint32_t sample_rate; // pocet vzorku za sekundu - 44 100, 48 100
        uint32_t bytes_per_sec; // (sample_rate * bits_per_sample * channes) / 8
        uint16_t block_size; // (bits_per_sample * channels) / 8
        uint16_t bits_per_sample;
    }
    st_WAV_FMT16;

#if 0


    typedef struct st_WAV_FMT18 {
        uint16_t format_code; // podporujeme jen PCM
        uint16_t channels;
        uint32_t sample_rate; // pocet vzorku za sekundu - 44 100, 48 100
        uint32_t bytes_per_sec; // (sample_rate * bits_per_sample * channes) / 8
        uint16_t block_size; // (bits_per_sample * channels) / 8
        uint16_t bits_per_sample;
        uint16_t size_of_extension; // 0 - 22
    }
    st_WAV_FMT18;


    typedef struct st_WAV_FMT40 {
        uint16_t format_code; // podporujeme jen PCM
        uint16_t channels;
        uint32_t sample_rate; // pocet vzorku za sekundu - 44 100, 48 100
        uint32_t bytes_per_sec; // (sample_rate * bits_per_sample * channes) / 8
        uint16_t block_size; // (bits_per_sample * channels) / 8
        uint16_t bits_per_sample;
        uint16_t size_of_extension; // 0 - 22
        uint16_t number_of_valid_bits;
        uint32_t speaker_position_mask;
        uint8_t sub_format[16];
    }
    st_WAV_FMT18;
#endif


    typedef struct st_WAV_SIMPLE_HEADER {
        uint16_t format_code;
        uint16_t channels;
        uint32_t sample_rate;
        uint32_t bytes_per_sec;
        uint16_t block_size; // celkova velikost jednoho samplu
        uint32_t blocks; // pocet samplu
        uint16_t bits_per_sample;
        uint32_t real_data_offset;
        uint32_t data_size;
        float count_sec; // delka zaznamu v sekundach
    } st_WAV_SIMPLE_HEADER;


#define WAV_MAX_BITS_PER_SAMPLE 64


    typedef enum en_WAV_POLARITY {
        WAV_POLARITY_NORMAL = 0,
        WAV_POLARITY_INVERTED,
    } en_WAV_POLARITY;


    extern void wav_simple_header_destroy ( st_WAV_SIMPLE_HEADER *simple_header );
    extern st_WAV_SIMPLE_HEADER* wav_simple_header_new_from_handler ( st_HANDLER *h );
    extern int wav_get_bit_value_of_sample ( st_HANDLER *h, st_WAV_SIMPLE_HEADER *simple_header, uint32_t sample_position, en_WAV_POLARITY polarity, int *bit_value );

#ifdef __cplusplus
}
#endif

#endif /* WAV_H */

