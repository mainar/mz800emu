/* 
 * File:   unicard.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 21. června 2018, 16:07
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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <glib/gstdio.h>

#include "ui/ui_utils.h"
#include "ui/ui_unicard.h"
#include "fs_layer.h"
#include "cfgmain.h"
#include "qdisk/qdisk.h"


#include "ff_result.h"
#include "unicard.h"
#include "unimgr.h"
#include "MGR_MZF.h"
#include "MZFLOADER_MZQ.h"
#include "fdc/fdc.h"

st_UNICARD g_unicard;

static CFGELM *g_elm_connected;
static CFGELM *g_elm_sd_root;


char* unicard_get_sd_root_dirpath ( void ) {
    return cfgelement_get_text_value ( g_elm_sd_root );
}


void unicard_set_sd_root_dirpath ( char *sd_root ) {
    if ( ( !sd_root ) || ( sd_root[0] == 0x00 ) ) return;
    cfgelement_set_text_value ( g_elm_sd_root, sd_root );
    char *sd_root_locale = ui_utils_file_name_locale_from_utf8 ( sd_root );
    printf ( "Unicard: new SD root is '%s'\n", sd_root_locale );
    g_free ( sd_root_locale );
}


static int unicard_init_sd_dir ( char *dirpath, char *description ) {

    char *dirpath_locale = ui_utils_file_name_locale_from_utf8 ( dirpath );

    if ( !g_file_test ( dirpath, G_FILE_TEST_EXISTS ) ) {
        printf ( "%s: '%s'\n", description, dirpath_locale );
        if ( -1 == g_mkdir_with_parents ( dirpath, UNICARD_DEFAULT_DIR_MODE ) ) {
            fprintf ( stderr, "%s():%d - Can't create directory '%s'\n", __func__, __LINE__, dirpath_locale );
            g_free ( dirpath_locale );
            return EXIT_FAILURE;
        };
    };

    if ( !g_file_test ( dirpath, G_FILE_TEST_IS_DIR ) ) {
        fprintf ( stderr, "%s():%d - This is not a directory '%s'\n", __func__, __LINE__, dirpath_locale );
        g_free ( dirpath_locale );
        return EXIT_FAILURE;
    };

    g_free ( dirpath_locale );
    return EXIT_SUCCESS;
}


static int unicard_init_sd_file ( char *filepath, const uint8_t *src, uint32_t size ) {

    if ( !g_file_test ( filepath, G_FILE_TEST_EXISTS ) ) {
        char *filepath_locale = ui_utils_file_name_locale_from_utf8 ( filepath );
        printf ( "Unicard - create file: '%s'\n", filepath_locale );
        FILE *fh = g_fopen ( filepath, "wb" );
        if ( !fh ) {
            fprintf ( stderr, "%s():%d - Can't create file '%s'\n", __func__, __LINE__, filepath_locale );
            g_free ( filepath_locale );
            return EXIT_FAILURE;
        };

        uint32_t write_len = 0;
        int ret = fs_layer_fwrite ( &fh, (uint8_t*) src, size, &write_len );
        fclose ( fh );
        if ( ( ret != FS_LAYER_FR_OK ) || ( write_len != size ) ) {
            fprintf ( stderr, "%s():%d - Can't write file '%s'\n", __func__, __LINE__, filepath_locale );
            g_free ( filepath_locale );
            return EXIT_FAILURE;
        };

        g_free ( filepath_locale );
    };

    return EXIT_SUCCESS;
}


static int unicard_init_sd ( char *dirpath ) {

    int ret;

    ret = unicard_init_sd_dir ( dirpath, "Unicard - create SD root" );
    if ( EXIT_SUCCESS != ret ) return EXIT_FAILURE;

    char *unimgr_dir = g_build_filename ( dirpath, UNICARD_UNIMGR_DIR, NULL );

    ret = unicard_init_sd_dir ( unimgr_dir, "Unicard - create unimgr directory" );
    ui_utils_mem_free ( unimgr_dir );
    if ( EXIT_SUCCESS != ret ) return EXIT_FAILURE;

    char *filepath = NULL;

    filepath = g_build_filename ( dirpath, UNICARD_UNIMGR_DIR, UNICARD_UNIMGR_MZFLOADER_MZQ, NULL );
    ret = unicard_init_sd_file ( filepath, c_UNICARD_MZFLOADER_MZQ, UNICARD_MZFLOADER_MZQ_SIZE );
    ui_utils_mem_free ( filepath );
    if ( EXIT_SUCCESS != ret ) return EXIT_FAILURE;

    filepath = g_build_filename ( dirpath, UNICARD_UNIMGR_DIR, UNICARD_UNIMGR_MGR_MZF, NULL );
    ret = unicard_init_sd_file ( filepath, c_UNICARD_MGR_MZF, UNICARD_MGR_MZF_SIZE );
    ui_utils_mem_free ( filepath );
    if ( EXIT_SUCCESS != ret ) return EXIT_FAILURE;

    filepath = g_build_filename ( dirpath, UNICARD_UNIMGR_DIR, UNICARD_UNIMGR_MZFLOADER_CFG, NULL );

    uint32_t len = 1 + strlen ( UNICARD_UNIMGR_DIR ) + 1 + strlen ( UNICARD_UNIMGR_MGR_MZF ) + 2;
    char *buff = (char*) ui_utils_mem_alloc0 ( len );
    snprintf ( buff, len, "/%s/%s%c", UNICARD_UNIMGR_DIR, UNICARD_UNIMGR_MGR_MZF, 0x0d );

    ret = unicard_init_sd_file ( filepath, (uint8_t*) buff, len - 1 );
    ui_utils_mem_free ( buff );
    ui_utils_mem_free ( filepath );
    if ( EXIT_SUCCESS != ret ) return EXIT_FAILURE;

    return EXIT_SUCCESS;
}


char* unicard_get_mzfloader_image_filepath ( void ) {
    if ( TEST_UNICARD_CONNECTED ) {
        return g_build_filename ( unicard_get_sd_root_dirpath ( ), UNICARD_UNIMGR_DIR, UNICARD_UNIMGR_MZFLOADER_MZQ, NULL );
    };
    return (char*) ui_utils_mem_alloc0 ( 1 );
}


void unicard_set_connected ( en_UNICARD_CONNECTION conn ) {

    if ( strlen ( unicard_get_sd_root_dirpath ( ) ) == 0 ) {
        char *sd_root = (char*) ui_utils_mem_alloc0 ( strlen ( UNICARD_DEFAULT_SD_ROOT ) + 1 );
        unicard_set_sd_root_dirpath ( sd_root );
        ui_utils_mem_free ( sd_root );
    };

    if ( g_unicard.connected == conn ) return;

    if ( conn == UNICARD_CONNECTION_CONNECTED ) {
        if ( EXIT_SUCCESS != unicard_init_sd ( unicard_get_sd_root_dirpath ( ) ) ) {
            ui_unicard_update_menu ( );
            return;
        };
        g_unicard.connected = UNICARD_CONNECTION_CONNECTED;
        unimgr_init ( );
        qdisk_activate_unicard_boot_loader ( );
        printf ( "INFO: The Unicard device is connected. Press RESET (F12) + Q for boot into Unicard Manager\n" );
    } else {
        unimgr_exit ( ); // pozavirat otevrene deskriptory
        g_unicard.connected = UNICARD_CONNECTION_DISCONNECTED;
        qdisk_deactivate_unicard_boot_loader ( );
        printf ( "INFO: The Unicard device is disconnected\n" );
    };
    ui_unicard_update_menu ( );
}


void unicard_init ( void ) {

    g_unicard.connected = UNICARD_CONNECTION_DISCONNECTED;
    g_unicard.work_dir = (char*) ui_utils_mem_alloc0 ( 2 );
    g_unicard.work_dir[0] = '/';
    unimgr_init ( );

    CFGMOD *cmod = cfgroot_register_new_module ( g_cfgmain, "UNICARD" );

    g_elm_connected = cfgmodule_register_new_element ( cmod, "connected", CFGENTYPE_BOOL, UNICARD_CONNECTION_DISCONNECTED );
    cfgelement_set_handlers ( g_elm_connected, (void*) NULL, (void*) &g_unicard.connected );
    g_elm_sd_root = cfgmodule_register_new_element ( cmod, "sd_root", CFGENTYPE_TEXT, UNICARD_DEFAULT_SD_ROOT );

    cfgmodule_parse ( cmod );

    unicard_set_connected ( cfgelement_get_bool_value ( g_elm_connected ) );

    if ( !TEST_UNICARD_CONNECTED ) qdisk_deactivate_unicard_boot_loader ( );
}


void unicard_exit ( void ) {
    unimgr_exit ( );
    ui_utils_mem_free ( g_unicard.work_dir );
}


void unicard_reset ( void ) {
    if ( !TEST_UNICARD_CONNECTED ) return;
    unimgr_reset ( );
}


void unicard_write_byte ( uint16_t port, uint8_t data ) {
    switch ( port & 0xff ) {
        case 0x50:
        case 0x51:
            unimgr_write_byte ( port & 0x01, data );
            break;
        default:
            fprintf ( stderr, "%s():%d - Unsupported Unicard port 0x%02x\n", __func__, __LINE__, port );
    };
}


uint8_t unicard_read_byte ( uint16_t port ) {
    uint8_t ret = 0x00;

    switch ( port & 0xff ) {
        case 0x50:
        case 0x51:
            ret = unimgr_read_byte ( port & 0x01 );
            break;
        default:
            fprintf ( stderr, "%s():%d - Unsupported Unicard port 0x%02x\n", __func__, __LINE__, port );
    };
    return ret;
}


void unicard_read_rtc ( st_UNICARD_RTC *rtc ) {
    GTimeZone *tz = g_time_zone_new_local ( );
    GDateTime *datetime = g_date_time_new_now ( tz );
    rtc->year = g_date_time_get_year ( datetime );
    rtc->month = g_date_time_get_month ( datetime );
    rtc->day = g_date_time_get_day_of_month ( datetime );
    rtc->hour = g_date_time_get_hour ( datetime );
    rtc->min = g_date_time_get_minute ( datetime );
    rtc->sec = g_date_time_get_second ( datetime );
    g_time_zone_unref ( tz );
    g_date_time_unref ( datetime );
}


/**
 * Z filepath vytvorime absolutni filepath ve ktere je:
 * 
 * - zredukovan pocet lomitek
 * - odstraneno /./, ./
 * - zpracovano ../
 * 
 * Neoveruje se, zda adresare, ktere jsou uvedeny v ceste existuji.
 * 
 * Vracime nove vytvoreny retezec.
 */
static char* unicard_create_naked_filepath ( const char *filepath ) {
    char *naked_filepath = (char*) ui_utils_mem_alloc0 ( 1 );

    if ( !filepath ) return naked_filepath;

    int filepath_len = strlen ( filepath );
    if ( !filepath_len ) return naked_filepath;

    char *complete_filepath;
    int complete_len;

    //printf ( "NKFP RECEIVED '%s'\n", filepath );

    if ( filepath[0] != '/' ) {
        complete_filepath = g_build_filename ( g_unicard.work_dir, filepath, NULL );
        complete_len = strlen ( complete_filepath );
    } else {
        complete_filepath = (char*) ui_utils_mem_alloc0 ( filepath_len + 1 );
        memcpy ( complete_filepath, filepath, filepath_len );
        complete_len = filepath_len;
    };

    int real_count_nodes = 0;
    char **real_node = (char**) ui_utils_mem_alloc ( sizeof ( char* ) );
    int search_start = 1;
    int *path_components = (int*) ui_utils_mem_alloc0 ( sizeof ( int ) );
    int count_path_components = 0;

    int i;
    for ( i = 0; i <= complete_len; i++ ) {
        if ( complete_filepath[i] == '/' ) {
            complete_filepath[i] = 0x00;
            search_start = 1;
        } else if ( search_start ) {
            if ( complete_filepath[i] == '.' ) {
                int j = i + 1;
                if ( ( complete_filepath[j] == '/' ) || ( complete_filepath[j] == 0x00 ) ) continue;
                if ( complete_filepath[j] == '.' ) {
                    int k = j + 1;
                    if ( ( complete_filepath[k] == '/' ) || ( complete_filepath[k] == 0x00 ) ) {
                        if ( count_path_components ) count_path_components--;
                        continue;
                    };
                };
            };
            if ( strlen ( &complete_filepath[i] ) ) {
                search_start = 0;
                real_node = (char**) ui_utils_mem_realloc ( real_node, sizeof ( char** ) * ( real_count_nodes + 1 ) );
                real_node[real_count_nodes] = &complete_filepath[i];
                path_components = (int*) ui_utils_mem_realloc ( path_components, sizeof ( int ) * ( count_path_components + 1 ) );
                path_components[count_path_components++] = real_count_nodes;
                real_count_nodes++;
            };
        };
    };

    if ( count_path_components ) {
        int naked_filepath_len = 1;
        for ( i = 0; i < count_path_components; i++ ) {
            int j = path_components[i];
            naked_filepath_len += strlen ( real_node[j] ) + 1 + 1;
            naked_filepath = (char*) ui_utils_mem_realloc ( naked_filepath, naked_filepath_len );
            strncat ( naked_filepath, "/", naked_filepath_len );
            strncat ( naked_filepath, real_node[j], naked_filepath_len );
        };
    } else {
        naked_filepath = (char*) ui_utils_mem_realloc ( naked_filepath, 2 );
        strncpy ( naked_filepath, "/", 2 );
    };

    //printf ( "NKFP OUTPUT: '%s'\n", naked_filepath );

    g_free ( complete_filepath );
    g_free ( path_components );
    g_free ( real_node );

    return naked_filepath;
}


FRESULT unicard_chdir ( char *dirpath ) {

    //printf ( "CHDIR: '%s'\n", dirpath );

    char *naked_dirpath = unicard_create_naked_filepath ( dirpath );
    char *full_dirpath = g_build_filename ( unicard_get_sd_root_dirpath ( ), naked_dirpath, NULL );

    char *full_dirpath_locale = ui_utils_file_name_locale_from_utf8 ( full_dirpath );

    if ( !g_file_test ( full_dirpath, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR ) ) {
        fprintf ( stderr, "%s():%d - Error: dirpath '%s'\n", __func__, __LINE__, full_dirpath_locale );
        ui_utils_mem_free ( naked_dirpath );
        ui_utils_mem_free ( full_dirpath );
        ui_utils_mem_free ( full_dirpath_locale );
        return FR_NO_PATH;
    };
    ui_utils_mem_free ( full_dirpath );
    ui_utils_mem_free ( full_dirpath_locale );

    g_unicard.work_dir = naked_dirpath;

    return FR_OK;
}


int unicard_get_cwd ( FRESULT *ff_res, char *buff, int buff_size ) {
    // na skutecne unikarte je potreba z vystupu getcwd odstranit prvni 2 znaky "0:" 
    // *ff_res = f_getcwd ( (char*) buff_local, buff_size );
    *ff_res = FR_OK;
    int len = strlen ( g_unicard.work_dir );
    if ( len < buff_size ) {
        memcpy ( buff, g_unicard.work_dir, len + 1 );
        return EXIT_SUCCESS;
    };
    return EXIT_FAILURE;
}


void unicard_dir_init ( st_UNICARD_DIR *dir ) {
    dir->dh = NULL;
    dir->dirpath = NULL;
}


FRESULT unicard_dir_close ( st_UNICARD_DIR *dir ) {
    if ( dir->dh != NULL ) {
        g_dir_close ( dir->dh );
        dir->dh = NULL;
        ui_utils_mem_free ( dir->dirpath );
        dir->dirpath = NULL;
    };
    return FR_OK;
}


FRESULT unicard_dir_open ( st_UNICARD_DIR *dir, char *dirpath ) {

    if ( dir->dh != NULL ) unicard_dir_close ( dir );

    char *naked_dirpath = unicard_create_naked_filepath ( dirpath );
    char *full_dirpath = g_build_filename ( unicard_get_sd_root_dirpath ( ), naked_dirpath, NULL );

    char *full_dirpath_locale = ui_utils_file_name_locale_from_utf8 ( full_dirpath );

    if ( !g_file_test ( full_dirpath, G_FILE_TEST_IS_DIR ) ) {
        fprintf ( stderr, "%s():%d - This is not DIR '%s'\n", __func__, __LINE__, full_dirpath_locale );
        ui_utils_mem_free ( full_dirpath );
        ui_utils_mem_free ( full_dirpath_locale );
        ui_utils_mem_free ( naked_dirpath );
        return FR_NO_PATH;
    };

    GError* err = NULL;
    dir->dh = g_dir_open ( full_dirpath, 0, &err );
    ui_utils_mem_free ( full_dirpath );
    ui_utils_mem_free ( full_dirpath_locale );

    if ( err != NULL ) {
        fprintf ( stderr, "%s():%d - Error: %s\n", __func__, __LINE__, err->message );
        g_error_free ( err );
        ui_utils_mem_free ( naked_dirpath );
        return FR_DISK_ERR;
    };

    dir->dirpath = naked_dirpath;
    dir->position = 0;

    return FR_OK;
}


FRESULT unicard_dir_read_filelist ( st_UNICARD_DIR *dir, char *buff, int buff_size ) {

    if ( dir->dh == NULL ) return FR_DISK_ERR;

    if ( ( dir->position == 0 ) && ( !( ( dir->dirpath[0] == 0x00 ) || ( 0 == strcmp ( dir->dirpath, "/" ) ) ) ) ) {
        //printf ( "DP: '%s'\n", dir->dirpath );
        dir->position = 1;
        snprintf ( buff, buff_size, "../%c0%c", 0x0d, 0x0d );
        return FR_OK;
    };

    const gchar *filename;
    int length;

    do {
        filename = g_dir_read_name ( dir->dh );

        if ( filename == NULL ) {
            buff[0] = 0x00;
            return FR_OK;
        };

        length = strlen ( filename );

        if ( length >= _MAX_LFN ) {
            fprintf ( stderr, "%s():%d - Filename is too long '%s'\n", __func__, __LINE__, filename );
        };

    } while ( length >= _MAX_LFN );

    dir->position++;

    gchar *filepath = g_build_filename ( unicard_get_sd_root_dirpath ( ), dir->dirpath, filename, NULL );

    if ( g_file_test ( filepath, G_FILE_TEST_IS_DIR ) ) {
        snprintf ( buff, buff_size, "%s/%c0%c", filename, 0x0d, 0x0d );
    } else {
        GStatBuf statbuf;
        g_stat ( filepath, &statbuf );
        snprintf ( buff, buff_size, "%s%c%d%c", filename, 0x0d, (uint32_t) statbuf.st_size, 0x0d );
    };

    g_free ( filepath );
    return FR_OK;
}


int unicard_dir_is_open ( st_UNICARD_DIR *dir ) {
    if ( dir->dh == NULL ) return EXIT_FAILURE;
    return EXIT_SUCCESS;
}


void unicard_file_init ( st_UNICARD_FILE *file ) {
    file->fh = NULL;
    file->filepath = NULL;
}


FRESULT unicard_file_close ( st_UNICARD_FILE *file ) {
    //printf ( "%s():%d\n", __func__, __LINE__ );
    if ( file->fh == NULL ) return FR_OK;
    //printf ( "OK - close\n" );
    // na unikarte udelat sync
    fclose ( file->fh );
    file->fh = NULL;
    ui_utils_mem_free ( file->filepath );
    file->filepath = NULL;
    return FR_OK;
}


FRESULT unicard_file_open ( st_UNICARD_FILE *file, char *filepath, uint8_t fa_mode ) {

    //printf ( "unicard_open_file: '%s', mode: 0x%02x\n", filepath, fa_mode );

    if ( file->fh != NULL ) unicard_file_close ( file );

    if ( fa_mode == FA_WRITE ) return FR_INVALID_PARAMETER;

    if ( !( fa_mode & ( FA_READ | FA_WRITE ) ) ) {
        fa_mode |= FA_READ;
    };

    const char *mode;

    if ( fa_mode & FA_CREATE_NEW ) {
        if ( fa_mode & FA_READ ) {
            mode = "w+xb"; // RW
        } else {
            mode = "wxb"; // W
        };
#if 0 // nemame v unikarte implementovano
    } else if ( fa_mode & FA_OPEN_APPEND ) {
        if ( fa_mode & FA_READ ) {
            mode = "a+b"; // RW
        } else {
            mode = "ab"; // W
        };
#endif
    } else if ( fa_mode & FA_OPEN_ALWAYS ) {
        if ( fa_mode & FA_READ ) {
            mode = "w+b"; // RW
        } else {
            mode = "wb"; // W
        };
    } else {
        if ( fa_mode & FA_WRITE ) {
            mode = "r+b"; // RW
        } else {
            mode = "rb"; // R
        };
    };

    char *naked_filepath = unicard_create_naked_filepath ( filepath );
    char *full_filepath = g_build_filename ( unicard_get_sd_root_dirpath ( ), naked_filepath, NULL );

    char *full_filepath_locale = ui_utils_file_name_locale_from_utf8 ( full_filepath );

    // fdc hack
    int fdcfg1_len = strlen ( UNICARD_FDCFG_FILE1 );
    if ( ( 0 == strncmp ( naked_filepath, UNICARD_FDCFG_FILE1, fdcfg1_len ) ) &&
         ( 0 == strcmp ( &naked_filepath[( fdcfg1_len + 1 )], UNICARD_FDCFG_FILE2 ) ) &&
         ( fa_mode == FA_READ ) ) {

        int drive_id = naked_filepath[fdcfg1_len] - '0';
        if ( ( drive_id >= 0 ) && ( drive_id <= 3 ) ) {
            const char *dsk_filepath = fdc_get_dsk_filepath ( drive_id );
            FILE *fh = g_fopen ( full_filepath, "wb" );
            if ( ( dsk_filepath ) && dsk_filepath[0] != 0x00 ) {
                int len = strlen ( unicard_get_sd_root_dirpath ( ) );
                if ( 0 == strncmp ( dsk_filepath, unicard_get_sd_root_dirpath ( ), len ) ) {
                    fprintf ( fh, "%s\n", &dsk_filepath[len] );
                } else {
                    char *basename = g_path_get_basename ( dsk_filepath );
                    fprintf ( fh, "***mz800emu path*** '%s'\n", basename );
                    ui_utils_mem_free ( basename );
                };
            };
            fclose ( fh );
        };
    };

    if ( !g_file_test ( full_filepath, G_FILE_TEST_EXISTS ) ) {
        fprintf ( stderr, "%s():%d - File not exists '%s'\n", __func__, __LINE__, full_filepath_locale );
        ui_utils_mem_free ( full_filepath );
        ui_utils_mem_free ( full_filepath_locale );
        ui_utils_mem_free ( naked_filepath );
        return FR_NO_FILE;
    };

    file->fh = g_fopen ( full_filepath, mode );
    if ( !file->fh ) {
        fprintf ( stderr, "%s():%d - Cant open: %s, in mode '%s'\n", __func__, __LINE__, full_filepath_locale, mode );
        ui_utils_mem_free ( full_filepath );
        ui_utils_mem_free ( full_filepath_locale );
        ui_utils_mem_free ( naked_filepath );
        return FR_DISK_ERR;
    };

    ui_utils_mem_free ( full_filepath );
    ui_utils_mem_free ( full_filepath_locale );
    file->filepath = naked_filepath;

    return FR_OK;
}


FRESULT unicard_file_read ( st_UNICARD_FILE *file, uint8_t *buff, uint32_t buff_size, uint32_t *read_len ) {
    int ret = fs_layer_fread ( &file->fh, buff, buff_size, read_len );
    if ( ( ret == FS_LAYER_FR_OK ) || ( feof ( file->fh ) ) ) return FR_OK;
    return FR_DISK_ERR;
}


int unicard_file_is_open ( st_UNICARD_FILE *file ) {
    if ( file->fh == NULL ) return EXIT_FAILURE;
    return EXIT_SUCCESS;
}


int unicard_file_is_eof ( st_UNICARD_FILE *file ) {
    if ( file->fh == NULL ) return EXIT_FAILURE;
    if ( feof ( file->fh ) == 0 ) return EXIT_FAILURE;
    return EXIT_SUCCESS;
}


void unicard_fdc_mount ( uint8_t drive_id, char *filepath ) {
    //printf ( "unicard_fdc_mount (FD%d): '%s'\n", drive_id, filepath );

    if ( filepath[0] != 0x00 ) {
        char *naked_filepath = unicard_create_naked_filepath ( filepath );
        char *full_filepath = g_build_filename ( unicard_get_sd_root_dirpath ( ), naked_filepath, NULL );

        char *full_filepath_locale = ui_utils_file_name_locale_from_utf8 ( full_filepath );

        if ( !g_file_test ( full_filepath, G_FILE_TEST_EXISTS ) ) {
            fprintf ( stderr, "%s():%d - File not exists '%s'\n", __func__, __LINE__, full_filepath_locale );
        } else {
            fdc_mount_dskfile ( drive_id, full_filepath );
        };
        ui_utils_mem_free ( full_filepath );
        ui_utils_mem_free ( full_filepath_locale );
        ui_utils_mem_free ( naked_filepath );
    } else {
        fdc_umount ( drive_id );
    };
}
