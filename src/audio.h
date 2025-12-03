/* 
 * File:   audio.h
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 28. července 2015, 13:39
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

#ifndef AUDIO_H
#define AUDIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mz800emu_cfg.h"
#include "gdg/video.h"
#include "gdg/gdg.h"
#include "psg/psg.h"


#include "iface_sdl/iface_sdl_audio.h"
#include <stdint.h>

#define AUDIO_RESAMPLE_PERIOD             ( (unsigned) (GDGCLK_BASE / 50 ) / IFACE_AUDIO_20MS_SAMPLES )

#define AUDIO_BUF_MAX_VALUE             0xffff
#define AUDIO_SRC_CANNELS_COUNT         ( 4 + 1 )   /* Celkovy pocet audio kanalu: 4 (psg) + 1 (ctc0) */
#define AUDIO_MAXVAL_PER_CHANNEL        ( AUDIO_BUF_MAX_VALUE / AUDIO_SRC_CANNELS_COUNT )


    typedef uint16_t AUDIO_BUF_t;

    //extern AUDIO_BUF_t g_attenuator_volume_value [ PSG_OUT_OFF + 1 ];
    extern AUDIO_BUF_t g_attenuator_volume_value [4] [ 16 ];


#ifdef AUDIO_FILLBUFF_v2


    typedef struct st_AUDIO_SAMPLE {
        unsigned timestamp;
        AUDIO_BUF_t state;
    } st_AUDIO_SAMPLE;

#if ( GDGCLK_1M1_DIVIDER == 16 ) && ( MZ800EMU_CFG_MAX_SYNC_SPEED == 1000 )
#define AUDIO_MIN_CTC0_EVENT_WIDTH  ( GDGCLK_1M1_DIVIDER * 1.5 )  /* ( 24 = 16 * 1.5 ) */
#define AUDIO_MAX_SCAN_WIDTH  ( MZ800EMU_CFG_MAX_SYNC_SPEED * 0.01 * VIDEO_SCREEN_TICKS )  /* ( 3544320 = 1000 * 0.01 * 354432 ) */
#define AUDIO_MAX_CTC_SAMPLES ( 3544320 / 24 )
#else
#if ( GDGCLK_1M1_DIVIDER == 16 ) && ( MZ800EMU_CFG_MAX_SYNC_SPEED == 100 )
#define AUDIO_MIN_CTC0_EVENT_WIDTH  ( GDGCLK_1M1_DIVIDER * 1.5 )  /* ( 24 = 16 * 1.5 ) */
#define AUDIO_MAX_SCAN_WIDTH  VIDEO_SCREEN_TICKS
#define AUDIO_MAX_CTC_SAMPLES ( AUDIO_MAX_SCAN_WIDTH / 24 )
#else
#error Plese define AUDIO_MAX_CTC_SAMPLES
#endif
#endif


    typedef struct st_AUDIO_CTC {
        st_AUDIO_SAMPLE samples [ AUDIO_MAX_CTC_SAMPLES ];
        unsigned count;
    } st_AUDIO_CTC;

    extern st_AUDIO_CTC g_audio_ctc;
#endif


    typedef enum en_AUDIO_VOLUME_SETUP {
        AUDIO_VOLUME_SETUP_ALL = 0,
        AUDIO_VOLUME_SETUP_PER_CHANNEL,
    } en_AUDIO_VOLUME_SETUP;


    typedef struct st_AUDIO {
        AUDIO_BUF_t buffer[IFACE_AUDIO_20MS_SAMPLES];
        unsigned buffer_position;
        unsigned last_update;
        unsigned resample_timer;
        unsigned ctc0_output;
        AUDIO_BUF_t last_value;

        en_AUDIO_VOLUME_SETUP volume_setup;
        int volume_master;
        int volume_8253;
        int volume_psg0;
        int volume_psg1;
        int volume_psg2;
        int volume_psg3;
    } st_AUDIO;

    extern st_AUDIO g_audio;

    extern void audio_init ( void );
    extern void audio_ctc0_changed ( unsigned value, unsigned event_ticks );

    extern void audio_setup_type ( en_AUDIO_VOLUME_SETUP type );
    extern void audio_set_volume_master ( int volume );
    extern void audio_set_volume_8253 ( int volume );
    extern void audio_set_volume_psg0 ( int volume );
    extern void audio_set_volume_psg1 ( int volume );
    extern void audio_set_volume_psg2 ( int volume );
    extern void audio_set_volume_psg3 ( int volume );

#ifdef AUDIO_FILLBUFF_v1
    extern void audio_fill_buffer_v1 ( unsigned event_ticks );
#endif

#ifdef AUDIO_FILLBUFF_v2
    extern void audio_fill_buffer_v2 ( unsigned now_total_ticks );
#endif

#ifdef __cplusplus
}
#endif

#endif /* AUDIO_H */

