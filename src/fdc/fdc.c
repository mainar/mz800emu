/* 
 * File:   fdc.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 6. srpna 2015, 17:31
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

#include <stdint.h>
#include <string.h>

#include "wd279x.h"
#include "mz800.h"
#include "z80ex/include/z80ex.h"
#include "fdc.h"

#include "ui/ui_main.h"
#include "ui/ui_file_chooser.h"
#include "ui/ui_fdc.h"
#include "ui/ui_utils.h"

#include "cfgmain.h"


st_FDC g_fdc;


void fdc_reset ( void ) {
    wd279x_reset ( &g_fdc.wd279x );
    mz800_interrupt_manager ( );
}


void fdc_propagatecfg_connected ( void *e, void *data ) {
    g_fdc.connected = cfgelement_get_bool_value ( (CFGELM *) e );
    ui_fdc_menu_update ( );
}


void fdc_mount_dskfile ( unsigned drive_id, char *filename ) {
    if ( filename[0] != 0x00 ) {
        //        if ( WD279X_RET_OK != wd279x_open_dsk ( &g_fdc.wd279x, drive_id, filename ) ) {
        //            ui_show_error ( "Can't open DSK file '%s': %s", filename, strerror ( errno ) );
        //        };
        wd279x_open_dsk ( &g_fdc.wd279x, drive_id, filename );
    };
    ui_fdc_set_dsk ( drive_id, g_fdc.wd279x.drive[drive_id].filename );
}


void fdc_propagatecfg_fdd ( void *e, void *data ) {
    fdc_mount_dskfile ( (unsigned) ( (char*) data )[0], cfgelement_get_text_value ( (CFGELM *) e ) );
}


const char* fdc_get_dsk_filepath ( unsigned drive_id ) {
    if ( drive_id <= 3 ) return g_fdc.wd279x.drive[drive_id].filename;
    return NULL;
}


void fdc_init ( void ) {

    wd279x_init ( &g_fdc.wd279x, "Master FDC" );
    g_fdc.wd279x.mask = WD279X_MASK_NONE;

    int i;
    for ( i = 0; i < 4; i++ ) {
        fdc_umount ( i );
    };

    CFGMOD *cmod = cfgroot_register_new_module ( g_cfgmain, "FDC" );

    CFGELM *elm;
    elm = cfgmodule_register_new_element ( cmod, "wd279x_pluged", CFGENTYPE_BOOL, FDC_CONNECTED );
    cfgelement_set_propagate_cb ( elm, fdc_propagatecfg_connected, NULL );
    cfgelement_set_handlers ( elm, NULL, (void*) &g_fdc.connected );

    elm = cfgmodule_register_new_element ( cmod, "wd279x_fdd0_dskpath", CFGENTYPE_TEXT, "" );
    cfgelement_set_propagate_cb ( elm, fdc_propagatecfg_fdd, (void*) "\x00" );
    cfgelement_set_handlers ( elm, NULL, (void*) &g_fdc.wd279x.drive[0].filename );

    elm = cfgmodule_register_new_element ( cmod, "wd279x_fdd1_dskpath", CFGENTYPE_TEXT, "" );
    cfgelement_set_propagate_cb ( elm, fdc_propagatecfg_fdd, (void*) "\x01" );
    cfgelement_set_handlers ( elm, NULL, (void*) &g_fdc.wd279x.drive[1].filename );

    elm = cfgmodule_register_new_element ( cmod, "wd279x_fdd2_dskpath", CFGENTYPE_TEXT, "" );
    cfgelement_set_propagate_cb ( elm, fdc_propagatecfg_fdd, (void*) "\x02" );
    cfgelement_set_handlers ( elm, NULL, (void*) &g_fdc.wd279x.drive[2].filename );

    elm = cfgmodule_register_new_element ( cmod, "wd279x_fdd3_dskpath", CFGENTYPE_TEXT, "" );
    cfgelement_set_propagate_cb ( elm, fdc_propagatecfg_fdd, (void*) "\x03" );
    cfgelement_set_handlers ( elm, NULL, (void*) &g_fdc.wd279x.drive[3].filename );

    cfgmodule_parse ( cmod );
    cfgmodule_propagate ( cmod );

    ui_fdc_menu_update ( );
}


/* Pri ukonceni je potreba uzavrit vsechny otevrene DSK */
void fdc_exit ( void ) {
    int i;
    for ( i = 0; i < 4; i++ ) {
        fdc_umount ( i );
    };
}


void fdc_mount ( unsigned drive_id ) {

    if ( !g_fdc.connected ) {
        ui_show_warning ( "Can't mount DSK image into FD%d, because FD controller is not connected.", drive_id );
        return;
    };

    char *filename = ui_file_chooser_open_dsk ( g_fdc.wd279x.drive[drive_id].filename );
    if ( filename ) {
        if ( strlen ( filename ) < DSK_FILENAME_LENGTH ) {
            fdc_mount_dskfile ( drive_id, filename );
        } else {
            ui_show_warning ( "Sorry, filepath is too big (%d).", strlen ( filename ) );
        };
        ui_utils_mem_free ( filename );
    } else {
        fdc_mount_dskfile ( drive_id, "" );
    };
}


void fdc_umount ( unsigned drive_id ) {
    wd279x_close_dsk ( &g_fdc.wd279x, drive_id );
    ui_fdc_set_dsk ( drive_id, "\x00" );
}


int fdc_read_byte ( int i_addroffset, uint8_t *io_data ) {
    unsigned char read_byte;
    int retval = wd279x_read_byte ( &g_fdc.wd279x, i_addroffset, &read_byte );
    *io_data = read_byte;
    mz800_interrupt_manager ( );
    return retval;
}


int fdc_write_byte ( int i_addroffset, uint8_t *io_data ) {
    int retval = wd279x_write_byte ( &g_fdc.wd279x, i_addroffset, io_data );
    mz800_interrupt_manager ( );
    return retval;
}


int fdc_get_interrupt_state ( void ) {
    return wd279x_check_interrupt ( &g_fdc.wd279x );
}
