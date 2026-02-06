/*
 * Stub implementations for UI functions when ENABLE_UI is disabled
 *
 * This file provides compiled stub implementations for utility functions
 * that require actual logic (file I/O, memory management, etc.).
 * Simple no-op UI callbacks are provided as inline stubs in ui_stub.h.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <glib.h>
#include "libs/generic_driver/generic_driver.h"

// Memory allocation stubs - use standard malloc/free
void* ui_utils_mem_alloc_raw(guint32 size, int initialize0, const char *func, int line) {
    if (initialize0) {
        return calloc(1, size);
    }
    return malloc(size);
}

void* ui_utils_mem_realloc_raw(void *ptr, guint32 size, const char *func, int line) {
    return realloc(ptr, size);
}

void ui_utils_mem_free(void *ptr) {
    free(ptr);
}

// File operation stubs
char* ui_utils_file_name_locale_from_utf8(const char *filename_in_utf8) {
    // Use glib's locale conversion like the UI version does
    return g_locale_from_utf8(filename_in_utf8, -1, NULL, NULL, NULL);
}

FILE* ui_utils_file_open(const char *filename_in_utf8, const char *mode) {
    char *filename_in_locale;
    FILE *fp;

    filename_in_locale = ui_utils_file_name_locale_from_utf8(filename_in_utf8);
    if (!filename_in_locale) {
        return NULL;
    }

    fp = fopen(filename_in_locale, mode);
    g_free(filename_in_locale);
    return fp;
}

void ui_utils_file_close(FILE *fh) {
    if (fh) fclose(fh);
}

unsigned int ui_utils_file_read(void *buffer, unsigned int size, unsigned int count_bytes, FILE *fh) {
    return fread(buffer, size, count_bytes, fh);
}

unsigned int ui_utils_file_write(void *buffer, unsigned int size, unsigned int count_bytes, FILE *fh) {
    return fwrite(buffer, size, count_bytes, fh);
}

int ui_utils_file_access(const char *filename_in_utf8, int type) {
    char *filename_in_locale;
    filename_in_locale = ui_utils_file_name_locale_from_utf8(filename_in_utf8);
    int retval = access(filename_in_locale, type);
    g_free(filename_in_locale);
    return retval;
}

int ui_utils_file_rename(char *path, char *oldname, char *newname) {
    char old_path[1024], new_path[1024];
    snprintf(old_path, sizeof(old_path), "%s/%s", path, oldname);
    snprintf(new_path, sizeof(new_path), "%s/%s", path, newname);
    return rename(old_path, new_path);
}

int ui_utils_file_truncate(const char *filename_in_utf8, off_t length) {
    char *filename_in_locale = ui_utils_file_name_locale_from_utf8(filename_in_utf8);
    int ret = truncate(filename_in_locale, length);
    g_free(filename_in_locale);
    return ret;
}

// Directory operation stubs
GDir* ui_utils_dir_open(char *path) {
    return g_dir_open(path, 0, NULL);
}

void ui_utils_dir_close(GDir *dh) {
    if (dh) g_dir_close(dh);
}

const char* ui_utils_dir_read(GDir *dh) {
    return g_dir_read_name(dh);
}

unsigned ui_utils_ditem_is_file(char *path, const char *ditem) {
    char fullpath[1024];
    struct stat st;
    snprintf(fullpath, sizeof(fullpath), "%s/%s", path, ditem);
    if (stat(fullpath, &st) == 0) {
        return S_ISREG(st.st_mode);
    }
    return 0;
}

const gchar* ui_utils_ditem_get_filepath(const char *path, const char *ditem) {
    return g_build_filename(path, ditem, (gchar*)NULL);
}

const gchar* ui_utils_ditem_get_name(const char *ditem) {
    return ditem;
}

char* ui_utils_basename(const char *file_name) {
    return g_path_get_basename(file_name);
}

const gchar* ui_utils_get_error_message(void) {
    return strerror(errno);
}

char* ui_utils_strerror(void) {
    return strerror(errno);
}

// Audio stub
void ui_audio_init(void) {
    // No-op
}

// UI update stubs
void ui_fdc_menu_update(void) {}
void ui_fdc_set_dsk(int drive, const char *path) {}
void ui_memext_menu_update(void) {}
void ui_rom_menu_update(void) {}
void ui_rom_settings_open_window(void) {}
void ui_ide8_update_menu(void) {}
void ui_qdisk_menu_update(void) {}
void ui_qdisk_set_path(int drive, const char *path) {}
void ui_ramdisk_update_menu(void) {}
void ui_unicard_update_menu(void) {}

// Virtual keyboard stubs
void ui_vkbd_show_hide(void) {}
void ui_vkbd_reset_keyboard_state(void) {}
void ui_vkbd_autotype_deactivate(void) {}

// Joy calibration stub
void ui_joy_sysdevice_calibration(int device) {}

// Memory driver implementation
st_DRIVER g_ui_memory_driver_static;
st_DRIVER g_ui_memory_driver_realloc;

// Memory driver callbacks
static int ui_memory_driver_open_cb(st_HANDLER *h) {
    st_DRIVER *d = (st_DRIVER *)h->driver;
    st_HANDLER_MEMSPC *memspec = &h->spec.memspec;

    h->err = HANDLER_ERROR_NONE;
    d->err = GENERIC_DRIVER_ERROR_NONE;

    if (h->status & HANDLER_STATUS_READY) {
        d->err = GENERIC_DRIVER_ERROR_HANDLER_IS_BUSY;
        return EXIT_FAILURE;
    }

    if (h->type != HANDLER_TYPE_MEMORY) {
        d->err = GENERIC_DRIVER_ERROR_HANDLER_TYPE;
        return EXIT_FAILURE;
    }

    if (memspec->ptr != NULL) {
        d->err = GENERIC_DRIVER_ERROR_HANDLER_IS_BUSY;
        return EXIT_FAILURE;
    }

    if (memspec->open_size < 1) {
        d->err = GENERIC_DRIVER_ERROR_SIZE;
        return EXIT_FAILURE;
    }

    uint8_t *new_ptr = calloc(1, memspec->open_size);

    if (new_ptr == NULL) {
        d->err = GENERIC_DRIVER_ERROR_MALLOC;
        return EXIT_FAILURE;
    }

    memspec->ptr = new_ptr;
    memspec->size = memspec->open_size;
    h->status = HANDLER_STATUS_READY;

    return EXIT_SUCCESS;
}

static int ui_memory_driver_close_cb(st_HANDLER *h) {
    st_DRIVER *d = (st_DRIVER *)h->driver;
    st_HANDLER_MEMSPC *memspec = &h->spec.memspec;

    if (EXIT_SUCCESS != generic_driver_memory_operation_internal_bootstrap(h)) return EXIT_FAILURE;

    free(memspec->ptr);
    memspec->ptr = NULL;
    memspec->size = 0;

    h->err = HANDLER_ERROR_NONE;
    d->err = GENERIC_DRIVER_ERROR_NONE;
    h->status = HANDLER_STATUS_NOT_READY;

    return EXIT_SUCCESS;
}

static int ui_memory_driver_read_cb(st_HANDLER *h, uint32_t offset, void *buffer, uint32_t count_bytes, uint32_t *readlen) {
    st_DRIVER *d = (st_DRIVER *)h->driver;
    st_HANDLER_MEMSPC *memspec = &h->spec.memspec;

    *readlen = 0;

    if (EXIT_SUCCESS != generic_driver_memory_operation_internal_bootstrap(h)) return EXIT_FAILURE;

    if (offset > memspec->size) {
        d->err = GENERIC_DRIVER_ERROR_SEEK;
        return EXIT_FAILURE;
    }

    uint32_t need_size = offset + count_bytes;
    if (need_size > memspec->size) {
        d->err = GENERIC_DRIVER_ERROR_SIZE;
        return EXIT_FAILURE;
    }

    *readlen = count_bytes;
    if (&memspec->ptr[offset] != buffer) {
        memmove(buffer, &memspec->ptr[offset], count_bytes);
    }

    return EXIT_SUCCESS;
}

static int ui_memory_driver_write_cb(st_HANDLER *h, uint32_t offset, void *buffer, uint32_t count_bytes, uint32_t *writelen) {
    st_DRIVER *d = (st_DRIVER *)h->driver;
    st_HANDLER_MEMSPC *memspec = &h->spec.memspec;

    *writelen = 0;

    if (EXIT_SUCCESS != generic_driver_memory_operation_internal_bootstrap(h)) return EXIT_FAILURE;

    if (h->status & HANDLER_STATUS_READ_ONLY) {
        h->err = HANDLER_ERROR_WRITE_PROTECTED;
        return EXIT_FAILURE;
    }

    if (offset > memspec->size) {
        d->err = GENERIC_DRIVER_ERROR_SEEK;
        return EXIT_FAILURE;
    }

    uint32_t need_size = offset + count_bytes;
    if (need_size > memspec->size) {
        d->err = GENERIC_DRIVER_ERROR_SIZE;
        return EXIT_FAILURE;
    }

    *writelen = count_bytes;
    if (&memspec->ptr[offset] != buffer) {
        memmove(&memspec->ptr[offset], buffer, count_bytes);
    }

    return EXIT_SUCCESS;
}

static int ui_memory_driver_prepare_static_cb(st_HANDLER *h, uint32_t offset, void **buffer, uint32_t count_bytes) {
    st_DRIVER *d = (st_DRIVER *)h->driver;
    st_HANDLER_MEMSPC *memspec = &h->spec.memspec;

    if (EXIT_SUCCESS != generic_driver_memory_operation_internal_bootstrap(h)) return EXIT_FAILURE;

    uint32_t need_size = offset + count_bytes;
    *buffer = NULL;

    if (offset > memspec->size) {
        d->err = GENERIC_DRIVER_ERROR_SEEK;
        return EXIT_FAILURE;
    }

    if (need_size > memspec->size) {
        d->err = GENERIC_DRIVER_ERROR_SIZE;
        return EXIT_FAILURE;
    }

    *buffer = &memspec->ptr[offset];

    return EXIT_SUCCESS;
}

static int ui_memory_driver_prepare_realloc_cb(st_HANDLER *h, uint32_t offset, void **buffer, uint32_t count_bytes) {
    st_DRIVER *d = (st_DRIVER *)h->driver;
    st_HANDLER_MEMSPC *memspec = &h->spec.memspec;

    if (EXIT_SUCCESS != generic_driver_memory_operation_internal_bootstrap(h)) return EXIT_FAILURE;

    uint32_t need_size = offset + count_bytes;
    *buffer = NULL;

    if ((offset > memspec->size) || (need_size > memspec->size)) {
        if (h->status & HANDLER_STATUS_READ_ONLY) {
            h->err = HANDLER_ERROR_WRITE_PROTECTED;
            return EXIT_FAILURE;
        }

        uint8_t *new = realloc(memspec->ptr, need_size);

        if (new == NULL) {
            d->err = GENERIC_DRIVER_ERROR_REALLOC;
            return EXIT_FAILURE;
        }

        memspec->ptr = new;
        memspec->size = need_size;
    }

    *buffer = &memspec->ptr[offset];

    return EXIT_SUCCESS;
}

static int ui_memory_driver_truncate_cb(st_HANDLER *h, uint32_t size) {
    st_DRIVER *d = (st_DRIVER *)h->driver;
    st_HANDLER_MEMSPC *memspec = &h->spec.memspec;

    if (EXIT_SUCCESS != generic_driver_memory_operation_internal_bootstrap(h)) return EXIT_FAILURE;

    if (h->status & HANDLER_STATUS_READ_ONLY) {
        h->err = HANDLER_ERROR_WRITE_PROTECTED;
        return EXIT_FAILURE;
    }

    if (size < 1) {
        d->err = GENERIC_DRIVER_ERROR_SIZE;
        return EXIT_FAILURE;
    }

    uint8_t *new = realloc(memspec->ptr, size);

    if (new == NULL) {
        d->err = GENERIC_DRIVER_ERROR_REALLOC;
        return EXIT_FAILURE;
    }

    memspec->ptr = new;
    memspec->size = size;

    return EXIT_SUCCESS;
}

void ui_memory_driver_init(void) {
    generic_driver_setup(&g_ui_memory_driver_static, ui_memory_driver_open_cb, ui_memory_driver_close_cb, ui_memory_driver_read_cb, ui_memory_driver_write_cb, ui_memory_driver_prepare_static_cb, ui_memory_driver_truncate_cb);
    generic_driver_setup(&g_ui_memory_driver_realloc, ui_memory_driver_open_cb, ui_memory_driver_close_cb, ui_memory_driver_read_cb, ui_memory_driver_write_cb, ui_memory_driver_prepare_realloc_cb, ui_memory_driver_truncate_cb);
}
