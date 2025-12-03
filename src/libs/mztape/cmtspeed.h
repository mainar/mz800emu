/* 
 * File:   cmtspeed.h
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 23. května 2018, 8:46
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


#ifndef CMTSPEED_H
#define CMTSPEED_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>
#include <math.h>


    typedef enum en_CMTSPEED {
        CMTSPEED_NONE = 0,
        CMTSPEED_1_1, // 1:1
        CMTSPEED_2_1, // 2:1
        CMTSPEED_2_1_CPM, // 2:1
        CMTSPEED_3_1, // 3:1
        CMTSPEED_3_2, // 3:2
        CMTSPEED_7_3, // 7:3
        CMTSPEED_8_3, // 8:3
        CMTSPEED_9_7, // 9:7
        CMTSPEED_25_14, // 25:14
    } en_CMTSPEED;

    extern const double g_cmtspeed_divisor[];
    extern const char *g_cmtspeed_ratio[];


    static inline double cmtspeed_get_divisor ( en_CMTSPEED cmtspeed ) {
        return g_cmtspeed_divisor[cmtspeed];
    }


    static inline uint16_t cmtspeed_get_bdspeed ( en_CMTSPEED cmtspeed, uint16_t base_bdspeed ) {
        return (uint16_t) round ( g_cmtspeed_divisor[cmtspeed] * base_bdspeed );
    }


    static inline const char* cmtspeed_get_ratiotxt ( en_CMTSPEED cmtspeed ) {
        return g_cmtspeed_ratio[cmtspeed];
    }


    static inline void cmtspeed_get_speedtxt ( char *dsttxt, int size, en_CMTSPEED cmtspeed, uint16_t base_bdspeed ) {
        if ( cmtspeed == CMTSPEED_2_1_CPM ) {
            snprintf ( dsttxt, size, "%d Bd (cp/m)", cmtspeed_get_bdspeed ( cmtspeed, base_bdspeed ) );
        } else {
            snprintf ( dsttxt, size, "%d Bd", cmtspeed_get_bdspeed ( cmtspeed, base_bdspeed ) );
        };
    }


    static inline void cmtspeed_get_ratiospeedtxt ( char *dsttxt, int size, en_CMTSPEED cmtspeed, uint16_t base_bdspeed ) {
        snprintf ( dsttxt, size, "%s - %d Bd", cmtspeed_get_ratiotxt ( cmtspeed ), cmtspeed_get_bdspeed ( cmtspeed, base_bdspeed ) );
    }

#ifdef __cplusplus
}
#endif

#endif /* CMTSPEED_H */

