/* 
 * File:   debugger.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 23. srpna 2015, 16:19
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

#include "mz800emu_cfg.h"

#include <stdio.h>
#include <string.h>

#ifdef MZ800EMU_CFG_DEBUGGER_ENABLED


#include "debugger.h"
#include "ui/debugger/ui_debugger.h"
#include "ui/debugger/ui_breakpoints.h"
#include "ui/debugger/ui_membrowser.h"
#include "mz800.h"
#include "z80ex/include/z80ex.h"
#include "memory/memory.h"
#include "breakpoints.h"
#include "cfgfile/cfgtools.h"
#include "gdg/framebuffer.h"
#include "iface_sdl/iface_sdl.h"

#include "cfgmain.h"

st_DEBUGGER g_debugger;
st_DEBUGGER_HISTORY g_debugger_history;


void debugger_step_call ( unsigned value ) {

    value &= 1;
    if ( value == g_debugger.step_call ) return;

    if ( value ) {
#if 0
        if ( !TEST_EMULATION_PAUSED ) return;
#else
        /* Emulator neni v pauze, takze neprovedeme step, ale prozatim jen zastavime emulaci */
        if ( !TEST_EMULATION_PAUSED ) {
            //mz800_pause_emulation ( 1 );
            ui_debugger_pause_emulation ( );
            return;
        };
#endif
        g_debugger.step_call = 1;
        //printf ( "Debugger step call - PC: 0x%04x.\n", z80ex_get_reg ( g_mz800.cpu, regPC ) );
    } else {
        g_debugger.step_call = 0;
    };
}


void debugger_exit ( void ) {
    if ( g_debugger.auto_save_breakpoints ) {
        ui_breakpoints_save ( );
    };
}


void debugger_reset_history ( void ) {
    memset ( &g_debugger_history, 0x00, sizeof (g_debugger_history ) );
}


void debugger_membrowser_selected_addr_propagatecfg_cb ( void *e, void *data ) {
    char *encoded_txt = cfgelement_get_text_value ( (CFGELM *) e );

    int ret = EXIT_FAILURE;
    int value = cfgtools_strtol ( encoded_txt, &ret );

    if ( ret == EXIT_FAILURE ) {
        value = 0;
    };

    g_membrowser.selected_addr = value;
}


void debugger_membrowser_selected_addr_save_cb ( void *e, void *data ) {
    char value_txt[20];
    sprintf ( value_txt, "0x%04x", g_membrowser.selected_addr );
    cfgelement_set_text_value ( (CFGELM *) e, value_txt );
}


void debugger_init ( void ) {
    g_debugger.active = 0;
    g_debugger.memop_call = 0;
    debugger_step_call ( 0 );
    g_debugger.animated_updates = DEBUGGER_ANIMATED_UPDATES_ENABLED_WTHOUT_AUDIO;
    breakpoints_init ( );

    CFGMOD *cmod = cfgroot_register_new_module ( g_cfgmain, "DEBUGGER" );

    CFGELM *elm;
    elm = cfgmodule_register_new_element ( cmod, "animated_updates", CFGENTYPE_KEYWORD, DEBUGGER_ANIMATED_UPDATES_ENABLED_WTHOUT_AUDIO,
                                           0, "DISABLED",
                                           1, "ENABLED",
                                           2, "ENABLED_WITHOUT_AUDIO",
                                           -1 );
    cfgelement_set_handlers ( elm, (void*) &g_debugger.animated_updates, (void*) &g_debugger.animated_updates );

    elm = cfgmodule_register_new_element ( cmod, "auto_save_breakpoints", CFGENTYPE_BOOL, 1 );
    cfgelement_set_handlers ( elm, (void*) &g_debugger.auto_save_breakpoints, (void*) &g_debugger.auto_save_breakpoints );

    elm = cfgmodule_register_new_element ( cmod, "focus_to_addr_history", CFGENTYPE_TEXT, "0x0000" );
    cfgelement_set_propagate_cb ( elm, ui_debugger_focus_to_addr_history_propagatecfg_cb, NULL );
    cfgelement_set_save_cb ( elm, ui_debugger_focus_to_addr_history_save_cb, NULL );

    elm = cfgmodule_register_new_element ( cmod, "screen_refresh_on_edit", CFGENTYPE_BOOL, 0 );
    cfgelement_set_handlers ( elm, (void*) &g_debugger.screen_refresh_on_edit, (void*) &g_debugger.screen_refresh_on_edit );

    elm = cfgmodule_register_new_element ( cmod, "screen_refresh_at_step", CFGENTYPE_BOOL, 0 );
    cfgelement_set_handlers ( elm, (void*) &g_debugger.screen_refresh_at_step, (void*) &g_debugger.screen_refresh_at_step );

    // ui_membrowser
    elm = cfgmodule_register_new_element ( cmod, "membrowser_sharp_ascii", CFGENTYPE_BOOL, 0 );
    cfgelement_set_handlers ( elm, (void*) &g_membrowser.sh_ascii_conversion, (void*) &g_membrowser.sh_ascii_conversion );

    elm = cfgmodule_register_new_element ( cmod, "membrowser_show_comparative", CFGENTYPE_BOOL, 0 );
    cfgelement_set_handlers ( elm, (void*) &g_membrowser.show_comparative, (void*) &g_membrowser.show_comparative );

    elm = cfgmodule_register_new_element ( cmod, "membrowser_forced_screen_refresh", CFGENTYPE_BOOL, 0 );
    cfgelement_set_handlers ( elm, (void*) &g_membrowser.forced_screen_refresh, (void*) &g_membrowser.forced_screen_refresh );

    elm = cfgmodule_register_new_element ( cmod, "membrowser_memsrc", CFGENTYPE_KEYWORD, MEMBROWSER_SOURCE_MAPED,
                                           MEMBROWSER_SOURCE_MAPED, "MAPED",
                                           MEMBROWSER_SOURCE_RAM, "RAM",
                                           MEMBROWSER_SOURCE_VRAM, "VRAM",
                                           MEMBROWSER_SOURCE_MZ700ROM, "MZ700ROM",
                                           MEMBROWSER_SOURCE_CGROM, "CGROM",
                                           MEMBROWSER_SOURCE_MZ800ROM, "MZ800ROM",
                                           MEMBROWSER_SOURCE_PEZIK_E8, "PEZIK_E8",
                                           MEMBROWSER_SOURCE_PEZIK_68, "PEZIK_68",
                                           MEMBROWSER_SOURCE_MZ1R18, "MZ1R18",
                                           MEMBROWSER_SOURCE_MEMEXT_PEHU, "MEMEXT_PEHU",
                                           MEMBROWSER_SOURCE_MEMEXT_LUFTNER, "MEMEXT_LUFTNER",
                                           -1 );
    cfgelement_set_handlers ( elm, (void*) &g_membrowser.memsrc, (void*) &g_membrowser.memsrc );

    /*
        elm = cfgmodule_register_new_element ( cmod, "membrowser_page", CFGENTYPE_UNSIGNED, 0, 0, 0xffffffff );
        cfgelement_set_handlers ( elm, (void*) &g_membrowser.page, (void*) &g_membrowser.page );

        elm = cfgmodule_register_new_element ( cmod, "membrowser_selected_addr", CFGENTYPE_TEXT, "0x0000" );
        cfgelement_set_propagate_cb ( elm, debugger_membrowser_selected_addr_propagatecfg_cb, NULL );
        cfgelement_set_save_cb ( elm, debugger_membrowser_selected_addr_save_cb, NULL );
     */

    elm = cfgmodule_register_new_element ( cmod, "membrowser_pzik_addressing", CFGENTYPE_KEYWORD, MEMBROWSER_PEZIK_ADDRESSING_BE,
                                           MEMBROWSER_PEZIK_ADDRESSING_BE, "B_ENDIAN",
                                           MEMBROWSER_PEZIK_ADDRESSING_LE, "L_ENDIAN",
                                           -1 );
    cfgelement_set_handlers ( elm, (void*) &g_membrowser.pezik_addressing, (void*) &g_membrowser.pezik_addressing );

    elm = cfgmodule_register_new_element ( cmod, "membrowser_vram_plane", CFGENTYPE_UNSIGNED, 0, 0, 3 );
    cfgelement_set_handlers ( elm, (void*) &g_membrowser.vram_plane, (void*) &g_membrowser.vram_plane );

    elm = cfgmodule_register_new_element ( cmod, "membrowser_pezik_e8_bank", CFGENTYPE_UNSIGNED, 0, 0, 0x07 );
    cfgelement_set_handlers ( elm, (void*) &g_membrowser.pezik_bank[1], (void*) &g_membrowser.pezik_bank[1] );

    elm = cfgmodule_register_new_element ( cmod, "membrowser_pezik_68_bank", CFGENTYPE_UNSIGNED, 0, 0, 0x07 );
    cfgelement_set_handlers ( elm, (void*) &g_membrowser.pezik_bank[0], (void*) &g_membrowser.pezik_bank[0] );

    elm = cfgmodule_register_new_element ( cmod, "membrowser_mr1z18_bank", CFGENTYPE_UNSIGNED, 0, 0, 0xff );
    cfgelement_set_handlers ( elm, (void*) &g_membrowser.mr1z18_bank, (void*) &g_membrowser.mr1z18_bank );

    elm = cfgmodule_register_new_element ( cmod, "membrowser_pehu_bank", CFGENTYPE_UNSIGNED, 0, 0, 0x3f );
    cfgelement_set_handlers ( elm, (void*) &g_membrowser.memext_pehu_bank, (void*) &g_membrowser.memext_pehu_bank );

    elm = cfgmodule_register_new_element ( cmod, "membrowser_memext_luftner_bank", CFGENTYPE_UNSIGNED, 0, 0, 0xff );
    cfgelement_set_handlers ( elm, (void*) &g_membrowser.memext_luftner_bank, (void*) &g_membrowser.memext_luftner_bank );

    cfgmodule_parse ( cmod );
    cfgmodule_propagate ( cmod );
}


void debugger_show_main_window ( void ) {
    ui_debugger_show_main_window ( );
    debugger_step_call ( 0 );
    g_debugger.active = 1;
}


void debugger_hide_main_window ( void ) {
    ui_debugger_hide_main_window ( );
    debugger_step_call ( 0 );
    g_debugger.active = 0;
}


void debugger_show_hide_main_window ( void ) {
    if ( !TEST_DEBUGGER_ACTIVE ) {
        debugger_show_main_window ( );
    } else {
        debugger_hide_main_window ( );
    };
}


void debugger_update_all ( void ) {
    if ( !TEST_DEBUGGER_ACTIVE ) return;
    ui_debugger_update_all ( );
}


void debugger_animation ( void ) {
    if ( !TEST_DEBUGGER_ACTIVE ) return;
    ui_debugger_update_animated ( );
}


Z80EX_BYTE debugger_dasm_read_cb ( Z80EX_WORD addr, void *user_data ) {
    g_debugger.memop_call = 1;
    Z80EX_BYTE retval = memory_read_byte ( addr );
    g_debugger.memop_call = 0;
    return retval;
}


Z80EX_BYTE debugger_dasm_history_read_cb ( Z80EX_WORD addr, void *user_data ) {
    uint8_t *position = user_data;
    uint8_t retval = g_debugger_history.row[*position].byte[addr - g_debugger_history.row[*position].addr];
    return retval;
}


Z80EX_BYTE debugger_dasm_pure_ram_read_cb ( Z80EX_WORD addr, void *user_data ) {
    return memory_pure_ram_read_byte ( addr );
}


void debugger_memory_write_byte ( Z80EX_WORD addr, Z80EX_BYTE value ) {
    g_debugger.memop_call = 1;
    memory_write_byte ( addr, value );
    g_debugger.memop_call = 0;
}


void debugger_mmap_mount ( unsigned value ) {
    g_memory.map |= value;

    ui_debugger_update_mmap ( );
    ui_debugger_update_disassembled ( ui_debugger_dissassembled_get_first_addr ( ), -1 );
    ui_debugger_update_stack ( );
}


void debugger_mmap_umount ( unsigned value ) {
    g_memory.map &= ( ~value ) & 0x0f;

    ui_debugger_update_mmap ( );
    ui_debugger_update_disassembled ( ui_debugger_dissassembled_get_first_addr ( ), -1 );
    ui_debugger_update_stack ( );
}


void debugger_change_z80_flagbit ( unsigned flagbit, unsigned value ) {

    if ( !TEST_EMULATION_PAUSED ) {
        /* Toggle button neni potreba nastavovat zpet, protoze po pauze dojde k update */
        ui_debugger_pause_emulation ( );
        return;
    };

    Z80EX_WORD af_value = z80ex_get_reg ( g_mz800.cpu, regAF );

    if ( value ) {
        af_value |= 1 << flagbit;
    } else {
        af_value &= ~( 1 << flagbit );
    };

    z80ex_set_reg ( g_mz800.cpu, regAF, af_value );

    ui_debugger_update_registers ( );
}


void debugger_change_z80_register ( Z80_REG_T reg, Z80EX_WORD value ) {

    if ( !TEST_EMULATION_PAUSED ) {
        ui_debugger_pause_emulation ( );
        return;
    };

    if ( reg == regR ) {
        z80ex_set_reg ( g_mz800.cpu, regR, value & 0x7f );
        z80ex_set_reg ( g_mz800.cpu, regR7, value & 0x80 );
    } else {
        z80ex_set_reg ( g_mz800.cpu, reg, value );
    };

    ui_debugger_update_registers ( );

    if ( reg == regAF ) {
        ui_debugger_update_flag_reg ( );
    } else if ( reg == regPC ) {
        ui_debugger_update_disassembled ( z80ex_get_reg ( g_mz800.cpu, regPC ), -1 );
    } else if ( reg == regSP ) {
        ui_debugger_update_stack ( );
    };
}


void debugger_change_dmd ( Z80EX_BYTE value ) {

    if ( !TEST_EMULATION_PAUSED ) {
        /* Hodnotu comboboxu neni potreba nastavovat zpet, protoze po pauze dojde k update */
        ui_debugger_pause_emulation ( );
        return;
    };

    gdg_write_byte ( 0xce, value );

    ui_debugger_update_mmap ( );
    ui_debugger_update_disassembled ( ui_debugger_dissassembled_get_first_addr ( ), -1 );
    ui_debugger_update_stack ( );
}


void debugger_change_gdg_reg_border ( Z80EX_BYTE value ) {

    if ( !TEST_EMULATION_PAUSED ) {
        /* Hodnotu comboboxu neni potreba nastavovat zpet, protoze po pauze dojde k update */
        ui_debugger_pause_emulation ( );
        return;
    };

    gdg_write_byte ( 0x06cf, value );
}


void debugger_change_gdg_reg_palgrp ( Z80EX_BYTE value ) {

    if ( !TEST_EMULATION_PAUSED ) {
        /* Hodnotu comboboxu neni potreba nastavovat zpet, protoze po pauze dojde k update */
        ui_debugger_pause_emulation ( );
        return;
    };

    gdg_write_byte ( 0xf0, value | 0x40 );
}


void debugger_change_gdg_reg_pal ( Z80EX_BYTE pal, Z80EX_BYTE value ) {

    if ( !TEST_EMULATION_PAUSED ) {
        /* Hodnotu comboboxu neni potreba nastavovat zpet, protoze po pauze dojde k update */
        ui_debugger_pause_emulation ( );
        return;
    };

    gdg_write_byte ( 0xf0, value | ( pal << 4 ) );
}


void debugger_change_gdg_rfr ( Z80EX_BYTE value ) {

    if ( !TEST_EMULATION_PAUSED ) {
        /* Hodnotu comboboxu neni potreba nastavovat zpet, protoze po pauze dojde k update */
        ui_debugger_pause_emulation ( );
        return;
    };

    gdg_write_byte ( 0xcd, value );

    ui_debugger_update_disassembled ( ui_debugger_dissassembled_get_first_addr ( ), -1 );
    ui_debugger_update_stack ( );
}


void debugger_change_gdg_wfr ( Z80EX_BYTE value ) {

    if ( !TEST_EMULATION_PAUSED ) {
        /* Hodnotu comboboxu neni potreba nastavovat zpet, protoze po pauze dojde k update */
        ui_debugger_pause_emulation ( );
        return;
    };

    gdg_write_byte ( 0xcc, value );

    // pokud se nahodou zmenila i banka A/B, tak ta je pro WF a RF spolecna, takze musime udelat aktualizaci kvili cteni z pameti
    // TODO: fakt? :)
    ui_debugger_update_disassembled ( ui_debugger_dissassembled_get_first_addr ( ), -1 );
    ui_debugger_update_stack ( );
}


static Z80EX_WORD debugger_a2hex ( char c ) {

    if ( c >= '0' && c <= '9' ) {
        return ( c - '0' );
    };

    if ( c >= 'a' && c <= 'f' ) {
        return ( c - 'a' + 0x0a );
    };

    if ( c >= 'A' && c <= 'F' ) {
        return ( c - 'A' + 0x0a );
    };

    return 0;
}


uint32_t debuger_hextext_to_uint32 ( const char *txt ) {

    unsigned length;
    unsigned i;
    uint32_t value = 0;

    length = strlen ( txt );

    if ( length == 0 ) {
        return 0;
    };

    i = 0;
    while ( length ) {
        value += debugger_a2hex ( txt [ length - 1 ] ) << i;
        length--;
        i += 4;
    };

    return value;
}


void debugger_forced_screen_update ( void ) {
    g_iface_sdl.redraw_full_screen_request = 1;
    if ( DMD_TEST_MZ700 ) {
        framebuffer_update_MZ700_all_rows ( );
    } else {
        framebuffer_MZ800_all_screen_rows_fill ( );
    };
    framebuffer_border_all_rows_fill ( );
    iface_sdl_update_window ( );
}

#endif

