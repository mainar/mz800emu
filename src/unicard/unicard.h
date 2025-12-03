/* 
 * File:   unicard.h
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 21. června 2018, 16:08
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


#ifndef UNICARD_H
#define UNICARD_H

#ifdef __cplusplus
extern "C" {
#endif

#define UNICARD_EMULATED

#include <stdio.h>
#include <stdint.h>
#include <glib.h>

#ifdef UNICARD_EMULATED
#include "ff_result.h"
#include "ff_open_mode.h"
#define _MAX_LFN    32  // maximalni povolena delka filename, vcetne terminatoru
#endif


    typedef struct st_UNICARD_FILE {
        char *filepath;
        FILE *fh;
        uint8_t mode;
    } st_UNICARD_FILE;


    typedef struct st_UNICARD_DIR {
        char *dirpath;
        GDir *dh;
        int position; // pozice, kterou prave cteme (na nulte pozici vygenerujeme '..')
    } st_UNICARD_DIR;


    typedef struct st_UNICARD_RTC {
        uint8_t hour;
        uint8_t min;
        uint8_t sec;
        uint8_t day;
        uint16_t year;
        uint8_t month;
    } st_UNICARD_RTC;


    typedef enum en_UNICARD_CONNECTION {
        UNICARD_CONNECTION_DISCONNECTED = 0,
        UNICARD_CONNECTION_CONNECTED = 1
    } en_UNICARD_CONNECTION;


    typedef struct st_UNICARD {
        en_UNICARD_CONNECTION connected;
        //char *sd_root;
        char *work_dir; // na unikarte je to vlastnost FatFS
    } st_UNICARD;

    extern st_UNICARD g_unicard;

    extern void unicard_init ( void );
    extern void unicard_exit ( void );
    extern void unicard_reset ( void );
    extern char* unicard_get_mzfloader_image_filepath ( void );
    extern void unicard_set_connected ( en_UNICARD_CONNECTION conn );

    extern char* unicard_get_sd_root_dirpath ( void );
    extern void unicard_set_sd_root_dirpath ( char *dirpath );

    extern void unicard_write_byte ( uint16_t port, uint8_t data );
    extern uint8_t unicard_read_byte ( uint16_t port );

    extern void unicard_read_rtc ( st_UNICARD_RTC *rtc );

    extern FRESULT unicard_chdir ( char *dirpath );
    extern int unicard_get_cwd ( FRESULT *ff_res, char *buff, int buff_size );

    extern void unicard_dir_init ( st_UNICARD_DIR *dir );
    extern FRESULT unicard_dir_open ( st_UNICARD_DIR *dir, char *dirpath );
    extern FRESULT unicard_dir_close ( st_UNICARD_DIR *dir );
    extern FRESULT unicard_dir_read_filelist ( st_UNICARD_DIR *dir, char *buff, int buff_size );
    extern int unicard_dir_is_open ( st_UNICARD_DIR *dir );

    extern void unicard_file_init ( st_UNICARD_FILE *file );
    extern FRESULT unicard_file_open ( st_UNICARD_FILE *file, char *filepath, uint8_t fa_mode );
    extern FRESULT unicard_file_close ( st_UNICARD_FILE *file );
    extern FRESULT unicard_file_read ( st_UNICARD_FILE *file, uint8_t *buff, uint32_t buff_size, uint32_t *read_len );
    extern int unicard_file_is_open ( st_UNICARD_FILE *file );
    extern int unicard_file_is_eof ( st_UNICARD_FILE *file );

    extern void unicard_fdc_mount ( uint8_t drive_id, char *filepath );

#define TEST_UNICARD_CONNECTED ( g_unicard.connected == UNICARD_CONNECTION_CONNECTED )

#define UNICARD_DEFAULT_SD_ROOT     "SD"
#define UNICARD_DEFAULT_DIR_MODE     0775

#define UNICARD_UNIMGR_DIR                  "unicard"
    // /unicard/fd0.cfg
#define UNICARD_FDCFG_FILE1                 "/unicard/fd"
#define UNICARD_FDCFG_FILE2                 ".cfg"

#define UNICARD_UNIMGR_MZFLOADER_MZQ        "mzfloader.mzq"
#define UNICARD_UNIMGR_MZFLOADER_CFG        "mzfloader.cfg"
#define UNICARD_UNIMGR_MGR_MZF              "mgr.mzf"

#ifdef __cplusplus
}
#endif

#endif /* UNICARD_H */

