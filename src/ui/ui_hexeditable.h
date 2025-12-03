/* 
 * File:   ui_hexeditable.h
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 31. prosince 2017, 11:27
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


#ifndef UI_HEXEDITABLE_H
#define UI_HEXEDITABLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ui_main.h"

    extern void ui_hexeditable_changed ( GtkEditable *ed, gpointer user_data );
    extern void ui_digiteditable_changed ( GtkEditable *ed, gpointer user_data );

#ifdef __cplusplus
}
#endif

#endif /* UI_HEXEDITABLE_H */

