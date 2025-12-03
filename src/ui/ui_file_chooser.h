/* 
 * File:   ui_file_chooser.h
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 26. června 2018, 8:12
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


#ifndef UI_FILE_CHOOSER_H
#define UI_FILE_CHOOSER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ui_main.h"


    typedef enum en_FC_MODE {
        FC_MODE_OPEN = 1,
        FC_MODE_SAVE = 2,
        FC_MODE_OPEN_OR_NEW = 3,
    } en_FC_MODE;


    typedef struct st_UI_FCS_FILTERS {
        int count;
        GtkFileFilter **filter;
    } st_UI_FCS_FILTERS;

    extern void ui_file_chooser_init ( void );
    extern void ui_file_chooser_exit ( void );

    char* ui_file_chooser_get_filename_extension ( char *filename );

    extern char* ui_file_chooser_open_file ( const char *predefined_filepath, const char *predefined_dirpath, const char *title, void *parent_window, en_FC_MODE fcmode, st_UI_FCS_FILTERS *filters );

    extern char* ui_file_chooser_open_cmt_file ( const char *predefined_filename );
    extern char* ui_file_chooser_open_cmthack_file ( const char *predefined_filename );
    extern char* ui_file_chooser_open_mzf ( const char *predefined_filename, void *parent_window );
    extern char* ui_file_chooser_open_mzf_to_save ( void );
    extern char* ui_file_chooser_open_wav_to_record ( void );
    extern char* ui_file_chooser_open_mzq ( const char *predefined_filename );
    extern char* ui_file_chooser_open_dsk ( const char *predefined_filename );
    extern char* ui_file_chooser_open_dir ( const char *predefined_filepath, const char *title );
    extern char* ui_file_chooser_open_qddir ( const char *predefined_filepath );
    extern char* ui_file_chooser_open_dat ( const char *predefined_filepath, const char *title, void *parent_window, en_FC_MODE fcmode );

    extern const char* ui_filechooser_get_last_mzf_dir ( void );
    extern void ui_filechooser_set_last_mzf_dir ( const char *dirpath );
    extern const char* ui_filechooser_get_last_dsk_dir ( void );
    extern void ui_filechooser_set_last_dsk_dir ( const char *dirpath );
    extern const char* ui_filechooser_get_last_mzq_dir ( void );
    extern void ui_filechooser_set_last_mzq_dir ( const char *dirpath );
    extern const char* ui_filechooser_get_last_generic_dir ( void );
    extern void ui_filechooser_set_last_generic_dir ( const char *dirpath );

    extern st_UI_FCS_FILTERS* ui_file_chooser_filters_new ( void );
    extern void ui_file_chooser_filters_add_filter ( st_UI_FCS_FILTERS *filters, GtkFileFilter *filter );
    extern GtkFileFilter* ui_file_chooser_create_filter_mzf ( void );

#ifdef __cplusplus
}
#endif

#endif /* UI_FILE_CHOOSER_H */

