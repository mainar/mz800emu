/* 
 * File:   mztape.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 24. dubna 2018, 12:30
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


/*
 * 
 *	Popis standardniho MZ-800 CMT zaznamu 1200 baudu:
 *
 *              ___________
 *              |         |
 *      Long:   |         |________
 *
 *               470us     494us
 *
 *  Sharp GDG T:   8320      8640
 *
 *  Real GDG T:    8335      8760
 *
 *
 *              ______
 *              |    |
 *      Short:  |    |____
 *
 *              240us  278us
 *
 *  Sharp GDG T:  4220   4590
 *
 *  Real GDG T:   4356   4930
 *
 *
 *
 *  Read point:  379us od nastupne hrany = 6721 GDG T
 *
 * 
 * Format z MZ-800 ROM:
 * 
 * - Long GAP: 22 000 short
 * - Long Tape Mark: 40 long + 40 short
 * - 1 Long
 * - HDR - Header1
 * - CHK - Check Sum (2 bajty)
 * - 2 Long (ve skutecnosti jde pouze o to, aby se tam vyskytovaly 2 nabezne hrany - delka neni moc dulezita)
 * - 256 Short
 * - HDR - Header2
 * - CHK - Check Sum (2 bajty)
 * - 1 Long
 * - Short GAP: 11 000 short
 * - Short Tape Mark: 20 long + 20 short
 * - 2 Long (ve skutecnosti jde pouze o to, aby se tam vyskytovaly 2 nabezne hrany - delka neni moc dulezita)
 * - FILE - Data1
 * - CHK - Check Sum (2 bajty)
 * - 1 Long
 * - 256 Short
 * - FILE - Data2
 * - CHK - Check Sum (2 bajty)
 * - 1 Long
 * 
 * 
 * - datove bajty (HDR, FILE a CHK) jsou posilany od nejvyssiho bity k nejnizsimu
 * 
 * - za kazdym datovym bajtem nasleduje 1 stop bit (long)
 * 
 * - CHK je odesilan jako big endian !!! tedy nejprve horni bajt a pak dolni
 * 
 * Fyzicky zaznam:
 * 
 * Log1 (i8255 PC01) ma na audio CMT napetovou uroven < 0
 * Log0 (i8255 PC01) ma na audio CMT napetovou uroven >= 0
 * 
 * MZ-800 - zmena polarity pomoci zadniho switche na vliv pouze na vstupni data, nikoliv na vystupni!
 * 
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "ui/ui_utils.h"
#include "libs/generic_driver/generic_driver.h"
#include "libs/endianity/endianity.h"
#include "libs/cmt_stream/cmt_stream.h"

#include "gdg/gdg.h"

#include "cmtspeed.h"
#include "mztape.h"

const en_CMTSPEED g_mztape_speed[] = {
                                      CMTSPEED_1_1, // 1200 Bd
                                      CMTSPEED_2_1, // 2400 Bd
                                      CMTSPEED_2_1_CPM, // 2400 Bd
                                      CMTSPEED_7_3, // 2800 Bd
                                      CMTSPEED_8_3, // 3200 Bd
                                      CMTSPEED_3_1, // 3600 Bd
                                      CMTSPEED_NONE
};

en_MZTAPE_BLOCK g_mztape_format_sharp_sane[] = {
                                                MZTAPE_BLOCK_LGAP,
                                                MZTAPE_BLOCK_LTM,
                                                MZTAPE_BLOCK_2L,
                                                MZTAPE_BLOCK_HDR,
                                                MZTAPE_BLOCK_CHKH,
                                                MZTAPE_BLOCK_2L,
                                                MZTAPE_BLOCK_SGAP,
                                                MZTAPE_BLOCK_STM,
                                                MZTAPE_BLOCK_2L,
                                                MZTAPE_BLOCK_FILE,
                                                MZTAPE_BLOCK_CHKF,
                                                MZTAPE_BLOCK_2L,
                                                MZTAPE_BLOCK_STOP
};

en_MZTAPE_BLOCK g_mztape_format_sharp[] = {
                                           MZTAPE_BLOCK_LGAP,
                                           MZTAPE_BLOCK_LTM,
                                           MZTAPE_BLOCK_2L,
                                           MZTAPE_BLOCK_HDR,
                                           MZTAPE_BLOCK_CHKH,
                                           MZTAPE_BLOCK_2L,
                                           MZTAPE_BLOCK_256S,
                                           MZTAPE_BLOCK_HDRC,
                                           MZTAPE_BLOCK_CHKH,
                                           MZTAPE_BLOCK_2L,
                                           MZTAPE_BLOCK_SGAP,
                                           MZTAPE_BLOCK_STM,
                                           MZTAPE_BLOCK_2L,
                                           MZTAPE_BLOCK_FILE,
                                           MZTAPE_BLOCK_CHKF,
                                           MZTAPE_BLOCK_2L,
                                           MZTAPE_BLOCK_256S,
                                           MZTAPE_BLOCK_FILEC,
                                           MZTAPE_BLOCK_CHKF,
                                           MZTAPE_BLOCK_2L,
                                           MZTAPE_BLOCK_STOP
};

const st_MZTAPE_PULSES_LENGTH g_mztape_pulses_700 = {
    { 0.000464, 0.000494, 0.000958 }, // LONG PULSE
    { 0.000240, 0.000264, 0.000504 }, // SHORT PULSE
};

const st_MZTAPE_PULSES_LENGTH g_mztape_pulses_800 = {
    { 0.000470, 0.000494, 0.000964 }, // LONG PULSE
    { 0.000240, 0.000278, 0.000518 }, // SHORT PULSE
};

// pulzy podle Intercopy 10.2
/*
const st_MZTAPE_PULSES_LENGTH g_mztape_pulses_800 = {
    { 0.000470, 0.000495, 0.000965 }, // LONG PULSE
    { 0.000235, 0.000264, 0.000499 }, // SHORT PULSE
};
 */

// pulzy podle Intercopy 10.2
st_MZTAPE_PULSES_GDGTICS g_mztape_pulses_gdgticks_800_intercopy = {
    { 8335, 8760 },
    { 4356, 4930 },
};

// pulzy podle cmt.com v cp/m
st_MZTAPE_PULSES_GDGTICS g_mztape_pulses_gdgticks_800_cmtcom = {
    { 9300, 8660 },
    { 5400, 4660 },
};

const st_MZTAPE_PULSES_LENGTH g_mztape_pulses_80B = {
    { 0.000333, 0.000334, 0.000667 }, // LONG PULSE
    { 0.000166750, 0.000166, 0.000332750 }, // SHORT PULSE
};


const st_MZTAPE_PULSES_LENGTH *g_pulses_length[] = {
                                                    &g_mztape_pulses_700,
                                                    &g_mztape_pulses_800,
                                                    &g_mztape_pulses_80B,
};


const st_MZTAPE_FORMAT g_format_mz800_sane = {
                                              MZTAPE_LGAP_LENGTH_SANE,
                                              MZTAPE_SGAP_LENGTH_SANE,
                                              MZTAPE_PULSESET_800,
                                              g_mztape_format_sharp_sane,
};

const st_MZTAPE_FORMAT g_format_mz800 = {
                                         MZTAPE_LGAP_LENGTH_DEFAULT,
                                         MZTAPE_SGAP_LENGTH,
                                         MZTAPE_PULSESET_800,
                                         g_mztape_format_sharp,
};

// poradi zachovat podle en_MZTAPE_FORMATSET !
const st_MZTAPE_FORMAT *g_formats[] = {
                                       &g_format_mz800_sane,
                                       &g_format_mz800,
};


static uint32_t mztape_compute_block_checksum_32t ( uint8_t *block, uint16_t size ) {
    uint32_t checksum = 0;
    while ( size-- ) {
        uint8_t byte = *block;
        int bit;
        for ( bit = 0; bit < 8; bit++ ) {
            if ( byte & 1 ) checksum++;
            byte >>= 1;
        };
        block++;
    };
    return checksum;
}


static void mztape_compute_pulses ( st_MZTAPE_MZF *mztmzf, en_MZTAPE_FORMATSET mztape_format, uint64_t *long_pulses, uint64_t *short_pulses ) {

    uint64_t total_long = 0;
    uint64_t total_short = 0;

    const en_MZTAPE_BLOCK *format = g_formats[mztape_format]->blocks;

    int i = 0;
    while ( format[i] != MZTAPE_BLOCK_STOP ) {

        switch ( format[i] ) {
            case MZTAPE_BLOCK_LGAP:
                total_short += g_formats[mztape_format]->lgap;
                break;

            case MZTAPE_BLOCK_SGAP:
                total_short += g_formats[mztape_format]->sgap;
                break;

            case MZTAPE_BLOCK_LTM:
                total_long += MZTAPE_LTM_LLENGTH;
                total_short += MZTAPE_LTM_SLENGTH;
                break;

            case MZTAPE_BLOCK_STM:
                total_long += MZTAPE_STM_LLENGTH;
                total_short += MZTAPE_STM_SLENGTH;
                break;

            case MZTAPE_BLOCK_2L:
                total_long += 2;
                break;

            case MZTAPE_BLOCK_256S:
                total_short += 256;
                break;

            case MZTAPE_BLOCK_HDR:
                total_long += mztmzf->chkh + sizeof ( st_MZF_HEADER ); // + stop bity
                total_short += ( sizeof ( st_MZF_HEADER ) * 8 ) - mztmzf->chkh;
                break;

            case MZTAPE_BLOCK_FILE:
                total_long += mztmzf->chkb + mztmzf->size; // + stop bity
                total_short += ( mztmzf->size * 8 ) - mztmzf->chkb;
                break;

            case MZTAPE_BLOCK_CHKH:
            {
                uint16_t chk = mztmzf->chkh;
                uint32_t chklong = mztape_compute_block_checksum_32t ( ( uint8_t* ) & chk, 2 );
                total_long += chklong + 2; // + stop bity
                total_short += 16 - chklong;
                break;
            }

            case MZTAPE_BLOCK_CHKF:
            {
                uint16_t chk = mztmzf->chkb;
                uint32_t chklong = mztape_compute_block_checksum_32t ( ( uint8_t* ) & chk, 2 );
                total_long += chklong + 2; // + stop bity
                total_short += 16 - chklong;
                break;
            }

            default:
                fprintf ( stderr, "%s() - Error: unknown block id=%d\n", __func__, format[i] );
        };
        i++;
    };

    *long_pulses = total_long;
    *short_pulses = total_short;
}


static void mztape_prepare_pulses_length ( st_MZTAPE_PULSES_LENGTH *pulses, en_MZTAPE_PULSESET pulseset, en_CMTSPEED mztape_speed ) {
    const st_MZTAPE_PULSES_LENGTH *src = g_pulses_length[pulseset];

    pulses->long_pulse.high = src->long_pulse.high / g_cmtspeed_divisor[mztape_speed];
    pulses->long_pulse.low = src->long_pulse.low / g_cmtspeed_divisor[mztape_speed];
    //pulses->long_pulse.low = pulses->long_pulse.high;
    pulses->long_pulse.total = pulses->long_pulse.high + pulses->long_pulse.low;

    pulses->short_pulse.high = src->short_pulse.high / g_cmtspeed_divisor[mztape_speed];
    pulses->short_pulse.low = src->short_pulse.low / g_cmtspeed_divisor[mztape_speed];
    //pulses->short_pulse.low = pulses->short_pulse.high;
    pulses->short_pulse.total = pulses->short_pulse.high + pulses->short_pulse.low;
}


static inline st_MZTAPE_PULSE_LENGTH* mztape_scan_onestate_block ( uint32_t *bit_counter, uint32_t block_size, st_MZTAPE_PULSE_LENGTH *block_pulse, int *flag_next_format_phase ) {

    st_MZTAPE_PULSE_LENGTH *pulse = NULL;

    if ( *bit_counter < block_size ) {
        pulse = block_pulse;
    } else {
        *flag_next_format_phase = 1;
    };

    *bit_counter += 1;
    return pulse;
}


static inline st_MZTAPE_PULSE_LENGTH* mztape_scan_twostate_block ( uint32_t *bit_counter, uint32_t block_lsize, uint32_t block_ssize, st_MZTAPE_PULSES_LENGTH *block_pulses, int *flag_next_format_phase ) {

    st_MZTAPE_PULSE_LENGTH *pulse = NULL;

    if ( *bit_counter < block_lsize ) {
        pulse = &block_pulses->long_pulse;
    } else if ( *bit_counter < ( block_lsize + block_ssize ) ) {
        pulse = &block_pulses->short_pulse;
    } else {
        *flag_next_format_phase = 1;
    };

    *bit_counter += 1;
    return pulse;
}


static inline st_MZTAPE_PULSE_LENGTH* mztape_scan_data_block ( uint32_t *bit_counter, uint32_t block_size, st_MZTAPE_PULSES_LENGTH *block_pulses, uint8_t *data, int *flag_next_format_phase ) {

    st_MZTAPE_PULSE_LENGTH *pulse = NULL;

    if ( *bit_counter < block_size ) {

        uint32_t byte_pos = *bit_counter / 9;
        int bit = *bit_counter % 9;

        if ( bit < 8 ) {
            uint8_t byte = data[byte_pos] << bit;

            if ( byte & 0x80 ) {
                pulse = &block_pulses->long_pulse;
            } else {
                pulse = &block_pulses->short_pulse;
            };
        } else {
            // stop bit
            pulse = &block_pulses->long_pulse;
        }

    } else {
        *flag_next_format_phase = 1;
    };

    *bit_counter += 1;
    return pulse;
}


void mztape_mztmzf_destroy ( st_MZTAPE_MZF *mztmzf ) {
    if ( !mztmzf ) return;
    ui_utils_mem_free ( mztmzf );
}


st_MZTAPE_MZF* mztape_create_mztapemzf ( st_HANDLER *mzf_handler, uint32_t offset ) {

    st_HANDLER *h = mzf_handler;

    st_MZTAPE_MZF *mztmzf = ui_utils_mem_alloc0 ( sizeof ( st_MZTAPE_MZF ) );

    if ( !mztmzf ) {
        fprintf ( stderr, "%s():%d - Could create mztape mzf\n", __func__, __LINE__ );
        return NULL;
    };

    st_MZF_HEADER mzfhdr;

    if ( EXIT_SUCCESS != mzf_read_header_on_offset ( h, offset, &mzfhdr ) ) {
        fprintf ( stderr, "%s():%d - Could not read MZF header\n", __func__, __LINE__ );
        mztape_mztmzf_destroy ( mztmzf );
        return NULL;
    };

    mztmzf->size = mzfhdr.fsize;

    if ( EXIT_SUCCESS != generic_driver_read ( h, offset, mztmzf->header, sizeof ( st_MZF_HEADER ) ) ) {
        fprintf ( stderr, "%s():%d - Could not read MZF header\n", __func__, __LINE__ );
        mztape_mztmzf_destroy ( mztmzf );
        return NULL;
    };

    mztmzf->body = ui_utils_mem_alloc ( mztmzf->size );

    if ( EXIT_SUCCESS != generic_driver_read ( h, offset + sizeof ( st_MZF_HEADER ), mztmzf->body, mzfhdr.fsize ) ) {
        fprintf ( stderr, "%s():%d - Could not read MZF body\n", __func__, __LINE__ );
        mztape_mztmzf_destroy ( mztmzf );
        ui_utils_mem_free ( mztmzf->body );
        return NULL;
    };

    mztmzf->chkh = mztape_compute_block_checksum_32t ( mztmzf->header, sizeof ( st_MZF_HEADER ) );
    mztmzf->chkb = mztape_compute_block_checksum_32t ( mztmzf->body, mztmzf->size );

    return mztmzf;
}


/**
 * 
 * CMT stream vytvoreny pres bitstream_from_mztmzf asi neni uplne nejpresnejsi,
 * protoze kdyz v nem vytvorim interkatate screen v rychlosti 3600 Bd, tak se nenacte.
 * Stejny soubor vytvoreny jako vstream_from_mztmzf a prevedeny na bitstream se v pohode nacita
 * 
 */
st_CMT_BITSTREAM* mztape_create_cmt_bitstream_from_mztmzf ( st_MZTAPE_MZF *mztmzf, en_MZTAPE_FORMATSET mztape_format, en_CMTSPEED mztape_speed, uint32_t sample_rate ) {

    double sample_length = (double) 1 / sample_rate;

    /*
     * Zjisteni poctu pulzu a priprava jejich delky
     */

    uint64_t long_pulses;
    uint64_t short_pulses;

    mztape_compute_pulses ( mztmzf, mztape_format, &long_pulses, &short_pulses );

    st_MZTAPE_PULSES_LENGTH pulses;

    mztape_prepare_pulses_length ( &pulses, g_formats[mztape_format]->pulseset, mztape_speed );

    double stream_length = ( long_pulses * pulses.long_pulse.total ) + ( short_pulses * pulses.short_pulse.total );

    uint32_t data_bitsize = stream_length / sample_length;

    /*
     * Vytvoreni CMT_STREAM
     */
    uint32_t blocks = cmt_bitstream_compute_required_blocks_from_scans ( data_bitsize );

    st_CMT_BITSTREAM *bitstream = cmt_bitstream_new ( sample_rate, blocks, CMT_STREAM_POLARITY_NORMAL );
    if ( !bitstream ) {
        fprintf ( stderr, "%s():%d - Could create cmt bitstream\n", __func__, __LINE__ );
        return NULL;
    };

    uint32_t sample_position = 0;
    uint32_t bit_counter = 0;
    double scan_time = 0;
    en_MZTAPE_BLOCK *format = g_formats[mztape_format]->blocks;
    int format_phase = 0;
    double pulse_time = 0;

    st_MZTAPE_PULSE_LENGTH *pulse = NULL;

    while ( scan_time <= stream_length ) {

        if ( ( pulse == NULL ) || ( pulse_time >= pulse->total ) ) {

            if ( pulse != NULL ) {
                pulse_time -= pulse->total;
            };

            int flag_skip_this_pulse = 0;

            do {

                int flag_next_format_phase;

                do {

                    flag_next_format_phase = 0;

                    switch ( format[format_phase] ) {

                        case MZTAPE_BLOCK_LGAP:
                            pulse = mztape_scan_onestate_block ( &bit_counter, g_formats[mztape_format]->lgap, &pulses.short_pulse, &flag_next_format_phase );
                            break;

                        case MZTAPE_BLOCK_SGAP:
                            pulse = mztape_scan_onestate_block ( &bit_counter, g_formats[mztape_format]->sgap, &pulses.short_pulse, &flag_next_format_phase );
                            break;

                        case MZTAPE_BLOCK_LTM:
                            pulse = mztape_scan_twostate_block ( &bit_counter, MZTAPE_LTM_LLENGTH, MZTAPE_LTM_SLENGTH, &pulses, &flag_next_format_phase );
                            break;

                        case MZTAPE_BLOCK_STM:
                            pulse = mztape_scan_twostate_block ( &bit_counter, MZTAPE_STM_LLENGTH, MZTAPE_STM_SLENGTH, &pulses, &flag_next_format_phase );
                            break;

                        case MZTAPE_BLOCK_2L:
                            pulse = mztape_scan_onestate_block ( &bit_counter, 2, &pulses.long_pulse, &flag_next_format_phase );
                            break;

                        case MZTAPE_BLOCK_256S:
                            pulse = mztape_scan_onestate_block ( &bit_counter, 256, &pulses.short_pulse, &flag_next_format_phase );
                            break;

                        case MZTAPE_BLOCK_HDR:
                        case MZTAPE_BLOCK_HDRC:
                            pulse = mztape_scan_data_block ( &bit_counter, ( sizeof ( st_MZF_HEADER ) * 8 ) + sizeof ( st_MZF_HEADER ), &pulses, mztmzf->header, &flag_next_format_phase );
                            break;

                        case MZTAPE_BLOCK_FILE:
                        case MZTAPE_BLOCK_FILEC:
                            pulse = mztape_scan_data_block ( &bit_counter, ( mztmzf->size * 8 ) + mztmzf->size, &pulses, mztmzf->body, &flag_next_format_phase );
                            break;

                        case MZTAPE_BLOCK_CHKH:
                        {
                            uint16_t chk = endianity_bswap16_BE ( mztmzf->chkh );
                            pulse = mztape_scan_data_block ( &bit_counter, 16 + 2, &pulses, ( uint8_t* ) & chk, &flag_next_format_phase );
                            break;
                        }

                        case MZTAPE_BLOCK_CHKF:
                        {
                            uint16_t chk = endianity_bswap16_BE ( mztmzf->chkb );
                            pulse = mztape_scan_data_block ( &bit_counter, 16 + 2, &pulses, ( uint8_t* ) & chk, &flag_next_format_phase );
                            break;
                        }

                        case MZTAPE_BLOCK_STOP:
                        default:
                            pulse = NULL;
                            break;
                    };

                    if ( flag_next_format_phase != 0 ) {
                        pulse = NULL;
                        bit_counter = 0;
                        format_phase++;
                    };

                } while ( flag_next_format_phase != 0 );


                if ( pulse != NULL ) {
                    if ( pulse_time >= pulse->total ) {
                        pulse_time -= pulse->total;
                        flag_skip_this_pulse = 1;
                    }
                } else {
                    flag_skip_this_pulse = 0;
                }

            } while ( flag_skip_this_pulse != 0 );

        };

        if ( pulse != NULL ) {
            int sample_value = ( pulse_time < pulse->high ) ? 1 : 0;
            cmt_bitstream_set_value_on_position ( bitstream, sample_position++, sample_value );
            pulse_time += sample_length;
        };

        scan_time += sample_length;
    }

    return bitstream;
}


static inline int mztape_add_cmt_vstream_onestate_block ( st_CMT_VSTREAM* vstream, st_MZTAPE_PULSE_GDGTICS *gpulse, int count ) {
    int i;
    for ( i = 0; i < count; i++ ) {
        if ( EXIT_FAILURE == cmt_vstream_add_value ( vstream, 1, gpulse->high ) ) return EXIT_FAILURE;
        if ( EXIT_FAILURE == cmt_vstream_add_value ( vstream, 0, gpulse->low ) ) return EXIT_FAILURE;
    };
    return EXIT_SUCCESS;
}


static inline int mztape_add_cmt_vstream_data_block ( st_CMT_VSTREAM* vstream, st_MZTAPE_PULSES_GDGTICS *gpulses, uint8_t *data, uint16_t size ) {
    int i;
    for ( i = 0; i < size; i++ ) {
        uint8_t byte = data[i];
        int bit;
        for ( bit = 0; bit < 8; bit++ ) {
            if ( byte & 0x80 ) {
                if ( EXIT_FAILURE == cmt_vstream_add_value ( vstream, 1, gpulses->long_pulse.high ) ) return EXIT_FAILURE;
                if ( EXIT_FAILURE == cmt_vstream_add_value ( vstream, 0, gpulses->long_pulse.low ) ) return EXIT_FAILURE;
            } else {
                if ( EXIT_FAILURE == cmt_vstream_add_value ( vstream, 1, gpulses->short_pulse.high ) ) return EXIT_FAILURE;
                if ( EXIT_FAILURE == cmt_vstream_add_value ( vstream, 0, gpulses->short_pulse.low ) ) return EXIT_FAILURE;
            };
            byte = byte << 1;
        };
        if ( EXIT_FAILURE == cmt_vstream_add_value ( vstream, 1, gpulses->long_pulse.high ) ) return EXIT_FAILURE;
        if ( EXIT_FAILURE == cmt_vstream_add_value ( vstream, 0, gpulses->long_pulse.low ) ) return EXIT_FAILURE;
    };
    return EXIT_SUCCESS;
}


st_CMT_VSTREAM* mztape_create_cmt_vstream_from_mztmzf ( st_MZTAPE_MZF *mztmzf, en_MZTAPE_FORMATSET mztape_format, en_CMTSPEED mztape_speed, uint32_t rate ) {

    st_CMT_VSTREAM* vstream = cmt_vstream_new ( rate, CMT_VSTREAM_BYTELENGTH8, 1, CMT_STREAM_POLARITY_NORMAL );
    if ( !vstream ) {
        fprintf ( stderr, "%s():%d - Could create cmt vstream\n", __func__, __LINE__ );
        return NULL;
    };

    st_MZTAPE_PULSES_GDGTICS gpulses;

    double gdg_ticks_by_sample = (double) GDGCLK_BASE / rate;

    st_MZTAPE_PULSES_GDGTICS *srcpulses = ( mztape_speed == CMTSPEED_2_1_CPM ) ? &g_mztape_pulses_gdgticks_800_cmtcom : &g_mztape_pulses_gdgticks_800_intercopy;

#if 1
    gpulses.long_pulse.high = round ( ( (double) srcpulses->long_pulse.high / gdg_ticks_by_sample ) / g_cmtspeed_divisor[mztape_speed] );
    gpulses.long_pulse.low = round ( ( (double) srcpulses->long_pulse.low / gdg_ticks_by_sample ) / g_cmtspeed_divisor[mztape_speed] );
    gpulses.short_pulse.high = round ( ( (double) srcpulses->short_pulse.high / gdg_ticks_by_sample ) / g_cmtspeed_divisor[mztape_speed] );
    gpulses.short_pulse.low = round ( ( (double) srcpulses->short_pulse.low / gdg_ticks_by_sample ) / g_cmtspeed_divisor[mztape_speed] );
#else
    gpulses.long_pulse.high = ( ( ( srcpulses->long_pulse.high / gdg_ticks_by_sample / 5 ) * 1200 ) / cmtspeed_get_bdspeed ( mztape_speed, 1200 ) ) * 5;
    gpulses.long_pulse.low = ( ( ( srcpulses->long_pulse.low / gdg_ticks_by_sample / 5 ) * 1200 ) / cmtspeed_get_bdspeed ( mztape_speed, 1200 ) ) * 5;
    gpulses.short_pulse.high = ( ( ( srcpulses->short_pulse.high / gdg_ticks_by_sample / 5 ) * 1200 ) / cmtspeed_get_bdspeed ( mztape_speed, 1200 ) ) * 5;
    gpulses.short_pulse.low = ( ( ( srcpulses->short_pulse.high / gdg_ticks_by_sample / 5 ) * 1200 ) / cmtspeed_get_bdspeed ( mztape_speed, 1200 ) ) * 5;
#endif

    //printf ( "Long - H: %d, L: %d\n", gpulses.long_pulse.high, gpulses.long_pulse.low );
    //printf ( "Long - H: %d, L: %d\n", gpulses.short_pulse.high, gpulses.short_pulse.low );

    const en_MZTAPE_BLOCK *format = g_formats[mztape_format]->blocks;

    int i = 0;
    int ret;
    while ( format[i] != MZTAPE_BLOCK_STOP ) {

        switch ( format[i] ) {
            case MZTAPE_BLOCK_LGAP:
                ret = mztape_add_cmt_vstream_onestate_block ( vstream, &gpulses.short_pulse, g_formats[mztape_format]->lgap );
                break;

            case MZTAPE_BLOCK_SGAP:
                ret = mztape_add_cmt_vstream_onestate_block ( vstream, &gpulses.short_pulse, g_formats[mztape_format]->sgap );
                break;

            case MZTAPE_BLOCK_LTM:
                ret = mztape_add_cmt_vstream_onestate_block ( vstream, &gpulses.long_pulse, MZTAPE_LTM_LLENGTH );
                if ( ret != EXIT_FAILURE ) {
                    ret = mztape_add_cmt_vstream_onestate_block ( vstream, &gpulses.short_pulse, MZTAPE_LTM_SLENGTH );
                };
                break;

            case MZTAPE_BLOCK_STM:
                ret = mztape_add_cmt_vstream_onestate_block ( vstream, &gpulses.long_pulse, MZTAPE_STM_LLENGTH );
                if ( ret != EXIT_FAILURE ) {
                    ret = mztape_add_cmt_vstream_onestate_block ( vstream, &gpulses.short_pulse, MZTAPE_STM_SLENGTH );
                };
                break;

            case MZTAPE_BLOCK_2L:
                ret = mztape_add_cmt_vstream_onestate_block ( vstream, &gpulses.long_pulse, 2 );
                break;

            case MZTAPE_BLOCK_256S:
                ret = mztape_add_cmt_vstream_onestate_block ( vstream, &gpulses.short_pulse, 256 );
                break;

            case MZTAPE_BLOCK_HDR:
            case MZTAPE_BLOCK_HDRC:
                ret = mztape_add_cmt_vstream_data_block ( vstream, &gpulses, mztmzf->header, sizeof ( st_MZF_HEADER ) );
                break;

            case MZTAPE_BLOCK_FILE:
            case MZTAPE_BLOCK_FILEC:
                ret = mztape_add_cmt_vstream_data_block ( vstream, &gpulses, mztmzf->body, mztmzf->size );
                break;

            case MZTAPE_BLOCK_CHKH:
            {
                uint16_t chk = endianity_bswap16_BE ( mztmzf->chkh );
                ret = mztape_add_cmt_vstream_data_block ( vstream, &gpulses, ( uint8_t* ) & chk, 2 );
                break;
            }

            case MZTAPE_BLOCK_CHKF:
            {
                uint16_t chk = endianity_bswap16_BE ( mztmzf->chkb );
                ret = mztape_add_cmt_vstream_data_block ( vstream, &gpulses, ( uint8_t* ) & chk, 2 );
                break;
            }

            default:
                fprintf ( stderr, "%s():%d - Error: unknown block id=%d\n", __func__, __LINE__, format[i] );
                ret = EXIT_FAILURE;
        };

        if ( ret == EXIT_FAILURE ) {
            fprintf ( stderr, "%s():%d - Error: can't create cmt vstream\n", __func__, __LINE__ );
            cmt_vstream_destroy ( vstream );
            return NULL;
        };

        i++;
    };

    return vstream;
}


st_CMT_STREAM* mztape_create_stream_from_mztapemzf ( st_MZTAPE_MZF *mztmzf, en_CMTSPEED cmtspeed, en_CMT_STREAM_TYPE type, en_MZTAPE_FORMATSET mztape_fset, uint32_t rate ) {

    st_CMT_STREAM *stream = cmt_stream_new ( type );
    if ( !stream ) {
        return NULL;
    };

    switch ( stream->stream_type ) {
        case CMT_STREAM_TYPE_BITSTREAM:
        {
#if 0
            /*
             * CMT stream vytvoreny pres bitstream_from_mztmzf asi neni uplne nejpresnejsi,
             * protoze kdyz v nem vytvorim interkatate screen v rychlosti 3600 Bd, tak se nenacte.
             * Stejny soubor vytvoreny jako vstream_from_mztmzf a prevedeny na bitstream se v pohode nacita
             * 
             */
            st_CMT_BITSTREAM *bitstream = mztape_create_cmt_bitstream_from_mztmzf ( mztmzf, mztape_fset, cmtspeed, rate );
            if ( !bitstream ) {
                fprintf ( stderr, "%s():%d - Can't create bitstream\n", __func__, __LINE__ );
                cmt_stream_destroy ( stream );
                return NULL;
            };
#else
            st_CMT_VSTREAM *vstream = mztape_create_cmt_vstream_from_mztmzf ( mztmzf, mztape_fset, cmtspeed, rate );
            if ( !vstream ) {
                fprintf ( stderr, "%s() - %d: Can't create vstream\n", __func__, __LINE__ );
                cmt_stream_destroy ( stream );
                return NULL;
            };

            st_CMT_BITSTREAM *bitstream = cmt_bitstream_new_from_vstream ( vstream, rate );
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
            st_CMT_VSTREAM *vstream = mztape_create_cmt_vstream_from_mztmzf ( mztmzf, mztape_fset, cmtspeed, rate );
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

