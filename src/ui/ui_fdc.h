/* 
 * File:   ui_fdc.h
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 9. srpna 2015, 9:44
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

#ifndef UI_FDC_H
#define UI_FDC_H

#ifdef __cplusplus
extern "C" {
#endif


    extern void ui_fdc_set_dsk ( unsigned drive_id, char *dsk_filename );
    extern void ui_fdc_menu_update ( void );

#ifdef __cplusplus
}
#endif

#endif /* UI_FDC_H */

