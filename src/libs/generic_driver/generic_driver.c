/* 
 * File:   generic_driver.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 19. února 2017, 9:23
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

/*
 * Genericky driver
 * ================
 * 
 * Tato knihovna slouzi jako univerzalni prostredek pro pristup k datovym blokum.
 * Jeji hlavni smysl tkvi v tom, ze virtualizuje datovou vrstvu pod kterou se muze 
 * skryvat napr.:
 * 
 * - image ulozeny v souboru na disku
 * - image umisteny v pameti
 * - image umisteny v bankovanem ramdisku
 * 
 * Nadrazena vrstva, ktera pouziva tento univerzalni driver by mela pri praci s daty 
 * vyuzivat vzdy ty same metody. Rozdily v pristupu k datum budou urceny jen 
 * pouzitym driverem a handlerem.
 * 
 * driver
 * ------
 * 
 * - obsahuje zakladni callbacky pro operace cteni a zapisu z konkretniho offsetu
 * 
 *      generic_driver_read_cb
 *      generic_driver_write_cb
 * 
 * - muze obsahovat take callback generic_driver_truncate_cb
 * 
 * - predevsim u ruznych memory handleru pak muze byt potreba pred kazdou 
 * operaci zavolat pripravny callback generic_driver_prepare_cb
 * 
 * handler
 * -------
 * 
 * Existuje prozatim ve dvou zakladnich variantach - souborovy a pametovy. U souboroveho handleru
 * je udrzovan pouze skutecny systemovy file pointer. U pametoveho handleru udrzujeme 
 * informaci o jeho rozmeru v poctu bajtu a pointer, ktery identifikuje pocatecni 
 * misto v pameti. 
 * Handler pro praci s ramdiskem, ci s unikartou prozatim jeste neexistuje.
 *  
 * Pro pametove handlery lze pouzit rychly, avsak potencionalne nebezpecny zpusob primeho pristupu:
 * 
 *   uint8_t work_buffer[256];
 *   uint8_t *b;
 * 
 *   if ( EXIT_SUCCESS != generic_driver_prepare ( handler, d, offset, (void**) &buffer, &work_buffer, sizeof ( work_buffer ) ) ) return EXIT_FAILURE;
 *   if ( EXIT_SUCCESS != generic_driver_ppread ( handler, d, offset, buffer, sizeof ( work_buffer ) ) ) return EXIT_FAILURE;
 *
 *   // Pri cteni muzeme rovnou pouzit:
 *   if ( EXIT_SUCCESS != generic_driver_direct_read ( handler, d, offset, (void**) &buffer, &work_buffer, sizeof ( work_buffer ) ) ) return EXIT_FAILURE;
 * 
 * Uvnitr generic_driver_prepare() se krom jineho rozhodne, zda *b bude ukazovat na &work_buffer,
 * nebo v pripade pametoveho handleru muze *b obdrzet hodnotu, ktera ukazuje jiz na 
 * konkretni misto image ulozeneho v pameti a volani ppread pak zavola generic_driver_read_cb(),
 * ktery pozna, ze buffer odkazuje na stejne misto se kterym se chystame pracovat a tedy jiz do bufferu nic neprenasi.
 * Timto se usetri jeden datovy prenos, ale je zde potencionalni nebezpeci v tom, ze *b ukazuje primo do image.
 * 
 */

#include <string.h>

#ifdef WINDOWS
#include <windows.h>
#elif LINUX
/* pri kompilaci pridat -D_XOPEN_SOURCE=500 */
#include <unistd.h>
#endif

#include "generic_driver.h"

#define HAVE_UI_UTILS

#ifdef HAVE_UI_UTILS
#include "src/ui/ui_utils.h"

#define generic_driver_utils_mem_alloc0( size ) ui_utils_mem_alloc ( size )
#define generic_driver_utils_mem_free( ptr ) ui_utils_mem_free ( ptr )
#define generic_driver_utils_file_open( filename, mode ) ui_utils_file_open ( filename, mode )
#define generic_driver_utils_file_read( buffer, size, count_bytes, fh ) ui_utils_file_read ( buffer, size, count_bytes, fh )
#define generic_driver_utils_file_write( buffer, size, count_bytes, fh ) ui_utils_file_write ( buffer, size, count_bytes, fh )
#define generic_driver_utils_file_close( fh ) ui_utils_file_close ( fh )

#else


static inline void* generic_driver_utils_mem_alloc0 ( uint32_t size ) {
    void *ptr = malloc ( size );
    if ( !ptr ) return NULL;
    memset ( ptr, 0x00, size );
    return ptr;
}

#define generic_driver_utils_mem_free( ptr ) free ( ptr )
#define generic_driver_utils_file_open( filename, mode ) fopen ( filename, mode )


static inline unsigned int generic_driver_utils_file_read ( void *buffer, unsigned int size, unsigned int count_bytes, FILE *fh ) {
#ifdef WINDOWS
    /*  stdio bug projevujici se pri "RW" mode :( */
    fseek ( fh, ftell ( fh ), SEEK_SET );
#endif
    return fread ( buffer, size, count_bytes, fh );
}


static inline unsigned int generic_driver_utils_file_write ( void *buffer, unsigned int size, unsigned int count_bytes, FILE *fh ) {
#ifdef WINDOWS
    /*  stdio bug projevujici se pri "RW" mode :( */
    fseek ( fh, ftell ( fh ), SEEK_SET );
#endif
    return fwrite ( buffer, size, count_bytes, fh );
}

#define generic_driver_utils_file_close( fh ) fclose ( fh )

#endif


/**
 * Vygenerovani error mesage pro driver, nebo handler.
 * 
 * @param handler
 * @param d
 * @return 
 */
const char* generic_driver_error_message ( st_HANDLER *h, st_DRIVER *d ) {

    char *no_err_msg = "no error";
    char *unknown_err_msg = "unknown error";

    const char *handler_err_msg[] = {
                                     "handler not ready",
                                     "handler is write protected",
    };

    const char *driver_err_msg[] = {
                                    "driver not ready",
                                    "seek error",
                                    "read error",
                                    "write error",
                                    "size error",
                                    "malloc error",
                                    "realloc error",
                                    "truncate error",
                                    "bad handler type",
                                    "handler is busy",
                                    "fopen error",
                                    "callback not exist",
    };

    if ( d == NULL ) {
        return unknown_err_msg;
    };

    if ( d->err != GENERIC_DRIVER_ERROR_NONE ) {
        if ( d->err >= GENERIC_DRIVER_ERROR_UNKNOWN ) return unknown_err_msg;
        return driver_err_msg [ d->err - 1 ];
    };

    if ( h == NULL ) return unknown_err_msg;

    if ( h->err == HANDLER_ERROR_NONE ) return no_err_msg;
    if ( h->err >= HANDLER_ERROR_USER ) return unknown_err_msg;
    return handler_err_msg [ h->err - 1 ];
}


/**
 * Nastaveni callbacku driveru.
 * 
 * 
 * @param d
 * @param opcb Callback volany pro otevreni media
 * @param clcb Callback volany pro ukonceni prace s mediem
 * @param rdcb Callback volany pri operaci READ
 * @param wrcb Callback volany pri operaci WRITE
 * @param prepcb Callback volany pred kazdou operaci READ / WRITE
 * @param trucb Callback volany po obecne operaci, ktera zpusobila zmenseni datoveho zdroje
 */
void generic_driver_setup ( st_DRIVER *d, generic_driver_open_cb opcb, generic_driver_close_cb clcb, generic_driver_read_cb rdcb, generic_driver_write_cb wrcb, generic_driver_prepare_cb prepcb, generic_driver_truncate_cb trucb ) {
    d->open_cb = opcb;
    d->close_cb = clcb;
    d->read_cb = rdcb;
    d->write_cb = wrcb;
    d->prepare_cb = prepcb;
    d->truncate_cb = trucb;
    d->err = GENERIC_DRIVER_ERROR_NONE;
}


/**
 * Vynuluje nastaveni handleru a prideli mu pozadovany typ.
 * 
 * @param handler 
 * @param type
 */
void generic_driver_register_handler ( st_HANDLER *h, en_HANDLER_TYPE type ) {
    memset ( h, 0x00, sizeof ( st_HANDLER ) );
    h->type = type;
    h->err = HANDLER_ERROR_NONE;
    h->status = HANDLER_STATUS_NOT_READY;
    h->driver = NULL;
}


/**
 * Otevre souborove medium.
 * 
 * @param handler - pokud je NULL, tak bude vytvoren, jinak bude natvrdo prepsan!
 * @param d - driver - musi obsahovat open_cb!
 * @param filename - jmeno souboru
 * @param open_mode - zpusob otevreni souboru
 * @return je vracen ukazatel handleru, nebo NULL, pokud doslo k chybe
 */
st_HANDLER* generic_driver_open_file ( st_HANDLER *handler, st_DRIVER *d, char *filename, en_FILE_DRIVER_OPEN_MODE open_mode ) {

    if ( d == NULL ) {
        return NULL;
    };

    st_HANDLER *h;
    if ( handler == NULL ) {
        h = generic_driver_utils_mem_alloc0 ( sizeof ( st_HANDLER ) );
    } else {
        h = handler;
    };

    if ( !d->open_cb ) {
        d->err = GENERIC_DRIVER_ERROR_CB_NOT_EXIST;
        return NULL;
    };

    generic_driver_register_handler ( h, HANDLER_TYPE_FILE );

    h->spec.filespec.filename = filename;
    h->spec.filespec.open_mode = open_mode;
    h->driver = d;

    if ( EXIT_FAILURE == d->open_cb ( h ) ) {
        if ( handler == NULL ) {
            generic_driver_utils_mem_free ( h );
        };
        return NULL;
    };

    return h;
}


/**
 * Otevre pametove medium.
 * 
 * @param handler - pokud je NULL, tak bude vytvoren, jinak bude natvrdo prepsan!
 * @param d - driver - musi obsahovat open_cb!
 * @param size - musi byt >= 1
 * @return je vracen ukazatel handleru, nebo NULL, pokud doslo k chybe
 */
st_HANDLER* generic_driver_open_memory ( st_HANDLER *handler, st_DRIVER *d, uint32_t size ) {

    if ( d == NULL ) {
        return NULL;
    };

    st_HANDLER *h;
    if ( handler == NULL ) {
        h = generic_driver_utils_mem_alloc0 ( sizeof ( st_HANDLER ) );
        generic_driver_register_handler ( h, HANDLER_TYPE_MEMORY );
    } else {
        h = handler;
    };

    if ( !d->open_cb ) {
        d->err = GENERIC_DRIVER_ERROR_CB_NOT_EXIST;
        return NULL;
    };

    h->spec.memspec.open_size = size;
    h->driver = d;

    if ( EXIT_FAILURE == d->open_cb ( h ) ) {
        if ( handler == NULL ) {
            generic_driver_utils_mem_free ( h );
        };
        return NULL;
    };

    return h;
}


/**
 * Otevre pametove medium, ktere inicializuje obsahem souboru.
 * 
 * @param handler - pokud je NULL, tak bude vytvoren, jinak bude natvrdo prepsan!
 * @param d - driver - musi obsahovat open_cb!
 * @param filename
 * @return je vracen ukazatel handleru, nebo NULL, pokud doslo k chybe
 */
st_HANDLER* generic_driver_open_memory_from_file ( st_HANDLER *handler, st_DRIVER *d, char *filename ) {

    if ( d == NULL ) {
        return NULL;
    };

    FILE *fh = generic_driver_utils_file_open ( filename, "rb" );

    uint32_t size = 1;

    if ( !fh ) {
        //printf ( "%s() - Error - Cant open file: %s\n", __func__, filename );
        return NULL;
    } else {
        fseek ( fh, 0, SEEK_END );
        size = ftell ( fh );
        fseek ( fh, 0, SEEK_SET );
    };

    st_HANDLER *h = generic_driver_open_memory ( handler, d, size );

    if ( !( ( !h ) || ( d->err ) || ( h->err ) ) ) {

        generic_driver_utils_file_read ( h->spec.memspec.ptr, 1, size, fh );

        if ( ferror ( fh ) ) {
            if ( handler == NULL ) {
                generic_driver_utils_mem_free ( h );
            };
            h = NULL;
        };

        generic_driver_utils_file_close ( fh );
    };

    return h;
}


/**
 * Ulozeni pametoveho bloku do souboru.
 * 
 * @param handler
 * @param filename
 * @return 
 */
int generic_driver_save_memory ( st_HANDLER *h, char *filename ) {

    st_HANDLER_MEMSPC *memspec = &h->spec.memspec;

    if ( EXIT_SUCCESS != generic_driver_memory_operation_internal_bootstrap ( h ) ) return EXIT_FAILURE;

    FILE *fh = generic_driver_utils_file_open ( filename, "wb" );

    if ( !fh ) return EXIT_FAILURE;

    fseek ( fh, 0, SEEK_SET );

    generic_driver_utils_file_write ( memspec->ptr, 1, memspec->size, fh );

    generic_driver_utils_file_close ( fh );

    return EXIT_SUCCESS;
}


int generic_driver_close ( st_HANDLER *h ) {

    st_DRIVER *d = (st_DRIVER *) h->driver;

    if ( d == NULL ) {
        return EXIT_FAILURE;
    };

    if ( !d->close_cb ) {
        d->err = GENERIC_DRIVER_ERROR_CB_NOT_EXIST;
        return EXIT_FAILURE;
    };
    return d->close_cb ( h );
}


/**
 * Set/Reset readonly status handleru.
 * 
 * @param handler
 * @param readonly
 */
void generic_driver_set_handler_readonly_status ( st_HANDLER *h, int readonly ) {
    if ( readonly == 0 ) {
        h->status &= ~HANDLER_ERROR_WRITE_PROTECTED;
    } else {
        h->status |= HANDLER_ERROR_WRITE_PROTECTED;
    };
}


/**
 * Priprav driver na nasledujici operaci.
 * 
 * @param handler
 * @param offset
 * @param buffer
 * @param work_buffer
 * @param buffer_size
 * @return 
 */
int generic_driver_prepare ( st_HANDLER *h, uint32_t offset, void **buffer, void *work_buffer, uint32_t buffer_size ) {

    st_DRIVER *d = (st_DRIVER *) h->driver;

    if ( d == NULL ) {
        return EXIT_FAILURE;
    };

    if ( d->prepare_cb != NULL ) {
        if ( EXIT_SUCCESS != d->prepare_cb ( h, offset, buffer, buffer_size ) ) return EXIT_FAILURE;
    };
    if ( *buffer == NULL ) {
        *buffer = work_buffer;
    };
    return EXIT_SUCCESS;
}


/**
 * Cteni z jiz pripraveneho driveru.
 * 
 * @param handler
 * @param offset
 * @param buffer
 * @param buffer_size
 * @return 
 */
int generic_driver_ppread ( st_HANDLER *h, uint32_t offset, void *buffer, uint32_t buffer_size ) {

    st_DRIVER *d = (st_DRIVER *) h->driver;

    if ( d == NULL ) {
        return EXIT_FAILURE;
    };

    if ( d->read_cb == NULL ) {
        d->err = GENERIC_DRIVER_ERROR_CB_NOT_EXIST;
        return EXIT_FAILURE;
    };

    uint32_t result_len;
    int result = d->read_cb ( h, offset, buffer, buffer_size, &result_len );
    if ( ( result != EXIT_SUCCESS ) || ( result_len != buffer_size ) ) {
        return EXIT_FAILURE;
    };

    return EXIT_SUCCESS;
}


/**
 * Zapis do jiz pripraveneho driveru.
 * 
 * @param handler
 * @param d
 * @param offset
 * @param buffer
 * @param buffer_size
 * @return 
 */
int generic_driver_ppwrite ( st_HANDLER *h, uint32_t offset, void *buffer, uint32_t buffer_size ) {

    st_DRIVER *d = (st_DRIVER *) h->driver;

    if ( d == NULL ) {
        return EXIT_FAILURE;
    };

    if ( d->write_cb == NULL ) {
        d->err = GENERIC_DRIVER_ERROR_CB_NOT_EXIST;
        return EXIT_FAILURE;
    };

    uint32_t result_len;
    int result = d->write_cb ( h, offset, buffer, buffer_size, &result_len );
    if ( ( result != EXIT_SUCCESS ) || ( result_len != buffer_size ) ) {
        return EXIT_FAILURE;
    };

    return EXIT_SUCCESS;
}


/**
 * Provede prepare + cteni.
 * 
 * @param handler
 * @param offset
 * @param buffer
 * @param buffer_size
 * @return 
 */
int generic_driver_read ( st_HANDLER *h, uint32_t offset, void *buffer, uint32_t buffer_size ) {
    void *work_buffer = NULL;
    if ( EXIT_SUCCESS != generic_driver_prepare ( h, offset, &work_buffer, NULL, buffer_size ) ) return EXIT_FAILURE;
    return generic_driver_ppread ( h, offset, buffer, buffer_size );
}


/**
 * Provede prepare + cteni do predem pripraveneho work_buffer.
 * 
 * @param handler
 * @param offset
 * @param buffer
 * @param work_buffer
 * @param buffer_size
 * @return 
 */
int generic_driver_direct_read ( st_HANDLER *h, uint32_t offset, void **buffer, void *work_buffer, uint32_t buffer_size ) {
    if ( EXIT_SUCCESS != generic_driver_prepare ( h, offset, buffer, work_buffer, buffer_size ) ) return EXIT_FAILURE;
    return generic_driver_ppread ( h, offset, *buffer, buffer_size );
}


/**
 * Provede prepare + zapis.
 * 
 * @param handler
 * @param offset
 * @param buffer
 * @param buffer_size
 * @return 
 */
int generic_driver_write ( st_HANDLER *h, uint32_t offset, void *buffer, uint32_t buffer_size ) {
    void *work_buffer = NULL;
    if ( EXIT_SUCCESS != generic_driver_prepare ( h, offset, &work_buffer, NULL, buffer_size ) ) return EXIT_FAILURE;
    return generic_driver_ppwrite ( h, offset, buffer, buffer_size );
}


/**
 * Provede zarovnani media.
 * 
 * @param handler
 * @param size
 * @return 
 */
int generic_driver_truncate ( st_HANDLER *h, uint32_t size ) {

    st_DRIVER *d = (st_DRIVER *) h->driver;

    if ( d == NULL ) {
        return EXIT_FAILURE;
    };

    if ( d->truncate_cb != NULL ) {
        if ( EXIT_SUCCESS != d->truncate_cb ( h, size ) ) return EXIT_FAILURE;
    };
    return EXIT_SUCCESS;
}


/*
 * 
 * 
 * Priklady callbacku
 * 
 * 
 */



/*
 * 
 * Driver pro praci s obrazem v souboru.
 * 
 */
#ifdef GENERIC_DRIVER_FILE_CB


/**
 * Obecny driver pro cteni ze souboru.
 * 
 * @param handler
 * @param offset
 * @param buffer
 * @param count_bytes
 * @param readlen
 * @return EXIT_FAILURE | EXIT_SUCCESS
 */
int generic_driver_read_file_cb ( st_HANDLER *h, uint32_t offset, void *buffer, uint32_t count_bytes, uint32_t *readlen ) {

    st_DRIVER *d = (st_DRIVER *) h->driver;
    st_HANDLER_FILESPC *filespec = &h->spec.filespec;

    *readlen = 0;

    if ( EXIT_SUCCESS != generic_driver_file_operation_internal_bootstrap ( h ) ) return EXIT_FAILURE;

    FILE *fh = filespec->fh;

    if ( EXIT_SUCCESS != fseek ( fh, offset, SEEK_SET ) ) {
        d->err = GENERIC_DRIVER_ERROR_SEEK;
        return EXIT_FAILURE;
    };

#ifdef WINDOWS
    /*  stdio bug projevujici se pri "RW" mode :( */
    fseek ( fh, ftell ( fh ), SEEK_SET );
#endif

    *readlen = fread ( buffer, 1, count_bytes, fh );
    if ( *readlen != count_bytes ) {
        d->err = GENERIC_DRIVER_ERROR_READ;
        return EXIT_FAILURE;
    };

    return EXIT_SUCCESS;
}


/**
 * Obecny driver pro zapis do souboru.
 * 
 * @param handler
 * @param offset
 * @param buffer
 * @param count_bytes
 * @param writelen
 * @return EXIT_FAILURE | EXIT_SUCCESS
 */
int generic_driver_write_file_cb ( st_HANDLER *h, uint32_t offset, void *buffer, uint32_t count_bytes, uint32_t *writelen ) {

    st_DRIVER *d = (st_DRIVER *) h->driver;
    st_HANDLER_FILESPC *filespec = &h->spec.filespec;

    *writelen = 0;

    if ( EXIT_SUCCESS != generic_driver_file_operation_internal_bootstrap ( h ) ) return EXIT_FAILURE;

    if ( h->status & HANDLER_STATUS_READ_ONLY ) {
        h->err = HANDLER_ERROR_WRITE_PROTECTED;
        return EXIT_FAILURE;
    };

    FILE *fh = filespec->fh;

    if ( EXIT_SUCCESS != fseek ( fh, offset, SEEK_SET ) ) {
        d->err = GENERIC_DRIVER_ERROR_SEEK;
        return EXIT_FAILURE;
    };

#ifdef WINDOWS
    /*  stdio bug projevujici se pri "RW" mode :( */
    fseek ( fh, ftell ( fh ), SEEK_SET );
#endif

    *writelen = fwrite ( buffer, 1, count_bytes, fh );
    if ( *writelen != count_bytes ) {
        d->err = GENERIC_DRIVER_ERROR_WRITE;
        return EXIT_FAILURE;
    };

    return EXIT_SUCCESS;
}


/**
 * Obecny driver pro zkraceni souboru na pozadovanou delku.
 * 
 * @param handler
 * @param size
 * @return EXIT_FAILURE | EXIT_SUCCESS
 */
int generic_driver_truncate_file_cb ( st_HANDLER *h, uint32_t size ) {

    st_DRIVER *d = (st_DRIVER *) h->driver;
    st_HANDLER_FILESPC *filespec = &h->spec.filespec;

    if ( EXIT_SUCCESS != generic_driver_file_operation_internal_bootstrap ( h ) ) return EXIT_FAILURE;

    if ( h->status & HANDLER_STATUS_READ_ONLY ) {
        h->err = HANDLER_ERROR_WRITE_PROTECTED;
        return EXIT_FAILURE;
    };

    FILE *fh = filespec->fh;

    if ( fh == NULL ) {
        d->err = GENERIC_DRIVER_ERROR_NOT_READY;
        return EXIT_FAILURE;
    };

#ifdef WINDOWS
    if ( EXIT_SUCCESS != fseek ( fh, size, SEEK_SET ) ) {
        d->err = GENERIC_DRIVER_ERROR_SEEK;
        return EXIT_FAILURE;
    };
    if ( EXIT_SUCCESS != SetEndOfFile ( fh ) ) {
        d->err = GENERIC_DRIVER_ERROR_TRUNCATE;
        return EXIT_FAILURE;
    };
#elif LINUX
    if ( EXIT_SUCCESS != ftruncate ( fileno ( fh ), size ) ) {
        d->err = GENERIC_DRIVER_ERROR_TRUNCATE;
        return EXIT_FAILURE;
    };
#endif

    return EXIT_SUCCESS;
}


/**
 * Otevreni souboroveho handleru.
 * 
 * @param handler
 * @return EXIT_FAILURE | EXIT_SUCCESS
 */
int generic_driver_open_file_cb ( st_HANDLER *h ) {

    st_DRIVER *d = (st_DRIVER *) h->driver;
    st_HANDLER_FILESPC *filespec = &h->spec.filespec;

    int fl_readonly = 1;
    char *txt_mode = "rb";

    h->err = HANDLER_ERROR_NONE;
    d->err = GENERIC_DRIVER_ERROR_NONE;

    if ( h->status & HANDLER_STATUS_READY ) {
        d->err = GENERIC_DRIVER_ERROR_HANDLER_IS_BUSY;
        return EXIT_FAILURE;
    };

    if ( h->type != HANDLER_TYPE_FILE ) {
        d->err = GENERIC_DRIVER_ERROR_HANDLER_TYPE;
        return EXIT_FAILURE;
    };

    if ( filespec->fh != NULL ) {
        d->err = GENERIC_DRIVER_ERROR_HANDLER_IS_BUSY;
        return EXIT_FAILURE;
    };

    d->err = GENERIC_DRIVER_ERROR_NONE;
    h->status = HANDLER_STATUS_NOT_READY;

    switch ( filespec->open_mode ) {
        case FILE_DRIVER_OPMODE_RO:
            txt_mode = "rb";
            fl_readonly = 1;
            break;

        case FILE_DRIVER_OPMODE_RW:
            txt_mode = "r+b";
            fl_readonly = 0;
            break;

        case FILE_DRIVER_OPMODE_W:
            txt_mode = "w+b";
            fl_readonly = 0;
            break;
    };

    filespec->fh = fopen ( filespec->filename, txt_mode );

    if ( !filespec->fh ) {
        d->err = GENERIC_DRIVER_ERROR_FOPEN;
        return EXIT_FAILURE;
    };

    h->status = HANDLER_STATUS_READY;

    if ( fl_readonly ) {
        h->status |= HANDLER_STATUS_READ_ONLY;
    };

    return EXIT_SUCCESS;
}


/**
 * Zavreni souboroveho handleru.
 * 
 * @param handler
 * @return EXIT_FAILURE | EXIT_SUCCESS
 */
int generic_driver_close_file_cb ( st_HANDLER *h ) {

    st_DRIVER *d = (st_DRIVER *) h->driver;
    st_HANDLER_FILESPC *filespec = &h->spec.filespec;

    if ( EXIT_SUCCESS != generic_driver_file_operation_internal_bootstrap ( h ) ) return EXIT_FAILURE;

    fclose ( filespec->fh );

    filespec->fh = NULL;

    h->err = HANDLER_ERROR_NONE;
    d->err = GENERIC_DRIVER_ERROR_NONE;
    h->status = HANDLER_STATUS_NOT_READY;

    return EXIT_SUCCESS;
}


st_DRIVER* generic_driver_file_init ( st_DRIVER *d ) {

    if ( d == NULL ) {
        d = generic_driver_utils_mem_alloc0 ( sizeof ( st_DRIVER ) );
    };

    generic_driver_setup (
                           d,
                           generic_driver_open_file_cb,
                           generic_driver_close_file_cb,
                           generic_driver_read_file_cb,
                           generic_driver_write_file_cb,
                           NULL,
                           generic_driver_truncate_file_cb );

    return d;

}


#endif


/*
 * 
 * Driver pro praci s obrazem v pameti.
 * 
 */
#ifdef GENERIC_DRIVER_MEMORY_CB


/**
 * Obecny driver pro pripravu pozadovane casti obrazu do pameti.
 * 
 * @param handler
 * @param offset
 * @param buffer
 * @param count_bytes
 * @return EXIT_FAILURE | EXIT_SUCCESS
 */
int generic_driver_prepare_memory_cb ( st_HANDLER *h, uint32_t offset, void **buffer, uint32_t count_bytes ) {

    st_DRIVER *d = (st_DRIVER *) h->driver;
    st_HANDLER_MEMSPC *memspec = &h->spec.memspec;

    if ( EXIT_SUCCESS != generic_driver_memory_operation_internal_bootstrap ( h ) ) return EXIT_FAILURE;

#ifndef GENERIC_DRIVER_MEMORY_CB_USE_REALLOC
    if ( memspec->ptr == NULL ) {
        d->err = GENERIC_DRIVER_ERROR_NOT_READY;
        return EXIT_FAILURE;
    };
#endif

    uint32_t need_size = offset + count_bytes;
    *buffer = NULL;

#ifdef GENERIC_DRIVER_MEMORY_CB_USE_REALLOC

    if ( memspec->swelling_enabled ) {
        if ( ( offset > memspec->size ) || ( need_size > memspec->size ) ) {
            if ( h->status & HANDLER_STATUS_READ_ONLY ) {
                h->err = HANDLER_ERROR_WRITE_PROTECTED;
                return EXIT_FAILURE;
            };

            memspec->updated = 1;

            uint8_t * new = realloc ( memspec->ptr, need_size );

            if ( new == NULL ) {
                d->err = GENERIC_DRIVER_ERROR_REALLOC;
                return EXIT_FAILURE;
            };

            memspec->ptr = new;
            memspec->size = need_size;
        };
    } else {
        if ( offset > memspec->size ) {
            d->err = GENERIC_DRIVER_ERROR_SEEK;
            return EXIT_FAILURE;
        };

        if ( need_size > memspec->size ) {
            d->err = GENERIC_DRIVER_ERROR_SIZE;
            return EXIT_FAILURE;
        };
    };

#else

    if ( offset > memspec->size ) {
        d->err = GENERIC_DRIVER_ERROR_SEEK;
        return EXIT_FAILURE;
    };

    if ( need_size > memspec->size ) {
        d->err = GENERIC_DRIVER_ERROR_SIZE;
        return EXIT_FAILURE;
    };

#endif

    *buffer = &memspec->ptr[offset];

    return EXIT_SUCCESS;
}


/**
 * Obecny driver pro cteni z pameti.
 * 
 * @param handler
 * @param offset
 * @param buffer
 * @param count_bytes
 * @param readlen
 * @return EXIT_FAILURE | EXIT_SUCCESS
 */
int generic_driver_read_memory_cb ( st_HANDLER *h, uint32_t offset, void *buffer, uint32_t count_bytes, uint32_t *readlen ) {

    st_DRIVER *d = (st_DRIVER *) h->driver;
    st_HANDLER_MEMSPC *memspec = &h->spec.memspec;

    *readlen = 0;

    if ( EXIT_SUCCESS != generic_driver_memory_operation_internal_bootstrap ( h ) ) return EXIT_FAILURE;

    if ( offset > memspec->size ) {
        d->err = GENERIC_DRIVER_ERROR_SEEK;
        return EXIT_FAILURE;
    };

    uint32_t need_size = offset + count_bytes;
    if ( need_size > memspec->size ) {
        d->err = GENERIC_DRIVER_ERROR_SIZE;
        return EXIT_FAILURE;
    };

    *readlen = count_bytes;
    if ( &memspec->ptr[offset] != buffer ) {
        memmove ( buffer, &memspec->ptr[offset], count_bytes );
    };

    return EXIT_SUCCESS;
}


/**
 * Obecny driver pro zapis do pameti.
 * 
 * @param handler
 * @param offset
 * @param buffer
 * @param count_bytes
 * @param writelen
 * @return EXIT_FAILURE | EXIT_SUCCESS
 */
int generic_driver_write_memory_cb ( st_HANDLER *h, uint32_t offset, void *buffer, uint32_t count_bytes, uint32_t *writelen ) {

    st_DRIVER *d = (st_DRIVER *) h->driver;
    st_HANDLER_MEMSPC *memspec = &h->spec.memspec;

    *writelen = 0;

    if ( EXIT_SUCCESS != generic_driver_memory_operation_internal_bootstrap ( h ) ) return EXIT_FAILURE;

    if ( h->status & HANDLER_STATUS_READ_ONLY ) {
        h->err = HANDLER_ERROR_WRITE_PROTECTED;
        return EXIT_FAILURE;
    };

    if ( offset > memspec->size ) {
        d->err = GENERIC_DRIVER_ERROR_SEEK;
        return EXIT_FAILURE;
    };

    uint32_t need_size = offset + count_bytes;
    if ( need_size > memspec->size ) {
        d->err = GENERIC_DRIVER_ERROR_SIZE;
        return EXIT_FAILURE;
    };

    *writelen = count_bytes;
    if ( &memspec->ptr[offset] != buffer ) {
        memspec->updated = 1;
        memmove ( &memspec->ptr[offset], buffer, count_bytes );
    };

    return EXIT_SUCCESS;
}


/**
 * Obecny driver pro zkraceni obrazu ulozeneho v pameti.
 * 
 * @param handler
 * @param size
 * @return EXIT_FAILURE | EXIT_SUCCESS
 */
int generic_driver_truncate_memory_cb ( st_HANDLER *h, uint32_t size ) {

    st_DRIVER *d = (st_DRIVER *) h->driver;
    st_HANDLER_MEMSPC *memspec = &h->spec.memspec;

    if ( EXIT_SUCCESS != generic_driver_memory_operation_internal_bootstrap ( h ) ) return EXIT_FAILURE;

    if ( h->status & HANDLER_STATUS_READ_ONLY ) {
        h->err = HANDLER_ERROR_WRITE_PROTECTED;
        return EXIT_FAILURE;
    };

    if ( size < 1 ) {
        d->err = GENERIC_DRIVER_ERROR_SIZE;
        return EXIT_FAILURE;
    };

    memspec->updated = 1;

    uint8_t * new = realloc ( memspec->ptr, size );

    if ( new == NULL ) {
        d->err = GENERIC_DRIVER_ERROR_REALLOC;
        return EXIT_FAILURE;
    };

    memspec->ptr = new;
    memspec->size = size;

    return EXIT_SUCCESS;
}


/**
 * Otevreni noveho memory handleru.
 * 
 * @param handler
 * @return 
 */
int generic_driver_open_memory_cb ( st_HANDLER *h ) {

    st_DRIVER *d = (st_DRIVER *) h->driver;
    st_HANDLER_MEMSPC *memspec = &h->spec.memspec;

    h->err = HANDLER_ERROR_NONE;
    d->err = GENERIC_DRIVER_ERROR_NONE;

    if ( h->status & HANDLER_STATUS_READY ) {
        d->err = GENERIC_DRIVER_ERROR_HANDLER_IS_BUSY;
        return EXIT_FAILURE;
    };

    if ( h->type != HANDLER_TYPE_MEMORY ) {
        d->err = GENERIC_DRIVER_ERROR_HANDLER_TYPE;
        return EXIT_FAILURE;
    };

    if ( memspec->ptr != NULL ) {
        d->err = GENERIC_DRIVER_ERROR_HANDLER_IS_BUSY;
        return EXIT_FAILURE;
    };

    if ( memspec->open_size < 1 ) {
        d->err = GENERIC_DRIVER_ERROR_SIZE;
        return EXIT_FAILURE;
    };

    d->err = GENERIC_DRIVER_ERROR_NONE;
    h->status = HANDLER_STATUS_NOT_READY;

    uint8_t *new_ptr = malloc ( memspec->open_size );

    if ( new_ptr == NULL ) {
        d->err = GENERIC_DRIVER_ERROR_MALLOC;
        return EXIT_FAILURE;
    };

    memspec->ptr = new_ptr;
    memspec->size = memspec->open_size;

    memspec->updated = 0;
    h->status = HANDLER_STATUS_READY;

    return EXIT_SUCCESS;
}


/**
 * Uzavreni memory handleru.
 * 
 * @param handler
 * @param driver
 * @return 
 */
int generic_driver_close_memory_cb ( st_HANDLER *h ) {

    st_DRIVER *d = (st_DRIVER *) h->driver;
    st_HANDLER_MEMSPC *memspec = &h->spec.memspec;

    if ( EXIT_SUCCESS != generic_driver_memory_operation_internal_bootstrap ( h ) ) return EXIT_FAILURE;

    free ( memspec->ptr );

    memspec->ptr = NULL;
    memspec->size = 0;

    memspec->updated = 0;
    h->err = HANDLER_ERROR_NONE;
    d->err = GENERIC_DRIVER_ERROR_NONE;
    h->status = HANDLER_STATUS_NOT_READY;

    return EXIT_SUCCESS;
}


st_DRIVER* generic_driver_memory_init ( st_DRIVER *d ) {

    if ( d == NULL ) {
        d = generic_driver_utils_mem_alloc0 ( sizeof ( st_DRIVER ) );
    };

    generic_driver_setup (
                           d,
                           generic_driver_open_memory_cb,
                           generic_driver_close_memory_cb,
                           generic_driver_read_memory_cb,
                           generic_driver_write_memory_cb,
                           generic_driver_prepare_memory_cb,
                           generic_driver_truncate_memory_cb );

    return d;

}
#endif
