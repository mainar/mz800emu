/* 
 * File:   cmt.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 11. srpna 2015, 12:07
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
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <assert.h>

#include "cmt.h"
#include "mz800.h"

#include "gdg/gdg.h"

#include "ui/ui_utils.h"
#include "ui/ui_main.h"
#include "ui/ui_file_chooser.h"
#include "ui/ui_cmt.h"

#include "cfgmain.h"

#include "cmthack.h"
#include "libs/mztape/cmtspeed.h"
#include "cmtext.h"
#include "cmtext_block.h"
#include "cmt_mzf.h"
#include "libs/mzf/mzf.h"

st_CMT g_cmt;


void cmt_stop ( void ) {
    if ( !TEST_CMT_FILLED ) return;
    if ( TEST_CMT_STOP ) return;

    if ( g_cmt.ext->cb_stop ) g_cmt.ext->cb_stop ( );

    printf ( "Virtual CMT is stopped.\n" );

    g_cmt.state = CMT_STATE_STOP;
    g_cmt.paused = 0;
    g_cmt.playsts = CMTEXT_BLOCK_PLAYSTS_STOP;
    g_cmt.output = 0;

    if ( g_cmt.cpu_boost == CMT_CPU_BOOST_ENABLED ) {
        mz800_switch_emulation_speed ( 0 );
    };

    ui_cmt_window_update ( );
}


void cmt_pause ( int value ) {

    if ( value == g_cmt.paused ) return;

    if ( value ) {
        if ( TEST_CMT_STOP ) {
            cmt_play_paused ( );
        } else {
            g_cmt.paused = 1;
        };
        if ( TEST_CMT_STOP ) {
            g_cmt.paused = 0;
        };
        g_cmt.paused_time = gdg_get_total_ticks ( ) - g_cmt.start_time;
    } else {
        g_cmt.paused = 0;
    };

    if ( ( g_cmt.cpu_boost == CMT_CPU_BOOST_ENABLED ) && ( !TEST_CMT_PAUSED ) && ( !TEST_CMT_STOP ) ) {
        mz800_switch_emulation_speed ( 1 );
    } else {
        mz800_switch_emulation_speed ( 0 );
    };

    if ( TEST_CMT_PAUSED ) {
        printf ( "Virtual CMT - %s is paused.\n", ( TEST_CMT_PLAY ) ? "playing" : "recording" );
    } else {
        if ( TEST_CMT_PLAY ) {
            printf ( "Virtual CMT is playing.\n" );
        } else { //if ( TEST_CMT_RECORD ) {
            printf ( "Virtual CMT is recording.\n" );
        };
        g_cmt.start_time = g_cmt.recording_last_event = gdg_get_total_ticks ( ) - g_cmt.paused_time;
    };

    ui_cmt_window_update ( );
}


void cmt_eject ( void ) {
    if ( !TEST_CMT_FILLED ) return;
    if ( !TEST_CMT_STOP ) cmt_stop ( );
    if ( g_cmt.ext->cb_eject ) g_cmt.ext->cb_eject ( );
    g_cmt.ext = NULL;
    ui_cmt_window_update ( );
}


void cmt_play ( void ) {
    if ( !TEST_CMT_FILLED ) return;
    if ( !TEST_CMT_STOP ) return;
    if ( EXIT_SUCCESS != cmtext_is_playable ( g_cmt.ext ) ) return;
    g_cmt.state = CMT_STATE_PLAY;
    g_cmt.playsts = CMTEXT_BLOCK_PLAYSTS_BODY;
    g_cmt.start_time = gdg_get_total_ticks ( );
    //printf ( "CMT start: %ul\n", gdg_get_total_ticks ( ) );
    g_cmt.ui_player_update = 0;
    ui_cmt_window_update ( );
    if ( ( g_cmt.cpu_boost == CMT_CPU_BOOST_ENABLED ) && ( !TEST_CMT_PAUSED ) ) {
        mz800_switch_emulation_speed ( 1 );
    };
    if ( !TEST_CMT_PAUSED ) {
        printf ( "Virtual CMT is playing.\n" );
    } else {
        g_cmt.paused_time = 0;
    };
    g_cmt.ext->block->cb_play ( g_cmt.ext );
}


void cmt_play_paused ( void ) {
    g_cmt.paused = 1;
    cmt_play ( );
}


void cmt_record ( void ) {
    if ( !TEST_CMT_STOP ) return;
    if ( ( TEST_CMT_FILLED ) && ( EXIT_SUCCESS != cmtext_is_recordable ( g_cmt.ext ) ) ) {
        cmt_eject ( );
    };
    if ( !TEST_CMT_FILLED ) {
        char *filepath = ui_file_chooser_open_wav_to_record ( );
        if ( !filepath ) {
            ui_cmt_window_update ( );
            return;
        };

        char *fileext = ui_file_chooser_get_filename_extension ( filepath );
        if ( fileext != NULL ) {
            if ( 0 != strcasecmp ( fileext, "wav" ) ) fileext = NULL;
        };

        if ( fileext == NULL ) {
            GString *gs = g_string_new ( 0 );
            int i = 0;

            FILE *tst_fh = NULL;

            do {
                if ( i > 100 ) break;
                g_string_sprintf ( gs, "%s", filepath );
                if ( i != 0 ) {
                    g_string_sprintfa ( gs, "_%d", i );
                };
                i++;
                g_string_append ( gs, ".wav" );
                tst_fh = ui_utils_file_open ( gs->str, "rb+" );
            } while ( tst_fh );

            if ( tst_fh ) {
                g_string_free ( gs, TRUE );
                ui_utils_file_close ( tst_fh );
                printf ( "Can't save to file: %s.wav\n", filepath );
                free ( filepath );
                ui_cmt_window_update ( );
                return;
            } else {
                free ( filepath );
                filepath = g_string_free ( gs, FALSE );
            };
        };

        st_CMTEXT *ext = cmtext_get_recording_extension ( );

        if ( EXIT_SUCCESS != ext->cb_open ( filepath ) ) {
            ui_show_error ( "%s can't open file '%s'\n", cmtext_get_description ( ext ), filepath );
            ui_utils_mem_free ( filepath );
            ui_cmt_window_update ( );
            return;
        };

        ui_utils_mem_free ( filepath );
        g_cmt.ext = ext;
        //ui_cmt_window_update ( );
    };

    // implicitne zacneme v pauze
    g_cmt.paused = 1;

    g_cmt.state = CMT_STATE_RECORD;
    g_cmt.playsts = CMTEXT_BLOCK_PLAYSTS_BODY;
    g_cmt.start_time = gdg_get_total_ticks ( );
    //printf ( "CMT start: %ul\n", gdg_get_total_ticks ( ) );
    g_cmt.ui_player_update = 0;
    g_cmt.recording_to_stream = 0;
    ui_cmt_set_show_time ( UICMT_SHOW_PLAY_TIME );
    ui_cmt_window_update ( );
    /*
        if ( g_cmt.cpu_boost == CMT_CPU_BOOST_ENABLED ) {
            mz800_switch_emulation_speed ( 1 );
        };
     */

    if ( !TEST_CMT_PAUSED ) {
        printf ( "Virtual CMT is recording.\n" );
    } else {
        printf ( "Virtual CMT - recording is paused.\n" );
        g_cmt.paused_time = 0;
    };
}


int cmt_change_speed ( en_CMTSPEED cmtspeed ) {

    if ( !TEST_CMT_STOP ) return EXIT_FAILURE;
    if ( g_cmt.mz_cmtspeed == cmtspeed ) return EXIT_SUCCESS;


    int ret = EXIT_SUCCESS;

    if ( TEST_CMT_FILLED ) {
        if ( cmtext_block_get_block_speed ( g_cmt.ext->block ) == CMTEXT_BLOCK_SPEED_DEFAULT ) {
            assert ( g_cmt.ext->block->cb_set_speed != NULL );
            if ( g_cmt.ext->block->cb_set_speed != NULL ) {
                if ( EXIT_SUCCESS != g_cmt.ext->block->cb_set_speed ( g_cmt.ext, cmtspeed ) ) {
                    ret = EXIT_FAILURE;
                };
            };
        };
    };

    if ( ret == EXIT_SUCCESS ) {
        g_cmt.mz_cmtspeed = cmtspeed;
    }

    ui_cmt_window_update ( );

    return ret;
}


void cmt_exit ( void ) {

    cmt_eject ( );
    if ( g_cmt.last_filename ) ui_utils_mem_free ( g_cmt.last_filename );

    cmthack_exit ( );
    cmtext_exit ( );
}


void cmt_propagatecfg_cmt_speed ( void *e, void *data ) {
    g_cmt.mz_cmtspeed = cfgelement_get_keyword_value ( (CFGELM *) e );
    ui_cmt_window_update ( );
}


void cmt_rear_dip_switch_cmt_inverted_polarity ( unsigned value ) {
    value &= 1;
    if ( value == g_cmt.polarity ) return;
    g_cmt.polarity = value;
    ui_main_update_rear_dip_switch_cmt_inverted_polarity ( g_cmt.polarity );
    if ( !TEST_CMT_FILLED ) return;
    if ( g_cmt.ext->block->cb_set_polarity != NULL ) g_cmt.ext->block->cb_set_polarity ( g_cmt.ext, value );
}


void cmt_propagatecfg_inverted_polarity ( void *e, void *data ) {
    ui_main_update_rear_dip_switch_cmt_inverted_polarity ( cfgelement_get_bool_value ( (CFGELM *) e ) );
}


void cmt_propagatecfg_cpu_boost ( void *e, void *data ) {
    (void) e;
    (void) data;
    cmt_cpu_boost_set ( cfgelement_get_bool_value ( (CFGELM *) e ) );
}


void cmt_propagatecfg_mzfsize_check ( void *e, void *data ) {
    (void) e;
    (void) data;
    cmt_mzfsize_check_set ( cfgelement_get_bool_value ( (CFGELM *) e ) );
}


void cmt_init ( void ) {

    CFGMOD *cmod = cfgroot_register_new_module ( g_cfgmain, "CMT" );

    CFGELM *elm;
    elm = cfgmodule_register_new_element ( cmod, "mz_cmtspeed", CFGENTYPE_KEYWORD, CMTSPEED_1_1,
                                           CMTSPEED_1_1, "SPEED_1/1",
                                           CMTSPEED_2_1, "SPEED_2/1",
                                           CMTSPEED_7_3, "SPEED_7/3",
                                           CMTSPEED_8_3, "SPEED_8/3",
                                           CMTSPEED_3_1, "SPEED_3/1",
                                           -1 );
    cfgelement_set_propagate_cb ( elm, cmt_propagatecfg_cmt_speed, NULL );
    cfgelement_set_handlers ( elm, NULL, (void*) &g_cmt.mz_cmtspeed );

    elm = cfgmodule_register_new_element ( cmod, "cmt_polarity_inverted", CFGENTYPE_BOOL, CMT_STREAM_POLARITY_NORMAL );
    cfgelement_set_propagate_cb ( elm, cmt_propagatecfg_inverted_polarity, NULL );
    cfgelement_set_handlers ( elm, (void*) &g_cmt.polarity, (void*) &g_cmt.polarity );

    elm = cfgmodule_register_new_element ( cmod, "cpu_boost", CFGENTYPE_BOOL, CMT_CPU_BOOST_ENABLED );
    cfgelement_set_propagate_cb ( elm, cmt_propagatecfg_cpu_boost, NULL );
    cfgelement_set_handlers ( elm, (void*) &g_cmt.cpu_boost, (void*) &g_cmt.cpu_boost );

    elm = cfgmodule_register_new_element ( cmod, "mzfsize_check", CFGENTYPE_BOOL, CMT_MZFSIZE_CHECK_ENABLED );
    cfgelement_set_propagate_cb ( elm, cmt_propagatecfg_mzfsize_check, NULL );
    cfgelement_set_handlers ( elm, (void*) &g_cmt.mzfsize_check, (void*) &g_cmt.mzfsize_check );

    cfgmodule_parse ( cmod );
    cfgmodule_propagate ( cmod );

    g_cmt.output = 0;
    g_cmt.start_time = 0;
    g_cmt.state = CMT_STATE_STOP;
    g_cmt.paused = 0;
    g_cmt.playsts = CMTEXT_BLOCK_PLAYSTS_STOP;
    g_cmt.ext = NULL;
    g_cmt.last_filename = ui_utils_mem_alloc0 ( 1 );
    g_cmt.ui_player_update = 0;

    cmtext_init ( );

    ui_cmt_init ( );
    ui_cmt_window_update ( );

    cmthack_init ( );
}


int cmt_open_file_by_extension ( char *filename ) {

    st_CMTEXT *ext = cmtext_get_extension ( filename );

    if ( !ext ) {
        ui_show_error ( "Unknown CMT file extension '%s'\n", filename );
        return EXIT_FAILURE;
    }

    cmt_eject ( );

    int ret = ext->cb_open ( filename );
    if ( ret == EXIT_SUCCESS ) {
        g_cmt.ext = ext;
    } else {
        ui_show_error ( "%s can't open file '%s'\n", cmtext_get_description ( ext ), filename );
    };
    ui_cmt_window_update ( );

    if ( ( ret == EXIT_SUCCESS ) && ( TEST_CMT_MZFSIZE_CHECK_ENABLED ) ) {
        if ( 0 == strcmp ( cmtext_get_name ( g_cmt.ext ), "MZF" ) ) {
            st_CMTEXT_BLOCK *mzfblk = cmtext_get_block ( g_cmt.ext );
            st_CMTMZF_BLOCKSPEC *blspec = mzfblk->spec;
            st_MZF_HEADER *mzfhdr = blspec->hdr;
            int mzf_size = mzfhdr->fsize + sizeof ( st_MZF_HEADER );
            ui_cmt_check_mzf_filesize ( filename, mzf_size );
        };
    };

    return ret;
}


int cmt_open ( void ) {

    char *filename = ui_file_chooser_open_cmt_file ( g_cmt.last_filename );

    if ( filename == NULL ) return EXIT_FAILURE;

    cmt_open_file_by_extension ( filename );

    ui_utils_mem_free ( filename );

    return EXIT_SUCCESS;
}


void cmt_update_output ( void ) {

    if ( ( !TEST_CMT_PLAY ) || ( TEST_CMT_PAUSED ) ) return;

    uint64_t play_ticks = gdg_get_total_ticks ( ) - g_cmt.start_time;
    uint64_t transferred_ticks = 0;

    en_CMTEXT_BLOCK_PLAYSTS playsts = cmtext_block_get_output ( g_cmt.ext->block, play_ticks, &g_cmt.output, &transferred_ticks );

    if ( playsts != g_cmt.playsts ) {
        if ( CMTEXT_BLOCK_PLAYSTS_STOP == playsts ) {
            int total_blocks = cmtext_container_get_count_blocks ( g_cmt.ext->container );
            int play_block = cmtext_block_get_block_id ( g_cmt.ext->block ) + 1;

            if ( play_block < total_blocks ) {
                assert ( g_cmt.ext->container->cb_next_block );

                if ( ( !g_cmt.ext->container->cb_next_block ) || ( EXIT_FAILURE == g_cmt.ext->container->cb_next_block ( ) ) ) {
                    cmt_stop ( );
                } else {
                    g_cmt.playsts = CMTEXT_BLOCK_PLAYSTS_BODY;
                    g_cmt.start_time = gdg_get_total_ticks ( ) - transferred_ticks;
                    ui_cmt_window_update ( );
                };

            } else {
                cmt_stop ( );
            };

        } else if ( CMTEXT_BLOCK_PLAYSTS_PAUSE == playsts ) {
            printf ( "CMT: Playing a gap space of %d ms\n", cmtext_block_get_pause_after ( g_cmt.ext->block ) );
            g_cmt.playsts = playsts;
            ui_cmt_window_update ( );
        };
    };
}


void cmt_screen_done_period ( void ) {

    if ( ( !TEST_CMT_FILLED ) || ( TEST_CMT_STOP ) || ( TEST_CMT_PAUSED ) ) return;

    cmt_update_output ( );

    if ( g_cmt.ui_player_update++ < 49 ) return;

    g_cmt.ui_player_update = 0;
    ui_cmt_update_player ( );

    if ( TEST_CMT_RECORD ) {
        if ( g_cmt.recording_to_stream == 0 ) {
            st_CMTEXT_BLOCK *block = cmtext_get_block ( g_cmt.ext );
            st_CMT_STREAM *stream = cmtext_block_get_stream ( block );
            if ( stream != NULL ) {
                g_cmt.recording_to_stream = 1;
                if ( g_cmt.cpu_boost == CMT_CPU_BOOST_ENABLED ) {
                    mz800_switch_emulation_speed ( 1 );
                };
            };
        } else {
            if ( ( gdg_get_total_ticks ( ) - g_cmt.recording_last_event ) > ( 5 * GDGCLK_BASE ) ) {
                if ( ( g_cmt.cpu_boost == CMT_CPU_BOOST_ENABLED ) && ( TEST_MAX_EMULATION_SPEED ) ) {
                    mz800_switch_emulation_speed ( 0 );
                };
            } else {
                if ( ( g_cmt.cpu_boost == CMT_CPU_BOOST_ENABLED ) && ( !TEST_MAX_EMULATION_SPEED ) ) {
                    mz800_switch_emulation_speed ( 1 );
                };
            };
        };
    };
}


int cmt_read_data ( void ) {
    cmt_update_output ( );
    return g_cmt.output;
}


void cmt_write_data ( int value ) {
    if ( ( !TEST_CMT_RECORD ) || ( TEST_CMT_PAUSED ) ) return;
    g_cmt.recording_last_event = gdg_get_total_ticks ( );
    uint64_t play_ticks = g_cmt.recording_last_event - g_cmt.start_time;
    if ( g_cmt.ext->cb_write ) g_cmt.ext->cb_write ( play_ticks, ~value );
}


void cmt_cpu_boost_set ( en_CMT_CPU_BOOST cpu_boost ) {

    g_cmt.cpu_boost = cpu_boost;

    if ( !TEST_CMT_STOP ) {
        if ( g_cmt.cpu_boost == CMT_CPU_BOOST_ENABLED ) {
            mz800_switch_emulation_speed ( 1 );
        } else {
            mz800_switch_emulation_speed ( 0 );
        };
    };

    ui_cmt_cpu_boost_menu_update ( );
}


void cmt_mzfsize_check_set ( en_CMT_MZFSIZE_CHECK mzfsize_check ) {
    g_cmt.mzfsize_check = mzfsize_check;
    ui_cmt_mzfsize_check_menu_update ( );
}
