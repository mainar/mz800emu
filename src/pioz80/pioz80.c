/* 
 * File:   pioz80.c
 * Author: Michal Hucik <hucik@ordoz.com>
 *
 * Created on 3. července 2015, 21:27
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


#include <stdio.h>
#include <string.h>

#include "z80ex/include/z80ex.h"

#include "pioz80/pioz80.h"
#include "mz800.h"
#include "ctc8253/ctc8253.h"
#include "gdg/gdg.h"
#include "memory/memory.h"

//#define DBGLEVEL (DBGNON /* | DBGERR | DBGWAR | DBGINF*/)
//#define DBGLEVEL (DBGNON | DBGERR | DBGWAR | DBGINF )
#include "debug.h"
#include "typedefs.h"


st_PIOZ80 g_pioz80;


#if ( DBGLEVEL )


/*******************************************************************************
 * 
 * 
 * Nastroje pro DBGPRINT
 *  
 * 
 ******************************************************************************/


static inline char pioz80_dbg_get_port_name ( en_PIOZ80_PORT_ID value ) {
    return ( 'A' + value );
}


static inline const char* pioz80_dbg_get_icfnc_name ( en_PIOZ80_ICFNC value ) {
    if ( value == PIOZ80_ICFNC_OR ) return "Or";
    return "And";
}


static inline const char* pioz80_dbg_get_iclvl_name ( en_PIOZ80_ICLVL value ) {
    if ( value == PIOZ80_ICLVL_LOW ) return "Low";
    return "High";
}


static inline const char* pioz80_dbg_get_icena_status ( en_PIOZ80_ICENA value ) {
    if ( value == PIOZ80_ICENA_DISABLED ) return "Disabled";
    return "Enabled";
}


static inline char* pioz80_dbg_get_fullmask_string ( Z80EX_BYTE io_mask, Z80EX_BYTE icmask ) {
    static char retval[9];
    int i;
    char *c = &retval[7];
    for ( i = 0; i < 8; i++ ) {
        *c = ( io_mask & 0x01 ) ? 'i' : 'o'; // i - input, o - output
        if ( !( icmask & 0x01 ) ) {
            *c -= 0x20; // I - int input, O - int output
        };
        io_mask >>= 1;
        icmask >>= 1;
        c--;
    };
    return retval;
}


static inline char* pioz80_dbg_get_byte_string ( Z80EX_BYTE value ) {
    static char retval[9];
    int i;
    char *c = &retval[7];
    for ( i = 0; i < 8; i++ ) {
        *c = ( value & 0x01 ) ? '1' : '0';
        value >>= 1;
        c--;
    };
    return retval;
}


static inline const char* pioz80_dbg_get_intstate_string ( en_PIOZ80_INTERRUPT value ) {
    const char *retval;
    switch ( value ) {
        case PIOZ80_INTERRUPT_NONE:
            retval = "INTERRUPT_NONE";
            break;

        case PIOZ80_INTERRUPT_PENDING:
            retval = "INTERRUPT_PENDING";
            break;

        case PIOZ80_INTERRUPT_NEXTPRIO:
            retval = "INTERRUPT_NEXTPRIO";
            break;

        case PIOZ80_INTERRUPT_RECEIVED:
            retval = "INTERRUPT_RECEIVED";
            break;

        default:
            retval = "UNKNOWN";
            break;
    };
    return retval;
}


static inline const char* pioz80_dbg_get_icfnc_result_string ( en_PIOZ80_ICFNC_RESULT value ) {
    if ( value == PIOZ80_INTFNC_RESULT_FALSE ) {
        return "FALSE";
    };
    return "TRUE";
}


static inline const char* pioz80_dbg_get_port_event_name ( en_PIOZ80_PORT_EVENT value ) {
    const char *retval;
    switch ( value ) {

        case PIOZ80_PORT_EVENT_PA4_CTC0:
            retval = "PA4_CTC0";
            break;

        case PIOZ80_PORT_EVENT_PA5_VBLN:
            retval = "PA5_VBLN";
            break;

        case PIOZ80_PORT_EVENT_INTERNAL_MODE3_LEAVED:
            retval = "MODE3_LEAVED";
            break;

        case PIOZ80_PORT_EVENT_INTERNAL_WR_DATA:
            retval = "WR_DATA";
            break;

        case PIOZ80_PORT_EVENT_INTERNAL_CHANGED_IC_FNCLVL:
            retval = "CHANGED_IC_FNCLVL";
            break;

        case PIOZ80_PORT_EVENT_INTERNAL_CHANGED_IOMSK:
            retval = "CHANGED_IOMSK";
            break;

        case PIOZ80_PORT_EVENT_INTERNAL_INT_PENDING_RESET:
            retval = "INT_PENDING_RESET";
            break;

        case PIOZ80_PORT_EVENT_CPUBUS_INTACK:
            retval = "CPUBUS_INTACK";
            break;

        case PIOZ80_PORT_EVENT_CPUBUS_RETI:
            retval = "CPUBUS_RETI";
            break;

        case PIOZ80_PORT_EVENT_INTERNAL_CHANGED_ICENA:
            retval = "CHANGED_ICENA";
            break;

        default:
            retval = "UNKNOWN";
            break;
    };
    return retval;
}

#endif

/*******************************************************************************
 * 
 * 
 * Popis PIO-Z80
 *  
 * 
 ******************************************************************************/


/**
 * Inicializace PIO-Z80
 * 
 */
void pioz80_init ( void ) {
    DBGPRINTF ( DBGINF, "\n" );
    memset ( &g_pioz80, 0x00, sizeof ( g_pioz80 ) );
    g_pioz80.port[0].port_id = PIOZ80_PORT_A;
    g_pioz80.port[1].port_id = PIOZ80_PORT_B;
    g_pioz80.icena_event.ticks = -1;
    g_pioz80.icena_event.event_name = EVENT_PIOZ80;
    g_pioz80.icena_event_port_id = PIOZ80_PORT_NONE;
}


/**
 * Resetovani obvodu PIO-Z80
 * 
 */
void pioz80_reset ( void ) {
    DBGPRINTF ( DBGINF, "\n" );
    en_PIOZ80_PORT_ID port_id;
    for ( port_id = PIOZ80_PORT_A; port_id < PIOZ80_PORT_COUNT; port_id++ ) {
        st_PIOZ80_PORT *port = &g_pioz80.port[port_id];
        // na interrupt vector se podle DS pri resetu nesaha
        port->io_mask = 0xff;
        port->icmask = 0xff;
        port->mode = PIOZ80_PORT_MODE1_INPUT;
        port->icena = PIOZ80_ICENA_DISABLED;
        port->masked_input = 0x00;
        port->data_output = 0x00;
        port->last_intfnc_result = PIOZ80_INTFNC_RESULT_FALSE;
        port->ctrl_expect = PIOZ80_CWSTATE_EXPECT_COMMAND;
        port->port_int = PIOZ80_PORT_INT_NONE;
        // TODO: tyto registry nejsou popsany v manualu, tak je potreba overit zda se resetuji a jak
        port->icfnc = PIOZ80_ICFNC_OR;
        port->iclvl = PIOZ80_ICLVL_LOW;
    };

    g_pioz80.interrupt = PIOZ80_INTERRUPT_NONE;
    g_pioz80.interrupt_port_id = PIOZ80_PORT_NONE;

    g_pioz80.icena_event.ticks = -1;
    g_pioz80.icena_event.event_name = EVENT_PIOZ80;
    g_pioz80.icena_event_port_id = PIOZ80_PORT_NONE;
}


/**
 * Ziskani vysledku z aktualni intfunc (AND/OR)
 * 
 * @param port
 * @return 
 */
static inline en_PIOZ80_ICFNC_RESULT pioz80_port_get_intfnc_result ( st_PIOZ80_PORT *port ) {
    if ( port->icfnc == PIOZ80_ICFNC_OR ) {
        if ( port->masked_input ) {
            return PIOZ80_INTFNC_RESULT_TRUE;
        };
    } else {
        Z80EX_BYTE mask = ~port->icmask;
        if ( ( port->masked_input & mask ) == mask ) {
            return PIOZ80_INTFNC_RESULT_TRUE;
        };
    };
    return PIOZ80_INTFNC_RESULT_FALSE;
}


/**
 * Eskalace stavu signalu /INT. Port A ma vzdy prioritu.
 * 
 * @param port_event
 */
static inline void pioz80_interrupt_manager ( en_PIOZ80_PORT_EVENT port_event ) {

    // pokud jsme ve stavu INTERRUPT_RECEIVED, tak tu neni co resit - vysvobodi nas jen RESET, nebo RETI
    if ( g_pioz80.interrupt == PIOZ80_INTERRUPT_RECEIVED ) return;

    en_PIOZ80_PORT_ID port_id_pending = PIOZ80_PORT_NONE;
    en_PIOZ80_PORT_ID port_id;

    for ( port_id = PIOZ80_PORT_A; port_id < PIOZ80_PORT_COUNT; port_id++ ) {

        st_PIOZ80_PORT *port = &g_pioz80.port[port_id];

        // ve stavu INT_RECEIVED muze byt vzdy bud jen jeden port, nebo zadny
        if ( port->port_int == PIOZ80_PORT_INT_RECEIVED ) {
            DBGPRINTF ( DBGINF, "INTERRUPT_RECEIVED - screens: %d, ticks: %d\n", g_gdg.total_elapsed.screens, gdg_get_insigeop_ticks ( ) );
            g_pioz80.interrupt = PIOZ80_INTERRUPT_RECEIVED;
            g_pioz80.interrupt_port_id = port_id;
            // tohle je zarucene novy stav a projevi se okamzite - predchozi stav byl "1"
            mz800_interrupt_manager ( );
            return;
        } else if ( ( port->port_int == PIOZ80_PORT_INT_PENDING ) && ( port->icena == PIOZ80_ICENA_ENABLED ) ) {
            // cekajici udalosti na portu A maji vzdy prednost
            if ( port_id_pending == PIOZ80_PORT_NONE ) {
                port_id_pending = port_id;
            };
        };
    };

    if ( port_id_pending == PIOZ80_PORT_NONE ) {
        // setrvavame v klidovem stavu
        if ( g_pioz80.interrupt == PIOZ80_INTERRUPT_NONE ) return;

        // prechazime ze stavu INT do klidoveho stavu - vzdy okamzite
        DBGPRINTF ( DBGINF, "INTERRUPT_NONE - screens: %d, ticks: %d\n", g_gdg.total_elapsed.screens, gdg_get_insigeop_ticks ( ) );

        g_pioz80.interrupt = PIOZ80_INTERRUPT_NONE;
        mz800_interrupt_manager ( );

        return;
    };

    // setrvavame v neklidovem stavu
    if ( g_pioz80.interrupt == PIOZ80_INTERRUPT_PENDING ) return;

    // prechazime z klidoveho stavu do INT
    DBGPRINTF ( DBGINF, "INTERRUPT_PENDING - screens: %d, ticks: %d\n", g_gdg.total_elapsed.screens, gdg_get_insigeop_ticks ( ) );
    g_pioz80.interrupt = PIOZ80_INTERRUPT_PENDING;
    mz800_interrupt_manager ( );
}


/**
 * Resetovani cekajiciho interruptu - aktivuje se pouze nastavenim 4. bitu v ICRW
 * 
 * @param port
 */
static inline void pioz80_port_interrupt_reset ( st_PIOZ80_PORT *port ) {
    DBGPRINTF ( DBGINF, "port: %c, RESET PORT INTERRUPT SIGNAL, port_int_signal = %d, icena = %s\n", pioz80_dbg_get_port_name ( port->port_id ), port->port_int, pioz80_dbg_get_icena_status ( port->icena ) );
    // PENDING -> NONE
    // REPENDING -> RECEIVED
    port->port_int &= ~PIOZ80_PORT_INT_PENDING_BIT;
    pioz80_interrupt_manager ( PIOZ80_PORT_EVENT_INTERNAL_INT_PENDING_RESET );
}


/**
 * Byla splnena podminka pro aktivaci interruptu
 * 
 * @param port
 * @param port_event
 */
static inline void pioz80_port_interrupt_activate ( st_PIOZ80_PORT *port, en_PIOZ80_PORT_EVENT port_event ) {

    // port uz nyni ceka na vyrizeni interruptu
    if ( port->port_int & PIOZ80_PORT_INT_PENDING_BIT ) return;

    if ( port->port_int == PIOZ80_PORT_INT_NONE ) {
        port->port_int = PIOZ80_PORT_INT_PENDING;
        DBGPRINTF ( DBGINF, "port: %c, ACTIVATE PORT INTERRUPT SIGNAL, icena = %s\n", pioz80_dbg_get_port_name ( port->port_id ), pioz80_dbg_get_icena_status ( port->icena ) );
    } else {
        port->port_int = PIOZ80_PORT_INT_REPENDING;
        DBGPRINTF ( DBGINF, "port: %c, RE-ACTIVATE PORT INTERRUPT SIGNAL, icena = %s\n", pioz80_dbg_get_port_name ( port->port_id ), pioz80_dbg_get_icena_status ( port->icena ) );
    };

    pioz80_interrupt_manager ( port_event );
}


/**
 * Byla splnena podminka pro deaktivaci interruptu v dobe, kdy jsme po ACK a cekame na RETI (jsme ve stavu REPENDING)
 * 
 * @param port
 */
static inline void pioz80_port_interrupt_deactivate ( st_PIOZ80_PORT *port ) {
    port->port_int &= ~PIOZ80_PORT_INT_PENDING_BIT;
    DBGPRINTF ( DBGINF, "port: %c, DE-ACTIVATE PORT INTERRUPT SIGNAL\n", pioz80_dbg_get_port_name ( port->port_id ) );
}


/**
 * Nacteni skutecneho stavu vstupnich pinu zvolene brany.
 * 
 * Konstantni hodnoty na jednotlivych portech.
 * 
 * PA = 0x03
 *  PA0 - vstupni hodnota z nezapojeneho LPT
 *  PA1 - vstupni hodnota z nezapojeneho LPT
 * 
 * PB = 0xff - zpusobeno pripojenim na vstupy do invertoru
 * 
 * TODO: proverit - PA6 a PA7 mi podle mereni obcas vykazovaly hazadrni stav "1"
 * 
 * 
 * @param port
 * @return 
 */
static inline Z80EX_BYTE pioz80_port_get_raw_input ( st_PIOZ80_PORT *port ) {
    const Z80EX_BYTE g_pioz80_port_input[] = { 0x03, 0xff };
    Z80EX_BYTE retval = g_pioz80_port_input[port->port_id];
    if ( port->port_id == PIOZ80_PORT_A ) {
        retval |= ( CTC8253_OUT ( 0 ) == 1 ) ? 0 : 1 << 4; /* invertovany CTC0 */
        retval |= SIGNAL_GDG_VBLNK ? 1 << 5 : 0x00;
    };
    return retval;
}


/**
 * Nacteni hodnoty data portu dle io_mask.
 * 
 * @param port
 * @return 
 */
static inline Z80EX_BYTE pioz80_port_get_iomask_input ( st_PIOZ80_PORT *port ) {
    Z80EX_BYTE retval = 0x00;
    Z80EX_BYTE port_pins = pioz80_port_get_raw_input ( port );
    retval = ( port_pins & port->io_mask ) | ( port->data_output & ( ~port->io_mask ) );
    //DBGPRINTF ( DBGINF, "port: %c, port_pins = 0x%02x (\"%s\"), input_register = 0x%02x (\"%s\")\n", pioz80_dbg_get_port_name ( port->port_id ), port_pins, pioz80_dbg_get_byte_string ( port_pins ), retval, pioz80_dbg_get_byte_string ( retval ) );
    return retval;
}


/**
 * Precteme port io_mask input a bity v urovni a masce, ktere nas zajimaji nastavime '1'.
 * 
 * @param port
 * @return 
 */
static inline Z80EX_BYTE pioz80_port_get_intmask_result ( st_PIOZ80_PORT *port ) {
    Z80EX_BYTE data_input = pioz80_port_get_iomask_input ( port );
    Z80EX_BYTE result = ( port->iclvl == PIOZ80_ICLVL_LOW ) ? ~data_input : data_input;
    result &= ( ~port->icmask );
    return result;
}


/**
 * Cteni z data registru.
 * 
 * @param port
 * @return 
 */
static inline Z80EX_BYTE pioz80_port_rd_data ( st_PIOZ80_PORT *port ) {
    Z80EX_BYTE retval = pioz80_port_get_iomask_input ( port );
    DBGPRINTF ( DBGINF, "port: %c, retval = 0x%02x, PC = 0x%04x\n", pioz80_dbg_get_port_name ( port->port_id ), retval, g_mz800.instruction_addr );
    return retval;
}


/**
 * IORQ - cteni z PIO-Z80
 * 
 * @param addr
 * @return 
 */
Z80EX_BYTE pioz80_read_byte ( en_PIOZ80_ADDR addr ) {

    en_PIOZ80_PORT_ID port_id = ( addr & PIOZ80_PORT_ID_MASK );
    en_PIOZ80_ADDRTYPE addr_type = ( addr & PIOZ80_ADDR_TYPE_MASK );

    if ( PIOZ80_ADDRTYPE_DATA == addr_type ) {
        st_PIOZ80_PORT *port = &g_pioz80.port[port_id];
        return pioz80_port_rd_data ( port );
    };

    // cteni z ctrl adresy vraci vzdy 0xff
    Z80EX_BYTE retval = 0xff;
    DBGPRINTF ( DBGINF, "read from CTRL reg - port: %c, retval = 0x%02x, PC = 0x%04x\n", pioz80_dbg_get_port_name ( port_id ), retval, g_mz800.instruction_addr );
    return retval;
}


/**
 * Zpracovani udalosti pri zmene urovni vstupnich pinu, vystupnich bitu a intcrtl
 * 
 * @param port
 * @param port_event
 * @param pinvalue
 */
static inline void pioz80_port_event ( st_PIOZ80_PORT *port, en_PIOZ80_PORT_EVENT port_event, int pinvalue ) {

#if DBGLEVEL
    char dbg_pinvalue[16];
    if ( port_event <= PIOZ80_PORT_EVENT_PA5_VBLN ) {
        snprintf ( dbg_pinvalue, sizeof ( dbg_pinvalue ), ", pinvalue = %d,", pinvalue );
    } else {
        dbg_pinvalue[0] = 0x00;
    };
#endif

    // dokud cekame na INTMCW, tak nelze vytvorit interrupt
    if ( port->ctrl_expect == PIOZ80_CWSTATE_EXPECT_INTMCW ) {
        DBGPRINTF ( DBGINF, "port: %c, ignored = EXPECT_INTMCW, event = %s%s\n", pioz80_dbg_get_port_name ( port->port_id ), pioz80_dbg_get_port_event_name ( port_event ), dbg_pinvalue );
        return;
    };

    // pokud je INT_PENDING
    if ( port->port_int == PIOZ80_PORT_INT_PENDING ) {
        DBGPRINTF ( DBGINF, "port: %c, ignored = PENDING, event = %s%s\n", pioz80_dbg_get_port_name ( port->port_id ), pioz80_dbg_get_port_event_name ( port_event ), dbg_pinvalue );
        return;
    };

    Z80EX_BYTE old_masked_input = port->masked_input;
    port->masked_input = pioz80_port_get_intmask_result ( port );

    // event nezpusobil zadnou zmenu
    if ( old_masked_input == port->masked_input ) {
        DBGPRINTF ( DBGINF, "port: %c, ignored = SAME_INPUT, data_input = 0x%02x, event = %s%s\n", pioz80_dbg_get_port_name ( port->port_id ), port->masked_input, pioz80_dbg_get_port_event_name ( port_event ), dbg_pinvalue );
        return;
    };

    en_PIOZ80_ICFNC_RESULT last_result = port->last_intfnc_result;
    port->last_intfnc_result = pioz80_port_get_intfnc_result ( port );

    // vysledek intfunc je stale stejny
    if ( last_result == port->last_intfnc_result ) {
        DBGPRINTF ( DBGINF, "port: %c, ignored = SAME_RESULT, result = %s, data_input = 0x%02x, event = %s%s\n", pioz80_dbg_get_port_name ( port->port_id ), pioz80_dbg_get_icfnc_result_string ( port->last_intfnc_result ), port->masked_input, pioz80_dbg_get_port_event_name ( port_event ), dbg_pinvalue );
        return;
    };

    DBGPRINTF ( DBGINF, "port: %c, result = %s, data_input = 0x%02x, event = %s%s\n", pioz80_dbg_get_port_name ( port->port_id ), pioz80_dbg_get_icfnc_result_string ( port->last_intfnc_result ), port->masked_input, pioz80_dbg_get_port_event_name ( port_event ), dbg_pinvalue );

    if ( port->last_intfnc_result == PIOZ80_INTFNC_RESULT_TRUE ) {
        pioz80_port_interrupt_activate ( port, port_event );
    } else if ( port->port_int == PIOZ80_PORT_INT_REPENDING ) {
        // pokud jsme v RE-PENDING (obsluhujeme potvrzeny interrupt), tak je mozne z PENDING prejit do NONE
        pioz80_port_interrupt_deactivate ( port );
    };
}


/**
 * Zpracovani lokalne vyvolane udalosti nad portem
 * 
 * @param port
 * @param port_event
 */
static inline void pioz80_port_event_internal ( st_PIOZ80_PORT *port, en_PIOZ80_PORT_EVENT port_event ) {
    pioz80_port_event ( port, port_event, 0 );
}


/**
 * Zapis do datoveho registru - data urcena pro vystup.
 * 
 * @param port
 * @param value
 */
static inline void pioz80_port_wr_data ( st_PIOZ80_PORT *port, Z80EX_BYTE value ) {
    port->data_output = value;
    DBGPRINTF ( DBGINF, "port: %c, value = 0x%02x, PC = 0x%04x\n", pioz80_dbg_get_port_name ( port->port_id ), port->data_output, g_mz800.instruction_addr );
    // pripadny INT je spusten ihned
    // TODO: tento fenomen se zrejme neobjevuje uplne vzdy
    pioz80_port_event_internal ( port, PIOZ80_PORT_EVENT_INTERNAL_WR_DATA );
}


/**
 * Ulozeni (inicializace) posledniho stavu na portu
 * 
 * @param port
 */
static inline void pioz80_port_save_input_state ( st_PIOZ80_PORT *port ) {
    port->masked_input = pioz80_port_get_intmask_result ( port );
    port->last_intfnc_result = pioz80_port_get_intfnc_result ( port );
    DBGPRINTF ( DBGINF, "port: %c, data_input = 0x%02x, intfnc_result = %s\n", pioz80_dbg_get_port_name ( port->port_id ), port->masked_input, pioz80_dbg_get_icfnc_result_string ( port->last_intfnc_result ) );
}


/**
 * Nastaveni interrupt vektoru
 * 
 * @param port
 * @param value
 */
static inline void pioz80_port_wr_ctrl_ivector ( st_PIOZ80_PORT *port, Z80EX_BYTE value ) {
    port->interrupt_vector = value;
    DBGPRINTF ( DBGINF, "port: %c, ivector = 0x%02x, PC = 0x%04x\n", pioz80_dbg_get_port_name ( port->port_id ), port->interrupt_vector, g_mz800.instruction_addr );
}


/**
 * Zpracovani Mode Control Word
 * 
 * @param port
 * @param value
 */
static inline void pioz80_port_wr_ctrl_mcw ( st_PIOZ80_PORT *port, Z80EX_BYTE value ) {

    switch ( value ) {

        case PIOZ80_PORT_MODE0_OUTPUT:
            port->io_mask = 0x00;
            DBGPRINTF ( DBGINF, "port: %c, MODE0-OUTPUT, PC = 0x%04x\n", pioz80_dbg_get_port_name ( port->port_id ), g_mz800.instruction_addr );
            break;

        case PIOZ80_PORT_MODE1_INPUT:
            port->io_mask = 0xff;
            DBGPRINTF ( DBGINF, "port: %c, MODE1-INPUT, PC = 0x%04x\n", pioz80_dbg_get_port_name ( port->port_id ), g_mz800.instruction_addr );
            break;

        case PIOZ80_PORT_MODE2_BIDIR:
            // TODO: tento rezim je platny jen pro branu A - nemam uplne ve vsem overeno jak se chova pri nastaveni pro B
            // vicemene se to vsak v aplikaci MZ-800 zrejme chova stejne jako v MODE1
            port->io_mask = 0xff;
            DBGPRINTF ( DBGINF, "port: %c, MODE2-BIDIR, PC = 0x%04x\n", pioz80_dbg_get_port_name ( port->port_id ), g_mz800.instruction_addr );
            break;

        case PIOZ80_PORT_MODE3_USER:
            port->ctrl_expect = PIOZ80_CWSTATE_EXPECT_IOMCW;
            DBGPRINTF ( DBGINF, "port: %c, MODE3-USER in latch - waiting for IOMCW..., PC = 0x%04x\n", pioz80_dbg_get_port_name ( port->port_id ), g_mz800.instruction_addr );
            return;
            break;
    };

    // TODO: tento fenomen se zrejme neobjevuje uplne vzdy
    if ( ( port->mode == PIOZ80_PORT_MODE3_USER ) && ( port->icfnc == PIOZ80_ICFNC_OR ) ) {
        // vyvolani interruptu probehne v podstate ihned uvnitr IORQ operace
        pioz80_port_interrupt_activate ( port, PIOZ80_PORT_EVENT_INTERNAL_MODE3_LEAVED );
    };

    port->mode = value;
}


/**
 * Zpracovani zmeny IC_ENA.
 * Pokud je DISABLED, tak zmena probehne ihned.
 * Pokud je ENABLED, tak zmena probehne za 3 CPU takty po nasledujicim M1
 * 
 * @param port
 * @param value
 * @return 
 *      0 - normalni casovani
 *      1 - byl vytvoren posunuty event
 */
static inline int pioz80_port_set_icena ( st_PIOZ80_PORT *port, en_PIOZ80_ICENA value ) {

    en_PIOZ80_ICENA old_icena = port->icena;
    if ( old_icena == value ) return 0;

    if ( value == PIOZ80_ICENA_DISABLED ) {
        port->icena = value;
        pioz80_interrupt_manager ( PIOZ80_PORT_EVENT_INTERNAL_CHANGED_ICENA );
        return 0;
    };

    unsigned tstates;
    Z80EX_BYTE byte = memory_read_byte ( g_mz800.instruction_addr );

    if ( byte == 0xd3 ) {

        // vykonavame OUT (#), a - 11 tstates
        tstates = 11;

    } else {

        Z80EX_BYTE byte = memory_read_byte ( g_mz800.instruction_addr + 1 );

        switch ( byte >> 8 ) {

            case 0x0a:
                // vykonavame OUTI, OUTD - 16 tstates
                tstates = 16;
                break;

            case 0x0b:
                // vykonavame OTIR, OTDR - 16/21 tstates
                if ( ( z80ex_get_reg ( g_mz800.cpu, regBC ) >> 8 ) == 0x00 ) {
                    tstates = 16;
                } else {
                    tstates = 21;
                };
                break;

            default:
                // vykonavame OUT (c), r, OUT (C),0 - 12 tstates
                tstates = 12;
                break;
        };
    };

    tstates += 3;
    unsigned event_ticks = mz800_get_instruction_start_ticks ( ) + ( tstates * GDGCLK2CPU_DIVIDER );

    SET_MZ800_EVENT ( EVENT_PIOZ80, event_ticks );
    g_pioz80.icena_event.ticks = event_ticks;
    g_pioz80.icena_event_port_id = port->port_id;

    return 1;
}


/**
 * Zpracovani Interrupt Control Word
 * 
 * @param port
 * @param value
 */
static inline void pioz80_port_wr_ctrl_icw ( st_PIOZ80_PORT *port, Z80EX_BYTE value ) {

    port->iclvl = ( value & PIOZ80_ICLVL_BIT ) ? PIOZ80_ICLVL_HIGH : PIOZ80_ICLVL_LOW;
    port->icfnc = ( value & PIOZ80_ICFNC_BIT ) ? PIOZ80_ICFNC_AND : PIOZ80_ICFNC_OR;
    en_PIOZ80_ICENA icena = ( value & PIOZ80_ICENA_BIT ) ? PIOZ80_ICENA_ENABLED : PIOZ80_ICENA_DISABLED;

    if ( value & PIOZ80_ICMSK_BIT ) {
        port->icena = icena;
        DBGPRINTF ( DBGINF, "port: %c, icfnc = %s, iclvl = %s, icena: %s, waiting for int_mask..., PC = 0x%04x\n", pioz80_dbg_get_port_name ( port->port_id ), pioz80_dbg_get_icfnc_name ( port->icfnc ), pioz80_dbg_get_iclvl_name ( port->iclvl ), pioz80_dbg_get_icena_status ( port->icena ), g_mz800.instruction_addr );
        port->ctrl_expect = PIOZ80_CWSTATE_EXPECT_INTMCW;
        pioz80_port_interrupt_reset ( port );
    } else {
        DBGPRINTF ( DBGINF, "port: %c, icfnc = %s, iclvl = %s, icena: %s, PC = 0x%04x\n", pioz80_dbg_get_port_name ( port->port_id ), pioz80_dbg_get_icfnc_name ( port->icfnc ), pioz80_dbg_get_iclvl_name ( port->iclvl ), pioz80_dbg_get_icena_status ( icena ), g_mz800.instruction_addr );
        int icena_event = pioz80_port_set_icena ( port, icena );
        if ( port->mode == PIOZ80_PORT_MODE3_USER ) {
            pioz80_port_event_internal ( port, PIOZ80_PORT_EVENT_INTERNAL_CHANGED_IC_FNCLVL );
            if ( !icena_event ) {
                pioz80_interrupt_manager ( PIOZ80_PORT_EVENT_INTERNAL_CHANGED_ICENA );
            };
        };
    };
}


/**
 * Zpracovani Interrupt Disable Word
 * 
 * @param port
 * @param value
 */
static inline void pioz80_port_wr_ctrl_idw ( st_PIOZ80_PORT *port, en_PIOZ80_ICENA value ) {
    DBGPRINTF ( DBGINF, "port: %c, icena: %s, PC = 0x%04x\n", pioz80_dbg_get_port_name ( port->port_id ), pioz80_dbg_get_icena_status ( value ), g_mz800.instruction_addr );
    int icena_event = pioz80_port_set_icena ( port, value );
    if ( !icena_event ) {
        pioz80_interrupt_manager ( PIOZ80_PORT_EVENT_INTERNAL_CHANGED_ICENA );
    };
}


/**
 * Zpracovani Interrupt Mask Control Word
 * 
 * @param port
 * @param value
 */
static inline void pioz80_port_wr_ctrl_intmcw ( st_PIOZ80_PORT *port, Z80EX_BYTE value ) {
    port->icmask = value;
    DBGPRINTF ( DBGINF, "port: %c, fullmask = 0x%02x (\"%s\"), PC = 0x%04x\n", pioz80_dbg_get_port_name ( port->port_id ), port->icmask, pioz80_dbg_get_fullmask_string ( port->io_mask, port->icmask ), g_mz800.instruction_addr );
    port->ctrl_expect = PIOZ80_CWSTATE_EXPECT_COMMAND;
    // predchozi ctrl bajt resetoval interrupt, nini si ulozime startovaci stav
    pioz80_port_save_input_state ( port );
}


/**
 * Zpracovani I/O Mask Control Word
 * 
 * @param port
 * @param value
 */
static inline void pioz80_port_wr_ctrl_iomcw ( st_PIOZ80_PORT *port, Z80EX_BYTE value ) {
    en_PIOZ80_PORT_MODE old_mcr = port->mode;
    port->mode = PIOZ80_PORT_MODE3_USER;
    port->io_mask = value;
    DBGPRINTF ( DBGINF, "port: %c, MODE3-USER, io_mask = 0x%02x (\"%s\"), PC = 0x%04x\n", pioz80_dbg_get_port_name ( port->port_id ), port->io_mask, pioz80_dbg_get_fullmask_string ( port->io_mask, port->icmask ), g_mz800.instruction_addr );
    port->ctrl_expect = PIOZ80_CWSTATE_EXPECT_COMMAND;
    if ( old_mcr == PIOZ80_PORT_MODE3_USER ) {
        //  po zmene I/O masky muze byt splnena podminka pro okamzity INT, ktery probehne ihned IORQ
        pioz80_port_event_internal ( port, PIOZ80_PORT_EVENT_INTERNAL_CHANGED_IOMSK );
    } else {
        pioz80_port_save_input_state ( port );
    };
}


/**
 * Zapis do control registru
 * 
 * @param port
 * @param value
 */
static inline void pioz80_port_wr_ctrl ( st_PIOZ80_PORT *port, Z80EX_BYTE value ) {

    switch ( port->ctrl_expect ) {

        case PIOZ80_CWSTATE_EXPECT_COMMAND:

            if ( PIOZ80_CTRL_IVW == ( value & PIOZ80_CTRL_IVW_MASK ) ) {
                pioz80_port_wr_ctrl_ivector ( port, value & 0xfe );
                break;
            };

            Z80EX_BYTE cmd = value & PIOZ80_CTRL_MASK;

            switch ( cmd ) {

                case PIOZ80_CTRL_MCW:
                    pioz80_port_wr_ctrl_mcw ( port, ( value >> 6 ) & 0x03 );
                    break;

                case PIOZ80_CTRL_ICW:
                    pioz80_port_wr_ctrl_icw ( port, ( value >> 4 ) & 0x0f );
                    break;

                case PIOZ80_CTRL_IDW:
                    pioz80_port_wr_ctrl_idw ( port, ( value >> 7 ) & 0x01 );
                    break;

                default:
                    DBGPRINTF ( DBGINF, "port: %c, UNKNOWN CONTROL WORD!, value = 0x%02x, PC = 0x%04x\n", pioz80_dbg_get_port_name ( port->port_id ), value, g_mz800.instruction_addr );
                    break;
            };
            break;

        case PIOZ80_CWSTATE_EXPECT_INTMCW:
            pioz80_port_wr_ctrl_intmcw ( port, value );
            break;

        case PIOZ80_CWSTATE_EXPECT_IOMCW:
            pioz80_port_wr_ctrl_iomcw ( port, value );
            break;
    };
}


/**
 * IORQ - zapis do PIO-Z80
 * 
 * @param addr
 * @param value
 */
void pioz80_write_byte ( en_PIOZ80_ADDR addr, Z80EX_BYTE value ) {

    en_PIOZ80_PORT_ID port_id = ( addr & PIOZ80_PORT_ID_MASK );
    en_PIOZ80_ADDRTYPE addr_type = ( addr & PIOZ80_ADDR_TYPE_MASK );

    st_PIOZ80_PORT *port = &g_pioz80.port[port_id];

    if ( PIOZ80_ADDRTYPE_DATA == addr_type ) {
        pioz80_port_wr_data ( port, value );
    } else {
        pioz80_port_wr_ctrl ( port, value );
    };
}

#if DBGLEVEL
static unsigned dbg_interrupt_ack_ticks = 0;
#endif


/**
 * Callback volany pro potvrzeni IM 2 interruptu
 * 
 * @param cpu
 * @param user_data
 * @return Z80EX_BYTE interrupt_vector LSB
 */
Z80EX_BYTE pioz80_interrupt_ack_im2_cb ( Z80EX_CONTEXT *cpu, void *user_data ) {

    if ( g_pioz80.interrupt != PIOZ80_INTERRUPT_PENDING ) {
        // TODO: prozatim nemam uplne nejlepe prozkoumano - 
        // vraci to vetsinou pripad od pripadu stejne hodnoty, ale netusim cim jsou predurcene
        DBGPRINTF ( DBGINF, "PIOZ80 is not in INTERRUPT_PENDING state, pioz80_state = %s, return ivector = 0x%04x, PC = 0x%04x\n", pioz80_dbg_get_intstate_string ( g_pioz80.interrupt ), ( z80ex_get_reg ( cpu, regPC ) << 8 ), g_mz800.instruction_addr );
        return 0x00;
    };

    st_PIOZ80_PORT *port;
    en_PIOZ80_PORT_ID port_id;

    // cekajici udalosti na portu A maji vzdy prednost
    for ( port_id = PIOZ80_PORT_A; port_id < PIOZ80_PORT_COUNT; port_id++ ) {

        port = &g_pioz80.port[port_id];

        if ( ( port->port_int == PIOZ80_PORT_INT_PENDING ) && ( port->icena == PIOZ80_ICENA_ENABLED ) ) {
            break;
        };
    };

#if DBGLEVEL
    dbg_interrupt_ack_ticks = gdg_get_total_ticks ( );
    Z80EX_WORD dbg_vector_addr = ( z80ex_get_reg ( cpu, regI ) << 8 ) | port->interrupt_vector;
    Z80EX_WORD dbg_newPC = ( memory_read_byte ( dbg_vector_addr + 1 ) << 8 ) | memory_read_byte ( dbg_vector_addr );
    DBGPRINTF ( DBGINF, "port: %c, INTERRUPT_RECEIVED - return ivector = 0x%04x (newPC = 0x%04x), PC = 0x%04x\n", pioz80_dbg_get_port_name ( port->port_id ), dbg_vector_addr, dbg_newPC, g_mz800.instruction_addr );
#endif

    port->port_int = PIOZ80_PORT_INT_RECEIVED;

    // casovani neemulujeme - deaktivace INT nastane po 3 CPU taktech
    pioz80_interrupt_manager ( PIOZ80_PORT_EVENT_CPUBUS_INTACK );

    pioz80_port_save_input_state ( port );

    return port->interrupt_vector;
}


/* TODO: zjistit co se stane, kdyz PIO po IM 2 CB nedostane RETI, ale opet dostane INT ACK a jak v takovem pripade vypada IM2 INT ACK */


/**
 * Potvrzeni o tom, ze CPU prijal interrupt - predevsim v pripadech, kdy neni v rezimu IM 2
 * 
 */
void pioz80_interrupt_ack ( void ) {

    // bud na zadny interrupt necekame, nebo uz byl potvrzen pres IM 2 callback
    if ( g_pioz80.interrupt != PIOZ80_INTERRUPT_PENDING ) return;

    // casovani neemulujeme - deaktivace INT nastane po 3 CPU taktech

    st_PIOZ80_PORT *port;
    en_PIOZ80_PORT_ID port_id;

    // cekajici udalosti na portu A maji vzdy prednost
    for ( port_id = PIOZ80_PORT_A; port_id < PIOZ80_PORT_COUNT; port_id++ ) {

        port = &g_pioz80.port[port_id];

        if ( ( port->port_int == PIOZ80_PORT_INT_PENDING ) && ( port->icena == PIOZ80_ICENA_ENABLED ) ) {
            break;
        };
    };

#if DBGLEVEL
    dbg_interrupt_ack_ticks = gdg_get_total_ticks ( );
#endif

    DBGPRINTF ( DBGINF, "port: %c, INTERRUPT_RECEIVED, PC = 0x%04x\n", pioz80_dbg_get_port_name ( port->port_id ), g_mz800.instruction_addr );

    port->port_int = PIOZ80_PORT_INT_RECEIVED;

    pioz80_interrupt_manager ( PIOZ80_PORT_EVENT_CPUBUS_INTACK );

    pioz80_port_save_input_state ( port );
}


/**
 * Potvrzeni o navratu z interruptu
 * 
 * @param cpu
 * @param user_data
 */
void pioz80_interrupt_reti_cb ( Z80EX_CONTEXT *cpu, void *user_data ) {

    // neemulujeme - skutecne PIO v tomto pripade prejde na 4 CPU takty do INTERRUPT_NEXTPRIO a pak zpet do INTERRUPT_PENDING
    //if ( g_pioz80.interrupt == PIOZ80_INTERRUPT_PENDING ) {};

    if ( g_pioz80.interrupt != PIOZ80_INTERRUPT_RECEIVED ) return;

    // casovani neemulujeme - tohle by melo nastat az 4 CPU takty po zacatku RETI

    st_PIOZ80_PORT *port = &g_pioz80.port[g_pioz80.interrupt_port_id];

#if DBGLEVEL
    unsigned dbg_interrupt_ticks = gdg_get_total_ticks ( ) - dbg_interrupt_ack_ticks;
    DBGPRINTF ( DBGINF, "port: %c, INTERRUPT_RETI, screens: %d, ticks: %d, interrupt_ticks = %d, PC = 0x%04x\n", pioz80_dbg_get_port_name ( port->port_id ), g_gdg.total_elapsed.screens, gdg_get_insigeop_ticks ( ), dbg_interrupt_ticks, g_mz800.instruction_addr );
#endif


    port->port_int &= ~PIOZ80_PORT_INT_RECEIVED_BIT;

    // jedine misto (mimo reset a pioz80_interrupt_manager(), kde se saha na top signaly )
    g_pioz80.interrupt = PIOZ80_INTERRUPT_NONE;
    g_pioz80.interrupt_port_id = PIOZ80_PORT_NONE;

    pioz80_interrupt_manager ( PIOZ80_PORT_EVENT_CPUBUS_RETI );
}


void pioz80_port_id_event ( en_PIOZ80_PORT_ID port_id, en_PIOZ80_PORT_EVENT port_event, int pinvalue ) {
    st_PIOZ80_PORT *port = &g_pioz80.port[port_id];
    pioz80_port_event ( port, port_event, pinvalue );
}


/**
 * Casove posunuta udalost - IC_ENA byl nastaven na ENABLED.
 * Zmena se projevi 3 CPU takty po nasledujicim M1.
 * 
 * @param event_ticks
 */
void pioz80_icena_event ( void ) {
    st_PIOZ80_PORT *port = &g_pioz80.port[g_pioz80.icena_event_port_id];
    DBGPRINTF ( DBGINF, "port: %c, IC_ENA - time shifted event\n", pioz80_dbg_get_port_name ( port->port_id ) );
    port->icena = PIOZ80_ICENA_ENABLED;
    pioz80_interrupt_manager ( PIOZ80_PORT_EVENT_INTERNAL_CHANGED_ICENA );
    g_pioz80.icena_event.ticks = -1;
    g_pioz80.icena_event_port_id = PIOZ80_PORT_NONE;
}

