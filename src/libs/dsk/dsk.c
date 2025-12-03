/* 
 * File:   dsk.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 7. září 2016, 12:27
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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "dsk.h"
#include "../generic_driver/generic_driver.h"


const char* dsk_error_message ( st_HANDLER *h, st_DRIVER *d ) {

    if ( d->err != GENERIC_DRIVER_ERROR_NONE ) return generic_driver_error_message ( h, d );

    char *unknown_err_msg = "unknown error";

    const char *dsk_err_msg[] = {
                                 "no error",
                                 "image not ready",
                                 "image is write protected",
                                 "track not found",
                                 "sector not found",
                                 "not available on double-sided disk",
                                 "no tracks"
    };

    if ( (en_DSK_ERROR) h->err >= DSK_ERROR_UNKNOWN ) return unknown_err_msg;

    return dsk_err_msg[h->err];
}


/**
 * Vypocet offsetu pro absolutni stopu.
 *  
 * @param abstrack
 * @param tsizes
 * @return offset
 */
uint32_t dsk_compute_track_offset ( uint8_t abstrack, uint8_t *tsizes ) {
    uint32_t offset = sizeof ( st_DSK_HEADER );
    int i;
    for ( i = 0; i < abstrack; i++ ) {
        offset += dsk_decode_track_size ( tsizes[i] );
    };
    return offset;
}


/**
 * Vypocet offsetu sectoru v ramci stopy.
 * 
 * @param sector
 * @param tinfo
 * @return offset, nebo -1 = nenalezeno
 */
int32_t dsk_compute_sector_offset ( uint8_t sector, st_DSK_SHORT_TRACK_INFO *tinfo ) {
    uint32_t offset = sizeof ( st_DSK_TRACK_INFO );
    uint32_t ssize_bytes = dsk_decode_sector_size ( tinfo->ssize );
    int i;
    for ( i = 0; i < tinfo->sectors; i++ ) {
        if ( tinfo->sinfo[i] == sector ) {
            return offset;
        };
        offset += ssize_bytes;
    };
    return -1;
}


/**
 * Precte image info a ulozi jej do short_image_info struktury.
 * 
 * @param handler
 * @param short_image_info - vysledek snazeni
 * @return EXIT_FAILURE | EXIT_SUCCESS
 */
int dsk_read_short_image_info ( st_HANDLER *h, st_DSK_SHORT_IMAGE_INFO *short_image_info ) {

    st_DRIVER *d = (st_DRIVER *) h->driver;

    h->err = HANDLER_ERROR_NONE;
    d->err = GENERIC_DRIVER_ERROR_NONE;

    uint8_t tmpbuffer [ DSK_MAX_TOTAL_TRACKS ];
    uint8_t *buffer = NULL;

    uint32_t offset = DSK_FILEINFO_FIELD_LENGTH + DSK_CREATOR_FIELD_LENGTH;

    if ( EXIT_SUCCESS != generic_driver_prepare ( h, offset, (void**) &buffer, &tmpbuffer, 2 ) ) return EXIT_FAILURE;
    if ( EXIT_SUCCESS != generic_driver_read ( h, offset, buffer, 2 ) ) return EXIT_FAILURE;

    short_image_info->sides = ( buffer[1] <= 1 ) ? 1 : 2;
    short_image_info->tracks = buffer[0];

    uint8_t total_tracks = short_image_info->tracks * short_image_info->sides;

    if ( total_tracks > DSK_MAX_TOTAL_TRACKS ) {
        total_tracks = DSK_MAX_TOTAL_TRACKS;
        short_image_info->tracks = DSK_MAX_TOTAL_TRACKS / short_image_info->sides;
    };

    offset += 4;

    if ( EXIT_SUCCESS != generic_driver_prepare ( h, offset, (void**) &buffer, &tmpbuffer, total_tracks ) ) return EXIT_FAILURE;
    if ( EXIT_SUCCESS != generic_driver_read ( h, offset, buffer, total_tracks ) ) return EXIT_FAILURE;

    memcpy ( short_image_info->tsize, buffer, total_tracks );

    return EXIT_SUCCESS;
}


/**
 * Precteme informaci o velikosti a rozlozeni sectoru na stope, kterou jsme urcili offsetem.
 * 
 * @param handler
 * @param track_offset
 * @param short_track_info - vysledek snazeni
 * @return EXIT_FAILURE | EXIT_SUCCESS
 */
int dsk_read_short_track_info_on_offset ( st_HANDLER *h, uint32_t track_offset, st_DSK_SHORT_TRACK_INFO *short_track_info ) {

    st_DRIVER *d = (st_DRIVER *) h->driver;

    h->err = HANDLER_ERROR_NONE;
    d->err = GENERIC_DRIVER_ERROR_NONE;

    uint8_t tmpbuffer [ 6 ];
    uint8_t *buffer = NULL;

    uint32_t offset = track_offset + DSK_TRACKINFO_FIELD_LENGTH + 4;

    if ( EXIT_SUCCESS != generic_driver_prepare ( h, offset, (void**) &buffer, &tmpbuffer, 6 ) ) return EXIT_FAILURE;
    if ( EXIT_SUCCESS != generic_driver_read ( h, offset, buffer, 6 ) ) return EXIT_FAILURE;

    short_track_info->track = buffer[0];
    short_track_info->side = buffer[1];
    short_track_info->ssize = buffer[4];
    short_track_info->sectors = buffer[5];

    offset += 8;

    int i;
    for ( i = 0; i < short_track_info->sectors; i++ ) {
        st_DSK_SECTOR_INFO tmpbuffer;
        st_DSK_SECTOR_INFO *buffer = NULL;
        if ( EXIT_SUCCESS != generic_driver_prepare ( h, offset, (void**) &buffer, &tmpbuffer, sizeof ( st_DSK_SECTOR_INFO ) ) ) return EXIT_FAILURE;
        if ( EXIT_SUCCESS != generic_driver_read ( h, offset, buffer, sizeof ( st_DSK_SECTOR_INFO ) ) ) return EXIT_FAILURE;
        offset += sizeof ( st_DSK_SECTOR_INFO );
        short_track_info->sinfo[i] = buffer->sector;
    };

    return EXIT_SUCCESS;
}


/**
 * Precteme informaci o velikosti a rozlozeni sectoru na absolutni stope.
 * 
 * @param handler
 * @param short_image_info - pokud je NULL, tak si jej nejprve nacteme
 * @param abstrack
 * @param short_track_info - vysledek snazeni
 * @return EXIT_FAILURE | EXIT_SUCCESS
 */
int dsk_read_short_track_info ( st_HANDLER *h, st_DSK_SHORT_IMAGE_INFO *short_image_info, uint8_t abstrack, st_DSK_SHORT_TRACK_INFO *short_track_info ) {

    st_DRIVER *d = (st_DRIVER *) h->driver;

    h->err = HANDLER_ERROR_NONE;
    d->err = GENERIC_DRIVER_ERROR_NONE;

    uint32_t track_offset = sizeof ( st_DSK_HEADER );

    if ( abstrack != 0 ) {
        st_DSK_SHORT_IMAGE_INFO local_short_image_info;
        st_DSK_SHORT_IMAGE_INFO *iinfo = short_image_info;

        if ( iinfo == NULL ) {
            if ( EXIT_SUCCESS != dsk_read_short_image_info ( h, &local_short_image_info ) ) return EXIT_FAILURE;
            iinfo = &local_short_image_info;
        };

        if ( abstrack >= ( iinfo->tracks * iinfo->sides ) ) {
            h->err = DSK_ERROR_TRACK_NOT_FOUND;
            return EXIT_FAILURE;
        };

        track_offset = dsk_compute_track_offset ( abstrack, iinfo->tsize );
    };

    if ( EXIT_SUCCESS != dsk_read_short_track_info_on_offset ( h, track_offset, short_track_info ) ) return EXIT_FAILURE;

    return EXIT_SUCCESS;
}


/**
 * Pro pozadovany sector na konkretni stope ziskame offset a velikost v bajtech.
 * 
 * @param handler
 * @param short_image_info - muze byt NULL, pak si jej nacteme sami
 * @param short_track_info - muze byt NULL, pak si jej nacteme sami
 * @param abstrack
 * @param sector
 * @param sector_offset - absolutni pozice sektoru
 * @param ssize_bytes - velikost sektoru v bajtech
 * @return EXIT_FAILURE | EXIT_SUCCESS
 */
int dsk_read_short_sector_info ( st_HANDLER *h, st_DSK_SHORT_IMAGE_INFO *short_image_info, st_DSK_SHORT_TRACK_INFO *short_track_info, uint8_t abstrack, uint8_t sector, uint32_t *sector_offset, uint16_t *ssize_bytes ) {

    st_DRIVER *d = (st_DRIVER *) h->driver;

    h->err = HANDLER_ERROR_NONE;
    d->err = GENERIC_DRIVER_ERROR_NONE;

    *sector_offset = 0;
    *ssize_bytes = 0;

    st_DSK_SHORT_IMAGE_INFO local_short_image_info;
    st_DSK_SHORT_IMAGE_INFO *iinfo = short_image_info;

    if ( iinfo == NULL ) {
        if ( EXIT_SUCCESS != dsk_read_short_image_info ( h, &local_short_image_info ) ) return EXIT_FAILURE;
        iinfo = &local_short_image_info;
    };

    if ( ( abstrack >= ( iinfo->tracks * iinfo->sides ) ) || ( iinfo->tsize[abstrack] == 0 ) ) {
        h->err = DSK_ERROR_TRACK_NOT_FOUND;
        return EXIT_FAILURE;
    };

    uint32_t track_offset = dsk_compute_track_offset ( abstrack, iinfo->tsize );

    st_DSK_SHORT_TRACK_INFO local_short_track_info;
    st_DSK_SHORT_TRACK_INFO *tinfo = short_track_info;

    if ( tinfo == NULL ) {
        if ( EXIT_SUCCESS != dsk_read_short_track_info_on_offset ( h, track_offset, &local_short_track_info ) ) return EXIT_FAILURE;
        tinfo = &local_short_track_info;
    };

    int32_t sector_on_track_offset = dsk_compute_sector_offset ( sector, tinfo );

    if ( sector_on_track_offset == -1 ) {
        h->err = DSK_ERROR_SECTOR_NOT_FOUND;
        return EXIT_FAILURE;
    };

    *sector_offset = track_offset + sector_on_track_offset;
    *ssize_bytes = dsk_decode_sector_size ( tinfo->ssize );

    return EXIT_SUCCESS;
}


/**
 * Provede operaci cteni, nebo zapisu na konkretnim absolutnim sektoru.
 * 
 * @param handler
 * @param rwop  - typ operace 0 READ, 1 WRITE
 * @param short_image_info
 * @param short_track_info
 * @param abstrack
 * @param sector
 * @param buffer
 * @return EXIT_FAILURE | EXIT_SUCCESS
 */
int dsk_rw_sector ( st_HANDLER *h, en_DSK_RWOP rwop, st_DSK_SHORT_IMAGE_INFO *short_image_info, st_DSK_SHORT_TRACK_INFO *short_track_info, uint8_t abstrack, uint8_t sector, void *buffer ) {

    uint32_t sector_offset = 0;
    uint16_t ssize_bytes = 0;

    if ( EXIT_SUCCESS != dsk_read_short_sector_info ( h, short_image_info, short_track_info, abstrack, sector, &sector_offset, &ssize_bytes ) ) return EXIT_FAILURE;

    if ( DSK_RWOP_READ == rwop ) return generic_driver_read ( h, sector_offset, buffer, ssize_bytes );
    return generic_driver_write ( h, sector_offset, buffer, ssize_bytes );
}


int dsk_read_on_offset ( st_HANDLER *h, uint32_t offset, void *buffer, uint16_t buffer_size ) {
    return generic_driver_read ( h, offset, buffer, buffer_size );
}


int dsk_write_on_offset ( st_HANDLER *h, uint32_t offset, void *buffer, uint16_t buffer_size ) {
    return generic_driver_write ( h, offset, buffer, buffer_size );
}


int dsk_read_sector ( st_HANDLER *h, uint8_t abstrack, uint8_t sector, void *buffer ) {
    return dsk_rw_sector ( h, DSK_RWOP_READ, NULL, NULL, abstrack, sector, buffer );
}


int dsk_write_sector ( st_HANDLER *h, uint8_t abstrack, uint8_t sector, void *buffer ) {
    return dsk_rw_sector ( h, DSK_RWOP_WRITE, NULL, NULL, abstrack, sector, buffer );
}
