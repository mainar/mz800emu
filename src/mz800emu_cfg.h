/* 
 * File:   mz800emu_cfg.h
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 16. srpna 2016, 9:50
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

#ifndef MZ800EMU_CFG_H
#define MZ800EMU_CFG_H

#ifdef __cplusplus
extern "C" {
#endif



    /*
     * Konfiguracni vypnuti modulu MZ-800 debugger
     * ===========================================
     * 
     * Emulace se zjednodusi a mela by byt rychlejsi (nemeril jsem, zda je tomu skutecne tak).
     * 
     */

#define MZ800EMU_CFG_DEBUGGER_ENABLED



    /*
     * Konfigurace pro emulaci CLK 1.1 MHz (CTC8253-CTC0 a CMT )
     * =========================================================
     * 
     * !!! Pouzit jednu z voleb:
     * 
     * MZ800EMU_CFG_CLK1M1_SLOW - presna varianta pri ktere se vola CTC0_step a CMT_step 
     * pri kazdem 16 pixelclk
     * 
     * MZ800EMU_CFG_CLK1M1_FAST - prozatim nedokoncena varianta pri ktere se CLK 1M1 
     * step() zavola jen tehdy, pokud ma nastat konkretni event
     * 
     */
#if 1
#define MZ800EMU_CFG_CLK1M1_FAST  
#else
#define MZ800EMU_CFG_CLK1M1_SLOW
#endif

    /*
     * Variabilni speed sync - umoznuje menit plynule synchronizovanou rychlost 
     * 
     */
    //#define MZ800EMU_CFG_VARIABLE_SPEED


    /*
     * 
     * Audio buffer v1 - je generovan on the fly
     * 
     * Audio buffer v2 - zabira mnohem vice mista v pameti a dynamicky reaguje na zmeny rychlosti
     * 
     */
#if 0
#define AUDIO_FILLBUFF_v2
#else
#define AUDIO_FILLBUFF_v1
#endif


    /*
     * Maximalni povolena variabilni rychlost emulace v procentech, pri ktere jeste probiha poctiva synchronizace.
     * 
     * 100 % = 50 snimku za sekundu
     * 250 % = 150 snimku za sekundu
     * ...
     * 
     */

#ifdef AUDIO_FILLBUFF_v2
#ifndef MZ800EMU_CFG_VARIABLE_SPEED
#define MZ800EMU_CFG_MAX_SYNC_SPEED 100
#else
#define MZ800EMU_CFG_MAX_SYNC_SPEED 1000
#endif
#endif

    /*
     * Experimentalni vypnuti audio vystupu a synchronizace snimku se zvukem
     * =====================================================================
     * 
     * Slouzi pouze k lepsimu ladeni maximalniho vykonu.
     * 
     */

    //#define MZ800EMU_CFG_AUDIO_DISABLED


    /*
     * Experimentalni zavedeni MZF do pameti a vykonani jen predvoleny pocet snimku
     * 
     * Slouzi pouze k lepsimu ladeni maximalniho vykonu.
     * 
     */
    //#define MZ800EMU_CFG_SPEED_TEST


    /*
     * Memory statistics
     * =================
     * 
     * Chci zkusit zjistit, jak nejlepe optimalizovat umisteni podminek pro cteni a zapis do pameti.
     * Tato volba vytvori aditivni soubor se statistikou pro cteni a zapis pameti.
     * 
     */
//#define MEMORY_MAKE_STATISTICS

#ifdef __cplusplus
}
#endif

#endif /* MZ800EMU_CFG_H */

