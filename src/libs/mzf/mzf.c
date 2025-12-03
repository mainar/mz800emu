/* 
 * File:   mzf.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 30. září 2016, 16:27
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
#include <stdint.h>

#include "mzf.h"
#include "../generic_driver/generic_driver.h"
#include "../endianity/endianity.h"

#define MZF_FIX_FILENAME_TERMINATOR


void mzf_header_items_correction ( st_MZF_HEADER *mzfhdr ) {
    mzfhdr->fsize = endianity_bswap16_LE ( mzfhdr->fsize );
    mzfhdr->fstrt = endianity_bswap16_LE ( mzfhdr->fstrt );
    mzfhdr->fexec = endianity_bswap16_LE ( mzfhdr->fexec );

#ifdef MZF_FIX_FILENAME_TERMINATOR
    mzfhdr->fname.terminator = MZF_FNAME_TERMINATOR;
#endif

}


int mzf_read_header_on_offset ( st_HANDLER *h, uint32_t offset, st_MZF_HEADER *mzfhdr ) {
    if ( EXIT_SUCCESS != generic_driver_read ( h, offset, mzfhdr, sizeof ( st_MZF_HEADER ) ) ) return EXIT_FAILURE;
    mzf_header_items_correction ( mzfhdr );
    return EXIT_SUCCESS;
}


static int mzf_write_word_on_offset ( st_HANDLER *h, uint32_t offset, uint16_t w ) {
    uint8_t buffer = w & 0xff;
    if ( EXIT_SUCCESS != generic_driver_write ( h, offset, &buffer, 1 ) ) return EXIT_FAILURE;
    buffer = w >> 8;
    return generic_driver_write ( h, ( offset + 1 ), &buffer, 1 );
}


int mzf_write_header_on_offset ( st_HANDLER *h, uint32_t offset, st_MZF_HEADER *mzfhdr ) {

#ifdef MZF_FIX_FILENAME_TERMINATOR
    mzfhdr->fname.terminator = MZF_FNAME_TERMINATOR;
#endif

    if ( EXIT_SUCCESS != generic_driver_write ( h, offset, mzfhdr, sizeof (mzfhdr->ftype ) + MZF_FNAME_FULL_LENGTH ) ) return EXIT_FAILURE;
    offset += sizeof (mzfhdr->ftype ) + MZF_FNAME_FULL_LENGTH;

    if ( EXIT_SUCCESS != mzf_write_word_on_offset ( h, offset, mzfhdr->fsize ) ) return EXIT_FAILURE;
    offset += sizeof (mzfhdr->fsize );

    if ( EXIT_SUCCESS != mzf_write_word_on_offset ( h, offset, mzfhdr->fstrt ) ) return EXIT_FAILURE;
    offset += sizeof (mzfhdr->fstrt );

    if ( EXIT_SUCCESS != mzf_write_word_on_offset ( h, offset, mzfhdr->fexec ) ) return EXIT_FAILURE;
    offset += sizeof (mzfhdr->fexec );

    if ( EXIT_SUCCESS != generic_driver_write ( h, offset, &mzfhdr->cmnt, MZF_CMNT_LENGTH ) ) return EXIT_FAILURE;
    return EXIT_SUCCESS;
}


int mzf_read_header ( st_HANDLER *h, st_MZF_HEADER *mzfhdr ) {
    return mzf_read_header_on_offset ( h, 0, mzfhdr );
}


int mzf_write_header ( st_HANDLER *h, st_MZF_HEADER *mzfhdr ) {
    return mzf_write_header_on_offset ( h, 0, mzfhdr );
}


int mzf_read_body_on_offset ( st_HANDLER *h, uint32_t offset, uint8_t *buffer, uint32_t buffer_size ) {
    return generic_driver_read ( h, offset, buffer, buffer_size );
}


int mzf_read_body ( st_HANDLER *h, uint8_t *buffer, uint32_t buffer_size ) {
    return mzf_read_body_on_offset ( h, sizeof ( st_MZF_HEADER ), buffer, buffer_size );
}


int mzf_write_body_on_offset ( st_HANDLER *h, uint32_t offset, uint8_t *buffer, uint32_t buffer_size ) {
    return generic_driver_write ( h, offset, buffer, buffer_size );
}


int mzf_write_body ( st_HANDLER *h, uint8_t *buffer, uint32_t buffer_size ) {
    return mzf_write_body_on_offset ( h, sizeof ( st_MZF_HEADER ), buffer, buffer_size );
}


const char* mzf_error_message ( st_HANDLER *h, st_DRIVER *d ) {
    return generic_driver_error_message ( h, d );
}


int mzf_header_test_fname_terminator_on_offset ( st_HANDLER *h, uint32_t offset ) {
    st_MZF_FILENAME mzffname;
    uint8_t *d = (uint8_t*) &mzffname;
    if ( EXIT_SUCCESS != generic_driver_read ( h, offset, d, sizeof ( mzffname ) ) ) return EXIT_FAILURE;
    int i;
    for ( i = 0; i < sizeof ( mzffname ); i++ ) {
        if ( *d == MZF_FNAME_TERMINATOR ) {
            return EXIT_SUCCESS;
        };
        d++;
    };
    return EXIT_FAILURE;
}

int mzf_header_test_fname_terminator ( st_HANDLER *h ) {
    return mzf_header_test_fname_terminator_on_offset ( h, 1 );
}
