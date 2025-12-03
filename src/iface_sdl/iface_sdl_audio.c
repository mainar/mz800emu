/* 
 * File:   iface_sdl_audio.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 18. července 2015, 18:29
 */

#include "mz800emu_cfg.h"

#include <stdio.h>
#include <stdint.h>

#include <string.h>
#include <SDL.h>

#include "iface_sdl_audio.h"
#include "mz800.h"
#include "audio.h"

#ifdef MZ800EMU_CFG_DEBUGGER_ENABLED
#include "debugger/debugger.h"
#endif


typedef enum en_IFACE_AUDIO_BUFFER_STATE {
    IFACE_AUDIO_BUFFER_STATE_NORMAL = 0,
    IFACE_AUDIO_BUFFER_STATE_UNSYNC,
    IFACE_AUDIO_BUFFER_STATE_EXITING,
} en_IFACE_AUDIO_BUFFER_STATE;


typedef struct st_IFACE_AUDIO {
    SDL_AudioDeviceID dev;
    SDL_AudioSpec spec_want;
    SDL_AudioSpec spec_have;

    uint16_t buffer[IFACE_AUDIO_20MS_SAMPLES];
    en_IFACE_AUDIO_BUFFER_STATE state;

    SDL_mutex *write_lock;
    SDL_cond *write_cond;

    SDL_mutex *play_lock;
    SDL_cond *play_cond;

    uint32_t last_played_sample;
    uint32_t last_writed_sample;

#if LINUX
    uint32_t last_20ms_sync;
#endif
} st_IFACE_AUDIO;

st_IFACE_AUDIO g_iface_audio;


void iface_sdl_audio_callback ( void *userdata, Uint8 *stream, int len ) {

    SDL_LockMutex ( g_iface_audio.write_lock );

    while ( ( 0 == ( g_iface_audio.last_writed_sample - g_iface_audio.last_played_sample ) ) && ( g_iface_audio.state == IFACE_AUDIO_BUFFER_STATE_NORMAL ) ) {
        SDL_CondWait ( g_iface_audio.write_cond, g_iface_audio.write_lock );
    };

    if ( g_iface_audio.state == IFACE_AUDIO_BUFFER_STATE_EXITING ) {
        memset ( stream, 0x00, len );
        printf ( "SDL audio player exiting\r\n" );
        SDL_UnlockMutex ( g_iface_audio.write_lock );
        return;
    } else {

        uint32_t writed_sample = g_iface_audio.last_writed_sample;

        SDL_LockMutex ( g_iface_audio.play_lock );
        g_iface_audio.last_played_sample = writed_sample;
        SDL_CondSignal ( g_iface_audio.play_cond );
        SDL_UnlockMutex ( g_iface_audio.play_lock );
        SDL_UnlockMutex ( g_iface_audio.write_lock );

#ifdef MZ800EMU_CFG_DEBUGGER_ENABLED
        if ( ( g_debugger.animated_updates == DEBUGGER_ANIMATED_UPDATES_ENABLED_WTHOUT_AUDIO ) && ( TEST_DEBUGGER_ACTIVE ) ) {
            memset ( stream, 0x00, len );
        } else {
#endif        
            if ( SDL_AUDIO_ISSIGNED ( g_iface_audio.spec_have.format ) ) {
                /* bugfix: SDL-2.0.7 v linuxu vynucuje signed format */
                uint16_t *value = g_iface_audio.buffer;
                int reallen = len / sizeof (uint16_t );
                int i;
                uint16_t *dst = (uint16_t *) stream;
                for ( i = 0; i < reallen; i++ ) {
                    *dst++ = ( *value++ ) - 0x8000;
                };
            } else {
                memcpy ( stream, g_iface_audio.buffer, len );
            };
#ifdef MZ800EMU_CFG_DEBUGGER_ENABLED
        };
#endif
    };
}


/**
 * Vratime ID hledaneho audio driveru, nebo -1, pokud jsme jej nenasli.
 * 
 * @param driver_name
 * @return 
 */
int iface_sdl_audio_getDriverIdByName ( const char *driver_name ) {
    if ( driver_name != NULL ) {
        int i;
        for ( i = 0; i < SDL_GetNumAudioDrivers ( ); ++i ) {
            if ( 0 == strcmp ( SDL_GetAudioDriver ( i ), driver_name ) ) {
                return i;
            };
        };
    };
    return -1;
}


void iface_audio_buffer_init ( void ) {
    g_iface_audio.last_played_sample = 0;
    g_iface_audio.last_writed_sample = 0;
    g_iface_audio.write_lock = SDL_CreateMutex ( );
    g_iface_audio.write_cond = SDL_CreateCond ( );
    g_iface_audio.play_lock = SDL_CreateMutex ( );
    g_iface_audio.play_cond = SDL_CreateCond ( );
    g_iface_audio.state = IFACE_AUDIO_BUFFER_STATE_NORMAL;
}


int iface_sdl_audio_init ( const char *preferedAudioDriverName, int preferedAudioDeviceId ) {

#ifndef MZ800EMU_CFG_AUDIO_DISABLED

    if ( SDL_InitSubSystem ( SDL_INIT_AUDIO ) != 0 ) {
        SDL_Log ( "Unable to initialize SDL: %s", SDL_GetError ( ) );
        return EXIT_FAILURE;
    }

    /* Search and init audio driver */

    int driverId = iface_sdl_audio_getDriverIdByName ( preferedAudioDriverName );

    if ( driverId != -1 ) {
        SDL_Log ( "Initializing the prefferred audio driver '%s'.", preferedAudioDriverName );
    } else {

        if ( preferedAudioDriverName != NULL ) {
            SDL_Log ( "Can't found the prefferred audio driver '%s'.", preferedAudioDriverName );
        };

        preferedAudioDeviceId = -1;

        int count_drivers = SDL_GetNumAudioDrivers ( );
        //SDL_Log ( "Found %d available audio driver(s).", count_drivers );

        int i;
        for ( i = 0; i < count_drivers; ++i ) {
            if ( SDL_AudioInit ( SDL_GetAudioDriver ( i ) ) ) {
                SDL_Log ( "Probing audio driver %s - FAILED", SDL_GetAudioDriver ( i ) );
            } else {
                if ( driverId == -1 ) {
                    driverId = i;
                };
                //SDL_Log ( "Probing audio driver %s - OK", SDL_GetAudioDriver ( i ) );
                SDL_AudioQuit ( );
            };
        };

        if ( driverId == -1 ) {
            SDL_Log ( "No audio driver found." );
            return EXIT_FAILURE;
        };

        SDL_Log ( "Initializing audio driver '%s'.", SDL_GetAudioDriver ( driverId ) );
    }

    if ( SDL_AudioInit ( SDL_GetAudioDriver ( driverId ) ) ) {
        SDL_Log ( "Can't initialize audio driver '%s': %s", SDL_GetAudioDriver ( driverId ), SDL_GetError ( ) );
        return EXIT_FAILURE;
    };


    /* Search audio device */

    int count_audio_devices = SDL_GetNumAudioDevices ( 0 );

    if ( count_audio_devices < 1 ) {
        SDL_Log ( "No audio device found." );
        return EXIT_FAILURE;
    };

    int deviceId = -1;

    if ( ( preferedAudioDeviceId != -1 ) && ( preferedAudioDeviceId >= count_audio_devices ) ) {
        SDL_Log ( "Prefferred audio device '%d' not found.", preferedAudioDeviceId );
    } else if ( ( preferedAudioDeviceId != -1 ) && ( preferedAudioDeviceId < count_audio_devices ) ) {
        deviceId = preferedAudioDeviceId;
        SDL_Log ( "Using prefferred audio device '%d' - %s", deviceId, SDL_GetAudioDeviceName ( deviceId, 0 ) );
    };

    if ( deviceId == -1 ) {
        SDL_Log ( "Found %d audio device(s).", count_audio_devices );
        deviceId = 0;
        //int i;
        //for ( i = 0; i < count_audio_devices; i++ ) {
        //    SDL_Log ( "DEV(%d) - %s", i, SDL_GetAudioDeviceName ( i, 0 ) );
        //};
        //SDL_Log ( "Using audio device DEV(0)." );
    };

    /* Open audio device */

    SDL_AudioSpec *spec_want = &g_iface_audio.spec_want;
    SDL_AudioSpec *spec_have = &g_iface_audio.spec_have;

    SDL_memset ( &g_iface_audio.spec_want, 0, sizeof ( g_iface_audio.spec_want ) );

    spec_want->freq = IFACE_AUDIO_SAMPLE_RATE;
    spec_want->format = AUDIO_U16SYS;
    spec_want->channels = IFACE_AUDIO_CHANNELS;
    spec_want->samples = IFACE_AUDIO_20MS_SAMPLES;
    spec_want->callback = iface_sdl_audio_callback;

    int attempts = 0;

    while ( 1 ) {
        g_iface_audio.dev = SDL_OpenAudioDevice ( NULL, 0, spec_want, spec_have, SDL_AUDIO_ALLOW_FORMAT_CHANGE | SDL_AUDIO_ALLOW_CHANNELS_CHANGE );

        if ( g_iface_audio.dev == 0 ) {
            SDL_Log ( "Failed to open audio: %s", SDL_GetError ( ) );
            return EXIT_FAILURE;
        };


        if ( spec_want->freq != spec_have->freq ) {
            SDL_Log ( "OpenAudioDevice(%i) - changed freq: want %i, have %i.", attempts, spec_want->freq, spec_have->freq );
            SDL_Log ( "OpenAudioDevice() - can't configure ouptut stream." );
            return EXIT_FAILURE;
#if 0
            /* tohle se zatim neni potreba */
            if ( attempts++ == 0 ) {
                spec_want->freq = spec_have->freq;
                spec_want->samples = spec_have->freq / 50;
                continue;
            } else {
                SDL_Log ( "OpenAudioDevice() - can't configure ouptut stream." );
                return EXIT_FAILURE;
            };
#endif
        };


        if ( spec_want->samples != spec_have->samples ) {
            SDL_Log ( "OpenAudioDevice(%i) - changed samples: want %i, have %i.", attempts, spec_want->samples, spec_have->samples );
#ifdef LINUX
            /* bugfix: Linux SDL do verze 2.0.7 vytvari 1/2 velikost pozadovaneho bufferu */
            if ( spec_have->samples == ( spec_want->samples / 2 ) ) {
                if ( spec_have->samples == ( spec_want->freq / 50 ) ) {
                    SDL_Log ( "OpenAudioDevice() - samples count is now fixed" );
                } else {
                    if ( attempts++ == 0 ) {
                        spec_want->samples = ( spec_want->freq / 50 ) * 2;
                        continue;
                    } else {
                        SDL_Log ( "OpenAudioDevice() - can't configure ouptut stream." );
                        return EXIT_FAILURE;
                    }
                };
            };
#else
            SDL_Log ( "OpenAudioDevice() - can't configure ouptut stream." );
            return EXIT_FAILURE;
#endif
        };

        if ( spec_want->format != spec_have->format ) {
            int format_error = 0;
            if ( SDL_AUDIO_BITSIZE ( spec_want->format ) != SDL_AUDIO_BITSIZE ( spec_have->format ) ) {
                format_error++;
                SDL_Log ( "OpenAudioDevice() - format bitsize changed." );
            };
            if ( SDL_AUDIO_ISFLOAT ( spec_want->format ) != SDL_AUDIO_ISFLOAT ( spec_have->format ) ) {
                format_error++;
                SDL_Log ( "OpenAudioDevice() - format datatype changed." );
            };
#if 0
            /* SDL-2.0.7 v linuxu nam vnucuje signed format - fixnuto v callbacku */
            if ( SDL_AUDIO_ISSIGNED ( spec_want->format ) != SDL_AUDIO_ISSIGNED ( spec_have->format ) ) {
                SDL_Log ( "OpenAudioDevice() - audio format changed: is signed - fixed" );
            };
#endif
            if ( format_error ) {
                SDL_Log ( "OpenAudioDevice() - can't configure ouptut stream." );
                return EXIT_FAILURE;
            };
        };

        if ( spec_want->channels != spec_have->channels ) {
            SDL_Log ( "OpenAudioDevice() - changed channels: want %i, have %i.", spec_want->channels, spec_have->channels );
            SDL_Log ( "OpenAudioDevice() - can't configure ouptut stream." );
            return EXIT_FAILURE;
        };

        break;
    };

    SDL_Log ( "SDL Audio interface is now succesfully initialized. Sample rate %d Hz, format: %d bit%s, buff_size %d.", spec_have->freq, SDL_AUDIO_BITSIZE ( spec_have->format ), SDL_AUDIO_ISSIGNED ( spec_have->format ) ? ", signed" : "", spec_have->size );

    iface_audio_buffer_init ( );

#if LINUX
    g_iface_audio.last_20ms_sync = SDL_GetTicks ( );
#endif

    SDL_PauseAudioDevice ( g_iface_audio.dev, 0 );
    SDL_Delay ( 20 );

#else // !MZ800EMU_CFG_AUDIO_DISABLED
    iface_audio_buffer_init ( );
#endif

    return EXIT_SUCCESS;
}


void iface_sdl_audio_quit ( void ) {

#ifndef MZ800EMU_CFG_AUDIO_DISABLED
    if ( g_iface_audio.write_lock ) {
        SDL_LockMutex ( g_iface_audio.write_lock );
        g_iface_audio.state = IFACE_AUDIO_BUFFER_STATE_EXITING;
        SDL_CondSignal ( g_iface_audio.write_cond );
        SDL_UnlockMutex ( g_iface_audio.write_lock );
    };

    if ( g_iface_audio.dev != 0 ) {

        if ( SDL_AUDIO_STOPPED != SDL_GetAudioDeviceStatus ( g_iface_audio.dev ) ) {
            SDL_PauseAudioDevice ( g_iface_audio.dev, 1 );
        };

        SDL_CloseAudioDevice ( g_iface_audio.dev );
    };

    if ( g_iface_audio.write_lock ) {
        SDL_DestroyMutex ( g_iface_audio.write_lock );
        SDL_DestroyCond ( g_iface_audio.write_cond );
        SDL_DestroyMutex ( g_iface_audio.play_lock );
        SDL_DestroyCond ( g_iface_audio.play_cond );
    };

    SDL_AudioQuit ( );
#endif // !MZ800EMU_CFG_AUDIO_DISABLED
}


void iface_sdl_audio_sync_20ms_cycle ( void ) {

    memcpy ( g_iface_audio.buffer, g_audio.buffer, sizeof (AUDIO_BUF_t ) * IFACE_AUDIO_20MS_SAMPLES );

    SDL_LockMutex ( g_iface_audio.write_lock );
    g_iface_audio.last_writed_sample++;
    SDL_CondSignal ( g_iface_audio.write_cond );
    SDL_UnlockMutex ( g_iface_audio.write_lock );

    if ( g_mz800.use_max_emulation_speed == 0 ) {
        /* pockame az si player prevezme posledni sample */
        SDL_LockMutex ( g_iface_audio.play_lock );
        while ( g_iface_audio.last_writed_sample != g_iface_audio.last_played_sample ) {
            SDL_CondWait ( g_iface_audio.play_cond, g_iface_audio.play_lock );
        };
        SDL_UnlockMutex ( g_iface_audio.play_lock );

#if LINUX
        while ( ( SDL_GetTicks ( ) - g_iface_audio.last_20ms_sync ) < 19 ) {
        };
        g_iface_audio.last_20ms_sync = SDL_GetTicks ( );
#endif
    };

}


void iface_sdl_audio_update_buffer_state ( void ) {
    SDL_LockMutex ( g_iface_audio.write_lock );
    if ( g_mz800.use_max_emulation_speed == 0 ) {
        g_iface_audio.state = IFACE_AUDIO_BUFFER_STATE_NORMAL;
    } else {
        g_iface_audio.state = IFACE_AUDIO_BUFFER_STATE_UNSYNC;
    };
    SDL_CondSignal ( g_iface_audio.write_cond );
    SDL_UnlockMutex ( g_iface_audio.write_lock );
}


void iface_sdl_audio_pause_emulation ( unsigned value ) {
    SDL_LockMutex ( g_iface_audio.write_lock );
    if ( value ) {
        g_iface_audio.state = IFACE_AUDIO_BUFFER_STATE_UNSYNC;
        SDL_CondSignal ( g_iface_audio.write_cond );
        SDL_UnlockMutex ( g_iface_audio.write_lock );
        SDL_PauseAudioDevice ( g_iface_audio.dev, 1 );
        memset ( g_iface_audio.buffer, 0x00, sizeof ( g_iface_audio.buffer ) );
    } else {
        g_iface_audio.state = IFACE_AUDIO_BUFFER_STATE_NORMAL;
        SDL_CondSignal ( g_iface_audio.write_cond );
        SDL_UnlockMutex ( g_iface_audio.write_lock );
        SDL_PauseAudioDevice ( g_iface_audio.dev, 0 );
    };
}
