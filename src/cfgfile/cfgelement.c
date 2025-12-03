/* 
 * File:   cfgelement.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 14. září 2015, 9:25
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
#include <stdarg.h>


#include "cfgelement.h"
#include "cfgcommon.h"
#include "cfgmodule.h"
#include "cfgroot.h"


static void* cfgelement_get_variable_pointer ( st_CFGELEMENT *e, en_CFGELVAR vartype ) {

    assert ( e != NULL );
    assert ( vartype < CFGELVAR_COUNT );

    if ( vartype == CFGELVAR_VALUE ) {
        return &e->value;
    }
    if ( vartype == CFGELVAR_DEFAULT_VALUE ) {
        return &e->default_value;
    };
    return NULL;
}


static char* cfgelement_property_get_keyword_by_value ( st_CFGELPROPKEYWORD *pkw, int value ) {

    assert ( pkw != NULL );

    if ( pkw->keywords == NULL ) return NULL;

    int i;
    for ( i = 0; i < pkw->words_count; i++ ) {
        if ( pkw->keyword_values[i] == value ) return pkw->keywords[i];
    };
    return NULL;
}


int cfgelement_property_get_value_by_keyword ( st_CFGELPROPKEYWORD *pkw, char *key_word ) {

    assert ( pkw != NULL );

    if ( pkw->keywords == NULL ) return -1;

    int i;
    for ( i = 0; i < pkw->words_count; i++ ) {
        if ( 0 == strcasecmp ( pkw->keywords[i], key_word ) ) return pkw->keyword_values[i];
    };
    return -1;
}


int cfgelement_property_test_unsigned_value ( st_CFGELPROPUNSIGNED *ppu, unsigned value ) {

    assert ( ppu != NULL );
    if ( value < ppu->min ) return -1;
    if ( value > ppu->max ) return -1;
    return 0;
}


unsigned cfgelement_get_unsigned_value ( st_CFGELEMENT *e ) {
    assert ( e != NULL );
    assert ( e->type == CFGENTYPE_UNSIGNED );
    void **var_poi = cfgelement_get_variable_pointer ( e, CFGELVAR_VALUE );
    return *(unsigned*) *var_poi;
}


char* cfgelement_get_text_value ( st_CFGELEMENT *e ) {
    assert ( e != NULL );
    assert ( e->type == CFGENTYPE_TEXT );
    void **var_poi = cfgelement_get_variable_pointer ( e, CFGELVAR_VALUE );
    return (char*) *var_poi;
}


int cfgelement_get_bool_value ( st_CFGELEMENT *e ) {
    assert ( e != NULL );
    assert ( e->type == CFGENTYPE_BOOL );
    void **var_poi = cfgelement_get_variable_pointer ( e, CFGELVAR_VALUE );
    return *(int*) *var_poi;
}


int cfgelement_get_keyword_value ( st_CFGELEMENT *e ) {
    assert ( e != NULL );
    assert ( e->type == CFGENTYPE_KEYWORD );
    void **var_poi = cfgelement_get_variable_pointer ( e, CFGELVAR_VALUE );
    return *(int*) *var_poi;
}


unsigned cfgelement_get_unsigned_default_value ( st_CFGELEMENT *e ) {
    assert ( e != NULL );
    assert ( e->type == CFGENTYPE_UNSIGNED );
    void **var_poi = cfgelement_get_variable_pointer ( e, CFGELVAR_DEFAULT_VALUE );
    return *(unsigned*) *var_poi;
}


char* cfgelement_get_text_default_value ( st_CFGELEMENT *e ) {
    assert ( e != NULL );
    assert ( e->type == CFGENTYPE_TEXT );
    void **var_poi = cfgelement_get_variable_pointer ( e, CFGELVAR_DEFAULT_VALUE );
    return (char*) *var_poi;
}


int cfgelement_get_bool_default_value ( st_CFGELEMENT *e ) {
    assert ( e != NULL );
    assert ( e->type == CFGENTYPE_BOOL );
    void **var_poi = cfgelement_get_variable_pointer ( e, CFGELVAR_DEFAULT_VALUE );
    return *(int*) *var_poi;
}


int cfgelement_get_keyword_default_value ( st_CFGELEMENT *e ) {
    assert ( e != NULL );
    assert ( e->type == CFGENTYPE_KEYWORD );
    void **var_poi = cfgelement_get_variable_pointer ( e, CFGELVAR_DEFAULT_VALUE );
    return *(int*) *var_poi;
}


char* cfgelement_get_keyword_by_value ( st_CFGELEMENT *e ) {
    assert ( e != NULL );
    assert ( e->type == CFGENTYPE_KEYWORD );
    int value = cfgelement_get_keyword_value ( e );
    return cfgelement_property_get_keyword_by_value ( e->pkw, value );
}


char* cfgelement_get_keyword_by_default_value ( st_CFGELEMENT *e ) {
    assert ( e != NULL );
    assert ( e->type == CFGENTYPE_KEYWORD );
    int value = cfgelement_get_keyword_default_value ( e );
    return cfgelement_property_get_keyword_by_value ( e->pkw, value );
}


/*
 * Alokuje a nastavi value, nebo default value
 *      0 - error (nenalezen keyword)
 *      1 - OK
 */
static int cfgelement_set_variable ( st_CFGELEMENT *e, en_CFGELVAR vartype, void *set_value ) {

    assert ( e != NULL );
    assert ( vartype < CFGELVAR_COUNT );

    void **var_poi = cfgelement_get_variable_pointer ( e, vartype );

    if ( e->type == CFGENTYPE_KEYWORD ) {

        if ( NULL == cfgelement_property_get_keyword_by_value ( e->pkw, *(int*) set_value ) ) return 0;

        if ( *var_poi == NULL ) {
            *var_poi = malloc ( sizeof ( int ) );
            CFGCOMMON_MALLOC_ERROR ( *var_poi == NULL );
        };
        int *int_poi = *var_poi;
        *int_poi = *(int*) set_value;

    } else if ( e->type == CFGENTYPE_BOOL ) {
        if ( *var_poi == NULL ) {
            *var_poi = malloc ( sizeof ( int ) );
            CFGCOMMON_MALLOC_ERROR ( *var_poi == NULL );
        };
        int *int_poi = *var_poi;
        *int_poi = ( *(int*) set_value ) ? 1 : 0;

    } else if ( e->type == CFGENTYPE_TEXT ) {

        cfgcommon_set_text ( (char**) var_poi, *(char**) set_value );

    } else if ( e->type == CFGENTYPE_UNSIGNED ) {
        if ( *var_poi == NULL ) {
            *var_poi = malloc ( sizeof ( int ) );
            CFGCOMMON_MALLOC_ERROR ( *var_poi == NULL );
        };
        int *int_poi = *var_poi;
        *int_poi = *(int*) set_value;
    };

    return 1;
}


void cfgelement_set_unsigned_value ( st_CFGELEMENT *e, unsigned unsigned_value ) {
    assert ( e != NULL );
    assert ( e->type == CFGENTYPE_UNSIGNED );
    cfgelement_set_variable ( e, CFGELVAR_VALUE, (void *) &unsigned_value );
}


void cfgelement_set_text_value ( st_CFGELEMENT *e, const char *text_value ) {
    assert ( e != NULL );
    assert ( e->type == CFGENTYPE_TEXT );
    cfgelement_set_variable ( e, CFGELVAR_VALUE, (void *) &text_value );
}


void cfgelement_set_bool_value ( st_CFGELEMENT *e, int bool_value ) {
    assert ( e != NULL );
    assert ( e->type == CFGENTYPE_BOOL );
    cfgelement_set_variable ( e, CFGELVAR_VALUE, (void *) &bool_value );
}


void cfgelement_set_keyword_value ( st_CFGELEMENT *e, int key_value ) {
    assert ( e != NULL );
    assert ( e->type == CFGENTYPE_KEYWORD );
    int set_key_value = cfgelement_set_variable ( e, CFGELVAR_VALUE, (void *) &key_value );
    assert ( set_key_value == 1 );
}


void cfgelement_set_unsigned_default_value ( st_CFGELEMENT *e, unsigned unsigned_value ) {
    assert ( e != NULL );
    assert ( e->type == CFGENTYPE_UNSIGNED );
    cfgelement_set_variable ( e, CFGELVAR_DEFAULT_VALUE, (void *) &unsigned_value );
}


void cfgelement_set_text_default_value ( st_CFGELEMENT *e, char *text_value ) {
    assert ( e != NULL );
    assert ( e->type == CFGENTYPE_TEXT );
    cfgelement_set_variable ( e, CFGELVAR_DEFAULT_VALUE, (void *) &text_value );
}


void cfgelement_set_bool_default_value ( st_CFGELEMENT *e, int bool_value ) {
    assert ( e != NULL );
    assert ( e->type == CFGENTYPE_BOOL );
    cfgelement_set_variable ( e, CFGELVAR_DEFAULT_VALUE, (void *) &bool_value );
}


void cfgelement_set_keyword_default_value ( st_CFGELEMENT *e, int key_value ) {
    assert ( e != NULL );
    assert ( e->type == CFGENTYPE_KEYWORD );
    int set_key_default_value = cfgelement_set_variable ( e, CFGELVAR_DEFAULT_VALUE, (void *) &key_value );
    assert ( set_key_default_value == 1 );
}


/*
 * Vytvoreni / pridani keyword polozky
 *      0 - nepridano (duplicita klice)
 *      1 - OK
 * 
 */
static int cfgelement_property_add_keyword ( st_CFGELPROPKEYWORD *pkw, int key_value, char *key_word ) {

    assert ( pkw != NULL );

    if ( -1 != cfgelement_property_get_value_by_keyword ( pkw, key_word ) ) return 0;

    unsigned words_count = pkw->words_count + 1;
    if ( NULL == pkw->keywords ) {
        pkw->keywords = malloc ( sizeof ( char * ) );
        pkw->keyword_values = malloc ( sizeof ( int ) );
    } else {
        pkw->keywords = realloc ( pkw->keywords, words_count * sizeof ( char * ) );
        pkw->keyword_values = realloc ( pkw->keyword_values, words_count * sizeof ( int ) );
    };
    CFGCOMMON_MALLOC_ERROR ( pkw->keywords == NULL );
    CFGCOMMON_MALLOC_ERROR ( pkw->keyword_values == NULL );

    pkw->keyword_values[pkw->words_count] = key_value;
    pkw->keywords[pkw->words_count] = NULL;
    cfgcommon_set_text ( (char**) &pkw->keywords[pkw->words_count], key_word );
    pkw->words_count++;

    return 1;
}


/*
 * Nastaveni minimalni a maximalni hranice pro element typu unsigned
 * 
 */
static void cfgelement_property_set_unsigned ( st_CFGELPROPUNSIGNED *p, unsigned min, unsigned max ) {
    assert ( p != NULL );
    p->min = min;
    p->max = max;
}


/*
 * Vytvoreni noveho elementu
 *      NULL - nelze vytvorit (jiz existuje?)
 * 
 * Variabilni parametry:
 * 
 *      pro CFGENTYPE_KEYWORD nasleduje seznam hodnot a keywordu, hodnota -1 znaci konec seznamu
 *      pro CFGENTYPE_UNSIGNED nasleduje min a max hodnota
 * 
 */
st_CFGELEMENT* cfgcommon_new_element ( void *parent, char *element_name, en_CFGELEMENTTYPE type, va_list args ) {

    assert ( type < CFGENTYPE_COUNT );

    void *default_value_pointer = NULL;

    int default_int_value;
    char *default_text_value;
    st_CFGELPROPKEYWORD *pkw = NULL;
    st_CFGELPROPUNSIGNED *ppu = NULL;

    if ( type == CFGENTYPE_KEYWORD ) {

        default_int_value = va_arg ( args, int );
        default_value_pointer = &default_int_value;

        pkw = malloc ( sizeof ( st_CFGELPROPKEYWORD ) );
        memset ( pkw, 0x00, sizeof ( st_CFGELPROPKEYWORD ) );

        int key_value;

        while ( -1 != ( key_value = va_arg ( args, int ) ) ) {
            char *key_word = va_arg ( args, char * );
            int add_keyword = cfgelement_property_add_keyword ( pkw, key_value, key_word );
            assert ( add_keyword == 1 );
        };

    } else if ( type == CFGENTYPE_BOOL ) {

        default_int_value = va_arg ( args, int );
        default_value_pointer = &default_int_value;

    } else if ( type == CFGENTYPE_TEXT ) {

        default_text_value = va_arg ( args, char * );
        default_value_pointer = &default_text_value;

    } else if ( type == CFGENTYPE_UNSIGNED ) {

        default_int_value = va_arg ( args, int );
        default_value_pointer = &default_int_value;

        ppu = malloc ( sizeof ( st_CFGELPROPUNSIGNED ) );
        memset ( ppu, 0x00, sizeof ( st_CFGELPROPUNSIGNED ) );

        unsigned min = va_arg ( args, int );
        unsigned max = va_arg ( args, int );

        cfgelement_property_set_unsigned ( ppu, min, max );

    } else {
        fprintf ( stderr, "%s():%d - Unknown element type: %d\n", __func__, __LINE__, type );
    };

    st_CFGELEMENT *e = malloc ( sizeof ( st_CFGELEMENT ) );
    CFGCOMMON_MALLOC_ERROR ( e == NULL );
    memset ( e, 0x00, sizeof ( st_CFGELEMENT ) );

    cfgcommon_set_text ( &e->name, element_name );
    e->type = type;
    e->parent = parent;

    if ( pkw != NULL ) {
        e->pkw = pkw;
    };
    if ( ppu != NULL ) {
        e->ppu = ppu;
    };

    int set_variable;
    set_variable = cfgelement_set_variable ( e, CFGELVAR_DEFAULT_VALUE, default_value_pointer );
    assert ( set_variable == 1 );
    cfgelement_set_variable ( e, CFGELVAR_VALUE, default_value_pointer );

    return e;
}


static void cfgelement_property_keyword_destroy ( st_CFGELPROPKEYWORD *pkw ) {

    if ( pkw == NULL ) return;

    if ( pkw->keyword_values != NULL ) {
        free ( pkw->keyword_values );
    };

    int i;
    for ( i = 0; i < pkw->words_count; i++ ) {
        if ( pkw->keywords[i] != NULL ) {
            free ( pkw->keywords[i] );
        };
    };

    free ( pkw );
}


void cfgcommon_destroy_element ( st_CFGELEMENT *e ) {

    if ( e == NULL ) return;

    if ( e->name != NULL ) {
        free ( e->name );
    };

    if ( e->value != NULL ) {
        free ( e->value );
    };

    if ( e->default_value != NULL ) {
        free ( e->default_value );
    };

    if ( e->type == CFGENTYPE_KEYWORD ) {
        cfgelement_property_keyword_destroy ( e->pkw );
    };

    //    if ( e->properties != NULL ) {
    //        free ( e->properties );
    //    };

    free ( e );
}


void cfgelement_reset ( st_CFGELEMENT *e ) {
    assert ( e != NULL );
    void **default_value_pointer = cfgelement_get_variable_pointer ( e, CFGELVAR_DEFAULT_VALUE );
    cfgelement_set_variable ( e, CFGELVAR_VALUE, default_value_pointer );
}


void cfgelement_set_propagate_cb ( st_CFGELEMENT *e, cfgelement_propagate_cb_t cbexec, void *cbdata ) {
    assert ( e != NULL );
    e->propagate_cb.exec = cbexec;
    e->propagate_cb.data = cbdata;
}


void cfgelement_set_save_cb ( st_CFGELEMENT *e, cfgelement_save_cb_t cbexec, void *cbdata ) {
    assert ( e != NULL );
    e->save_cb.exec = cbexec;
    e->save_cb.data = cbdata;
}


void cfgelement_set_handlers ( st_CFGELEMENT *e, void *propagate_handler, void *save_handler ) {
    assert ( e != NULL );
    if ( propagate_handler != NULL ) {
        assert ( e->type != CFGENTYPE_TEXT );
    };
    e->propagate_value_handler = propagate_handler;
    e->save_value_handler = save_handler;
}


void cfgelement_set_pointers ( st_CFGELEMENT *e, void *propagate_pointer, void *save_pointer ) {
    assert ( e != NULL );
    assert ( e->type == CFGENTYPE_TEXT );
    e->propagate_value_pointer = propagate_pointer;
    e->save_value_pointer = save_pointer;
}


void cfgelement_propagate ( st_CFGELEMENT *e ) {
    assert ( e != NULL );

    if ( NULL != e->propagate_cb.exec ) {
        e->propagate_cb.exec ( e, e->propagate_cb.data );
    };

    if ( NULL != e->propagate_value_handler ) {
        void **value_pointer = cfgelement_get_variable_pointer ( e, CFGELVAR_VALUE );
        if ( ( e->type == CFGENTYPE_KEYWORD ) || ( e->type == CFGENTYPE_BOOL ) || ( e->type == CFGENTYPE_UNSIGNED ) ) {
            memcpy ( e->propagate_value_handler, *value_pointer, sizeof ( int ) );
        } else if ( e->type == CFGENTYPE_TEXT ) {
            // museli by jsme nekde ukladat i jeho max delku
            fprintf ( stderr, "%s():%d - Can't propagate TEXT type element by handler\n", __func__, __LINE__ );
        } else {
            fprintf ( stderr, "%s():%d - Unknown element type: %d\n", __func__, __LINE__, e->type );
        };
    };

    if ( NULL != e->propagate_value_pointer ) {
        void **value_pointer = cfgelement_get_variable_pointer ( e, CFGELVAR_VALUE );
        if ( e->type == CFGENTYPE_TEXT ) {
            unsigned length = strlen ( (char*) *value_pointer ) + 1;
            char **dst = e->propagate_value_pointer;
            *dst = malloc ( length );
            CFGCOMMON_MALLOC_ERROR ( *dst == NULL );
            if ( length > 1 ) {
                strncpy ( *dst, (char*) *value_pointer, length );
            } else {
                *dst[0] = 0x00;
            };
        } else {
            fprintf ( stderr, "%s():%d - Unknown element type: %d\n", __func__, __LINE__, e->type );
        };
    };
}


void cfgelement_save ( st_CFGELEMENT *e ) {
    assert ( e != NULL );

    /*
     * Priprava na save
     */
    if ( NULL != e->save_cb.exec ) {
        e->save_cb.exec ( e, e->save_cb.data );
    };

    if ( NULL != e->save_value_handler ) {
        void **value_pointer = cfgelement_get_variable_pointer ( e, CFGELVAR_VALUE );

        if ( ( e->type == CFGENTYPE_KEYWORD ) || ( e->type == CFGENTYPE_BOOL ) || ( e->type == CFGENTYPE_UNSIGNED ) ) {
            int *dst = *value_pointer;
            int src = *(int *) e->save_value_handler;
            *dst = src;

        } else if ( e->type == CFGENTYPE_TEXT ) {
            cfgcommon_set_text ( (char**) value_pointer, (char*) e->save_value_handler );

        } else {
            fprintf ( stderr, "%s():%d - Unknown element type: %d\n", __func__, __LINE__, e->type );
            int save_unknown_element_type = 1;
            assert ( save_unknown_element_type == 0 );
        };
    };

    if ( NULL != e->save_value_pointer ) {
        void **value_pointer = cfgelement_get_variable_pointer ( e, CFGELVAR_VALUE );
        if ( e->type == CFGENTYPE_TEXT ) {
            char **src = e->save_value_pointer;
            cfgcommon_set_text ( (char**) value_pointer, *src );

        } else {
            fprintf ( stderr, "%s():%d - Unknown element type: %d\n", __func__, __LINE__, e->type );
            int save_unknown_element_type = 1;
            assert ( save_unknown_element_type == 0 );
        };
    };
    /*
     * Tady zacina samotny save
     */

    st_CFGMODULE *m = e->parent;
    st_CFGROOT *r = m->parent;

    if ( e->type == CFGENTYPE_KEYWORD ) {

        fprintf ( r->ini_fp, "%s = %s\n", e->name, cfgelement_get_keyword_by_value ( e ) );

    } else if ( e->type == CFGENTYPE_BOOL ) {

        fprintf ( r->ini_fp, "%s = %d\n", e->name, cfgelement_get_bool_value ( e ) );

    } else if ( e->type == CFGENTYPE_TEXT ) {

        fprintf ( r->ini_fp, "%s = %s\n", e->name, cfgelement_get_text_value ( e ) );

    } else if ( e->type == CFGENTYPE_UNSIGNED ) {

        fprintf ( r->ini_fp, "%s = 0x%02x\n", e->name, cfgelement_get_unsigned_value ( e ) );

    } else {

        fprintf ( stderr, "%s():%d - Unknown element type: %d\n", __func__, __LINE__, e->type );
        int save_unknown_element_type = 1;
        assert ( save_unknown_element_type == 0 );
    };
}
