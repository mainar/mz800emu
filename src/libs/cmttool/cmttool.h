/* 
 * File:   cmttool.h
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


#ifndef CMTTOOL_H
#define CMTTOOL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <glib.h>
#include "libs/mzf/mzf.h"
#include "libs/cmt_stream/cmt_stream.h"


    typedef enum en_CMTTOOL_FORMAT {
        CMTTOOL_FORMAT_NORMAL = 0,
        CMTTOOL_FORMAT_TURBO,
        CMTTOOL_FORMAT_FASTIPL,
        CMTTOOL_FORMAT_BSD,
    } en_CMTTOOL_FORMAT;


    typedef enum en_CMTTOOL_TAPEMARK {
        CMTTOOL_TAPEMARK_HEADER = 40,
        CMTTOOL_TAPEMARK_BODY = 20,
    } en_CMTTOOL_TAPEMARK;


    typedef enum en_CMTTOOL_STATUS {
        CMTTOOL_STATUS_OK = 0,
        CMTTOOL_STATUS_ERR_TMARK,
        CMTTOOL_STATUS_ERR_DATA,
        CMTTOOL_STATUS_ERR_CHECKSUM,
        CMTTOOL_STATUS_EOF,
        CMTTOOL_STATUS_ERR_POSITION, // analyzujeme od neexistujici pozice ve streamu
    } en_CMTTOOL_STATUS;


    typedef struct st_CMTTOOL_FILE {
        uint32_t id;
        uint32_t start_pos;
        en_CMTTOOL_FORMAT format;
        st_MZF_HEADER *hdr;
        uint32_t hdr_baud_rate;
        uint32_t hdr_shortTm;
        uint8_t *body;
        uint32_t body_baud_rate;
        uint32_t body_shortTm;
        uint32_t body_size;
    } st_CMTTOOL_FILE;


    typedef struct st_CMTTOOL {
        gboolean debug;
        st_CMT_STREAM *stream;
        uint32_t pos;
        uint32_t shortTm;
        uint32_t sampTm;
        uint32_t loadTm;
        int bit_value;
        uint16_t block_size;
        uint32_t block_samples;
        uint32_t block_baud_rate;
        uint32_t count_files;
        st_CMTTOOL_FILE **cf;
        en_CMTTOOL_STATUS status;
    } st_CMTTOOL;

    extern void cmttool_destroy ( st_CMTTOOL *cmttool );
    extern st_CMTTOOL *cmttool_create_from_wav ( char *filename, en_CMT_STREAM_POLARITY polarity );
    extern void cmttool_set_debug ( st_CMTTOOL *cmttool, gboolean debug );
    extern guint32 cmttool_analyze ( st_CMTTOOL *cmttool, uint32_t position );

    extern st_CMT_STREAM* cmttool_get_stream ( st_CMTTOOL *cmttool );
    extern guint32 cmttool_get_count_files ( st_CMTTOOL *cmttool );
    extern st_CMTTOOL_FILE* cmttool_get_file ( st_CMTTOOL *cmttool, guint32 position );

    extern st_MZF_HEADER* cmttool_file_get_header ( st_CMTTOOL_FILE *cf );
    extern en_CMTTOOL_FORMAT cmttool_file_get_format ( st_CMTTOOL_FILE *cf );
    extern const gchar* cmttool_file_get_format_txt ( st_CMTTOOL_FILE *cf );
    extern guint32 cmttool_file_get_id ( st_CMTTOOL_FILE *cf );
    extern guint32 cmttool_file_get_start_pos ( st_CMTTOOL_FILE *cf );
    extern guint32 cmttool_file_get_body_size ( st_CMTTOOL_FILE *cf );
    extern guint8* cmttool_file_get_body ( st_CMTTOOL_FILE *cf );

#ifdef __cplusplus
}
#endif

#endif /* CMTTOOL_H */

