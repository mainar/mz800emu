/* 
 * File:   wd279x.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 5. srpna 2015, 12:41
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
 * Popis chovani radice WD279x: http://www.scav.ic.cz/sharp_mz-800/sharp_mz-800_8_FDC-WD2793.htm
 *
 * Popis struktury DSK souboru: http://cpctech.cpc-live.com/docs/extdsk.html
 *
 */

#ifdef WINDOWS
#define COMPILE_FOR_EMULATOR
#undef COMPILE_FOR_UNICARD
#undef FS_LAYER_FATFS
#elif LINUX
#define COMPILE_FOR_EMULATOR
#undef COMPILE_FOR_UNICARD
#undef FS_LAYER_FATFS
#else
#undef COMPILE_FOR_EMULATOR
#define COMPILE_FOR_UNICARD
#define FS_LAYER_FATFS
#endif


//#define DBGLEVEL        ( DBGNON /* | DBGERR | DBGWAR | DBGINF */ )
//#define DBGLEVEL        ( DBGNON | DBGERR | DBGWAR | DBGINF )
#include "debug.h"

#include "../fs_layer.h"

#ifdef COMPILE_FOR_UNICARD
#include "hal.h"
#include "monitor.h"
#include "mzint.h"
#else
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "ui/ui_main.h"
#include "ui/ui_utils.h"
#endif

#include "wd279x.h"
//st_WD279X FDC;

#if ( DBGLEVEL & DBGINF )
static int SUPPRESSED_DBGMSG = 1;
#ifdef COMPILE_FOR_UNICARD
#define XPRINTF xprintf
#else
#define XPRINTF printf
#endif
#endif


/*
 * Podle tabulky stop v aktualnim DSK souboru spocita offset pro pozadovanou stopu.
 *
 * Vstup: 
 *      drive_id
 *      track
 *      side
 *
 * Vraci:
 *      offset v DSK souboru, nebo 
 *      0 - v pripade chyby
 * 
 */
static int32_t wd279x_get_DSK_track_offset ( st_WD279X *FDC, uint8_t drive_id, uint8_t track, uint8_t side ) {
    uint8_t i, buffer;
    unsigned int readlen;
    uint32_t offset = 0;
    int32_t seek_offset;

    DBGPRINTF ( DBGINF, "FDC = %s, drive_id = %d, track = 0x%02x, side = %d\n", FDC->name, drive_id, track, side );

    seek_offset = 0x34; /* Nastavime se na zacatek tabulky stop */

    if ( FS_LAYER_FR_OK != FS_LAYER_FSEEK ( FDC->drive[ drive_id ].fh, seek_offset ) ) {
        DBGPRINTF ( DBGERR, "fseek() error: FDC = %s, drive_id = %d, track = 0x%02x, side = %d\n", FDC->name, drive_id, track, side );
        return ( 0 );
    };

    for ( i = 0; i < ( ( track * 2 ) + side ); i++ ) {
        FS_LAYER_FREAD ( FDC->drive[ drive_id ].fh, &buffer, 1, &readlen );
        if ( 1 != readlen ) {
            DBGPRINTF ( DBGERR, "fread() error: FDC = %s, drive_id = %d, track = 0x%02x, side = %d\n", FDC->name, drive_id, track, side );
            return ( 0 );
        };

        if ( buffer == 0x00 ) {
            DBGPRINTF ( DBGERR, "Track not exist: FDC = %s, drive_id = %d, track = 0x%02x, side = %d\n", FDC->name, drive_id, track, side );
            return ( 0 );
        };

        /* BUGFIX: nektere HD DSK soubory maji vadne info o Sharp boot stope */
        if ( i == 1 ) {
            if ( buffer == 0x25 ) {
                buffer = 0x11; /* vnutime bezny pocet sektoru, ktery odpovida Sharp boot stope  */
            };
        };

        offset += buffer * 0x100;
    };

    offset += 0x100;

    DBGPRINTF ( DBGINF, "track_offset: 0x%x\n", offset );

    return ( offset );
}


/*
 * Podle tabulky sektoru na prave nastavene stope spocita a nastavi pozici v 
 * aktualnim DSK souboru na zacatek pozadovaneho sektoru.
 * 
 * Spocita a nastavi:
 *      FDC->drive[ drive_id ].sector_size
 *      FDC->drive[ drive_id ].SECTOR 
 *
 * Vstup: 
 *      drive_id
 *      sector
 *
 * Vraci:
 *   WD279X_RET_OK
 *   WD279X_RET_ERR v pripade chyby, nebo pokud sektor nebyl nalezen
 * 
 */
static int wd279x_seek_to_sector ( st_WD279X *FDC, uint8_t drive_id, uint8_t sector ) {
    uint8_t i, sector_count;
    uint8_t buffer [ 8 ];
    unsigned int readlen;
    uint16_t offset = 0;
    int32_t seek_offset;

    DBGPRINTF ( DBGINF, "FDC = %s, drive_id = %d, sector = %d\n", FDC->name, drive_id, sector );

    FDC->drive[drive_id].sector_size = 0;

    seek_offset = FDC->drive[drive_id].track_offset + 0x15;

    if ( FS_LAYER_FR_OK != FS_LAYER_FSEEK ( FDC->drive[drive_id].fh, seek_offset ) ) {
        DBGPRINTF ( DBGERR, "fseek() error: FDC = %s, drive_id = %d, sector = %d\n", FDC->name, drive_id, sector );
#ifdef COMPILE_FOR_EMULATOR
        ui_show_error ( "%s():%d - fseek error: %s", __func__, __LINE__, strerror ( errno ) );
#endif
        return ( WD279X_RET_ERR );
    };

    FS_LAYER_FREAD ( FDC->drive[drive_id].fh, &sector_count, 1, &readlen );
    if ( 1 != readlen ) {
        DBGPRINTF ( DBGERR, "fread() error: FDC = %s, drive_id = %d, sector = %d\n", FDC->name, drive_id, sector );
#ifdef COMPILE_FOR_EMULATOR
        ui_show_error ( "%s():%d - fread error: %s", __func__, __LINE__, strerror ( errno ) );
#endif
        return ( WD279X_RET_ERR );
    };

    seek_offset = FDC->drive[drive_id].track_offset + 0x18;
    if ( FS_LAYER_FR_OK != FS_LAYER_FSEEK ( FDC->drive[drive_id].fh, seek_offset ) ) {
        DBGPRINTF ( DBGERR, "fseek() error: FDC = %s, drive_id = %d, sector = %d\n", FDC->name, drive_id, sector );
#ifdef COMPILE_FOR_EMULATOR
        ui_show_error ( "%s():%d - fseek error: %s", __func__, __LINE__, strerror ( errno ) );
#endif
        return ( WD279X_RET_ERR );
    };

    for ( i = 0; i < sector_count; i++ ) {
        FS_LAYER_FREAD ( FDC->drive[drive_id].fh, buffer, 8, &readlen );
        if ( 8 != readlen ) {
            DBGPRINTF ( DBGERR, "fread() error: FDC = %s, drive_id = %d, sector = %d\n", FDC->name, drive_id, sector );
#ifdef COMPILE_FOR_EMULATOR
            ui_show_error ( "%s():%d - fread error: %s", __func__, __LINE__, strerror ( errno ) );
#endif
            return ( WD279X_RET_ERR );
        };

        if ( sector == buffer [ 2 ] ) {
            FDC->drive[drive_id].sector_size = buffer [ 3 ] * 0x100;
            break;
        };
        offset += buffer [ 3 ] * 0x100;
    };

    if ( FDC->drive[drive_id].sector_size == 0 ) {
        DBGPRINTF ( DBGINF, "sector not found: FDC = %s, drive_id = %d, sector = %d\n", FDC->name, drive_id, sector );
        return ( WD279X_RET_ERR );
    };

    seek_offset = FDC->drive[ drive_id ].track_offset + offset + 0x100;
    if ( FS_LAYER_FR_OK != FS_LAYER_FSEEK ( FDC->drive[ drive_id ].fh, seek_offset ) ) {
        DBGPRINTF ( DBGERR, "fseek() error: FDC = %s, drive_id = %d, sector = %d\n", FDC->name, drive_id, sector );
#ifdef COMPILE_FOR_EMULATOR
        ui_show_error ( "%s():%d - fseek error: %s", __func__, __LINE__, strerror ( errno ) );
#endif
        FDC->drive[drive_id].SECTOR = 0;
        FDC->drive[drive_id].sector_size = 0;
        return ( WD279X_RET_ERR );
    };

    FDC->drive[drive_id].SECTOR = sector;
    DBGPRINTF ( DBGINF, "OK: FDC = %s, drive_id = %d, sector = %d, track = 0x%02x, side = %d, sector_offset: 0x%x\n", FDC->name, drive_id, sector, FDC->drive[ drive_id ].TRACK, FDC->drive[drive_id].SIDE, seek_offset );
    return ( WD279X_RET_OK );

}

#ifdef COMPILE_FOR_UNICARD


/*
 * FDC_setFDfromCFG() - podle obsahu konfiguracniho souboru /unicard/fd[0-3].cfg
 * otevre prislusny DSK soubor do zvolene mechaniky.
 * Pokud je jiz v mechanice nejaky DSK otevren, tak jej korektne zavre.
 * Pokud konfiguracni soubor neexistuje, nebo nelze otevrit, tak nastavi vsechny 
 * hodnoty ve strukture aktualni mechaniky na nulu.
 *
 * Prijma: drive_id
 *
 * Vraci:
 *   -1 v pripade chyby
 *       0 pokud zadny disk neprimountoval
 *    1 pokud primountoval disk
 */

int8_t FDC_setFDfromCFG ( uint8_t drive_id ) {
    FILEHANDLE_t fh;
    char filename [] = "/unicard/fd0.cfg";
    unsigned int ff_readlen;


    DBGPRINTF ( DBGINF, "FDC_setFDfromCFG(): Requested for drive_id = 0x%02x\n", drive_id );


    if ( FDC->drive[drive_id].fh.fs ) {
        if ( FS_LAYER_FR_OK != FS_LAYER_FSYNC ( FDC->drive[drive_id].fh ) ) return ( -1 ); // sync error
        FS_LAYER_FCLOSE ( FDC->drive[drive_id].fh );
    };
    memset ( &FDC->drive[ drive_id ], 0x00, sizeof ( FDC->drive[ drive_id ] ) ); //tohle bych asi radeji nedelal...

    filename [ sizeof ( filename ) - 6 ] = drive_id + 0x30;

    //XPRINTF ( filename );
    if ( FS_LAYER_FR_OK != f_open ( &fh, filename, FS_LAYER_FMODE_RO ) ) {
        return (-1 ); // open file error
    }
    if ( FS_LAYER_FR_OK != FS_LAYER_FREAD ( fh, (uint8_t*) FDC->drive [ drive_id ].path, sizeof ( FDC->drive [ drive_id ].path ), ff_readlen ) ) {
        FS_LAYER_FCLOSE ( fh );
        return ( -1 ); // read error
    };
    FS_LAYER_FCLOSE ( fh );
    FDC->drive [ drive_id ].path[ff_readlen] = 0x00;

    /*     while ( FDC->drive [ drive_id ].path [ strlen ( FDC->drive [ drive_id ].path ) - 1] < 0x20 ) { */
    /*         FDC->drive [ drive_id ].path [ strlen ( FDC->drive [ drive_id ].path ) - 1] = '\0'; */
    /*         if ( FDC->drive [ drive_id ].path [ 0 ] == '\0' ) return ( -1 ); // bad /path/filename in cfg file */
    /*     }; */

    /*     fname = strrchr ( FDC->drive [ drive_id ].path, '/' ); */
    /*     strncpy ( FDC->drive [ drive_id ].filename, fname +1 , sizeof ( FDC->drive [ drive_id ].filename ) ); */
    /*     if ( FDC->drive [ drive_id ].filename [ 0 ] == '\0' ) { */
    /*         FDC->drive [ drive_id ].path [ 0 ] = '\0'; */
    /*         return ( -1 );   // bad filename in cfg file */
    /*     }; */
    /*     fname [ 0 ] = '\0'; */

    DBGPRINTF ( DBGINF, "FDC_setFDfromCFG(): New cfg: '%s' and file '%s', DRIVE: 0x%02x\n",
                FDC->drive [ drive_id ].path, FDC->drive[drive_id].filename, drive_id );

    if ( FS_LAYER_FR_OK != f_open ( &( FDC->drive[drive_id].fh ), FDC->drive[drive_id].path, FS_LAYER_FMODE_RW ) ) {

        DBGPRINTF ( DBGERR, "FDC_setFDfromCFG(): error when opening path '%s' and file '%s', DRIVE: 0x%02x\n",
                    FDC->drive [ drive_id ].path, FDC->drive [ drive_id ].filename, drive_id );

        FDC->drive [ drive_id ].path [ 0 ] = '\0';
        FDC->drive [ drive_id ].filename [ 0 ] = '\0';

        return ( -1 ); // error when open DSK file
    };

    FDC->drive[ drive_id ].track_offset = wd279x_get_DSK_track_offset ( drive_id, FDC->drive[ drive_id ].TRACK, FDC->drive[ drive_id ].SIDE );

    if ( !FDC->drive[ drive_id ].track_offset ) {

        DBGPRINTF ( DBGERR, "FDC_setFDfromCFG(): FDC_GetTrackOffset - returned error!\n" );

        FS_LAYER_FCLOSE ( FDC->drive [ drive_id ].fh );
        FDC->drive [ drive_id ].path [ 0 ] = '\0';
        FDC->drive [ drive_id ].filename [ 0 ] = '\0';

        return ( -1 ); // error when setting track 0 side 0
    };

    DBGPRINTF ( DBGINF, "FDC_setFDfromCFG(): DRIVE: 0x02%, new FH: 0x%02x\n",
                drive_id, FDC->drive [ drive_id ].fh );

    return ( 1 );
}
#endif


/*
 * Odmountuje DSK z mechaniky.
 *
 * Vstup:
 *      drive_id
 * 
 */
void wd279x_close_dsk ( st_WD279X *FDC, uint8_t drive_id ) {

    DBGPRINTF ( DBGINF, "FDC = %s, drive_id = %d\n", FDC->name, drive_id );

#ifdef FS_LAYER_FATFS    
    if ( FDC->drive[drive_id].fh.fs ) {
#else
    if ( FDC->drive[drive_id].dsk_in_drive ) {
#endif
        if ( FS_LAYER_FR_OK != FS_LAYER_FSYNC ( FDC->drive[drive_id].fh ) ) {
            DBGPRINTF ( DBGERR, "fsync(), FDC = %s, drive_id = %d\n", FDC->name, drive_id );
#ifdef COMPILE_FOR_EMULATOR
            ui_show_error ( "%s():%d - fsync error: %s", __func__, __LINE__, strerror ( errno ) );
#endif
        };
        FS_LAYER_FCLOSE ( FDC->drive[drive_id].fh );
    };

    memset ( &FDC->drive[drive_id], 0x00, sizeof ( st_FDDrive ) );
}


/*
 * Primountuje DSK do mechaniky.
 *
 * Vstup:
 *      drive_id
 *      DSK_filename
 *
 * Vraci:
 *      WD279X_RET_ERR
 *      WD279X_RET_OK
 */
int wd279x_open_dsk ( st_WD279X *FDC, uint8_t drive_id, char *DSK_filename ) {

    DBGPRINTF ( DBGINF, "FDC = %s, drive_id = %d, DSK_filename = %s\n", FDC->name, drive_id, DSK_filename );

#ifdef FS_LAYER_FATFS    
    if ( FDC->drive[drive_id].fh.fs ) {
#else
    if ( FDC->drive[drive_id].dsk_in_drive ) {
#endif
        wd279x_close_dsk ( FDC, drive_id );
    };

    if ( FS_LAYER_FR_OK != FS_LAYER_FOPEN ( FDC->drive[drive_id].fh, DSK_filename, FS_LAYER_FMODE_RW ) ) {
        DBGPRINTF ( DBGERR, "fopen(), FDC = %s, drive_id = %d, DSK_filename = %s\n", FDC->name, drive_id, DSK_filename );
#ifdef COMPILE_FOR_EMULATOR
        ui_show_error ( "%s():%d - '%s' - fopen error: %s", __func__, __LINE__, DSK_filename, strerror ( errno ) );
#endif
        return ( WD279X_RET_ERR );
    };

    /* Provedeme kontrolu hlavicky DSK souboru */
    uint8_t buffer [ 35 ];
    unsigned int readlen;
    FS_LAYER_FREAD ( FDC->drive[ drive_id ].fh, &buffer, 34, &readlen );
    if ( 34 != readlen ) {
        DBGPRINTF ( DBGERR, "fopen(), FDC = %s, drive_id = %d, DSK_filename = %s, when read DSK header\n", FDC->name, drive_id, DSK_filename );
#ifdef COMPILE_FOR_EMULATOR
        ui_show_error ( "%s():%d - '%s' - read error: %s", __func__, __LINE__, DSK_filename, strerror ( errno ) );
#endif
        return ( WD279X_RET_ERR );
    };
    buffer [ 34 ] = 0x00;
    if ( 0 != strcmp ( (char*) buffer, "EXTENDED CPC DSK File\r\nDisk-Info\r\n" ) ) {
        DBGPRINTF ( DBGERR, "This is not valid DSK file! FDC = %s, drive_id = %d, DSK_filename = %s\n", FDC->name, drive_id, DSK_filename );
#ifdef COMPILE_FOR_EMULATOR
        ui_show_error ( "This is not valid DSK file! FDC = %s, drive_id = %d, DSK_filename = %s\n", FDC->name, drive_id, DSK_filename );
#endif
        return ( WD279X_RET_ERR );
    };


    FDC->drive[drive_id].TRACK = 0;
    FDC->drive[drive_id].SIDE = 0;

    FDC->drive[drive_id].track_offset = wd279x_get_DSK_track_offset ( FDC, drive_id, FDC->drive[drive_id].TRACK, FDC->drive[drive_id].SIDE );

    if ( !FDC->drive[ drive_id ].track_offset ) {
        DBGPRINTF ( DBGERR, "Cannot set TRACK, SIDE = 0, 0: FDC = %s, drive_id = %d, DSK_filename = %s\n", FDC->name, drive_id, DSK_filename );
        wd279x_close_dsk ( FDC, drive_id );
        return ( WD279X_RET_ERR );
    };

#ifndef FS_LAYER_FATFS    
    strncpy ( FDC->drive[drive_id].filename, DSK_filename, sizeof ( FDC->drive[drive_id].filename ) );
    FDC->drive[drive_id].filename [ sizeof ( FDC->drive[drive_id].filename ) - 1 ] = 0x00;
    FDC->drive[drive_id].dsk_in_drive = 1;
#endif

    return ( WD279X_RET_OK );
}


/*
 * FDC_Init() - vynulovani vsech hodnot ve strukture FDC a primountovani 
 * DSK souboru podle konfigurace v /unicard/fd[0-3].cfg
 *
 */
void wd279x_init ( st_WD279X *FDC, char *name ) {

    memset ( FDC, 0x00, sizeof ( st_WD279X ) );
    strncpy ( FDC->name, name, sizeof ( FDC->name ) - 1 );

#if 0
    uint8_t i;
    for ( i = 0; i < FDC_NUM_DRIVES; i++ ) {
        if ( -1 == FDC_setFDfromCFG ( i ) ) {
            DBGPRINTF ( DBGINF, "emu_FDC_Init(): mounting drive %d failed\n", i );
        };
    };

    reload_FDD = FDC_setFDfromCFG;
    return 1;
#endif 
}


/*
 * Nastavi mechaniku na pozadovanou stopu podle hodnot ulozenych v FDC->regTRACK a FDC->SIDE
 *
 * Vstup:
 *      -
 * 
 * Vystup:
 *      WD279X_RET_OK
 *      WD279X_RET_ERR - doslo k chybe (stopa nenalezena)
 */
static int wd279x_set_track ( st_WD279X *FDC ) {
    int32_t track_offset;

    DBGPRINTF ( DBGINF, "FDC = %s, drive_id = %d, track = 0x%02x, side = %d\n", FDC->name, FDC->MOTOR & 0x03, FDC->regTRACK, FDC->SIDE );

    /* Je potreba nastavovat, nebo uz je mechanika v pozadovanem stavu? */
    if ( FDC->drive[ FDC->MOTOR & 0x03 ].TRACK != FDC->regTRACK || FDC->drive[ FDC->MOTOR & 0x03 ].SIDE != FDC->SIDE ) {

        /* byla nastavena jinak, takze ji musime prenastavit */
        FDC->drive[ FDC->MOTOR & 0x03 ].SECTOR = 0;
        FDC->drive[ FDC->MOTOR & 0x03 ].sector_size = 0;

        track_offset = wd279x_get_DSK_track_offset ( FDC, FDC->MOTOR & 0x03, FDC->regTRACK, FDC->SIDE );

        if ( !track_offset ) {
            DBGPRINTF ( DBGERR, "set track err: FDC = %s, drive_id = %d, track = 0x%02x, side = %d\n", FDC->name, FDC->MOTOR & 0x03, FDC->regTRACK, FDC->SIDE );
            /* TODO: set status to seek err !!! */
            return ( WD279X_RET_ERR );
        };
        FDC->drive[ FDC->MOTOR & 0x03 ].track_offset = track_offset;
        FDC->drive[ FDC->MOTOR & 0x03 ].TRACK = FDC->regTRACK;
        FDC->drive[ FDC->MOTOR & 0x03 ].SIDE = FDC->SIDE;
    };

    return ( WD279X_RET_OK );
}


/*
 * Zpracovani prikazu z COMMAND registru.
 * 
 */
int wd279x_do_command ( st_WD279X *FDC ) {

    /*
     * 
     *  FDC TYPE I. COMMANDS
     * 
     */
    if ( FDC->COMMAND & 0x80 ) {

        /* Before all type I. commands: */
        FDC->regSTATUS = 0x00;

        /* empty drive */
#ifdef FS_LAYER_FATFS
        if ( !FDC->drive[ FDC->MOTOR & 0x03 ].fh.fs ) {
#else
        if ( !FDC->drive[ FDC->MOTOR & 0x03 ].dsk_in_drive ) {
#endif
            DBGPRINTF ( DBGWAR, "Empty drive, NOT READY for command type I: 0x%02x, FDC = %s, drive_id = %d\n", FDC->COMMAND, FDC->name, FDC->MOTOR & 0x03 );
            FDC->regSTATUS = 0x80; /* not ready */
            return WD279X_RET_ERR;
        };

        if ( FDC->COMMAND >> 4 == 0x0f ) { /* 0b1111 */
            /* FDC COMMAND: RESTORE */

            DBGPRINTF ( DBGINF, "FDC do COMMAND: 0x%02x - RESTORE: FDC = %s, drive_id = %d\n", FDC->COMMAND, FDC->name, FDC->MOTOR & 0x03 );

            FDC->regTRACK = 0;
            FDC->SIDE = 0;

        } else if ( FDC->COMMAND >> 4 == 0x0e ) { /* 0b1110 */
            /* FDC COMMAND: SEEK */

            DBGPRINTF ( DBGINF, "FDC do COMMAND: 0x%02x - SEEK: FDC = %s, drive_id = %d, from track: %d, to track: %d\n", FDC->COMMAND, FDC->name, FDC->MOTOR & 0x03, FDC->regTRACK, FDC->regDATA );
            FDC->regTRACK = FDC->regDATA;

        } else if ( FDC->COMMAND >> 5 == 0x05 ) { /* 0b101 */
            /* FDC COMMAND: STEP IN (track +1) */

            DBGPRINTF ( DBGINF, "FDC do COMMAND: 0x%02x - STEP IN (track + 1): FDC = %s, drive_id = %d, from track: %d\n", FDC->COMMAND, FDC->name, FDC->MOTOR & 0x03, FDC->regTRACK );
            FDC->regTRACK++;
            FDC->STATUS_SCRIPT = 1;

        } else if ( FDC->COMMAND >> 5 == 0x04 ) { /* 0b100 */
            /* STEP OUT (track -1) */

            DBGPRINTF ( DBGINF, "FDC do COMMAND: 0x%02x - STEP OUT (track - 1): FDC = %s, drive_id = %d, from track: %d\n", FDC->COMMAND, FDC->name, FDC->MOTOR & 0x03, FDC->regTRACK );
            if ( FDC->regTRACK ) {
                FDC->regTRACK--;
            };
        };


        /* After all type I. commands: */
        FDC->COMMAND = 0x00;
        FDC->DATA_COUNTER = 0;
        FDC->buffer_pos = 0;
        if ( FDC->regTRACK == 0 ) {
            FDC->regSTATUS |= 0x04; /* TRC00 */
        };
        FDC->STATUS_SCRIPT = 1; /* one BUSY, next READY */
        return WD279X_RET_OK;



        /*
         * 
         *  FDC TYPE II. COMMANDS
         * 
         */
    } else if ( FDC->COMMAND >> 6 == 0x01 ) {

        /* Before all type II. commands: */
        FDC->regSTATUS = 0;
        FDC->DATA_COUNTER = 0;
        FDC->buffer_pos = 0;
        FDC->STATUS_SCRIPT = 1; /* one BUSY, next READY */

        /* empty drive */
#ifdef FS_LAYER_FATFS
        if ( !FDC->drive[ FDC->MOTOR & 0x03 ].fh.fs ) {
#else
        if ( !FDC->drive[ FDC->MOTOR & 0x03 ].dsk_in_drive ) {
#endif
            DBGPRINTF ( DBGWAR, "Empty drive, NOT READY for command type II: 0x%02x, FDC = %s, drive_id = %d\n", FDC->COMMAND, FDC->name, FDC->MOTOR & 0x03 );
            FDC->regSTATUS = 0x80; /* not ready */
            return WD279X_RET_ERR;
        };

        if ( FDC->COMMAND & 0x10 ) {
            FDC->MULTIBLOCK_RW = 0;
        } else {
            FDC->MULTIBLOCK_RW = 1;
        };

        if ( wd279x_set_track ( FDC ) ) {
            /* stopa nenalezena */
            /* TODO: melo by se to nejak osetrit statuskodem */
            return WD279X_RET_ERR;
        };



        /* FDC COMMAND: READ SECTOR,  WRITE SECTOR */
        if ( FDC->COMMAND >> 5 == 0x03 || FDC->COMMAND >> 5 == 0x02 ) { /* 0b011, 0b010 */

            DBGPRINTF ( DBGINF, "FDC do COMMAND: 0x%02x - ", FDC->COMMAND );
#if ( DBGLEVEL & DBGINF )

            if ( FDC->COMMAND >> 5 == 0x02 ) { /* 0b010 */
                XPRINTF ( "WRITE SECTOR" );
            } else {
                XPRINTF ( "READ SECTOR" );
            };

            if ( FDC->MULTIBLOCK_RW ) {
                XPRINTF ( " (multiblock)" );
            };

            XPRINTF ( ", FDC = %s, drive_id = %d, track: %d, sector: %d, side: %d\n", FDC->name, FDC->MOTOR & 0x03, FDC->drive[ FDC->MOTOR & 0x03 ].TRACK, FDC->regSECTOR, FDC->drive[ FDC->MOTOR & 0x03 ].SIDE );
#endif

            if ( !FDC->drive[ FDC->MOTOR & 0x03 ].track_offset ) {
                /* status code set to track error ? */
                FDC->STATUS_SCRIPT = 3;
                return WD279X_RET_ERR;
            };

            if ( wd279x_seek_to_sector ( FDC, FDC->MOTOR & 0x03, FDC->regSECTOR ) ) {
                DBGPRINTF ( DBGINF, "Do LEC cp/m patch!\n" );
                /* sector not found! */
                FDC->STATUS_SCRIPT = 3; /* LEC cp/m v1.3 specs ... */
                return WD279X_RET_ERR;
            };


            if ( FDC->COMMAND >> 5 == 0x03 ) { /* 0b011 */
                uint16_t fdd_io_size;
                if ( FDC->drive[ FDC->MOTOR & 0x03 ].sector_size < sizeof ( FDC->buffer ) ) {
                    fdd_io_size = FDC->drive[ FDC->MOTOR & 0x03 ].sector_size;
                } else {
                    fdd_io_size = sizeof ( FDC->buffer );
                };

                unsigned int ff_readlen;
                FS_LAYER_FREAD ( FDC->drive[ FDC->MOTOR & 0x03 ].fh, FDC->buffer, fdd_io_size, &ff_readlen );
                if ( ff_readlen != fdd_io_size ) {
                    DBGPRINTF ( DBGERR, "error when reading2 DSK file: FDC = %s, drive_id = %d, track: %d, sector: %d, side: %d, track_offset: 0x%x\n", FDC->name, FDC->MOTOR & 0x03, FDC->drive[ FDC->MOTOR & 0x03 ].TRACK, FDC->regSECTOR, FDC->drive[ FDC->MOTOR & 0x03 ].SIDE, FDC->drive[ FDC->MOTOR & 0x03 ].track_offset );
#ifdef COMPILE_FOR_EMULATOR
                    ui_show_error ( "%s():%d - fread error: %s", __func__, __LINE__, strerror ( errno ) );
#endif
                    /* TODO: status code */
                    return WD279X_RET_ERR;
                };
            };


            FDC->DATA_COUNTER = FDC->drive[ FDC->MOTOR & 0x03 ].sector_size;
            FDC->regSTATUS |= 0x01; /* BUSY */
            FDC->regSTATUS |= 0x02; /* DRQ */
            return WD279X_RET_OK;

        } else {
            DBGPRINTF ( DBGERR, "????? NOT IMPLEMENTED COMMAND Type II. COMMAND: 0x%02x: FDC = %s, drive_id = %d\n", FDC->COMMAND, FDC->name, FDC->MOTOR & 0x03 );
        };

        /* Command type III. - read track addr */
    } else if ( FDC->COMMAND >> 4 == 0x03 ) { /* 0b0011 */
        /*                } else if ( FDC->COMMAND == 0x3f ) { */

        DBGPRINTF ( DBGINF, "FDC do COMMAND: 0x%02x - READ TRACK: FDC = %s, drive_id = %d, track: %d, side: %d\n", FDC->COMMAND, FDC->name, FDC->MOTOR & 0x03, FDC->regTRACK, FDC->drive[ FDC->MOTOR & 0x03 ].SIDE );

#ifdef FS_LAYER_FATFS
        if ( !FDC->drive[ FDC->MOTOR & 0x03 ].fh.fs ) {
#else
        if ( !FDC->drive[ FDC->MOTOR & 0x03 ].dsk_in_drive ) {
#endif
            DBGPRINTF ( DBGWAR, "Empty drive, NOT READY for command type III: 0x%02x, FDC = %s, drive_id = %d\n", FDC->COMMAND, FDC->name, FDC->MOTOR & 0x03 );
            FDC->regSTATUS = 0x80; /* not ready */
            return WD279X_RET_ERR;
        };

        if ( wd279x_set_track ( FDC ) ) {
            /* stopa nenalezena */
            /* TODO: status code */
            return WD279X_RET_ERR;
        };

        if ( !FDC->drive[ FDC->MOTOR & 0x03 ].SECTOR || !FDC->drive[ FDC->MOTOR & 0x03 ].sector_size ) {
            if ( wd279x_seek_to_sector ( FDC, FDC->MOTOR & 0x03, 1 ) ) {
                /* sektor nenalezen */
                /* TODO: status code */
                return WD279X_RET_ERR;
            };
        };

        /* Hlavicka sektoru */
        FDC->regSECTOR = FDC->drive[ FDC->MOTOR & 0x03 ].SECTOR;
        FDC->buffer [ 0 ] = FDC->drive[ FDC->MOTOR & 0x03 ].TRACK;
        FDC->buffer [ 1 ] = FDC->drive[ FDC->MOTOR & 0x03 ].SECTOR;
        FDC->buffer [ 2 ] = FDC->drive[ FDC->MOTOR & 0x03 ].SIDE;
        FDC->buffer [ 3 ] = FDC->drive[ FDC->MOTOR & 0x03 ].sector_size / 0x100;
        FDC->buffer [ 4 ] = 0x00;
        FDC->buffer [ 5 ] = 0x00;

        FDC->DATA_COUNTER = 6;
        FDC->regSTATUS = 0x00;
        FDC->regSTATUS |= 0x01; /* BUSY */
        FDC->regSTATUS |= 0x02; /* DRQ */
        FDC->STATUS_SCRIPT = 1;
        return WD279X_RET_OK;

        /* Command type III. - write track (format) */
    } else if ( FDC->COMMAND == 0x0f || FDC->COMMAND == 0x0b ) {

        DBGPRINTF ( DBGINF, "FDC do COMMAND: 0x%02x - WRITE TRACK: FDC = %s, drive_id = %d, track: %d, side: %d\n", FDC->COMMAND, FDC->name, FDC->MOTOR & 0x03, FDC->regTRACK, FDC->drive[ FDC->MOTOR & 0x03 ].SIDE );
#ifdef FS_LAYER_FATFS
        if ( !FDC->drive[ FDC->MOTOR & 0x03 ].fh.fs ) {
#else
        if ( !FDC->drive[ FDC->MOTOR & 0x03 ].dsk_in_drive ) {
#endif
            DBGPRINTF ( DBGWAR, "Empty drive, NOT READY for command type III. - WRITE TRACK: FDC = %s, drive_id = %d, track: %d, side: %d\n", FDC->name, FDC->MOTOR & 0x03, FDC->regTRACK, FDC->drive[ FDC->MOTOR & 0x03 ].SIDE );
            FDC->regSTATUS = 0x80; /* not ready */
            return WD279X_RET_ERR;
        };

        /* TODO: zkratit soubor, smazat tabulku stop a vynulovat pocet stop (??) */

        FDC->write_track_stage = 0;
        FDC->write_track_counter = 0;

        FDC->regSTATUS = 0x00;
        FDC->regSTATUS |= 0x01; /* BUSY */
        FDC->regSTATUS |= 0x02; /* DRQ */
        FDC->STATUS_SCRIPT = 1;
        return WD279X_RET_OK;

        /* Command type IV. - interrupts */
    } else if ( FDC->COMMAND == 0x27 || FDC->COMMAND == 0x2f ) {

        DBGPRINTF ( DBGINF, "FDC do COMMAND: 0x%02x - INTERRUPT: FDC = %s, drive_id = %d\n", FDC->COMMAND, FDC->name, FDC->MOTOR & 0x03 );

        FDC->DATA_COUNTER = 0;
        FDC->buffer_pos = 0;
        FDC->COMMAND = 0x00;
        FDC->regSTATUS = 0x00;
        FDC->STATUS_SCRIPT = 0;
        return WD279X_RET_OK;

    } else {
        DBGPRINTF ( DBGERR, "????? UNKNOWN COMMAND: 0x%02x: FDC = %s, drive_id = %d\n", FDC->COMMAND, FDC->name, FDC->MOTOR & 0x03 );
    };

    /* Unknown command */
    return WD279X_RET_ERR;
}


/*
 * Datove zpracovani prikazu WRITE TRACK.
 * 
 */
int wd279x_do_write_track ( st_WD279X *FDC, unsigned char *io_data ) {

    unsigned int ff_readlen;

    //printf ( "wd279x_do_write_track: 0x%02x\n", *io_data );

#if (DBGLEVEL & DBGINF)
    uint8_t last_write_track_stage = FDC->write_track_stage;
#endif

    // jsme na zacatku zapisu stopy - cekame na indexovou znacku
    if ( FDC->write_track_stage == 0 ) {


        DBGPRINTF ( DBGINF, "WRITE TRACK - waiting for index\n" );


        // prisel pocatecni index, takze budeme opravdu formatovat :)
        if ( *io_data == 0x03 ) { // ~0xfc
            FDC->write_track_stage = 1;
            FDC->write_track_counter = 0;


            // u prvni stopy upravime hlavicku DSK
            // a prepiseme tabulku stop
            if ( FDC->regTRACK == 0 && FDC->SIDE == 0 ) {
                int32_t write_track_offset = 0x22;
                if ( FS_LAYER_FR_OK != FS_LAYER_FSEEK ( FDC->drive[ FDC->MOTOR & 0x03 ].fh, write_track_offset ) ) {
#ifdef COMPILE_FOR_EMULATOR
                    ui_show_error ( "%s():%d - fseek error: %s", __func__, __LINE__, strerror ( errno ) );
#endif
                    // TODO: err status 
                    FDC->regSTATUS = 0x00;
                    FDC->STATUS_SCRIPT = 0;
                    FDC->COMMAND = 0x00;
                    return WD279X_RET_ERR;
                };
                if ( FS_LAYER_FR_OK != FS_LAYER_FWRITE ( FDC->drive[ FDC->MOTOR & 0x03 ].fh, ( uint8_t * ) &"Unicard v1.00\0\0\2\0\0", 18, &ff_readlen ) ) {
#ifdef COMPILE_FOR_EMULATOR
                    ui_show_error ( "%s():%d - fwrite error: %s", __func__, __LINE__, strerror ( errno ) );
#endif
                    // TODO: err status
                    FDC->regSTATUS = 0x00;
                    FDC->STATUS_SCRIPT = 0;
                    FDC->COMMAND = 0x00;
                    return WD279X_RET_ERR;
                };
                if ( 18 != ff_readlen ) {
#ifdef COMPILE_FOR_EMULATOR
                    ui_show_error ( "%s():%d - fwrite error: %s", __func__, __LINE__, strerror ( errno ) );
#endif
                    FDC->regSTATUS = 0x00;
                    FDC->STATUS_SCRIPT = 0;
                    FDC->COMMAND = 0x00;
                    return WD279X_RET_ERR;
                }
                memset ( &FDC->buffer, 0x00, sizeof ( FDC->buffer ) );

                uint8_t write_need = 204;
                uint16_t write_length;

                while ( write_need ) {
                    if ( write_need > sizeof ( FDC->buffer ) ) {
                        write_length = sizeof ( FDC->buffer );
                        write_need -= sizeof ( FDC->buffer );
                    } else {
                        write_length = write_need;
                        write_need = 0;
                    };
                    FS_LAYER_FWRITE ( FDC->drive[ FDC->MOTOR & 0x03 ].fh, FDC->buffer, write_length, &ff_readlen );
                    if ( ff_readlen != write_length ) {
#ifdef COMPILE_FOR_EMULATOR
                        ui_show_error ( "%s():%d - fwrite error: %s", __func__, __LINE__, strerror ( errno ) );
#endif
                        // TODO: err status
                        FDC->regSTATUS = 0x00;
                        FDC->STATUS_SCRIPT = 0;
                        FDC->COMMAND = 0x00;
                        return WD279X_RET_ERR;
                    };
                };

            } else {
                memset ( &FDC->buffer, 0x00, sizeof ( FDC->buffer ) );
            };


            // melo se formatovat, ale nikdo s tim nezacal !
        } else if ( FDC->write_track_counter > 100 ) {
            // TODO: data lost
            FDC->regSTATUS = 0x00;
            FDC->STATUS_SCRIPT = 0;
            FDC->COMMAND = 0x00;
            return WD279X_RET_ERR;
        };


        // cekame az prijde identifikacni znacka
    } else if ( FDC->write_track_stage == 1 ) {

        // prisla identifikacni znacka
        if ( *io_data == 0x01 ) { // ~0xfe
            FDC->write_track_stage = 2;
            FDC->write_track_counter = 0;

            // zacina fyzicky prvni sektor na stope
            // docasnou tabulku sektoru si vytvorime ve FDC->buffer
            FDC->buffer [ 0 ] = FDC->regTRACK;
            FDC->buffer [ 1 ] = FDC->SIDE;
            // 2 - 3 unused, 4 sector size, 
            // 5 number of sectors
            FDC->buffer [ 5 ] = 1;
            //6 GAP#3 length, 7 filler byte
            FDC->buffer [ 6 ] = 0x4e;
            FDC->buffer [ 7 ] = 0xe5;
            FDC->buffer_pos = 8; // tady uz pokracuje pouze seznam ID pro jednotlive sektory

            // neobdrzeli jsme identifikacni znacku
        } else if ( FDC->write_track_counter > 100 ) {
            // TODO: data lost
            FDC->regSTATUS = 0x00;
            FDC->STATUS_SCRIPT = 0;
            FDC->COMMAND = 0x00;
            return WD279X_RET_ERR;
        };

        // po identifikacni znacce si precteme
        // stopu, stranu, sektor a delku sektoru
    } else if ( FDC->write_track_stage == 2 ) {
        if ( FDC->write_track_counter <= 4 ) {
            // ulozime si ID sektoru
            if ( FDC->write_track_counter == 3 ) {
                FDC->buffer [ FDC->buffer_pos++ ] = ~*io_data;

                // ulozime si velikost sektoru
                // predpokladame, ze velikost vsech sektoru na stope musi byt stejna
            } else if ( FDC->write_track_counter == 4 ) {
                FDC->buffer [ 4 ] = ~*io_data;
            };

            // cekame na znacku dat
        } else if ( *io_data == 0x04 || *io_data == 0x07 ) { // 0xfb
            FDC->write_track_stage = 3;
            FDC->write_track_counter = 0;
            FDC->DATA_COUNTER = FDC->buffer [ 4 ] * 0x0100;

            // znacka dat neprisla
        } else if ( FDC->write_track_counter > 100 ) {
            // TODO: data lost
            FDC->regSTATUS = 0x00;
            FDC->STATUS_SCRIPT = 0;
            FDC->COMMAND = 0x00;
            return WD279X_RET_ERR;
        };

        // cteme obsah formatovaneho sektoru
    } else if ( FDC->write_track_stage == 3 ) {

        // prvni bajt si ulozime
        if ( FDC->write_track_counter == 1 ) {
            FDC->buffer [ sizeof ( FDC->buffer ) - 1 ] = ~*io_data;
        };

        // posledni bajt sektoru?
        if ( FDC->write_track_counter > FDC->DATA_COUNTER ) {
            FDC->write_track_stage = 4;
            FDC->write_track_counter = 0;
            FDC->DATA_COUNTER = 0;


            DBGPRINTF ( DBGINF, "WRITE TRACK - finished sector field DRIVE: 0x%02x, TRACK: 0x%02x, SIDE: 0x%02x, SECTOR: 0x%02x, SIZE: 0x%02x, value: 0x%02x\n",
                        FDC->MOTOR & 0x03,
                        FDC->regTRACK,
                        FDC->SIDE,
                        FDC->buffer [ FDC->buffer_pos - 1 ],
                        FDC->buffer [ 4 ],
                        FDC->buffer [ sizeof ( FDC->buffer ) - 1 ] );

        };

        // zapis do sektoru skoncil, tak cekame zda prijde dalsi,
        // nebo zda uz je konec stopy
    } else if ( FDC->write_track_stage == 4 ) {
        if ( *io_data == 0x01 ) { //0xfe
            FDC->write_track_stage = 2;
            FDC->write_track_counter = 0;
            FDC->buffer [ 5 ]++; // zvysime cislo s informaci o poctu sektoru na stope

            // zrejme konec stopy
        } else if ( FDC->write_track_counter > 200 ) {
            FDC->COMMAND = 0x00;
            FDC->regSTATUS = 0x00;
            FDC->STATUS_SCRIPT = 0;
            FDC->write_track_stage = 5;


            DBGPRINTF ( DBGINF, "WRITE TRACK - finishing DRIVE: 0x%02x, TRACK: 0x%02x, SIDE: 0x%02x\n",
                        FDC->MOTOR & 0x03,
                        FDC->regTRACK,
                        FDC->SIDE );


            // zapiseme stopu do DSK

            if ( wd279x_set_track ( FDC ) ) return WD279X_RET_ERR; // stopa nenalezena
            // TODO: err status

            DBGPRINTF ( DBGINF, "new track offset: 0x%x\n", FDC->drive[ FDC->MOTOR & 0x03 ].track_offset );


            if ( FS_LAYER_FR_OK != FS_LAYER_FSEEK ( FDC->drive[ FDC->MOTOR & 0x03 ].fh, FDC->drive[ FDC->MOTOR & 0x03 ].track_offset ) ) {
#ifdef COMPILE_FOR_EMULATOR
                ui_show_error ( "%s():%d - fseek error: %s", __func__, __LINE__, strerror ( errno ) );
#endif
                // TODO: err sts
                return WD279X_RET_ERR;
            };

            FS_LAYER_FWRITE ( FDC->drive[ FDC->MOTOR & 0x03 ].fh, ( uint8_t * ) &"Track-Info\x0d\x0a\0\0\0\0", 16, &ff_readlen );
            if ( ff_readlen != 16 ) {
#ifdef COMPILE_FOR_EMULATOR
                ui_show_error ( "%s():%d - fwrite error: %s", __func__, __LINE__, strerror ( errno ) );
#endif
                return WD279X_RET_ERR; // write error
            };
            // TODO: err status
            FS_LAYER_FWRITE ( FDC->drive[ FDC->MOTOR & 0x03 ].fh, FDC->buffer, 8, &ff_readlen );
            if ( ff_readlen != 8 ) {
#ifdef COMPILE_FOR_EMULATOR
                ui_show_error ( "%s():%d - fwrite error: %s", __func__, __LINE__, strerror ( errno ) );
#endif
                return WD279X_RET_ERR; // write error
            }
            // TODO: err status

            FDC->buffer_pos = 8; // opet na zacatek tabulky sektoru
            // zapiseme info o kazdem sektoru
            FDC->buffer [ 3 ] = FDC->buffer [ 4 ]; // sector size 
            FDC->buffer [ 4 ] = 0x00; // (info z DSK) FDC status register 1
            FDC->buffer [ 5 ] = 0x00; // (info z DSK) FDC status register 2
            FDC->buffer [ 6 ] = 0; //( FDC->buffer [ 3 ] * 0x0100 ) & 0xff; // skutecna velikost sektoru spodni bajt
            FDC->buffer [ 7 ] = ( FDC->buffer [ 3 ] ); // * 0x0100 ) >> 8; // skutecna velikost sektoru horni bajt

            uint8_t i;
            uint8_t all_sec_size = 0;
            for ( i = 1; i <= 29; i++ ) {
                if ( FDC->buffer_pos != 0 ) {
                    if ( FDC->buffer [ FDC->buffer_pos ] != 0x00 ) {
                        FDC->buffer [ 2 ] = FDC->buffer [ FDC->buffer_pos ];
                        FDC->buffer_pos++;
                        all_sec_size += FDC->buffer [ 3 ];
                    } else {
                        memset ( FDC->buffer, 0x00, 8 );
                        FDC->buffer_pos = 0;
                    };
                };
                FS_LAYER_FWRITE ( FDC->drive[ FDC->MOTOR & 0x03 ].fh, FDC->buffer, 8, &ff_readlen );
                if ( ff_readlen != 8 ) {
#ifdef COMPILE_FOR_EMULATOR
                    ui_show_error ( "%s():%d - fwrite error: %s", __func__, __LINE__, strerror ( errno ) );
#endif
                    return WD279X_RET_ERR; // write error
                };

                // TODO: err status
            };

            // fyzicky zapis vsech sektoru na stope
            memset ( &FDC->buffer, FDC->buffer [ sizeof ( FDC->buffer ) - 1 ], sizeof ( FDC->buffer ) );
            uint16_t write_need = all_sec_size * 0x0100;
            uint16_t write_length;

            while ( write_need ) {
                if ( write_need > sizeof ( FDC->buffer ) ) {
                    write_length = sizeof ( FDC->buffer );
                    write_need -= sizeof ( FDC->buffer );
                } else {
                    write_length = write_need;
                    write_need = 0;
                };
                FS_LAYER_FWRITE ( FDC->drive[ FDC->MOTOR & 0x03 ].fh, FDC->buffer, write_length, &ff_readlen );
                if ( ff_readlen != write_length ) {
#ifdef COMPILE_FOR_EMULATOR
                    ui_show_error ( "%s():%d - fwrite error: %s", __func__, __LINE__, strerror ( errno ) );
#endif
                    return WD279X_RET_ERR; // write error
                };
                // TODO: err status
            };

            // urizneme konec DSK souboru
            /*                             int32_t file_pos = MySD_get_file_pos ( FDC->drive[ FDC->MOTOR & 0x03 ].fh ); */

            /*                             int32_t offset = 0; */
            /*                             if ( FR_OK != f_lseek ( &FDC->drive[ FDC->MOTOR & 0x03 ].fh, offset) ) { */
            /* // TODO: err sts */
            /*                                 return RETURN_FDC_ERR; */
            /*                             }; */

            if ( FS_LAYER_FR_OK != FS_LAYER_FSYNC ( FDC->drive[ FDC->MOTOR & 0x03].fh ) ) {
#ifdef COMPILE_FOR_EMULATOR
                ui_show_error ( "%s():%d - fsync error: %s", __func__, __LINE__, strerror ( errno ) );
#endif
                // TODO: err stat
                return WD279X_RET_ERR;
            };

            if ( FS_LAYER_FR_OK != FS_LAYER_FTRUNCATE ( FDC->drive[ FDC->MOTOR & 0x03].fh ) ) {
#ifdef COMPILE_FOR_EMULATOR
                ui_show_error ( "%s():%d - ftruncate error: %s", __func__, __LINE__, strerror ( errno ) );
#endif
                // TODO: err stat
                return WD279X_RET_ERR;
            };

            if ( FS_LAYER_FR_OK != FS_LAYER_FSYNC ( FDC->drive[ FDC->MOTOR & 0x03].fh ) ) {
#ifdef COMPILE_FOR_EMULATOR
                ui_show_error ( "%s():%d - fsync error: %s", __func__, __LINE__, strerror ( errno ) );
#endif
                // TODO: err stat
                return WD279X_RET_ERR;
            };


            // upravime tabulku stop

            int32_t offset = 0x30; // info o poctu stop
            if ( FS_LAYER_FR_OK != FS_LAYER_FSEEK ( FDC->drive[FDC->MOTOR & 0x03].fh, offset ) ) {
#ifdef COMPILE_FOR_EMULATOR
                ui_show_error ( "%s():%d - fseek error: %s", __func__, __LINE__, strerror ( errno ) );
#endif
                return WD279X_RET_ERR; // seek error
            };
            // TODO: err stat
            if ( FDC->SIDE == 1 ) {
                FDC->buffer [ 0 ] = FDC->regTRACK + 1;
            } else {
                FDC->buffer [ 0 ] = FDC->regTRACK;
            };
            FS_LAYER_FWRITE ( FDC->drive[FDC->MOTOR & 0x03].fh, FDC->buffer, 1, &ff_readlen );
            if ( ff_readlen != 1 ) {
#ifdef COMPILE_FOR_EMULATOR
                ui_show_error ( "%s():%d - fwrite error: %s", __func__, __LINE__, strerror ( errno ) );
#endif
                return WD279X_RET_ERR; // write error
            };
            // TODO: err status
            // velikost ulozene stopy
            FDC->buffer [ 0 ] = all_sec_size + 1;
            offset = 0x34 + ( FDC->regTRACK * 2 ) + FDC->SIDE;
            if ( FS_LAYER_FR_OK != FS_LAYER_FSEEK ( FDC->drive[FDC->MOTOR & 0x03].fh, offset ) ) {
#ifdef COMPILE_FOR_EMULATOR
                ui_show_error ( "%s():%d - fseek error: %s", __func__, __LINE__, strerror ( errno ) );
#endif
                return WD279X_RET_ERR; // seek error
            };
            // TODO: err stat
            FS_LAYER_FWRITE ( FDC->drive[FDC->MOTOR & 0x03].fh, FDC->buffer, 1, &ff_readlen );
            if ( ff_readlen != 1 ) {
#ifdef COMPILE_FOR_EMULATOR
                ui_show_error ( "%s():%d - fwrite error: %s", __func__, __LINE__, strerror ( errno ) );
#endif
                return WD279X_RET_ERR; // write error
            };
            // TODO: err status
            if ( FS_LAYER_FR_OK != FS_LAYER_FSYNC ( FDC->drive[FDC->MOTOR & 0x03].fh ) ) {
#ifdef COMPILE_FOR_EMULATOR
                ui_show_error ( "%s():%d - fsync error: %s", __func__, __LINE__, strerror ( errno ) );
#endif
                return WD279X_RET_ERR; // TODO: err stat
            };
            DBGPRINTF ( DBGINF, "WRITE TRACK - finished\n" );
        };
    };

#if (DBGLEVEL & DBGINF)

    if ( last_write_track_stage != FDC->write_track_stage ) {

        DBGPRINTF ( DBGINF, "WRITE TRACK - stage changed 0x%02x\n", FDC->write_track_stage );

    };
#endif

    FDC->write_track_counter++;
    return WD279X_RET_OK;
}


/*
 * Datove zpracovani prikazu WRITE SECTOR.
 * 
 */
int wd279x_do_write_sector ( st_WD279X *FDC, unsigned char *io_data ) {

    unsigned int wrlen;

    /* empty drive */
#ifdef FS_LAYER_FATFS
    if ( !FDC->drive[ FDC->MOTOR & 0x03 ].fh.fs ) {
#else
    if ( !FDC->drive[ FDC->MOTOR & 0x03 ].dsk_in_drive ) {
#endif
        DBGPRINTF ( DBGWAR, "Empty drive, NOT READY for write data into sector, FDC = %s, drive_id = %d\n", FDC->name, FDC->MOTOR & 0x03 );
        FDC->regSTATUS = 0x80; /* not ready */
        return WD279X_RET_ERR;
    };

#if (DBGLEVEL & DBGINF)
    if ( FDC->DATA_COUNTER == FDC->drive[ FDC->MOTOR & 0x03 ].sector_size ) {
        DBGPRINTF ( DBGINF, "Writing first byte into FDD sector. Debug messages are suppressed.\n" );
    };
#endif


#if (DBGLEVEL & DBGINF)
    SUPPRESSED_DBGMSG = 1;
#endif

    FDC->buffer [ FDC->buffer_pos ] = ~*io_data;

    FDC->DATA_COUNTER--;

    uint16_t fdd_io_size;
    if ( FDC->drive[ FDC->MOTOR & 0x03 ].sector_size < sizeof ( FDC->buffer ) ) {
        fdd_io_size = FDC->drive[ FDC->MOTOR & 0x03 ].sector_size;
    } else {
        fdd_io_size = sizeof ( FDC->buffer );
    };
    if ( FDC->buffer_pos == fdd_io_size - 1 ) {
        FDC->buffer_pos = 0;
        FS_LAYER_FWRITE ( FDC->drive[FDC->MOTOR & 0x03].fh, FDC->buffer, fdd_io_size, &wrlen );
        if ( wrlen != fdd_io_size ) {
            DBGPRINTF ( DBGERR, "FDControllerMain(): error when writing DSK file! readlen: 0x%02x\n", wrlen );
#ifdef COMPILE_FOR_EMULATOR
            ui_show_error ( "%s():%d - fwrite error: %s", __func__, __LINE__, strerror ( errno ) );
#endif
        };
    } else {
        FDC->buffer_pos++;
    };


    if ( !FDC->DATA_COUNTER ) {
        DBGPRINTF ( DBGINF, "Writing last byte into FDD sector.\nDRIVE: 0x%02x, TRACK: 0x%02x, SIDE: 0x%02x, SECTOR: 0x%02x, track_offset: 0x%x\n",
                    FDC->MOTOR & 0x03,
                    FDC->drive[ FDC->MOTOR & 0x03 ].TRACK,
                    FDC->drive[ FDC->MOTOR & 0x03 ].SIDE,
                    FDC->drive[ FDC->MOTOR & 0x03 ].SECTOR,
                    FDC->drive[ FDC->MOTOR & 0x03 ].track_offset );


        if ( FS_LAYER_FR_OK != FS_LAYER_FSYNC ( FDC->drive[FDC->MOTOR & 0x03].fh ) ) {
#ifdef COMPILE_FOR_EMULATOR
            ui_show_error ( "%s():%d - fssync error: %s", __func__, __LINE__, strerror ( errno ) );
#endif
            // TODO: err stat ?
            DBGPRINTF ( DBGERR, "FDControllerMain(): error syncing disk\n" );
        };


        if ( FDC->MULTIBLOCK_RW ) {

            DBGPRINTF ( DBGINF, "Multiblock sector writing - sector is finished. Go to next...\n" );

            FDC->regSECTOR = FDC->drive[ FDC->MOTOR & 0x03 ].SECTOR + 1;

            if ( wd279x_seek_to_sector ( FDC, FDC->MOTOR & 0x03, FDC->regSECTOR ) ) {

                DBGPRINTF ( DBGINF, "Not found next sector on this track. Sending RNF!\n" );

                FDC->regSECTOR--;
                FDC->STATUS_SCRIPT = 4;

            } else {
                FDC->DATA_COUNTER = FDC->drive[ FDC->MOTOR & 0x03 ].sector_size;
                FDC->buffer_pos = 0;
                FDC->STATUS_SCRIPT = 2;
            };

        } else {
            FDC->COMMAND = 0x00;
            FDC->regSTATUS = 0x00;
            FDC->STATUS_SCRIPT = 0;
        };
    };

    return WD279X_RET_OK;
}


typedef enum en_FDCPORT_OFFSET {
    FDCPORT_CMDSTS = 0,
    FDCPORT_TRACK,
    FDCPORT_SECTOR,
    FDCPORT_DATA,
    FDCPORT_MOTOR,
    FDCPORT_SIDE,
    FDCPORT_DENSITY,
    FDCPORT_EINT,
} en_FDCPORT_OFFSET;


/*
 * IORQ zapis do WD279x
 * 
 */
int wd279x_write_byte ( st_WD279X *FDC, int i_addroffset, unsigned char *io_data ) {

    en_FDCPORT_OFFSET off = i_addroffset & 0x07;


    switch ( off ) {

            /*
             * 
             * Zapis do COMMAND registru
             * 
             */
        case FDCPORT_CMDSTS:

            /* pokud jsme byli v INT rezimu, tak jej vyresetujeme */
            if ( FDC->waitForInt ) {
                FDC->waitForInt = 0;
                //DBGPRINTF ( DBGINF, "FDC = %s, INT is deactivated\n", FDC->name );
                /*mzint_ResInterrupt ( mzintFDC );*/
            };

            FDC->COMMAND = *io_data;
            FDC->reading_status_counter = 0;

            return wd279x_do_command ( FDC );



            /*
             * 
             * Zapis do TRACK registru
             * 
             */
        case FDCPORT_TRACK:
#define DBGENA_WRITE_FDCPORT_TRACK    0
            /*
             * cp/m testuje jaka je aktualni stopa tak, ze do track registru zapise 0x00
             * a nasledne jej cte a ocekava tam aktualni stopu na ktere je mechanika.
             * Takze pokud chce nekdo nastavit stopu 0, tak mu to nepovolime, protoze
             * pak by cp/m neustale seekovala.
             * 
             */
            if ( *io_data != 0xff ) {
                FDC->regTRACK = ~*io_data;
#if ( DBGLEVEL & DBGINF )
#if DBGENA_WRITE_FDCPORT_TRACK
                DBGPRINTF ( DBGINF, "FDC = %s, drive_id = %d, SET regTRACK: 0x%02x\n", FDC->name, FDC->MOTOR & 0x03, FDC->regTRACK );
            } else {
                DBGPRINTF ( DBGINF, "FDC = %s, drive_id = %d, SET regTRACK: 0x00 is ignored - cp/m patch\n", FDC->name, FDC->MOTOR & 0x03 );
#endif
#endif
            };
            break;


            /*
             * 
             * Zapis do SECTOR registru
             * 
             */
        case FDCPORT_SECTOR:
            FDC->regSECTOR = ~*io_data;
            DBGPRINTF ( DBGINF, "FDC = %s, drive_id = %d, SET regSECTOR: %d\n", FDC->name, FDC->MOTOR & 0x03, FDC->regSECTOR );
            break;


            /*
             * 
             * Zapis do DATA registru
             * 
             */
        case FDCPORT_DATA:

            /* pokud byl pro MZ-800 vystaven /INT, tak jej deaktivujeme */
            if ( FDC->waitForInt ) {
                FDC->waitForInt = 0;
                //DBGPRINTF ( DBGINF, "FDC = %s, INT is deactivated\n", FDC->name );
                /*mzint_ResInterrupt ( mzintFDC );*/
            };

            FDC->reading_status_counter = 0;


            /* probiha formatovani? */
            if ( FDC->COMMAND == 0x0f || FDC->COMMAND == 0x0b ) {
                return wd279x_do_write_track ( FDC, io_data );
            };


            /* Write byte only into DATA register */
            if ( !FDC->DATA_COUNTER ) {
                FDC->regDATA = ~*io_data;
                DBGPRINTF ( DBGINF, "FDC SET regDATA: 0x%02x\n", FDC->regDATA );
                return WD279X_RET_OK;
            };


            /* WRITE sector */
            return wd279x_do_write_sector ( FDC, io_data );
            break;


            /*
             * 
             * Zapis na port motoru
             * 
             */
        case FDCPORT_MOTOR:
            /* ID mechaniky (0. - 1. bit) se zmeni jen pokud je soucasne nataven i 2. bit */
            /* 7. bit zapina/vypina motor */
            if ( 0x04 == ( *io_data & 0x04 ) ) {
                FDC->MOTOR = *io_data & 0x83;
            } else {
                if ( 0x80 == ( *io_data & 0x80 ) ) {
                    //                    if ( ( FDC->MOTOR & 0x80 ) == 0 )  {
                    //                        FDC->regSTATUS = 0x00;
                    //                    };
                    FDC->MOTOR = FDC->MOTOR | 0x80;
                } else {
                    FDC->MOTOR = FDC->MOTOR & 0x03;
                };
            };
            FDC->regSTATUS = 0x00;
            if ( FDC->MOTOR & 0x03 == 1 ) {
                printf ( "ted\n" );
            }
            if ( ( FDC->MOTOR & 0x03 ) == 1 ) {
                printf ( "ted\n" );
            };
            DBGPRINTF ( DBGINF, "FDC = %s, drive_id = %d, Set MOTOR: %d, recv.: 0x%02x\n", FDC->name, FDC->MOTOR & 0x03, FDC->MOTOR, *io_data );
            break;


            /*
             * 
             * Zapis na port strany
             * 
             */
        case FDCPORT_SIDE:
            FDC->SIDE = *io_data & 0x01;
            DBGPRINTF ( DBGINF, "FDC = %s, drive_id = %d, Set SIDE: %d\n", FDC->name, FDC->MOTOR & 0x03, FDC->SIDE );
            break;


            /*
             * 
             * Zapis na port density
             * 
             */
        case FDCPORT_DENSITY:
            FDC->DENSITY = *io_data & 0x01;
            DBGPRINTF ( DBGINF, "FDC = %s, drive_id = %d, Set SIDE: %d\n", FDC->name, FDC->MOTOR & 0x03, FDC->DENSITY );
            break;


            /*
             * 
             * Zapis na port EINT
             * 
             */
        case FDCPORT_EINT:
            FDC->EINT = *io_data & 0x01;
            /* pokud uz neni o INT rezim zajem, tak vypneme /INT signal */
            if ( !FDC->EINT ) {
                FDC->waitForInt = 0;
                //DBGPRINTF ( DBGINF, "FDC = %s, INT is deactivated\n", FDC->name );
                /*mzint_ResInterrupt ( mzintFDC );*/
            };
            DBGPRINTF ( DBGINF, "FDC = %s, drive_id = %d, Set EINT: %d\n", FDC->name, FDC->MOTOR & 0x03, FDC->EINT );
            break;
    };

    return WD279X_RET_OK;
}


/*
 * IORQ cteni z WD279x
 * 
 */
int wd279x_read_byte ( st_WD279X *FDC, int i_addroffset, unsigned char *io_data ) {

    en_FDCPORT_OFFSET off = i_addroffset & 0x07;
    unsigned int readlen;

    switch ( off ) {

            /*
             * 
             * Cteni STATUS registru
             * 
             */
        case FDCPORT_CMDSTS:

#ifdef FS_LAYER_FATFS
            if ( !FDC->drive[ FDC->MOTOR & 0x03 ].fh.fs ) {
#else
            if ( !FDC->drive[ FDC->MOTOR & 0x03 ].dsk_in_drive ) {
#endif

                FDC->STATUS_SCRIPT = 0;

                // Pokud startuje cp/m a testuje si status po nastaveni neexistujiciho sectoru, tak si na zadny timeout nehrajeme.
            } else if ( FDC->regSTATUS != 0x18 ) {

                // Hack pro diskovy MZ-800 BASIC, ktery po zapisu provadi overeni ctenim
                // sektoru v multiblokovem rezimu, ale samotna data necte a pouze sleduje
                // zda ve status registru neprijde chyba - dokud multiblokovym ctenim radic 
                // neprojde vsechny kontrolovane sektory
                // Pokud je tedy nastaveno multiblokove cteni a 10x se MZ-800 zeptal na status
                // a necetl zadne data, tak automaticky prejdeme na dalsi sektor.
                // 
                // Stejny mechanismus verifikace je pouzity i v cp/m 4.1 format4.com, akorat se
                // pouziva jednoblokove cteni.
                if ( ( FDC->DATA_COUNTER == FDC->drive[ FDC->MOTOR & 0x03 ].sector_size ) && ( FDC->COMMAND >> 5 == 0x03 ) ) { /* 0b011 */

                    FDC->reading_status_counter++;
                    if ( FDC->reading_status_counter > 10 ) {
                        FDC->reading_status_counter = 0;

                        // probiha multiblokove cteni - prejdeme na dalsi sektor
                        if ( FDC->MULTIBLOCK_RW ) {

                            DBGPRINTF ( DBGINF, "Sekvencni cteni - predchozi sektor skoncil TIMEOUTEM , tzn. prechod na dalsi.\n" );

                            FDC->drive[ FDC->MOTOR & 0x03 ].SECTOR++;
                            FDC->regSECTOR = FDC->drive[ FDC->MOTOR & 0x03 ].SECTOR;

                            if ( wd279x_seek_to_sector ( FDC, FDC->MOTOR & 0x03, FDC->regSECTOR ) ) {
                                // sector not found!

                                DBGPRINTF ( DBGINF, "Dalsi sector s naslednym poradovym cislem uz tu neni! Posilam RNF\n" );

                                //FDC->regSECTOR--;
                                FDC->STATUS_SCRIPT = 4;
                            } else {

                                uint16_t fdd_io_size;
                                if ( FDC->drive[ FDC->MOTOR & 0x03 ].sector_size < sizeof ( FDC->buffer ) ) {
                                    fdd_io_size = FDC->drive[ FDC->MOTOR & 0x03 ].sector_size;
                                } else {
                                    fdd_io_size = sizeof ( FDC->buffer );
                                };

                                FS_LAYER_FREAD ( FDC->drive[ FDC->MOTOR & 0x03 ].fh, FDC->buffer, fdd_io_size, &readlen );
                                if ( readlen != fdd_io_size ) {

                                    DBGPRINTF ( DBGERR, "FDControllerMain(): error when reading2 DSK file! readsize = 0x%02x\n", readlen );
#ifdef COMPILE_FOR_EMULATOR
                                    ui_show_error ( "%s():%d - fread error: %s", __func__, __LINE__, strerror ( errno ) );
#endif


                                    DBGPRINTF ( DBGINF, "DRIVE: 0x%02x, TRACK: 0x%02x, SIDE: 0x%02x, SECTOR: 0x%02x, track_offset: 0x%x\n",
                                                FDC->MOTOR & 0x03,
                                                FDC->drive[ FDC->MOTOR & 0x03 ].TRACK,
                                                FDC->drive[ FDC->MOTOR & 0x03 ].SIDE,
                                                FDC->drive[ FDC->MOTOR & 0x03 ].SECTOR,
                                                FDC->drive[ FDC->MOTOR & 0x03 ].track_offset );

                                    // status code?
                                    return WD279X_RET_ERR;
                                };


                                FDC->buffer_pos = 0;
                                FDC->DATA_COUNTER = FDC->drive[ FDC->MOTOR & 0x03 ].sector_size;

                                FDC->STATUS_SCRIPT = 2;
                            };

                            // probiha jednoblokove cteni - vyrobime "konec sektoru"
                        } else {

                            DBGPRINTF ( DBGINF, "Cteni sektoru ukonceno TIMEOUTEM.\n" );

                            FDC->DATA_COUNTER = 0;
                            FDC->COMMAND = 0x00;
                            FDC->regSTATUS = 0x00;
                            FDC->STATUS_SCRIPT = 0;
                        };
                    };
                };
            };

            //DBGPRINTF(DBGINF, "FDC->STATUS_SCRIPT: 0x%02x\n", FDC->STATUS_SCRIPT);

            switch ( FDC->STATUS_SCRIPT ) {

                case 1:
                    // Status after normal commands
                    // on first status reading put BUSY and on all next readings put real status code
                    *io_data = ~( FDC->regSTATUS | 0x01 );
                    FDC->STATUS_SCRIPT = 0;
                    break;

                case 2:
                    // Status code after multiblock R/W sector finished and go to next
                    // 1x BUSY, and next on reading BUSY + DRQ
                    *io_data = ~0x01; // BUSY
                    FDC->regSTATUS = 0x03; // BUSY + DRQ
                    FDC->STATUS_SCRIPT = 0;
                    break;

                case 3:
                    // hack for cp/m 1.3, when cp/m on startup is requested reading from unknown sector id
                    *io_data = ~0x01; // BUSY
                    FDC->regSTATUS = 0x18;
                    FDC->STATUS_SCRIPT = 0;
                    break;

                case 4:
                    // Status code after multiblock R/W sector finished and next not found
                    // 1x BUSY + RNF, and next 0x00
                    *io_data = ~0x11; // BUSY + RNF
                    FDC->regSTATUS = 0x00;
                    FDC->STATUS_SCRIPT = 0;
                    break;

                case 0xff:
                    // experimental script for hacking
                    //fdc.STATUS = 0x00; // -TRK00
                    *io_data = ~( FDC->regSTATUS & ~0x06 );
                    FDC->regSTATUS++;
                    break;

                default:
                    *io_data = ~FDC->regSTATUS;
            };

#if (DBGLEVEL & DBGINF)
            // suppressed debug messages when reading data from sector
            // regSTATUS == 0x18 is when cp/m 1.3 starting
            // ( ( FDC->COMMAND == 0x0f || FDC->COMMAND == 0x0b ) == WRITE TRACK
            if ( !( ( SUPPRESSED_DBGMSG == 1 ) || ( FDC->regSTATUS == 0x18 ) || ( FDC->COMMAND == 0x0f || FDC->COMMAND == 0x0b ) ) ) {
                // chaky
                //DBGPRINTF(DBGINF, "FDC Get regSTATUS: 0x%02x\n", ~*io_data );

            };
#endif
            //DBGPRINTF(DBGINF, "FDC Get regSTATUS: 0x%02x\n", ~*io_data );
            break;


            /*
             * 
             * Cteni TRACK registru
             * 
             */
        case FDCPORT_TRACK:
#define DBGENA_READ_FDCPORT_TRACK    0
            if ( ( FDC->regTRACK == 0x5a ) && ( FDC->mask != WD279X_MASK_NONE ) ) {

                if ( FDC->mask == WD279X_MASK_EVERY_TIME ) {
#if DBGENA_READ_FDCPORT_TRACK
                    DBGPRINTF ( DBGINF, "FDC = %s, drive_id = %d, Get TRACK (ROM FDC TEST): %d - (mask every time: 0x00)\n", FDC->name, FDC->MOTOR & 0x03, FDC->regTRACK );
#endif
                    *io_data = 0xff;
                } else {
                    /* FDC->mask == WD279X_MASK_EMPTY */
#ifdef FS_LAYER_FATFS
                    if ( !FDC->drive[ 0 ].fh.fs ) {
#else
                    if ( !FDC->drive[ 0 ].dsk_in_drive ) {
#endif
#if DBGENA_READ_FDCPORT_TRACK
                        DBGPRINTF ( DBGINF, "FDC = %s, drive_id = %d, Get TRACK (ROM FDC TEST): %d - DSK not in drive (mask: 0x00)\n", FDC->name, FDC->MOTOR & 0x03, FDC->regTRACK );
#endif
                        *io_data = 0xff;
                    } else {
#if DBGENA_READ_FDCPORT_TRACK
                        DBGPRINTF ( DBGINF, "FDC = %s, drive_id = %d, Get TRACK (ROM FDC TEST): %d - DSK in drive (no mask)\n", FDC->name, FDC->MOTOR & 0x03, FDC->regTRACK );
#endif
                        *io_data = ~FDC->regTRACK;
                    };
                };

            } else {
#if DBGENA_READ_FDCPORT_TRACK
                DBGPRINTF ( DBGINF, "FDC = %s, drive_id = %d, Get TRACK: 0x%02x\n", FDC->name, FDC->MOTOR & 0x03, FDC->regTRACK );
#endif
                *io_data = ~FDC->regTRACK;
            };
            break;


            /*
             * 
             * Cteni SECTOR registru
             * 
             */
        case FDCPORT_SECTOR:
            // pokud bezi motor, tak vracime stkutecny sektor nad kterym jsme
            // jinak vracime co, co si kdo nastavil do registru sektoru 
            if ( FDC->MOTOR & 0x80 ) {
                FDC->regSECTOR = FDC->drive[ FDC->MOTOR & 0x03 ].SECTOR;
            };
            *io_data = ~FDC->regSECTOR;

            DBGPRINTF ( DBGINF, "FDC Get regSECTOR: 0x%02x\n", FDC->regSECTOR );

            break;


            /*
             * 
             * Cteni DATA registru
             * 
             */
        case FDCPORT_DATA:

            /* pokud byl pro MZ-800 vystaven /INT, tak jej deaktivujeme */
            if ( FDC->waitForInt ) {
                FDC->waitForInt = 0;
                //DBGPRINTF ( DBGINF, "FDC = %s, INT is deactivated\n", FDC->name );
                /*mzint_ResInterrupt ( mzintFDC );*/
            };

            FDC->reading_status_counter = 0;


            /* empty drive */
#ifdef FS_LAYER_FATFS
            if ( !FDC->drive[ FDC->MOTOR & 0x03 ].fh.fs ) {
#else
            if ( !FDC->drive[ FDC->MOTOR & 0x03 ].dsk_in_drive ) {
#endif
                DBGPRINTF ( DBGWAR, "Empty drive, NOT READY for read data from sector, FDC = %s, drive_id = %d\n", FDC->name, FDC->MOTOR & 0x03 );
                FDC->regSTATUS = 0x80; /* not ready */
                *io_data = 0xff;
                return WD279X_RET_ERR;
            };


            // requested DATA from TRACK ADDR
            if ( FDC->COMMAND == 0x3f ) {

                *io_data = ~FDC->buffer[ 6 - FDC->DATA_COUNTER ];

                DBGPRINTF ( DBGINF, "TRACK ADDR READING (0x%02x) - 0x%02x\n",
                            FDC->DATA_COUNTER - 1,
                            FDC->buffer[ 6 - FDC->DATA_COUNTER ] );


                FDC->DATA_COUNTER--;

                if ( !FDC->DATA_COUNTER ) {
                    FDC->COMMAND = 0x00;
                    FDC->regSTATUS = 0x00;
                    FDC->STATUS_SCRIPT = 0;
                };

                // requested DATA from SECTOR
            } else {

#if (DBGLEVEL & DBGINF)

                if ( FDC->DATA_COUNTER == FDC->drive[ FDC->MOTOR & 0x03 ].sector_size ) {

                    DBGPRINTF ( DBGINF, "Requested first byte from FDD sector. Debug messages are suppressed.\n" );

                };
#endif

                if ( FDC->DATA_COUNTER ) {

#if (DBGLEVEL & DBGINF)

                    SUPPRESSED_DBGMSG = 1;
#endif

                    *io_data = ~FDC->buffer[ FDC->buffer_pos ];

                    DBGPRINTF ( DBGINF, "Read byte from sector: 0x%02x ( ~ 0x%02x )\n", FDC->buffer[ FDC->buffer_pos ], ( uint8_t ) * io_data );


                    FDC->DATA_COUNTER--;

                    uint16_t fdd_io_size;
                    if ( FDC->drive[ FDC->MOTOR & 0x03 ].sector_size < sizeof ( FDC->buffer ) ) {
                        fdd_io_size = FDC->drive[ FDC->MOTOR & 0x03 ].sector_size;
                    } else {
                        fdd_io_size = sizeof ( FDC->buffer );
                    };

                    if ( FDC->DATA_COUNTER ) {


                        if ( FDC->buffer_pos == fdd_io_size - 1 ) {
                            FDC->buffer_pos = 0;

                            FS_LAYER_FREAD ( FDC->drive[ FDC->MOTOR & 0x03 ].fh, FDC->buffer, fdd_io_size, &readlen );
                            if ( readlen != fdd_io_size ) {

                                DBGPRINTF ( DBGERR, "FDControllerMain(): error when reading3 DSK file! readsize = 0x%02x\n", readlen );
#ifdef COMPILE_FOR_EMULATOR
                                ui_show_error ( "%s():%d - fread error: %s", __func__, __LINE__, strerror ( errno ) );
#endif

                                DBGPRINTF ( DBGERR, "DRIVE: 0x%02x, TRACK: 0x%02x, SIDE: 0x%02x, SECTOR: 0x%02x, track_offset: 0x%x\n",
                                            FDC->MOTOR & 0x03,
                                            FDC->drive[ FDC->MOTOR & 0x03 ].TRACK,
                                            FDC->drive[ FDC->MOTOR & 0x03 ].SIDE,
                                            FDC->drive[ FDC->MOTOR & 0x03 ].SECTOR,
                                            FDC->drive[ FDC->MOTOR & 0x03 ].track_offset );

                                // status code?
                                FDC->regSTATUS = 0x18; // zkusime vystavit RNF a CRC_ERR 
                                return WD279X_RET_ERR;
                            };
                        } else {
                            FDC->buffer_pos++;
                        };

                    } else {
                        /* FDC->DATA_COUNTER == 0; jsme na konci sektoru */

#if (DBGLEVEL & DBGINF)
                        SUPPRESSED_DBGMSG = 0;
                        DBGPRINTF ( DBGINF, "Sector reading finished.\nDRIVE: 0x%02x, TRACK: 0x%02x, SIDE: 0x%02x, SECTOR: 0x%02x, track_offset: 0x%x\n",
                                    FDC->MOTOR & 0x03,
                                    FDC->drive[ FDC->MOTOR & 0x03 ].TRACK,
                                    FDC->drive[ FDC->MOTOR & 0x03 ].SIDE,
                                    FDC->drive[ FDC->MOTOR & 0x03 ].SECTOR,
                                    FDC->drive[ FDC->MOTOR & 0x03 ].track_offset );

#endif

                        if ( FDC->MULTIBLOCK_RW ) {
                            FDC->STATUS_SCRIPT = 2;
                            // hack for MZ BASIC
                            FDC->MULTIBLOCK_RW = 1;


                            DBGPRINTF ( DBGINF, "Multiblock sector reading - sector is finished. Go to next...\n" );

                            FDC->regSECTOR = FDC->drive[ FDC->MOTOR & 0x03 ].SECTOR + 1;

                            if ( wd279x_seek_to_sector ( FDC, FDC->MOTOR & 0x03, FDC->regSECTOR ) ) {

                                DBGPRINTF ( DBGINF, "Not found next sector on this track. Sending RNF!\n" );

                                FDC->regSECTOR--;
                                FDC->STATUS_SCRIPT = 4;

                            } else {
                                FS_LAYER_FREAD ( FDC->drive[ FDC->MOTOR & 0x03 ].fh, FDC->buffer, fdd_io_size, &readlen );
                                if ( readlen != fdd_io_size ) {
                                    DBGPRINTF ( DBGERR, "FDControllerMain(): error when reading4 DSK file!\n" );
#ifdef COMPILE_FOR_EMULATOR
                                    ui_show_error ( "%s():%d - fread error: %s", __func__, __LINE__, strerror ( errno ) );
#endif
                                    // err status
                                    return WD279X_RET_ERR;
                                };

                                FDC->buffer_pos = 0;
                                FDC->DATA_COUNTER = FDC->drive[ FDC->MOTOR & 0x03 ].sector_size;
                                FDC->STATUS_SCRIPT = 2;
                            };

                        } else {
                            FDC->COMMAND = 0x00;
                            FDC->regSTATUS = 0x00;
                            FDC->STATUS_SCRIPT = 0;
                        };
                    };

                } else {
                    // ma nekdo duvod cist data, kdyz si o zadne nerekl? :)
                    DBGPRINTF ( DBGERR, "ERROR?? MZ requested DATA from FDC, but DATA_COUNTER is empty!!!\n" );
                };
            };

            break;

        default:

            DBGPRINTF ( DBGWAR, "FDC UNKNOWN read on PORT offset %c\n",
                        0x30 + off );

            break;
    }

    return WD279X_RET_OK;
}


#ifdef COMPILE_FOR_EMULATOR


void wd279x_reset ( st_WD279X *FDC ) {
    FDC->EINT = FDC->COMMAND = FDC->MOTOR = FDC->DENSITY = FDC->DATA_COUNTER = FDC->MULTIBLOCK_RW = 0x00;
}
#endif


/*
 * Vyhodnoti, zda by mel radic vyvolat MZ interrupt.
 * 
 * Vystup:
 *      0 - zadny interrupt
 *      1 - ano, je cas na interrupt
 */
int wd279x_check_interrupt ( st_WD279X *FDC ) {

    if ( !FDC->EINT ) return 0; /* radic neni v rezimu INT */

    /* Mame pripravena data ke cteni, nebo ocekavame data k zapisu? */
    if ( ( FDC->DATA_COUNTER && ( FDC->COMMAND >> 5 == 0x03 || FDC->COMMAND >> 5 == 0x02 || FDC->COMMAND == 0x3f ) ) || ( FDC->COMMAND == 0x0f || FDC->COMMAND == 0x0b ) ) { /* 0b011, 0b010 */

        /* Signal /INT neposilame neustale, ale jen jednou za cas ... */
        FDC->waitForInt++;

        /* ... tedy po kazdem 2 pozadavku na Unicard */
        if ( FDC->waitForInt > 2 ) {
            //DBGPRINTF ( DBGINF, "FDC = %s, INT is eactivated\n", FDC->name );
            //mzint_SetInterrupt ( mzintFDC );
            return 1; /* je potreba vyvolat MZ interrupt */
        };
    };
    return 0;
}
