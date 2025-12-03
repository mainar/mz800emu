/* 
 * File:   ui_tool_pixbuf.h
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 12. prosince 2017, 10:28
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


#ifndef UI_TOOL_PIXBUF_H
#define UI_TOOL_PIXBUF_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <gtk/gtk.h>

    extern void ui_tool_pixbuf_put_pixel ( GdkPixbuf *pixbuf, guint x, guint y, uint32_t argb );

    extern void ui_tool_pixbuf_fill ( GdkPixbuf *pixbuf, uint32_t argb );
    extern void ui_tool_pixbuf_fill_box ( GdkPixbuf *pixbuf, guint x, guint y, guint box_width, guint box_height, uint32_t argb );

    extern void ui_tool_pixbuf_create_horizontal_line ( GdkPixbuf *pixbuf, guint x, guint y, guint length, guint strength, uint32_t argb );
    extern void ui_tool_pixbuf_create_vertical_line ( GdkPixbuf *pixbuf, guint x, guint y, guint length, guint strength, uint32_t argb );

    extern void ui_tool_pixbuf_create_horizontal_dashline ( GdkPixbuf *pixbuf, guint x, guint y, guint length, guint strength, guint dash1_length, guint dash2_length, uint32_t argb1, uint32_t argb2 );
    extern void ui_tool_pixbuf_create_vertical_dashline ( GdkPixbuf *pixbuf, guint x, guint y, guint length, guint strength, guint dash1_length, guint dash2_length, uint32_t argb1, uint32_t argb2 );

#ifdef __cplusplus
}
#endif

#endif /* UI_TOOL_PIXBUF_H */

