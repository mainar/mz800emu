/* 
 * File:   dsk.h
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 7. září 2016, 10:04
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


#ifndef DSK_H
#define DSK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "../generic_driver/generic_driver.h"


    typedef enum en_DSK_SECTOR_SIZE {
        DSK_SECTOR_SIZE_128 = 0,
        DSK_SECTOR_SIZE_256,
        DSK_SECTOR_SIZE_512,
        DSK_SECTOR_SIZE_1024,
        DSK_SECTOR_SIZE_INVALID = 0xff
    } en_DSK_SECTOR_SIZE;


#define DSK_MAX_TOTAL_TRACKS        204
#define DSK_MAX_SECTORS             29

#define DSK_MAX_SECTOR_SIZE         1024

#define DSK_FILEINFO_FIELD_LENGTH   34
    /* 34 znaku, bez 0x00 ! */
#define DSK_DEFAULT_FILEINFO        "EXTENDED CPC DSK File\r\nDisk-Info\r\n"

#define DSK_CREATOR_FIELD_LENGTH    14
    /* 14 znaku, bez 0x00 */
#define DSK_DEFAULT_CREATOR         "DSKLib v1.0\x0\x0\x0"

#define DSK_TRACKINFO_FIELD_LENGTH  12
    /* 12 znaku, bez 0x00 */
#define DSK_DEFAULT_TRACKINFO       "Track-Info\r\n"

#define DSK_DEFAULT_GAP             0x4e
#define DSK_DEFAULT_FILLER          0xe5


    typedef struct st_DSK_HEADER {
        uint8_t file_info [ DSK_FILEINFO_FIELD_LENGTH ];
        uint8_t creator [ DSK_CREATOR_FIELD_LENGTH ];
        uint8_t tracks;
        uint8_t sides;
        uint8_t unused [ 2 ];
        uint8_t tsize [ DSK_MAX_TOTAL_TRACKS ]; /* Pokud ma stopa tsize = 0, tak se chovame jako kdyby neexistovala */
    } st_DSK_HEADER; // 256 B


    typedef struct st_DSK_SECTOR_INFO {
        uint8_t track;
        uint8_t side;
        uint8_t sector;
        uint8_t ssize;
        uint8_t fdc_sts1;
        uint8_t fdc_sts2;
        uint8_t unused [ 2 ];
    } st_DSK_SECTOR_INFO; // 8 B


    typedef struct st_DSK_TRACK_INFO {
        uint8_t track_info [ DSK_TRACKINFO_FIELD_LENGTH ];
        uint8_t unused1 [ 4 ];
        uint8_t track;
        uint8_t side;
        uint8_t unused2 [ 2 ];
        uint8_t ssize;
        uint8_t sectors;
        uint8_t gap; // GAP#3 length 
        uint8_t filler; // filler byte 
        st_DSK_SECTOR_INFO sinfo [ DSK_MAX_SECTORS ];
    } st_DSK_TRACK_INFO; // 256 B


    static inline uint16_t dsk_decode_sector_size ( en_DSK_SECTOR_SIZE ssize ) {
        uint16_t retval = 0x80;
        return ( retval << ssize );
    }


    static inline uint16_t dsk_decode_track_size ( uint8_t tsize ) {
        return ( (uint16_t) tsize << 8 );
    }


    static inline en_DSK_SECTOR_SIZE dsk_encode_sector_size ( uint16_t size_kb ) {
        switch ( size_kb ) {
            case 1024:
                return DSK_SECTOR_SIZE_1024;
            case 512:
                return DSK_SECTOR_SIZE_512;
            case 256:
                return DSK_SECTOR_SIZE_256;
            case 128:
                return DSK_SECTOR_SIZE_128;
            default:
                /* error */
                break;
        };
        return DSK_SECTOR_SIZE_INVALID;
    }


    static inline uint8_t dsk_encode_track_size ( uint8_t sectors, en_DSK_SECTOR_SIZE ssize ) {
        if ( sectors == 0 ) return 0;
        uint16_t retval = sizeof ( st_DSK_TRACK_INFO ) + ( dsk_decode_sector_size ( ssize ) * sectors );
        return (uint8_t) ( retval >> 8 );
    }


    typedef enum en_DSK_ERROR {
        DSK_ERROR_NONE = HANDLER_ERROR_NONE,
        DSK_ERROR_NOT_READY = HANDLER_ERROR_NOT_READY,
        DSK_ERROR_WRITE_PROTECTED = HANDLER_ERROR_WRITE_PROTECTED,
        DSK_ERROR_TRACK_NOT_FOUND = HANDLER_ERROR_USER,
        DSK_ERROR_SECTOR_NOT_FOUND,
        DSK_ERROR_DOUBLE_SIDED, // pokousime se vyrobit oboustranny disk s lichym poctem stop
        DSK_ERROR_NO_TRACKS, // pokousime se vyrobit disk s nulovym poctem stop
        DSK_ERROR_UNKNOWN
    } en_DSK_ERROR;


    typedef enum en_DSK_STATUS {
        DSK_STATUS_NOT_READY = HANDLER_STATUS_NOT_READY,
        DSK_STATUS_READY = HANDLER_STATUS_READY,
        DSK_STATUS_READ_ONLY = HANDLER_STATUS_READ_ONLY
    } en_DSK_STATUS;


    typedef struct st_DSK_SHORT_IMAGE_INFO {
        uint8_t tracks;
        uint8_t sides;
        uint8_t tsize [ DSK_MAX_TOTAL_TRACKS ];
    } st_DSK_SHORT_IMAGE_INFO;


    typedef struct st_DSK_SHORT_TRACK_INFO {
        uint8_t track;
        uint8_t side;
        uint8_t ssize;
        uint8_t sectors;
        uint8_t sinfo [ DSK_MAX_SECTORS ];
    } st_DSK_SHORT_TRACK_INFO;


    extern const char* dsk_error_message ( st_HANDLER *h, st_DRIVER *d );

    extern uint32_t dsk_compute_track_offset ( uint8_t abstrack, uint8_t *tsizes );
    extern int32_t dsk_compute_sector_offset ( uint8_t sector, st_DSK_SHORT_TRACK_INFO *tinfo );

    extern int dsk_read_short_image_info ( st_HANDLER *h, st_DSK_SHORT_IMAGE_INFO *short_image_info );
    extern int dsk_read_short_track_info_on_offset ( st_HANDLER *h, uint32_t track_offset, st_DSK_SHORT_TRACK_INFO *short_track_info );
    extern int dsk_read_short_track_info ( st_HANDLER *h, st_DSK_SHORT_IMAGE_INFO *short_image_info, uint8_t abstrack, st_DSK_SHORT_TRACK_INFO *short_track_info );
    extern int dsk_read_short_sector_info ( st_HANDLER *h, st_DSK_SHORT_IMAGE_INFO *short_image_info, st_DSK_SHORT_TRACK_INFO *short_track_info, uint8_t abstrack, uint8_t sector, uint32_t *sector_offset, uint16_t *ssize_bytes );


    typedef enum en_DSK_RWOP {
        DSK_RWOP_READ = 0,
        DSK_RWOP_WRITE,
    } en_DSK_RWOP;

    extern int dsk_rw_sector ( st_HANDLER *h, en_DSK_RWOP rwop, st_DSK_SHORT_IMAGE_INFO *short_image_info, st_DSK_SHORT_TRACK_INFO *short_track_info, uint8_t abstrack, uint8_t sector, void *buffer );


    extern int dsk_read_on_offset ( st_HANDLER *h, uint32_t offset, void *buffer, uint16_t buffer_size );
    extern int dsk_write_on_offset ( st_HANDLER *h, uint32_t offset, void *buffer, uint16_t buffer_size );
    extern int dsk_read_sector ( st_HANDLER *h, uint8_t abstrack, uint8_t sector, void *buffer );
    extern int dsk_write_sector ( st_HANDLER *h, uint8_t abstrack, uint8_t sector, void *buffer );

#ifdef __cplusplus
}
#endif

#endif /* DSK_H */

