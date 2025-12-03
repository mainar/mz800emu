/* 
 * File:   cfgcommon.h
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 14. září 2015, 10:01
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

#ifndef CFGCOMMON_H
#define	CFGCOMMON_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <errno.h>
#include "main.h"

#define CFGCOMMON_MALLOC_ERROR(expr) {\
    if ( expr ) {\
        fprintf ( stderr, "%s():%d - Could not allocate memory: %s\n", __func__, __LINE__, strerror ( errno ) );\
        main_app_quit ( EXIT_FAILURE );\
    };\
}

    /*
     * Privatni funkce
     */
    extern void cfgcommon_set_text ( char **mem_point, char *txt );


#ifdef	__cplusplus
}
#endif

#endif	/* CFGCOMMON_H */

