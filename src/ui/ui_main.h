/* 
 * File:   ui_main.h
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 3. července 2015, 9:44
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

#ifndef UI_MAIN_H
#define UI_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mz800emu_cfg.h"

#include <gtk/gtk.h>

//#define OLD_GTK_RUNTIME

// po aktualizaci windows gtk runtime se jiz topmenu chova podobne jako v linuxu 
#ifndef OLD_GTK_RUNTIME
#define UI_TOPMENU_IS_WINDOW
#endif
    
#define UI_USE_ERRORLOG 

#ifdef UI_USE_ERRORLOG
#define UI_ERRORLOG_FILE    "error.log"
#endif    

#define LOCK_UICALLBACKS()         g_ui.calback_lock = 1
#define UNLOCK_UICALLBACKS()       g_ui.calback_lock = 0
#define TEST_UICALLBACKS_LOCKED    ( g_ui.calback_lock )

    //#define ui_get_object(name) gtk_builder_get_object ( g_ui.builder, name )
#define ui_get_object(name) ui_get_object_safely ( name )
#define ui_get_widget(name) GTK_WIDGET ( ui_get_object ( name ) )
#define ui_get_label(name) GTK_LABEL ( ui_get_object ( name ) )
#define ui_get_menu_item(name) GTK_MENU_ITEM ( ui_get_object ( name ) )
#define ui_get_check_menu_item(name) GTK_CHECK_MENU_ITEM ( ui_get_object ( name ) )
#define ui_get_combo_box(name) GTK_COMBO_BOX ( ui_get_object ( name ) )
#define ui_get_progress_bar(name) GTK_PROGRESS_BAR ( ui_get_object ( name ) )
#define ui_get_check_button(name) GTK_CHECK_BUTTON ( ui_get_object ( name ) )
#define ui_get_tree_view(name) GTK_TREE_VIEW ( ui_get_object ( name ) )
#define ui_get_range(name) GTK_RANGE ( ui_get_object ( name ) )
#define ui_get_window(name) GTK_WINDOW ( ui_get_object ( name ) )
#define ui_get_entry(name) GTK_ENTRY ( ui_get_object ( name ) )
#define ui_get_toggle(name) GTK_TOGGLE_BUTTON ( ui_get_object ( name ) )
#define ui_get_adjustment(name) GTK_ADJUSTMENT ( ui_get_object ( name ) )


    typedef struct st_UIWINPOS {
        gint x;
        gint y;
    } st_UIWINPOS;


    typedef struct st_UI {
        GtkBuilder *builder;
        unsigned calback_lock;
        st_UIWINPOS filebrowser_pos;
        unsigned disable_hotkeys;
    } st_UI;

    extern st_UI g_ui;


    extern GObject* ui_get_object_safely ( gchar *name );

    //extern void ui_init ( int argc, char *argv[] );
    extern void ui_init ( void );
    extern void ui_exit ( void );

    extern void ui_main_setpos ( st_UIWINPOS *wpos, gint x, gint y );
    extern void ui_main_win_move_to_pos ( GtkWindow *w, st_UIWINPOS *wpos );
    extern void ui_main_win_get_pos ( GtkWindow *w, st_UIWINPOS *wpos );

    extern void ui_iteration ( void );

#ifdef UI_TOPMENU_IS_WINDOW
    extern void ui_show_hide_main_menu_window ( void );
#else
    extern void ui_popup_main_menu ( void );
#endif
    extern void ui_hide_main_menu_window ( void );

    extern int ui_show_yesno_dialog ( char *format, ... );
    extern void ui_show_error ( char *format, ... );
    extern void ui_show_warning ( char *format, ... );

    extern void ui_main_update_cpu_speed_menu ( unsigned state );
    extern void ui_main_update_emulation_state ( unsigned state );

    extern void ui_main_update_rear_dip_switch_mz800_mode ( unsigned state );
    extern void ui_main_update_rear_dip_switch_cmt_inverted_polarity ( unsigned state );

    extern void ui_main_update_hwcompatibility_mz700_pal_timing ( unsigned state );
    extern void ui_main_update_hwcompatibility_mz700_fixed_e008 ( unsigned state );

#ifdef MZ800EMU_CFG_DEBUGGER_ENABLED
    extern void ui_main_debugger_windows_refresh ( void );
#endif

#ifdef __cplusplus
}
#endif

#endif /* UI_MAIN_H */

