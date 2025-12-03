/* 
 * File:   dsk_tools.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 9. září 2016, 18:57
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
#include "dsk_tools.h"
#include "../generic_driver/generic_driver.h"


#define DSK_TOOLS_MIN_SECTOR_SIZE   128


/**
 * Prirazeni zaznamu v existujici strukture st_DSK_DESCRIPTION.
 * 
 * Zaznamy musi byt ulozeny vzestupne podle total track.
 * 
 * @param desc Odkaz na existujici strukturu.
 * @param rule Poradove cislo zaznamu
 * @param absolute_track Absolutni stopa od ktere pravidlo plati
 * @param sectors
 * @param ssize
 * @param sector_order
 * @param sector_map
 * @param default_value
 */
void dsk_tools_assign_description ( st_DSK_DESCRIPTION *desc, uint8_t rule, uint8_t absolute_track, uint8_t sectors, en_DSK_SECTOR_SIZE ssize, en_DSK_SECTOR_ORDER_TYPE sector_order, uint8_t *sector_map, uint8_t default_value ) {
    st_DSK_DESCRIPTION_RULE *rules = ( st_DSK_DESCRIPTION_RULE* ) & desc->rules;
    rules[rule].absolute_track = absolute_track;
    rules[rule].sectors = sectors;
    rules[rule].ssize = ssize;
    rules[rule].sector_order = sector_order;
    rules[rule].sector_map = ( sector_order == DSK_SEC_ORDER_CUSTOM ) ? sector_map : NULL;
    rules[rule].filler = default_value;
}


static int dsk_tools_create_tsizes ( uint8_t *tsize, st_DSK_DESCRIPTION *desc, uint8_t first_abs_track ) {

    st_DSK_DESCRIPTION_RULE *rules = ( st_DSK_DESCRIPTION_RULE* ) & desc->rules;

    if ( first_abs_track < rules[0].absolute_track ) {
        return EXIT_FAILURE;
    };

    uint8_t sectors = rules[0].sectors;
    en_DSK_SECTOR_SIZE ssize = rules[0].ssize;
    uint8_t rule = 0;

    uint8_t abs_track = first_abs_track;
    uint8_t track;
    for ( track = ( first_abs_track / desc->sides ); track < desc->tracks; track++ ) {
        uint8_t side;
        for ( side = 0; side < desc->sides; side++ ) {
            if ( rule < desc->count_rules ) {
                if ( rules[rule].absolute_track == abs_track ) {
                    sectors = rules[rule].sectors;
                    ssize = rules[rule].ssize;
                    rule++;
                };
            };
            tsize[abs_track] = dsk_encode_track_size ( sectors, ssize );
            if ( ( ssize == DSK_SECTOR_SIZE_128 ) && ( sectors & 1 ) ) {
                tsize[abs_track] += 1;
            };
            abs_track++;
        };
    };

    return EXIT_SUCCESS;
}


/**
 * Vytvori DSK header podle description.
 * 
 * @param handler
 * @param desc
 * @return EXIT_FAILURE | EXIT_SUCCESS
 */
int dsk_tools_create_image_header ( st_HANDLER *h, st_DSK_DESCRIPTION *desc ) {

    if ( ( !desc->count_rules ) || ( !desc->tracks ) ) {
        h->err = DSK_ERROR_NO_TRACKS;
        return EXIT_FAILURE;
    };

    st_DSK_HEADER dskhdr_buffer;
    st_DSK_HEADER *dskhdr = NULL;

    if ( EXIT_SUCCESS != generic_driver_prepare ( h, 0, (void*) &dskhdr, &dskhdr_buffer, sizeof ( st_DSK_HEADER ) ) ) return EXIT_FAILURE;

    memset ( dskhdr, 0x00, sizeof ( st_DSK_HEADER ) );
    memcpy ( dskhdr->file_info, DSK_DEFAULT_FILEINFO, DSK_FILEINFO_FIELD_LENGTH );
    memcpy ( dskhdr->creator, DSK_DEFAULT_CREATOR, DSK_CREATOR_FIELD_LENGTH );

    dskhdr->tracks = desc->tracks;
    dskhdr->sides = desc->sides;

    if ( EXIT_SUCCESS != dsk_tools_create_tsizes ( dskhdr->tsize, desc, 0 ) ) return EXIT_FAILURE;

    return generic_driver_ppwrite ( h, 0, dskhdr, sizeof ( st_DSK_HEADER ) );
}


/**
 * Vytvoreni mapy sektoru, podle definice sector order.
 * 
 * @param sectors
 * @param sector_order - typ CUSTOM je automaticky preveden na NORMAL
 * @param sector_map - navratove pole o velikosti sectors, ktere bude obsahovat vsechny sector ID v pozadovanem poradi
 */
void dsk_tools_make_sector_map ( uint8_t sectors, en_DSK_SECTOR_ORDER_TYPE sector_order, uint8_t *sector_map ) {

    if ( sectors == 0 ) return;

    uint8_t sectors_to_order = ( sectors & 1 ) ? ( sectors + 1 ) : sectors;
    uint8_t sector_pos = 0;
    uint8_t i;

    if ( ( sector_order == DSK_SEC_ORDER_CUSTOM ) || ( sector_order > DSK_SEC_ORDER_INTERLACED_LEC_HD ) ) {
        sector_order = DSK_SEC_ORDER_NORMAL;
    };

    for ( i = 0; i < sectors; i += sector_order ) {
        uint8_t j = 0;
        while ( ( j < sector_order ) && ( ( i + j ) < sectors ) ) {
            uint8_t sector_id = 1 + ( i / sector_order ) + ( sectors_to_order / sector_order ) * ( j % sector_order );
            j++;
            sector_map[sector_pos] = sector_id;
            sector_pos++;
        };
    };
}


/**
 * Vytvoreni hlavicky pro stopu.
 * 
 * @param handler
 * @param dsk_offset
 * @param track
 * @param side
 * @param sectors
 * @param ssize
 * @param sector_map - seznam ID jednotlivych sektoru tak, jak jdou po sobe
 * @return EXIT_FAILURE | EXIT_SUCCESS
 */
int dsk_tools_create_track_header ( st_HANDLER *h, uint32_t dsk_offset, uint8_t track, uint8_t side, uint8_t sectors, en_DSK_SECTOR_SIZE ssize, uint8_t *sector_map ) {

    st_DSK_TRACK_INFO trkhdr_buffer;
    st_DSK_TRACK_INFO *trkhdr = NULL;

    if ( EXIT_SUCCESS != generic_driver_prepare ( h, dsk_offset, (void*) &trkhdr, &trkhdr_buffer, sizeof ( st_DSK_TRACK_INFO ) ) ) return EXIT_FAILURE;

    memset ( trkhdr, 0x00, sizeof ( st_DSK_TRACK_INFO ) );
    memcpy ( trkhdr->track_info, DSK_DEFAULT_TRACKINFO, DSK_TRACKINFO_FIELD_LENGTH );
    trkhdr->track = track;
    trkhdr->side = side;
    trkhdr->sectors = sectors;
    trkhdr->ssize = ssize;

    int i;
    for ( i = 0; i < sectors; i++ ) {
        trkhdr->sinfo[i].track = track;
        trkhdr->sinfo[i].side = side;
        trkhdr->sinfo[i].sector = sector_map[i];
        trkhdr->sinfo[i].ssize = ssize;
    };

    return generic_driver_ppwrite ( h, dsk_offset, trkhdr, sizeof ( st_DSK_TRACK_INFO ) );
}


/**
 * Vyplni vsechny sectory na stope defaultni hodnotou.
 * 
 * @param handler
 * @param dsk_offset
 * @param sectors
 * @param ssize
 * @param default_value
 * @param sectors_total_bytes
 * @return EXIT_FAILURE | EXIT_SUCCESS
 */
int dsk_tools_create_track_sectors ( st_HANDLER *h, uint32_t dsk_offset, uint8_t sectors, en_DSK_SECTOR_SIZE ssize, uint8_t default_value, uint16_t *sectors_total_bytes ) {

    *sectors_total_bytes = 0;

    int my_ssize = dsk_decode_sector_size ( ssize ) / DSK_TOOLS_MIN_SECTOR_SIZE;

    uint8_t i;
    for ( i = 0; i < sectors; i++ ) {
        int j;
        for ( j = 0; j < my_ssize; j++ ) {

            uint8_t data_buffer [ DSK_TOOLS_MIN_SECTOR_SIZE ];
            uint8_t *sector_data = NULL;

            if ( EXIT_SUCCESS != generic_driver_prepare ( h, dsk_offset, (void*) &sector_data, &data_buffer, DSK_TOOLS_MIN_SECTOR_SIZE ) ) return EXIT_FAILURE;

            memset ( sector_data, default_value, DSK_TOOLS_MIN_SECTOR_SIZE );

            generic_driver_ppwrite ( h, dsk_offset, sector_data, DSK_TOOLS_MIN_SECTOR_SIZE );

            *sectors_total_bytes += DSK_TOOLS_MIN_SECTOR_SIZE;
            dsk_offset += DSK_TOOLS_MIN_SECTOR_SIZE;
        };
    };

    return EXIT_SUCCESS;
}


/**
 * Vytvoreni jedne DSK stopy.
 * 
 * @param handler
 * @param dsk_offset
 * @param track
 * @param side
 * @param sectors
 * @param ssize
 * @param sector_map - seznam ID jednotlivych sektoru tak, jak jdou po sobe
 * @param default_value
 * @param track_total_bytes Obsahuje celkovou velikost zapsane stopy
 * @return EXIT_FAILURE | EXIT_SUCCESS
 */
int dsk_tools_create_track ( st_HANDLER *h, uint32_t dsk_offset, uint8_t track, uint8_t side, uint8_t sectors, en_DSK_SECTOR_SIZE ssize, uint8_t *sector_map, uint8_t default_value, uint32_t *track_total_bytes ) {

    *track_total_bytes = 0;

    if ( sectors != 0 ) {
        if ( EXIT_SUCCESS != dsk_tools_create_track_header ( h, dsk_offset, track, side, sectors, ssize, sector_map ) ) return EXIT_FAILURE;

        *track_total_bytes += sizeof ( st_DSK_TRACK_INFO );
        dsk_offset += sizeof ( st_DSK_TRACK_INFO );

        uint16_t sectors_total_size = 0;
        if ( EXIT_SUCCESS != dsk_tools_create_track_sectors ( h, dsk_offset, sectors, ssize, default_value, &sectors_total_size ) ) return EXIT_FAILURE;

        if ( ( ssize == DSK_SECTOR_SIZE_128 ) && ( sectors & 1 ) ) {

            uint8_t data_buffer [ DSK_TOOLS_MIN_SECTOR_SIZE ];
            uint8_t *sector_data = NULL;

            uint32_t zero_filling_offset = dsk_offset + sectors_total_size;

            if ( EXIT_SUCCESS != generic_driver_prepare ( h, zero_filling_offset, (void*) &sector_data, &data_buffer, DSK_TOOLS_MIN_SECTOR_SIZE ) ) return EXIT_FAILURE;

            memset ( sector_data, 0x00, DSK_TOOLS_MIN_SECTOR_SIZE );

            generic_driver_ppwrite ( h, zero_filling_offset, sector_data, DSK_TOOLS_MIN_SECTOR_SIZE );

            sectors_total_size += DSK_TOOLS_MIN_SECTOR_SIZE;
        };

        *track_total_bytes += sectors_total_size;
    };

    return EXIT_SUCCESS;
}


/**
 * Vytvori postupne vsechny stopy podle description.
 * 
 * @param handler
 * @param desc
 * @param first_track
 * @param dsk_offset / 0 = sizeof ( st_DSK_HEADER )
 * @return EXIT_FAILURE | EXIT_SUCCESS
 */
int dsk_tools_create_image_tracks ( st_HANDLER *h, st_DSK_DESCRIPTION *desc, uint8_t first_abs_track, uint32_t dsk_offset ) {

    if ( dsk_offset == 0 ) dsk_offset = sizeof ( st_DSK_HEADER );

    st_DSK_DESCRIPTION_RULE *rules = ( st_DSK_DESCRIPTION_RULE* ) & desc->rules;

    if ( first_abs_track < rules[0].absolute_track ) {
        return EXIT_FAILURE;
    };

    uint8_t sectors = rules[0].sectors;
    en_DSK_SECTOR_SIZE ssize = rules[0].ssize;
    en_DSK_SECTOR_ORDER_TYPE sector_order = rules[0].sector_order;
    uint8_t default_value = rules[0].filler;
    uint8_t rule = 0;

    uint8_t abs_track = first_abs_track;
    uint8_t track;

    en_DSK_SECTOR_ORDER_TYPE last_sector_order = sector_order;
    uint8_t local_sector_map [ DSK_MAX_SECTORS ];
    uint8_t *sector_map;

    if ( ( sector_order == DSK_SEC_ORDER_CUSTOM ) && ( rules[0].sector_map != NULL ) ) {
        sector_map = rules[0].sector_map;
    } else {
        dsk_tools_make_sector_map ( sectors, sector_order, local_sector_map );
        sector_map = local_sector_map;
    };

    for ( track = ( first_abs_track / desc->sides ); track < desc->tracks; track++ ) {
        uint8_t side;
        for ( side = 0; side < desc->sides; side++ ) {
            if ( rule < desc->count_rules ) {
                if ( rules[rule].absolute_track == abs_track ) {
                    sectors = rules[rule].sectors;
                    ssize = rules[rule].ssize;
                    sector_order = rules[rule].sector_order;
                    default_value = rules[rule].filler;


                    if ( ( sector_order == DSK_SEC_ORDER_CUSTOM ) && ( rules[rule].sector_map != NULL ) ) {
                        sector_map = rules[rule].sector_map;
                    } else {
                        if ( sector_order != last_sector_order ) {
                            dsk_tools_make_sector_map ( sectors, sector_order, local_sector_map );
                            sector_map = local_sector_map;
                        };
                    };
                    last_sector_order = sector_order;

                    rule++;
                };
            };

            /* vytvoreni stopy */
            uint32_t track_total_bytes = 0;

            if ( EXIT_SUCCESS != dsk_tools_create_track ( h, dsk_offset, track, side, sectors, ssize, sector_map, default_value, &track_total_bytes ) ) return EXIT_FAILURE;
            dsk_offset += track_total_bytes;

            abs_track++;
        };
    };

    return EXIT_SUCCESS;
}


/**
 * Vytvoreni DSK podle popisu v desc.
 * 
 * @param handler
 * @param desc
 * @return EXIT_FAILURE | EXIT_SUCCESS
 */
int dsk_tools_create_image ( st_HANDLER *h, st_DSK_DESCRIPTION *desc ) {
    if ( EXIT_SUCCESS != dsk_tools_create_image_header ( h, desc ) ) return EXIT_FAILURE;
    return dsk_tools_create_image_tracks ( h, desc, 0, 0 );
}


/**
 * Zmena parametru a default obsahu konkretni absolutni stopy.
 * 
 * @param handler
 * @param short_image_info
 * @param abstrack
 * @param sectors
 * @param ssize
 * @param sector_map - seznam ID jednotlivych sektoru tak, jak jdou po sobe
 * @param default_value
 * @return EXIT_FAILURE | EXIT_SUCCESS
 */
int dsk_tools_change_track ( st_HANDLER *h, st_DSK_SHORT_IMAGE_INFO *short_image_info, uint8_t abstrack, uint8_t sectors, en_DSK_SECTOR_SIZE ssize, uint8_t *sector_map, uint8_t default_value ) {

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

    uint32_t track_offset = dsk_compute_track_offset ( abstrack, iinfo->tsize );
    uint16_t track_size = dsk_decode_track_size ( iinfo->tsize[abstrack] );

    uint16_t new_track_size = dsk_decode_track_size ( dsk_encode_track_size ( sectors, ssize ) );

    uint8_t last_track = ( iinfo->tracks * iinfo->sides ) - 1;
    uint32_t last_track_offset = dsk_compute_track_offset ( last_track, iinfo->tsize );
    uint16_t last_track_size = dsk_decode_track_size ( iinfo->tsize[last_track] );

    uint32_t last_image_byte = last_track_offset + last_track_size;
    uint32_t new_last_image_byte = last_image_byte;

    if ( track_size != new_track_size ) {

        uint8_t buffer [ DSK_TOOLS_MIN_SECTOR_SIZE ];

        uint32_t next_track_offset = track_offset + track_size;

        uint32_t src_offset;
        uint32_t dst_offset;
        int32_t step = 0;

        if ( track_size < new_track_size ) {
            uint16_t size_diference = new_track_size - track_size;
            src_offset = last_image_byte - sizeof ( buffer );
            dst_offset = src_offset + size_diference;
            step -= sizeof ( buffer );
            new_last_image_byte += size_diference;
        } else {
            uint16_t size_diference = track_size - new_track_size;
            src_offset = next_track_offset;
            dst_offset = src_offset - size_diference;
            step = sizeof ( buffer );
            new_last_image_byte -= size_diference;
        };

        uint32_t i;
        for ( i = ( last_image_byte - next_track_offset ); i > 0; i -= sizeof ( buffer ) ) {
            if ( EXIT_SUCCESS != dsk_read_on_offset ( h, src_offset, &buffer, sizeof ( buffer ) ) ) return EXIT_FAILURE;
            if ( EXIT_SUCCESS != dsk_write_on_offset ( h, dst_offset, &buffer, sizeof ( buffer ) ) ) return EXIT_FAILURE;
            src_offset += step;
            dst_offset += step;
        };

        if ( track_size > new_track_size ) {
            if ( EXIT_SUCCESS != generic_driver_truncate ( h, new_last_image_byte ) ) return EXIT_FAILURE;
        };

        iinfo->tsize[abstrack] = dsk_encode_track_size ( sectors, ssize );
        uint32_t offset = DSK_FILEINFO_FIELD_LENGTH + DSK_CREATOR_FIELD_LENGTH + 4 + abstrack;
        if ( EXIT_SUCCESS != dsk_write_on_offset ( h, offset, &iinfo->tsize[abstrack], 1 ) ) return EXIT_FAILURE;
    };

    uint8_t side = ( iinfo->sides == 1 ) ? 0 : ( abstrack & 1 );
    uint8_t track = abstrack / iinfo->sides;

    if ( sectors != 0 ) {
        if ( EXIT_SUCCESS != dsk_tools_create_track_header ( h, track_offset, track, side, sectors, ssize, sector_map ) ) return EXIT_FAILURE;

        uint16_t sectors_total_bytes;
        if ( EXIT_SUCCESS != dsk_tools_create_track_sectors ( h, track_offset + sizeof ( st_DSK_TRACK_INFO ), sectors, ssize, default_value, &sectors_total_bytes ) ) return EXIT_FAILURE;
    };

    return EXIT_SUCCESS;
}


int dsk_tools_add_tracks ( st_HANDLER *h, st_DSK_DESCRIPTION *desc ) {

    st_DSK_HEADER dskhdr_buffer;
    st_DSK_HEADER *dskhdr = NULL;

    if ( EXIT_SUCCESS != generic_driver_prepare ( h, 0, (void*) &dskhdr, &dskhdr_buffer, sizeof ( st_DSK_HEADER ) ) ) return EXIT_FAILURE;

    if ( EXIT_SUCCESS != generic_driver_ppread ( h, 0, dskhdr, sizeof ( st_DSK_HEADER ) ) ) return EXIT_FAILURE;

    dskhdr->tracks = desc->tracks;

    st_DSK_DESCRIPTION_RULE *rules = ( st_DSK_DESCRIPTION_RULE* ) & desc->rules;

    uint8_t first_abs_track = rules[0].absolute_track;

    if ( EXIT_SUCCESS != dsk_tools_create_tsizes ( dskhdr->tsize, desc, first_abs_track ) ) return EXIT_FAILURE;

    if ( EXIT_SUCCESS != generic_driver_ppwrite ( h, 0, dskhdr, sizeof ( st_DSK_HEADER ) ) ) return EXIT_FAILURE;

    st_DSK_SHORT_IMAGE_INFO local_short_image_info;
    st_DSK_SHORT_IMAGE_INFO *iinfo;

    if ( EXIT_SUCCESS != dsk_read_short_image_info ( h, &local_short_image_info ) ) return EXIT_FAILURE;
    iinfo = &local_short_image_info;

    uint32_t track_offset = dsk_compute_track_offset ( first_abs_track, iinfo->tsize );

    return dsk_tools_create_image_tracks ( h, desc, first_abs_track, track_offset );
}


int dsk_tools_shrink_image ( st_HANDLER *h, st_DSK_SHORT_IMAGE_INFO *short_image_info, uint8_t total_tracks ) {

    st_DSK_SHORT_IMAGE_INFO local_short_image_info;
    st_DSK_SHORT_IMAGE_INFO *iinfo = short_image_info;
    st_DRIVER *d = h->driver;

    if ( iinfo == NULL ) {
        if ( EXIT_SUCCESS != dsk_read_short_image_info ( h, &local_short_image_info ) ) return EXIT_FAILURE;
        iinfo = &local_short_image_info;
    };

    if ( total_tracks == 0 ) {
        h->err = DSK_ERROR_NO_TRACKS;
        return EXIT_FAILURE;
    };

    if ( total_tracks >= ( iinfo->tracks * iinfo->sides ) ) {
        h->err = DSK_ERROR_TRACK_NOT_FOUND;
        return EXIT_FAILURE;
    };

    if ( ( iinfo->sides == 2 ) && ( total_tracks & 1 ) ) {
        h->err = DSK_ERROR_DOUBLE_SIDED;
        return EXIT_FAILURE;
    };

    uint32_t track_offset = dsk_compute_track_offset ( total_tracks, iinfo->tsize );

    if ( d->truncate_cb == NULL ) {
        d->err = GENERIC_DRIVER_ERROR_CB_NOT_EXIST;
    };

    if ( EXIT_SUCCESS != d->truncate_cb ( h, track_offset ) ) {
        return EXIT_FAILURE;
    }


    st_DSK_HEADER dskhdr_buffer;
    st_DSK_HEADER *dskhdr = NULL;

    if ( EXIT_SUCCESS != generic_driver_prepare ( h, 0, (void*) &dskhdr, &dskhdr_buffer, sizeof ( st_DSK_HEADER ) ) ) return EXIT_FAILURE;

    if ( EXIT_SUCCESS != generic_driver_ppread ( h, 0, dskhdr, sizeof ( st_DSK_HEADER ) ) ) return EXIT_FAILURE;

    dskhdr->tracks = total_tracks / dskhdr->sides;

    memset ( &dskhdr->tsize[total_tracks], 0x00, DSK_MAX_TOTAL_TRACKS - total_tracks );

    return generic_driver_ppwrite ( h, 0, dskhdr, sizeof ( st_DSK_HEADER ) );
}


int dsk_tools_get_dsk_fileinfo ( st_HANDLER *h, uint8_t *dsk_fileinfo_buffer ) {
    uint32_t offset = 0;
    return dsk_read_on_offset ( h, offset, dsk_fileinfo_buffer, DSK_FILEINFO_FIELD_LENGTH );
}


int dsk_tools_check_dsk_fileinfo ( st_HANDLER *h ) {
    uint8_t dsk_fileinfo_buffer[DSK_FILEINFO_FIELD_LENGTH + 1];
    if ( EXIT_FAILURE == dsk_tools_get_dsk_fileinfo ( h, dsk_fileinfo_buffer ) ) return EXIT_FAILURE;
    dsk_fileinfo_buffer[DSK_FILEINFO_FIELD_LENGTH] = 0x00;
    if ( 0 != strcmp ( (char*) dsk_fileinfo_buffer, DSK_DEFAULT_FILEINFO ) ) return EXIT_FAILURE;
    return EXIT_SUCCESS;
}


int dsk_tools_get_dsk_creator ( st_HANDLER *h, uint8_t *dsk_creator_buffer ) {
    uint32_t offset = DSK_FILEINFO_FIELD_LENGTH;
    return dsk_read_on_offset ( h, offset, dsk_creator_buffer, DSK_CREATOR_FIELD_LENGTH );
}


en_DSK_TOOLS_CHCKTRKINFO dsk_tools_check_dsk_trackinfo_on_offset ( st_HANDLER *h, uint32_t offset ) {
    uint8_t dsk_trackinfo_buffer[DSK_TRACKINFO_FIELD_LENGTH + 1];
    if ( EXIT_FAILURE == dsk_read_on_offset ( h, offset, dsk_trackinfo_buffer, DSK_TRACKINFO_FIELD_LENGTH ) ) return DSK_TOOLS_CHCKTRKINFO_READ_ERROR;
    dsk_trackinfo_buffer[DSK_TRACKINFO_FIELD_LENGTH] = 0x00;
    if ( 0 != strcmp ( (char*) dsk_trackinfo_buffer, DSK_DEFAULT_TRACKINFO ) ) return DSK_TOOLS_CHCKTRKINFO_FAILURE;
    return DSK_TOOLS_CHCKTRKINFO_SUCCESS;
}


int dsk_tools_check_dsk_OLD ( st_HANDLER *h, int autofix ) {

    if ( autofix != 0 ) {
        printf ( "Checking DSK format (in autofix mode) ...\n\n" );
    } else {
        printf ( "Checking DSK format ...\n\n" );
    };

    if ( EXIT_FAILURE == dsk_tools_check_dsk_fileinfo ( h ) ) {
        fprintf ( stderr, "%s():%d - DSK file info check failed\n", __func__, __LINE__ );
        return EXIT_FAILURE;
    } else {
        printf ( "DSK fileinfo: OK\n" );
    };

    uint8_t dsk_creator_buffer[DSK_CREATOR_FIELD_LENGTH + 1];
    if ( EXIT_FAILURE == dsk_tools_get_dsk_creator ( h, dsk_creator_buffer ) ) {
        fprintf ( stderr, "%s():%d - can't get DSK creator info\n", __func__, __LINE__ );
        return EXIT_FAILURE;
    } else {
        dsk_creator_buffer[DSK_CREATOR_FIELD_LENGTH] = 0x00;
        uint8_t *c = dsk_creator_buffer;
        printf ( "DSK creator: " );
        while ( *c >= 0x20 ) {
            printf ( "%c", *c );
            c++;
        };
        printf ( "\n\n" );
    };

    st_DSK_SHORT_IMAGE_INFO sh_img_info;
    if ( EXIT_FAILURE == dsk_read_short_image_info ( h, &sh_img_info ) ) {
        fprintf ( stderr, "%s():%d - can't get DSK image info\n", __func__, __LINE__ );
        return EXIT_FAILURE;
    };

    printf ( "Header DSK sides: %d\n", sh_img_info.sides );
    printf ( "Header DSK tracks: %d\n", sh_img_info.tracks );

    printf ( "\nAnalyzing tracks ...\n\n" );

    st_DSK_SHORT_IMAGE_INFO analyzed_sh_img_info;
    memset ( &analyzed_sh_img_info, 0x00, sizeof ( analyzed_sh_img_info ) );

    analyzed_sh_img_info.sides = 1;
    int abs_tracks = 0;
    int last_side = -1;

    uint32_t offset = sizeof ( st_DSK_HEADER );

    while ( EXIT_SUCCESS == dsk_tools_check_dsk_trackinfo_on_offset ( h, offset ) ) {

        st_DSK_SHORT_TRACK_INFO sh_trk_info;
        if ( EXIT_FAILURE == dsk_read_short_track_info_on_offset ( h, offset, &sh_trk_info ) ) {
            fprintf ( stderr, "%s():%d - can't get track info on 0x%08x\n", __func__, __LINE__, offset );
            return EXIT_FAILURE;
        };

        if ( sh_trk_info.side > 1 ) {
            fprintf ( stderr, "%s():%d - bad side '%d' on 0x%08x\n", __func__, __LINE__, sh_trk_info.side, offset );
            return EXIT_FAILURE;
        };

        if ( ( abs_tracks == 0 ) && ( sh_trk_info.side != 0 ) ) {
            fprintf ( stderr, "%s():%d - bad side '%d' on 0x%08x, abstrack: %d\n", __func__, __LINE__, sh_trk_info.side, offset, abs_tracks );
            return EXIT_FAILURE;
        };

        if ( abs_tracks == 1 ) {
            if ( ( sh_trk_info.track == 1 ) && ( sh_trk_info.side == 0 ) ) {
                analyzed_sh_img_info.sides = 1;
            } else if ( ( sh_trk_info.track == 0 ) && ( sh_trk_info.side == 1 ) ) {
                analyzed_sh_img_info.sides = 2;
            } else {
                fprintf ( stderr, "%s():%d - can't identify count sides (track: %d, side: %d) on 0x%08x, abstrack: %d\n", __func__, __LINE__, sh_trk_info.track, sh_trk_info.side, offset, abs_tracks );
                return EXIT_FAILURE;
            };
        } else if ( abs_tracks > 1 ) {
            if ( analyzed_sh_img_info.sides == 1 ) {
                if ( last_side != sh_trk_info.side ) {
                    fprintf ( stderr, "%s():%d - bad side '%d' (expected '%d') on 0x%08x, abstrack: %d\n", __func__, __LINE__, sh_trk_info.side, last_side, offset, abs_tracks );
                    return EXIT_FAILURE;
                };
            } else if ( last_side == sh_trk_info.side ) {
                fprintf ( stderr, "%s():%d - bad side '%d' (expected '%d') on 0x%08x, abstrack: %d\n", __func__, __LINE__, sh_trk_info.side, ( last_side + 1 ) & 0x01, offset, abs_tracks );
                return EXIT_FAILURE;
            };
        };

        last_side = sh_trk_info.side;

        uint8_t expected_track = ( analyzed_sh_img_info.sides == 1 ) ? abs_tracks : ( abs_tracks / 2 );

        if ( sh_trk_info.track != expected_track ) {
            fprintf ( stderr, "%s():%d - bad track '%d' (expected '%d') on 0x%08x, abstrack: %d\n", __func__, __LINE__, sh_trk_info.track, expected_track, offset, abs_tracks );
            return EXIT_FAILURE;
        };

        if ( ( sh_trk_info.sectors < 1 ) || ( sh_trk_info.sectors >= DSK_MAX_SECTORS ) ) {
            fprintf ( stderr, "%s():%d - bad sectors count '%d' on 0x%08x, abstrack: %d\n", __func__, __LINE__, sh_trk_info.sectors, offset, abs_tracks );
            return EXIT_FAILURE;
        };

        if ( sh_trk_info.ssize > DSK_SECTOR_SIZE_1024 ) {
            fprintf ( stderr, "%s():%d - bad ssize '0x%02x' on 0x%08x, abstrack: %d\n", __func__, __LINE__, sh_trk_info.ssize, offset, abs_tracks );
            return EXIT_FAILURE;
        };

        uint16_t sectors_size = dsk_decode_sector_size ( sh_trk_info.ssize ) * sh_trk_info.sectors;

        if ( ( sh_trk_info.ssize == DSK_SECTOR_SIZE_128 ) && ( sh_trk_info.sectors & 1 ) ) {
            sectors_size += 0x80;
        };

        uint8_t sector_buffer[0x80];

        uint16_t read_offset;
        for ( read_offset = 0; read_offset < sectors_size; read_offset += sizeof (sector_buffer ) ) {
            if ( EXIT_FAILURE == dsk_read_on_offset ( h, offset + sizeof (st_DSK_TRACK_INFO ) + read_offset, sector_buffer, sizeof ( sector_buffer ) ) ) {
                fprintf ( stderr, "%s():%d - error when reading 0x%04x from track on 0x%04x, abstrack: %d\n", __func__, __LINE__, (unsigned) ( read_offset + sizeof (st_DSK_TRACK_INFO ) ), (unsigned) offset, abs_tracks );
                return EXIT_FAILURE;
            };
        };

        uint16_t track_size = sectors_size + sizeof (st_DSK_TRACK_INFO );
        analyzed_sh_img_info.tsize[abs_tracks++] = track_size / 0x0100;
        offset += track_size;
    };

    printf ( "Analyzed sides: %d\n", analyzed_sh_img_info.sides );
    printf ( "Analyzed total tracks: %d\n\n", abs_tracks );

    int errors = 0;

    if ( analyzed_sh_img_info.sides != sh_img_info.sides ) {
        printf ( "DSK BUG: Bad sides info in DSK header.\n" );
        errors++;
    };

    if ( abs_tracks != sh_img_info.tracks * analyzed_sh_img_info.sides ) {
        printf ( "DSK BUG: Bad tracks info in DSK header.\n" );
        errors++;
    };

    if ( ( analyzed_sh_img_info.sides == 2 ) && ( abs_tracks & 1 ) ) {
        printf ( "DSK BUG: The disc is double-sided, but the count of tracks is not odd.\n" );
        errors++;
    };

    int tsize_diferences = 0;

    int i;
    for ( i = 0; i < abs_tracks; i++ ) {
        if ( analyzed_sh_img_info.tsize[i] != sh_img_info.tsize[i] ) {
            tsize_diferences++;
        };
    };

    if ( tsize_diferences != 0 ) {
        printf ( "DSK BUG: Some tracks have bad size info in DSK header.\n" );
        errors++;
    };

    if ( errors ) {
        if ( autofix ) {

            if ( ( analyzed_sh_img_info.sides == 2 ) && ( abs_tracks & 1 ) ) {
                /* TODO: pri lichem poctu stop pridat dalsi stopu */
            };

            analyzed_sh_img_info.tracks = abs_tracks / analyzed_sh_img_info.sides;

            st_DSK_HEADER dhdr;

            if ( dsk_read_on_offset ( h, 0, &dhdr, sizeof ( dhdr ) ) ) {
                fprintf ( stderr, "%s():%d - can't get DSK image info\n", __func__, __LINE__ );
                return EXIT_FAILURE;
            };

            //dhdr.tracks = analyzed_sh_img_info.tracks;
            dhdr.sides = analyzed_sh_img_info.sides;
            memcpy ( dhdr.tsize, analyzed_sh_img_info.tsize, sizeof ( dhdr.tsize ) );

            if ( dsk_write_on_offset ( h, 0, &dhdr, sizeof ( dhdr ) ) ) {
                fprintf ( stderr, "%s():%d - can't write DSK image info\n", __func__, __LINE__ );
                return EXIT_FAILURE;
            };

            printf ( "Result: %d error(s) repaired. DSK is OK!\n", errors );

        } else {
            printf ( "Result: this DSK have %d repairable error(s).\n", errors );
        };
    } else {
        printf ( "Result: DSK is OK!\n" );
    }

    return EXIT_SUCCESS;
}


int dsk_tools_check_dsk ( st_HANDLER *h, int print_info, int dsk_autofix ) {

    if ( dsk_autofix != 0 ) {
        printf ( "Checking DSK format (in autofix mode) ... " );
    } else {
        printf ( "Checking DSK format ... " );
    };

    if ( EXIT_FAILURE == dsk_tools_check_dsk_fileinfo ( h ) ) {
        fprintf ( stderr, "%s():%d - DSK file info check failed\n", __func__, __LINE__ );
        return EXIT_FAILURE;
    } else {
        if ( print_info ) printf ( "\n\nDSK fileinfo: OK\n" );
    };

    uint8_t dsk_creator_buffer[DSK_CREATOR_FIELD_LENGTH + 1];
    if ( EXIT_FAILURE == dsk_tools_get_dsk_creator ( h, dsk_creator_buffer ) ) {
        fprintf ( stderr, "%s():%d - can't get DSK creator info\n", __func__, __LINE__ );
        return EXIT_FAILURE;
    } else {
        dsk_creator_buffer[DSK_CREATOR_FIELD_LENGTH] = 0x00;
        uint8_t *c = dsk_creator_buffer;
        if ( print_info ) {
            printf ( "DSK creator: " );
            while ( *c >= 0x20 ) {
                printf ( "%c", *c );
                c++;
            };
            printf ( "\n" );
        };
    };

    st_DSK_SHORT_IMAGE_INFO sh_img_info;
    if ( EXIT_FAILURE == dsk_read_short_image_info ( h, &sh_img_info ) ) {
        fprintf ( stderr, "%s():%d - can't get DSK image info\n", __func__, __LINE__ );
        return EXIT_FAILURE;
    };

    if ( print_info ) {
        printf ( "DSK header sides: %d\n", sh_img_info.sides );
        printf ( "DSK header tracks: %d\n", sh_img_info.tracks );

        printf ( "\nAnalyzing tracks ... " );
    };

    uint8_t tsize [ DSK_MAX_TOTAL_TRACKS ];
    memset ( &tsize, 0x00, sizeof ( tsize ) );

    uint8_t expected_track = 0;
    uint8_t expected_side = 0;
    uint8_t abs_tracks = 0;

    uint32_t offset = sizeof ( st_DSK_HEADER );

    while ( 1 ) {

        en_DSK_TOOLS_CHCKTRKINFO res = dsk_tools_check_dsk_trackinfo_on_offset ( h, offset );

        if ( res == DSK_TOOLS_CHCKTRKINFO_READ_ERROR ) break;

        if ( res == DSK_TOOLS_CHCKTRKINFO_FAILURE ) {
            fprintf ( stderr, "%s():%d - expected track info on 0x%08x, abstrack: %d\n", __func__, __LINE__, offset, abs_tracks );
            return EXIT_FAILURE;
        };

        st_DSK_SHORT_TRACK_INFO sh_trk_info;
        if ( EXIT_FAILURE == dsk_read_short_track_info_on_offset ( h, offset, &sh_trk_info ) ) {
            fprintf ( stderr, "%s():%d - can't get track info on 0x%08x, abstrack: %d\n", __func__, __LINE__, offset, abs_tracks );
            return EXIT_FAILURE;
        };

        if ( sh_trk_info.track != expected_track ) {
            fprintf ( stderr, "%s():%d - bad track '%d' (expected %d) on 0x%08x, abstrack: %d\n", __func__, __LINE__, sh_trk_info.track, expected_track, offset, abs_tracks );
            return EXIT_FAILURE;
        };

        if ( sh_trk_info.side != expected_side ) {
            fprintf ( stderr, "%s():%d - bad side '%d' (expected %d) on 0x%08x, abstrack: %d\n", __func__, __LINE__, sh_trk_info.side, expected_side, offset, abs_tracks );
            return EXIT_FAILURE;
        };

        if ( sh_img_info.sides == 1 ) {
            expected_track++;
        } else {
            expected_side = ( ~expected_side ) & 1;
            if ( sh_trk_info.side == 1 ) {
                expected_track++;
            };
        };

        if ( ( sh_trk_info.sectors < 1 ) || ( sh_trk_info.sectors >= DSK_MAX_SECTORS ) ) {
            fprintf ( stderr, "%s():%d - bad sectors count '%d' on 0x%08x, abstrack: %d\n", __func__, __LINE__, sh_trk_info.sectors, offset, abs_tracks );
            return EXIT_FAILURE;
        };

        if ( sh_trk_info.ssize > DSK_SECTOR_SIZE_1024 ) {
            fprintf ( stderr, "%s():%d - bad ssize '0x%02x' on 0x%08x, abstrack: %d\n", __func__, __LINE__, sh_trk_info.ssize, offset, abs_tracks );
            return EXIT_FAILURE;
        };

        // TODO: kontrola hlavicek sektoru

        uint16_t sectors_size = dsk_decode_sector_size ( sh_trk_info.ssize ) * sh_trk_info.sectors;

        if ( ( sh_trk_info.ssize == DSK_SECTOR_SIZE_128 ) && ( sh_trk_info.sectors & 1 ) ) {
            sectors_size += 0x80;
        };

        uint8_t sector_buffer[0x80];

        uint16_t read_offset;
        for ( read_offset = 0; read_offset < sectors_size; read_offset += sizeof (sector_buffer ) ) {
            if ( EXIT_FAILURE == dsk_read_on_offset ( h, offset + sizeof (st_DSK_TRACK_INFO ) + read_offset, sector_buffer, sizeof ( sector_buffer ) ) ) {
                fprintf ( stderr, "%s():%d - error when reading 0x%04x from track on 0x%04x, abstrack: %d\n", __func__, __LINE__, (unsigned) ( read_offset + sizeof (st_DSK_TRACK_INFO ) ), (unsigned) offset, abs_tracks );
                return EXIT_FAILURE;
            };
        };

        uint16_t track_size = sectors_size + sizeof (st_DSK_TRACK_INFO );
        tsize[abs_tracks++] = track_size / 0x0100;
        offset += track_size;
    };

    if ( print_info ) printf ( "total tracks: %d\n\n", abs_tracks );

    int errors = 0;

    if ( abs_tracks != sh_img_info.tracks * sh_img_info.sides ) {
        printf ( "DSK BUG: Bad tracks info in DSK header.\n" );
        errors++;
    };

    if ( ( sh_img_info.sides == 2 ) && ( abs_tracks & 1 ) ) {
        printf ( "DSK BUG: The disc is double-sided, but the count of tracks is not odd.\n" );
        errors++;
    };

    int tsize_diferences = 0;

    int i;
    for ( i = 0; i < abs_tracks; i++ ) {
        if ( tsize[i] != sh_img_info.tsize[i] ) {
            tsize_diferences++;
        };
    };

    if ( tsize_diferences != 0 ) {
        printf ( "DSK BUG: Some tracks have bad size info in DSK header.\n" );
        errors++;
    };

    if ( errors ) {
        if ( dsk_autofix ) {

            if ( ( sh_img_info.sides == 2 ) && ( abs_tracks & 1 ) ) {
                /* TODO: pri lichem poctu stop pridat dalsi stopu */
                // abs_tracks++;
            };

            st_DSK_HEADER dhdr;

            if ( dsk_read_on_offset ( h, 0, &dhdr, sizeof ( dhdr ) ) ) {
                fprintf ( stderr, "%s():%d - can't get DSK image info\n", __func__, __LINE__ );
                return EXIT_FAILURE;
            };

            dhdr.tracks = abs_tracks / sh_img_info.sides;
            memcpy ( dhdr.tsize, tsize, sizeof ( dhdr.tsize ) );

            if ( dsk_write_on_offset ( h, 0, &dhdr, sizeof ( dhdr ) ) ) {
                fprintf ( stderr, "%s():%d - can't write DSK image info\n", __func__, __LINE__ );
                return EXIT_FAILURE;
            };

            printf ( "Result: %d error(s) repaired. DSK is OK!\n", errors );

        } else {
            printf ( "Result: this DSK have %d repairable error(s).\n", errors );
            return EXIT_FAILURE;
        };
    } else {
        printf ( "Result: DSK is OK!\n" );
    }

    return EXIT_SUCCESS;
}


void dsk_tools_destroy_track_rules ( st_DSK_TOOLS_TRACKS_RULES_INFO * tracks_rules ) {
    if ( ( tracks_rules->count_rules != 0 ) && ( tracks_rules->rule != NULL ) ) {
        free ( tracks_rules->rule );
    };
    free ( tracks_rules );
}


st_DSK_TOOLS_TRACKS_RULES_INFO * dsk_tools_get_tracks_rules ( st_HANDLER * h ) {

    st_DSK_SHORT_IMAGE_INFO sh_img_info;
    if ( EXIT_FAILURE == dsk_read_short_image_info ( h, &sh_img_info ) ) {
        fprintf ( stderr, "%s():%d - can't get DSK image info\n", __func__, __LINE__ );
        return NULL;
    };

    st_DSK_TOOLS_TRACKS_RULES_INFO *tracks_rules = malloc ( sizeof ( st_DSK_TOOLS_TRACKS_RULES_INFO ) );

    tracks_rules->total_tracks = sh_img_info.tracks * sh_img_info.sides;
    tracks_rules->sides = sh_img_info.sides;
    tracks_rules->count_rules = 0;
    tracks_rules->rule = NULL;
    tracks_rules->mzboot_track = 0;

    int last_rule = -1;

    int track;
    for ( track = 0; track < tracks_rules->total_tracks; track++ ) {

        st_DSK_SHORT_TRACK_INFO sh_trk_info;

        if ( EXIT_FAILURE == dsk_read_short_track_info ( h, &sh_img_info, track, &sh_trk_info ) ) {
            fprintf ( stderr, "%s():%d - can't get DSK track info for track %d\n", __func__, __LINE__, track );
            dsk_tools_destroy_track_rules ( tracks_rules );
            return NULL;
        };

        if ( ( tracks_rules->count_rules == 0 ) || ( sh_trk_info.sectors != tracks_rules->rule[last_rule].sectors ) || ( sh_trk_info.ssize != tracks_rules->rule[last_rule].ssize ) ) {
            if ( tracks_rules->rule == NULL ) {
                tracks_rules->rule = malloc ( sizeof ( st_DSK_TOOLS_TRACK_RULE_INFO ) );
            } else {
                tracks_rules->rule = realloc ( tracks_rules->rule, ( tracks_rules->count_rules + 1 ) * sizeof ( st_DSK_TOOLS_TRACK_RULE_INFO ) );
            };
            tracks_rules->rule[tracks_rules->count_rules].from_track = track;
            tracks_rules->rule[tracks_rules->count_rules].count_tracks = 1;
            tracks_rules->rule[tracks_rules->count_rules].sectors = sh_trk_info.sectors;
            tracks_rules->rule[tracks_rules->count_rules].ssize = sh_trk_info.ssize;
            tracks_rules->count_rules++;
            last_rule++;
        } else {
            tracks_rules->rule[last_rule].count_tracks++;
        };

        if ( ( track == 1 ) && ( sh_trk_info.sectors == 16 ) && ( sh_trk_info.ssize == DSK_SECTOR_SIZE_256 ) ) {
            tracks_rules->mzboot_track = 1;
        };
    };

    return tracks_rules;
}


en_DSK_TOOLS_IDENTFORMAT dsk_tools_identformat_from_tracks_rules ( st_DSK_TOOLS_TRACKS_RULES_INFO * tracks_rules ) {

    if ( ( tracks_rules == NULL ) || ( ( tracks_rules->mzboot_track != 1 ) ) ) return DSK_TOOLS_IDENTFORMAT_UNKNOWN;

    if ( ( tracks_rules->count_rules == 1 ) && ( tracks_rules->rule[0].sectors == 16 ) && ( tracks_rules->rule[0].ssize == DSK_SECTOR_SIZE_256 ) ) {
        return DSK_TOOLS_IDENTFORMAT_MZBASIC;
    } else if ( ( tracks_rules->count_rules == 3 ) && ( tracks_rules->rule[0].sectors == tracks_rules->rule[2].sectors ) && ( tracks_rules->rule[0].ssize == tracks_rules->rule[2].ssize ) ) {
        if ( ( tracks_rules->rule[0].sectors == 9 ) && ( tracks_rules->rule[0].ssize == DSK_SECTOR_SIZE_512 ) ) {
            return DSK_TOOLS_IDENTFORMAT_MZCPM;
        } else if ( ( tracks_rules->rule[0].sectors == 18 ) && ( tracks_rules->rule[0].ssize == DSK_SECTOR_SIZE_512 ) ) {
            return DSK_TOOLS_IDENTFORMAT_MZCPMHD;
        };
    };

    return DSK_TOOLS_IDENTFORMAT_MZBOOT;
}


int dsk_tools_identformat ( st_HANDLER *h, en_DSK_TOOLS_IDENTFORMAT * result ) {
    st_DSK_TOOLS_TRACKS_RULES_INFO *tracks_rules = dsk_tools_get_tracks_rules ( h );
    *result = dsk_tools_identformat_from_tracks_rules ( tracks_rules );
    if ( tracks_rules == NULL ) return EXIT_FAILURE;
    dsk_tools_destroy_track_rules ( tracks_rules );
    return EXIT_SUCCESS;
}


st_DSK_TOOLS_TRACK_RULE_INFO* dsk_tools_get_rule_for_track ( st_DSK_TOOLS_TRACKS_RULES_INFO *tracks_rules, uint8_t track ) {
    int i = tracks_rules->count_rules - 1;
    st_DSK_TOOLS_TRACK_RULE_INFO *rule = NULL;
    while ( i >= 0 ) {
        rule = &tracks_rules->rule[i--];
        if ( track >= rule->from_track ) break;
    };
    return rule;
}
