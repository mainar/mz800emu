/* 
 * File:   ui_vkbd.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 24. února 2017, 14:07
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
 * Spravny pomer velikosti a rozlozeni klaves:
 * -------------------------------------------
 * 
 * Row 1 - 4 maji celkovou sirku 15 a 1/2 klavesy.
 * 
 * F1 - F5, INS, DEL, TAB, ESC a CURSORS maji sirku 1 a 1/2 klavesy.
 * 
 * CTRL a CR maji sirku 1 a 3/4 klavesy.
 * 
 * SHIFT_L ma sirku 1 a 1/4 klavesy.
 * 
 * SHIFT_R ma sirku 2 a 1/4 klavesy.
 * 
 * SPACEBAR ma sirku 9 klaves. Zacina soucasne s klavesou X a konci s klavesou slash.
 * 
 * CURSOR LEFT je od CR vzdalen na delku 1 a 1/5 klavesy.
 * 
 * F1 - F5, INS, DEL ma vysku 1/2 klavesy.
 * 
 * Mezera mezi row0 a row1 je 1/2 klavesy.
 * 
 */

#include <stdlib.h>
#include <string.h>
#include <gmodule.h>

#include "main.h"
#include "mz800.h"
#include "../ui_main.h"
#include "ui_vkbd.h"
#include "ui_vkbd_scancode.h"
#ifdef LINUX
#include "ui_vkbd_linux_x11.h"
#endif
#ifdef WINDOWS
#include "ui_vkbd_windows.h"
#endif

#include "pio8255/pio8255.h"

#include "ui/tools/ui_tool_pixbuf.h"

#define VKBD_BMP_DIR "ui_resources/vkbd/"


typedef struct st_VKBD_KEYDEF {
    en_VKBD_SCANCODE scancode;
    int mzcolumn;
    int mzbit;
    const char *filename;
} st_VKBD_KEYDEF;


static const st_VKBD_KEYDEF vk_row0a_def[] = {
    { VKBD_SCANCODE_F1, 9, 7, "row0/f1.bmp" },
    { VKBD_SCANCODE_F2, 9, 6, "row0/f2.bmp" },
    { VKBD_SCANCODE_F3, 9, 5, "row0/f3.bmp" },
    { VKBD_SCANCODE_F4, 9, 4, "row0/f4.bmp" },
    { VKBD_SCANCODE_F5, 9, 3, "row0/f5.bmp" },
    { 0, 0, 0, (char*) NULL },
};

static const st_VKBD_KEYDEF vk_row0b_def[] = {
    { VKBD_SCANCODE_INSERT, 7, 7, "row0/inst.bmp" },
    { VKBD_SCANCODE_DELETE, 7, 6, "row0/del.bmp" }, // + VKBD_SCANCODE_BACKSPACE
    { 0, 0, 0, (char*) NULL },
};

static const st_VKBD_KEYDEF vk_row1_def[] = {
    { VKBD_SCANCODE_CAPSLOCK, 0, 6, "row1/graph.bmp" },
    { VKBD_SCANCODE_1, 5, 7, "row1/1.bmp" },
    { VKBD_SCANCODE_2, 5, 6, "row1/2.bmp" },
    { VKBD_SCANCODE_3, 5, 5, "row1/3.bmp" },
    { VKBD_SCANCODE_4, 5, 4, "row1/4.bmp" },
    { VKBD_SCANCODE_5, 5, 3, "row1/5.bmp" },
    { VKBD_SCANCODE_6, 5, 2, "row1/6.bmp" },
    { VKBD_SCANCODE_7, 5, 1, "row1/7.bmp" },
    { VKBD_SCANCODE_8, 5, 0, "row1/8.bmp" },
    { VKBD_SCANCODE_9, 6, 2, "row1/9.bmp" },
    { VKBD_SCANCODE_0, 6, 3, "row1/0.bmp" },
    { VKBD_SCANCODE_MINUS, 6, 5, "row1/minus.bmp" },
    { VKBD_SCANCODE_EQUALS, 6, 6, "row1/arrow_up.bmp" },
    { VKBD_SCANCODE_F7, 6, 7, "row1/backslash.bmp" },
    { VKBD_SCANCODE_ESCAPE, 8, 7, "row1/break.bmp" }, // + VKBD_SCANCODE_END
    { 0, 0, 0, (char*) NULL },
};


static const st_VKBD_KEYDEF vk_row2_def[] = {
    { VKBD_SCANCODE_TAB, 0, 3, "row2/tab.bmp" },
    { VKBD_SCANCODE_Q, 2, 7, "row2/q.bmp" },
    { VKBD_SCANCODE_W, 2, 1, "row2/w.bmp" },
    { VKBD_SCANCODE_E, 4, 3, "row2/e.bmp" },
    { VKBD_SCANCODE_R, 2, 6, "row2/r.bmp" },
    { VKBD_SCANCODE_T, 2, 4, "row2/t.bmp" },
    { VKBD_SCANCODE_Y, 1, 7, "row2/y.bmp" },
    { VKBD_SCANCODE_U, 2, 3, "row2/u.bmp" },
    { VKBD_SCANCODE_I, 3, 7, "row2/i.bmp" },
    { VKBD_SCANCODE_O, 3, 1, "row2/o.bmp" },
    { VKBD_SCANCODE_P, 3, 0, "row2/p.bmp" },
    { VKBD_SCANCODE_F6, 1, 5, "row2/at.bmp" },
    { VKBD_SCANCODE_LEFTBRACKET, 1, 4, "row2/bracketleft.bmp" },
    { VKBD_SCANCODE_F9, 0, 5, "row2/libra.bmp" },
    { VKBD_SCANCODE_GRAVE, 0, 7, "row2/blank.bmp" },
    { 0, 0, 0, (char*) NULL },
};


static const st_VKBD_KEYDEF vk_row3_def[] = {
    { VKBD_SCANCODE_LCTRL, 8, 6, "row3/ctrl.bmp" },
    { VKBD_SCANCODE_A, 4, 7, "row3/a.bmp" },
    { VKBD_SCANCODE_S, 2, 5, "row3/s.bmp" },
    { VKBD_SCANCODE_D, 4, 4, "row3/d.bmp" },
    { VKBD_SCANCODE_F, 4, 2, "row3/f.bmp" },
    { VKBD_SCANCODE_G, 4, 1, "row3/g.bmp" },
    { VKBD_SCANCODE_H, 4, 0, "row3/h.bmp" },
    { VKBD_SCANCODE_J, 3, 6, "row3/j.bmp" },
    { VKBD_SCANCODE_K, 3, 5, "row3/k.bmp" },
    { VKBD_SCANCODE_L, 3, 4, "row3/l.bmp" },
    { VKBD_SCANCODE_SEMICOLON, 0, 2, "row3/semicolon.bmp" },
    { VKBD_SCANCODE_APOSTROPHE, 0, 1, "row3/colon.bmp" },
    { VKBD_SCANCODE_RIGHTBRACKET, 1, 3, "row3/bracketright.bmp" },
    { VKBD_SCANCODE_RETURN, 0, 0, "row3/cr.bmp" }, // SDL_SCANCODE_RETURN2 A SDL_SCANCODE_KP_ENTER
    { 0, 0, 0, (char*) NULL },
};


static const st_VKBD_KEYDEF vk_row4_def[] = {
    { VKBD_SCANCODE_LSHIFT, 8, 0, "row4/shift_l.bmp" }, // duplicitni funkce jako VKBD_SCANCODE_RSHIFT
    { VKBD_SCANCODE_BACKSLASH, 0, 4, "row4/alpha.bmp" },
    { VKBD_SCANCODE_Z, 1, 6, "row4/z.bmp" },
    { VKBD_SCANCODE_X, 2, 0, "row4/x.bmp" },
    { VKBD_SCANCODE_C, 4, 5, "row4/c.bmp" },
    { VKBD_SCANCODE_V, 2, 2, "row4/v.bmp" },
    { VKBD_SCANCODE_B, 4, 6, "row4/b.bmp" },
    { VKBD_SCANCODE_N, 3, 2, "row4/n.bmp" },
    { VKBD_SCANCODE_M, 3, 3, "row4/m.bmp" },
    { VKBD_SCANCODE_COMMA, 6, 1, "row4/coma.bmp" },
    { VKBD_SCANCODE_PERIOD, 6, 0, "row4/period.bmp" },
    { VKBD_SCANCODE_SLASH, 7, 0, "row4/slash.bmp" },
    { VKBD_SCANCODE_F8, 7, 1, "row4/question.bmp" },
    { VKBD_SCANCODE_RSHIFT, 8, 0, "row4/shift_r.bmp" }, // duplicitni funkce jako VKBD_SCANCODE_LSHIFT
    { 0, 0, 0, (char*) NULL },
};


static const st_VKBD_KEYDEF vk_row5_def[] = {
    { VKBD_SCANCODE_SPACE, 6, 4, "row5/space.bmp" },
    { 0, 0, 0, (char*) NULL },
};


static const st_VKBD_KEYDEF vk_cursorUp_def[] = {
    { VKBD_SCANCODE_UP, 7, 5, "cursor_keys/up.bmp" },
    { 0, 0, 0, (char*) NULL },
};


static const st_VKBD_KEYDEF vk_cursorLR_def[] = {
    { VKBD_SCANCODE_LEFT, 7, 2, "cursor_keys/left.bmp" },
    { VKBD_SCANCODE_RIGHT, 7, 3, "cursor_keys/right.bmp" },
    { 0, 0, 0, (char*) NULL },
};


static const st_VKBD_KEYDEF vk_cursorDown_def[] = {
    { VKBD_SCANCODE_DOWN, 7, 4, "cursor_keys/down.bmp" },
    { 0, 0, 0, (char*) NULL },
};


typedef struct st_VKBD_OPTKEYDEF {
    en_VKBD_SCANCODE aditional_scancode;
    en_VKBD_SCANCODE scancode;
} st_VKBD_OPTKEYDEF;


static const st_VKBD_OPTKEYDEF vk_optdef[] = {
    { VKBD_SCANCODE_BACKSPACE, VKBD_SCANCODE_DELETE },
    { VKBD_SCANCODE_END, VKBD_SCANCODE_ESCAPE },
    { VKBD_SCANCODE_RETURN2, VKBD_SCANCODE_RETURN },
    { VKBD_SCANCODE_KP_ENTER, VKBD_SCANCODE_RETURN },
    { 0, 0 },
};

typedef void ( *vkbd_optfunc_cb )(void);


typedef struct st_VKBD_OPTFUNC {
    en_VKBD_SCANCODE scancode;
    vkbd_optfunc_cb callback;
} st_VKBD_OPTFUNC;


static const st_VKBD_OPTFUNC vk_optfunc[] = {
    { VKBD_SCANCODE_F12, mz800_reset },
    { 0, NULL }
};


typedef enum en_VKBD_ACT_CALLER {
    VK_ACTCALL_NONE = 0,
    VK_ACTCALL_MOUSE = 1,
    VK_ACTCALL_KEY1 = 2,
    VK_ACTCALL_KEY2 = 4,
} en_VKBD_ACT_CALLER;

#define VK_ALL_ACTCALLERS ( VK_ACTCALL_MOUSE | VK_ACTCALL_KEY1 | VK_ACTCALL_KEY2 )


typedef struct st_VKBD_KEY {
    const st_VKBD_KEYDEF *keydef;
    en_VKBD_ACT_CALLER act_caller;
    GtkWidget *src_img;
    GtkWidget *dst_img;
    GtkWidget *eventbox;
    GtkWidget *drawing_area;
} st_VKBD_KEY;


typedef struct st_VKBD_ROW_SZ1 {
    GtkWidget *box;
    int numkeys;
    st_VKBD_KEY key[1];
} st_VKBD_ROW_SZ1;


typedef struct st_VKBD_ROW_SZ2 {
    GtkWidget *box;
    int numkeys;
    st_VKBD_KEY key[2];
} st_VKBD_ROW_SZ2;


typedef struct st_VKBD_ROW_SZ5 {
    GtkWidget *box;
    int numkeys;
    st_VKBD_KEY key[5];
} st_VKBD_ROW_SZ5;


typedef struct st_VKBD_ROW_SZ14 {
    GtkWidget *box;
    int numkeys;
    st_VKBD_KEY key[14];
} st_VKBD_ROW_SZ14;


typedef struct st_VKBD_ROW_SZ15 {
    GtkWidget *box;
    int numkeys;
    st_VKBD_KEY key[15];
} st_VKBD_ROW_SZ15;


typedef struct st_VKBD {
    GtkWidget *window;
    GtkWidget *fixed;
    st_VKBD_ROW_SZ5 row0a;
    st_VKBD_ROW_SZ2 row0b;
    st_VKBD_ROW_SZ15 row1;
    st_VKBD_ROW_SZ15 row2;
    st_VKBD_ROW_SZ14 row3;
    st_VKBD_ROW_SZ14 row4;
    st_VKBD_ROW_SZ1 row5;
    st_VKBD_ROW_SZ1 cursorUp;
    st_VKBD_ROW_SZ2 cursorLR;
    st_VKBD_ROW_SZ1 cursorDown;
    int spacebar_src_width_requested;
} st_VKBD;

static st_VKBD g_vkbd;

#define VKBD_LSHIFT g_vkbd.row4.key[0]
#define VKBD_RSHIFT g_vkbd.row4.key[13]

static void *g_vkbd_allRows[] = {
                                 &g_vkbd.row0a,
                                 &g_vkbd.row0b,
                                 &g_vkbd.row1,
                                 &g_vkbd.row2,
                                 &g_vkbd.row3,
                                 &g_vkbd.row4,
                                 &g_vkbd.row5,
                                 &g_vkbd.cursorUp,
                                 &g_vkbd.cursorLR,
                                 &g_vkbd.cursorDown,
                                 NULL,
};

static gboolean g_vkbd_is_initialised = FALSE;

#define UI_VKBD_DSTIMG_TOP_BORDER   1
#define UI_VKBD_DSTIMG_LEFT_BORDER  1

#define UI_VKBD_PRESSED_X_SHIFT     2
#define UI_VKBD_PRESSED_Y_SHIFT     2


static void ui_vkbd_copy_src2dst ( st_VKBD_KEY *vk_key, int shiftX, int shiftY ) {

    GdkPixbuf *src_pixbuf = gtk_image_get_pixbuf ( GTK_IMAGE ( vk_key->src_img ) );
    GdkPixbuf *dst_pixbuf = gtk_image_get_pixbuf ( GTK_IMAGE ( vk_key->dst_img ) );

    gint n_channels = gdk_pixbuf_get_n_channels ( src_pixbuf );

    g_assert ( gdk_pixbuf_get_colorspace ( src_pixbuf ) == GDK_COLORSPACE_RGB );
    g_assert ( gdk_pixbuf_get_bits_per_sample ( src_pixbuf ) == 8 );

    gint src_width = gdk_pixbuf_get_width ( src_pixbuf );
    gint src_height = gdk_pixbuf_get_height ( src_pixbuf );

    gint dst_width = gdk_pixbuf_get_width ( dst_pixbuf );
    gint dst_height = gdk_pixbuf_get_height ( dst_pixbuf );

    gint src_rowstride = gdk_pixbuf_get_rowstride ( src_pixbuf );
    gint dst_rowstride = gdk_pixbuf_get_rowstride ( dst_pixbuf );

    guchar *src_pixels = gdk_pixbuf_get_pixels ( src_pixbuf );
    guchar *dst_pixels = gdk_pixbuf_get_pixels ( dst_pixbuf );

    gint dst_img_area_X = dst_width - shiftX;
    gint dst_img_area_Y = dst_height - shiftY;

    gint copy_width = ( src_width > dst_img_area_X ) ? dst_img_area_X : src_width;

    gint copy_height = ( src_height > dst_img_area_Y ) ? dst_img_area_Y : src_height;

    gint src_y = 0;
    gint dst_y;

    for ( dst_y = shiftY; dst_y < ( shiftY + copy_height ); dst_y++ ) {
        guchar *src = src_pixels + src_y * src_rowstride;
        guchar *dst = dst_pixels + dst_y * dst_rowstride + shiftX * n_channels;
        memcpy ( dst, src, copy_width * n_channels );
        src_y++;
    };

}


static void ui_vkbd_make_dst_key_up ( st_VKBD_KEY *vk_key ) {

    ui_vkbd_copy_src2dst ( vk_key, UI_VKBD_DSTIMG_TOP_BORDER, UI_VKBD_DSTIMG_LEFT_BORDER );

#if UI_VKBD_DSTIMG_TOP_BORDER || UI_VKBD_DSTIMG_LEFT_BORDER
    GdkPixbuf *dst_pixbuf = gtk_image_get_pixbuf ( GTK_IMAGE ( vk_key->dst_img ) );
#endif

#if UI_VKBD_DSTIMG_TOP_BORDER
    gint width = gdk_pixbuf_get_width ( dst_pixbuf );
    ui_tool_pixbuf_create_horizontal_line ( dst_pixbuf, 0, 0, width, UI_VKBD_DSTIMG_TOP_BORDER, 0xffffff );
#endif

#if UI_VKBD_DSTIMG_LEFT_BORDER
    gint height = gdk_pixbuf_get_height ( dst_pixbuf );
    ui_tool_pixbuf_create_vertical_line ( dst_pixbuf, 0, 0, height, UI_VKBD_DSTIMG_LEFT_BORDER, 0xffffff );
#endif

}


static void ui_vkbd_make_dst_key_down ( st_VKBD_KEY *vk_key ) {

    GdkPixbuf *dst_pixbuf = gtk_image_get_pixbuf ( GTK_IMAGE ( vk_key->dst_img ) );

    gint width = gdk_pixbuf_get_width ( dst_pixbuf );
    gint height = gdk_pixbuf_get_height ( dst_pixbuf );

#if UI_VKBD_PRESSED_X_SHIFT || UI_VKBD_PRESSED_Y_SHIFT
    ui_vkbd_copy_src2dst ( vk_key, UI_VKBD_DSTIMG_TOP_BORDER + UI_VKBD_PRESSED_Y_SHIFT, UI_VKBD_DSTIMG_LEFT_BORDER + UI_VKBD_PRESSED_X_SHIFT );
#endif

#if UI_VKBD_DSTIMG_TOP_BORDER
    ui_tool_pixbuf_create_horizontal_line ( dst_pixbuf, 0, 0, width, UI_VKBD_DSTIMG_TOP_BORDER, 0x808080 );
#endif

#if UI_VKBD_PRESSED_Y_SHIFT
    ui_tool_pixbuf_create_horizontal_line ( dst_pixbuf, 0, 0 + UI_VKBD_DSTIMG_TOP_BORDER, width, UI_VKBD_PRESSED_Y_SHIFT, 0x000000 );
#endif

#if UI_VKBD_DSTIMG_LEFT_BORDER
    ui_tool_pixbuf_create_vertical_line ( dst_pixbuf, 0, 0, height, UI_VKBD_DSTIMG_LEFT_BORDER, 0x808080 );
#endif

#if UI_VKBD_PRESSED_X_SHIFT
    ui_tool_pixbuf_create_vertical_line ( dst_pixbuf, 0 + UI_VKBD_DSTIMG_LEFT_BORDER, 0 + UI_VKBD_DSTIMG_TOP_BORDER, height - UI_VKBD_DSTIMG_TOP_BORDER, UI_VKBD_PRESSED_X_SHIFT, 0x000000 );
#endif
}


static GtkWidget* ui_vkbd_create_src_key_img ( const char *filename ) {

    char *full_filename = malloc ( strlen ( VKBD_BMP_DIR ) + strlen ( filename ) + 1 );
    strcpy ( full_filename, VKBD_BMP_DIR );
    strcat ( full_filename, filename );
    GtkWidget *img = gtk_image_new_from_file ( full_filename );

    if ( GTK_IMAGE_PIXBUF != gtk_image_get_storage_type ( GTK_IMAGE ( img ) ) ) {
        ui_show_error ( "%s() - can't create pixbuf from %s\n", __func__, full_filename );
        free ( full_filename );
        main_app_quit ( EXIT_FAILURE );
    };
    free ( full_filename );
    return img;
}


static void ui_vkbd_init_key_img ( st_VKBD_KEY *vk_key, const st_VKBD_KEYDEF *keydef ) {

    vk_key->keydef = keydef;
    vk_key->src_img = NULL;
    vk_key->act_caller = VK_ACTCALL_NONE;

    if ( ( keydef->scancode == VKBD_SCANCODE_SPACE ) && ( g_vkbd.spacebar_src_width_requested ) && ( UI_VKBD_DSTIMG_TOP_BORDER ) ) {
        GtkWidget *loaded_img = ui_vkbd_create_src_key_img ( keydef->filename );
        GdkPixbuf *loaded_pixbuf = gtk_image_get_pixbuf ( GTK_IMAGE ( loaded_img ) );
        gint loaded_width = gdk_pixbuf_get_width ( loaded_pixbuf );

        if ( g_vkbd.spacebar_src_width_requested > loaded_width ) {

            gint loaded_height = gdk_pixbuf_get_height ( loaded_pixbuf );
            gint n_channels = gdk_pixbuf_get_n_channels ( loaded_pixbuf );

            GdkPixbuf *resized_pixbuf = gdk_pixbuf_new ( gdk_pixbuf_get_colorspace ( loaded_pixbuf ),
                                                         gdk_pixbuf_get_has_alpha ( loaded_pixbuf ),
                                                         gdk_pixbuf_get_bits_per_sample ( loaded_pixbuf ),
                                                         g_vkbd.spacebar_src_width_requested,
                                                         loaded_height );

            g_assert ( gdk_pixbuf_get_colorspace ( loaded_pixbuf ) == GDK_COLORSPACE_RGB );
            g_assert ( gdk_pixbuf_get_bits_per_sample ( loaded_pixbuf ) == 8 );

            gint loaded_rowstride = gdk_pixbuf_get_rowstride ( loaded_pixbuf );
            gint resized_rowstride = gdk_pixbuf_get_rowstride ( resized_pixbuf );

            guchar *loaded_pixels = gdk_pixbuf_get_pixels ( loaded_pixbuf );
            guchar *resized_pixels = gdk_pixbuf_get_pixels ( resized_pixbuf );

            int y;

            int half_loaded_width = loaded_width / 2;
            int add_pixels = g_vkbd.spacebar_src_width_requested - loaded_width;
            if ( loaded_width & 1 ) {
                add_pixels++;
            };

            for ( y = 0; y < loaded_height; y++ ) {

                guchar *src = loaded_pixels + y * loaded_rowstride;
                guchar *dst = resized_pixels + y * resized_rowstride;

                memcpy ( dst, src, half_loaded_width * n_channels );

                src += half_loaded_width * n_channels;
                dst += half_loaded_width * n_channels;

                int i;
                for ( i = 0; i < add_pixels; i++ ) {
                    memcpy ( dst, src, n_channels );
                    dst += n_channels;
                };

                src += n_channels;

                memcpy ( dst, src, half_loaded_width * n_channels );
            };

            vk_key->src_img = gtk_image_new_from_pixbuf ( resized_pixbuf );

        } else {
            vk_key->src_img = loaded_img;
        };
    };

    if ( vk_key->src_img == NULL ) {
        vk_key->src_img = ui_vkbd_create_src_key_img ( keydef->filename );
    };

    GdkPixbuf *src_pixbuf = gtk_image_get_pixbuf ( GTK_IMAGE ( vk_key->src_img ) );

    GdkPixbuf *dst_pixbuf = gdk_pixbuf_new ( gdk_pixbuf_get_colorspace ( src_pixbuf ),
                                             gdk_pixbuf_get_has_alpha ( src_pixbuf ),
                                             gdk_pixbuf_get_bits_per_sample ( src_pixbuf ),
                                             gdk_pixbuf_get_width ( src_pixbuf ) + UI_VKBD_DSTIMG_TOP_BORDER,
                                             gdk_pixbuf_get_height ( src_pixbuf ) + UI_VKBD_DSTIMG_LEFT_BORDER );

    vk_key->dst_img = gtk_image_new_from_pixbuf ( dst_pixbuf );

    ui_vkbd_make_dst_key_up ( vk_key );
}


static void ui_vkbd_key_event ( st_VKBD_KEY *vk_key, en_VKBD_ACT_CALLER act_caller, gboolean keypress ) {

    if ( keypress ) {
        vk_key->act_caller |= act_caller;
    } else {
        vk_key->act_caller &= ~act_caller;
    };

    if ( ( vk_key == &VKBD_LSHIFT ) || ( vk_key == &VKBD_RSHIFT ) ) {
        if ( VKBD_LSHIFT.act_caller | VKBD_RSHIFT.act_caller ) {
            ui_vkbd_make_dst_key_down ( &VKBD_LSHIFT );
            ui_vkbd_make_dst_key_down ( &VKBD_RSHIFT );
            PIO8255_VKBDBIT_RESET ( vk_key->keydef->mzcolumn, vk_key->keydef->mzbit );
        } else {
            ui_vkbd_make_dst_key_up ( &VKBD_LSHIFT );
            ui_vkbd_make_dst_key_up ( &VKBD_RSHIFT );
            PIO8255_VKBDBIT_SET ( vk_key->keydef->mzcolumn, vk_key->keydef->mzbit );
        };
        gtk_widget_queue_draw ( VKBD_LSHIFT.drawing_area );
        gtk_widget_queue_draw ( VKBD_RSHIFT.drawing_area );
    } else {
        if ( vk_key->act_caller ) {
            ui_vkbd_make_dst_key_down ( vk_key );
            PIO8255_VKBDBIT_RESET ( vk_key->keydef->mzcolumn, vk_key->keydef->mzbit );
        } else {
            ui_vkbd_make_dst_key_up ( vk_key );
            PIO8255_VKBDBIT_SET ( vk_key->keydef->mzcolumn, vk_key->keydef->mzbit );
        };
        gtk_widget_queue_draw ( vk_key->drawing_area );
    };
}


static gboolean on_dst_img_button_press_event ( GtkWidget *widget, GdkEventButton *event, gpointer user_data ) {
    st_VKBD_KEY *vk_key = user_data;
    ui_vkbd_key_event ( vk_key, VK_ACTCALL_MOUSE, TRUE );
    return FALSE;
}


static gboolean on_dst_img_button_release_event ( GtkWidget *widget, GdkEventButton *event, gpointer user_data ) {
    st_VKBD_KEY *vk_key = user_data;
    ui_vkbd_key_event ( vk_key, VK_ACTCALL_MOUSE, FALSE );
    return FALSE;
}


static en_VKBD_SCANCODE ui_vkbd_KeycodeToScancode ( guint16 hardware_keycode ) {

    en_VKBD_SCANCODE scancode = VKBD_SCANCODE_UNKNOWN;

#ifdef LINUX    
    //GdkKeymap *km = gdk_keymap_get_default ( );
    GdkDisplay *disp = gdk_display_get_default ( );
    GdkKeymap *km = gdk_keymap_get_for_display ( disp );
    GdkKeymapKey *keys = NULL;
    guint *keyvals = NULL;
    gint entries = 0;

    gboolean key_found = gdk_keymap_get_entries_for_keycode ( km, hardware_keycode, &keys, &keyvals, &entries );

    if ( key_found ) {
        scancode = ui_vkbd_linux_x11_KeysymToScancode ( keyvals[0] );
    };

#if 0
    if ( !key_found ) {
        printf ( "Unknown key: keycode = 0x%02x, %d, keyval!\n", hardware_keycode, hardware_keycode );

        printf ( "\tkey_found: %d, entries: %d\n", key_found, entries );

        int i;
        for ( i = 0; i < entries; i++ ) {
            printf ( "\t\t%d. kv: 0x%02x\n", i, keyvals[i] );
        };
    };
#endif

    g_free ( keys );
    g_free ( keyvals );
#endif // LINUX

#ifdef WINDOWS
    scancode = ui_vkbd_windows_KeycodeToScancode ( hardware_keycode );
#endif // WINDOWS

    return scancode;
}


static gboolean ui_vkbd_optfnc_action ( en_VKBD_SCANCODE scancode ) {
    int i = 0;
    while ( vk_optfunc[i].scancode != 0 ) {
        if ( vk_optfunc[i].scancode == scancode ) {
            vk_optfunc[i].callback ( );
            return TRUE;
        };
        i++;
    };
    return FALSE;
}


static void ui_vkbd_key_press_release_event ( guint16 hardware_keycode, gboolean keypress ) {

    en_VKBD_SCANCODE scancode = ui_vkbd_KeycodeToScancode ( hardware_keycode );

    if ( scancode == VKBD_SCANCODE_UNKNOWN ) return;

    if ( ui_vkbd_optfnc_action ( scancode ) ) return;

    en_VKBD_ACT_CALLER act_caller = VK_ACTCALL_KEY1;

    int i = 0;
    while ( vk_optdef[i].aditional_scancode != 0 ) {
        if ( vk_optdef[i].aditional_scancode == scancode ) {
            scancode = vk_optdef[i].scancode;
            act_caller = VK_ACTCALL_KEY2;
            break;
        };
        i++;
    };

    int row_block = 0;
    while ( g_vkbd_allRows[row_block] != NULL ) {
        st_VKBD_ROW_SZ15 *r = g_vkbd_allRows[row_block];
        int i;
        for ( i = 0; i < r->numkeys; i++ ) {
            st_VKBD_KEY *k = &r->key[i];
            if ( k->keydef->scancode == scancode ) {
                //printf ( "found(%d): %s\n", keypress, k->keydef->filename );
                ui_vkbd_key_event ( k, act_caller, keypress );
                return;
            };
        };
        row_block++;
    };
}


static gboolean on_window_key_press_event ( GtkWidget *widget, GdkEventKey *event, gpointer user_data ) {
    ui_vkbd_key_press_release_event ( event->hardware_keycode, TRUE );
    return TRUE;
}


static gboolean on_window_key_release_event ( GtkWidget *widget, GdkEventKey *event, gpointer user_data ) {
    ui_vkbd_key_press_release_event ( event->hardware_keycode, FALSE );
    return TRUE;
}


static gboolean on_window_delete_event ( GtkWidget *widget, GdkEvent *event, gpointer user_data ) {
    gtk_widget_hide ( g_vkbd.window );
    return TRUE;
}


static gboolean on_drawing_area_draw ( GtkWidget *widget, cairo_t *cr, gpointer user_data ) {
    st_VKBD_KEY *vk_key = user_data;
    GdkPixbuf *pixbuf = gtk_image_get_pixbuf ( GTK_IMAGE ( vk_key->dst_img ) );
    gdk_cairo_set_source_pixbuf ( cr, pixbuf, 0, 0 );
    cairo_paint ( cr );
    return TRUE;
}


static void ui_vkbd_make_key ( GtkWidget *parent, st_VKBD_KEY *vk_key, const st_VKBD_KEYDEF *keydef ) {

    ui_vkbd_init_key_img ( vk_key, keydef );

    vk_key->eventbox = gtk_event_box_new ( );
    gtk_widget_show ( vk_key->eventbox );
    gtk_widget_set_size_request ( vk_key->eventbox, 0, 0 );

    gtk_box_pack_start ( GTK_BOX ( parent ), vk_key->eventbox, TRUE, TRUE, 0 );

    vk_key->drawing_area = gtk_drawing_area_new ( );

    GdkPixbuf *pixbuf = gtk_image_get_pixbuf ( GTK_IMAGE ( vk_key->dst_img ) );

    gtk_widget_set_size_request ( vk_key->drawing_area, gdk_pixbuf_get_width ( pixbuf ), gdk_pixbuf_get_height ( pixbuf ) );

    g_signal_connect ( G_OBJECT ( vk_key->drawing_area ), "draw",
                       G_CALLBACK ( on_drawing_area_draw ), vk_key );

    gtk_widget_show ( vk_key->drawing_area );

    gtk_container_add ( GTK_CONTAINER ( vk_key->eventbox ), vk_key->drawing_area );

    g_signal_connect ( (gpointer) vk_key->eventbox, "button_press_event",
                       G_CALLBACK ( on_dst_img_button_press_event ),
                       vk_key );

    g_signal_connect ( (gpointer) vk_key->eventbox, "button_release_event",
                       G_CALLBACK ( on_dst_img_button_release_event ),
                       vk_key );

}


static void ui_vkbd_make_row ( int posY, int posX, void *row, const st_VKBD_KEYDEF *keydef ) {

    st_VKBD_ROW_SZ15 *r = row;

    r->numkeys = 0;
    while ( keydef[r->numkeys].filename != NULL ) {
        r->numkeys++;
    };

    r->box = gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, r->numkeys );
    gtk_widget_show ( r->box );

    gtk_fixed_put ( GTK_FIXED ( g_vkbd.fixed ), r->box, posX, posY );
    gtk_box_set_homogeneous ( GTK_BOX ( r->box ), FALSE );
    gtk_box_set_spacing ( GTK_BOX ( r->box ), 0 );
    gtk_widget_set_size_request ( r->box, 0, 0 );

    int i;
    for ( i = 0; i < r->numkeys; i++ ) {
        ui_vkbd_make_key ( r->box, &r->key[i], &keydef[i] );
    };
}


#define UI_VKB_TOP_BORDER     15
#define UI_VKB_LEFT_BORDER    15
#define UI_VKB_RIGHT_BORDER   15
#define UI_VKB_BOTOM_BORDER   15


static gint ui_vkbd_get_dst_imgs_width ( st_VKBD_KEY *vk_key, int from, int count ) {

    gint imgs_width = 0;

    int i;
    for ( i = from; i < ( from + count ); i++ ) {
        GdkPixbuf *pixbuf = gtk_image_get_pixbuf ( GTK_IMAGE ( vk_key[i].dst_img ) );
        imgs_width += gdk_pixbuf_get_width ( pixbuf );
    };

    return imgs_width;
}


static void ui_vkbd_create ( void ) {

    g_vkbd.window = gtk_window_new ( GTK_WINDOW_TOPLEVEL );
    gtk_window_set_title ( GTK_WINDOW ( g_vkbd.window ), ( "Virtual Keyboard" ) );
    gtk_window_set_resizable ( GTK_WINDOW ( g_vkbd.window ), FALSE );
    gtk_window_set_type_hint ( GTK_WINDOW ( g_vkbd.window ), GDK_WINDOW_TYPE_HINT_UTILITY );

    g_signal_connect ( (gpointer) g_vkbd.window, "delete_event",
                       G_CALLBACK ( on_window_delete_event ),
                       NULL );

    g_signal_connect ( (gpointer) g_vkbd.window, "key_press_event",
                       G_CALLBACK ( on_window_key_press_event ),
                       NULL );

    g_signal_connect ( (gpointer) g_vkbd.window, "key_release_event",
                       G_CALLBACK ( on_window_key_release_event ),
                       NULL );

    g_vkbd.fixed = gtk_fixed_new ( );
    gtk_widget_show ( g_vkbd.fixed );
    gtk_container_add ( GTK_CONTAINER ( g_vkbd.window ), g_vkbd.fixed );


    int y = UI_VKB_TOP_BORDER;

    ui_vkbd_make_row ( y, UI_VKB_LEFT_BORDER, &g_vkbd.row0a, vk_row0a_def );

    GdkPixbuf *pixbuf = gtk_image_get_pixbuf ( GTK_IMAGE ( g_vkbd.row0a.key[0].dst_img ) );
    int key_height = 2 * gdk_pixbuf_get_height ( pixbuf );
    y += key_height;

    ui_vkbd_make_row ( y, UI_VKB_LEFT_BORDER, &g_vkbd.row1, vk_row1_def );
    y += key_height;

    ui_vkbd_make_row ( y, UI_VKB_LEFT_BORDER, &g_vkbd.row2, vk_row2_def );
    y += key_height;

    ui_vkbd_make_row ( y, UI_VKB_LEFT_BORDER, &g_vkbd.row3, vk_row3_def );
    y += key_height;

    ui_vkbd_make_row ( y, UI_VKB_LEFT_BORDER, &g_vkbd.row4, vk_row4_def );

    int spacebar_startX = UI_VKB_LEFT_BORDER + ui_vkbd_get_dst_imgs_width ( &g_vkbd.row4.key[0], 0, 3 );
    g_vkbd.spacebar_src_width_requested = ui_vkbd_get_dst_imgs_width ( &g_vkbd.row4.key[0], 3, 9 ) - UI_VKBD_DSTIMG_TOP_BORDER;

    y += key_height;
    ui_vkbd_make_row ( y, spacebar_startX, &g_vkbd.row5, vk_row5_def );

    gint f1_width = ui_vkbd_get_dst_imgs_width ( &g_vkbd.row0a.key[0], 0, 1 );
    gint row3_width = ui_vkbd_get_dst_imgs_width ( &g_vkbd.row3.key[0], 0, 14 );

    gint rightKeysX = UI_VKB_LEFT_BORDER + row3_width + f1_width;

    ui_vkbd_make_row ( UI_VKB_TOP_BORDER, rightKeysX, &g_vkbd.row0b, vk_row0b_def );

    ui_vkbd_make_row ( UI_VKB_TOP_BORDER + 3 * key_height, rightKeysX, &g_vkbd.cursorLR, vk_cursorLR_def );

    gint half_cursorKey_width = ui_vkbd_get_dst_imgs_width ( &g_vkbd.cursorLR.key[0], 0, 1 ) / 2;

    ui_vkbd_make_row ( UI_VKB_TOP_BORDER + 2 * key_height, rightKeysX + half_cursorKey_width, &g_vkbd.cursorUp, vk_cursorUp_def );
    ui_vkbd_make_row ( UI_VKB_TOP_BORDER + 4 * key_height, rightKeysX + half_cursorKey_width, &g_vkbd.cursorDown, vk_cursorDown_def );

    gint row0b_width = ui_vkbd_get_dst_imgs_width ( &g_vkbd.row0b.key[0], 0, 2 );

    gtk_widget_set_size_request ( g_vkbd.fixed,
                                  UI_VKB_LEFT_BORDER + row3_width + f1_width + row0b_width + UI_VKB_RIGHT_BORDER,
                                  UI_VKB_TOP_BORDER + 6 * key_height + UI_VKB_BOTOM_BORDER );
}


void ui_vkbd_show_hide ( void ) {

    if ( !g_vkbd_is_initialised ) {
        ui_vkbd_create ( );
        g_vkbd_is_initialised = TRUE;
    };

    if ( gtk_widget_get_visible ( g_vkbd.window ) ) {
        gtk_widget_hide ( g_vkbd.window );
    } else {
        gtk_widget_show ( g_vkbd.window );
    };

}


G_MODULE_EXPORT void on_menuitem_keyboard_virtual_show_activate ( GtkCheckMenuItem *menuitem, gpointer data ) {
    (void) menuitem;
    (void) data;
#ifdef UI_TOPMENU_IS_WINDOW
    ui_hide_main_menu_window ( );
#endif
    ui_vkbd_show_hide ( );
}


void ui_vkbd_reset_keyboard_state ( void ) {

    if ( !g_vkbd_is_initialised ) return;
    if ( !gtk_widget_get_visible ( g_vkbd.window ) ) return;

    //PIO8255_VKBD_MATRIX_RESET ( );

    int row_block = 0;
    while ( g_vkbd_allRows[row_block] != NULL ) {
        st_VKBD_ROW_SZ15 *r = g_vkbd_allRows[row_block];
        int i;
        for ( i = 0; i < r->numkeys; i++ ) {
            st_VKBD_KEY *k = &r->key[i];
            ui_vkbd_key_event ( k, VK_ALL_ACTCALLERS, FALSE );
        };
        row_block++;
    };
}
