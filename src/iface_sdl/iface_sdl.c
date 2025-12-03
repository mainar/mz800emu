/* 
 * File:   iface_sdl.c
 * Author: chaky
 *
 * Created on 10. června 2015, 23:28
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

#include "mz800emu_cfg.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL_version.h>

//#include <SDL_ttf.h>

#include "iface_sdl.h"
#include "iface_sdl_audio.h"
#include "iface_sdl_keyboard.h"
#include "iface_sdl_joy.h"

#include "main.h"
#include "display.h"

#include "gdg/video.h"
#include "gdg/gdg.h"

#include "ui/ui_main.h"


/* Zakladni rozmery okna */
#if 0
#define IFACE_STATUS_LINE_HEIGHT                  30
#else
#define IFACE_STATUS_LINE_HEIGHT                  0
#endif

#define IFACE_WINDOW_WIDTH                    VIDEO_DISPLAY_WIDTH
#define IFACE_WINDOW_HEIGHT                   ( ( VIDEO_DISPLAY_HEIGHT * 2 ) + IFACE_STATUS_LINE_HEIGHT )
#define IFACE_WINDOW_ASPECT_RATIO             ( (float) IFACE_WINDOW_WIDTH / IFACE_WINDOW_HEIGHT )

SDL_Rect mzdisplay_rect = { 0, 0, VIDEO_DISPLAY_WIDTH, VIDEO_DISPLAY_HEIGHT };
#if 0
SDL_Rect statusline_rect = { 0, 0 + VIDEO_DISPLAY_HEIGHT, VIDEO_DISPLAY_WIDTH, IFACE_STATUS_LINE_HEIGHT };
#endif

struct st_iface_sdl g_iface_sdl;

#define DEBUG_EVENTS 0

#if DEBUG_EVENTS
// <editor-fold defaultstate="collapsed" desc="PrintEvent function">


void PrintEvent ( SDL_Event *event ) {
    printf ( " event: " );
    switch ( event->type ) {
        case SDL_QUIT:
            printf ( "SDL_QUIT: %0.03f s\n", (float) event->quit.timestamp / 1000 );
            break;
        case SDL_KEYDOWN:
            printf ( "SDL_KEYDOWN" );
            goto label_keyup;
        case SDL_KEYUP:
            printf ( "SDL_KEYUP" );
label_keyup:
            printf ( " - state: %d, repeat: %d, ", event->key.state, event->key.repeat );
            if ( event->key.keysym.sym ) {
                printf ( "sym: %c\n", event->key.keysym.sym );
            } else {
                printf ( "scode: 0x%02x\n", event->key.keysym.scancode );
            };
            break;
        case SDL_WINDOWEVENT:
            printf ( "SDL_WINDOWEVENT - " );

            switch ( event->window.event ) {
                case SDL_WINDOWEVENT_SHOWN:
                    printf ( "SDL_WINDOWEVENT_SHOWN\n" );
                    break;
                case SDL_WINDOWEVENT_HIDDEN:
                    printf ( "SDL_WINDOWEVENT_HIDDEN\n" );
                    break;
                case SDL_WINDOWEVENT_EXPOSED:
                    printf ( "SDL_WINDOWEVENT_EXPOSED\n" );
                    break;
                case SDL_WINDOWEVENT_MOVED:
                    printf ( "SDL_WINDOWEVENT_MOVED: %d, %d\n", event->window.data1, event->window.data2 );
                    break;
                case SDL_WINDOWEVENT_RESIZED:
                    printf ( "SDL_WINDOWEVENT_RESIZED: %d x %d \n", event->window.data1, event->window.data2 );
                    break;
                case SDL_WINDOWEVENT_SIZE_CHANGED:
                    printf ( "SDL_WINDOWEVENT_SIZE_CHANGED %d, %d\n", event->window.data1, event->window.data2 );
                    break;
                case SDL_WINDOWEVENT_MINIMIZED:
                    printf ( "SDL_WINDOWEVENT_MINIMIZED\n" );
                    break;
                case SDL_WINDOWEVENT_MAXIMIZED:
                    printf ( "SDL_WINDOWEVENT_MAXIMIZED\n" );
                    break;
                case SDL_WINDOWEVENT_RESTORED:
                    printf ( "SDL_WINDOWEVENT_RESTORED\n" );
                    break;
                case SDL_WINDOWEVENT_ENTER:
                    printf ( "SDL_WINDOWEVENT_ENTER\n" );
                    break;
                case SDL_WINDOWEVENT_LEAVE:
                    printf ( "SDL_WINDOWEVENT_LEAVE\n" );
                    break;
                case SDL_WINDOWEVENT_FOCUS_GAINED:
                    printf ( "SDL_WINDOWEVENT_FOCUS_GAINED\n" );
                    break;
                case SDL_WINDOWEVENT_FOCUS_LOST:
                    printf ( "SDL_WINDOWEVENT_FOCUS_LOST\n" );
                    break;
                case SDL_WINDOWEVENT_CLOSE:
                    printf ( "SDL_WINDOWEVENT_CLOSE\n" );
                    break;
            };
            break;
        case SDL_MOUSEMOTION:
            printf ( "SDL_MOUSEMOTION - state: %d, x,y: %d, %d, rel x,y: %d, %d\n", event->motion.state, event->motion.x, event->motion.y, event->motion.xrel, event->motion.yrel );
            break;
        case SDL_MOUSEBUTTONDOWN:
            printf ( "SDL_MOUSEBUTTONDOWN" );
            goto mouse_button;
        case SDL_MOUSEBUTTONUP:
            printf ( "SDL_MOUSEBUTTONUP" );
mouse_button:
            printf ( " - state: %d, x,y: %d, %d, button: %d, clicks: %d\n", event->button.state, event->button.x, event->button.y, event->button.button, event->button.clicks );
            break;
        case SDL_MOUSEWHEEL:
            printf ( "SDL_MOUSEWHEEL - x,y: %d, %d\n", event->wheel.x, event->wheel.y ); /* direction je az ve verzi 2.0.4 */
            break;
        default:
            printf ( "!!! UNKNOWN EVENT TYPE - %d\n", event->type );
    };
}
// </editor-fold>
#endif


#define EVENT_FILTER_ACCEPT   1   /* event bude vlozen do fronty */
#define EVENT_FILTER_DROP     0   /* event bude zahozen */

#if 0


/* funkce volana pred vlozenim eventu do fronty */
int iface_sdl_event_filter ( void* userdata, SDL_Event* event ) {

    /* Budeme se zajimat jen o nasledujici window eventy */
    if ( event->type == SDL_WINDOWEVENT ) {
        if ( ( event->window.event == SDL_WINDOWEVENT_CLOSE ) || ( event->window.event == SDL_WINDOWEVENT_SIZE_CHANGED ) || ( event->window.event == SDL_WINDOWEVENT_RESIZED ) || ( event->window.event == SDL_WINDOWEVENT_EXPOSED ) ) {
            return EVENT_FILTER_ACCEPT;
        } else {
            return EVENT_FILTER_DROP;
        };
    };

    return EVENT_FILTER_ACCEPT;
}
#endif

#if 0


/* funkce volana po pridani eventu do fronty - retval je ignorovana */
int iface_sdl_event_watch ( void* userdata, SDL_Event* event ) {
    printf ( "Watch" );
    PrintEvent ( event );
    return 0;
}
#endif

#if 0
// <editor-fold defaultstate="collapsed" desc="iface_sdl_init_events - funkce pro inicializaci SDL eventu">


void iface_sdl_init_events ( void ) {
    /* Application events */
    SDL_EventState ( SDL_QUIT, SDL_ENABLE ); /* user-requested quit */

    /* Android and iOS events */
    SDL_EventState ( SDL_APP_TERMINATING, SDL_IGNORE ); /* OS is terminating the application */
    SDL_EventState ( SDL_APP_LOWMEMORY, SDL_IGNORE ); /* OS is low on memory; free some */
    SDL_EventState ( SDL_APP_WILLENTERBACKGROUND, SDL_IGNORE ); /* application is entering background */
    SDL_EventState ( SDL_APP_DIDENTERBACKGROUND, SDL_IGNORE ); /* application entered background */
    SDL_EventState ( SDL_APP_WILLENTERFOREGROUND, SDL_IGNORE ); /* application is entering foreground */
    SDL_EventState ( SDL_APP_DIDENTERFOREGROUND, SDL_IGNORE ); /* application entered foreground */


    /* Window events */
    SDL_EventState ( SDL_WINDOWEVENT, SDL_ENABLE ); /* window state change */
    SDL_EventState ( SDL_SYSWMEVENT, SDL_IGNORE ); /* system specific event */


    /* Keyboard events */
    SDL_EventState ( SDL_KEYDOWN, SDL_ENABLE ); /* key pressed */
    SDL_EventState ( SDL_KEYUP, SDL_ENABLE ); /* key released */
    SDL_EventState ( SDL_TEXTEDITING, SDL_IGNORE ); /* keyboard text editing (composition) */
    SDL_EventState ( SDL_TEXTINPUT, SDL_IGNORE ); /* keyboard text input */


    /* Mouse events */
    SDL_EventState ( SDL_MOUSEMOTION, SDL_IGNORE ); /* mouse moved */
    SDL_EventState ( SDL_MOUSEBUTTONDOWN, SDL_ENABLE ); /* mouse button pressed */
    SDL_EventState ( SDL_MOUSEBUTTONUP, SDL_IGNORE ); /* mouse button released */
    SDL_EventState ( SDL_MOUSEWHEEL, SDL_IGNORE ); /* mouse wheel motion */


    /* Joystick events */
    SDL_EventState ( SDL_JOYAXISMOTION, SDL_IGNORE ); /* joystick axis motion */
    SDL_EventState ( SDL_JOYBALLMOTION, SDL_IGNORE ); /* joystick trackball motion */
    SDL_EventState ( SDL_JOYHATMOTION, SDL_IGNORE ); /* joystick hat position change */
    SDL_EventState ( SDL_JOYBUTTONDOWN, SDL_IGNORE ); /* joystick button pressed */
    SDL_EventState ( SDL_JOYBUTTONUP, SDL_IGNORE ); /* joystick button released */
    SDL_EventState ( SDL_JOYDEVICEADDED, SDL_IGNORE ); /* joystick connected */
    SDL_EventState ( SDL_JOYDEVICEREMOVED, SDL_IGNORE ); /* joystick disconnected */


    /* Controller events */
    SDL_EventState ( SDL_CONTROLLERAXISMOTION, SDL_IGNORE ); /* controller axis motion */
    SDL_EventState ( SDL_CONTROLLERBUTTONDOWN, SDL_IGNORE ); /* controller button pressed */
    SDL_EventState ( SDL_CONTROLLERBUTTONUP, SDL_IGNORE ); /* controller button released */
    SDL_EventState ( SDL_CONTROLLERDEVICEADDED, SDL_IGNORE ); /* controller connected */
    SDL_EventState ( SDL_CONTROLLERDEVICEREMOVED, SDL_IGNORE ); /* controller disconnected */
    SDL_EventState ( SDL_CONTROLLERDEVICEREMAPPED, SDL_IGNORE ); /* controller mapping updated */


    /* Touch events */
    SDL_EventState ( SDL_FINGERDOWN, SDL_IGNORE ); /* user has touched input device */
    SDL_EventState ( SDL_FINGERUP, SDL_IGNORE ); /* user stopped touching input device */
    SDL_EventState ( SDL_FINGERMOTION, SDL_IGNORE ); /* user is dragging finger on input device */


    /* Gesture events */
    SDL_EventState ( SDL_DOLLARGESTURE, SDL_IGNORE ); /*  */
    SDL_EventState ( SDL_DOLLARRECORD, SDL_IGNORE ); /*  */
    SDL_EventState ( SDL_MULTIGESTURE, SDL_IGNORE ); /*  */

    /* Clipboard events */
    SDL_EventState ( SDL_CLIPBOARDUPDATE, SDL_IGNORE ); /* the clipboard changed */


    /* Drag and drop events */
    SDL_EventState ( SDL_DROPFILE, SDL_IGNORE ); /* the system requests a file open */


    /* Audio hotplug events */
    //    SDL_EventState ( SDL_AUDIODEVICEADDED, SDL_IGNORE ); /* a new audio device is available (>= SDL 2.0.4) */
    //    SDL_EventState ( SDL_AUDIODEVICEREMOVED, SDL_IGNORE ); /* an audio device has been removed (>= SDL 2.0.4) */


    /* Render events */
    SDL_EventState ( SDL_RENDER_TARGETS_RESET, SDL_IGNORE ); /* the render targets have been reset and their contents need to be updated (>= SDL 2.0.2) */
    //    SDL_EventState ( SDL_RENDER_DEVICE_RESET, SDL_IGNORE ); /* the device has been reset and all textures need to be recreated (>= SDL 2.0.4) */


    SDL_EventState ( SDL_USEREVENT, SDL_IGNORE ); /* a user-specified event */

    /* Vsechny prichozi eventy muzeme pred ulozenim do fronty protahnout uzivatelskym filtrem */
    //SDL_SetEventFilter ( iface_sdl_event_filter, NULL );

#if 0
    /* Vsechny prichozi eventy muzeme po ulozeni do fronty protahnout watchem */
    SDL_AddEventWatch ( iface_sdl_event_watch, NULL );
#endif

}
// </editor-fold>
#endif


void iface_sdl_set_surface_colors ( SDL_Surface *surface, uint32_t *colormap ) {

    SDL_Color colors [ DISPLAY_MZCOLORS ];

    int i;
    for ( i = 0; i < DISPLAY_MZCOLORS; i++ ) {
        colors [ i ] . r = colormap [ i ] >> 16;
        colors [ i ] . g = ( colormap [ i ] >> 8 ) & 0xff;
        colors [ i ] . b = colormap [ i ] & 0xff;
        colors [ i ] . a = 0;
    };

    SDL_SetPaletteColors ( surface->format->palette, colors, 0, DISPLAY_MZCOLORS );
}


void iface_sdl_set_colors ( uint32_t *colormap ) {
    iface_sdl_set_surface_colors ( g_iface_sdl.active_surface, colormap );
    g_iface_sdl.redraw_full_screen_request = 1;
}


void iface_sdl_fix_window_aspect_ratio ( char correction_by ) {

    Sint32 wsizeX, wsizeY;
    SDL_GetWindowSize ( g_iface_sdl.window, &wsizeX, &wsizeY );

    if ( correction_by == 'W' ) {
        wsizeY = ( 1.f / IFACE_WINDOW_ASPECT_RATIO ) * wsizeX;
    } else {
        wsizeX = IFACE_WINDOW_ASPECT_RATIO * wsizeY;
    };

    g_iface_sdl.last_wsizeX = wsizeX;
    g_iface_sdl.last_wsizeY = wsizeY;

    SDL_SetWindowSize ( g_iface_sdl.window, wsizeX, wsizeY );

    if ( SDL_RenderSetScale ( g_iface_sdl.renderer, (float) wsizeX / VIDEO_DISPLAY_WIDTH, (float) ( wsizeY - IFACE_STATUS_LINE_HEIGHT ) / VIDEO_DISPLAY_HEIGHT ) ) {
        fprintf ( stderr, "Could not set render scale: %s\n", SDL_GetError ( ) );
        main_app_quit ( EXIT_FAILURE );
    };
    g_iface_sdl.redraw_full_screen_request = 1;
}


void iface_sdl_set_window_size_by_scale ( float scale ) {

    unsigned w = IFACE_WINDOW_WIDTH * scale;
    unsigned h = IFACE_WINDOW_HEIGHT * scale;

    g_iface_sdl.last_wsizeX = w;
    g_iface_sdl.last_wsizeY = h;
    SDL_SetWindowSize ( g_iface_sdl.window, w, h );

    if ( SDL_RenderSetScale ( g_iface_sdl.renderer, (float) w / VIDEO_DISPLAY_WIDTH, (float) ( h - IFACE_STATUS_LINE_HEIGHT ) / VIDEO_DISPLAY_HEIGHT ) ) {
        fprintf ( stderr, "Could not set render scale: %s\n", SDL_GetError ( ) );
        main_app_quit ( EXIT_FAILURE );
    };
    g_iface_sdl.redraw_full_screen_request = 1;
}


void iface_sdl_init ( void ) {

    /* Kontrola verze SDL */
    if ( !SDL_VERSION_ATLEAST ( 2, 0, 2 ) ) {
        SDL_Log ( "SDL_VERSION %i.%i.%i is less than 2.0.2", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL );
        main_app_quit ( EXIT_FAILURE );
    };

    SDL_version compiled;
    SDL_version linked;

    SDL_VERSION ( &compiled );
    SDL_GetVersion ( &linked );

    if ( ( compiled.major != linked.major ) || ( compiled.minor != linked.minor ) || ( compiled.patch != linked.patch ) ) {
        SDL_Log ( "Compiled SDL ver.: %d.%d.%d", compiled.major, compiled.minor, compiled.patch );
    };
    SDL_Log ( "Linked SDL ver.: %d.%d.%d", linked.major, linked.minor, linked.patch );

    /* Inicializace video interface */
    if ( SDL_Init ( SDL_INIT_VIDEO ) ) {
        fprintf ( stderr, "Unable to initialize SDL VIDEO: %s\n", SDL_GetError ( ) );
        main_app_quit ( EXIT_FAILURE );
    };

    /* Inicializace eventu */
    //iface_sdl_init_events ( );


    /* Inicializace okna */
    g_iface_sdl.last_wsizeX = IFACE_WINDOW_WIDTH;
    g_iface_sdl.last_wsizeY = IFACE_WINDOW_HEIGHT;
    g_iface_sdl.window = SDL_CreateWindow ( "MZ-800", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, g_iface_sdl.last_wsizeX, g_iface_sdl.last_wsizeY, SDL_WINDOW_RESIZABLE );

    if ( NULL == g_iface_sdl.window ) {
        fprintf ( stderr, "Could not create window: %s\n", SDL_GetError ( ) );
        main_app_quit ( EXIT_FAILURE );
    };


    /* Inicializace rendereru */
    g_iface_sdl.renderer = SDL_CreateRenderer ( g_iface_sdl.window, -1, ( SDL_RENDERER_ACCELERATED /* | SDL_RENDERER_PRESENTVSYNC */ ) );

    if ( NULL == g_iface_sdl.renderer ) {
        fprintf ( stderr, "Could not create render: %s\n", SDL_GetError ( ) );
        main_app_quit ( EXIT_FAILURE );
    };
    if ( SDL_RenderSetScale ( g_iface_sdl.renderer, (float) IFACE_WINDOW_WIDTH / VIDEO_DISPLAY_WIDTH, (float) ( IFACE_WINDOW_HEIGHT - IFACE_STATUS_LINE_HEIGHT ) / VIDEO_DISPLAY_HEIGHT ) ) {
        fprintf ( stderr, "Could not set render scale: %s\n", SDL_GetError ( ) );
        main_app_quit ( EXIT_FAILURE );
    };

    /* Inicializace surface s 8 bitovou barevnou hloubkou (4 bitova hloubka neni podporovana) */
    g_iface_sdl.active_surface = SDL_CreateRGBSurface ( 0, VIDEO_DISPLAY_WIDTH, VIDEO_DISPLAY_HEIGHT, 8, 0, 0, 0, 0 );

    if ( NULL == g_iface_sdl.active_surface ) {
        fprintf ( stderr, "Could not create active surface: %s\n", SDL_GetError ( ) );
        main_app_quit ( EXIT_FAILURE );
    };

#if 0
    /* Inicializace zalozniho surface s 8 bitovou barevnou hloubkou (4 bitova hloubka neni podporovana) */
    g_iface_sdl.old_surface = SDL_CreateRGBSurface ( 0, VIDEO_DISPLAY_WIDTH, VIDEO_DISPLAY_HEIGHT, 8, 0, 0, 0, 0 );

    if ( NULL == g_iface_sdl.old_surface ) {
        fprintf ( stderr, "Could not create backup surface: %s\n", SDL_GetError ( ) );
        main_app_quit ( EXIT_FAILURE );
    };
#endif

    iface_sdl_set_window_size_by_scale ( display_get_window_startup_scale ( ) );

    /* Nastavit preddefinovane barvy */
    iface_sdl_set_colors ( display_get_default_color_schema ( ) );

    /* Clear screen */
    if ( 0 != SDL_FillRect ( g_iface_sdl.active_surface, NULL, 0 ) ) {
        fprintf ( stderr, "Could write into surface: %s\n", SDL_GetError ( ) );
        main_app_quit ( EXIT_FAILURE );
    };

#if 0
    /* Zkopirujeme active surface do old */
    if ( SDL_BlitSurface ( g_iface_sdl.active_surface, NULL, g_iface_sdl.old_surface, NULL ) ) {
        fprintf ( stderr, "Error SDL_BlitSurface(): %s\n", SDL_GetError ( ) );
        main_app_quit ( EXIT_FAILURE );
    };
#endif

    g_iface_sdl.redraw_full_screen_request = 1;

#if 0
    if ( TTF_Init ( ) ) {
        fprintf ( stderr, "TTF_Init(): %s\n", TTF_GetError ( ) );
        main_app_quit ( EXIT_FAILURE );
    };
#endif

    if ( EXIT_FAILURE == iface_sdl_audio_init ( NULL, -1 ) ) {
        main_app_quit ( EXIT_FAILURE );
    }

    iface_sdl_joy_init ( );
}


void iface_sdl_quit ( void ) {

    iface_sdl_audio_quit ( );
    iface_sdl_joy_quit ( );

    //    SDL_DestroyRenderer ( g_iface_sdl.renderer );
    //    SDL_DestroyWindow ( g_iface_sdl.window );
#if 0
    TTF_Quit ( );
#endif
    SDL_Quit ( );
}


void iface_sdl_pool_all_events ( void ) {

    if ( SDL_QuitRequested ( ) ) {
        printf ( "SDL_QuitRequested\n" );
        main_app_quit ( EXIT_SUCCESS );
    };

    SDL_Event event;

    int keyboard_events = 0;

    while ( SDL_PollEvent ( &event ) ) {

        switch ( event.type ) {

            case SDL_WINDOWEVENT:

                if ( event.window.event == SDL_WINDOWEVENT_CLOSE ) {
                    main_app_quit ( EXIT_SUCCESS );


                    //} else if ( event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED ) {
                    //printf ( "SIZE CHANGED - X = %d, Y = %d\n", event.window.data1, event.window.data2 );
                } else if ( event.window.event == SDL_WINDOWEVENT_RESIZED ) {

                    Sint32 w, h;
                    SDL_GetWindowSize ( g_iface_sdl.window, &w, &h );

                    //printf ( "RESIZED - X = %d (%d), Y = %d (%d)\n", event.window.data1, w, event.window.data2, h );
                    //printf ( "RESIZED - New scale: %f x %f\n", (float) event.window.data1 / DISPLAY_VISIBLE_WIDTH, (float) event.window.data2 / DISPLAY_VISIBLE_HEIGHT );

                    Sint32 wsizeX = event.window.data1;
                    Sint32 wsizeY = event.window.data2;

#ifdef WINDOWS
                    // Tohle proste funguje jen ve windows :(
                    if ( g_display.locked_window_aspect_ratio ) {
                        float aspect_ratio = (float) wsizeX / (float) wsizeY;

                        if ( aspect_ratio != IFACE_WINDOW_ASPECT_RATIO ) {
                            if ( ( wsizeX < g_iface_sdl.last_wsizeX ) && ( wsizeY == g_iface_sdl.last_wsizeY ) ) {
                                wsizeY = ( 1.f / IFACE_WINDOW_ASPECT_RATIO ) * wsizeX;
                            } else if ( ( wsizeY < g_iface_sdl.last_wsizeY ) && ( wsizeX == g_iface_sdl.last_wsizeX ) ) {
                                wsizeX = IFACE_WINDOW_ASPECT_RATIO * wsizeY;
                            } else {
                                if ( aspect_ratio > IFACE_WINDOW_ASPECT_RATIO ) {
                                    wsizeY = ( 1.f / IFACE_WINDOW_ASPECT_RATIO ) * wsizeX;
                                } else {
                                    wsizeX = IFACE_WINDOW_ASPECT_RATIO * wsizeY;
                                };
                            };
                            g_iface_sdl.last_wsizeX = wsizeX;
                            g_iface_sdl.last_wsizeY = wsizeY;
                            //printf ( "\tNEW RESIZE: %d, %d\n", wsizeX, wsizeY );
                            SDL_SetWindowSize ( g_iface_sdl.window, wsizeX, wsizeY );
                        };
                    };
#endif
                    g_iface_sdl.last_wsizeX = wsizeX;
                    g_iface_sdl.last_wsizeY = wsizeY;

                    float width, height;
                    width = (float) wsizeX / VIDEO_DISPLAY_WIDTH;
                    height = (float) wsizeY / VIDEO_DISPLAY_HEIGHT;

                    //printf ( "\tNEW SIZE: %d, %d\n", wsizeX, wsizeY );

                    if ( SDL_RenderSetScale ( g_iface_sdl.renderer, width, height ) ) {
                        fprintf ( stderr, "Could not set render scale: %s\n", SDL_GetError ( ) );
                        main_app_quit ( EXIT_FAILURE );
                    };
                    g_iface_sdl.redraw_full_screen_request = 1;

                } else if ( event.window.event == SDL_WINDOWEVENT_EXPOSED ) {
                    //iface_sdl_redraw_screen ( );
                    g_iface_sdl.redraw_full_screen_request = 1;
                };
                break;

            case SDL_KEYDOWN:
                keyboard_events++;
                iface_sdl_keydown_event ( &event );
                break;

            case SDL_KEYUP:
                keyboard_events++;
                iface_sdl_keyup_event ( &event );
                break;


            case SDL_MOUSEBUTTONDOWN:

                if ( event.button.button == SDL_BUTTON_RIGHT ) {
#ifdef UI_TOPMENU_IS_WINDOW
                    ui_show_hide_main_menu_window ( );
                } else {
                    ui_hide_main_menu_window ( );
#else
                    ui_popup_main_menu ( );
#endif
                };
                break;
        };
    };

    if ( keyboard_events != 0 ) {
        iface_sdl_full_keyboard_scan ( );
    }
}

#if 0


void iface_sdl_pool_window_events ( void ) {

    SDL_PumpEvents ( );

    if ( SDL_QuitRequested ( ) ) {
        printf ( "SDL_QuitRequested\n" );
        main_app_quit ( EXIT_SUCCESS );
    };

    SDL_Event events [ 20 ];

    /* Eventy oken */
    int num_events = SDL_PeepEvents ( events, 20, SDL_GETEVENT, SDL_WINDOWEVENT, SDL_WINDOWEVENT );

    int resize_request = -1;

    SDL_Event event;

    for ( int i = 0; i < num_events; i++ ) {
        SDL_Event event = events [ i ];

#if DEBUG_EVENTS
        PrintEvent ( &event );
#endif
        if ( event.window.event == SDL_WINDOWEVENT_CLOSE ) {
            main_app_quit ( EXIT_SUCCESS );

        } else if ( event.window.event == SDL_WINDOWEVENT_RESIZED ) {
            resize_request = i;

        } else if ( event.window.event == SDL_WINDOWEVENT_EXPOSED ) {
            //iface_sdl_redraw_screen ( );
            g_iface_sdl.redraw_full_screen_request = 1;
        };
    };

    if ( resize_request != -1 ) {
        SDL_Event event = events [ resize_request ];
        //printf ( "Resize accepted - New scale: %f x %f\n", (float) event.window.data1 / DISPLAY_COLS, (float) event.window.data2 / DISPLAY_ROWS );
        if ( SDL_RenderSetScale ( g_iface_sdl.renderer, (float) event.window.data1 / VIDEO_DISPLAY_WIDTH, (float) ( event.window.data2 - IFACE_STATUS_LINE_HEIGHT ) / VIDEO_DISPLAY_HEIGHT ) ) {
            fprintf ( stderr, "Could not set render scale: %s\n", SDL_GetError ( ) );
            main_app_quit ( EXIT_FAILURE );
        };
        g_iface_sdl.redraw_full_screen_request = 1;
    };

    /* Eventy klavesnice */
    //num_events = SDL_PeepEvents ( events, 20, SDL_GETEVENT, SDL_KEYDOWN, SDL_KEYUP );

#if 0
    num_events = SDL_PeepEvents ( events, 20, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT );

    for ( int i = 0; i < num_events; i++ ) {
        SDL_Event event = events [ i ];
#else
    while ( SDL_PollEvent ( &event ) ) {
#endif

#if DEBUG_EVENTS
        PrintEvent ( &event );
#endif

        static unsigned keyboard_alt = 0;

        if ( event.type == SDL_KEYDOWN ) {

            if ( event.key.keysym.scancode == SDL_SCANCODE_F12 ) {
                mz800_reset ( );
#if 0
            } else if ( event.key.keysym.scancode == SDL_SCANCODE_F11 ) {
                if ( g_mz800.debug_pc == 0 ) {
                    printf ( "Turn ON debug PC\n" );
                } else {
                    printf ( "Turn OFF debug PC\n" );
                };
                g_mz800.debug_pc = ~g_mz800.debug_pc & 1;
#endif
            } else if ( event.key.keysym.scancode == SDL_SCANCODE_F10 ) {
                printf ( "F10 - INTERRUPT\n" );
                unsigned interrupt_ticks = z80ex_int ( g_mz800.cpu );
                if ( interrupt_ticks ) {
                    /* interrupt byl prijat */
                    printf ( "Interrupt received!\n" );
                } else {
                    printf ( "Interrupt NOT received!\n" );
                }

            } else if ( event.key.keysym.scancode == SDL_SCANCODE_LALT ) {
                keyboard_alt = 1;

                /*
                 * 
                 *  Obsluha klavesovych zkratek ALT+xx
                 * 
                 */
            } else if ( keyboard_alt ) {

                if ( event.key.keysym.scancode == SDL_SCANCODE_C ) {
                    /*
                     * Virtual CMT: Alt + C
                     */
                    ui_cmt_window_show_hide ( );

                } else if ( event.key.keysym.scancode == SDL_SCANCODE_M ) {
                    /*
                     * Max speed: Alt + M
                     */
                    mz800_switch_emulation_speed ( ( ~g_mz800.use_max_emulation_speed ) & 0x01 );
                } else if ( event.key.keysym.scancode == SDL_SCANCODE_P ) {
                    /*
                     * Pause emulation: Alt + P
                     */
                    mz800_pause_emulation ( ( ~g_mz800.emulation_paused ) & 0x01 );
#ifdef MZ800EMU_CFG_DEBUGGER_ENABLED                    
                } else if ( event.key.keysym.scancode == SDL_SCANCODE_D ) {
                    /*
                     * Debugger window: Alt + D
                     */
                    debugger_show_hide_main_window ( );
#endif
                };

            };

        } else if ( event.type == SDL_KEYUP ) {

            if ( event.key.keysym.scancode == SDL_SCANCODE_LALT ) {
                keyboard_alt = 0;
            };

        } else if ( event.type == SDL_MOUSEBUTTONDOWN ) {
            if ( event.button.button == SDL_BUTTON_RIGHT ) {
                ui_popup_main_menu ( );
#ifdef UI_TOPMENU_IS_WINDOW 
            } else {
                ui_hide_main_menu_window ( );
#endif
            };
        };
    };

}
#endif


int lcmp_fb_row ( uint8_t *f0, uint8_t *f1, int start_x ) {

    int x;

    //    for ( x = start_x; x < DISPLAY_VISIBLE_WIDTH; x++ ) {
    for ( x = start_x; x < VIDEO_BORDER_LEFT_WIDTH + VIDEO_CANVAS_WIDTH; x++ ) {
        if ( f0 [ x ] != f1 [ x ] ) {
            return x;
        };
    };
    return -1;
}


unsigned fb_search_block_end ( uint8_t *f0, uint8_t *f1, int start_x ) {

    int x;
    int count_eq = 0;

    //    for ( x = start_x; x < DISPLAY_VISIBLE_WIDTH; x++ ) {
    for ( x = start_x; x < VIDEO_BORDER_LEFT_WIDTH + VIDEO_CANVAS_WIDTH; x++ ) {
        if ( f0 [ x ] != f1 [ x ] ) {
            count_eq = 0;
        } else {
            /* pocet bajtu, ktere musi byt za sebou stejne, aby jsme rekli, z je konec bloku */
            if ( count_eq == 5 ) {
                return x - count_eq;
            };
            count_eq++;
        };
    };
    return x - count_eq;
}


// <editor-fold defaultstate="collapsed" desc="iface_sdl_init_events - funkce pro vlastni statusline v SDL rendereru">
/* Tato status line neni uplne nejlepsi */
#if 0


void iface_sdl_create_status_line ( void ) {
    SDL_SetRenderDrawColor ( g_iface_sdl.renderer, 240, 240, 240, 0 );
    SDL_RenderFillRect ( g_iface_sdl.renderer, &statusline_rect );
    SDL_SetRenderDrawColor ( g_iface_sdl.renderer, 160, 160, 160, 0 );
    SDL_RenderDrawLine ( g_iface_sdl.renderer, statusline_rect.x, statusline_rect.y + 2, statusline_rect.x + statusline_rect.w - 2, statusline_rect.y + 2 );
    SDL_RenderDrawLine ( g_iface_sdl.renderer, statusline_rect.x, statusline_rect.y + 2, statusline_rect.x, statusline_rect.y + statusline_rect.h - 1 );
}


void iface_sdl_update_status_line ( void ) {

    //const char fontpath[] = "/usr/share/fonts/gnu-free/FreeSans.ttf";
    //const char fontpath[] = "/usr/share/fonts/open-sans/OpenSans-Bold.ttf";
    const char fontpath[] = "/usr/local/Trolltech/QtEmbedded-4.6.3-arm/lib/fonts/DejaVuSans.ttf";

    TTF_Font *font = TTF_OpenFont ( fontpath, 10 );
    if ( font == NULL ) {
        fprintf ( stderr, "%s():%d - TTF_OpenFont(): %s\n", __func__, __LINE__, TTF_GetError ( ) );
        main_app_quit ( EXIT_FAILURE );
    };

    SDL_Color fg_col = { 0, 0, 0, 0 };

    char statusline_txt [30];
    sprintf ( statusline_txt, "%d %%", g_mz800.status_emulation_speed );
    g_mz800.status_changed = 0;

    SDL_Surface *surface = TTF_RenderText_Solid ( font, statusline_txt, fg_col );
    if ( surface == NULL ) {
        fprintf ( stderr, "%s():%d - TTF_RenderText_Solid(): %s\n", __func__, __LINE__, TTF_GetError ( ) );
        main_app_quit ( EXIT_FAILURE );
    };

    TTF_CloseFont ( font );

    SDL_Texture *texture = SDL_CreateTextureFromSurface ( g_iface_sdl.renderer, surface );
    if ( NULL == texture ) {
        fprintf ( stderr, "%s():%d - SDL_CreateTextureFromSurface(): %s\n", __func__, __LINE__, SDL_GetError ( ) );
        main_app_quit ( EXIT_FAILURE );
    };


    static SDL_Rect last_text_rect = { 0, 0, 0, 0 };

    if ( last_text_rect.w != 0 ) {
        SDL_SetRenderDrawColor ( g_iface_sdl.renderer, 240, 240, 240, 0 );
        SDL_RenderFillRect ( g_iface_sdl.renderer, &last_text_rect );
    };

    SDL_Rect src_text_rect;
    src_text_rect.x = 0;
    src_text_rect.y = 0;
    src_text_rect.w = surface->w;
    src_text_rect.h = surface->h;

    last_text_rect.x = statusline_rect.x + 10;
    last_text_rect.y = statusline_rect.y + 3;
    last_text_rect.w = surface->w;
    last_text_rect.h = surface->h;


    printf ( "w, h: %d, %d\n", surface->w, surface->h );

    SDL_FreeSurface ( surface );

    if ( SDL_RenderCopy ( g_iface_sdl.renderer, texture, &src_text_rect, &last_text_rect ) ) {
        fprintf ( stderr, "%s():%d - SDL_RenderCopy(): %s\n", __func__, __LINE__, TTF_GetError ( ) );
        main_app_quit ( EXIT_FAILURE );
    };
    SDL_DestroyTexture ( texture );

    SDL_RenderPresent ( g_iface_sdl.renderer );
}
#endif
// </editor-fold>


void iface_sdl_render_status_line ( void ) {

#if 0
    iface_sdl_update_status_line ( );
    SDL_RenderPresent ( g_iface_sdl.renderer );
#endif
    char txt [100];
    strcpy ( txt, "MZ-800 - " );
    if ( TEST_EMULATION_PAUSED ) {
        strcat ( txt, "PAUSED" );
    } else {
        switch ( g_mz800.use_max_emulation_speed ) {
            case 0:
                strcat ( txt, "SYNC" );
                break;

            case 1:
                strcat ( txt, "MAX" );
                break;
        };
        char speed[100];
        sprintf ( speed, ": %d %%", g_mz800.status_emulation_speed );
        //g_mz800.status_changed = 0;

        strcat ( txt, speed );
    }

    //printf ( "TITLE: %s\n", txt );
    SDL_SetWindowTitle ( g_iface_sdl.window, txt );
}


void iface_sdl_update_window ( void ) {

#if 0
    SDL_Surface *txt_surface = iface_sdl_statusline_surface ( );
    /* bud to zapiseme do surface */
    SDL_Rect txt_rect_sur = { 100, 100, 0, 0 };
    if ( SDL_BlitSurface ( txt_surface, NULL, g_iface_sdl.active_surface, &txt_rect_sur ) ) {
        fprintf ( stderr, "%s():%d - SDL_CreateTextureFromSurface(): %s\n", __func__, __LINE__, TTF_GetError ( ) );
        main_app_quit ( EXIT_FAILURE );
    };
#endif


    /* TODO: surface by se mel zamykat, ale takhle mi to akosi nechodi! */
    //    if ( SDL_LockSurface ( g_iface_sdl.active_surface ) ) {
    //        fprintf ( stderr, "%s():%d - Could not lock surface: %s\n", __func__, __LINE__, SDL_GetError ( ) );
    //        main_app_quit ( EXIT_FAILURE );
    //    };

    /* Zobrazit obsah surface */
    SDL_Texture *texture = SDL_CreateTextureFromSurface ( g_iface_sdl.renderer, g_iface_sdl.active_surface );

    if ( NULL == texture ) {
        fprintf ( stderr, "%s():%d - SDL_CreateTextureFromSurface(): %s\n", __func__, __LINE__, SDL_GetError ( ) );
        main_app_quit ( EXIT_FAILURE );
    };

    SDL_RenderPresent ( g_iface_sdl.renderer );


    if ( ( g_display.forced_full_screen_redrawing ) || ( g_iface_sdl.redraw_full_screen_request ) || ( g_gdg.framebuffer_state == ( FB_STATE_BORDER_CHANGED | FB_STATE_SCREEN_CHANGED ) ) ) {

        /* Rendrujeme cely framebuffer */
        if ( SDL_RenderCopy ( g_iface_sdl.renderer, texture, &mzdisplay_rect, &mzdisplay_rect ) ) {
            fprintf ( stderr, "%s():%d - SDL_RenderCopy(): %s\n", __func__, __LINE__, SDL_GetError ( ) );
            main_app_quit ( EXIT_FAILURE );
        };
#if 0
        iface_sdl_create_status_line ( );
        iface_sdl_update_status_line ( );
#endif

        /* Protoze mame vypnuto hledani rozdilu mezi dvema surface, tak neni potreba delat kopii */
#if 0
        /* Zkopirujeme active surface do old */
        if ( SDL_BlitSurface ( g_iface_sdl.active_surface, NULL, g_iface_sdl.old_surface, NULL ) ) {
            fprintf ( stderr, "%s():%d - SDL_BlitSurface: %s\n", __func__, __LINE__, SDL_GetError ( ) );
            main_app_quit ( EXIT_FAILURE );
        };
#endif
        g_iface_sdl.redraw_full_screen_request = 0;

    } else {

        SDL_Rect update_box;

        if ( g_gdg.framebuffer_state == FB_STATE_BORDER_CHANGED ) {

            update_box.x = VIDEO_BEAM_BORDER_TOP_FIRST_COLUMN;
            update_box.y = VIDEO_BEAM_BORDER_TOP_FIRST_ROW;
            update_box.w = VIDEO_BORDER_TOP_WIDTH;
            update_box.h = VIDEO_BORDER_TOP_HEIGHT;
            if ( SDL_RenderCopyEx ( g_iface_sdl.renderer, texture, &update_box, &update_box, 0, NULL, SDL_FLIP_NONE ) ) {
                fprintf ( stderr, "%s():%d - SDL_RenderCopyEx(): %s\n", __func__, __LINE__, SDL_GetError ( ) );
                main_app_quit ( EXIT_FAILURE );
            };

            update_box.x = VIDEO_BEAM_BORDER_LEFT_FIRST_COLUMN;
            update_box.y = VIDEO_BEAM_BORDER_LEFT_FIRST_ROW;
            update_box.w = VIDEO_BORDER_LEFT_WIDTH;
            update_box.h = VIDEO_BORDER_LEFT_HEIGHT;
            if ( SDL_RenderCopyEx ( g_iface_sdl.renderer, texture, &update_box, &update_box, 0, NULL, SDL_FLIP_NONE ) ) {
                fprintf ( stderr, "%s():%d - SDL_RenderCopyEx(): %s\n", __func__, __LINE__, SDL_GetError ( ) );
                main_app_quit ( EXIT_FAILURE );
            };

            update_box.x = VIDEO_BEAM_BORDER_RIGHT_FIRST_COLUMN;
            update_box.y = VIDEO_BEAM_BORDER_RIGHT_FIRST_ROW;
            update_box.w = VIDEO_BORDER_RIGHT_WIDTH;
            update_box.h = VIDEO_BORDER_RIGHT_HEIGHT;
            if ( SDL_RenderCopyEx ( g_iface_sdl.renderer, texture, &update_box, &update_box, 0, NULL, SDL_FLIP_NONE ) ) {
                fprintf ( stderr, "%s():%d - SDL_RenderCopyEx(): %s\n", __func__, __LINE__, SDL_GetError ( ) );
                main_app_quit ( EXIT_FAILURE );
            };

            update_box.x = VIDEO_BEAM_BORDER_BOTOM_FIRST_COLUMN;
            update_box.y = VIDEO_BEAM_BORDER_BOTOM_FIRST_ROW;
            update_box.w = VIDEO_BORDER_BOTOM_WIDTH;
            update_box.h = VIDEO_BORDER_BOTOM_HEIGHT;
            if ( SDL_RenderCopyEx ( g_iface_sdl.renderer, texture, &update_box, &update_box, 0, NULL, SDL_FLIP_NONE ) ) {
                fprintf ( stderr, "%s():%d - SDL_RenderCopyEx(): %s\n", __func__, __LINE__, SDL_GetError ( ) );
                main_app_quit ( EXIT_FAILURE );
            };

        } else if ( g_gdg.framebuffer_state == FB_STATE_SCREEN_CHANGED ) {

#if 1
            update_box.x = VIDEO_BEAM_CANVAS_FIRST_COLUMN;
            update_box.y = VIDEO_BEAM_CANVAS_FIRST_ROW;
            update_box.w = VIDEO_CANVAS_WIDTH;
            update_box.h = VIDEO_CANVAS_HEIGHT;
            if ( SDL_RenderCopyEx ( g_iface_sdl.renderer, texture, &update_box, &update_box, 0, NULL, SDL_FLIP_NONE ) ) {
                fprintf ( stderr, "%s():%d - Could not copy from texture: %s\n", __func__, __LINE__, SDL_GetError ( ) );
                main_app_quit ( EXIT_FAILURE );
            };
            SDL_RenderPresent ( g_iface_sdl.renderer );
#else
            /* Update jen zmenenych bloku se obcas spatne vykresli (Yoshin cp/m obcas zapomene na radku kus kurzoru) - divne ... */

            update_box.h = 1;

            unsigned row;

            for ( row = VIDEO_BEAM_CANVAS_FIRST_ROW; row <= VIDEO_BEAM_CANVAS_LAST_ROW; row++ ) {
                uint8_t *active = (uint8_t*) g_iface_sdl.active_surface->pixels + ( row * VIDEO_DISPLAY_WIDTH );
                uint8_t *old = (uint8_t*) g_iface_sdl.old_surface->pixels + ( row * VIDEO_DISPLAY_WIDTH );

                int px_start = lcmp_fb_row ( active, old, VIDEO_BORDER_LEFT_WIDTH );


                if ( -1 != px_start ) {

                    update_box.y = row;

                    do {
                        unsigned px_end = fb_search_block_end ( active, old, px_start );

                        update_box.x = px_start;
                        update_box.w = px_end - px_start;
                        if ( SDL_RenderCopyEx ( g_iface_sdl.renderer, texture, &update_box, &update_box, 0, NULL, SDL_FLIP_NONE ) ) {
                            fprintf ( stderr, "%s():%d - Could not copy from texture: %s\n", __func__, __LINE__, SDL_GetError ( ) );
                            main_app_quit ( EXIT_FAILURE );
                        };
#if 1
                        memcpy ( &old [ px_start ], &active [ px_start ], update_box.w );
#elif 0
                        /* Zkopirujeme active surface do old */
                        if ( SDL_BlitSurface ( g_iface_sdl.active_surface, &update_box, g_iface_sdl.old_surface, &update_box ) ) {
                            fprintf ( stderr, "%s():%d - SDL_BlitSurface: %s\n", __func__, __LINE__, SDL_GetError ( ) );
                            main_app_quit ( EXIT_FAILURE );
                        };
#endif

                        px_start = lcmp_fb_row ( active, old, px_end + 1 );

                    } while ( -1 != px_start );
                };

            };
#if 0
            /* Zkopirujeme active surface do old */
            if ( SDL_BlitSurface ( g_iface_sdl.active_surface, NULL, g_iface_sdl.old_surface, NULL ) ) {
                fprintf ( stderr, "%s():%d - SDL_BlitSurface: %s\n", __func__, __LINE__, SDL_GetError ( ) );
                main_app_quit ( EXIT_FAILURE );
            };
#endif

#endif


        };

    }

    SDL_DestroyTexture ( texture );

#if 0
    if ( g_mz800.status_changed != 0 ) {
        iface_sdl_update_status_line ( );
    };
#endif
    SDL_RenderPresent ( g_iface_sdl.renderer );

}


/*
 * 
 * Provedeme aktualizaci obrazu jen v intervalu < beam_start, beam_end >
 * 
 */
void iface_sdl_update_window_in_beam_interval ( unsigned beam_start, unsigned beam_end ) {


    if ( beam_start >= VIDEO_GET_DISPLAY_ADDR ( VIDEO_BEAM_DISPLAY_LAST_ROW, VIDEO_BEAM_DISPLAY_LAST_COLUMN ) ) return;

    if ( beam_end > VIDEO_GET_DISPLAY_ADDR ( VIDEO_BEAM_DISPLAY_LAST_ROW, VIDEO_BEAM_DISPLAY_LAST_COLUMN ) ) {
        beam_end = VIDEO_GET_DISPLAY_ADDR ( VIDEO_BEAM_DISPLAY_LAST_ROW, VIDEO_BEAM_DISPLAY_LAST_COLUMN );
    };


    SDL_Texture *texture = SDL_CreateTextureFromSurface ( g_iface_sdl.renderer, g_iface_sdl.active_surface );

    if ( NULL == texture ) {
        fprintf ( stderr, "%s():%d - SDL_CreateTextureFromSurface(): %s\n", __func__, __LINE__, SDL_GetError ( ) );
        main_app_quit ( EXIT_FAILURE );
    };

    SDL_RenderPresent ( g_iface_sdl.renderer );

    SDL_Rect update_box;

    unsigned row1 = VIDEO_GET_SCREEN_ROW ( beam_start );
    unsigned col1 = VIDEO_GET_SCREEN_COL ( beam_start );
    if ( col1 >= VIDEO_BEAM_DISPLAY_LAST_COLUMN ) {
        row1++;
        col1 = 0;
    };

    unsigned row2 = VIDEO_GET_SCREEN_ROW ( beam_end );
    unsigned col2 = VIDEO_GET_SCREEN_COL ( beam_end );


    if ( col2 > VIDEO_BEAM_DISPLAY_LAST_COLUMN ) {
        col2 = VIDEO_BEAM_DISPLAY_LAST_COLUMN;
    };

    if ( VIDEO_GET_DISPLAY_ADDR ( row1, col1 ) >= VIDEO_GET_DISPLAY_ADDR ( row2, col2 ) ) return;

    //printf ( "Update interval: %d, %d - %d, %d\n", row1, col1, row2, col2 );

    update_box.x = col1;
    update_box.y = row1;

    if ( row1 == row2 ) {
        update_box.w = col2 - col1;
    } else {
        update_box.w = VIDEO_BEAM_DISPLAY_LAST_COLUMN - col1;
    };
    update_box.h = 1;

    if ( SDL_RenderCopyEx ( g_iface_sdl.renderer, texture, &update_box, &update_box, 0, NULL, SDL_FLIP_NONE ) ) {
        fprintf ( stderr, "%s():%d - SDL_RenderCopyEx(): %s\n", __func__, __LINE__, SDL_GetError ( ) );
        main_app_quit ( EXIT_FAILURE );
    };

    col1 = 0;
    update_box.y++;

    if ( VIDEO_GET_DISPLAY_ADDR ( update_box.y, col1 ) < VIDEO_GET_DISPLAY_ADDR ( row2, col2 ) ) {

        update_box.x = 0;

        if ( update_box.y != row2 ) {
            update_box.w = VIDEO_BEAM_DISPLAY_LAST_COLUMN;
            update_box.h = row2 - update_box.y;

            if ( SDL_RenderCopyEx ( g_iface_sdl.renderer, texture, &update_box, &update_box, 0, NULL, SDL_FLIP_NONE ) ) {
                fprintf ( stderr, "%s():%d - SDL_RenderCopyEx(): %s\n", __func__, __LINE__, SDL_GetError ( ) );
                main_app_quit ( EXIT_FAILURE );
            };
        };

        if ( col2 != 0 ) {
            update_box.y = row2;
            update_box.w = col2;
            update_box.h = 1;

            if ( SDL_RenderCopyEx ( g_iface_sdl.renderer, texture, &update_box, &update_box, 0, NULL, SDL_FLIP_NONE ) ) {
                fprintf ( stderr, "%s():%d - SDL_RenderCopyEx(): %s\n", __func__, __LINE__, SDL_GetError ( ) );
                main_app_quit ( EXIT_FAILURE );
            };
        };
    };

    SDL_DestroyTexture ( texture );

    SDL_RenderPresent ( g_iface_sdl.renderer );
}


void iface_sdl_set_main_window_focus ( void ) {
    SDL_RaiseWindow ( g_iface_sdl.window );
}
