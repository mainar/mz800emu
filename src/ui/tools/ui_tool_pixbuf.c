/* 
 * File:   ui_tool_pixbuf.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 12. prosince 2017, 10:26
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

#include <string.h>
#include "ui_tool_pixbuf.h"


void ui_tool_pixbuf_put_pixel ( GdkPixbuf *pixbuf, guint x, guint y, uint32_t argb ) {

    gint width;
    gint height;
    gint rowstride;
    gint n_channels;
    guchar *pixels, *p;
    gboolean has_alpha;

    guchar alpha = (guchar) ( ( argb >> 24 ) & 0xff );
    guchar red = (guchar) ( ( argb >> 16 ) & 0xff );
    guchar green = (guchar) ( ( argb >> 8 ) & 0xff );
    guchar blue = (guchar) ( argb & 0xff );

    n_channels = gdk_pixbuf_get_n_channels ( pixbuf );
    has_alpha = gdk_pixbuf_get_has_alpha ( pixbuf );

    g_assert ( gdk_pixbuf_get_colorspace ( pixbuf ) == GDK_COLORSPACE_RGB );
    g_assert ( gdk_pixbuf_get_bits_per_sample ( pixbuf ) == 8 );

    if ( has_alpha ) {
        g_assert ( n_channels == 4 );
    } else {
        g_assert ( n_channels == 3 );
    };

    width = gdk_pixbuf_get_width ( pixbuf );
    height = gdk_pixbuf_get_height ( pixbuf );

    g_assert ( x >= 0 && x < width );
    g_assert ( y >= 0 && y < height );

    rowstride = gdk_pixbuf_get_rowstride ( pixbuf );
    pixels = gdk_pixbuf_get_pixels ( pixbuf );

    p = pixels + y * rowstride + x * n_channels;

    p[0] = red;
    p[1] = green;
    p[2] = blue;

    if ( has_alpha ) {
        p[3] = alpha;
    };
}


void ui_tool_pixbuf_fill_box ( GdkPixbuf *pixbuf, guint x, guint y, guint box_width, guint box_height, uint32_t argb ) {

    gint width;
    gint height;
    gint rowstride;
    gint n_channels;
    guchar *pixels, *p, *pp;
    gboolean has_alpha;

    guchar alpha = (guchar) ( ( argb >> 24 ) & 0xff );
    guchar red = (guchar) ( ( argb >> 16 ) & 0xff );
    guchar green = (guchar) ( ( argb >> 8 ) & 0xff );
    guchar blue = (guchar) ( argb & 0xff );

    n_channels = gdk_pixbuf_get_n_channels ( pixbuf );
    has_alpha = gdk_pixbuf_get_has_alpha ( pixbuf );

    g_assert ( gdk_pixbuf_get_colorspace ( pixbuf ) == GDK_COLORSPACE_RGB );
    g_assert ( gdk_pixbuf_get_bits_per_sample ( pixbuf ) == 8 );

    if ( has_alpha ) {
        g_assert ( n_channels == 4 );
    } else {
        g_assert ( n_channels == 3 );
    };

    width = gdk_pixbuf_get_width ( pixbuf );
    height = gdk_pixbuf_get_height ( pixbuf );

    g_assert ( x >= 0 && x < width );
    g_assert ( y >= 0 && y < height );
    g_assert ( ( x + box_width ) >= 1 && ( x + box_width ) <= width );
    g_assert ( ( y + box_height ) >= 1 && ( y + box_height ) <= height );

    rowstride = gdk_pixbuf_get_rowstride ( pixbuf );
    pixels = gdk_pixbuf_get_pixels ( pixbuf );

    p = pixels + y * rowstride + x * n_channels;

    p[0] = red;
    p[1] = green;
    p[2] = blue;

    if ( has_alpha ) {
        p[3] = alpha;
    };

    int i;
    for ( i = 1; i < box_width; i++ ) {
        memcpy ( &p[i * n_channels], p, n_channels );
    };

    for ( i = 1; i < box_height; i++ ) {
        pp = pixels + ( y + i ) * rowstride + x * n_channels;
        memcpy ( pp, p, n_channels * box_width );
    };
}


void ui_tool_pixbuf_fill ( GdkPixbuf *pixbuf, uint32_t argb ) {

    gint width;
    gint height;
    gint n_channels;
    guchar *pixels;
    gboolean has_alpha;

    guchar alpha = (guchar) ( ( argb >> 24 ) & 0xff );
    guchar red = (guchar) ( ( argb >> 16 ) & 0xff );
    guchar green = (guchar) ( ( argb >> 8 ) & 0xff );
    guchar blue = (guchar) ( argb & 0xff );

    n_channels = gdk_pixbuf_get_n_channels ( pixbuf );
    has_alpha = gdk_pixbuf_get_has_alpha ( pixbuf );

    g_assert ( gdk_pixbuf_get_colorspace ( pixbuf ) == GDK_COLORSPACE_RGB );
    g_assert ( gdk_pixbuf_get_bits_per_sample ( pixbuf ) == 8 );

    pixels = gdk_pixbuf_get_pixels ( pixbuf );

    pixels[0] = red;
    pixels[1] = green;
    pixels[2] = blue;

    if ( has_alpha ) {
        g_assert ( n_channels == 4 );
        pixels[3] = alpha;
    } else {
        g_assert ( n_channels == 3 );
    };

    width = gdk_pixbuf_get_width ( pixbuf );
    height = gdk_pixbuf_get_height ( pixbuf );

    int i;
    for ( i = 1; i < ( width * height ); i++ ) {
        memcpy ( &pixels[i * n_channels], pixels, n_channels );
    };

}


void ui_tool_pixbuf_create_horizontal_line ( GdkPixbuf *pixbuf, guint x, guint y, guint length, guint strength, uint32_t argb ) {
    ui_tool_pixbuf_fill_box ( pixbuf, x, y, length, strength, argb );
}


void ui_tool_pixbuf_create_horizontal_dashline ( GdkPixbuf *pixbuf, guint x, guint y, guint length, guint strength, guint dash1_length, guint dash2_length, uint32_t argb1, uint32_t argb2 ) {
    int i = 0;
    while ( i < length ) {

        int dash_length = ( ( i + dash1_length ) < length ) ? dash1_length : ( length - i );
        ui_tool_pixbuf_create_horizontal_line ( pixbuf, x + i, y, dash_length, strength, argb1 );
        i += dash_length;

        if ( i >= ( length - 1 ) ) {
            return;
        };

        dash_length = ( ( i + dash2_length ) < length ) ? dash2_length : ( length - i );
        ui_tool_pixbuf_create_horizontal_line ( pixbuf, x + i, y, dash_length, strength, argb2 );
        i += dash_length;
    };
}


void ui_tool_pixbuf_create_vertical_line ( GdkPixbuf *pixbuf, guint x, guint y, guint length, guint strength, uint32_t argb ) {
    ui_tool_pixbuf_fill_box ( pixbuf, x, y, strength, length, argb );
}


void ui_tool_pixbuf_create_vertical_dashline ( GdkPixbuf *pixbuf, guint x, guint y, guint length, guint strength, guint dash1_length, guint dash2_length, uint32_t argb1, uint32_t argb2 ) {

    int i = 0;
    while ( i < length ) {

        int dash_length = ( ( i + dash1_length ) < length ) ? dash1_length : ( length - i );
        ui_tool_pixbuf_create_horizontal_line ( pixbuf, x, y + i, dash_length, strength, argb1 );
        i += dash_length;

        if ( i >= ( length - 1 ) ) {
            return;
        };

        dash_length = ( ( i + dash2_length ) < length ) ? dash2_length : ( length - i );
        ui_tool_pixbuf_create_horizontal_line ( pixbuf, x, y + i, dash_length, strength, argb2 );
        i += dash_length;
    };
}
