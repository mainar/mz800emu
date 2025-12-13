/* 
 * File:   main.c
 * Author: chaky
 *
 * Created on 10. června 2015, 22:18
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
#include <stdlib.h>
#include <locale.h>
#include <string.h>
#include <glib.h>

#include "main.h"

#include "cfgmain.h"
#include "build_time.h"
#include "display.h"
#include "iface_sdl/iface_sdl.h"
#include "mz800.h"
#include "ui/ui_main.h"
#include "version_check/version_check.h"

#if 0
#include <windows.h>
#endif

#define DBGLEVEL (DBGNON /* | DBGERR | DBGWAR | DBGINF*/)
//#define DBGLEVEL (DBGNON | DBGERR | DBGWAR | DBGINF )
#include "debug.h"


void print_usage ( const char *program_name ) {
    printf ( "Usage: %s [OPTIONS] [FILE]\n", program_name );
    printf ( "\n" );
    printf ( "MZ800 Emulator\n" );
    printf ( "\n" );
    printf ( "Options:\n" );
    printf ( "  -h, --help              Show this help message and exit\n" );
    printf ( "\n" );
    printf ( "Arguments:\n" );
    printf ( "  FILE                    Optional MZF cassette tape image file to load on startup\n" );
    printf ( "                          Supported extensions: .mzf, .m12\n" );
    printf ( "\n" );
}

void main_app_quit ( int exit_value ) {

    /* cfgmain musi byt prvni exit funkce !!! */
    if ( exit_value == 0 ) {
        fprintf ( stderr, "Main App is normaly exiting...\n" );
        cfgmain_exit ( );
        version_check_exit ( );
        mz800_exit ( );
    } else {
        fprintf ( stderr, "Oops ... Main App is abnormaly exiting...\n" );
    };

    ui_exit ( );
    iface_sdl_quit ( );

#if WINDOWS
    // Pokud doslo k chybe, tak nechame otevrene okno, aby uzivatel mohl videt, co se stalo
    if ( exit_value != 0 ) {
            while(1) {};
    };
#endif

    exit ( exit_value );
}

#if 0


void main_app_init ( void ) {
    iface_sdl_init ( );
    ui_main_init ( argc, argv );
    mz800_init ( );
}
#endif


/*
 *
 */
int main ( int argc, char** argv ) {

    char *mzf_file_to_load = NULL;

    // Parse command line arguments
    for ( int i = 1; i < argc; i++ ) {
        if ( strcmp ( argv[i], "-h" ) == 0 || strcmp ( argv[i], "--help" ) == 0 ) {
            print_usage ( argv[0] );
            return 0;
        } else if ( argv[i][0] != '-' ) {
            // Assume it's a file path
            if ( mzf_file_to_load == NULL ) {
                mzf_file_to_load = argv[i];
            } else {
                fprintf ( stderr, "Error: Multiple file arguments provided. Only one file can be loaded at a time.\n" );
                print_usage ( argv[0] );
                return 1;
            }
        } else {
            fprintf ( stderr, "Error: Unknown option '%s'\n", argv[i] );
            print_usage ( argv[0] );
            return 1;
        }
    }

    g_setenv ( "LC_ALL", "C", TRUE );
    setlocale ( LC_ALL, "" );

#if 0
    /*
     * nejak se mi s pomoci ddltool nepodarilo vyrobit a nalinkovat fungujici delaylib
     *
     */
    if ( SetDllDirectory ( "C:\\share\\mz800emu\\runtime\\sdl-2" ) ) {
        printf ( "1 OK\n" );
    } else {
        printf ( "1 FAIL\n" );
    };
    if ( LoadLibrary ( "C:\\share\\mz800emu\\runtime\\sdl-2\\SDL2.dll" ) ) {
        printf ( "2 OK\n" );
    } else {
        printf ( "2 FAIL\n" );
    };
    return 0;
#endif

    SDL_Log ( "Application start!" );
    SDL_Log ( "Version: %s (%s)", EMULATOR_VERSION, build_time_get_revision_txt ( ) );
    SDL_Log ( "Build time: %s", build_time_get ( ) );

    /* cfgmain musi byt prvni init funkce !!! */
    cfgmain_init ( );

    //main_app_init ( );
    display_init ( );
    
#if GTK_3_22_30
    iface_sdl_init ( );
    //ui_init ( argc, argv );
    ui_init ( );
#else
    // gtk_init() musi probehnout pred iface_sdl_init() !!!
    // v novejsi verzi gtk nam totiz gtk nejak ovlivnuje chovani SDL Window (Windows)

    //ui_init ( argc, argv );
    ui_init ( );
    iface_sdl_init ( );
#endif

    version_check_init ( );

    mz800_init ( );

    printf ( "\nTips:\n" );
    printf ( "   - use right-click mouse button on the emulator window to show the MAIN MENU.\n" );
    printf ( "   - edit the file ./ui_resources/mz800emu.css, if you need change UI menu fonts, colors, etc...\n" );
    printf ( "\nSome useful shortcut keys:\n" );
    printf ( "   Alt + C - virtual CMT\n" );
    printf ( "   Alt + K - virtual keyboard\n" );
    printf ( "   Alt + D - debugger\n" );
    printf ( "   Alt + B - breakpoints\n" );
    printf ( "   Alt + E - memory browser\n" );
    printf ( "   Alt + M - switch max / normal speed\n" );
    printf ( "   Alt + P - switch pause / play\n" );
    printf ( "   F12     - reset\n" );
    printf ( "\n" );

    mz800_main ( mzf_file_to_load );

    return 0;
}

