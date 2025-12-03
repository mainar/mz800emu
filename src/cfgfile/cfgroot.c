/* 
 * File:   cfgroot.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <assert.h>
#include <errno.h>

#include "cfgroot.h"
#include "cfgcommon.h"

#include "ui/ui_main.h"
#include "ui/ui_utils.h"


/*
 * Vytvoreni nove struktury cfgroot
 */
st_CFGROOT* cfgroot_new ( const char *filename ) {
    st_CFGROOT *r = malloc ( sizeof ( st_CFGROOT ) );
    CFGCOMMON_MALLOC_ERROR ( r == NULL );
    memset ( r, 0x00, sizeof ( st_CFGROOT ) );

    r->filename = filename;
    return r;
}


/*
 * Odstraneni konfiguracni struktury
 */
void cfgroot_destroy ( st_CFGROOT *r ) {

    if ( r == NULL ) return;

    int i;
    for ( i = 0; i < r->modules_count; i++ ) {
        st_CFGMODULE *m = r->modules[i];
        cfgcommon_destroy_module ( m );
    };
    
    free ( r );
}


/*
 * Vytvoreni noveho modulu
 *      NULL - nelze vytvorit (jiz existuje?)
 */
st_CFGMODULE* cfgroot_register_new_module ( st_CFGROOT *r, char *module_name ) {

    assert ( r != NULL );

    if ( NULL != cfgroot_get_module_by_name ( r, module_name ) ) return NULL;

    st_CFGMODULE *m = cfgcommon_new_module ( r, module_name );
    if ( NULL == m ) return NULL;

    unsigned modules_count = r->modules_count + 1;

    if ( NULL == r->modules ) {
        r->modules = malloc ( sizeof ( st_CFGMODULE* ) );
    } else {
        r->modules = realloc ( r->modules, modules_count * sizeof ( st_CFGMODULE* ) );
    };
    CFGCOMMON_MALLOC_ERROR ( r->modules == NULL );

    r->modules[ r->modules_count++ ] = m;

    return m;
}


/*
 * Ziskani modulu podle jmena
 *      NULL - modul nenalezen
 */
st_CFGMODULE* cfgroot_get_module_by_name ( st_CFGROOT *r, char *module_name ) {

    assert ( r != NULL );

    int i;
    for ( i = 0; i < r->modules_count; i++ ) {
        st_CFGMODULE *m = r->modules[i];
        if ( 0 == strcasecmp ( m->name, module_name ) ) return m;
    };
    return NULL;
}


void cfgroot_reset ( st_CFGROOT *r ) {

    assert ( r != NULL );

    int i;
    for ( i = 0; i < r->modules_count; i++ ) {
        st_CFGMODULE *m = r->modules[i];
        cfgmodule_reset ( m );
    };
}


void cfgroot_propagate ( st_CFGROOT *r ) {

    assert ( r != NULL );

    int i;
    for ( i = 0; i < r->modules_count; i++ ) {
        st_CFGMODULE *m = r->modules[i];
        cfgmodule_propagate ( m );
    };
}


/*
 * Prozatim experimentalne natvrdo .INI 
 */
void cfgroot_save ( st_CFGROOT *r ) {

    assert ( r != NULL );

    printf ( "Save config into: %s\n", r->filename );


    if ( !( r->ini_fp = ui_utils_file_open ( r->filename, "w" ) ) ) {
        ui_show_error ( "%s() - Can't open file '%s' to write: %s", __func__, r->filename, strerror ( errno ) );
        return;
    };

    //r->ini_fp = stderr;

    fprintf ( r->ini_fp, "\n" );

    int i;
    for ( i = 0; i < r->modules_count; i++ ) {
        st_CFGMODULE *m = r->modules[i];
        cfgmodule_save ( m );
    };

    fclose ( r->ini_fp );
}


/*
 * CFGCOMMON
 */

void cfgcommon_set_text ( char **mem_point, char *txt ) {
    
    if ( *mem_point == txt ) {
        return;
    };
    
    unsigned length = strlen ( txt ) + 1;
    if ( *mem_point == NULL ) {
        *mem_point = malloc ( length );
    } else {
        *mem_point = realloc ( *mem_point, length );
    };
    CFGCOMMON_MALLOC_ERROR ( *mem_point == NULL );
    strcpy ( *mem_point, txt );
}

