/* 
 * File:   ui_qdisk.h
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 11. února 2016, 21:31
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

#ifndef UI_QDISK_H
#define	UI_QDISK_H

#ifdef	__cplusplus
extern "C" {
#endif

#define UI_QDISK_NEW_IMAGE_NAME "new_qd_image"
    
    extern void ui_qdisk_set_path ( char *path );
    extern void ui_qdisk_menu_update ( void );
    
#ifdef	__cplusplus
}
#endif

#endif	/* UI_QDISK_H */

