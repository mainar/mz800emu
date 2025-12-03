/* 
 * File:   cmttool_turbo.h
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 26. září 2018, 14:01
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


#ifndef CMTTOOL_TURBO_H
#define CMTTOOL_TURBO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "libs/mzf/mzf.h"


    typedef enum en_CMTTOOL_TURBO_VERSION {
        CMTTOOL_TURBO_VERSION_NONE = 0,
        CMTTOOL_TURBO_VERSION_10 = 10,
        CMTTOOL_TURBO_VERSION_12 = 12,
    } en_CMTTOOL_TURBO_VERSION;

    extern en_CMTTOOL_TURBO_VERSION cmttool_turbo_test_loader ( st_MZF_HEADER *hdr, uint8_t *body );
    extern uint8_t cmttool_turbo_get_readpoint ( uint8_t *body, en_CMTTOOL_TURBO_VERSION version );
    extern uint8_t cmttool_turbo_get_blcount ( uint8_t *body, en_CMTTOOL_TURBO_VERSION version );
    extern uint16_t cmttool_turbo_get_fsize ( uint8_t *body, en_CMTTOOL_TURBO_VERSION version );
    extern uint16_t cmttool_turbo_get_fstrt ( uint8_t *body, en_CMTTOOL_TURBO_VERSION version );
    extern uint16_t cmttool_turbo_get_fexec ( uint8_t *body, en_CMTTOOL_TURBO_VERSION version );
    extern const char* cmttool_turbo_get_version_txt ( en_CMTTOOL_TURBO_VERSION version );
    extern void cmttool_turbo12_fix_hdrcmnt ( st_MZF_HEADER *hdr, uint8_t *body );

#ifdef __cplusplus
}
#endif

#endif /* CMTTOOL_TURBO_H */

