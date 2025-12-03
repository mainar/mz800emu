/* 
 * File:   zxtape.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 22. května 2018, 7:56
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
#include <math.h>

#include "libs/mztape/cmtspeed.h"
#include "libs/mztape/mztape.h"
#include "libs/cmt_stream/cmt_stream.h"

#include "gdg/gdg.h"
#include "zxtape.h"

/*
 * Hodnoty delek vsech pulzu byly namereny z Intercopy
 * 
 * 
 * 
 *             |------|
 *       ______|      |
 * 
 * 
 * Typy pulzu:
 *  - PILOT: 612.075 us, 612.085 us
 *  - SYNC: 145.475, 186.085
 *  - LONG: 483.795 us, 483.805 us
 *  - SHORT: 241.895 us, 241.905 us
 * 
 * Struktura:
 *  - 4032 PILOT pulzu
 *  - SYNC_HIGH
 *  - SYNC_LOW
 *  - Header Flag: 0x00
 *  - Header:
 *      uint8_t code (1 - 3)
 *      uint8_t name[10] (nevyuzita cast je vyplnena mezerami 0x20)
 *      uint16_t datablock_length;
 *      uint16_t param1
 *      uint16_t param2
 *  - Header checksum (XOR vsech bajtu z headeru)
 *  - nasleduje pauza s klidovym stavem signalu 1 (simulujeme jednou pulvlnou)
 *  - 1610 PILOT pulzu
 *  - Data Flag: 0xff
 *  - Data
 *  - Data checksum
 *  - nasleduje pauza s klidovym stavem signalu 1 (simulujeme jednou pulvlnou)
 * 
 * Datove bity jsou posilany od nejvyssiho k nejnizsimu.
 * Jako stop bit se pouziva jeden short!
 * 
 */

st_MZTAPE_PULSE_GDGTICS g_zxtape_pilot_gdgticks = { 10853, 10853 };
st_MZTAPE_PULSE_GDGTICS g_zxtape_sync_gdgticks = { 2571, 3299 };

st_MZTAPE_PULSES_GDGTICS g_zxtape_pulses_gdgticks = {
    { 8583, 8583 }, // long
    { 4291, 4291 }, // short
};

uint32_t g_zxtape_pause_ticks = 6118394;

#define ZXTAPE_HEADER_PILOT_LENGTH 4032
#define ZXTAPE_DATA_PILOT_LENGTH 1610

#define ZXTAPE_PILOT441 27
#define ZXTAPE_SYNCHIGH441 8
#define ZXTAPE_SYNCLOW441 9
#define ZXTAPE_SHORT441 11
#define ZXTAPE_LONG441 22

const en_CMTSPEED g_zxtape_speed[] = {
                                      CMTSPEED_1_1, // 1400 Bd
                                      CMTSPEED_9_7, // 1800 Bd
                                      CMTSPEED_3_2, // 2100 Bd
                                      CMTSPEED_25_14, // 2500 Bd
                                      CMTSPEED_NONE
};


static inline void zxtape_bitstream_add_wave ( st_CMT_BITSTREAM *bitstream, uint32_t *sample_position, int samples, int *polarity ) {
    while ( samples ) {
        cmt_bitstream_set_value_on_position ( bitstream, *sample_position, *polarity );
        *sample_position += 1;
        samples--;
    };
    *polarity = ( ~*polarity ) & 1;
}


st_CMT_BITSTREAM* zxtape_create_cmt_bitstream_from_tapblock ( en_ZXTAPE_BLOCK_FLAG flag, uint8_t *data, uint16_t data_size, en_CMTSPEED cmtspeed, uint32_t rate ) {

    /* prozatim natvrdo 44100, protoze tak mame pripravene delky pulzu */
    rate = 44100;

    uint32_t count_pilot_pulses = 0;
    uint32_t count_long_pulses = 0;
    uint32_t count_short_pulses = 0;

    int i;
    for ( i = 0; i < data_size; i++ ) {
        uint8_t byte = data[i];
        int j;
        for ( j = 0; j < 8; j++ ) {
            if ( byte & 0x80 ) {
                count_long_pulses++;
            } else {
                count_short_pulses++;
            };
            byte <<= 1;
        };
    };

    switch ( flag ) {
        case ZXTAPE_BLOCK_FLAG_HEADER:
            count_pilot_pulses = ZXTAPE_HEADER_PILOT_LENGTH;
            break;
        case ZXTAPE_BLOCK_FLAG_DATA:
            count_pilot_pulses = ZXTAPE_DATA_PILOT_LENGTH;
            break;
        default:
            fprintf ( stderr, "%s():%d - Unknown ZXtape block flag '%d'\n", __func__, __LINE__, flag );
            return NULL;
    };

    uint32_t stream_bitsize = ( count_pilot_pulses * ZXTAPE_PILOT441 * 2 ) + ZXTAPE_SYNCHIGH441 + ZXTAPE_SYNCLOW441 + ( count_long_pulses * ZXTAPE_LONG441 * 2 ) + ( count_short_pulses * ZXTAPE_SHORT441 * 2 ) + ZXTAPE_PILOT441;

    /*
     * Vytvoreni CMT_STREAM
     */
    uint32_t blocks = cmt_bitstream_compute_required_blocks_from_scans ( stream_bitsize );

    st_CMT_BITSTREAM *bitstream = cmt_bitstream_new ( rate, blocks, CMT_STREAM_POLARITY_NORMAL );
    if ( !bitstream ) {
        fprintf ( stderr, "%s():%d - Could create cmt bitstream\n", __func__, __LINE__ );
        return NULL;
    };

    uint32_t sample_position = 0;
    int polarity = 0;

    /*
     * Pilot
     */
    uint32_t pilot_pulse_length = round ( ZXTAPE_PILOT441 / g_cmtspeed_divisor[cmtspeed] );
    for ( i = 0; i < count_pilot_pulses; i++ ) {
        zxtape_bitstream_add_wave ( bitstream, &sample_position, pilot_pulse_length, &polarity );
        zxtape_bitstream_add_wave ( bitstream, &sample_position, pilot_pulse_length, &polarity );
    };

    /*
     * Sync
     */
    uint32_t synchigh_pulse_length = round ( ZXTAPE_SYNCHIGH441 / g_cmtspeed_divisor[cmtspeed] );
    uint32_t synclow_pulse_length = round ( ZXTAPE_SYNCLOW441 / g_cmtspeed_divisor[cmtspeed] );
    zxtape_bitstream_add_wave ( bitstream, &sample_position, synchigh_pulse_length, &polarity );
    zxtape_bitstream_add_wave ( bitstream, &sample_position, synclow_pulse_length, &polarity );

    /*
     * Data
     */
    uint32_t short_pulse_length = round ( ZXTAPE_SHORT441 / g_cmtspeed_divisor[cmtspeed] );
    uint32_t long_pulse_length = round ( ZXTAPE_LONG441 / g_cmtspeed_divisor[cmtspeed] );
    for ( i = 0; i < data_size; i++ ) {
        uint8_t byte = data[i];
        int j;
        for ( j = 0; j < 8; j++ ) {

            int wave_length;
            if ( byte & 0x80 ) {
                wave_length = long_pulse_length;
            } else {
                wave_length = short_pulse_length;
            };

            zxtape_bitstream_add_wave ( bitstream, &sample_position, wave_length, &polarity );
            zxtape_bitstream_add_wave ( bitstream, &sample_position, wave_length, &polarity );

            byte <<= 1;
        };
    };

    zxtape_bitstream_add_wave ( bitstream, &sample_position, ZXTAPE_PILOT441, &polarity );

    return bitstream;
}


/*
 * 
 * vstream
 * 
 * 
 */

static inline int zxtape_add_cmt_vstream_onestate_block ( st_CMT_VSTREAM* vstream, st_MZTAPE_PULSE_GDGTICS *gpulse, int count ) {
    int i;
    for ( i = 0; i < count; i++ ) {
        if ( EXIT_FAILURE == cmt_vstream_add_value ( vstream, 0, gpulse->low ) ) return EXIT_FAILURE;
        if ( EXIT_FAILURE == cmt_vstream_add_value ( vstream, 1, gpulse->high ) ) return EXIT_FAILURE;
    };
    return EXIT_SUCCESS;
}


static inline int zxtape_add_cmt_vstream_data_block ( st_CMT_VSTREAM* cmt_vstream, st_MZTAPE_PULSES_GDGTICS *gpulses, uint8_t *data, uint16_t size ) {
    int i;
    for ( i = 0; i < size; i++ ) {
        uint8_t byte = data[i];
        int bit;
        for ( bit = 0; bit < 8; bit++ ) {
            if ( byte & 0x80 ) {
                if ( EXIT_FAILURE == cmt_vstream_add_value ( cmt_vstream, 0, gpulses->long_pulse.low ) ) return EXIT_FAILURE;
                if ( EXIT_FAILURE == cmt_vstream_add_value ( cmt_vstream, 1, gpulses->long_pulse.high ) ) return EXIT_FAILURE;
            } else {
                if ( EXIT_FAILURE == cmt_vstream_add_value ( cmt_vstream, 0, gpulses->short_pulse.low ) ) return EXIT_FAILURE;
                if ( EXIT_FAILURE == cmt_vstream_add_value ( cmt_vstream, 1, gpulses->short_pulse.high ) ) return EXIT_FAILURE;
            };
            byte = byte << 1;
        };
    };
    return EXIT_SUCCESS;
}


st_CMT_VSTREAM* zxtape_create_cmt_vstream_from_tapblock ( en_ZXTAPE_BLOCK_FLAG flag, uint8_t *data, uint16_t data_size, en_CMTSPEED cmtspeed, uint32_t rate ) {

    st_CMT_VSTREAM *vstream = cmt_vstream_new ( rate, CMT_VSTREAM_BYTELENGTH16, 1, CMT_STREAM_POLARITY_NORMAL );
    if ( !vstream ) {
        fprintf ( stderr, "%s():%d - Could create cmt vstream\n", __func__, __LINE__ );
        return NULL;
    };

    double gdg_ticks_by_sample = (double) GDGCLK_BASE / rate;

    st_MZTAPE_PULSE_GDGTICS gpilot;
    gpilot.high = round ( ( (double) g_zxtape_pilot_gdgticks.high / gdg_ticks_by_sample ) / g_cmtspeed_divisor[cmtspeed] );
    gpilot.low = round ( ( (double) g_zxtape_pilot_gdgticks.low / gdg_ticks_by_sample ) / g_cmtspeed_divisor[cmtspeed] );

    st_MZTAPE_PULSE_GDGTICS gsync;
    gsync.high = round ( ( (double) g_zxtape_sync_gdgticks.high / gdg_ticks_by_sample ) / g_cmtspeed_divisor[cmtspeed] );
    gsync.low = round ( ( (double) g_zxtape_sync_gdgticks.low / gdg_ticks_by_sample ) / g_cmtspeed_divisor[cmtspeed] );

    st_MZTAPE_PULSES_GDGTICS gpulses;
    gpulses.long_pulse.high = round ( ( (double) g_zxtape_pulses_gdgticks.long_pulse.high / gdg_ticks_by_sample ) / g_cmtspeed_divisor[cmtspeed] );
    gpulses.long_pulse.low = round ( ( (double) g_zxtape_pulses_gdgticks.long_pulse.low / gdg_ticks_by_sample ) / g_cmtspeed_divisor[cmtspeed] );
    gpulses.short_pulse.high = round ( ( (double) g_zxtape_pulses_gdgticks.short_pulse.high / gdg_ticks_by_sample ) / g_cmtspeed_divisor[cmtspeed] );
    gpulses.short_pulse.low = round ( ( (double) g_zxtape_pulses_gdgticks.short_pulse.low / gdg_ticks_by_sample ) / g_cmtspeed_divisor[cmtspeed] );

    int pilot_length = 0;

    switch ( flag ) {
        case ZXTAPE_BLOCK_FLAG_HEADER:
            pilot_length = ZXTAPE_HEADER_PILOT_LENGTH;
            break;
        case ZXTAPE_BLOCK_FLAG_DATA:
            pilot_length = ZXTAPE_DATA_PILOT_LENGTH;
            break;
        default:
            fprintf ( stderr, "%s():%d - Unknown ZXtape block flag '%d'\n", __func__, __LINE__, flag );
            cmt_vstream_destroy ( vstream );
            return NULL;
    };

    if ( EXIT_FAILURE == zxtape_add_cmt_vstream_onestate_block ( vstream, &gpilot, pilot_length ) ) {
        fprintf ( stderr, "%s():%d - Error: can't create cmt vstream\n", __func__, __LINE__ );
        cmt_vstream_destroy ( vstream );
        return NULL;
    };

    if ( EXIT_FAILURE == zxtape_add_cmt_vstream_onestate_block ( vstream, &gsync, 1 ) ) {
        fprintf ( stderr, "%s():%d - Error: can't create cmt vstream\n", __func__, __LINE__ );
        cmt_vstream_destroy ( vstream );
        return NULL;
    };

    if ( EXIT_FAILURE == zxtape_add_cmt_vstream_data_block ( vstream, &gpulses, data, data_size ) ) {
        fprintf ( stderr, "%s():%d - Error: can't create cmt vstream\n", __func__, __LINE__ );
        cmt_vstream_destroy ( vstream );
        return NULL;
    };

    if ( EXIT_FAILURE == cmt_vstream_add_value ( vstream, 0, gpilot.low ) ) {
        fprintf ( stderr, "%s():%d - Error: can't create cmt vstream\n", __func__, __LINE__ );
        cmt_vstream_destroy ( vstream );
        return NULL;
    };

    return vstream;
}


st_CMT_STREAM* zxtape_create_stream_from_tapblock ( en_ZXTAPE_BLOCK_FLAG flag, uint8_t *data, uint16_t data_size, en_CMTSPEED cmtspeed, uint32_t rate, en_CMT_STREAM_TYPE type ) {

    st_CMT_STREAM *stream = cmt_stream_new ( type );
    if ( !stream ) {
        return NULL;
    };

    switch ( stream->stream_type ) {
        case CMT_STREAM_TYPE_BITSTREAM:
        {
#if 0
            /*
             * Ve funkci bitstream_from_tapblock mame natvrdo nastaven rate 44100,
             * narozdil od toho vstream_from_tapblock se umi prispusobit libovolne hodnote rate.
             * 
             */
            st_CMT_BITSTREAM *bitstream = zxtape_create_cmt_bitstream_from_tapblock ( flag, data, data_size, cmtspeed, rate );
            if ( !bitstream ) {
                fprintf ( stderr, "%s():%d - Can't create bitstream\n", __func__, __LINE__ );
                cmt_stream_destroy ( stream );
                return NULL;
            };
#else
            st_CMT_VSTREAM *vstream = zxtape_create_cmt_vstream_from_tapblock ( flag, data, data_size, cmtspeed, rate );
            if ( !vstream ) {
                fprintf ( stderr, "%s() - %d: Can't create vstream\n", __func__, __LINE__ );
                cmt_stream_destroy ( stream );
                return NULL;
            };

            st_CMT_BITSTREAM *bitstream = cmt_bitstream_new_from_vstream ( vstream, 0 );
            cmt_vstream_destroy ( vstream );

            if ( !bitstream ) {
                fprintf ( stderr, "%s():%d - Can't create bitstream\n", __func__, __LINE__ );
                cmt_stream_destroy ( stream );
                return NULL;
            };
#endif
            stream->str.bitstream = bitstream;
            break;
        }

        case CMT_STREAM_TYPE_VSTREAM:
        {
            st_CMT_VSTREAM *vstream = zxtape_create_cmt_vstream_from_tapblock ( flag, data, data_size, cmtspeed, rate );
            if ( !vstream ) {
                fprintf ( stderr, "%s() - %d: Can't create vstream\n", __func__, __LINE__ );
                cmt_stream_destroy ( stream );
                return NULL;
            };
            stream->str.vstream = vstream;
            break;
        }

        default:
            fprintf ( stderr, "%s():%d - Unknown stream type '%d'\n", __func__, __LINE__, stream->stream_type );
            cmt_stream_destroy ( stream );
            return NULL;
    };

    return stream;
}
