/* 
 * File:   cfgmodule.h
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 14. září 2015, 9:26
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

#ifndef CFGMODULE_H
#define CFGMODULE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cfgelement.h"

    typedef void (*cfgmodule_propagate_cb_t ) (void *m, void *data );
    typedef void (*cfgmodule_save_cb_t ) (void *m, void *data );


    typedef struct st_CFGMODPROPCB {
        cfgmodule_propagate_cb_t exec;
        void *data;
    } st_CFGMODPROPCB;


    typedef struct st_CFGMODSAVECB {
        cfgmodule_save_cb_t exec;
        void *data;
    } st_CFGMODSAVECB;


    typedef struct st_CFGMODULE {
        char *name;

        void *parent;

        int elements_count;
        st_CFGELEMENT **elements;

        st_CFGMODPROPCB propagate_cb; /* pokud neni NULL, tak se zavola po propagate celeho modulu */
        st_CFGMODSAVECB save_cb; /* pokud neni NULL, tak se zavola pred save celeho modulu */

    } st_CFGMODULE;

    typedef st_CFGMODULE CFGMOD;


    extern void cfgmodule_set_propagate_cb ( st_CFGMODULE *m, cfgmodule_propagate_cb_t cbexec, void *cbdata );
    extern void cfgmodule_set_save_cb ( st_CFGMODULE *m, cfgmodule_save_cb_t cbexec, void *cbdata );

    extern void cfgmodule_reset ( st_CFGMODULE *m );

    extern st_CFGELEMENT* cfgmodule_register_new_element ( st_CFGMODULE *m, char *element_name, en_CFGELEMENTTYPE type, ... );
    extern st_CFGELEMENT* cfgmodule_get_element_by_name ( st_CFGMODULE *m, char *element_name );

    extern unsigned cfgmodule_get_element_unsigned_value_by_name ( st_CFGMODULE *m, char *element_name );
    extern char* cfgmodule_get_element_text_value_by_name ( st_CFGMODULE *m, char *element_name );
    extern int cfgmodule_get_element_bool_value_by_name ( st_CFGMODULE *m, char *element_name );
    extern int cfgmodule_get_element_keyword_value_by_name ( st_CFGMODULE *m, char *element_name );

    extern unsigned cfgmodule_get_element_unsigned_default_value_by_name ( st_CFGMODULE *m, char *element_name );
    extern char* cfgmodule_get_element_text_default_value_by_name ( st_CFGMODULE *m, char *element_name );
    extern int cfgmodule_get_element_bool_default_value_by_name ( st_CFGMODULE *m, char *element_name );
    extern int cfgmodule_get_element_keyword_default_value_by_name ( st_CFGMODULE *m, char *element_name );

    extern char* cfgmodule_get_element_keyword_by_value_by_name ( st_CFGMODULE *m, char *element_name );
    extern char* cfgmodule_get_element_keyword_by_default_value_by_name ( st_CFGMODULE *m, char *element_name );

    extern void cfgmodule_propagate ( st_CFGMODULE *m );
    extern void cfgmodule_save ( st_CFGMODULE *m );

    extern int cfgmodule_parse ( st_CFGMODULE *m );

    /*
     * Privatni funkce
     */
    extern st_CFGMODULE* cfgcommon_new_module ( void *parent, char *module_name );
    extern void cfgcommon_destroy_module ( st_CFGMODULE *m );

#ifdef __cplusplus
}
#endif

#endif /* CFGMODULE_H */

