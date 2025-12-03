/* 
 * File:   ui_joy.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 4. února 2018, 8:22
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

#include "ui_main.h"
#include "ui_joy.h"
#include "src/joy/joy.h"
#include "src/iface_sdl/iface_sdl_joy.h"
#include "ui_utils.h"


typedef struct st_UI_JOY {
    gboolean ui_lock;
    int count_devices;
    st_IFACE_JOY_SYSDEVICE *sysdevices;
} st_UI_JOY;

st_UI_JOY g_ui_joy;


static void ui_joy_clean_sysdevices_alocation ( void ) {
    if ( g_ui_joy.sysdevices != NULL ) {
        int i;
        for ( i = 0; i < g_ui_joy.count_devices; i++ ) {
            ui_utils_mem_free ( g_ui_joy.sysdevices[i].name );
        };
        ui_utils_mem_free ( g_ui_joy.sysdevices );
        g_ui_joy.sysdevices = NULL;
    };
    g_ui_joy.count_devices = 0;
}


static void ui_joy_hide_window ( void ) {
    GtkWidget *window = ui_get_widget ( "window_joystick" );

    if ( !gtk_widget_is_visible ( window ) ) return;

    ui_joy_clean_sysdevices_alocation ( );

    gtk_widget_hide ( window );
}


static en_JOY_TYPE ui_joy_get_selected_type ( en_JOY_DEVID joy_devid ) {
    char comboname[] = "joyX_type_combobox";
    comboname[3] = '1' + joy_devid;
    en_JOY_TYPE dev_type = gtk_combo_box_get_active ( GTK_COMBO_BOX ( ui_get_widget ( comboname ) ) );
    return dev_type;
}


static void ui_joy_showhide_keypad_info ( en_JOY_DEVID joy_devid, gboolean show ) {

    char joycode = '1' + joy_devid;
    char kp_info_grid[] = "joyX_keypad_info_grid";
    kp_info_grid[3] = joycode;
    char kp_info1_box[] = "joyX_keypad_info1_box";
    kp_info1_box[3] = joycode;
    char kp_info2_box[] = "joyX_keypad_info2_box";
    kp_info2_box[3] = joycode;

    if ( show ) {
        gtk_widget_show ( ui_get_widget ( kp_info_grid ) );
        gtk_widget_show ( ui_get_widget ( kp_info1_box ) );
        gtk_widget_show ( ui_get_widget ( kp_info2_box ) );
    } else {
        gtk_widget_hide ( ui_get_widget ( kp_info_grid ) );
        gtk_widget_hide ( ui_get_widget ( kp_info1_box ) );
        gtk_widget_hide ( ui_get_widget ( kp_info2_box ) );
    };
}


static void ui_joy_showhide_realdev_setup ( en_JOY_DEVID joy_devid, gboolean show ) {

    char joycode = '1' + joy_devid;
    char device_comboboxtext[] = "joyX_device_comboboxtext";
    device_comboboxtext[3] = joycode;
    char device_setup_grid[] = "joyX_device_setup_grid";
    device_setup_grid[3] = joycode;

    if ( show ) {
        gtk_widget_show ( ui_get_widget ( device_comboboxtext ) );
        gtk_widget_show ( ui_get_widget ( device_setup_grid ) );
    } else {
        gtk_widget_hide ( ui_get_widget ( device_comboboxtext ) );
        gtk_widget_hide ( ui_get_widget ( device_setup_grid ) );
    };
}


static int ui_joy_get_selected_sysdevid ( en_JOY_DEVID joy_devid ) {
    char device_comboboxtext[] = "joyX_device_comboboxtext";
    device_comboboxtext[3] = '1' + joy_devid;
    int joy_sysdevid = gtk_combo_box_get_active ( GTK_COMBO_BOX ( ui_get_widget ( device_comboboxtext ) ) );
    if ( joy_sysdevid >= 0 ) {
        joy_sysdevid--;
    };
    return joy_sysdevid;
}


static void ui_joy_set_sysdev_cfg_limits ( en_JOY_DEVID joy_devid, st_IFACE_JOY_SYSDEVICE *joy_sysdevice ) {

    int axes = ( joy_sysdevice->axes > 1 ) ? ( joy_sysdevice->axes - 1 ) : 1;
    int buttons = ( joy_sysdevice->buttons > 1 ) ? ( joy_sysdevice->buttons - 1 ) : 1;

    char widget_name[100];
    widget_name[sizeof ( widget_name ) - 1] = 0x00;

    int widget_name_size = sizeof ( widget_name ) - 1;
    char joycode = '1' + joy_devid;

    snprintf ( widget_name, widget_name_size, "joy%c_horizontal_axis_adjustment", joycode );
    gtk_adjustment_set_upper ( ui_get_adjustment ( widget_name ), axes );

    snprintf ( widget_name, widget_name_size, "joy%c_vertical_axis_adjustment", joycode );
    gtk_adjustment_set_upper ( ui_get_adjustment ( widget_name ), axes );

    snprintf ( widget_name, widget_name_size, "joy%c_trig1_button_adjustment", joycode );
    gtk_adjustment_set_upper ( ui_get_adjustment ( widget_name ), buttons );

    snprintf ( widget_name, widget_name_size, "joy%c_trig2_button_adjustment", joycode );
    gtk_adjustment_set_upper ( ui_get_adjustment ( widget_name ), buttons );
}


static void ui_joy_get_sysdevice_params ( en_JOY_DEVID joy_devid, st_IFACE_JOY_PARAMS *params ) {

    char widget_name[100];
    widget_name[sizeof ( widget_name ) - 1] = 0x00;

    int widget_name_size = sizeof ( widget_name ) - 1;
    char joycode = '1' + joy_devid;

    snprintf ( widget_name, widget_name_size, "joy%c_horizontal_axis_spinbutton", joycode );
    params->x_axis = gtk_spin_button_get_value ( GTK_SPIN_BUTTON ( ui_get_widget ( widget_name ) ) );

    snprintf ( widget_name, widget_name_size, "joy%c_horizontal_center_spinbutton", joycode );
    params->x_center = (int16_t) gtk_spin_button_get_value ( GTK_SPIN_BUTTON ( ui_get_widget ( widget_name ) ) );

    snprintf ( widget_name, widget_name_size, "joy%c_vertical_axis_spinbutton", joycode );
    params->y_axis = gtk_spin_button_get_value ( GTK_SPIN_BUTTON ( ui_get_widget ( widget_name ) ) );

    snprintf ( widget_name, widget_name_size, "joy%c_vertical_center_spinbutton", joycode );
    params->y_center = (int16_t) gtk_spin_button_get_value ( GTK_SPIN_BUTTON ( ui_get_widget ( widget_name ) ) );

    snprintf ( widget_name, widget_name_size, "joy%c_trig1_button_spinbutton", joycode );
    params->trig1 = gtk_spin_button_get_value ( GTK_SPIN_BUTTON ( ui_get_widget ( widget_name ) ) );

    snprintf ( widget_name, widget_name_size, "joy%c_trig2_button_spinbutton", joycode );
    params->trig2 = gtk_spin_button_get_value ( GTK_SPIN_BUTTON ( ui_get_widget ( widget_name ) ) );

    snprintf ( widget_name, widget_name_size, "joy%c_horizontal_inverted_checkbutton", joycode );
    params->x_invert = gtk_toggle_button_get_active ( ui_get_toggle ( widget_name ) );

    snprintf ( widget_name, widget_name_size, "joy%c_vertical_inverted_checkbutton", joycode );
    params->y_invert = gtk_toggle_button_get_active ( ui_get_toggle ( widget_name ) );
}


static void ui_joy_update_joy_configuration ( en_JOY_DEVID joy_devid ) {

    en_JOY_TYPE dev_type = ui_joy_get_selected_type ( joy_devid );
    if ( dev_type == JOY_TYPE_JOYSTICK ) {
        int joy_sysdevid = ui_joy_get_selected_sysdevid ( joy_devid );
        if ( joy_sysdevid >= 0 ) {
            iface_sdl_joy_open_sysdevice ( joy_devid, &g_ui_joy.sysdevices[joy_sysdevid] );
            st_IFACE_JOY_PARAMS params;
            ui_joy_get_sysdevice_params ( joy_devid, &params );
            iface_sdl_joy_set_params ( joy_devid, &params );
        } else {


            dev_type = JOY_TYPE_NONE;
        };
    };

    g_joy.dev[joy_devid].type = dev_type;
}


static void ui_joy_set_sysdevice_calibration_params ( en_JOY_DEVID joy_devid, st_IFACE_JOY_PARAMS *params ) {

    char widget_name[100];
    widget_name[sizeof ( widget_name ) - 1] = 0x00;

    int widget_name_size = sizeof ( widget_name ) - 1;
    char joycode = '1' + joy_devid;

    snprintf ( widget_name, widget_name_size, "joy%c_horizontal_center_spinbutton", joycode );
    gtk_spin_button_set_value ( GTK_SPIN_BUTTON ( ui_get_widget ( widget_name ) ), params->x_center );

    snprintf ( widget_name, widget_name_size, "joy%c_vertical_center_spinbutton", joycode );
    gtk_spin_button_set_value ( GTK_SPIN_BUTTON ( ui_get_widget ( widget_name ) ), params->y_center );
}


void ui_joy_sysdevice_calibration ( en_JOY_DEVID joy_devid, st_IFACE_JOY_PARAMS *params ) {

    GtkWidget *window = ui_get_widget ( "window_joystick" );
    if ( !gtk_widget_is_visible ( window ) ) return;

    g_ui_joy.ui_lock = TRUE;
    ui_joy_set_sysdevice_calibration_params ( joy_devid, params );
    g_ui_joy.ui_lock = FALSE;
}


static void ui_joy_set_sysdevice_params ( en_JOY_DEVID joy_devid, st_IFACE_JOY_PARAMS *params ) {

    char widget_name[100];
    widget_name[sizeof ( widget_name ) - 1] = 0x00;

    int widget_name_size = sizeof ( widget_name ) - 1;
    char joycode = '1' + joy_devid;

    snprintf ( widget_name, widget_name_size, "joy%c_horizontal_axis_spinbutton", joycode );
    gtk_spin_button_set_value ( GTK_SPIN_BUTTON ( ui_get_widget ( widget_name ) ), params->x_axis );

    snprintf ( widget_name, widget_name_size, "joy%c_vertical_axis_spinbutton", joycode );
    gtk_spin_button_set_value ( GTK_SPIN_BUTTON ( ui_get_widget ( widget_name ) ), params->y_axis );

    snprintf ( widget_name, widget_name_size, "joy%c_trig1_button_spinbutton", joycode );
    gtk_spin_button_set_value ( GTK_SPIN_BUTTON ( ui_get_widget ( widget_name ) ), params->trig1 );

    snprintf ( widget_name, widget_name_size, "joy%c_trig2_button_spinbutton", joycode );
    gtk_spin_button_set_value ( GTK_SPIN_BUTTON ( ui_get_widget ( widget_name ) ), params->trig2 );

    snprintf ( widget_name, widget_name_size, "joy%c_horizontal_inverted_checkbutton", joycode );
    gtk_toggle_button_set_active ( ui_get_toggle ( widget_name ), params->x_invert );

    snprintf ( widget_name, widget_name_size, "joy%c_vertical_inverted_checkbutton", joycode );
    gtk_toggle_button_set_active ( ui_get_toggle ( widget_name ), params->y_invert );

    ui_joy_set_sysdevice_calibration_params ( joy_devid, params );
}


static void ui_joy_update_joy_type_elements ( en_JOY_DEVID joy_devid ) {

    en_JOY_TYPE dev_type = ui_joy_get_selected_type ( joy_devid );

    ui_joy_showhide_keypad_info ( joy_devid, ( dev_type == JOY_TYPE_NUM_KEYPAD ) );

    if ( dev_type == JOY_TYPE_JOYSTICK ) {
        int joy_sysdevid = ui_joy_get_selected_sysdevid ( joy_devid );
        gboolean sysdev_is_selected = ( joy_sysdevid >= 0 ) ? TRUE : FALSE;
        char device_setup_grid[] = "joyX_device_setup_grid";
        device_setup_grid[3] = '1' + joy_devid;
        gtk_widget_set_sensitive ( ui_get_widget ( device_setup_grid ), sysdev_is_selected );
        if ( sysdev_is_selected ) {
            ui_joy_set_sysdev_cfg_limits ( joy_devid, &g_ui_joy.sysdevices[joy_sysdevid] );
        };
    };

    ui_joy_showhide_realdev_setup ( joy_devid, ( dev_type == JOY_TYPE_JOYSTICK ) );
}


static void ui_joy_update_window_elements ( void ) {

    g_ui_joy.ui_lock = TRUE;

    gboolean initialize_subsystem = gtk_toggle_button_get_active ( ui_get_toggle ( "joy_initialize_subsystem_checkbutton" ) );
    gtk_widget_set_sensitive ( ui_get_widget ( "joy_rescan_devices_button" ), initialize_subsystem );
    GtkTreeModel *model = gtk_combo_box_get_model ( GTK_COMBO_BOX ( ui_get_widget ( "joy1_type_combobox" ) ) );
    GtkTreeIter iter;
    gtk_tree_model_get_iter_from_string ( GTK_TREE_MODEL ( model ), &iter, "2" );
    gtk_list_store_set ( GTK_LIST_STORE ( model ), &iter, 1, initialize_subsystem, -1 );

    en_JOY_DEVID joy_devid;
    for ( joy_devid = 0; joy_devid < JOY_DEVID_COUNT; joy_devid++ ) {

        if ( !initialize_subsystem ) {
            en_JOY_TYPE joy_type = ui_joy_get_selected_type ( joy_devid );
            if ( joy_type == JOY_TYPE_JOYSTICK ) {
                char comboname[] = "joyX_type_combobox";
                comboname[3] = '1' + joy_devid;
                gtk_combo_box_set_active ( GTK_COMBO_BOX ( ui_get_widget ( comboname ) ), JOY_TYPE_NONE );
            };
        };

        ui_joy_update_joy_type_elements ( joy_devid );
    };

    g_ui_joy.ui_lock = FALSE;
}


void static ui_joy_reload_device_comboboxtext_items ( void ) {

    char device_comboname[] = "joyX_device_comboboxtext";
    en_JOY_DEVID joy_devid;

    for ( joy_devid = 0; joy_devid < JOY_DEVID_COUNT; joy_devid++ ) {
        device_comboname[3] = '1' + joy_devid;
        gtk_combo_box_text_remove_all ( GTK_COMBO_BOX_TEXT ( ui_get_widget ( device_comboname ) ) );
    };

    g_ui_joy.count_devices = iface_sdl_joy_available_realdev_count ( );

    if ( g_ui_joy.count_devices > 0 ) {

        en_JOY_DEVID joy_devid;
        for ( joy_devid = 0; joy_devid < JOY_DEVID_COUNT; joy_devid++ ) {
            device_comboname[3] = '1' + joy_devid;
            gtk_combo_box_text_append_text ( GTK_COMBO_BOX_TEXT ( ui_get_widget ( device_comboname ) ), "Please select a device ..." );
        };

        if ( g_ui_joy.sysdevices != NULL ) {
            ui_joy_clean_sysdevices_alocation ( );
        };

        g_ui_joy.sysdevices = ui_utils_mem_alloc0 ( sizeof ( st_IFACE_JOY_SYSDEVICE ) * g_ui_joy.count_devices );

        int joy_sysdevid;
        for ( joy_sysdevid = 0; joy_sysdevid < g_ui_joy.count_devices; joy_sysdevid++ ) {

            st_IFACE_JOY_SYSDEVICE *sysdevice = &g_ui_joy.sysdevices[joy_sysdevid];

            iface_sdl_joy_get_realdev_info ( joy_sysdevid, sysdevice );

            char *full_device_name = ui_utils_mem_alloc ( ( joy_sysdevid / 10 ) + 1 + 2 + strlen ( sysdevice->name ) + 1 );

            sprintf ( full_device_name, "%d: %s", joy_sysdevid, sysdevice->name );

            for ( joy_devid = 0; joy_devid < JOY_DEVID_COUNT; joy_devid++ ) {
                device_comboname[3] = '1' + joy_devid;
                gtk_combo_box_text_append_text ( GTK_COMBO_BOX_TEXT ( ui_get_widget ( device_comboname ) ), full_device_name );
            };

            ui_utils_mem_free ( full_device_name );
        };

    } else {
        en_JOY_DEVID joy_devid;
        for ( joy_devid = 0; joy_devid < JOY_DEVID_COUNT; joy_devid++ ) {
            device_comboname[3] = '1' + joy_devid;
            gtk_combo_box_text_append_text ( GTK_COMBO_BOX_TEXT ( ui_get_widget ( device_comboname ) ), "No device found..." );
        };

        ui_joy_clean_sysdevices_alocation ( );
    };

    for ( joy_devid = 0; joy_devid < JOY_DEVID_COUNT; joy_devid++ ) {


        device_comboname[3] = '1' + joy_devid;
        gtk_combo_box_set_active ( ui_get_combo_box ( device_comboname ), 0 );
    };

}


void ui_joy_show_window ( void ) {

    GtkWidget *window = ui_get_widget ( "window_joystick" );
    if ( gtk_widget_is_visible ( window ) ) return;

    g_ui_joy.ui_lock = TRUE;
    g_ui_joy.count_devices = 0;
    g_ui_joy.sysdevices = NULL;

    static gboolean is_initialized = FALSE;
    if ( !is_initialized ) {
        is_initialized = TRUE;
#if LINUX
        gtk_widget_show ( ui_get_widget ( "joy_initialize_subsystem_slow_down_label" ) );
#else
        gtk_widget_hide ( ui_get_widget ( "joy_initialize_subsystem_slow_down_label" ) );
#endif
        en_JOY_DEVID joy_devid;
        for ( joy_devid = 0; joy_devid < JOY_DEVID_COUNT; joy_devid++ ) {
            char type_comboname[] = "joyX_type_combobox";
            type_comboname[3] = '1' + joy_devid;
            gtk_combo_box_set_active ( ui_get_combo_box ( type_comboname ), 0 );
            char device_comboname[] = "joyX_device_comboboxtext";
            device_comboname[3] = '1' + joy_devid;
            gtk_combo_box_set_active ( ui_get_combo_box ( device_comboname ), 0 );
        };
    };

    gtk_toggle_button_set_active ( ui_get_toggle ( "joy_initialize_subsystem_checkbutton" ), g_iface_joy.startup_joy_subsystem );

    ui_joy_reload_device_comboboxtext_items ( );

    en_JOY_DEVID joy_devid;
    for ( joy_devid = 0; joy_devid < JOY_DEVID_COUNT; joy_devid++ ) {

        char widget_name[100];
        widget_name[sizeof ( widget_name ) - 1] = 0x00;

        int widget_name_size = sizeof ( widget_name ) - 1;
        char joycode = '1' + joy_devid;

        st_IFACE_JOY_DEVICE *ifacejoydev = &g_iface_joy.dev[joy_devid];
        st_IFACE_JOY_SYSDEVICE *joy_sysdevice = &ifacejoydev->joy_sysdevice;
        st_IFACE_JOY_PARAMS *params = &ifacejoydev->params;

        snprintf ( widget_name, widget_name_size, "joy%c_type_combobox", joycode );
        gtk_combo_box_set_active ( ui_get_combo_box ( widget_name ), g_joy.dev[joy_devid].type );

        if ( joy_sysdevice->id < g_ui_joy.count_devices ) {
            snprintf ( widget_name, widget_name_size, "joy%c_device_comboboxtext", joycode );
            gtk_combo_box_set_active ( ui_get_combo_box ( widget_name ), joy_sysdevice->id + 1 );
            ui_joy_set_sysdev_cfg_limits ( joy_devid, joy_sysdevice );
            ui_joy_set_sysdevice_params ( joy_devid, params );
        };
    };


    g_ui_joy.ui_lock = FALSE;

    ui_joy_update_window_elements ( );

    gtk_widget_show ( window );
}


static void ui_joy_reset_to_defaults ( en_JOY_DEVID joy_devid ) {
    st_IFACE_JOY_PARAMS params;
    iface_sdl_joy_set_default_params ( &params );

    g_ui_joy.ui_lock = TRUE;
    ui_joy_set_sysdevice_params ( joy_devid, &params );
    g_ui_joy.ui_lock = FALSE;

    ui_joy_update_joy_configuration ( joy_devid );
}


/*
 * 
 * Calbacks
 * 
 */

G_MODULE_EXPORT gboolean on_window_joystick_delete_event ( GtkWidget *widget, GdkEvent *event, gpointer user_data ) {
    ui_joy_hide_window ( );
    return TRUE;
}


G_MODULE_EXPORT gboolean on_window_joystick_key_press_event ( GtkWidget *widget, GdkEventKey *event, gpointer user_data ) {
    if ( event->keyval == GDK_KEY_Escape ) {
        ui_joy_hide_window ( );
        return TRUE;
        //} else if ( event->keyval == GDK_KEY_Return ) {
        //gtk_button_clicked ( GTK_BUTTON ( ui_get_widget ( "button_dsk_new_ok" ) ) );

    };
    return FALSE;
}


G_MODULE_EXPORT void on_joy_close_button_clicked ( GtkButton *button, gpointer user_data ) {
    ui_joy_hide_window ( );
}


G_MODULE_EXPORT void on_joy_initialize_subsystem_checkbutton_toggled ( GtkToggleButton *togglebutton, gpointer user_data ) {

    if ( g_ui_joy.ui_lock ) return;

    gboolean new_state = gtk_toggle_button_get_active ( ui_get_toggle ( "joy_initialize_subsystem_checkbutton" ) );
    if ( g_iface_joy.startup_joy_subsystem == new_state ) return;

    g_iface_joy.startup_joy_subsystem = new_state;

    ui_joy_clean_sysdevices_alocation ( );

    if ( g_iface_joy.startup_joy_subsystem ) {
        iface_sdl_joy_subsystem_init ( );
    } else {
        iface_sdl_joy_subsystem_shutdown ( );
    };

    g_ui_joy.ui_lock = TRUE;
    ui_joy_reload_device_comboboxtext_items ( );
    g_ui_joy.ui_lock = FALSE;

    ui_joy_update_window_elements ( );

    en_JOY_DEVID joy_devid;
    for ( joy_devid = 0; joy_devid < JOY_DEVID_COUNT; joy_devid++ ) {
        ui_joy_update_joy_configuration ( joy_devid );
    };
}


G_MODULE_EXPORT void on_joy_rescan_devices_button_clicked ( GtkButton *button, gpointer user_data ) {

    ui_joy_clean_sysdevices_alocation ( );

    iface_sdl_joy_rescan_devices ( );

    g_ui_joy.ui_lock = TRUE;
    ui_joy_reload_device_comboboxtext_items ( );
    g_ui_joy.ui_lock = FALSE;

    ui_joy_update_window_elements ( );

    en_JOY_DEVID joy_devid;
    for ( joy_devid = 0; joy_devid < JOY_DEVID_COUNT; joy_devid++ ) {
        ui_joy_update_joy_configuration ( joy_devid );
    };
}


/*
 * CB - JOY1
 */

G_MODULE_EXPORT void on_joy1_type_combobox_changed ( GtkComboBox *combobox, gpointer user_data ) {
    if ( g_ui_joy.ui_lock ) return;
    ui_joy_update_window_elements ( );
    ui_joy_update_joy_configuration ( JOY_DEVID_0 );
}


G_MODULE_EXPORT void on_joy1_device_comboboxtext_changed ( GtkComboBox *combobox, gpointer user_data ) {
    if ( g_ui_joy.ui_lock ) return;
    ui_joy_update_window_elements ( );
    ui_joy_update_joy_configuration ( JOY_DEVID_0 );
}


G_MODULE_EXPORT void on_joy1_horizontal_axis_spinbutton_value_changed ( GtkSpinButton *spin_button, gpointer user_data ) {
    if ( g_ui_joy.ui_lock ) return;
    ui_joy_update_joy_configuration ( JOY_DEVID_0 );
}


G_MODULE_EXPORT void on_joy1_horizontal_center_spinbutton_value_changed ( GtkSpinButton *spin_button, gpointer user_data ) {
    if ( g_ui_joy.ui_lock ) return;
    ui_joy_update_joy_configuration ( JOY_DEVID_0 );
}


G_MODULE_EXPORT void on_joy1_vertical_axis_spinbutton_value_changed ( GtkSpinButton *spin_button, gpointer user_data ) {
    if ( g_ui_joy.ui_lock ) return;
    ui_joy_update_joy_configuration ( JOY_DEVID_0 );
}


G_MODULE_EXPORT void on_joy1_vertical_center_spinbutton_value_changed ( GtkSpinButton *spin_button, gpointer user_data ) {
    if ( g_ui_joy.ui_lock ) return;
    ui_joy_update_joy_configuration ( JOY_DEVID_0 );
}


G_MODULE_EXPORT void on_joy1_trig1_button_spinbutton_value_changed ( GtkSpinButton *spin_button, gpointer user_data ) {
    if ( g_ui_joy.ui_lock ) return;
    ui_joy_update_joy_configuration ( JOY_DEVID_0 );
}


G_MODULE_EXPORT void on_joy1_trig2_button_spinbutton_value_changed ( GtkSpinButton *spin_button, gpointer user_data ) {
    if ( g_ui_joy.ui_lock ) return;
    ui_joy_update_joy_configuration ( JOY_DEVID_0 );
}


G_MODULE_EXPORT void on_joy1_horizontal_inverted_checkbutton_toggled ( GtkToggleButton *togglebutton, gpointer user_data ) {
    if ( g_ui_joy.ui_lock ) return;
    ui_joy_update_joy_configuration ( JOY_DEVID_0 );
}


G_MODULE_EXPORT void on_joy1_vertical_inverted_checkbutton_toggled ( GtkToggleButton *togglebutton, gpointer user_data ) {
    if ( g_ui_joy.ui_lock ) return;
    ui_joy_update_joy_configuration ( JOY_DEVID_0 );
}


G_MODULE_EXPORT void on_joy1_reset_to_defaults_button_clicked ( GtkButton *button, gpointer user_data ) {
    ui_joy_reset_to_defaults ( JOY_DEVID_0 );
}


/*
 * CB - JOY2
 */

G_MODULE_EXPORT void on_joy2_type_combobox_changed ( GtkComboBox *combobox, gpointer user_data ) {
    if ( g_ui_joy.ui_lock ) return;
    ui_joy_update_window_elements ( );
    ui_joy_update_joy_configuration ( JOY_DEVID_1 );
}


G_MODULE_EXPORT void on_joy2_device_comboboxtext_changed ( GtkComboBox *combobox, gpointer user_data ) {
    if ( g_ui_joy.ui_lock ) return;
    ui_joy_update_window_elements ( );
    ui_joy_update_joy_configuration ( JOY_DEVID_1 );
}


G_MODULE_EXPORT void on_joy2_horizontal_axis_spinbutton_value_changed ( GtkSpinButton *spin_button, gpointer user_data ) {
    if ( g_ui_joy.ui_lock ) return;
    ui_joy_update_joy_configuration ( JOY_DEVID_1 );
}


G_MODULE_EXPORT void on_joy2_horizontal_center_spinbutton_value_changed ( GtkSpinButton *spin_button, gpointer user_data ) {
    if ( g_ui_joy.ui_lock ) return;
    ui_joy_update_joy_configuration ( JOY_DEVID_1 );
}


G_MODULE_EXPORT void on_joy2_vertical_axis_spinbutton_value_changed ( GtkSpinButton *spin_button, gpointer user_data ) {
    if ( g_ui_joy.ui_lock ) return;
    ui_joy_update_joy_configuration ( JOY_DEVID_1 );
}


G_MODULE_EXPORT void on_joy2_vertical_center_spinbutton_value_changed ( GtkSpinButton *spin_button, gpointer user_data ) {
    if ( g_ui_joy.ui_lock ) return;
    ui_joy_update_joy_configuration ( JOY_DEVID_1 );
}


G_MODULE_EXPORT void on_joy2_trig1_button_spinbutton_value_changed ( GtkSpinButton *spin_button, gpointer user_data ) {
    if ( g_ui_joy.ui_lock ) return;
    ui_joy_update_joy_configuration ( JOY_DEVID_1 );
}


G_MODULE_EXPORT void on_joy2_trig2_button_spinbutton_value_changed ( GtkSpinButton *spin_button, gpointer user_data ) {
    if ( g_ui_joy.ui_lock ) return;
    ui_joy_update_joy_configuration ( JOY_DEVID_1 );
}


G_MODULE_EXPORT void on_joy2_horizontal_inverted_checkbutton_toggled ( GtkToggleButton *togglebutton, gpointer user_data ) {
    if ( g_ui_joy.ui_lock ) return;
    ui_joy_update_joy_configuration ( JOY_DEVID_1 );
}


G_MODULE_EXPORT void on_joy2_vertical_inverted_checkbutton_toggled ( GtkToggleButton *togglebutton, gpointer user_data ) {
    if ( g_ui_joy.ui_lock ) return;
    ui_joy_update_joy_configuration ( JOY_DEVID_1 );
}


G_MODULE_EXPORT void on_joy2_reset_to_defaults_button_clicked ( GtkButton *button, gpointer user_data ) {
    ui_joy_reset_to_defaults ( JOY_DEVID_1 );
}
