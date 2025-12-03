/* 
 * File:   cmttool.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 25. září 2018, 10:26
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

#include <glib.h>
#include <string.h>

#include "libs/generic_driver/generic_driver.h"
#include "libs/cmt_stream/cmt_stream.h"
#include "ui/ui_utils.h"
#include "ui/generic_driver/ui_memory_driver.h"

#include "libs/mzf/mzf.h"
#include "libs/mzf/mzf_tools.h"

#include "cmttool_turbo.h"
#include "cmttool_fastipl.h"
#include "cmttool.h"


static st_DRIVER *g_driver = &g_ui_memory_driver_static;

#define CMTTOOL_PILOT_MIN_PULSES    4000

#define CMTTOOL_READ_SYNC    256 /* pocet prvnich pulzu, ktere nechame projit bez povsimnuti */

#define CMTTOOL_SYNCHRO_BLOCK_SIZE 0x0200
#define CMTTOOL_SYMCHRO_SAMPLE_DIVIDER 0x0180

#define CMTTOOL_BSD_FTYPE 0x04
#define CMTTOOL_BSD_BLOCK_SIZE 258
#define CMTTOOL_BSD_LAST_CHUNK_ID 0xffff


static void cmttool_cmtfile_destroy ( st_CMTTOOL_FILE *cf ) {
    if ( !cf ) return;
    if ( cf->hdr ) g_free ( cf->hdr );
    if ( cf->body ) g_free ( cf->body );
    g_free ( cf );
}


static void cmttool_reset_cmtfiles ( st_CMTTOOL *cmttool ) {
    if ( !cmttool ) return;
    int i;
    for ( i = 0; i < cmttool->count_files; i++ ) {
        cmttool_cmtfile_destroy ( cmttool->cf[i] );
    };
    if ( cmttool->cf ) g_free ( cmttool->cf );
    cmttool->count_files = 0;
    cmttool->cf = NULL;
}


void cmttool_destroy ( st_CMTTOOL *cmttool ) {
    if ( !cmttool ) return;
    if ( cmttool->stream ) cmt_stream_destroy ( cmttool->stream );

    int i;
    for ( i = 0; i < cmttool->count_files; i++ ) {
        cmttool_cmtfile_destroy ( cmttool->cf[i] );
    };
    if ( cmttool->cf ) g_free ( cmttool->cf );

    g_free ( cmttool );
}


st_CMTTOOL *cmttool_create_from_wav ( char *filename, en_CMT_STREAM_POLARITY polarity ) {

    st_HANDLER *h = generic_driver_open_memory_from_file ( NULL, g_driver, filename );
    if ( !h ) {
        fprintf ( stderr, "%s():%d - Can't open file '%s'\n", __func__, __LINE__, filename );
        return NULL;
    };

    st_CMTTOOL *cmttool = (st_CMTTOOL*) ui_utils_mem_alloc0 ( sizeof ( st_CMTTOOL ) );
    if ( !cmttool ) {
        fprintf ( stderr, "%s():%d - Can't alocate memory (%d)\n", __func__, __LINE__, (int) sizeof ( st_CMTTOOL ) );
        return NULL;
    };

    cmttool->stream = cmt_stream_new_from_wav ( h, polarity, CMT_STREAM_TYPE_BITSTREAM );
    generic_driver_close ( h );
    if ( !cmttool->stream ) {
        cmttool_destroy ( cmttool );
        return NULL;
    };

    generic_driver_close ( h );

    cmttool->debug = FALSE;
    return cmttool;
}


void cmttool_set_debug ( st_CMTTOOL *cmttool, gboolean debug ) {
    cmttool->debug = debug;
}


static int cmttool_read_pulse ( st_CMTTOOL *cmttool ) {

    en_CMT_STREAM_TYPE strtype = cmt_stream_get_stream_type ( cmttool->stream );

    if ( CMT_STREAM_TYPE_BITSTREAM == strtype ) {
        st_CMT_BITSTREAM *bstr = cmttool->stream->str.bitstream;
        uint32_t i;
        for ( i = cmttool->pos; i < bstr->scans; i++ ) {
            int value = cmt_bitstream_get_value_on_position ( bstr, i );
            if ( value == 0 ) {
                cmttool->loadTm = i - cmttool->pos;
                int value = ( ( !cmttool->sampTm ) || ( cmttool->loadTm < cmttool->sampTm ) ) ? 0 : 1;
                cmttool->bit_value = value;
                cmttool->pos = i;
                for ( cmttool->pos = i; cmttool->pos < bstr->scans; cmttool->pos++ ) {
                    int value = cmt_bitstream_get_value_on_position ( bstr, cmttool->pos );
                    if ( value == 1 ) {
                        //uint32_t length0 = cmttool->pos - i;
                        //uint32_t length1 = cmttool->loadTm;
                        return EXIT_SUCCESS;
                    };
                };
            };
        };
        // dosli jsme na konec streamu a nenasli jsme value = 1 !

    } else if ( CMT_STREAM_TYPE_VSTREAM == strtype ) {
        fprintf ( stderr, "%s():%d - Error: Unsupported stream type 'VSTREAM'\n", __func__, __LINE__ );
    } else {
        fprintf ( stderr, "%s():%d - Error: Unknown stream type '%d'\n", __func__, __LINE__, strtype );
    };

    cmttool->status = CMTTOOL_STATUS_EOF;

    return EXIT_FAILURE;
}


static int cmttool_read_tapemark_block ( st_CMTTOOL *cmttool, int count, int rq_value ) {
    while ( count-- ) {
        if ( EXIT_FAILURE == cmttool_read_pulse ( cmttool ) ) return EXIT_FAILURE;
        if ( cmttool->bit_value != rq_value ) {
            cmttool->status = CMTTOOL_STATUS_ERR_TMARK;
            return EXIT_SUCCESS;
        };
    };
    return EXIT_SUCCESS;
}


/**
 * Precteni symetrickeho tapemarku, ktery zacina blokem "1", nasledovanych stejnym blokem "0".
 * Za tapemarkem musi nasledovat jeden pulz s hodnotou "1".
 * 
 * @param cmttool
 * @param tmark
 * @return 
 */
static int cmttool_read_tapemark ( st_CMTTOOL *cmttool, en_CMTTOOL_TAPEMARK tmark ) {

    // prvni bit z "1" bloku uz byl nacten jinde, takze ho odecteme
    if ( EXIT_FAILURE == cmttool_read_tapemark_block ( cmttool, ( tmark - 1 ), 1 ) ) return EXIT_FAILURE;
    if ( cmttool->status != CMTTOOL_STATUS_OK ) return EXIT_SUCCESS;

    if ( EXIT_FAILURE == cmttool_read_tapemark_block ( cmttool, tmark, 0 ) ) return EXIT_FAILURE;
    if ( cmttool->status != CMTTOOL_STATUS_OK ) return EXIT_SUCCESS;

    if ( EXIT_FAILURE == cmttool_read_pulse ( cmttool ) ) return EXIT_FAILURE;
    if ( cmttool->bit_value != 1 ) {
        cmttool->status = CMTTOOL_STATUS_ERR_TMARK;
        return EXIT_SUCCESS;
    };

    //printf ( "Tapemark - OK: %d\n", cmttool->pos );
    return EXIT_SUCCESS;
}


static int cmttool_read_byte ( st_CMTTOOL *cmttool, uint8_t *byte, uint16_t *checksum ) {

    *byte = 0;

    if ( EXIT_FAILURE == cmttool_read_pulse ( cmttool ) ) return EXIT_FAILURE;

    if ( cmttool->bit_value != 1 ) {
        cmttool->status = CMTTOOL_STATUS_ERR_DATA;
        return EXIT_FAILURE;
    };

    int i;

    for ( i = 0; i < 8; i++ ) {

        if ( EXIT_FAILURE == cmttool_read_pulse ( cmttool ) ) return EXIT_FAILURE;

        if ( ( checksum != NULL ) && ( cmttool->bit_value ) ) *checksum = *checksum + 1;

        *byte <<= 1;
        *byte |= cmttool->bit_value;
    };

    return EXIT_SUCCESS;
}


static uint8_t* cmttool_read_block ( st_CMTTOOL *cmttool, en_CMTTOOL_TAPEMARK tmark, uint16_t size, uint32_t position ) {

    if ( position >= cmt_stream_get_count_scans ( cmttool->stream ) ) {
        cmttool->status = CMTTOOL_STATUS_ERR_POSITION;
        return NULL;
    };
    cmttool->pos = position;

    if ( cmttool->debug ) printf ( "Search block - tapemark: %d, size: 0x%04x, position: %d\n", tmark, size, position );

    cmttool->block_samples = 0;
    cmttool->block_baud_rate = 0;
    cmttool->block_size = size;

    while ( 1 ) {

        cmttool->status = CMTTOOL_STATUS_OK;
        cmttool->shortTm = 0;
        cmttool->sampTm = 0;

        int i;
        for ( i = 0; i < CMTTOOL_READ_SYNC; i++ ) {
            if ( EXIT_FAILURE == cmttool_read_pulse ( cmttool ) ) return NULL;
        };

        while ( cmttool->status == CMTTOOL_STATUS_OK ) {

            uint32_t totalScans = 0;

            int i;
            for ( i = 0; i < CMTTOOL_SYNCHRO_BLOCK_SIZE; i++ ) {
                if ( EXIT_FAILURE == cmttool_read_pulse ( cmttool ) ) return NULL;

                if ( cmttool->bit_value == 1 ) {
                    if ( EXIT_FAILURE == cmttool_read_tapemark ( cmttool, tmark ) ) return NULL;
                    if ( cmttool->status != CMTTOOL_STATUS_OK ) {
                        break;
                    } else {

                        uint32_t block_start_pos = cmttool->pos;
                        uint8_t *data = (uint8_t*) ui_utils_mem_alloc0 ( size );
                        uint16_t checksum = 0;

                        int i = 0;
                        while ( i < size ) {
                            if ( EXIT_FAILURE == cmttool_read_byte ( cmttool, &data[i++], &checksum ) ) {
                                g_free ( data );
                                return NULL;
                            };
                        };

                        uint8_t byte;
                        uint16_t ctrl_checksum = 0;

                        int ret = cmttool_read_byte ( cmttool, &byte, NULL );

                        if ( EXIT_FAILURE != ret ) {
                            ctrl_checksum = byte << 8;
                            ret = cmttool_read_byte ( cmttool, &byte, NULL );
                        };

                        if ( EXIT_FAILURE == ret ) {
                            g_free ( data );
                            return NULL;
                        };

                        ctrl_checksum |= byte;

                        if ( ctrl_checksum != checksum ) {
                            g_free ( data );
                            cmttool->status = CMTTOOL_STATUS_ERR_CHECKSUM;
                            return NULL;
                        };

                        cmttool->block_samples = cmttool->pos - block_start_pos;

                        uint32_t count_symbols = ( size + 2 ) * 9;
                        float stream_rate = (float) cmt_stream_get_rate ( cmttool->stream );
                        cmttool->block_baud_rate = stream_rate / ( (float) cmttool->block_samples / count_symbols );

                        return data;
                    };
                } else {
                    totalScans += cmttool->loadTm;
                };
            };

            if ( cmttool->status == CMTTOOL_STATUS_OK ) {
                cmttool->shortTm = totalScans / CMTTOOL_SYNCHRO_BLOCK_SIZE;
                cmttool->sampTm = totalScans / CMTTOOL_SYMCHRO_SAMPLE_DIVIDER;
            };
        };
    };

    return NULL;
}


static void cmttool_print_header ( st_MZF_HEADER *hdr, gboolean full ) {
    if ( full ) {
        char ascii_filename[MZF_FNAME_FULL_LENGTH];
        mzf_tools_get_fname ( hdr, ascii_filename );
        printf ( "fname: %s\n", ascii_filename );
        printf ( "ftype: 0x%02x\n", hdr->ftype );
    };
    printf ( "fstrt: 0x%04x\n", hdr->fstrt );
    printf ( "fsize: 0x%04x\n", hdr->fsize );
    printf ( "fexec: 0x%04x\n", hdr->fexec );
}


static st_MZF_HEADER* cmttool_search_mzf_header ( st_CMTTOOL *cmttool, uint32_t position ) {

    if ( cmttool->debug ) printf ( "Searching MZF header\n" );

    st_MZF_HEADER *hdr = (st_MZF_HEADER*) cmttool_read_block ( cmttool, CMTTOOL_TAPEMARK_HEADER, sizeof ( st_MZF_HEADER ), position );
    if ( !hdr ) return NULL;

    if ( cmttool->debug ) printf ( "Header found! (%d Baud/s). End at position: %d\n", cmttool->block_baud_rate, cmttool->pos );
    mzf_header_items_correction ( hdr );
    return hdr;
}

#if 0


static void cmttool_hexdump_block ( guint8 *body, guint16 size ) {
    int i = 0;
    while ( size ) {
        int row_size = ( ( size >= 8 ) ? 8 : size );
        size -= row_size;
        row_size += i;
        for ( i = i; i < row_size; i++ ) {
            g_print ( "0x%02x, ", body[i] );
        };
        g_print ( "\n" );
    };
}
#endif


static st_CMTTOOL_FILE* cmttool_search_file ( st_CMTTOOL *cmttool, uint32_t position ) {

    st_MZF_HEADER *hdr = cmttool_search_mzf_header ( cmttool, position );
    if ( !hdr ) return NULL;

    if ( cmttool->debug ) cmttool_print_header ( hdr, TRUE );

    //cmttool_hexdump_block ( (guint8*) hdr, sizeof ( st_MZF_HEADER ) );

    uint32_t hdr_baud_rate = cmttool->block_baud_rate;
    uint32_t hdr_shortTm = cmttool->shortTm;
    uint32_t body_baud_rate;
    uint32_t body_shortTm;

    en_CMTTOOL_FORMAT format = CMTTOOL_FORMAT_NORMAL;
    uint8_t *body = NULL;
    uint32_t body_size;

    en_CMTTOOL_FASTIPL_VERSION fastipl_version = cmttool_fastipl_test_loader ( hdr );
    if ( CMTTOOL_FASTIPL_VERSION_NONE != fastipl_version ) {

        format = CMTTOOL_FORMAT_FASTIPL;
        if ( cmttool->debug ) printf ( "Fast IPL %s loader detect!\n", cmttool_fastipl_get_version_txt ( fastipl_version ) );

        uint8_t readpoint = cmttool_fastipl_get_readpoint ( hdr );
        uint8_t blocks = cmttool_fastipl_get_blcount ( hdr );

        hdr->fsize = cmttool_fastipl_get_fsize ( hdr );
        hdr->fstrt = cmttool_fastipl_get_fstrt ( hdr );
        hdr->fexec = cmttool_fastipl_get_fexec ( hdr );
        body_size = hdr->fsize;

        if ( cmttool->debug ) {
            printf ( "blocks: %d\n", blocks );
            printf ( "readpoint: 0x%02x\n", readpoint );
            cmttool_print_header ( hdr, FALSE );
        };

        int i = 0;
        while ( !body ) {
            i++;

            body = cmttool_read_block ( cmttool, CMTTOOL_TAPEMARK_BODY, body_size, cmttool->pos );

            if ( !body ) {
                if ( ( i < blocks ) && ( cmttool->status != CMTTOOL_STATUS_EOF ) ) {
                    fprintf ( stderr, "Error when reading file FastIPL body '%d'! Try next block...\n", cmttool->status );
                } else {
                    fprintf ( stderr, "Error when reading file FastIPL body '%d'!\n", cmttool->status );
                    g_free ( hdr );
                    return NULL;
                };
            };
        };

        if ( cmttool->debug ) printf ( "\nFastIPL body - OK! (%d Baud/s). End at position: %d\n", cmttool->block_baud_rate, cmttool->pos );

        body_baud_rate = cmttool->block_baud_rate;
        body_shortTm = cmttool->shortTm;

    } else if ( ( hdr->ftype == CMTTOOL_BSD_FTYPE ) && ( hdr->fsize == 0 ) && ( hdr->fstrt == 0 ) && ( hdr->fexec == 0 ) ) {

        format = CMTTOOL_FORMAT_BSD;
        if ( cmttool->debug ) printf ( "BSD data header detected!\n" );

        uint16_t chunk_id;
        uint16_t last_chunk_id = 0;
        int bsd_chunks_count = 0;
        uint32_t chunk_baud_rate = 0;
        uint32_t chunk_shortTm = 0;

        do {

            uint8_t *chunk_body = cmttool_read_block ( cmttool, CMTTOOL_TAPEMARK_BODY, CMTTOOL_BSD_BLOCK_SIZE, cmttool->pos );

            if ( !chunk_body ) {
                fprintf ( stderr, "Error when reading file body '%d'!\n", cmttool->status );
                g_free ( hdr );
                if ( body ) g_free ( body );
                return NULL;
            };

            chunk_id = chunk_body[0] | ( chunk_body[1] << 8 );

            if ( bsd_chunks_count == 0 ) {
                if ( !( ( chunk_id == 0x0000 ) || ( chunk_id == CMTTOOL_BSD_LAST_CHUNK_ID ) ) ) {
                    fprintf ( stderr, "Incorrect 0. BSD chunk id 0x%04x!\n", chunk_id );
                    g_free ( hdr );
                    g_free ( chunk_body );
                    if ( body ) g_free ( body );
                    return NULL;
                };
            } else if ( chunk_id != CMTTOOL_BSD_LAST_CHUNK_ID ) {
                if ( chunk_id != ( last_chunk_id + 1 ) ) {
                    fprintf ( stderr, "Incorrect %d. BSD chunk id 0x%04x!\n", bsd_chunks_count, chunk_id );
                    g_free ( hdr );
                    g_free ( chunk_body );
                    if ( body ) g_free ( body );
                    return NULL;
                };
            };
            last_chunk_id = chunk_id;

            if ( cmttool->debug ) printf ( "%d. BSD chunk - OK (id = 0x%04x)! (%d Baud/s). End at position: %d\n", bsd_chunks_count, chunk_id, cmttool->block_baud_rate, cmttool->pos );
            bsd_chunks_count++;

            body_size = ( CMTTOOL_BSD_BLOCK_SIZE - 2 ) * bsd_chunks_count;

            if ( bsd_chunks_count == 1 ) {
                body = (uint8_t*) ui_utils_mem_alloc ( body_size );
            } else {
                body = (uint8_t*) ui_utils_mem_realloc ( body, body_size );
            };
            memcpy ( &body[ ( body_size - ( CMTTOOL_BSD_BLOCK_SIZE - 2 ) ) ], &chunk_body[2], ( CMTTOOL_BSD_BLOCK_SIZE - 2 ) );
            g_free ( chunk_body );

            chunk_baud_rate += cmttool->block_baud_rate;
            chunk_shortTm += cmttool->shortTm;

        } while ( chunk_id != CMTTOOL_BSD_LAST_CHUNK_ID );

        if ( cmttool->debug ) printf ( "BSD body (size: %d) - DONE! End at position: %d\n", body_size, cmttool->pos );

        body_baud_rate = ( ( chunk_baud_rate * 10 ) / bsd_chunks_count ) / 10;
        body_shortTm = ( ( chunk_shortTm * 10 ) / chunk_baud_rate ) / 10;

    } else {

        body_size = hdr->fsize;
        body = cmttool_read_block ( cmttool, CMTTOOL_TAPEMARK_BODY, body_size, cmttool->pos );

        if ( !body ) {
            fprintf ( stderr, "Error when reading file body '%d'!\n", cmttool->status );
            g_free ( hdr );
            return NULL;
        };

        if ( cmttool->debug ) printf ( "Body - OK! (%d Baud/s). End at position: %d\n", cmttool->block_baud_rate, cmttool->pos );

        //cmttool_hexdump_block ( body, body_size );

        en_CMTTOOL_TURBO_VERSION turbo_version = cmttool_turbo_test_loader ( hdr, body );
        if ( turbo_version != CMTTOOL_TURBO_VERSION_NONE ) {

            format = CMTTOOL_FORMAT_TURBO;
            if ( cmttool->debug ) printf ( "TurboCopy %s loader detect!\n", cmttool_turbo_get_version_txt ( turbo_version ) );

            uint8_t readpoint = cmttool_turbo_get_readpoint ( body, turbo_version );
            uint8_t blocks = cmttool_turbo_get_blcount ( body, turbo_version );

            hdr->fsize = cmttool_turbo_get_fsize ( body, turbo_version );
            hdr->fstrt = cmttool_turbo_get_fstrt ( body, turbo_version );
            hdr->fexec = cmttool_turbo_get_fexec ( body, turbo_version );
            body_size = hdr->fsize;
            if ( turbo_version == CMTTOOL_TURBO_VERSION_12 ) cmttool_turbo12_fix_hdrcmnt ( hdr, body );

            g_free ( body );
            body = NULL;

            if ( cmttool->debug ) {
                printf ( "blocks: %d\n", blocks );
                printf ( "readpoint: 0x%02x\n", readpoint );
                cmttool_print_header ( hdr, FALSE );
            };

            int i = 0;
            while ( !body ) {
                i++;

                body = cmttool_read_block ( cmttool, CMTTOOL_TAPEMARK_BODY, body_size, cmttool->pos );

                if ( !body ) {
                    if ( ( i < blocks ) && ( cmttool->status != CMTTOOL_STATUS_EOF ) ) {
                        fprintf ( stderr, "Error when reading file TURBO body '%d'! Try next block...\n", cmttool->status );
                    } else {
                        fprintf ( stderr, "Error when reading file TURBO body '%d'!\n", cmttool->status );
                        g_free ( hdr );
                        return NULL;
                    };
                };
            };

            if ( cmttool->debug ) printf ( "\nTURBO body - OK! (%d Baud/s). End at position: %d\n", cmttool->block_baud_rate, cmttool->pos );
        };

        body_baud_rate = cmttool->block_baud_rate;
        body_shortTm = cmttool->shortTm;
    };

    st_CMTTOOL_FILE *cf = (st_CMTTOOL_FILE*) ui_utils_mem_alloc ( sizeof ( st_CMTTOOL_FILE ) );

    cf->start_pos = position;
    cf->format = format;
    cf->hdr = hdr;
    cf->hdr_baud_rate = hdr_baud_rate;
    cf->hdr_shortTm = hdr_shortTm;
    cf->body = body;
    cf->body_baud_rate = body_baud_rate;
    cf->body_shortTm = body_shortTm;
    cf->body_size = body_size;

    return cf;
}


void cmttool_add_file ( st_CMTTOOL *cmttool, st_CMTTOOL_FILE *cf ) {
    if ( !cmttool->count_files ) {
        cmttool->cf = (st_CMTTOOL_FILE**) ui_utils_mem_alloc ( sizeof ( cmttool->cf ) );
    } else {
        cmttool->cf = (st_CMTTOOL_FILE**) ui_utils_mem_realloc ( cmttool->cf, ( cmttool->count_files + 1 ) * sizeof ( cmttool->cf ) );
    };
    cf->id = cmttool->count_files;
    cmttool->cf[cmttool->count_files++] = cf;
}


st_CMT_STREAM* cmttool_get_stream ( st_CMTTOOL *cmttool ) {
    return cmttool->stream;
}


guint32 cmttool_get_count_files ( st_CMTTOOL *cmttool ) {
    return cmttool->count_files;
}


st_CMTTOOL_FILE* cmttool_get_file ( st_CMTTOOL *cmttool, guint32 position ) {
    if ( !( cmttool->count_files > position ) ) return NULL;
    return cmttool->cf[position];
}


guint32 cmttool_analyze ( st_CMTTOOL *cmttool, uint32_t position ) {

    cmttool_reset_cmtfiles ( cmttool );
    uint32_t new_position = position;

    do {
        st_CMTTOOL_FILE *cf = cmttool_search_file ( cmttool, new_position );
        if ( cf ) cmttool_add_file ( cmttool, cf );
        new_position = cmttool->pos;
    } while ( cmttool->status < CMTTOOL_STATUS_EOF );

    return cmttool_get_count_files ( cmttool );
}


st_MZF_HEADER* cmttool_file_get_header ( st_CMTTOOL_FILE *cf ) {
    return cf->hdr;
}


en_CMTTOOL_FORMAT cmttool_file_get_format ( st_CMTTOOL_FILE *cf ) {
    return cf->format;
}


const gchar* cmttool_file_get_format_txt ( st_CMTTOOL_FILE *cf ) {

    en_CMTTOOL_FORMAT format = cmttool_file_get_format ( cf );

    if ( format == CMTTOOL_FORMAT_NORMAL ) {
        return "NORMAL";
    } else if ( format == CMTTOOL_FORMAT_BSD ) {
        return "BSD";
    } else if ( format == CMTTOOL_FORMAT_TURBO ) {
        return "TURBO";
    } else if ( format == CMTTOOL_FORMAT_FASTIPL ) {
        return "FastIPL";
    }

    return "??????";
}


guint32 cmttool_file_get_id ( st_CMTTOOL_FILE *cf ) {
    return cf->id;
}


guint32 cmttool_file_get_start_pos ( st_CMTTOOL_FILE *cf ) {
    return cf->start_pos;
}


guint32 cmttool_file_get_body_size ( st_CMTTOOL_FILE *cf ) {
    return cf->body_size;
}


guint8* cmttool_file_get_body ( st_CMTTOOL_FILE *cf ) {
    return cf->body;
}
