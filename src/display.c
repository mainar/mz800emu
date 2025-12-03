/* 
 * File:   display.c
 * Author: chaky
 *
 * Created on 14. června 2015, 9:41
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

#include "display.h"
#include "iface_sdl/iface_sdl.h"
#include "cfgmain.h"



uint32_t display_predef_colors [ DISPLAY_MZCOLORS ] = {
                                                       0x000000, 0x4040ac, 0xd03400, 0xb40c8c,
                                                       0x406c00, 0x24ccff, 0xe8d430, 0xd0d0d0,
                                                       0x848484, 0x008ce8, 0xff0000, 0xf054cc,
                                                       0x54ff54, 0x80ffff, 0xffff28, 0xffffff
};


uint32_t display_predef_grays [ DISPLAY_MZCOLORS ] = {
                                                      0x000000, 0x545454, 0x606060, 0x6c6c6c,
                                                      0x909090, 0x9c9c9c, 0xc0c0c0, 0xcccccc,
                                                      0x787878, 0x848484, 0xa8a8a8, 0xb4b4b4,
                                                      0xd8d8d8, 0xe4e4e4, 0xf0f0f0, 0xffffff
};


uint32_t display_predef_greens [ DISPLAY_MZCOLORS ] = {
                                                       0x000000, 0x005400, 0x006000, 0x006c00,
                                                       0x009000, 0x009c00, 0x00c000, 0x00cc00,
                                                       0x007800, 0x008400, 0x00a800, 0x00b400,
                                                       0x00d800, 0x00e400, 0x00f000, 0x00ff00
};

const char *display_color_schema_name [DISPLAY_COLORS_COUNT] = {
                                                                "Normal",
                                                                "Grayscale",
                                                                "Green"
};

const float display_predef_scale [DISPLAY_STARTUP_SIZES_COUNT] = { 1, 1.5, 3 };

const char *display_predef_scale_name [DISPLAY_STARTUP_SIZES_COUNT] = {
                                                                       "Normal",
                                                                       "Bigger",
                                                                       "Big"
};

st_DISPLAY g_display;


#define DEFAULT_COLOR_SCHEMA        DISPLAY_NORMAL
#define DEFAULT_STARTUP_SIZE        DISPLAY_STARTUP_SIZE_BIGGER


static void display_print_current_color_schema ( void ) {
    printf ( "Display: set color schema '%s'\n", display_color_schema_name[g_display.color_schema] );
}


static void display_print_current_window_startup_size ( void ) {
    printf ( "Display: set window size '%s'\n", display_predef_scale_name[g_display.startup_window_size] );
}


void display_init ( void ) {

    en_DISPLAY_COLOR_SCHEMA i;
    for ( i = 0; i < DISPLAY_COLORS_COUNT; i++ ) {
        switch ( i ) {
            case DISPLAY_NORMAL:
                g_display.color_predef[i] = display_predef_colors;
                break;
            case DISPLAY_GRAYSCALE:
                g_display.color_predef[i] = display_predef_grays;
                break;
            case DISPLAY_GREEN:
                g_display.color_predef[i] = display_predef_greens;
                break;
            case DISPLAY_COLORS_COUNT:
                break;
        };
    };


    CFGMOD *cmod = cfgroot_register_new_module ( g_cfgmain, "DISPLAY" );

    CFGELM *elm;
    elm = cfgmodule_register_new_element ( cmod, "color_schema", CFGENTYPE_KEYWORD, DEFAULT_COLOR_SCHEMA,
                                           DISPLAY_NORMAL, "NORMAL",
                                           DISPLAY_GRAYSCALE, "GRAYSCALE",
                                           DISPLAY_GREEN, "GREEN",
                                           -1 );
    cfgelement_set_handlers ( elm, (void*) &g_display.color_schema, (void*) &g_display.color_schema );

    elm = cfgmodule_register_new_element ( cmod, "forced_full_screen_redrawing", CFGENTYPE_BOOL, 0 );
    cfgelement_set_handlers ( elm, (void*) &g_display.forced_full_screen_redrawing, (void*) &g_display.forced_full_screen_redrawing );

    elm = cfgmodule_register_new_element ( cmod, "locked_window_aspect_ratio", CFGENTYPE_BOOL, 0 );
    cfgelement_set_handlers ( elm, (void*) &g_display.locked_window_aspect_ratio, (void*) &g_display.locked_window_aspect_ratio );

    elm = cfgmodule_register_new_element ( cmod, "startup_size", CFGENTYPE_KEYWORD, DEFAULT_STARTUP_SIZE,
                                           DISPLAY_STARTUP_SIZE_NORMAL, "NORMAL",
                                           DISPLAY_STARTUP_SIZE_BIGGER, "BIGGER",
                                           DISPLAY_STARTUP_SIZE_BIG, "BIG",
                                           -1 );
    cfgelement_set_handlers ( elm, (void*) &g_display.startup_window_size, (void*) &g_display.startup_window_size );

    cfgmodule_parse ( cmod );
    cfgmodule_propagate ( cmod );

    display_print_current_color_schema ( );
    display_print_current_window_startup_size ( );

    /* ui jeste neni inicializovan */
    //ui_display_update_menu ( );
}


uint32_t* display_get_default_color_schema ( void ) {
    return g_display.color_predef[g_display.color_schema];
}


void display_set_colors ( en_DISPLAY_COLOR_SCHEMA color_schema ) {
    if ( color_schema > ( DISPLAY_COLORS_COUNT - 1 ) ) {
        color_schema = DEFAULT_COLOR_SCHEMA;
    };
    g_display.color_schema = color_schema;
    display_print_current_color_schema ( );
    iface_sdl_set_colors ( g_display.color_predef[g_display.color_schema] );
}


unsigned display_get_window_color_schema ( void ) {
    return g_display.color_schema;
}


void display_set_window_startup_scale ( en_DISPLAY_STARTUP_SIZE startup_window_size ) {
    if ( startup_window_size > ( DISPLAY_STARTUP_SIZES_COUNT - 1 ) ) {
        startup_window_size = DEFAULT_STARTUP_SIZE;
    };
    g_display.startup_window_size = startup_window_size;
    display_print_current_window_startup_size ( );
    iface_sdl_set_window_size_by_scale ( display_predef_scale[startup_window_size] );
}


float display_get_window_startup_scale ( void ) {
    return display_predef_scale[g_display.startup_window_size];
}
