/* 
 * File:   cfgmodule.c
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

#ifdef WINDOWS
#include<windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <unistd.h>

#include "cfgmodule.h"
#include "cfgcommon.h"
#include "cfgroot.h"

#include "ui/ui_main.h"
#include "ui/ui_utils.h"


/*
 * Vytvoreni noveho modulu
 *      NULL - nelze vytvorit (jiz existuje?)
 */
st_CFGMODULE* cfgcommon_new_module ( void *parent, char *module_name ) {

    st_CFGMODULE *m = malloc ( sizeof ( st_CFGMODULE ) );
    CFGCOMMON_MALLOC_ERROR ( m == NULL );
    memset ( m, 0x00, sizeof ( st_CFGMODULE ) );
    m->parent = parent;

    cfgcommon_set_text ( &m->name, module_name );

    return m;
}


/*
 * Odstraneni konfiguracniho modulu
 */
void cfgcommon_destroy_module ( st_CFGMODULE *m ) {

    if ( m == NULL ) return;

    int i;
    for ( i = 0; i < m->elements_count; i++ ) {
        st_CFGELEMENT *e = m->elements[i];
        cfgcommon_destroy_element ( e );
    };

    if ( m->name != NULL ) {
        free ( m->name );
    };

    free ( m );
}


/*
 * Vytvoreni noveho elementu
 *      NULL - nelze vytvorit (jiz existuje?)
 * 
 * Variabilni parametry:
 * 
 *      - default_value
 *      - pro CFGENTYPE_KEYWORD nasleduje seznam hodnot a keywordu, hodnota -1 znaci konec seznamu
 *      - pro CFGENTYPE_UNSIGNED nasleduje min a max hodnota
 * 
 */
st_CFGELEMENT* cfgmodule_register_new_element ( st_CFGMODULE *m, char *element_name, en_CFGELEMENTTYPE type, ... ) {

    assert ( m != NULL );

    if ( NULL != cfgmodule_get_element_by_name ( m, element_name ) ) return NULL;

    va_list args;
    va_start ( args, type );

    st_CFGELEMENT *e = cfgcommon_new_element ( m, element_name, type, args );
    va_end ( args );

    if ( NULL == e ) return NULL;

    unsigned elements_count = m->elements_count + 1;

    if ( NULL == m->elements ) {
        m->elements = malloc ( sizeof ( st_CFGELEMENT* ) );
    } else {
        m->elements = realloc ( m->elements, elements_count * sizeof ( st_CFGELEMENT* ) );
    };
    CFGCOMMON_MALLOC_ERROR ( m->elements == NULL );

    m->elements[ m->elements_count++ ] = e;

    return e;
}


/*
 * Ziskani elementu podle jmena
 *      NULL - element nenalezen
 */
st_CFGELEMENT* cfgmodule_get_element_by_name ( st_CFGMODULE *m, char *element_name ) {

    assert ( m != NULL );

    int i;
    for ( i = 0; i < m->elements_count; i++ ) {
        st_CFGELEMENT *e = m->elements[i];
        if ( 0 == strcasecmp ( e->name, element_name ) ) return e;
    };
    return NULL;
}


void cfgmodule_reset ( st_CFGMODULE *m ) {

    assert ( m != NULL );

    int i;
    for ( i = 0; i < m->elements_count; i++ ) {
        st_CFGELEMENT *e = m->elements[i];
        cfgelement_reset ( e );
    };
}


void cfgmodule_set_propagate_cb ( st_CFGMODULE *m, cfgmodule_propagate_cb_t cbexec, void *cbdata ) {

    assert ( m != NULL );

    m->propagate_cb.exec = cbexec;
    m->propagate_cb.data = cbdata;
}


void cfgmodule_set_save_cb ( st_CFGMODULE *m, cfgmodule_save_cb_t cbexec, void *cbdata ) {

    assert ( m != NULL );

    m->save_cb.exec = cbexec;
    m->save_cb.data = cbdata;
}


unsigned cfgmodule_get_element_unsigned_value_by_name ( st_CFGMODULE *m, char *element_name ) {
    return cfgelement_get_unsigned_value ( cfgmodule_get_element_by_name ( (CFGMOD *) m, element_name ) );
}


char* cfgmodule_get_element_text_value_by_name ( st_CFGMODULE *m, char *element_name ) {
    return cfgelement_get_text_value ( cfgmodule_get_element_by_name ( (CFGMOD *) m, element_name ) );
}


int cfgmodule_get_element_bool_value_by_name ( st_CFGMODULE *m, char *element_name ) {
    return cfgelement_get_bool_value ( cfgmodule_get_element_by_name ( (CFGMOD *) m, element_name ) );
}


int cfgmodule_get_element_keyword_value_by_name ( st_CFGMODULE *m, char *element_name ) {
    return cfgelement_get_keyword_value ( cfgmodule_get_element_by_name ( (CFGMOD *) m, element_name ) );
}


char* cfgmodule_get_element_text_default_value_by_name ( st_CFGMODULE *m, char *element_name ) {
    return cfgelement_get_text_default_value ( cfgmodule_get_element_by_name ( (CFGMOD *) m, element_name ) );
}


unsigned cfgmodule_get_element_unsigned_default_value_by_name ( st_CFGMODULE *m, char *element_name ) {
    return cfgelement_get_unsigned_default_value ( cfgmodule_get_element_by_name ( (CFGMOD *) m, element_name ) );
}


int cfgmodule_get_element_bool_default_value_by_name ( st_CFGMODULE *m, char *element_name ) {
    return cfgelement_get_bool_default_value ( cfgmodule_get_element_by_name ( (CFGMOD *) m, element_name ) );
}


int cfgmodule_get_element_keyword_default_value_by_name ( st_CFGMODULE *m, char *element_name ) {
    return cfgelement_get_keyword_default_value ( cfgmodule_get_element_by_name ( (CFGMOD *) m, element_name ) );
}


char* cfgmodule_get_element_keyword_by_value_by_name ( st_CFGMODULE *m, char *element_name ) {
    return cfgelement_get_keyword_by_value ( cfgmodule_get_element_by_name ( (CFGMOD *) m, element_name ) );
}


char* cfgmodule_get_element_keyword_by_default_value_by_name ( st_CFGMODULE *m, char *element_name ) {
    return cfgelement_get_keyword_by_default_value ( cfgmodule_get_element_by_name ( (CFGMOD *) m, element_name ) );
}


void cfgmodule_propagate ( st_CFGMODULE *m ) {

    assert ( m != NULL );

    int i;
    for ( i = 0; i < m->elements_count; i++ ) {
        st_CFGELEMENT *e = m->elements[i];
        cfgelement_propagate ( e );
    };

    if ( NULL != m->propagate_cb.exec ) {
        m->propagate_cb.exec ( m, m->save_cb.data );
    };
}


void cfgmodule_save ( st_CFGMODULE *m ) {

    assert ( m != NULL );

    st_CFGROOT *r = m->parent;

    fprintf ( r->ini_fp, "[%s]\n", m->name );

    if ( NULL != m->save_cb.exec ) {
        m->save_cb.exec ( m, m->save_cb.data );
    }

    int i;
    for ( i = 0; i < m->elements_count; i++ ) {
        st_CFGELEMENT *e = m->elements[i];
        cfgelement_save ( e );
    };

    fprintf ( r->ini_fp, "\n" );
}


static char* cfgcommon_search_first_char ( char *str, unsigned str_length ) {
    int i;
    for ( i = 0; i < str_length; i++ ) {
        if ( *str > 0x20 ) break;
        str++;
    };
    return str;
}


static char* cfgcommon_search_word_end ( char *str, unsigned str_length ) {
    int i;
    for ( i = 0; i < str_length; i++ ) {
        if ( ( *str <= 0x20 ) || ( *str == '=' ) ) break;
        str++;
    };
    return str;
}


/* delsi radky jsou v parseru ignorovany (testuje se u nich jen konec modulu, pokud uz v nejakem jsme) */
#define CFG_MAX_LINE_LENGTH     4096


int cfgmodule_parse ( st_CFGMODULE *m ) {

    assert ( m != NULL );

    st_CFGROOT *r = m->parent;

    if ( access ( r->filename, F_OK ) == -1 ) return EXIT_FAILURE; /* Konfiguracni soubor neexistuje */

    if ( !( r->ini_fp = ui_utils_file_open ( r->filename, "r" ) ) ) {
        ui_show_error ( "%s() - Can't open file '%s' to read: %s", __func__, r->filename, strerror ( errno ) );
        return EXIT_FAILURE;
    };

    char str_line [ CFG_MAX_LINE_LENGTH ];


    int flag_in_my_module = 0;
    int flag_ignore_long_line = 0;
    unsigned found_elements = 0;

    while ( fgets ( str_line, sizeof ( str_line ), r->ini_fp ) ) {

        unsigned str_length = strlen ( str_line );

        if ( ( str_length == sizeof ( str_line ) - 1 ) && ( str_line [ str_length - 1 ] >= 0x20 ) ) {

            if ( ( flag_ignore_long_line == 0 ) && ( flag_in_my_module == 1 ) ) {
                /* Jsme na zacatku dlouheho radku - zjistime, zda na nem nezacina dalsi modul */

                char *str0 = cfgcommon_search_first_char ( str_line, str_length );
                if ( *str0 == '[' ) break; /* Koncime! zacina tu dalsi modul */
            };
            flag_ignore_long_line = 1;

        } else if ( flag_ignore_long_line == 1 ) {

            flag_ignore_long_line = 0;

        } else {

            while ( ( str_length > 0 ) && ( str_line[ str_length - 1 ] <= 0x20 ) ) {
                str_line[ str_length - 1 ] = 0x00;
                str_length--;
            };

            if ( str_length > 0 ) {
                char *word0 = cfgcommon_search_first_char ( str_line, str_length );

                if ( flag_in_my_module == 0 ) {

                    /* Hledame pozadovany modul */

                    if ( *word0 == '[' ) {
                        word0++;

                        unsigned str_length = strlen ( word0 );
                        if ( ( str_length > 0 ) && ( word0[ str_length - 1 ] == ']' ) ) {
                            word0[ str_length - 1 ] = 0x00;
                            str_length--;

                            if ( 0 == strcasecmp ( word0, m->name ) ) {
                                //printf ( "Module [%s] FOUND!\n", m->name );
                                flag_in_my_module = 1;
                            };
                        };
                    };


                } else {

                    if ( *word0 == '[' ) break; /* Koncime! zacina tu dalsi modul */

                    char *element_name = word0;
                    /* Jiz jsme v pozadovanem modulu - provedeme analyzu celeho radku */
                    char *element_name_end = cfgcommon_search_word_end ( element_name, str_length );
                    int equating_found = 0;

                    char *element_valtxt = element_name_end;
                    if ( *element_name_end == '=' ) {
                        element_valtxt++;
                        equating_found = 1;
                    } else if ( *element_name_end != 0x00 ) {
                        element_valtxt++;
                    };
                    *element_name_end = 0x00;

                    str_length = strlen ( element_valtxt );

                    if ( equating_found != 1 ) {
                        element_valtxt = cfgcommon_search_first_char ( element_valtxt, str_length );
                        if ( *element_valtxt == '=' ) {
                            equating_found = 1;
                            element_valtxt++;
                            str_length = strlen ( element_valtxt );
                        };
                    };

                    /* Ignorujeme radky ve kterych za prvnim slovem nenasleduje '=' */
                    if ( equating_found == 1 ) {
                        element_valtxt = cfgcommon_search_first_char ( element_valtxt, str_length );
                        //printf ( "key: '%s', value: '%s'\n", element_name, element_valtxt );
                        st_CFGELEMENT *e = cfgmodule_get_element_by_name ( m, element_name );
                        if ( e != NULL ) {

                            found_elements++;

                            if ( e->type == CFGENTYPE_TEXT ) {

                                if ( strlen ( element_valtxt ) ) {
                                    cfgelement_set_text_value ( e, element_valtxt );
                                };

                            } else if ( e->type == CFGENTYPE_BOOL ) {

                                if ( *element_valtxt == '0' ) {
                                    cfgelement_set_bool_value ( e, 0 );
                                } else if ( *element_valtxt == '1' ) {
                                    cfgelement_set_bool_value ( e, 1 );
                                };

                            } else if ( e->type == CFGENTYPE_KEYWORD ) {

                                int value = cfgelement_property_get_value_by_keyword ( e->pkw, element_valtxt );
                                if ( value != -1 ) {
                                    cfgelement_set_keyword_value ( e, value );
                                };

                            } else if ( e->type == CFGENTYPE_UNSIGNED ) {

                                /* najdeme zacatek ascii hex cisla */
                                if ( strncmp ( element_valtxt, "0x", 2 ) ) {
                                    element_valtxt += 2;
                                } else if ( strncmp ( element_valtxt, "0X", 2 ) ) {
                                    element_valtxt += 2;
                                } else if ( *element_valtxt == '#' ) {
                                    element_valtxt++;
                                };

                                unsigned length = strlen ( element_valtxt );
                                unsigned value = 0;
                                int bitpos = 0;

                                while ( length ) {

                                    char c = element_valtxt [ length - 1 ];
                                    unsigned position_value;

                                    if ( c >= '0' && c <= '9' ) {
                                        position_value = c - '0';
                                    } else if ( c >= 'a' && c <= 'f' ) {
                                        position_value = c - 'a' + 0x0a;
                                    } else if ( c >= 'A' && c <= 'F' ) {
                                        position_value = c - 'A' + 0x0a;
                                    } else {
                                        break;
                                    };

                                    value += position_value << bitpos;
                                    length--;
                                    bitpos += 4;
                                };

                                if ( -1 != cfgelement_property_test_unsigned_value ( e->ppu, value ) ) {
                                    cfgelement_set_unsigned_value ( e, value );
                                };
                            };
                        };
                    };
                };
            };
        };
    };

    fclose ( r->ini_fp );

    return EXIT_SUCCESS;
}

