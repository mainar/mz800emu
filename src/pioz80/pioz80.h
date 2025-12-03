/* 
 * File:   pioz80.h
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 3. července 2015, 21:28
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

#ifndef PIOZ80_H
#define PIOZ80_H

#ifdef __cplusplus
extern "C" {
#endif

#include "z80ex/include/z80ex.h"
#include "mz800.h"


    typedef enum en_PIOZ80_PORT_ID {
        PIOZ80_PORT_NONE = -1,
        PIOZ80_PORT_A = 0,
        PIOZ80_PORT_B = 1,
        PIOZ80_PORT_COUNT = 2
    } en_PIOZ80_PORT_ID;


    typedef enum en_PIOZ80_ADDRTYPE {
        PIOZ80_ADDRTYPE_CTRL = ( 0 << 1 ),
        PIOZ80_ADDRTYPE_DATA = ( 1 << 1 )
    } en_PIOZ80_ADDRTYPE;


    typedef enum en_PIOZ80_ADDR {
        PIOZ80_ADDR_CTRL_A = 0x00,
        PIOZ80_ADDR_CTRL_B = 0x01,
        PIOZ80_ADDR_DATA_A = 0x02,
        PIOZ80_ADDR_DATA_B = 0x03
    } en_PIOZ80_ADDR;

#define PIOZ80_PORT_ID_MASK     ( 1 << 0 )  // PORT_A, PORT_B
#define PIOZ80_ADDR_TYPE_MASK   ( 1 << 1 )  // CTRL / DATA


#define PIOZ80_CTRL_MASK     0x0f    // Dolni 4 bity urcuji vyznam kontrolniho slova
#define PIOZ80_CTRL_IVW_MASK 0x01    // podle 0. bitu identifikujeme IVW


#define PIOZ80_CTRL_IVW      0x00    // Interrupt Vector Word (jen 0. bit!)
#define PIOZ80_CTRL_MCW      0x0f    // Mode Control Word
#define PIOZ80_CTRL_ICW      0x07    // Interrupt Control Word
#define PIOZ80_CTRL_IDW      0x03    // Interrupt Disable Word


    typedef enum en_PIOZ80_PORT_MODE {
        PIOZ80_PORT_MODE0_OUTPUT = 0,
        PIOZ80_PORT_MODE1_INPUT,
        PIOZ80_PORT_MODE2_BIDIR, // obousmerny rezim podporuje pouze brana A
        PIOZ80_PORT_MODE3_USER,
    } en_PIOZ80_PORT_MODE;


    typedef enum en_PIOZ80_EXPECT {
        PIOZ80_CWSTATE_EXPECT_COMMAND = 0,
        PIOZ80_CWSTATE_EXPECT_IOMCW,
        PIOZ80_CWSTATE_EXPECT_INTMCW,
    } en_PIOZ80_EXPECT;


    // Posunuto o 4 bity dolu - ve skutecnosti v ICW 4. - 7. bit
#define PIOZ80_ICENA_BIT    ( 1 << 3 )
#define PIOZ80_ICFNC_BIT    ( 1 << 2 )
#define PIOZ80_ICLVL_BIT    ( 1 << 1 )
#define PIOZ80_ICMSK_BIT    ( 1 << 0 )


    typedef enum en_PIOZ80_ICENA {
        PIOZ80_ICENA_DISABLED = 0,
        PIOZ80_ICENA_ENABLED
    } en_PIOZ80_ICENA;


    typedef enum en_PIOZ80_ICFNC {
        PIOZ80_ICFNC_OR = 0,
        PIOZ80_ICFNC_AND
    } en_PIOZ80_ICFNC;


    typedef enum en_PIOZ80_ICLVL {
        PIOZ80_ICLVL_LOW = 0,
        PIOZ80_ICLVL_HIGH
    } en_PIOZ80_ICLVL;


#define PIOZ80_INTERRUPT_INT_BIT    ( 1 << 0 )
#define PIOZ80_INTERRUPT_IEO_BIT    ( 1 << 1 )


    /*
     * 0. bit je INT
     * 1. bit je IEO
     */
    typedef enum en_PIOZ80_INTERRUPT {
        // klidovy stav
        PIOZ80_INTERRUPT_NONE = 0x00,
        // minimalne jeden z portu je ve stavu PENDING && INTCTRL_ENABLED
        PIOZ80_INTERRUPT_PENDING = 0x03,
        // tohle neemulujeme - jsme v PENDING a na sbernici se objevilo RETI, tak na 4 CPU takty prejdeme do NEXTPRIO
        PIOZ80_INTERRUPT_NEXTPRIO = 0x01,
        // minimalne jeden z portu byl ve stavu PENDING && INTCTRL_ENABLED a CPU interrupt prijal
        PIOZ80_INTERRUPT_RECEIVED = 0x02
    } en_PIOZ80_INTERRUPT;

#define PIOZ80_PORT_INT_PENDING_BIT     ( 1 << 0 )
#define PIOZ80_PORT_INT_RECEIVED_BIT    ( 1 << 1 )


    typedef enum en_PIOZ80_PORT_INT {
        // klidovy stav
        PIOZ80_PORT_INT_NONE = 0x00,
        // port eviduje interrupt a ceka na vyrizeni
        PIOZ80_PORT_INT_PENDING = 0x01,
        // port eviduje interrupt, ktery je prave odbavovan
        PIOZ80_PORT_INT_RECEIVED = 0x02,
        // port eviduje interrupt, ktery je prave odbavovan a uz tam ceka dalsi udalost na vyrizeni
        PIOZ80_PORT_INT_REPENDING = 0x03,
    } en_PIOZ80_PORT_INT;


    typedef enum en_PIOZ80_ICFNC_RESULT {
        PIOZ80_INTFNC_RESULT_FALSE = 0,
        PIOZ80_INTFNC_RESULT_TRUE
    } en_PIOZ80_ICFNC_RESULT;


    typedef enum en_PIOZ80_PORT_EVENT {
        PIOZ80_PORT_EVENT_RESET = 0,
        // udalosti vytvorene zmenou napeti na skutecnych pinech
        // aktivace pripadneho interruptu je vystavena prakticky ihned
        PIOZ80_PORT_EVENT_PA4_CTC0, // udalost vznikla zmenou /CTC0
        PIOZ80_PORT_EVENT_PA5_VBLN, // udalost vznikla zmenou /VBLN

        // interni udalosti vznikle na zaklade IORQ - pripadny int je vystaven ihned

        // aktivace pripadneho interruptu je vystavena ihned uvnitr IORQ
        PIOZ80_PORT_EVENT_INTERNAL_MODE3_LEAVED, // doslo ke zmene MODE3 na MOD0, 1, 2
        PIOZ80_PORT_EVENT_INTERNAL_WR_DATA, // doslo k zapisu do data output registru
        PIOZ80_PORT_EVENT_INTERNAL_CHANGED_IC_FNCLVL, // doslo ke zmene Interrupt Controll (ICW)
        PIOZ80_PORT_EVENT_INTERNAL_CHANGED_IOMSK, // zmena I/O mask v MODE3 (IOMCW)

        // deaktivace pripadneho interruptu nasleduje ihned uvnitr IORQ
        PIOZ80_PORT_EVENT_INTERNAL_INT_PENDING_RESET, // v ICW byl nastaven ICMSK_BIT - interrupt reset

        // interni udalosti vznikle na zaklade sledovani sbernice CPU Z80

        // casovani neemulujeme - deaktivace INT po 3 CPU taktech 
        PIOZ80_PORT_EVENT_CPUBUS_INTACK, // CPU potvrdil prijeti interruptu
        // deaktivace IEO ihned, pokud uz je nejaky port ve stavu PENDING, tak aktivace INT ihned uvnitr RETI instrukce
        // casovani neemulujeme - zmena nastane 4 CPU takty po zacatku RETI
        PIOZ80_PORT_EVENT_CPUBUS_RETI, // CPU oznamil navrat z obsluhy interruptu

        // deaktivace ihned
        // aktivace pripadneho interruptu probehne az 3 CPU takty po ukonceni IORQ
        PIOZ80_PORT_EVENT_INTERNAL_CHANGED_ICENA, // v ICW byl nastaven ICMSK_BIT - interrupt reset

    } en_PIOZ80_PORT_EVENT;


    typedef struct st_PIOZ80_PORT {
        en_PIOZ80_PORT_ID port_id;
        en_PIOZ80_PORT_MODE mode; // Mode Control Register
        Z80EX_BYTE io_mask; // I/O Select Register (8 bitu, 0 - vystup, 1 - vstup)
        en_PIOZ80_EXPECT ctrl_expect;
        Z80EX_BYTE data_output; // Data Output Registr
        Z80EX_BYTE masked_input;
        Z80EX_BYTE interrupt_vector; // 7 bitu - 1. bit je vzdy '0'
        en_PIOZ80_ICFNC icfnc;
        en_PIOZ80_ICLVL iclvl; // Mask Control Registr
        Z80EX_BYTE icmask; // Interrupt Controll Mask - 8 bitu, 0 - on, 1 - off )
        en_PIOZ80_ICENA icena;
        en_PIOZ80_PORT_INT port_int;
        en_PIOZ80_ICFNC_RESULT last_intfnc_result;
    } st_PIOZ80_PORT;


    typedef struct st_PIOZ80 {
        st_PIOZ80_PORT port [ PIOZ80_PORT_COUNT ];
        en_PIOZ80_INTERRUPT interrupt; // 0. bit INT, 1. bit IEO
        en_PIOZ80_PORT_ID interrupt_port_id;

        st_EVENT icena_event;
        en_PIOZ80_PORT_ID icena_event_port_id;
    } st_PIOZ80;

    extern st_PIOZ80 g_pioz80;


    extern void pioz80_init ( void );
    extern void pioz80_reset ( void );

    extern Z80EX_BYTE pioz80_read_byte ( en_PIOZ80_ADDR addr );
    extern void pioz80_write_byte ( en_PIOZ80_ADDR addr, Z80EX_BYTE value );

    extern Z80EX_BYTE pioz80_interrupt_ack_im2_cb ( Z80EX_CONTEXT *cpu, void *user_data );
    extern void pioz80_interrupt_ack ( void );

    extern void pioz80_interrupt_reti_cb ( Z80EX_CONTEXT *cpu, void *user_data );

    extern void pioz80_port_id_event ( en_PIOZ80_PORT_ID port_id, en_PIOZ80_PORT_EVENT port_event, int pinvalue );

    extern void pioz80_icena_event ( void );

#include "gdg/video.h"
#define pioz80_on_screen_done_event() {\
    if ( g_pioz80.icena_event.ticks != -1 ) {\
        g_pioz80.icena_event.ticks -= VIDEO_SCREEN_TICKS;\
    };\
}


#ifdef __cplusplus
}
#endif

#endif /* PIOZ80_H */

