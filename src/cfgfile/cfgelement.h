/* 
 * File:   cfgelement.h
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 14. září 2015, 9:31
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

#ifndef CFGELEMENT_H
#define CFGELEMENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>

    typedef void ( *cfgelement_propagate_cb_t ) (void *e, void *data );
    typedef void ( *cfgelement_save_cb_t ) (void *e, void *data );


    typedef enum en_CFGELVAR {
        CFGELVAR_VALUE,
        CFGELVAR_DEFAULT_VALUE,
        CFGELVAR_COUNT
    } en_CFGELVAR;


    typedef enum en_CFGELEMENTTYPE {
        CFGENTYPE_KEYWORD = 0,
        CFGENTYPE_BOOL,
        CFGENTYPE_TEXT,
        CFGENTYPE_UNSIGNED,
        CFGENTYPE_COUNT
    } en_CFGELEMENTTYPE;


    typedef union un_CFGELVALUE {
        int int_val;
        char *text_val;
    } un_CFGELVALUE;


    typedef struct st_CFGELPROPKEYWORD {
        char **keywords;
        int *keyword_values;
        int words_count;
    } st_CFGELPROPKEYWORD;


    typedef struct st_CFGELPROPUNSIGNED {
        unsigned min;
        unsigned max;
    } st_CFGELPROPUNSIGNED;


    typedef struct st_CFGELPROPCB {
        cfgelement_propagate_cb_t exec;
        void *data;
    } st_CFGELPROPCB;


    typedef struct st_CFGELSAVECB {
        cfgelement_save_cb_t exec;
        void *data;
    } st_CFGELSAVECB;


    typedef struct st_CFGELEMENT {
        char *name;
        en_CFGELEMENTTYPE type;

        void *parent;

        void *default_value;
        void *value;

        /* TODO: vsechny properties by mely mit jen jeden univerzalni pointer na ktery se pripoji spravna struktura podle typu elementu */
        st_CFGELPROPKEYWORD *pkw;
        st_CFGELPROPUNSIGNED *ppu;

        st_CFGELPROPCB propagate_cb; /* pokud neni NULL, tak se provede pri propagate */
        void *propagate_value_handler; /* pokud neni NULL, tak se sem ulozi propagovana hodnota pri propagate (neplati pro CFGENTYPE_TEXT) */
        void *propagate_value_pointer; /* pokud neni NULL, tak se sem ilozi adresa nove naalokovane pameti (plati pouze pro CFGENTYPE_TEXT)*/

        st_CFGELSAVECB save_cb; /* pokud neni NULL, tak se provede pred save */
        void *save_value_handler; /* podud neni NULL, tak se z nej precte a nastavi hodnota pro ulozeni */
        void *save_value_pointer; /* podud neni NULL, tak se z nej precte adresa naalokovane pameti s obsahem CFGENTYPE_TEXT */

    } st_CFGELEMENT;

    typedef st_CFGELEMENT CFGELM;


    extern void cfgelement_set_propagate_cb ( st_CFGELEMENT *e, cfgelement_propagate_cb_t cbexec, void *cbdata );
    extern void cfgelement_set_save_cb ( st_CFGELEMENT *e, cfgelement_save_cb_t cbexec, void *cbdata );

    extern void cfgelement_set_handlers ( st_CFGELEMENT *e, void *propagate_handler, void *save_handler );
    extern void cfgelement_set_pointers ( st_CFGELEMENT *e, void *propagate_pointer, void *save_pointer );

    extern void cfgelement_reset ( st_CFGELEMENT *e );

    extern void cfgelement_set_unsigned_value ( st_CFGELEMENT *e, unsigned unsigned_value );
    extern void cfgelement_set_text_value ( st_CFGELEMENT *e, const char *text_value );
    extern void cfgelement_set_bool_value ( st_CFGELEMENT *e, int bool_value );
    extern void cfgelement_set_keyword_value ( st_CFGELEMENT *e, int key_value );

    extern void cfgelement_set_unsigned_default_value ( st_CFGELEMENT *e, unsigned unsigned_value );
    extern void cfgelement_set_text_default_value ( st_CFGELEMENT *e, char *text_value );
    extern void cfgelement_set_bool_default_value ( st_CFGELEMENT *e, int bool_value );
    extern void cfgelement_set_keyword_default_value ( st_CFGELEMENT *e, int key_value );

    extern unsigned cfgelement_get_unsigned_value ( st_CFGELEMENT *e );
    extern char* cfgelement_get_text_value ( st_CFGELEMENT *e );
    extern int cfgelement_get_bool_value ( st_CFGELEMENT *e );
    extern int cfgelement_get_keyword_value ( st_CFGELEMENT *e );

    extern unsigned cfgelement_get_unsigned_default_value ( st_CFGELEMENT *e );
    extern char* cfgelement_get_text_default_value ( st_CFGELEMENT *e );
    extern int cfgelement_get_bool_default_value ( st_CFGELEMENT *e );
    extern int cfgelement_get_keyword_default_value ( st_CFGELEMENT *e );

    extern char* cfgelement_get_keyword_by_value ( st_CFGELEMENT *e );
    extern char* cfgelement_get_keyword_by_default_value ( st_CFGELEMENT *e );

    extern int cfgelement_property_get_value_by_keyword ( st_CFGELPROPKEYWORD *pkw, char *key_word );

    extern int cfgelement_property_test_unsigned_value ( st_CFGELPROPUNSIGNED *ppu, unsigned value );

    extern void cfgelement_propagate ( st_CFGELEMENT *e );
    extern void cfgelement_save ( st_CFGELEMENT *e );

    /*
     * Privatni funkce
     */
    extern st_CFGELEMENT* cfgcommon_new_element ( void *parent, char *element_name, en_CFGELEMENTTYPE type, va_list args );
    extern void cfgcommon_destroy_element ( st_CFGELEMENT *e );


#ifdef __cplusplus
}
#endif

#endif /* CFGELEMENT_H */

