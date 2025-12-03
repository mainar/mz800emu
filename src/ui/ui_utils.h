/* 
 * File:   ui_utils.h
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 20. září 2015, 22:06
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

#ifndef UI_UTILS_H
#define UI_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <glib.h>
#include <sys/types.h>

    //#include <gtk/gtk.h>


    /*
     * 
     * Obecne
     * 
     */
    extern const gchar* ui_utils_get_error_message ( void );
    extern char* ui_utils_strerror ( void );
    extern void ui_utils_mem_free ( void *ptr );

    void* ui_utils_mem_alloc_raw ( guint32 size, int initialize0, const char *func, int line );
#define ui_utils_mem_alloc(size) ui_utils_mem_alloc_raw ( size, 0, __func__, __LINE__ )
#define ui_utils_mem_alloc0(size) ui_utils_mem_alloc_raw ( size, 1, __func__, __LINE__ )
    extern void* ui_utils_mem_realloc_raw ( void *ptr, guint32 size, const char *func, int line );
#define ui_utils_mem_realloc(ptr,size) ui_utils_mem_realloc_raw ( ptr, size, __func__, __LINE__ )

    extern char* ui_utils_basename ( const char *file_name );

    /*
     * 
     * Prace se soubory
     * 
     */
    extern char* ui_utils_file_name_locale_from_utf8 ( const char *filename_in_utf8 );
    extern FILE* ui_utils_file_open ( const char *filename_in_utf8, const char *mode );
    extern void ui_utils_file_close ( FILE *fh );
    extern unsigned int ui_utils_file_read ( void *buffer, unsigned int size, unsigned int count_bytes, FILE *fh );
    extern unsigned int ui_utils_file_write ( void *buffer, unsigned int size, unsigned int count_bytes, FILE *fh );
    extern int ui_utils_file_access ( const char *filename_in_utf8, int type );
    extern int ui_utils_file_rename ( char *path, char *oldname, char *newname );
    extern int ui_utils_file_truncate ( const char *filename_in_utf8, off_t length );

    /*
     * 
     * Adresarove funkce
     * 
     */
    typedef GDir UI_UTILS_DIR_HANDLE;
    typedef const char UI_UTILS_DIR_ITEM;

    extern UI_UTILS_DIR_HANDLE* ui_utils_dir_open ( char *path );
    extern void ui_utils_dir_close ( UI_UTILS_DIR_HANDLE *dh );

    extern UI_UTILS_DIR_ITEM* ui_utils_dir_read ( UI_UTILS_DIR_HANDLE *dh );

    extern unsigned ui_utils_ditem_is_file ( char *path, UI_UTILS_DIR_ITEM *ditem );
    extern const gchar* ui_utils_ditem_get_filepath ( const char *path, UI_UTILS_DIR_ITEM *ditem );

    extern const gchar* ui_utils_ditem_get_name ( UI_UTILS_DIR_ITEM *ditem );


#define ui_utils_build_filepath( path, filename ) ui_utils_ditem_get_filepath ( path, filename )
#define ui_utils_free_filepath( filepath ) ui_utils_mem_free ( (void *) filepath )

#ifdef __cplusplus
}
#endif

#endif /* UI_UTILS_H */

