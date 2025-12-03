/* 
 * File:   ui_ramdisk.h
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 10. srpna 2015, 20:45
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

#ifndef UI_RAMDISK_H
#define	UI_RAMDISK_H

#ifdef	__cplusplus
extern "C" {
#endif


    extern void ui_ramdisk_update_menu ( void );

#ifdef	__cplusplus
}
#endif

#endif	/* UI_RAMDISK_H */

   
//    int i;
//    for ( i = 0; i < FILETYPES_COUNT; i++ ) {
//        strcpy ( g_ui.last_dir[i], "./" );
//    };
//    g_ui.last_filetype = 0;


//            strncpy ( g_ui.last_dir[filetype], (char*) gtk_file_chooser_get_current_folder ( GTK_FILE_CHOOSER ( filechooserdialog ) ), sizeof ( UI_FILENAME_LENGTH ) );
//            g_ui.last_filetype = filetype;
