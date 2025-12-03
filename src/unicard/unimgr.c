/* 
 * File:   unimgr.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 22. června 2018, 10:01
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
 * 
 * Dokumentace
 * 
 */
// <editor-fold defaultstate="collapsed" desc="Dokumentace - Status">
/*
 *
 * Prikazy prijate pres CMD port
 * =============================
 *
 * Prikazy se posilaji na CMD port. Vsechny jsou jednobajtove a pokud 
 * nevyzaduji dalsi parametr, tak jsou okamzite vykonany.
 * 
 * Pokud je kdykoliv vlozen prikaz, tak se tim automaticky zrusi vykonavani 
 * predchoziho prikazu (napr. pokud ceka na vstupni parametry,
 * nebo na odebrani vystupnich dat).
 * 
 * V pripade vlozeni prikazu STSR se pouze vynuluje ukazatel statusu (viz. nize) a 
 * nedojde k zadnemu ovlivneni predchoziho prikazu, ani obsahu statusu.
 * 
 * 
 * Vstup:
 * 
 * Pokud prikaz vyzaduje dalsi parametry, tak jsou ocekavany v 
 * predem urcenem poradi na DATA portu a prikaz bude vykonan okmzite po 
 * vlozeni vsech parametru.
 * 
 * Pokud je vstupnim parametrem string, tak za jeho konec je povazovano 
 * prijeti libovolneho znaku s hodnotou mensi nez 0x20.
 * 
 * Pokud jsou vstupnim parametrem dva retezce, tak za oddelovac je povazovan 
 * libovolny jeden znak s hodnotou mensi nez 0x20.
 * 
 * Pokud je vstupnim parametrem retezec, tak tento vstup prochazi uvnitr 
 * unikarty I/O translatorem ASCII.
 * 
 * Pokud je parametrem soubor, nebo adresar (string), tak je mozne pouzit
 * absolutni cestu, ktera zacina od korene "/". Nebo relativni, ktera zacne od CWD.
 * Pouziti relativni cesty neni vhodne u prikazu FDDMOUNT, protoze se pak v teto podobe
 * ulozi do konfigurace. Pri startu unikarty se nastavi CWD=/ a relativni cesta k DSK nemusi
 * byt platna.
 * 
 * 
 * Vystup:
 * 
 * Pokud se ocekava, ze vystupem prikazu budou nejake data, 
 * tak jsou k dispozici na dataportu.
 * 
 * Pokud jsou vystupni hodnotou retezce, tak jsou vzdy oddeleny a ukonceny znakem 0x0d.
 * 
 * Pokud unikarta nema zadne data, ktere by mohla poslat, tak na datovem portu vraci 0x00.
 * 
 * Pokud je vystupnim parametrem retezec, tak tento vystup prochazi uvnitr 
 * unikarty I/O translatorem ASCII.
 * 
 * 
 * Pokud je pres OPEN otevren nejaky soubor, tak zapis a cteni dataportu je povazovano 
 * za pokus o getc(), nebo putc().
 * 
 * Pokud je pres OPEN otevren nejaky soubor a vlozime prikaz, ktery ocekava parametry, 
 * nebo vraci nejaka data, tak tento prikaz ziska prioritu v pristupu na dataport. 
 * To plati do doby, dokud neni prikaz kompletne vykonan, nebo stornovan prikazem STORNO.
 *
 *
 * Ciselne hodnoty WORD (2B)  a DWORD (4) jsou predavany od nejnizsiho bajtu k nejvyssimu.
 *
 * Struktura FILINFO:
 *
 *	DWORD	-	4B, File size
 *	WORD	-	2B, Last modified date
 *	WORD	-	2B, Last modified time
 *	BYTE	-	1B, Attribute
 *	TCHAR	-	13B, Short file name (8.3 format), zakonceno 0x00
 *	BYTE	-	1B, LFN strlen
 *	TCHAR	-	32B, LFN, zakonceno 0x00
 *
 *
 *
 * Status kody:
 * ============
 *
 * Status repozitare obsahuje 4 bajty a lze jej kdykoliv ziskat prectenim CMD portu.
 *
 * Prectenim jednoho bajtu ze statusu se interne zvedne pozice ukazatele na dalsi bajt
 * a jakmile jsou precteny vsechny ctyri bajty, tak zustane ukazatel "zaparkovan" a pri dalsim
 * cteni vraci unikarta misto statusu 0x00.
 * 
 * Po kazde I/O operaci DATA portu, nebo pri zapisu prikazu se vzdy v unikarte aktualizuje
 * stav, ktery je sdelovan statusem a zaroven se take vzdy resetuje ukazatel statusu 
 * na prvni bajt.
 *
 * Pozici ukazatele lze kdykoliv resetovat prikazem STSR, ktery zaroven jako jediny prikaz
 * zadnym zpusobem neovlivnije obsah statusu a ani neprerusi vykonavani jiz aktivniho prikazu.
 * Je tedy bezpecne jej kdykoliv pouzit.
 *
 *
 * Vyznam jednotlivych bajtu:
 *
 * 1. bajt - master status byte:
 *
 *	0. bit	- BUSY/READY		- pokud je bit vynulovan, tak to znamena, ze je
 *					repozitar ve stavu READY
 *
 *					- pokud je nastaven, tak to znamena, ze je repozitar
 *					ve stavu BUSY - tedy je prave aktivni nejaky prikaz, 
 *					ktery vyzaduje vlozeni dalsich parametru
 *					pres DATA port
 *  
 *					- s vlozenim posledniho parametru, nebo pretecenim
 *					vstupniho zasobniku se tento bit opet vynuluje a repozitar
 *					prejde do stavu READY (Pokud doslo k preteceni zasobniku,
 *					tak se zaroven status ERROR.)
 *
 *					- s vlozenim prikazu STORNO prejde repozitar do stavu READY
 *
 *                                      
 *
 *	1. bit 	- CMD_OUTPUT		- jestlize je nastaven, tak to znamena, ze mame v zasobniku
 *					nejaka data, ktera jsou vystupem z posledniho prikazu
 *
 *					- s prectenim posledniho bajtu, nebo vlozenim prikazu STORNO
 *					se tento bit vynuluje
 *
 *
 *
 *	2. bit	- READDIR		- pokud je nastaven, tak to znamena, ze mame otevren
 *					adresar prikazem READDIR, nebo FILELIST a pokud nejsou
 *					v zasobniku zadna data k vyzvednuti (CMD_OUTPUT), tak 
 *					v tuto chvili ctenim z DATA portu ziskame odpovidajici 
 *					data z otevreneho adresare
 *
 *					- s prectenim posledniho bajtu z posledni polozky adresare
 *					se tento status bit vynuluje
 *
 *					- pokud je vlozen prikaz NEXT a jiz neni k dispozici
 *					zadna dalsi polozka adresare, tak se tento bit vynuluje
 *
 *
 *
 *	3. bit	- READ_FILE		- pokud je nastaven, tak to znamena, ze mame otevren 
 *					nejaky soubor pro cteni a pokud nejsou v zasobniku
 * 					zasobniku zadna data k vyzvednuti (CMD_OUTPUT), 
 *					tak v tuto chvili ctenim z DATA portu provedeme 
 *					fget() - precteme bajt z otevreneho souboru
 *
 *
 *
 *	4. bit	- WRITE_FILE		- pokud je nastaven, tak to znamena, ze mame otevren 
 *                                      nejaky soubor pro zapis a poku je repozitar READY,
 *					tak zapisem na DATA port provedeme fput() - zapiseme
 *					bajt do otevreneho souboru
 *
 *
 *
 *	5. bit 	- EOF			- pokud mame otevreny nejaky soubor a pozice ukazatele 
 *					se momentalne nachazi na jeho konci, tak se nam nastavi 
 *					tento bit
 *
 *
 *	6. bit	- nepouzito
 *
 *
 *	7. bit	- ERROR			- nastaven, pokud doslo k chybe, coz muze nastat:
 *
 *					- vlozeni prikazu (pripadne vlozenim jeho posledniho 
 *                                      parametru, cimz je prikaz spusten)
 *                                        
 *                                      - pretecenim vstupniho bufferu pri vkladani parametru
 *                                      typu string
 *                                      
 *					- zapisem/ctenim otevreneho souboru
 *                  
 *                                      - prectenim posledniho bajtu polozky adresare (to interne
 *					aktivuje prikaz NEXT pro nacteni dalsi polozky z FAT 
 *					do vystupniho bufferu)
 *
 *                                      - pri neocekavane I/O operaci na DATA portu
 *
 *
 * 	Pozn.: Status bity READDIR a READ_FILE/WRITE_FILE se v tuto chvili navzajem vylucuji.
 *
 *
 * 2. bajt - hodnota posledniho aktivniho prikazu
 *
 *	Pozn.:	V pripade cteni/zapisu do souboru pres DATA port se zde jako hodnota projevi
 *		interni prikazy INTGETC, nebo INTPUTC.
 *
 *
 * 3. bajt - v pripade, ze je v prvnim master status bajtu nastaven ERROR, tak zde muze 
 *	     byt error kod z unikarty, napr. pri preteceni bufferu, spatne hodnote parametru, atd... 
 *           (v tuto chvili jeste neni implementovano a vraci 0x00)
 *
 *	   - v pripade, ze vkladame parametry k prikazu (BUSY) je zde informace o zbyvajici
 *	     velikosti volne pameti ve vstupnim bufferu
 *
 *	   - v pripade, ze cteme vystupni data z prikazu, tak je zde zbyvajici pocet
 *	     bajtu ve vystupnim bufferu
 *
 *	   - v pripade, ze cteme polozku adresare otevreneho pres READDIR, nebo FILELIST, 
 *	     tak je zde zbyvajici pocet bajtu aktualni polozky, ktera je ulozena v bufferu 
 *
 * 4. bajt - FatFS result
 *
 *	Zde je ulozen navratovy kod z posledni zavolane FatFS operace. Ma smysl se
 *	o nej zajimat jen v pripade, ze byl v prvnim master status bajtu nastaven ERROR.
 *	viz. FatFS - http://elm-chan.org/fsw/ff/en/rc.html 
 */
// </editor-fold>

/*
 * 
 * Dokumentace k prikazum je ulozena v samostatnem souboru
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sharpmz_ascii.h"
#include "unimgr_commands.h"
#include "unicard.h"
#include "unimgr.h"

#ifdef UNICARD_EMULATED
const char FIRMWARE_REVISION[] = "mz800emu/rev.01"; //"trunk/rev.60";
const uint32_t FIRMWARE_REVISION_DWORD = 60;
#else


include "revision.h"
#endif


typedef enum en_UNIMGR_CMDSTATE {
    // Faze vykonavani prikazu
    UNIMGR_CMDSTATE_DONE = 0x00, // hotovo
    UNIMGR_CMDSTATE_PARAMRQ = 0x01, // cekame na vstupni data
    UNIMGR_CMDSTATE_DOUTRQ = 0x02, // cekame na vyzvednuti vystupnich dat
    // interni stavy
    UNIMGR_CMDSTATE_START = 0x03, // prikaz byl prave vlozen
    UNIMGR_CMDSTATE_PARAM = 0x04, // prijali jsme 1B parametru
    UNIMGR_CMDSTATE_PARAMOK = 0x05, // vsechny parametry byly vlozeny, GO!
    UNIMGR_CMDSTATE_WORK = 0x06 // pracovni cyklus (nacteni dalsi polozky adresare :-)
} en_UNIMGR_CMDSTATE;


typedef enum en_UNIMGR_ASCII_CNV {
    UNIMGR_ASCII_CNV_OFF = 0,
    UNIMGR_ASCII_CNV_ON = 1
} en_UNIMGR_ASCII_CNV;


typedef enum en_UNIMGR_STS {
    UNIMGR_STS_OK = 0,
    UNIMGR_STS_ERROR = 1
} en_UNIMGR_STS;


typedef enum en_READDIR_MODE {
    READDIR_MODE_NORMAL = 0,
    READDIR_MODE_FILELIST,
} en_READDIR_MODE;

#define PARAM_BUFFER_SIZE 255
//#define FILE_BUFFER_SIZE	255
#define FILE_BUFFER_SIZE 55 // FILINFO 23+_MAX_LFN=55, FILELIST 32+20=52

/* Doplneni access mode k prikaziu cmdOPEN */
#define FA_UNIMGR_ASCII_CNV 0x20


typedef struct st_UNIMGR {
    uint8_t cmd; // posledni aktivni prikaz
    en_UNIMGR_CMDSTATE cmd_phase; // faze vykonavani prikazu
    en_UNIMGR_ASCII_CNV cnv_cmd; // budeme provadet SharpASCII konverzi?

    FRESULT ff_res; // navratovy kod z posledni FatFS operace

    uint8_t sts_pos; // pozice ukazatele cteni statusu
    en_UNIMGR_STS sts_err; // pokud je true, tak budeme ve statusu reportovat ERROR

    uint8_t *buf; // ukazatel I/O bufferu pro parametry a vystup z prikazu
    uint8_t buf_count; // pocet zbyvajicich bajtu
    uint8_t buf_byte[PARAM_BUFFER_SIZE];
    const char *param_format; // odkaz na retezec s popisem vstupnich parametru 'B' - byte, 'S' - string (ukoncen znakem mensim jak 0x20)

    uint8_t *filebuf; // ukazatel I/O bufferu pro OPEN, READDIR a FILELIST
    uint8_t filebuf_count; // pocet zbyvajicich bajtu
    uint8_t filebuf_byte[FILE_BUFFER_SIZE];

    st_UNICARD_FILE file; // filehandle pro praci se souborem (OPEN)
    uint8_t file_mode; // tady si ukladame rezim v jakem je soubor otevren

    st_UNICARD_DIR dir; // dirhandle pro READDIR a FILELIST
    en_READDIR_MODE readdir_mode;
} st_UNIMGR;

st_UNIMGR g_unimgr;


/* InputParams
 *
 *
 * V g_param_format ocekava strukturu parametru:
 *	B - bajt
 *	S - string (ukoncen znakem mensim jak 0x20)
 *
 * Vraci pocet parametru, ktere zbyva nacist, nebo -1 pri preteceni bufferu. 
 */
int8_t unimgr_input_params ( en_UNIMGR_CMDSTATE phase, uint8_t data ) {

    g_unimgr.cmd_phase = UNIMGR_CMDSTATE_PARAMRQ;

    if ( phase == UNIMGR_CMDSTATE_START ) {
        g_unimgr.sts_err = UNIMGR_STS_OK;
        g_unimgr.buf = &g_unimgr.buf_byte[0];
        g_unimgr.buf_count = PARAM_BUFFER_SIZE;
        uint8_t i;
        for ( i = 0; i < strlen ( g_unimgr.param_format ); i++ ) {
            if ( 'S' == g_unimgr.param_format[i] ) {
                g_unimgr.buf_count--; // pro kazdy vstupni retezec musime udelat misto pro terminator (0x00)
            };
        };
    } else {
        if ( 'B' == g_unimgr.param_format[0] ) {
            //DBGPRINTF ( DBGINF, "InputParams ( BYTE ) - 0x%02lx\n", inputByte );
            g_unimgr.buf[0] = data;
            g_unimgr.buf++;
            g_unimgr.buf_count--;
            g_unimgr.param_format++;
        } else {
            if ( g_unimgr.cnv_cmd ) data = sharpmz_cnv_to ( data );

            if ( data >= 0x20 ) {
                g_unimgr.buf[0] = data;
                g_unimgr.buf_count--;
            } else {
                g_unimgr.buf[0] = '\x00';
                g_unimgr.param_format++;
            };
            g_unimgr.buf++;
        };
    };

    if ( !g_unimgr.buf_count ) {
        // TODO: unicard err code: BUFFER_SIZE_EXCEEDED
        return ( -1 );
    };
    return ( strlen ( g_unimgr.param_format ) );
}


static void do_cmdCLOSE ( void ) {
    g_unimgr.cmd_phase = UNIMGR_CMDSTATE_DONE;
    unicard_file_close ( &g_unimgr.file );
    unicard_dir_close ( &g_unimgr.dir );
    g_unimgr.sts_err = UNIMGR_STS_OK;
    g_unimgr.cmd_phase = UNIMGR_CMDSTATE_DONE;
}


static void do_cmdRESET ( void ) {
    g_unimgr.cmd_phase = UNIMGR_CMDSTATE_DONE;
    g_unimgr.cnv_cmd = UNIMGR_ASCII_CNV_OFF;
    unicard_file_close ( &g_unimgr.file );
    unicard_dir_close ( &g_unimgr.dir );
    unicard_chdir ( "/" );
    g_unimgr.ff_res = FR_OK;
    g_unimgr.sts_err = UNIMGR_STS_OK;
}


/* do_cmdCHDIR()
 *
 * Provede zmenu CWD.
 */
static void do_cmdCHDIR ( en_UNIMGR_CMDSTATE cmd_phase ) {

    if ( UNIMGR_CMDSTATE_START == cmd_phase ) {
        g_unimgr.param_format = "S";
        unimgr_input_params ( cmd_phase, 0 );
    } else {
        // zpracovani prikazu
        g_unimgr.cmd_phase = UNIMGR_CMDSTATE_DONE;
        g_unimgr.ff_res = unicard_chdir ( (char*) g_unimgr.buf_byte );
        if ( FR_OK != g_unimgr.ff_res ) return;
        g_unimgr.sts_err = UNIMGR_STS_OK;
    };
}


static void do_cmdREADDIR ( en_UNIMGR_CMDSTATE cmd_phase ) {

    if ( UNIMGR_CMDSTATE_START == cmd_phase ) {
        do_cmdCLOSE ( ); // Zavreme soubor, pokud byl otevreny
        g_unimgr.param_format = "S";
        unimgr_input_params ( cmd_phase, 0 );
        return;
    } else if ( UNIMGR_CMDSTATE_PARAMOK == cmd_phase ) {
        g_unimgr.cmd_phase = UNIMGR_CMDSTATE_DONE;
        g_unimgr.ff_res = unicard_dir_open ( &g_unimgr.dir, (char*) g_unimgr.buf_byte );
        if ( FR_OK != g_unimgr.ff_res ) return;
    };

    // nacteni polozky adresare

    if ( READDIR_MODE_FILELIST == g_unimgr.readdir_mode ) {
        g_unimgr.cmd = cmdFILELIST;
        g_unimgr.ff_res = unicard_dir_read_filelist ( &g_unimgr.dir, (char*) g_unimgr.filebuf_byte, sizeof ( g_unimgr.filebuf_byte ) );
        g_unimgr.filebuf_count = strlen ( (char*) g_unimgr.filebuf_byte );
        g_unimgr.filebuf = g_unimgr.filebuf_byte;
    } else {
        g_unimgr.cmd = cmdREADDIR;
        fprintf ( stderr, "%s():%d - Not implemented UNIMGR command 0x%02x!\n", __func__, __LINE__, g_unimgr.cmd );
        g_unimgr.sts_err = UNIMGR_STS_ERROR;
        g_unimgr.cmd_phase = UNIMGR_CMDSTATE_DONE;
    };

    if ( g_unimgr.ff_res == FR_OK ) g_unimgr.sts_err = UNIMGR_STS_OK;
    g_unimgr.cmd_phase = UNIMGR_CMDSTATE_DONE;
}


void do_cmdOPEN ( en_UNIMGR_CMDSTATE cmd_phase ) {

    if ( UNIMGR_CMDSTATE_START == cmd_phase ) {
        unicard_file_close ( &g_unimgr.file );
        unicard_dir_close ( &g_unimgr.dir );
        g_unimgr.param_format = "BS";
        unimgr_input_params ( cmd_phase, 0 );
        return;
    };

    // zpracovani prikazu
    g_unimgr.cmd_phase = UNIMGR_CMDSTATE_DONE;
    g_unimgr.buf = g_unimgr.buf_byte + 1;
    g_unimgr.file_mode = g_unimgr.buf_byte[0];
    g_unimgr.ff_res = unicard_file_open ( &g_unimgr.file, (char*) g_unimgr.buf, ( g_unimgr.file_mode & 0xdf ) );
    if ( FR_OK != g_unimgr.ff_res ) return;
    g_unimgr.sts_err = UNIMGR_STS_OK;
}


/* do_cmdFDDMOUNT()
 *
 * Provede remount FDD a ulozi konfiguraci.
 */
static void do_cmdFDDMOUNT ( en_UNIMGR_CMDSTATE cmd_phase ) {

    if ( UNIMGR_CMDSTATE_START == cmd_phase ) {
        g_unimgr.param_format = "BS";
        unimgr_input_params ( cmd_phase, 0 );
    } else {
        // zpracovani prikazu
        if ( 3 < g_unimgr.buf_byte[0] ) {
            // DBGPRINTF ( DBGERR, "do_cmdFDDMOUNT() - invalid FDD id '0x%02lx'!\n", MZFrepo.buf_byte[0] );
            // TODO: unicard err code: INVALID_PARAMETER
            return;
        };
#ifdef UNICARD_EMULATED
        unicard_fdc_mount ( g_unimgr.buf_byte[0], (char*) &g_unimgr.buf_byte[1] );
#else
#error "Not implemented command cmdFDDMOUNT!"
#endif
        g_unimgr.cmd_phase = UNIMGR_CMDSTATE_DONE;
        g_unimgr.sts_err = UNIMGR_STS_OK;
    };
}

#ifdef UNICARD_EMULATED


void unimgr_exit ( void ) {
    unicard_file_close ( &g_unimgr.file );
    unicard_dir_close ( &g_unimgr.dir );
}


void unimgr_reset ( void ) {
    do_cmdRESET ( );
}

#endif


int unimgr_init ( void ) {
    unicard_file_init ( &g_unimgr.file );
    unicard_dir_init ( &g_unimgr.dir );
    // cmdRESET
    g_unimgr.cmd = cmdRESET;
    g_unimgr.sts_pos = 0x00;
    g_unimgr.cmd_phase = UNIMGR_CMDSTATE_DONE;
    g_unimgr.cnv_cmd = UNIMGR_ASCII_CNV_OFF;
    g_unimgr.sts_err = UNIMGR_STS_OK;
#ifdef UNICARD_EMULATED
    if ( !TEST_UNICARD_CONNECTED ) return EXIT_SUCCESS;
#endif
    g_unimgr.ff_res = unicard_chdir ( "/" );
    if ( g_unimgr.ff_res != FR_OK ) {
        g_unimgr.sts_err = UNIMGR_STS_ERROR;
        return EXIT_FAILURE;
    };
    return EXIT_SUCCESS;
}


static void unimgr_write_CMD ( uint8_t data ) {

    //printf ( "cmd: 0x%02x\n", data );

    if ( cmdSTSR == data ) {
        //DBGPRINTF ( DBGINF, "do_cmdSTSR: reset sts pointer\n" );
        return;
    };

    g_unimgr.cmd = data;
    g_unimgr.sts_err = UNIMGR_STS_ERROR;

    st_UNICARD_RTC rtc;

    switch ( g_unimgr.cmd ) {

        case cmdRESET:
            do_cmdRESET ( );
            break;

        case cmdASCII:
            g_unimgr.cnv_cmd = UNIMGR_ASCII_CNV_OFF;
            g_unimgr.cmd_phase = UNIMGR_CMDSTATE_DONE;
            g_unimgr.sts_err = UNIMGR_STS_OK;
            break;

        case cmdSHASCII:
            //DBGPRINTF ( DBGINF, "do_cmdSHASCII: cnv/ON\n" );
            g_unimgr.cnv_cmd = UNIMGR_ASCII_CNV_ON;
            g_unimgr.cmd_phase = UNIMGR_CMDSTATE_DONE;
            g_unimgr.sts_err = UNIMGR_STS_OK;
            break;

        case cmdSTORNO:
            //DBGPRINTF ( DBGINF, "do_cmdSTORNO\n" );
            g_unimgr.cmd_phase = UNIMGR_CMDSTATE_DONE;
            g_unimgr.sts_err = UNIMGR_STS_OK;
            break;

        case cmdREV:
            g_unimgr.sts_err = UNIMGR_STS_OK;
            g_unimgr.buf = (uint8_t*) FIRMWARE_REVISION;
            //DBGPRINTF ( DBGINF, "do_cmdREV ( START ) - rev: '%s'\n", g_unimgr.buf );
            g_unimgr.buf_count = strlen ( FIRMWARE_REVISION ) + 1; // + 0x0d
            g_unimgr.cmd_phase = UNIMGR_CMDSTATE_DOUTRQ;
            break;

        case cmdREVD:
            g_unimgr.sts_err = UNIMGR_STS_OK;
            g_unimgr.buf = ( uint8_t* ) & FIRMWARE_REVISION_DWORD;
            //DBGPRINTF ( DBGINF, "do_cmdREVD ( START ) - rev: 0x%04lx\n", (uint32_t*) g_unimgr.buf );
            g_unimgr.buf_count = 4;
            g_unimgr.cmd_phase = UNIMGR_CMDSTATE_DOUTRQ;
            break;

        case cmdFDDMOUNT:
            do_cmdFDDMOUNT ( UNIMGR_CMDSTATE_START );
            break;

        case cmdCHDIR:
            do_cmdCHDIR ( UNIMGR_CMDSTATE_START );
            break;

        case cmdGETCWD:
            if ( EXIT_SUCCESS == unicard_get_cwd ( &g_unimgr.ff_res, (char*) g_unimgr.buf_byte, sizeof ( g_unimgr.buf_byte ) ) ) {
                g_unimgr.buf = g_unimgr.buf_byte;
                g_unimgr.buf_count = strlen ( (char*) g_unimgr.buf_byte ) + 1;
                //g_unimgr.buf[g_unimgr.buf_count] = 0x0d;
                g_unimgr.sts_err = UNIMGR_STS_OK;
                g_unimgr.cmd_phase = UNIMGR_CMDSTATE_DOUTRQ;
            } else {
                g_unimgr.cmd_phase = UNIMGR_CMDSTATE_DONE;
            };
            break;

        case cmdFILELIST:
            g_unimgr.readdir_mode = READDIR_MODE_FILELIST;
            do_cmdREADDIR ( UNIMGR_CMDSTATE_START );
            break;

        case cmdOPEN:
            do_cmdOPEN ( UNIMGR_CMDSTATE_START );
            break;

        case cmdCLOSE:
            do_cmdCLOSE ( );
            break;

        case cmdRTCGETD:
        case cmdRTCGETT:

            unicard_read_rtc ( &rtc );
            if ( g_unimgr.cmd == cmdRTCGETD ) {
                g_unimgr.buf_byte[0] = rtc.day;
                g_unimgr.buf_byte[1] = rtc.month;
                g_unimgr.buf_byte[2] = rtc.year - 1980;
            } else {
                g_unimgr.buf_byte[0] = rtc.hour;
                g_unimgr.buf_byte[1] = rtc.min;
                g_unimgr.buf_byte[2] = rtc.sec;
            };
            g_unimgr.buf = g_unimgr.buf_byte;
            g_unimgr.buf_count = 3;
            g_unimgr.cmd_phase = UNIMGR_CMDSTATE_DOUTRQ;
            g_unimgr.sts_err = UNIMGR_STS_OK;
            break;

        default:
            fprintf ( stderr, "%s():%d - Not implemented UNIMGR command 0x%02x!\n", __func__, __LINE__, g_unimgr.cmd );
    };
}


static void unimgr_write_param ( uint8_t data ) {

    int8_t params_left = unimgr_input_params ( UNIMGR_CMDSTATE_PARAM, data );

    if ( -1 == params_left ) {
        // preteceni bufferu
        g_unimgr.cmd_phase = UNIMGR_CMDSTATE_DONE;
        return;
    }

    if ( 0 != params_left ) {
        // pokracujeme v prijimani dalsich parametru
        g_unimgr.sts_err = UNIMGR_STS_OK;
        return;
    };

    switch ( g_unimgr.cmd ) {

        case cmdCHDIR:
            do_cmdCHDIR ( UNIMGR_CMDSTATE_PARAMOK );
            break;

        case cmdFILELIST:
            do_cmdREADDIR ( UNIMGR_CMDSTATE_PARAMOK );
            break;

        case cmdOPEN:
            do_cmdOPEN ( UNIMGR_CMDSTATE_PARAMOK );
            break;

        case cmdFDDMOUNT:
            do_cmdFDDMOUNT ( UNIMGR_CMDSTATE_PARAMOK );
            break;

        default:
            fprintf ( stderr, "%s():%d - Not implemented UNIMGR command 0x%02x!\n", __func__, __LINE__, g_unimgr.cmd );
    };
}


static void unimgr_write_DATA ( uint8_t data ) {
    g_unimgr.sts_err = UNIMGR_STS_ERROR;
    if ( UNIMGR_CMDSTATE_PARAMRQ == g_unimgr.cmd_phase ) {
        unimgr_write_param ( data );
    } else if ( EXIT_SUCCESS == unicard_file_is_open ( &g_unimgr.file ) ) {
        fprintf ( stderr, "%s():%d - Write to file is not implemented (0x%02x)\n", __func__, __LINE__, data );
    };
}


void unimgr_write_byte ( en_UNIMGR_ADDR addr, uint8_t data ) {

    /* po kazdem zapisu na kterykoliv unimgr port provedeme reset sts ukazatele */
    g_unimgr.sts_pos = 0;

    if ( addr == UNIMGR_ADDR_DATA ) {
        unimgr_write_DATA ( data );
    } else {
        unimgr_write_CMD ( data );
    };
}


/* OutputData
 *
 * Vystavi data z *MZFrepo.buf pro vystup. Pokud txt_data = 1, 
 * tak provede konverzi ASCII pokud je pozadovana a za daty prida 0x0d.
 */
static uint8_t unimgr_output_data ( int txt_data ) {
    uint8_t ret;

    g_unimgr.sts_err = UNIMGR_STS_OK;
    ret = g_unimgr.buf [0];
    if ( txt_data && ( ret == '\x00' ) ) {
        ret = '\x0d';
    };

    if ( ( g_unimgr.cnv_cmd == UNIMGR_ASCII_CNV_ON ) && ( txt_data == 1 ) ) {
        uint8_t cnv_val = sharpmz_cnv_to ( ret );
        ret = cnv_val;
    };
    g_unimgr.buf++;
    g_unimgr.buf_count--;
    if ( g_unimgr.buf_count ) {
        g_unimgr.cmd_phase = UNIMGR_CMDSTATE_DOUTRQ;
    } else {
        g_unimgr.cmd_phase = UNIMGR_CMDSTATE_DONE;
    };

    return ret;
}


static inline uint8_t unimgr_output_data_txt ( void ) {
    return unimgr_output_data ( 1 );
}


static inline uint8_t unimgr_output_data_bin ( void ) {
    return unimgr_output_data ( 0 );
}


static uint8_t unimgr_read_DATA ( void ) {

    g_unimgr.sts_pos = 0x00;
    g_unimgr.sts_err = UNIMGR_STS_ERROR;

    uint8_t ret = 0x00;

    if ( UNIMGR_CMDSTATE_DOUTRQ == g_unimgr.cmd_phase ) {

        // cteni vystupu z prikazu            
        switch ( g_unimgr.cmd ) {

            case cmdREV:
            case cmdGETCWD:
                ret = unimgr_output_data_txt ( );
                break;

            case cmdREVD:
            case cmdRTCGETD:
            case cmdRTCGETT:
                ret = unimgr_output_data_bin ( );
                break;

            default:
                fprintf ( stderr, "%s():%d - Not implemented command 0x%02x\n", __func__, __LINE__, g_unimgr.cmd );
        };

    } else if ( EXIT_SUCCESS == unicard_dir_is_open ( &g_unimgr.dir ) ) {

        // cteni polozek adresare
        uint8_t ret_val = g_unimgr.filebuf [0];
        if ( ( g_unimgr.cnv_cmd == UNIMGR_ASCII_CNV_ON ) && ( g_unimgr.readdir_mode == READDIR_MODE_FILELIST ) ) {
            uint8_t cnv_val = sharpmz_cnv_to ( ret_val );
            ret_val = cnv_val;
        };
        ret = ret_val;
        g_unimgr.filebuf++;
        g_unimgr.filebuf_count--;
        if ( g_unimgr.filebuf_count ) {
            g_unimgr.sts_err = UNIMGR_STS_OK;
        } else {
            // nacist dalsi polozku adresare
            do_cmdREADDIR ( UNIMGR_CMDSTATE_WORK );
        };

    } else if ( EXIT_SUCCESS == unicard_file_is_open ( &g_unimgr.file ) ) {

        // cmdINTGETC: cteni z otevreneho souboru (TODO: dopsat cache)
        g_unimgr.cmd = cmdINTGETC;
        unsigned int ff_readlen;
        uint8_t read_value;
        g_unimgr.ff_res = unicard_file_read ( &g_unimgr.file, &read_value, 1, &ff_readlen );
        if ( FR_OK != g_unimgr.ff_res ) return 0x00;
        g_unimgr.sts_err = UNIMGR_STS_OK;
        if ( 1 == ff_readlen ) {
            if ( g_unimgr.file_mode & FA_UNIMGR_ASCII_CNV ) {
                uint8_t cnv_val = sharpmz_cnv_to ( read_value );
                read_value = cnv_val;
            };
            ret = read_value;
            //} else {
            // EOF - neni co cist
            //*io_data = 0x00;
        };
    };

    return ret;
}


static uint8_t unimgr_read_STATUS ( void ) {

    uint8_t ret = 0x00;

    switch ( g_unimgr.sts_pos ) {

        case 0:
            if ( UNIMGR_CMDSTATE_PARAMRQ == g_unimgr.cmd_phase ) ret |= 0x01; // 0. bit - BUSY(paramRQ)=1

            if ( UNIMGR_CMDSTATE_DOUTRQ == g_unimgr.cmd_phase ) ret |= 0x02; // 1. bit - CMD_OUTPUT=1

            if ( EXIT_SUCCESS == unicard_dir_is_open ( &g_unimgr.dir ) ) ret |= 0x04; // 2. bit - READDIR_DATA=1

            if ( EXIT_SUCCESS == unicard_file_is_open ( &g_unimgr.file ) ) {
                if ( g_unimgr.file_mode & FA_READ ) ret |= 0x08; // 3. bit - FA_READ=1

                if ( g_unimgr.file_mode & FA_WRITE ) ret |= 0x10; // 4. bit - FA_WRITE=1
            };

            if ( EXIT_SUCCESS == unicard_file_is_open ( &g_unimgr.file ) ) {
                if ( EXIT_SUCCESS == unicard_file_is_eof ( &g_unimgr.file ) ) ret |= 0x20; // 5. bit - EOF=1
            };

            // 6. bit - nepouzito

            if ( UNIMGR_STS_ERROR == g_unimgr.sts_err ) ret |= 0x80; // 7. bit - ERROR=1

            break;

        case 1:
            ret = g_unimgr.cmd; // kod posledniho vlozeneho prikazu
            break;


        case 2: /* err code, ani data k predani ci vyzvednuti ? */
            if ( UNIMGR_STS_ERROR == g_unimgr.sts_err ) {
                ret = 0x00; // err code unikarty - dopsat! ;)
            } else if ( ( UNIMGR_CMDSTATE_DOUTRQ == g_unimgr.cmd_phase ) || ( UNIMGR_CMDSTATE_PARAMRQ == g_unimgr.cmd_phase ) ) {
                ret = g_unimgr.buf_count; // tady je obcas zbyvajici pocet bajtu z prikazu k vyzvednuti
            } else if ( ( EXIT_SUCCESS == unicard_dir_is_open ( &g_unimgr.dir ) ) || ( EXIT_SUCCESS == unicard_file_is_open ( &g_unimgr.file ) ) ) {
                ret = g_unimgr.filebuf_count; // zbyvajici miste ve filebufferu
            };
            break;

        case 3:
            ret = g_unimgr.ff_res; // navratovy kod posledni FatFS operace
            break;
    };

    if ( g_unimgr.sts_pos != PARAM_BUFFER_SIZE ) {
        //printf ( "STS(%d): 0x%02x\n", g_unimgr.sts_pos, ret );
        g_unimgr.sts_pos++;
        if ( g_unimgr.sts_pos == 4 ) g_unimgr.sts_pos = PARAM_BUFFER_SIZE;
    };

    return ret;
}


uint8_t unimgr_read_byte ( en_UNIMGR_ADDR addr ) {
    if ( addr == UNIMGR_ADDR_DATA ) return unimgr_read_DATA ( );
    return unimgr_read_STATUS ( );
}
