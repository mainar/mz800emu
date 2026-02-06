/* 
 * File:   ui_cmt.h
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 11. srpna 2015, 19:41
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

#ifndef UI_CMT_H
#define UI_CMT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <glib.h>

// This enum is used by non-UI code, so keep it outside #ifdef
typedef enum en_UICMT_SHOW {
    UICMT_SHOW_PLAY_TIME = 0,
    UICMT_SHOW_REMAINING_TIME,
} en_UICMT_SHOW;

#define UI_CMT_RECORDING_MAX_TIME_IN_SEC    (99 * 60)

#ifdef ENABLE_UI

    extern void ui_cmt_init ( void );
    extern void ui_cmt_window_show_hide ( void );
    extern void ui_cmt_hack_menu_update ( void );
    extern void ui_cmt_cpu_boost_menu_update ( void );
    extern void ui_cmt_mzfsize_check_menu_update ( void );
    extern void ui_cmt_window_update ( void );
    extern void ui_cmt_update_player ( void );
    extern void ui_cmt_set_filename ( char *filename );
    extern void ui_cmt_tape_window_hide ( void );
    extern void ui_cmt_tape_update_filelist ( void );
    extern void ui_cmt_set_show_time ( en_UICMT_SHOW show_time );
    extern void ui_cmt_check_mzf_filesize ( char *filename, int valid_size );
    extern void ui_cmt_main_window_set_sensitive ( gboolean sensitive );

#else
    // Stub implementations when UI is disabled
    static inline void ui_cmt_init(void) {}
    static inline void ui_cmt_window_show_hide(void) {}
    static inline void ui_cmt_hack_menu_update(void) {}
    static inline void ui_cmt_cpu_boost_menu_update(void) {}
    static inline void ui_cmt_mzfsize_check_menu_update(void) {}
    static inline void ui_cmt_window_update(void) {}
    static inline void ui_cmt_update_player(void) {}
    static inline void ui_cmt_set_filename(char *filename) {}
    static inline void ui_cmt_tape_window_hide(void) {}
    static inline void ui_cmt_tape_update_filelist(void) {}
    static inline void ui_cmt_set_show_time(en_UICMT_SHOW show_time) {}
    static inline void ui_cmt_check_mzf_filesize(char *filename, int valid_size) {}
    static inline void ui_cmt_main_window_set_sensitive(gboolean sensitive) {}

#endif

#ifdef __cplusplus
}
#endif

#endif /* UI_CMT_H */

