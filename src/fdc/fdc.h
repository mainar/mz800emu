/* 
 * File:   fdc.h
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

#ifndef FDC_H
#define FDC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "wd279x.h"    

#define FDC_CONNECTED        1
#define FDC_DISCONNECTED     0


    typedef struct st_FDC {
        unsigned connected;
        st_WD279X wd279x;
    } st_FDC;

    extern st_FDC g_fdc;

    extern void fdc_init ( void );
    extern void fdc_reset ( void );
    extern void fdc_exit ( void );

    extern int fdc_read_byte ( int i_addroffset, uint8_t *io_data );
    extern int fdc_write_byte ( int i_addroffset, uint8_t *io_data );

    extern int fdc_get_interrupt_state ( void );

    extern void fdc_mount ( unsigned drive_id );
    extern void fdc_umount ( unsigned drive_id );

    extern void ui_fdc_set_dsk ( unsigned drive_id, char *dsk_filename );

    extern void fdc_mount_dskfile ( unsigned drive_id, char *filename );
    extern const char* fdc_get_dsk_filepath ( unsigned drive_id );

#ifdef __cplusplus
}
#endif

#endif /* FDC_H */

