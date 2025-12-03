/* 
 * File:   cfgroot.h
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 9. září 2015, 8:54
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

#ifndef CFGROOT_H
#define	CFGROOT_H

#ifdef	__cplusplus
extern "C" {
#endif

    
#include <stdio.h>

#include "cfgmodule.h"
    
    typedef struct st_CFGROOT {
        char *name;
        int modules_count;
        st_CFGMODULE **modules;
        
        /* varianta: .INI souboru */
        const char *filename;
        FILE *ini_fp;
    } st_CFGROOT;

    typedef st_CFGROOT CFGR;

    extern st_CFGROOT* cfgroot_new ( const char *filename );
    extern void cfgroot_destroy ( st_CFGROOT *r );

    extern void cfgroot_reset ( st_CFGROOT *r );

    extern st_CFGMODULE* cfgroot_register_new_module ( st_CFGROOT *r, char *module_name );
    extern st_CFGMODULE* cfgroot_get_module_by_name ( st_CFGROOT *r, char *module_name );

    extern void cfgroot_propagate ( st_CFGROOT *r );
    extern void cfgroot_save ( st_CFGROOT *r );


#ifdef	__cplusplus
}
#endif

#endif	/* CFGROOT_H */

