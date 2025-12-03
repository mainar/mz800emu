/* 
 * File:   iface_sdl_joy.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 1. února 2018, 17:16
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
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>


#include "iface_sdl.h"

#include "mz800.h"
#include "joy/joy.h"
#include "iface_sdl.h"
#include "iface_sdl_joy.h"

#include "cfgmain.h"
#include "ui/ui_utils.h"
#include "ui/ui_joy.h"


st_IFACE_JOY g_iface_joy;


int iface_sdl_joy_rescan_devices ( void ) {
    int count_devices = 0;
    if ( SDL_WasInit ( SDL_INIT_JOYSTICK ) ) {
        count_devices = iface_sdl_joy_available_realdev_count ( );
        SDL_Log ( "Found %d joystick device(s)", count_devices );
    } else {
        SDL_Log ( "SDL Joystick Subsystem was not initialized - can't rescan devices!" );
    };
    return count_devices;
}


void iface_sdl_joy_subsystem_init ( void ) {
    if ( SDL_WasInit ( SDL_INIT_JOYSTICK ) ) return;
#if LINUX
    SDL_Log ( "Initializing SDL Joystick Subsystem (here can be a bit time delay...)" );
#else
    SDL_Log ( "Initializing SDL Joystick Subsystem" );
#endif
    SDL_InitSubSystem ( SDL_INIT_JOYSTICK );
    int count_devices = iface_sdl_joy_available_realdev_count ( );
    SDL_Log ( "Found %d joystick device(s)", count_devices );
}


void iface_sdl_joy_set_default_params ( st_IFACE_JOY_PARAMS *params ) {
    params->x_axis = 0;
    params->x_invert = SDL_FALSE;
    params->x_center = 0;
    params->y_axis = 1;
    params->y_center = 0;
    params->y_invert = SDL_FALSE;
    params->trig1 = 0;
    params->trig2 = 1;
}


static void iface_sdl_joy_reset_dev_struct ( st_IFACE_JOY_DEVICE *ifacejoydev ) {
    memset ( ifacejoydev, 0x00, sizeof ( st_IFACE_JOY_DEVICE ) );
    st_IFACE_JOY_SYSDEVICE *joy_sysdevice = &ifacejoydev->joy_sysdevice;
    joy_sysdevice->id = -1;
    st_IFACE_JOY_PARAMS *params = &ifacejoydev->params;
    iface_sdl_joy_set_default_params ( params );
}


static void iface_sdl_joy_close_dev ( st_IFACE_JOY_DEVICE *ifacejoydev ) {

    if ( SDL_WasInit ( SDL_INIT_JOYSTICK ) ) {
        if ( SDL_JoystickGetAttached ( ifacejoydev->joy ) ) {
            SDL_JoystickClose ( ifacejoydev->joy );
        };
    };

    ifacejoydev->joy = NULL;
}


static int iface_sdl_joy_compare_sysdevices ( st_IFACE_JOY_SYSDEVICE *sysdevice1, st_IFACE_JOY_SYSDEVICE *sysdevice2, SDL_bool compare_ids ) {

    if ( ( compare_ids ) && ( sysdevice1->id != sysdevice2->id ) ) {
        return EXIT_FAILURE;
    };

    if ( ( 0 == strcmp ( sysdevice1->name, sysdevice2->name ) ) &&
         ( sysdevice1->axes == sysdevice2->axes ) &&
         ( sysdevice1->balls == sysdevice2->balls ) &&
         ( sysdevice1->buttons == sysdevice2->buttons ) ) {
        return EXIT_SUCCESS;
    };

    return EXIT_FAILURE;
}


int iface_sdl_joy_open_sysdevice ( en_JOY_DEVID joy_devid, st_IFACE_JOY_SYSDEVICE *joy_sysdevice ) {

    if ( !SDL_WasInit ( SDL_INIT_JOYSTICK ) ) return EXIT_FAILURE;
    if ( SDL_NumJoysticks ( ) < joy_sysdevice->id ) return EXIT_FAILURE;
    if ( joy_sysdevice->id < 0 ) return EXIT_FAILURE;

    st_IFACE_JOY_DEVICE *ifacejoydev = &g_iface_joy.dev[joy_devid];
    st_IFACE_JOY_SYSDEVICE *my_joy_sysdevice = &ifacejoydev->joy_sysdevice;

    if ( ifacejoydev->joy ) {

        if ( EXIT_SUCCESS == iface_sdl_joy_compare_sysdevices ( my_joy_sysdevice, joy_sysdevice, SDL_TRUE ) ) {
            return EXIT_SUCCESS;
        };

        iface_sdl_joy_close_dev ( ifacejoydev );
    };

    ifacejoydev->joy = SDL_JoystickOpen ( joy_sysdevice->id );

    if ( !ifacejoydev->joy ) {
        fprintf ( stderr, "%s(%d) - Error: Couldn't open Joystick device id=%d\n", __func__, __LINE__, joy_sysdevice->id );
        return EXIT_FAILURE;
    };

    SDL_Log ( "JOY(%d) connected - %d:%s", joy_devid, joy_sysdevice->id, joy_sysdevice->name );

    my_joy_sysdevice->id = joy_sysdevice->id;

    int device_name_length = strlen ( SDL_JoystickName ( ifacejoydev->joy ) ) + 1;
    if ( !my_joy_sysdevice->name ) {
        my_joy_sysdevice->name = ui_utils_mem_alloc0 ( device_name_length );
    } else {
        my_joy_sysdevice->name = ui_utils_mem_realloc ( my_joy_sysdevice->name, device_name_length );
    };

    strncpy ( my_joy_sysdevice->name, SDL_JoystickName ( ifacejoydev->joy ), device_name_length );

    my_joy_sysdevice->axes = SDL_JoystickNumAxes ( ifacejoydev->joy );
    my_joy_sysdevice->buttons = SDL_JoystickNumButtons ( ifacejoydev->joy );
    my_joy_sysdevice->balls = SDL_JoystickNumBalls ( ifacejoydev->joy );

    return EXIT_SUCCESS;
}


int iface_sdl_joy_open_configured_joyid ( en_JOY_DEVID joy_devid ) {

    st_IFACE_JOY_DEVICE *ifacejoydev = &g_iface_joy.dev[joy_devid];
    st_IFACE_JOY_SYSDEVICE *joy_sysdev = &ifacejoydev->joy_sysdevice;

    if ( !SDL_WasInit ( SDL_INIT_JOYSTICK ) ) return EXIT_FAILURE;
    if ( joy_sysdev->id == -1 ) return EXIT_FAILURE;
    if ( ifacejoydev->joy ) return EXIT_SUCCESS;

    int count_devices = iface_sdl_joy_available_realdev_count ( );

    if ( joy_sysdev->id < count_devices ) {

        st_IFACE_JOY_SYSDEVICE sysdevice;

        if ( EXIT_FAILURE == iface_sdl_joy_get_realdev_info ( joy_sysdev->id, &sysdevice ) ) return EXIT_FAILURE;

        if ( EXIT_SUCCESS == iface_sdl_joy_compare_sysdevices ( &sysdevice, joy_sysdev, SDL_TRUE ) ) {
            return iface_sdl_joy_open_sysdevice ( joy_devid, &sysdevice );
        };
    };

    SDL_Log ( "JOY(%d) device not found! %d:%s", joy_devid, joy_sysdev->id, joy_sysdev->name );
    return EXIT_FAILURE;
}


void iface_sdl_joy_set_params ( en_JOY_DEVID joy_devid, st_IFACE_JOY_PARAMS *params ) {
    st_IFACE_JOY_DEVICE *ifacejoydev = &g_iface_joy.dev[joy_devid];
    st_IFACE_JOY_PARAMS *my_params = &ifacejoydev->params;
    memcpy ( my_params, params, sizeof ( st_IFACE_JOY_PARAMS ) );
}


static void iface_sdl_joy_close_all_devices ( void ) {

    if ( SDL_WasInit ( SDL_INIT_JOYSTICK ) ) {
        SDL_Log ( "Closing Joystick Devices" );
        en_JOY_DEVID joy_devid;
        for ( joy_devid = 0; joy_devid < JOY_DEVID_COUNT; joy_devid++ ) {
            st_IFACE_JOY_DEVICE *ifacejoydev = &g_iface_joy.dev[joy_devid];
            iface_sdl_joy_close_dev ( ifacejoydev );
        };
    };
}


void iface_sdl_joy_subsystem_shutdown ( void ) {
    if ( SDL_WasInit ( SDL_INIT_JOYSTICK ) ) {
#if LINUX
        SDL_Log ( "Shutting down SDL Joystick Subsystem (here can be a bit time delay...)" );
#else
        SDL_Log ( "Shutting down SDL Joystick Subsystem" );
#endif
        iface_sdl_joy_close_all_devices ( );
        SDL_QuitSubSystem ( SDL_INIT_JOYSTICK );
    };
}


void iface_sdl_joy_quit ( void ) {
    iface_sdl_joy_subsystem_shutdown ( );

    en_JOY_DEVID joy_devid;
    for ( joy_devid = 0; joy_devid < JOY_DEVID_COUNT; joy_devid++ ) {
        st_IFACE_JOY_DEVICE *ifacejoydev = &g_iface_joy.dev[joy_devid];
        st_IFACE_JOY_SYSDEVICE *joy_sysdevice = &ifacejoydev->joy_sysdevice;

        if ( joy_sysdevice->name ) {
            ui_utils_mem_free ( joy_sysdevice->name );
        };
    };
}


int iface_sdl_joy_available_realdev_count ( void ) {
    if ( !SDL_WasInit ( SDL_INIT_JOYSTICK ) ) return -1;
    return SDL_NumJoysticks ( );
}


int iface_sdl_joy_get_realdev_info ( int joy_sysdevid, st_IFACE_JOY_SYSDEVICE *joy_sysdevice ) {
    if ( !SDL_WasInit ( SDL_INIT_JOYSTICK ) ) return EXIT_FAILURE;
    if ( SDL_NumJoysticks ( ) < joy_sysdevid ) return EXIT_FAILURE;
    if ( joy_sysdevid < 0 ) return EXIT_FAILURE;
    SDL_Joystick *joy = SDL_JoystickOpen ( joy_sysdevid );
    if ( !joy ) {
        fprintf ( stderr, "%s(%d) - Error: Couldn't open Joystick device id=%d\n", __func__, __LINE__, joy_sysdevid );
        return EXIT_FAILURE;
    };
    joy_sysdevice->id = joy_sysdevid;

    int device_name_length = strlen ( SDL_JoystickName ( joy ) );
    joy_sysdevice->name = ui_utils_mem_alloc0 ( device_name_length + 1 );
    strncpy ( joy_sysdevice->name, SDL_JoystickName ( joy ), device_name_length );

    joy_sysdevice->axes = SDL_JoystickNumAxes ( joy );
    joy_sysdevice->buttons = SDL_JoystickNumButtons ( joy );
    joy_sysdevice->balls = SDL_JoystickNumBalls ( joy );
    SDL_JoystickClose ( joy );
    return EXIT_SUCCESS;
}


static void iface_sdl_joy_savecfg_sysdev_name ( void *e, void *data ) {
    st_IFACE_JOY_SYSDEVICE *joy_sysdevice = (st_IFACE_JOY_SYSDEVICE*) data;
    char *name;
    if ( joy_sysdevice->name ) {
        name = joy_sysdevice->name;
    } else {
        name = cfgelement_get_text_default_value ( (CFGELM *) e );
    };
    cfgelement_set_text_value ( (CFGELM *) e, name );
}


static void iface_sdl_joy_propagatecfg_sysdev_name ( void *e, void *data ) {
    st_IFACE_JOY_SYSDEVICE *joy_sysdevice = (st_IFACE_JOY_SYSDEVICE*) data;
    char *name = cfgelement_get_text_value ( (CFGELM *) e );
    int length = strlen ( name );
    if ( length ) {
        joy_sysdevice->name = ui_utils_mem_alloc0 ( length + 1 );
        strncpy ( joy_sysdevice->name, name, length );
    } else {
        joy_sysdevice->name = NULL;
    };
}


static void iface_sdl_joy_savecfg_int2txt ( void *e, void *data ) {
    int *value = (int*) data;
    char txtvalue[12]; // int32 -> txt

    txtvalue[sizeof ( txtvalue ) - 1] = 0x00;

    snprintf ( txtvalue, sizeof ( txtvalue ) - 1, "%d", *value );
    cfgelement_set_text_value ( (CFGELM *) e, txtvalue );
}


static int strisnum ( char *str ) {
    if ( *str == '-' ) {
        str++;
        if ( *str == 0x00 ) return EXIT_FAILURE;
    };
    while ( *str != 0x00 ) {
        if ( 0 == isdigit ( *str++ ) ) return EXIT_FAILURE;
    };
    return EXIT_SUCCESS;
}


static void iface_sdl_joy_propagatecfg_txt2int ( void *e, void *data ) {

    int *value_poi = (int*) data;
    int value;

    char *txt = cfgelement_get_text_value ( (CFGELM *) e );

    if ( EXIT_SUCCESS == strisnum ( txt ) ) {
        value = atoi ( txt );
    } else {
        value = atoi ( cfgelement_get_text_default_value ( (CFGELM *) e ) );
    };

    *value_poi = value;
}


static void iface_sdl_joy_sysdev_cfg ( CFGMOD *cmod, en_JOY_DEVID joy_devid ) {

    st_IFACE_JOY_DEVICE *ifacejoydev = &g_iface_joy.dev[joy_devid];
    st_IFACE_JOY_SYSDEVICE *joy_sysdevice = &ifacejoydev->joy_sysdevice;
    st_IFACE_JOY_PARAMS *params = &ifacejoydev->params;

    CFGELM *elm;

    char elm_name[100];
    elm_name[sizeof ( elm_name ) - 1] = 0x00;

    int elm_name_size = sizeof ( elm_name ) - 1;
    char joycode = '1' + joy_devid;

    snprintf ( elm_name, elm_name_size, "joy%c_sysdev_id", joycode );
    elm = cfgmodule_register_new_element ( cmod, elm_name, CFGENTYPE_TEXT, "-1" );
    cfgelement_set_propagate_cb ( elm, iface_sdl_joy_propagatecfg_txt2int, (void*) &joy_sysdevice->id );
    cfgelement_set_save_cb ( elm, iface_sdl_joy_savecfg_int2txt, (void*) &joy_sysdevice->id );

    snprintf ( elm_name, elm_name_size, "joy%c_sysdev_name", joycode );
    elm = cfgmodule_register_new_element ( cmod, elm_name, CFGENTYPE_TEXT, "" );
    cfgelement_set_propagate_cb ( elm, iface_sdl_joy_propagatecfg_sysdev_name, (void*) joy_sysdevice );
    cfgelement_set_save_cb ( elm, iface_sdl_joy_savecfg_sysdev_name, (void*) joy_sysdevice );

    snprintf ( elm_name, elm_name_size, "joy%c_sysdev_axes", joycode );
    elm = cfgmodule_register_new_element ( cmod, elm_name, CFGENTYPE_UNSIGNED, 0, 0, 0xffffffff );
    cfgelement_set_handlers ( elm, (void*) &joy_sysdevice->axes, (void*) &joy_sysdevice->axes );

    snprintf ( elm_name, elm_name_size, "joy%c_sysdev_buttons", joycode );
    elm = cfgmodule_register_new_element ( cmod, elm_name, CFGENTYPE_UNSIGNED, 0, 0, 0xffffffff );
    cfgelement_set_handlers ( elm, (void*) &joy_sysdevice->buttons, (void*) &joy_sysdevice->buttons );

    snprintf ( elm_name, elm_name_size, "joy%c_sysdev_balls", joycode );
    elm = cfgmodule_register_new_element ( cmod, elm_name, CFGENTYPE_UNSIGNED, 0, 0, 0xffffffff );
    cfgelement_set_handlers ( elm, (void*) &joy_sysdevice->balls, (void*) &joy_sysdevice->balls );

    /* params */

    snprintf ( elm_name, elm_name_size, "joy%c_x_axis", joycode );
    elm = cfgmodule_register_new_element ( cmod, elm_name, CFGENTYPE_TEXT, "0" );
    cfgelement_set_propagate_cb ( elm, iface_sdl_joy_propagatecfg_txt2int, (void*) &params->x_axis );
    cfgelement_set_save_cb ( elm, iface_sdl_joy_savecfg_int2txt, (void*) &params->x_axis );

    snprintf ( elm_name, elm_name_size, "joy%c_x_axis_invert", joycode );
    elm = cfgmodule_register_new_element ( cmod, elm_name, CFGENTYPE_BOOL, SDL_FALSE );
    cfgelement_set_handlers ( elm, (void*) &params->x_invert, (void*) &params->x_invert );

    snprintf ( elm_name, elm_name_size, "joy%c_x_center", joycode );
    elm = cfgmodule_register_new_element ( cmod, elm_name, CFGENTYPE_TEXT, "0" );
    cfgelement_set_propagate_cb ( elm, iface_sdl_joy_propagatecfg_txt2int, (void*) &params->x_center );
    cfgelement_set_save_cb ( elm, iface_sdl_joy_savecfg_int2txt, (void*) &params->x_center );

    snprintf ( elm_name, elm_name_size, "joy%c_y_axis", joycode );
    elm = cfgmodule_register_new_element ( cmod, elm_name, CFGENTYPE_TEXT, "1" );
    cfgelement_set_propagate_cb ( elm, iface_sdl_joy_propagatecfg_txt2int, (void*) &params->y_axis );
    cfgelement_set_save_cb ( elm, iface_sdl_joy_savecfg_int2txt, (void*) &params->y_axis );

    snprintf ( elm_name, elm_name_size, "joy%c_y_axis_invert", joycode );
    elm = cfgmodule_register_new_element ( cmod, elm_name, CFGENTYPE_BOOL, SDL_FALSE );
    cfgelement_set_handlers ( elm, (void*) &params->y_invert, (void*) &params->y_invert );

    snprintf ( elm_name, elm_name_size, "joy%c_y_center", joycode );
    elm = cfgmodule_register_new_element ( cmod, elm_name, CFGENTYPE_TEXT, "0" );
    cfgelement_set_propagate_cb ( elm, iface_sdl_joy_propagatecfg_txt2int, (void*) &params->y_center );
    cfgelement_set_save_cb ( elm, iface_sdl_joy_savecfg_int2txt, (void*) &params->y_center );

    snprintf ( elm_name, elm_name_size, "joy%c_trig1", joycode );
    elm = cfgmodule_register_new_element ( cmod, elm_name, CFGENTYPE_TEXT, "0" );
    cfgelement_set_propagate_cb ( elm, iface_sdl_joy_propagatecfg_txt2int, (void*) &params->trig1 );
    cfgelement_set_save_cb ( elm, iface_sdl_joy_savecfg_int2txt, (void*) &params->trig1 );

    snprintf ( elm_name, elm_name_size, "joy%c_trig2", joycode );
    elm = cfgmodule_register_new_element ( cmod, elm_name, CFGENTYPE_TEXT, "1" );
    cfgelement_set_propagate_cb ( elm, iface_sdl_joy_propagatecfg_txt2int, (void*) &params->trig2 );
    cfgelement_set_save_cb ( elm, iface_sdl_joy_savecfg_int2txt, (void*) &params->trig2 );
}


void iface_sdl_joy_init ( void ) {

    en_JOY_DEVID joy_devid;

    for ( joy_devid = 0; joy_devid < JOY_DEVID_COUNT; joy_devid++ ) {
        st_IFACE_JOY_DEVICE *ifacejoydev = &g_iface_joy.dev[joy_devid];
        iface_sdl_joy_reset_dev_struct ( ifacejoydev );
    };

    CFGMOD *cmod = cfgroot_register_new_module ( g_cfgmain, "IFACE_SDL_JOYSTICK" );

    CFGELM *elm;
    elm = cfgmodule_register_new_element ( cmod, "startup_joy_subsystem", CFGENTYPE_BOOL, SDL_FALSE );
    cfgelement_set_handlers ( elm, (void*) &g_iface_joy.startup_joy_subsystem, (void*) &g_iface_joy.startup_joy_subsystem );

    for ( joy_devid = 0; joy_devid < JOY_DEVID_COUNT; joy_devid++ ) {
        iface_sdl_joy_sysdev_cfg ( cmod, joy_devid );
    };

    cfgmodule_parse ( cmod );
    cfgmodule_propagate ( cmod );

    if ( g_iface_joy.startup_joy_subsystem ) {
        iface_sdl_joy_subsystem_init ( );
    };
}


static void iface_sdl_joy_get_position ( en_JOY_DEVID joy_devid, st_IFACE_JOY_POSITION *pos ) {

    st_IFACE_JOY_DEVICE *dev = &g_iface_joy.dev[joy_devid];
    st_IFACE_JOY_PARAMS *params = &dev->params;

    pos->x = SDL_JoystickGetAxis ( dev->joy, params->x_axis );
    if ( ( params->x_invert ) && ( pos->x != 0 ) ) {
        pos->x = ~pos->x;
    };

    pos->y = SDL_JoystickGetAxis ( dev->joy, params->y_axis );
    if ( ( params->y_invert ) && ( pos->y != 0 ) ) {
        pos->y = ~pos->y;
    };
}


void iface_sdl_joy_get_calibration ( void ) {
    en_JOY_DEVID joy_devid;
    for ( joy_devid = 0; joy_devid < JOY_DEVID_COUNT; joy_devid++ ) {
        st_IFACE_JOY_DEVICE *dev = &g_iface_joy.dev[joy_devid];
        if ( dev->joy ) {
            st_IFACE_JOY_PARAMS *params = &dev->params;
            if ( SDL_JoystickGetButton ( dev->joy, params->trig1 ) ) {
                st_IFACE_JOY_POSITION pos;
                iface_sdl_joy_get_position ( joy_devid, &pos );
                SDL_Log ( "JOY(%d) calibration complete: x_center = %d, y_center = %d", joy_devid, pos.x, pos.y );
                params->x_center = pos.x;
                params->y_center = pos.y;
                ui_joy_sysdevice_calibration ( joy_devid, params );
            };
        };
    };
}


Z80EX_BYTE iface_sdl_joy_scan ( en_JOY_DEVID joy_devid ) {

    st_IFACE_JOY_DEVICE *dev = &g_iface_joy.dev[joy_devid];
    st_IFACE_JOY_PARAMS *params = &dev->params;

    Z80EX_BYTE status = 0xff;

    if ( !dev->joy ) return status;

    st_IFACE_JOY_POSITION pos;

    iface_sdl_joy_get_position ( joy_devid, &pos );

    if ( pos.y < params->y_center ) {
        JOY_STATEBIT_RESET ( status, JOY_STATEBIT_UP );
    } else if ( pos.y > params->y_center ) {
        JOY_STATEBIT_RESET ( status, JOY_STATEBIT_DOWN );
    };

    if ( pos.x < params->x_center ) {
        JOY_STATEBIT_RESET ( status, JOY_STATEBIT_LEFT );
    } else if ( pos.x > params->x_center ) {
        JOY_STATEBIT_RESET ( status, JOY_STATEBIT_RIGHT );
    };

    if ( SDL_JoystickGetButton ( dev->joy, params->trig1 ) ) {
        JOY_STATEBIT_RESET ( status, JOY_STATEBIT_TRIG1 );
    };

    if ( SDL_JoystickGetButton ( dev->joy, params->trig2 ) ) {
        JOY_STATEBIT_RESET ( status, JOY_STATEBIT_TRIG2 );
    };

    return status;
}
