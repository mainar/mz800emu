/* 
 * File:   ui_fcbutton.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 14. března 2016, 15:27
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

#include <gtk/gtk.h>
#include <string.h>

#include "ui_fcbutton.h"


st_UI_FCBUTTON* ui_fcbutton_new ( void ) {

    st_UI_FCBUTTON *fcb = g_malloc0 ( sizeof ( st_UI_FCBUTTON ) );
    fcb->filepath = g_malloc0 ( 1 );

    fcb->button = gtk_button_new ( );
    gtk_widget_show ( fcb->button );

    fcb->box = gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
    gtk_widget_show ( fcb->box );
    gtk_container_add ( GTK_CONTAINER ( fcb->button ), fcb->box );

    fcb->label = gtk_label_new ( "(none)" );
    gtk_widget_show ( fcb->label );
#ifdef WINDOWS
    gtk_misc_set_alignment ( GTK_MISC ( fcb->label ), 0, 0.5 );
#else
    gtk_label_set_xalign ( GTK_LABEL ( fcb->label ), 0 );
    gtk_label_set_yalign ( GTK_LABEL ( fcb->label ), 0.5 );
#endif
    gtk_box_pack_start ( GTK_BOX ( fcb->box ), fcb->label, TRUE, TRUE, 0 );

    fcb->separator = gtk_separator_new ( GTK_ORIENTATION_VERTICAL );
    gtk_widget_show ( fcb->separator );
    gtk_box_pack_start ( GTK_BOX ( fcb->box ), fcb->separator, FALSE, FALSE, 0 );
    gtk_widget_set_size_request ( fcb->separator, 10, -1 );

#ifdef WINDOWS
    // ..._from_stock je deprecated, nicmene ..._from_icon_name mi nefunguje ve verzi pod win32
    fcb->image = gtk_image_new_from_stock ( "gtk-open", GTK_ICON_SIZE_BUTTON );
#else
    fcb->image = gtk_image_new_from_icon_name ( "gtk-open", GTK_ICON_SIZE_BUTTON );
#endif
    gtk_widget_show ( fcb->image );
    gtk_box_pack_start ( GTK_BOX ( fcb->box ), fcb->image, FALSE, FALSE, 0 );

    return fcb;
}


void ui_fcbutton_destroy ( st_UI_FCBUTTON *fcb ) {
    if ( NULL != fcb->filepath ) {
        g_free ( fcb->filepath );
    };
    g_free ( fcb );
}


void ui_fcbutton_set_filepath ( st_UI_FCBUTTON *fcb, gchar *filepath ) {

    guint len = strlen ( filepath );
    fcb->filepath = g_realloc ( fcb->filepath, len + 1 );
    strcpy ( fcb->filepath, filepath );

    if ( len ) {
        gchar *txt = g_path_get_basename ( filepath );
        gtk_label_set_text ( (GtkLabel*) fcb->label, txt );
        g_free ( txt );
    } else {
        gtk_label_set_text ( (GtkLabel*) fcb->label, "(none)" );
    };
}

#if 0


gchar* ui_fcbutton_get_filepath ( st_UI_FCBUTTON *fcb ) {
    gchar *ptr = g_malloc ( strlen ( fcb->filepath ) );
    strcpy ( ptr, fcb->filepath );
    return ptr;
}
#else


const gchar* ui_fcbutton_get_filepath ( st_UI_FCBUTTON *fcb ) {
    return fcb->filepath;
}
#endif

