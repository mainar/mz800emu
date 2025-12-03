/* 
 * File:   iface_sdl_joy.h
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


#ifndef IFACE_SDL_JOY_H
#define IFACE_SDL_JOY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "iface_sdl.h"

#include "mz800.h"
#include "joy/joy.h"


    typedef struct st_IFACE_JOY_POSITION {
        Sint16 x;
        Sint16 y;
    } st_IFACE_JOY_POSITION;


    typedef struct st_IFACE_JOY_SYSDEVICE {
        int id;
        char *name;
        int axes;
        int buttons;
        int balls;
    } st_IFACE_JOY_SYSDEVICE;


    typedef struct st_IFACE_JOY_PARAMS {
        int x_axis;
        SDL_bool x_invert;
        Sint16 x_center;
        int y_axis;
        SDL_bool y_invert;
        Sint16 y_center;
        int trig1;
        int trig2;
    } st_IFACE_JOY_PARAMS;


    typedef struct st_IFACE_JOY_DEVICE {
        st_IFACE_JOY_SYSDEVICE joy_sysdevice;
        SDL_Joystick *joy;
        st_IFACE_JOY_PARAMS params;
    } st_IFACE_JOY_DEVICE;


    typedef struct st_IFACE_JOY {
        SDL_bool startup_joy_subsystem;
        st_IFACE_JOY_DEVICE dev[JOY_DEVID_COUNT];
    } st_IFACE_JOY;

    extern st_IFACE_JOY g_iface_joy;

    extern void iface_sdl_joy_init ( void );
    extern void iface_sdl_joy_quit ( void );
    extern int iface_sdl_joy_rescan_devices ( void );
    extern void iface_sdl_joy_subsystem_init ( void );
    extern void iface_sdl_joy_subsystem_shutdown ( void );
    extern int iface_sdl_joy_available_realdev_count ( void );
    extern int iface_sdl_joy_get_realdev_info ( int joy_sysdevid, st_IFACE_JOY_SYSDEVICE *joy_sysdevice );
    extern int iface_sdl_joy_open_sysdevice ( en_JOY_DEVID joy_devid, st_IFACE_JOY_SYSDEVICE *joy_sysdevice );
    extern void iface_sdl_joy_set_params ( en_JOY_DEVID joy_devid, st_IFACE_JOY_PARAMS *params );
    extern void iface_sdl_joy_set_default_params ( st_IFACE_JOY_PARAMS *params );
    extern int iface_sdl_joy_open_configured_joyid ( en_JOY_DEVID joy_devid );
    extern void iface_sdl_joy_get_calibration ( void );
    extern Z80EX_BYTE iface_sdl_joy_scan ( en_JOY_DEVID joy_devid );


#ifdef __cplusplus
}
#endif

#endif /* IFACE_SDL_JOY_H */

