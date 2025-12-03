/* 
 * File:   ui_membrowser.h
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 14. června 2018, 12:09
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


#ifndef UI_MEMBROWSER_H
#define UI_MEMBROWSER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mz800emu_cfg.h"

#ifdef MZ800EMU_CFG_DEBUGGER_ENABLED

#include "ui/ui_main.h"
#include "ramdisk/ramdisk.h"


    typedef enum en_MEMBROWSER_SOURCE {
        MEMBROWSER_SOURCE_MAPED = 0,
        MEMBROWSER_SOURCE_RAM,
        MEMBROWSER_SOURCE_VRAM,
        MEMBROWSER_SOURCE_MZ700ROM,
        MEMBROWSER_SOURCE_CGROM,
        MEMBROWSER_SOURCE_MZ800ROM,
        /* externi pameti - pred praci s nimi kontrolujeme, zda jsou pripojeny */
        MEMBROWSER_SOURCE_PEZIK_E8,
        MEMBROWSER_SOURCE_PEZIK_68,
        MEMBROWSER_SOURCE_MZ1R18,
        MEMBROWSER_SOURCE_MEMEXT_PEHU,
        MEMBROWSER_SOURCE_MEMEXT_LUFTNER,
    } en_MEMBROWSER_SOURCE;


    typedef enum en_MEMBROWSER_PEZIK_ADDRESSING {
        MEMBROWSER_PEZIK_ADDRESSING_BE = 0,
        MEMBROWSER_PEZIK_ADDRESSING_LE
    } en_MEMBROWSER_PEZIK_ADDRESSING;


    typedef enum en_MEMBROWSER_MODE {
        MEMBROWSER_MODE_VIEW = 0,
        MEMBROWSER_MODE_EDIT_HEX,
        MEMBROWSER_MODE_EDIT_ASCII,
    } en_MEMBROWSER_MODE;


    typedef struct st_UI_MEMBROWSER {
        st_UIWINPOS main_pos;

        en_MEMBROWSER_SOURCE memsrc;
        int vram_plane;
        int pezik_bank[2];
        en_MEMBROWSER_PEZIK_ADDRESSING pezik_addressing;
        int mr1z18_bank;
        int memext_pehu_bank;
        int memext_luftner_bank;

        uint8_t *MEM;

        Z80EX_BYTE *data_current;
        Z80EX_BYTE *data_old;
        uint32_t mem_size;
        gint total_pages;

        gboolean show_comparative;
        gboolean sh_ascii_conversion;
        gboolean forced_screen_refresh;

        uint32_t selected_addr;
        gint page;
        en_MEMBROWSER_MODE mode;

        gboolean key_shift_l;
        gboolean key_shift_r;
        gboolean key_ctrl_l;
        gboolean key_ctrl_r;
        gboolean key_up;
        gboolean key_left;

        gboolean lock_textbuffer_changed;

        // text tagy
        GtkTextTag *tag_addr;
        GtkTextTag *tag_text;
        GtkTextTag *tag_text_changed;
        GtkTextTag *tag_text_selected;
        GtkTextTag *tag_edit;

        // vscale
        GtkAdjustment *page_adjustment;
        GtkWidget *page_vscale;
        gboolean lock_page_vscale;
        gboolean lock_source_combotext;
        gboolean lock_bank256_combobox;
        gboolean lock_bank_combotext;
        gboolean lock_pezik_addr_combotext;
        gboolean lock_goto_entry;

    } st_UI_MEMBROWSER;

    extern st_UI_MEMBROWSER g_membrowser;

    extern void ui_membrowser_show_hide ( void );
    extern void ui_membrowser_refresh ( void );

#endif

#ifdef __cplusplus
}
#endif

#endif /* UI_MEMBROWSER_H */

