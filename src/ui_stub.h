/*
 * File:   ui_stub.h
 * Stub UI functions for headless (no GTK) builds
 *
 * This file provides empty stub implementations of UI functions
 * when ENABLE_UI is not defined, allowing the emulator to run
 * without GTK dependencies.
 */

#ifndef UI_STUB_H
#define UI_STUB_H

#ifdef __cplusplus
extern "C" {
#endif

// Empty stub functions when UI is disabled
static inline void ui_init(void) {
    // No-op in headless mode
}

static inline void ui_exit(void) {
    // No-op in headless mode
}

static inline void ui_iteration(void) {
    // No-op in headless mode
}

static inline void ui_show_hide_main_menu_window(void) {
    // No-op in headless mode
}

static inline void ui_popup_main_menu(void) {
    // No-op in headless mode
}

static inline void ui_hide_main_menu_window(void) {
    // No-op in headless mode
}

static inline int ui_show_yesno_dialog(char *format, ...) {
    // Return "yes" by default in headless mode
    return 1;
}

static inline void ui_show_error(char *format, ...) {
    // No-op in headless mode - errors will be logged to stderr elsewhere
}

static inline void ui_show_warning(char *format, ...) {
    // No-op in headless mode - warnings will be logged to stderr elsewhere
}

static inline void ui_main_update_cpu_speed_menu(unsigned state) {
    // No-op in headless mode
}

static inline void ui_main_update_emulation_state(unsigned state) {
    // No-op in headless mode
}

static inline void ui_main_update_rear_dip_switch_mz800_mode(unsigned state) {
    // No-op in headless mode
}

static inline void ui_main_update_rear_dip_switch_cmt_inverted_polarity(unsigned state) {
    // No-op in headless mode
}

static inline void ui_main_update_hwcompatibility_mz700_pal_timing(unsigned state) {
    // No-op in headless mode
}

static inline void ui_main_update_hwcompatibility_mz700_fixed_e008(unsigned state) {
    // No-op in headless mode
}

#ifdef MZ800EMU_CFG_DEBUGGER_ENABLED
static inline void ui_main_debugger_windows_refresh(void) {
    // No-op in headless mode
}
#endif

#ifdef __cplusplus
}
#endif

#endif /* UI_STUB_H */
