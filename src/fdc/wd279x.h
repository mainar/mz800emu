/* 
 * File:   wd279x.h
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 5. srpna 2015, 12:42
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

#ifndef WD279X_H
#define	WD279X_H

#ifdef	__cplusplus
extern "C" {
#endif


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


#ifdef FS_LAYER_FATFS

#include "ff.h"
#define DSK_FILENAME_LENGTH 32

#else

#include <stdio.h>
#define DSK_FILENAME_LENGTH 1024

#endif


#define FDC_NUM_DRIVES  4   /* pocet emulovanych FD mechanik (max 4) */


    /*
     * datovy buffer pro cteni a zapis sektoru
     * Velikost bufferu musi byt takova, aby byl sektor delitelny velikosti bufferu
     *
     * Pri zapisu stopy pouzivame FDC.buffer, tak pokud se nekdo rozhodne,
     * ze zmensi velikost bufferu, tak by se mel nejprve podivat na WRITE TRACK, aby
     * mu potom kus bufferu nechybelo :)
     */
#define WD279X_BUFFER_SIZE  0x100

    typedef struct st_FDDrive {
#ifdef FS_LAYER_FATFS
        char path [ 3 * DSK_FILENAME_LENGTH + 3 ]; /* cesta k DSK souboru */
        FIL fh; /* filehandle otevreneho DSK souboru */
#else
        unsigned dsk_in_drive; /* 1 = mame pripojen DSK */
        FILE *fh; /* filehandle otevreneho DSK souboru */
#endif
        char filename [ DSK_FILENAME_LENGTH ]; /* jmeno DSK souboru */

        uint8_t TRACK; /* stopa se kterou v DSK prave pracujeme */
        uint8_t SECTOR; /* sektor se kterym v DSK prave pracujeme */
        uint8_t SIDE; /* 0.bit urcuje stranu se kterou v DSK prave pracujeme */

        uint32_t track_offset; /* Offset aktualniho sektoru v ramci DSK souboru */
        uint16_t sector_size; /* Velikost aktualniho sektoru v bajtech */
    } st_FDDrive;

    typedef enum en_WD279X_MASK {
        WD279X_MASK_NONE = 0, /* Pokud se ROM dotazuje na radic, tak vzdy dostane kladnou odpoved */
        WD279X_MASK_EMPTY, /* Pokud se ROM dotazuje na radic, tak dostane kladnou odpoved jen pokud je v mechanice 0 vlozen disk */
        WD279X_MASK_EVERY_TIME /* Pokud se ROM dotazuje na radic, tak nikdy nedostane kladnou odpoved */
    } en_WD279X_MASK;

    /* TODO: mam pocit, ze na unikarte jsem zamerne nepouzil typedef, ale jen "struct FDController_s {", protoze to pak sezralo 2x tolik pameti */
    typedef struct st_WD279X {
        char name [ DSK_FILENAME_LENGTH ]; /* identifikace radice - pokud jich emulujeme vic */

        en_WD279X_MASK mask; /* jak se bude chovat FDC pri ROM testu radice - track 0x5a */

        uint8_t regSTATUS; /* Status registr radice */
        uint8_t regDATA; /* Datovy registr radice */
        uint8_t regTRACK; /* Registr stopy */
        uint8_t regSECTOR; /* Registr sektoru */
        uint8_t SIDE; /* 0. bit urcuje pozadovanou stranu */

        uint8_t buffer [ WD279X_BUFFER_SIZE ]; /* datovy buffer pro cteni a zapis sektoru */
        uint16_t buffer_pos; /* ukazatel pozice v datovem bufferu */

        uint8_t COMMAND; /* zde je ulozen FDC command, ktery jsme prijali na portu 0xd8 */

        /* 
         * 0. a 1. bit urcuje FD mechaniku se kterou prave pracujeme 0 - 3
         * 7. bit zapina motor mechaniky (pouzivame pri cteni registru sektoru)
         */
        uint8_t MOTOR;

        uint8_t DENSITY;

        /*
         * 1 => INT rezim je povolen, 0 => zakazan
         * v pripade povoleneho INT rezimu rika radic signalem /INT,
         * ze ma pro MZ-800 pripraveny data
         */
        uint8_t EINT;

        /*
         * Pri vykonavani prikazu cteni a zapisu sektoru, cteni adresy 
         * sektoru je do tohoto citace vlozena velikost dat, ktere budou prijimany,
         * nebo odesilany. Tzn. velukost sektoru, nebo pocet bajt pri cteni adresy.
         * Pri kazdem cteni, nebo zapisu je tato hodnota snizovana.
         * Pokud je v tomto citaci nenulova hodnota, tak to znamena, ze radic ocekava
         * nejaka data v pripade zapisu, nebo ma pripravena data k odberu.
         */
        uint16_t DATA_COUNTER;

        st_FDDrive drive [ FDC_NUM_DRIVES ]; /* jednotlive mechaniky */

        /*
         * 1 - znamena, ze posledni prikaz cteni/zapisu sektoru byl multiblokovy
         * tzn., ze az precteme / zapiseme posledni bajt aktualniho sektoru,
         * tak automaticky prejdeme na dalsi a pokracujeme dokud neprijde prikaz preruseni,
         * nebo dokud uz na stope neexistuje sektor s nasledujicim poradovym cislem
         * 0 - znamena obycejne cteni / zapis, ktere konci s poslednim bajtem sektoru.
         */
        uint8_t MULTIBLOCK_RW;


        uint8_t STATUS_SCRIPT; /* Scenar podle ktereho se ma chovat status registr - viz. cteni status registru */

        uint8_t waitForInt; /* Pokud jsme v rezimu INT, tak si zde radic pocita za jak dlouho poslat dalsi interrupt - viz. FDC.waitForInt()  */

        /*
         * Pokud se formatuje, tak zde je ulozena uroven vychazejici z prijate znacky 
         * 0 - zacatek zapisu stopy (prijimame data pro WRITE TRACK)
         * 1 - dorazila znacka indexu (0xfc)
         * 2 - dorazila znacka adresy (0xfe)
         * 3 - znacka dat sektoru (0xfb)
         * 4 - konec dat
         * 5 - konec stopy
         */
        uint8_t write_track_stage;

        uint16_t write_track_counter; /* Nuluje se vzdy pri zmene znacky, takze podle nej identifikujeme kde se prave nachazime. */

        /*
         * pri cteni a zapisu sektoru pocitame kolikrat po sobe se 
         * cetl regSTATUS bez toho, aniz by se mezi tim pracovalo s regDATA
         * pokud vice, nez 5x, tak se chovame jako kdyby uz byl konec sektoru.
         * Tohle cteni dat bez skutecneho cteni pouzivaji nektere programy jako
         * verifikaci ulozenych dat.
         */
        uint8_t reading_status_counter;

        /* TODO: na unikarte jsem zamerne pouzil rovnou definici "} FDC;" - bez typedef na zacatku - sezere to mene pameti */
    } st_WD279X;


#define WD279X_RET_ERR  1
#define WD279X_RET_OK   0


#ifdef COMPILE_FOR_EMULATOR
    extern void wd279x_reset ( st_WD279X *FDC );
#endif

    extern void wd279x_init ( st_WD279X *FDC, char *name );

    extern void wd279x_close_dsk ( st_WD279X *FDC, uint8_t drive_id );
    extern int wd279x_open_dsk ( st_WD279X *FDC, uint8_t drive_id, char *DSK_filename );

    extern int wd279x_write_byte ( st_WD279X *FDC, int i_addroffset, unsigned char *io_data );
    extern int wd279x_read_byte ( st_WD279X *FDC, int i_addroffset, unsigned char *io_data );
    extern int wd279x_check_interrupt ( st_WD279X *FDC );

#ifdef	__cplusplus
}
#endif

#endif	/* WD279X_H */

