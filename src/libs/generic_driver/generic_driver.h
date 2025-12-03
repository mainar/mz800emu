/* 
 * File:   generic_driver.h
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 19. února 2017, 9:24
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


#ifndef GENERIC_DRIVER_H
#define GENERIC_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


#define GENERIC_DRIVER_FILE
#define GENERIC_DRIVER_MEMORY

    //#define GENERIC_DRIVER_FILE_CB

    //#define GENERIC_DRIVER_MEMORY_CB
    //#define GENERIC_DRIVER_MEMORY_CB_USE_REALLOC


    typedef enum en_HANDLER_ERROR {
        HANDLER_ERROR_NONE = 0,
        HANDLER_ERROR_NOT_READY = 1,
        HANDLER_ERROR_WRITE_PROTECTED = 2,
        HANDLER_ERROR_USER
    } en_HANDLER_ERROR;


    typedef enum en_GENERIC_DRIVER_ERROR {
        GENERIC_DRIVER_ERROR_NONE = 0,
        GENERIC_DRIVER_ERROR_NOT_READY,
        GENERIC_DRIVER_ERROR_SEEK,
        GENERIC_DRIVER_ERROR_READ,
        GENERIC_DRIVER_ERROR_WRITE,
        GENERIC_DRIVER_ERROR_SIZE,
        GENERIC_DRIVER_ERROR_MALLOC,
        GENERIC_DRIVER_ERROR_REALLOC,
        GENERIC_DRIVER_ERROR_TRUNCATE,
        GENERIC_DRIVER_ERROR_HANDLER_TYPE,
        GENERIC_DRIVER_ERROR_HANDLER_IS_BUSY,
        GENERIC_DRIVER_ERROR_FOPEN,
        GENERIC_DRIVER_ERROR_CB_NOT_EXIST,
        GENERIC_DRIVER_ERROR_UNKNOWN
    } en_GENERIC_DRIVER_ERROR;


    typedef enum en_HANDLER_TYPE {
        HANDLER_TYPE_UNKNOWN = 0,
        HANDLER_TYPE_FILE,
        HANDLER_TYPE_MEMORY,
    } en_HANDLER_TYPE;


    typedef enum en_HANDLER_STATUS {
        HANDLER_STATUS_NOT_READY = 0,
        HANDLER_STATUS_READY = 1,
        HANDLER_STATUS_READ_ONLY = 2,
    } en_HANDLER_STATUS;


    typedef enum en_FILE_DRIVER_OPEN_MODE {
        FILE_DRIVER_OPMODE_RO = 0,
        FILE_DRIVER_OPMODE_RW,
        FILE_DRIVER_OPMODE_W,
    } en_FILE_DRIVER_OPEN_MODE;


    typedef struct st_HANDLER_MEMSPC {
        uint8_t *ptr;
        size_t open_size;
        size_t size;
        int swelling_enabled; // pokud ma nenulovou hodnotu, tak je povoleno "narustani" pri prepared
        int updated; // pokud probehla nejaka WR operace, tak ma nenulovou hodnotu
    } st_HANDLER_MEMSPC;


    typedef struct st_HANDLER_FILESPC {
        FILE *fh;
        en_FILE_DRIVER_OPEN_MODE open_mode;
        char *filename;
    } st_HANDLER_FILESPC;


    typedef union un_HANDLER_SPEC {
#ifdef GENERIC_DRIVER_FILE
        st_HANDLER_FILESPC filespec;
#endif
#ifdef GENERIC_DRIVER_MEMORY
        st_HANDLER_MEMSPC memspec;
#endif
    } un_HANDLER_SPEC;


    typedef struct st_HANDLER {
        en_HANDLER_TYPE type;
        en_HANDLER_STATUS status;
        en_HANDLER_ERROR err;
        un_HANDLER_SPEC spec;
        void *driver;
    } st_HANDLER;


    typedef int (*generic_driver_open_cb )( st_HANDLER *h );
    typedef int (*generic_driver_close_cb )( st_HANDLER *h );
    typedef int (*generic_driver_prepare_cb )( st_HANDLER *h, uint32_t offset, void **prepared_buffer, uint32_t count_bytes );
    typedef int (*generic_driver_read_cb )( st_HANDLER *h, uint32_t offset, void *buffer, uint32_t count_bytes, uint32_t *readlen );
    typedef int (*generic_driver_write_cb )( st_HANDLER *h, uint32_t offset, void *buffer, uint32_t count_bytes, uint32_t *writelen );
    typedef int (*generic_driver_truncate_cb )( st_HANDLER *h, uint32_t new_size );


    typedef struct st_DRIVER {
        generic_driver_open_cb open_cb;
        generic_driver_close_cb close_cb;
        generic_driver_read_cb read_cb;
        generic_driver_write_cb write_cb;
        generic_driver_prepare_cb prepare_cb;
        generic_driver_truncate_cb truncate_cb;
        en_GENERIC_DRIVER_ERROR err;
    } st_DRIVER;


    extern void generic_driver_setup ( st_DRIVER *d, generic_driver_open_cb opcb, generic_driver_close_cb clcb, generic_driver_read_cb rdcb, generic_driver_write_cb wrcb, generic_driver_prepare_cb prepcb, generic_driver_truncate_cb trunccb );
    extern const char* generic_driver_error_message ( st_HANDLER *h, st_DRIVER *d );
    extern void generic_driver_register_handler ( st_HANDLER *h, en_HANDLER_TYPE type );
    extern st_HANDLER* generic_driver_open_memory ( st_HANDLER *handler, st_DRIVER *d, uint32_t size );
    extern st_HANDLER* generic_driver_open_file ( st_HANDLER *handler, st_DRIVER *d, char *filename, en_FILE_DRIVER_OPEN_MODE open_mode );
    extern st_HANDLER* generic_driver_open_memory_from_file ( st_HANDLER *handler, st_DRIVER *d, char *filename );
    extern int generic_driver_save_memory ( st_HANDLER *h, char *filename );
    extern int generic_driver_close ( st_HANDLER *h );

    extern void generic_driver_set_handler_readonly_status ( st_HANDLER *h, int readonly );

    extern int generic_driver_prepare ( st_HANDLER *h, uint32_t offset, void **buffer, void *tmpbuffer, uint32_t size );
    extern int generic_driver_ppread ( st_HANDLER *h, uint32_t offset, void *buffer, uint32_t size );
    extern int generic_driver_ppwrite ( st_HANDLER *h, uint32_t offset, void *buffer, uint32_t size );

    extern int generic_driver_read ( st_HANDLER *h, uint32_t offset, void *buffer, uint32_t buffer_size );
    extern int generic_driver_write ( st_HANDLER *h, uint32_t offset, void *buffer, uint32_t buffer_size );
    extern int generic_driver_truncate ( st_HANDLER *h, uint32_t size );

    extern int generic_driver_direct_read ( st_HANDLER *h, uint32_t offset, void **buffer, void *work_buffer, uint32_t buffer_size );


    /* Preddefinovane callbacky */

#ifdef GENERIC_DRIVER_FILE_CB
    extern st_DRIVER* generic_driver_file_init ( st_DRIVER *d );
#endif


#ifdef GENERIC_DRIVER_MEMORY_CB
    extern st_DRIVER* generic_driver_memory_init ( st_DRIVER *d );
#endif


    static inline int generic_driver_memory_operation_internal_bootstrap ( st_HANDLER *h ) {

        st_HANDLER_MEMSPC *memspec = &h->spec.memspec;
        st_DRIVER *d = (st_DRIVER *) h->driver;

        h->err = HANDLER_ERROR_NONE;
        d->err = GENERIC_DRIVER_ERROR_NONE;

        if ( h->type != HANDLER_TYPE_MEMORY ) {
            d->err = GENERIC_DRIVER_ERROR_HANDLER_TYPE;
            return EXIT_FAILURE;
        };

        if ( !( h->status & HANDLER_STATUS_READY ) ) {
            h->err = HANDLER_ERROR_NOT_READY;
            return EXIT_FAILURE;
        };

        if ( memspec->ptr == NULL ) {
            d->err = GENERIC_DRIVER_ERROR_NOT_READY;
            return EXIT_FAILURE;
        };

        return EXIT_SUCCESS;
    }


    static inline int generic_driver_file_operation_internal_bootstrap ( st_HANDLER *h ) {

        st_HANDLER_FILESPC *filespec = &h->spec.filespec;
        st_DRIVER *d = (st_DRIVER *) h->driver;

        h->err = HANDLER_ERROR_NONE;
        d->err = GENERIC_DRIVER_ERROR_NONE;

        if ( h->type != HANDLER_TYPE_FILE ) {
            d->err = GENERIC_DRIVER_ERROR_HANDLER_TYPE;
            return EXIT_FAILURE;
        };

        if ( !( h->status & HANDLER_STATUS_READY ) ) {
            h->err = HANDLER_ERROR_NOT_READY;
            return EXIT_FAILURE;
        };

        if ( filespec->fh == NULL ) {
            d->err = GENERIC_DRIVER_ERROR_NOT_READY;
            return EXIT_FAILURE;
        };

        return EXIT_SUCCESS;
    }

#ifdef __cplusplus
}
#endif

#endif /* GENERIC_DRIVER_H */
