/* 
 * File:   mzf.h
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


#ifndef MZF_H
#define MZF_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "../generic_driver/generic_driver.h"

#define MZF_FILE_NAME_LENGTH 16
#define MZF_CMNT_LENGTH 104

#define MZF_FNAME_TERMINATOR 0x0d


    typedef struct st_MZF_FILENAME {
        uint8_t name [ MZF_FILE_NAME_LENGTH ];
        uint8_t terminator; // 0x0d
    } st_MZF_FILENAME;

#define MZF_FNAME_FULL_LENGTH sizeof ( st_MZF_FILENAME )


    typedef struct st_MZF_HEADER {
        uint8_t ftype;
        st_MZF_FILENAME fname;
        uint16_t fsize;
        uint16_t fstrt;
        uint16_t fexec;
        uint8_t cmnt [ MZF_CMNT_LENGTH ];
    } st_MZF_HEADER;


    typedef struct st_MZF {
        st_MZF_HEADER header;
        uint8_t *body;
    } st_MZF;

    extern void mzf_header_items_correction ( st_MZF_HEADER *mzfhdr );
    extern int mzf_read_header_on_offset ( st_HANDLER *h, uint32_t offset, st_MZF_HEADER *mzfhdr );
    extern int mzf_write_header_on_offset ( st_HANDLER *h, uint32_t offset, st_MZF_HEADER *mzfhdr );

    extern int mzf_read_header ( st_HANDLER *h, st_MZF_HEADER *mzfhdr );
    extern int mzf_write_header ( st_HANDLER *h, st_MZF_HEADER *mzfhdr );
    extern int mzf_read_body_on_offset ( st_HANDLER *h, uint32_t offset, uint8_t *buffer, uint32_t buffer_size );
    extern int mzf_read_body ( st_HANDLER *h, uint8_t *buffer, uint32_t buffer_size );
    extern int mzf_write_body_on_offset ( st_HANDLER *h, uint32_t offset, uint8_t *buffer, uint32_t buffer_size );
    extern int mzf_write_body ( st_HANDLER *h, uint8_t *buffer, uint32_t buffer_size );
    extern const char* mzf_error_message ( st_HANDLER *h, st_DRIVER *d );
    
    extern int mzf_header_test_fname_terminator_on_offset ( st_HANDLER *h, uint32_t offset );
    extern int mzf_header_test_fname_terminator ( st_HANDLER *h );

#define MZF_UINT8_FNAME(n) (uint8_t*)&n

#ifdef __cplusplus
}
#endif

#endif /* MZF_H */

