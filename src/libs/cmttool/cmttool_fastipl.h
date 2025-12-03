/* 
 * File:   cmttool_fastipl.h
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 26. září 2018, 16:29
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


#ifndef CMTTOOL_FASTIPL_H
#define CMTTOOL_FASTIPL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "libs/mzf/mzf.h"


    typedef enum en_CMTTOOL_FASTIPL_VERSION {
        CMTTOOL_FASTIPL_VERSION_NONE = 0,
        CMTTOOL_FASTIPL_VERSION_02 = 2,
        CMTTOOL_FASTIPL_VERSION_07 = 7,
    } en_CMTTOOL_FASTIPL_VERSION;

    extern en_CMTTOOL_FASTIPL_VERSION cmttool_fastipl_test_loader ( st_MZF_HEADER *hdr );
    extern uint8_t cmttool_fastipl_get_readpoint ( st_MZF_HEADER *hdr );
    extern uint8_t cmttool_fastipl_get_blcount ( st_MZF_HEADER *hdr );
    extern uint16_t cmttool_fastipl_get_fsize ( st_MZF_HEADER *hdr );
    extern uint16_t cmttool_fastipl_get_fstrt ( st_MZF_HEADER *hdr );
    extern uint16_t cmttool_fastipl_get_fexec ( st_MZF_HEADER *hdr );
    extern const char* cmttool_fastipl_get_version_txt ( en_CMTTOOL_FASTIPL_VERSION version );

#ifdef __cplusplus
}
#endif

#endif /* CMTTOOL_FASTIPL_H */

