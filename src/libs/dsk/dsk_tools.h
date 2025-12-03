/* 
 * File:   dsk_tools.h
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 9. září 2016, 19:00
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


#ifndef DSK_TOOLS_H
#define DSK_TOOLS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "dsk.h"


    typedef enum en_DSK_SECTOR_ORDER_TYPE {
        DSK_SEC_ORDER_CUSTOM = 0, /* vyzaduje prilozenou mapu */
        DSK_SEC_ORDER_NORMAL = 1, /* vytvori mapu 1, 2, 3, ... */
        DSK_SEC_ORDER_INTERLACED_LEC = 2, /* vytvori 1x prokladanou mapu */
        DSK_SEC_ORDER_INTERLACED_LEC_HD = 3 /* vytvori 2x prokladanou mapu */
    } en_DSK_SECTOR_ORDER_TYPE;


    typedef struct st_DSK_DESCRIPTION_RULE {
        uint8_t absolute_track;
        uint8_t sectors;
        en_DSK_SECTOR_SIZE ssize;
        en_DSK_SECTOR_ORDER_TYPE sector_order;
        uint8_t *sector_map;
        uint8_t filler;
    } st_DSK_DESCRIPTION_RULE;


    typedef struct st_DSK_DESCRIPTION {
        uint16_t count_rules;
        uint8_t tracks;
        uint8_t sides;
        void *rules;
    } st_DSK_DESCRIPTION;


    static inline size_t dsk_tools_compute_description_size ( uint8_t rules ) {
        return ( sizeof ( st_DSK_DESCRIPTION ) + sizeof ( st_DSK_DESCRIPTION_RULE ) * rules );
    }

    extern void dsk_tools_assign_description ( st_DSK_DESCRIPTION *dskdesc, uint8_t rule, uint8_t abs_track, uint8_t sectors, en_DSK_SECTOR_SIZE ssize, en_DSK_SECTOR_ORDER_TYPE sector_order, uint8_t *sector_map, uint8_t default_value );

    extern void dsk_tools_make_sector_map ( uint8_t sectors, en_DSK_SECTOR_ORDER_TYPE sector_order, uint8_t *sector_map );

    extern int dsk_tools_create_image ( st_HANDLER *h, st_DSK_DESCRIPTION *desc );
    extern int dsk_tools_create_image_header ( st_HANDLER *h, st_DSK_DESCRIPTION *desc );
    extern int dsk_tools_create_image_tracks ( st_HANDLER *h, st_DSK_DESCRIPTION *desc, uint8_t first_abs_track, uint32_t dsk_offset );
    extern int dsk_tools_create_track ( st_HANDLER *h, uint32_t dsk_offset, uint8_t track, uint8_t side, uint8_t sectors, en_DSK_SECTOR_SIZE ssize, uint8_t *sector_map, uint8_t default_value, uint32_t *track_total_bytes );
    extern int dsk_tools_create_track_header ( st_HANDLER *h, uint32_t dsk_offset, uint8_t track, uint8_t side, uint8_t sectors, en_DSK_SECTOR_SIZE ssize, uint8_t *sector_map );
    extern int dsk_tools_create_track_sectors ( st_HANDLER *h, uint32_t dsk_offset, uint8_t sectors, en_DSK_SECTOR_SIZE ssize, uint8_t default_value, uint16_t *sectors_total_bytes );
    extern int dsk_tools_change_track ( st_HANDLER *h, st_DSK_SHORT_IMAGE_INFO *short_image_info, uint8_t abstrack, uint8_t sectors, en_DSK_SECTOR_SIZE ssize, uint8_t *sector_map, uint8_t default_value );
    extern int dsk_tools_add_tracks ( st_HANDLER *h, st_DSK_DESCRIPTION *desc );
    extern int dsk_tools_shrink_image ( st_HANDLER *h, st_DSK_SHORT_IMAGE_INFO *short_image_info, uint8_t total_tracks );

    extern int dsk_tools_get_dsk_fileinfo ( st_HANDLER *h, uint8_t *dsk_fileinfo_buffer );
    extern int dsk_tools_check_dsk_fileinfo ( st_HANDLER *h );
    extern int dsk_tools_get_dsk_creator ( st_HANDLER *h, uint8_t *dsk_creator_buffer );


    typedef enum en_DSK_TOOLS_CHCKTRKINFO {
        DSK_TOOLS_CHCKTRKINFO_SUCCESS = 0,
        DSK_TOOLS_CHCKTRKINFO_READ_ERROR,
        DSK_TOOLS_CHCKTRKINFO_FAILURE,
    } en_DSK_TOOLS_CHCKTRKINFO;

    extern en_DSK_TOOLS_CHCKTRKINFO dsk_tools_check_dsk_trackinfo_on_offset ( st_HANDLER *h, uint32_t offset );

    extern int dsk_tools_check_dsk ( st_HANDLER *h, int print_info, int dsk_autofix );


    typedef struct st_DSK_TOOLS_TRACK_RULE_INFO {
        uint8_t from_track;
        uint8_t count_tracks;
        uint8_t sectors;
        en_DSK_SECTOR_SIZE ssize;
    } st_DSK_TOOLS_TRACK_RULE_INFO;


    typedef struct st_DSK_TOOLS_TRACKS_RULES_INFO {
        uint8_t total_tracks;
        uint8_t sides;
        uint8_t count_rules;
        uint8_t mzboot_track;
        st_DSK_TOOLS_TRACK_RULE_INFO *rule;
    } st_DSK_TOOLS_TRACKS_RULES_INFO;


    extern void dsk_tools_destroy_track_rules ( st_DSK_TOOLS_TRACKS_RULES_INFO *tracks_rules );
    extern st_DSK_TOOLS_TRACKS_RULES_INFO* dsk_tools_get_tracks_rules ( st_HANDLER *h );


    typedef enum en_DSK_TOOLS_IDENTFORMAT {
        DSK_TOOLS_IDENTFORMAT_UNKNOWN = 0,
        DSK_TOOLS_IDENTFORMAT_MZBASIC, // cely format disku je 16 x 256 B
        DSK_TOOLS_IDENTFORMAT_MZCPM, // 9 x 512 B + 1. stopa je MZBASIC
        DSK_TOOLS_IDENTFORMAT_MZCPMHD, // 18 x 512 B + 1. stopa je MZBASIC
        DSK_TOOLS_IDENTFORMAT_MZBOOT, // 1. stopa je MZBASIC, ale zbyvajici format disku je neznamy
    } en_DSK_TOOLS_IDENTFORMAT;


    extern en_DSK_TOOLS_IDENTFORMAT dsk_tools_identformat_from_tracks_rules ( st_DSK_TOOLS_TRACKS_RULES_INFO *tracks_rules );
    extern int dsk_tools_identformat ( st_HANDLER *h, en_DSK_TOOLS_IDENTFORMAT *result );
    extern st_DSK_TOOLS_TRACK_RULE_INFO* dsk_tools_get_rule_for_track ( st_DSK_TOOLS_TRACKS_RULES_INFO *tracks_rules, uint8_t track );

#ifdef __cplusplus
}
#endif

#endif /* DSK_TOOLS_H */

