/* 
 * File:   audio.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 28. července 2015, 13:38
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

#include <math.h>
#include <stdio.h>

#include "mz800emu_cfg.h"
#include "audio.h"
#include "psg/psg.h"
#include "gdg/gdg.h"

#include "ui/ui_audio.h"
#include "cfgmain.h"

st_AUDIO g_audio;

AUDIO_BUF_t g_attenuator_volume_value [ PSG_CHANNELS_COUNT ] [ PSG_OUT_OFF + 1 ];
AUDIO_BUF_t g_8253_volume_value;

#ifdef AUDIO_FILLBUFF_v2

st_AUDIO_CTC g_audio_ctc;

#endif

#define DEFAULT_VOLUME 50 // 0...100


void audio_set_8253_volume ( int volume ) {
    g_8253_volume_value = ( ( (float) AUDIO_MAXVAL_PER_CHANNEL / 100 ) * volume );
}


void audio_set_psg_attenuator ( int channel, int volume ) {
    int i;
    for ( i = 0; i <= PSG_OUT_OFF; i++ ) {
        g_attenuator_volume_value[channel][i] = ( ( (float) AUDIO_MAXVAL_PER_CHANNEL / 100 ) * volume ) * pow ( 10, -( (float) i / 10 ) );
    };
}


void audio_setup_type ( en_AUDIO_VOLUME_SETUP type ) {
    g_audio.volume_setup = type;

    if ( type == AUDIO_VOLUME_SETUP_ALL ) {
        printf ( "Audio volume setup: all-in-one (%d%%)\n", g_audio.volume_master );
        audio_set_8253_volume ( g_audio.volume_master );
        int i;
        for ( i = 0; i < PSG_CHANNELS_COUNT; i++ ) {
            audio_set_psg_attenuator ( i, g_audio.volume_master );
        };
    } else {
        printf ( "Audio volume setup: per-channel (%d%%, %d%%, %d%%, %d%%, %d%%)\n", g_audio.volume_8253, g_audio.volume_psg0, g_audio.volume_psg1, g_audio.volume_psg2, g_audio.volume_psg3 );
        audio_set_8253_volume ( g_audio.volume_8253 );
        audio_set_psg_attenuator ( 0, g_audio.volume_psg0 );
        audio_set_psg_attenuator ( 1, g_audio.volume_psg1 );
        audio_set_psg_attenuator ( 2, g_audio.volume_psg2 );
        audio_set_psg_attenuator ( 3, g_audio.volume_psg3 );
    };
}


void audio_set_volume_master ( int volume ) {
    g_audio.volume_master = volume;

    audio_set_8253_volume ( g_audio.volume_master );
    int i;
    for ( i = 0; i < PSG_CHANNELS_COUNT; i++ ) {
        audio_set_psg_attenuator ( i, g_audio.volume_master );
    };

    if ( g_audio.volume_setup == AUDIO_VOLUME_SETUP_PER_CHANNEL ) {
        g_audio.volume_8253 = volume;
        g_audio.volume_psg0 = volume;
        g_audio.volume_psg1 = volume;
        g_audio.volume_psg2 = volume;
        g_audio.volume_psg3 = volume;
    };
}


void audio_set_volume_8253 ( int volume ) {
    g_audio.volume_8253 = volume;
    audio_set_8253_volume ( volume );
}


void audio_set_volume_psg0 ( int volume ) {
    g_audio.volume_psg0 = volume;
    audio_set_psg_attenuator ( 0, volume );
}


void audio_set_volume_psg1 ( int volume ) {
    g_audio.volume_psg1 = volume;
    audio_set_psg_attenuator ( 1, volume );
}


void audio_set_volume_psg2 ( int volume ) {
    g_audio.volume_psg2 = volume;
    audio_set_psg_attenuator ( 2, volume );
}


void audio_set_volume_psg3 ( int volume ) {
    g_audio.volume_psg3 = volume;
    audio_set_psg_attenuator ( 3, volume );
}


void audio_init ( void ) {

    CFGMOD *cmod = cfgroot_register_new_module ( g_cfgmain, "AUDIO" );

    CFGELM *elm;
    elm = cfgmodule_register_new_element ( cmod, "volume_setup", CFGENTYPE_KEYWORD, AUDIO_VOLUME_SETUP_ALL,
                                           AUDIO_VOLUME_SETUP_ALL, "ALL",
                                           AUDIO_VOLUME_SETUP_PER_CHANNEL, "PER_CHANNEL",
                                           -1 );
    cfgelement_set_handlers ( elm, (void*) &g_audio.volume_setup, (void*) &g_audio.volume_setup );

    elm = cfgmodule_register_new_element ( cmod, "volume_master", CFGENTYPE_UNSIGNED, DEFAULT_VOLUME, 0, 100 );
    cfgelement_set_handlers ( elm, (void*) &g_audio.volume_master, (void*) &g_audio.volume_master );

    elm = cfgmodule_register_new_element ( cmod, "volume_8253", CFGENTYPE_UNSIGNED, DEFAULT_VOLUME, 0, 100 );
    cfgelement_set_handlers ( elm, (void*) &g_audio.volume_8253, (void*) &g_audio.volume_8253 );

    elm = cfgmodule_register_new_element ( cmod, "volume_psg0", CFGENTYPE_UNSIGNED, DEFAULT_VOLUME, 0, 100 );
    cfgelement_set_handlers ( elm, (void*) &g_audio.volume_psg0, (void*) &g_audio.volume_psg0 );

    elm = cfgmodule_register_new_element ( cmod, "volume_psg1", CFGENTYPE_UNSIGNED, DEFAULT_VOLUME, 0, 100 );
    cfgelement_set_handlers ( elm, (void*) &g_audio.volume_psg1, (void*) &g_audio.volume_psg1 );

    elm = cfgmodule_register_new_element ( cmod, "volume_psg2", CFGENTYPE_UNSIGNED, DEFAULT_VOLUME, 0, 100 );
    cfgelement_set_handlers ( elm, (void*) &g_audio.volume_psg2, (void*) &g_audio.volume_psg2 );

    elm = cfgmodule_register_new_element ( cmod, "volume_psg3", CFGENTYPE_UNSIGNED, DEFAULT_VOLUME, 0, 100 );
    cfgelement_set_handlers ( elm, (void*) &g_audio.volume_psg3, (void*) &g_audio.volume_psg3 );

    cfgmodule_parse ( cmod );
    cfgmodule_propagate ( cmod );

#ifdef AUDIO_FILLBUFF_v2
    g_audio_ctc.samples[0].timestamp = 0;
    g_audio_ctc.samples[0].state = 0;
    g_audio_ctc.count = 0;
#endif

    g_audio.last_update = 0;
    g_audio.resample_timer = AUDIO_RESAMPLE_PERIOD;
    g_audio.buffer_position = 0;
    g_audio.ctc0_output = 0;
    g_audio.last_value = 0;

    audio_setup_type ( g_audio.volume_setup );

    ui_audio_init ( );
}


void audio_ctc0_changed ( unsigned value, unsigned event_ticks ) {
    if ( g_audio.ctc0_output == value ) return;

#ifdef AUDIO_FILLBUFF_v1
    audio_fill_buffer_v1 ( event_ticks );
#endif

    g_audio.ctc0_output = value;

#ifdef AUDIO_FILLBUFF_v2
    unsigned total_ticks = gdg_compute_total_ticks ( event_ticks );
    g_audio_ctc.count++;
    g_audio_ctc.samples[g_audio_ctc.count].timestamp = total_ticks;
    g_audio_ctc.samples[g_audio_ctc.count].state = ( value ) ? AUDIO_MAXVAL_PER_CHANNEL : 0;
#endif
}


static inline AUDIO_BUF_t psg_audio_scan ( void ) {

    AUDIO_BUF_t scan_value = 0;

    unsigned channel;
    for ( channel = 0; channel < PSG_CHANNELS_COUNT; channel++ ) {
        if ( g_psg.channel [ channel ].attn != PSG_OUT_OFF ) {
            if ( g_psg.channel [ channel ].output_signal ) {
                //if ( g_psg.channel [ channel ].attn == PSG_OUT_MAX ) {
                //    scan_value += AUDIO_MAXVAL_PER_CHANNEL;
                //} else {
                scan_value += g_attenuator_volume_value[channel][g_psg.channel[channel].attn];
                //};
            };
        };
    };

    return scan_value;
}

#ifdef AUDIO_FILLBUFF_v1


void audio_fill_buffer_v1 ( unsigned event_ticks ) {

    //printf ( "fill: %d, %d, %d\n", event_ticks, g_audio.last_update, g_audio.buffer_position );

    //static AUDIO_BUF_t last_value = 0;

    if ( event_ticks > ( VIDEO_SCREEN_TICKS ) ) {
        event_ticks = ( VIDEO_SCREEN_TICKS );
    };

    if ( ( event_ticks - g_audio.last_update ) < PSG_DIVIDER ) return;

    do {

        //printf ( "step: %d\n", gdg_compute_total_ticks ( g_audio.last_update ) );

        psg_step ( );

        /*
         * IIR filtr:
         * 
         *  OUT [ i + 1] = OUT [ i ] + ( IN [ i + 1] - OUT [ i ]) / x
         * 
         * x = vzorkovaci_frq / ( 2 * pi * delici_frq )
         * 
         */
        AUDIO_BUF_t scan_value = psg_audio_scan ( ) + g_audio.ctc0_output * g_8253_volume_value;
        g_audio.last_value = g_audio.last_value + ( scan_value - g_audio.last_value ) / 16;


        if ( g_audio.resample_timer <= PSG_DIVIDER ) {
            if ( g_audio.buffer_position < IFACE_AUDIO_20MS_SAMPLES ) {
                //printf ( "res: %d = %d\n", g_audio.buffer_position, last_value );
                if ( !TEST_EMULATION_PAUSED ) {
                    g_audio.buffer [ g_audio.buffer_position ] = g_audio.last_value;
                };
                g_audio.buffer_position++;
            };
            g_audio.resample_timer += AUDIO_RESAMPLE_PERIOD;
        };

        g_audio.resample_timer -= PSG_DIVIDER;
        g_audio.last_update += PSG_DIVIDER;

    } while ( g_audio.last_update < event_ticks );
}
#endif




#ifdef AUDIO_FILLBUFF_v2

#define AUDIO_SCAN_PERIOD   PSG_DIVIDER
//#define AUDIO_SCAN_PERIOD   GDGCLK_1M1_DIVIDER


void audio_fill_buffer_v2 ( unsigned now_total_ticks ) {

    unsigned ctc0_buf_pos = 0;
    unsigned psg_buf_pos;
    unsigned dst_sample_pos = 0;

    static unsigned already_scaned = 0;

    static unsigned last_scan_end = 0;

    unsigned start_scan_time;

    if ( g_psg_audio.samples[0].value != -1 ) {
        if ( ( g_audio_ctc.samples[0].timestamp - last_scan_end ) < ( g_psg_audio.samples[0].timestamp - last_scan_end ) ) {
            start_scan_time = g_audio_ctc.samples[0].timestamp;
        } else {
            start_scan_time = g_psg_audio.samples[0].timestamp;
        };
        psg_buf_pos = 0;
    } else {
        start_scan_time = g_audio_ctc.samples[0].timestamp;
        psg_buf_pos = -1;
    };

    unsigned samples_width = now_total_ticks - start_scan_time;
    unsigned current_scan_time = start_scan_time;

    static AUDIO_BUF_t audio_last_value = 0;

    unsigned resample_width = samples_width / IFACE_AUDIO_20MS_SAMPLES;

    int ctc0_current_state_width = g_audio_ctc.samples[ctc0_buf_pos + 1].timestamp - current_scan_time;
    AUDIO_BUF_t ctc0_current_value = g_audio_ctc.samples[0].state;

    //    printf ( "FILL: %d - %d = %d, %d, %d, %d\n", now_total_ticks, start_scan_time, samples_width, g_audio_ctc.count, g_psg_audio.count, g_psg_audio.samples[0].value );

    while ( 1 ) {

        if ( psg_buf_pos <= g_psg_audio.count ) {

            unsigned tst1 = current_scan_time - start_scan_time;
            unsigned tst2 = g_psg_audio.samples[psg_buf_pos].timestamp - start_scan_time;

            while ( ( tst1 >= tst2 ) && ( psg_buf_pos <= g_psg_audio.count ) ) {
                //                printf ( "REAL WRITE: %d - %d (%d): 0x%02x\n", current_scan_time, g_psg_audio.samples[psg_buf_pos].timestamp, current_scan_time - g_psg_audio.samples[psg_buf_pos].timestamp,  g_psg_audio.samples[psg_buf_pos].value );
                //  printf ( "REAL WRITE: %d: 0x%02x\n", g_psg_audio.samples[psg_buf_pos].timestamp, g_psg_audio.samples[psg_buf_pos].value );
                psg_real_write_byte ( g_psg_audio.samples[psg_buf_pos++].value );
                tst2 = g_psg_audio.samples[psg_buf_pos].timestamp - start_scan_time;
            };
        };

        //printf ( "step: %d\n", current_scan_time );

        psg_step ( );

        audio_last_value = audio_last_value + ( ( psg_audio_scan ( ) + ctc0_current_value ) - audio_last_value ) / 16;

        if ( ctc0_buf_pos < g_audio_ctc.count ) {
            if ( ctc0_current_state_width <= AUDIO_SCAN_PERIOD ) {

                unsigned tst1 = current_scan_time - start_scan_time;
                unsigned tst2;

                do {
                    tst2 = g_audio_ctc.samples[++ctc0_buf_pos].timestamp - start_scan_time;
                } while ( ( ctc0_buf_pos < g_audio_ctc.count ) && ( tst1 > tst2 ) );

                ctc0_current_value = g_audio_ctc.samples[ctc0_buf_pos].state;
                ctc0_current_state_width = g_audio_ctc.samples[ctc0_buf_pos + 1].timestamp - current_scan_time;
            };
        };

        already_scaned += AUDIO_SCAN_PERIOD;

        if ( already_scaned >= resample_width ) {
            //          printf ( "res: %d = %d\n", dst_sample_pos, audio_last_value );
            g_audio.buffer [ dst_sample_pos ] = audio_last_value;
            if ( dst_sample_pos < IFACE_AUDIO_20MS_SAMPLES ) dst_sample_pos++;
            already_scaned -= resample_width;
            last_scan_end = current_scan_time;
        };

        current_scan_time += AUDIO_SCAN_PERIOD;

        if ( samples_width < AUDIO_SCAN_PERIOD ) break;

        samples_width -= AUDIO_SCAN_PERIOD;
        ctc0_current_state_width -= AUDIO_SCAN_PERIOD;
    };

    int tst1 = current_scan_time - g_audio_ctc.samples[0].timestamp;
    int tst2;

    if ( ctc0_buf_pos < g_audio_ctc.count ) {
        do {
            tst2 = g_audio_ctc.samples[++ctc0_buf_pos].timestamp - start_scan_time;
        } while ( ( ctc0_buf_pos < g_audio_ctc.count ) && ( tst1 > tst2 ) );
    };

    g_audio_ctc.samples[ctc0_buf_pos].timestamp = current_scan_time;

    if ( g_audio_ctc.count != 0 ) {

        unsigned i;
        for ( i = 0; ctc0_buf_pos <= g_audio_ctc.count; i++ ) {
            g_audio_ctc.samples[i].timestamp = g_audio_ctc.samples[ctc0_buf_pos].timestamp;
            g_audio_ctc.samples[i].state = g_audio_ctc.samples[ctc0_buf_pos].state;
            ctc0_buf_pos++;
        };
        g_audio_ctc.count = i - 1;
    };

    if ( ( psg_buf_pos != -1 ) && ( g_psg_audio.count - psg_buf_pos ) != -1 ) {
        while ( ( psg_buf_pos <= g_psg_audio.count ) ) {
            //printf ( "REAL WRITE: %d: 0x%02x\n", g_psg_audio.samples[psg_buf_pos].timestamp, g_psg_audio.samples[psg_buf_pos].value );
            psg_real_write_byte ( g_psg_audio.samples[psg_buf_pos++].value );
        };
    };
    g_psg_audio.samples[0].value = -1;
    g_psg_audio.count = 0;

}
#endif
