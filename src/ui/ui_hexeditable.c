/* 
 * File:   ui_hexeditable.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 31. prosince 2017, 11:26
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

#include "ui_hexeditable.h"


G_MODULE_EXPORT void ui_hexeditable_changed ( GtkEditable *ed, gpointer user_data ) {

    gchar *text;
    gchar output[9];
    gint position;
    gint changed = 0;
    gint i, j = 0;

    text = gtk_editable_get_chars ( ed, 0, -1 );

    if ( text [ 0 ] == 0x00 ) {
        g_free ( text );
        return;
    };

    for ( i = 0; i < sizeof ( output ); i++ ) {
        if ( ( text [ i ] >= '0' && text [ i ] <= '9' ) ||
             ( text [ i ] >= 'a' && text [ i ] <= 'f' ) ||
             ( text [ i ] >= 'A' && text [ i ] <= 'F' ) ) {

            if ( text [ i ] >= 'a' && text [ i ] <= 'f' ) {
                output [ j ] = text [ i ] - 0x20;
                changed++;
            } else {
                output [ j ] = text [ i ];
            };
            j++;
        } else if ( text [ i ] == 0x00 ) {
            break;
        } else {
            changed++;
        };
    };
    output [ j ] = 0x00;
    /*    
        printf ( "o: %s\nn: %s\n\n", text, output );
     */

    if ( changed ) {
        gtk_editable_delete_text ( ed, 0, -1 );
        gtk_editable_insert_text ( ed, output, j, &position );
    };
    g_free ( text );
}


/**
 * Nefunguje mi spravne GTK_INPUT_PURPOSE_DIGITS (?)
 * 
 * 
 */
G_MODULE_EXPORT void ui_digiteditable_changed ( GtkEditable *ed, gpointer user_data ) {

    gchar *text;
    gchar output[9];
    gint position;
    gint changed = 0;
    gint i, j = 0;

    text = gtk_editable_get_chars ( ed, 0, -1 );

    if ( text [ 0 ] == 0x00 ) {
        g_free ( text );
        return;
    };

    for ( i = 0; i < sizeof ( output ); i++ ) {
        if ( text [ i ] >= '0' && text [ i ] <= '9' ) {
            output [ j ] = text [ i ];
            j++;
        } else if ( text [ i ] == 0x00 ) {
            break;
        } else {
            changed++;
        };
    };
    output [ j ] = 0x00;
    /*    
        printf ( "o: %s\nn: %s\n\n", text, output );
     */

    if ( changed ) {
        gtk_editable_delete_text ( ed, 0, -1 );
        gtk_editable_insert_text ( ed, output, j, &position );
    };
    g_free ( text );
}
