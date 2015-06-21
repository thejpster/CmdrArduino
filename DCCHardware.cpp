/*
 * CmdrArduino
 *
 * DCC Hardware Interface
 *
 * This module uses Timer 1 output pins A and B. On an AtMega328 based Arduino Uno
 * or similar, this is Pin 9 and Pin 10. Connect these two pins to the two
 * gate inputs on your L298 H-Bridge. If you have an LMD18200 H-Bridge with
 * a single direction input, use either Pin 9 or Pin 10.
 *
 * Author: Don Goodman-Wilson dgoodman@artificial-science.org
 * Changes by: Jonathan Pallant dcc@thejpster.org.uk
 *
 * based on software by Wolfgang Kufer, http://opendcc.de
 *
 * Copyright 2010 Don Goodman-Wilson
 * Copyright 2015 Jonathan Pallant
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/****************************************************************************
* Includes
****************************************************************************/
#include <Arduino.h>
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "DCCHardware.h"

/****************************************************************************
* Defines
****************************************************************************/

#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__AVR_AT90CAN128__) || defined(__AVR_AT90CAN64__) || defined(__AVR_AT90CAN32__)

//On Arduino MEGA, etc, OC1A is digital pin 11, or Port B/Pin 5
#define OC1A_OUTPUT_PIN (PINB & (1 << PINB5))
//On Arduino MEGA, etc, OC1A is or Port B/Pin 5 and OC1B Port B/Pin 6
#define OC1_OUTPUT_DIR ((1 << DDB5) | (1 << DDB6))

#else

//On Arduino UNO, etc, OC1A is digital pin 9, or Port B/Pin 1
#define OC1A_OUTPUT_PIN (PINB & (1 << PINB1))
//On Arduino UNO, etc, OC1A is Port B/Pin 1 and OC1B Port B/Pin 2
#define OC1_OUTPUT_DIR ((1 << DDB1) | (1 << DDB2))

#endif

#define PREAMBLE_BITS 13 /* Bit counter counts from 13..0 => 14 bits */

/****************************************************************************
* Data Types
**************************************************/

/// An enumerated type for keeping track of the state machine used in the
/// timer1 ISR
/** Given the structure of a DCC packet, the ISR can be in one of 5 states.
      * DCC_HW_STATE_IDLE: there is nothing to put on the rails. In this case,
        the only legal thing to do is to put a '1' on the rails.  The ISR
        should almost never be in this state.
      * DCC_HW_STATE_SEND_PREMABLE: A packet has been made available, and so
        we should broadcast the preamble: 14 '1's in a row
      * DCC_HW_STATE_SEND_BSTART: Each data byte is preceded by a '0'
      * DCC_HW_STATE_SEND_BYTE: Sending the current data byte
      * DCC_HW_STATE_END_BIT: After the final byte is sent, send a '0'.
*/
enum dcc_hw_state_t
{
    DCC_HW_STATE_IDLE,
    DCC_HW_STATE_SEND_PREAMBLE,
    DCC_HW_STATE_SEND_BSTART,
    DCC_HW_STATE_SEND_BYTE,
    DCC_HW_STATE_END_BIT
};

/****************************************************************************
* Function Prototypes
**************************************************/

/* None */

/****************************************************************************
* Public Data
**************************************************/


/****************************************************************************
* Private Data
**************************************************/

/// The currently queued packet to be put on the rails. Default is a reset packet.
static volatile uint8_t current_packet[6] = {0, 0, 0, 0, 0, 0};
/// How many data bytes in the queued packet?
static volatile uint8_t packet_size = 0;
/// How many bytes remain to be put on the rails?
static volatile uint8_t byte_counter = 0;
/// How many bits remain in the current data byte/preamble before changing states?
static volatile uint8_t bit_counter = PREAMBLE_BITS;

static enum dcc_hw_state_t dcc_hw_state = DCC_HW_STATE_IDLE; //just to start out


/// Timer1 TOP values for one and zero
/**

  S 9.1 A specifies that '1's are represented by a square wave with a half-
  period of 58us (valid range: 55-61us) and '0's with a half-period of >100us
  (valid range: 95-9900us). Because '0's are stretched to provide DC power to
  non-DCC locos, we need two zero counters, one for the top half, and one for
  the bottom half.

  Here is how to calculate the timer1 counter values (from ATMega168
  datasheet, 15.9.2):

  f_{OC1A} = \frac{f_{clk_I/O}}{2*N*(1+OCR1A)})

  where N = prescalar, and OCR1A is the TOP we need to calculate.

  We know the desired half period for each case, 58us and >100us.

  So:
  for ones:
  58us = (8*(1+OCR1A)) / (16MHz)
  58us * 16MHz = 8*(1+OCR1A)
  58us * 2MHz = 1+OCR1A
  OCR1A = 115

  for zeros:
  100us * 2MHz = 1+OCR1A
  OCR1A = 199

  Thus, we also know that the valid range for stretched-zero operation is something like this:
  9900us = (8*(1+OCR1A)) / (16MHz)
  9900us * 2MHz = 1+OCR1A
  OCR1A = 19799

*/
//static const uint16_t ONE_COUNT = 115; //58us
#define US_TO_TICKS(us) (((us) * F_CPU) / (8 * 1000000))
#define TIMER_COMP_VALUE(us) (US_TO_TICKS(us) - 1)
static const uint16_t ONE_COUNT = TIMER_COMP_VALUE(58);
static const uint16_t ZERO_HIGH_COUNT = TIMER_COMP_VALUE(100);
static const uint16_t ZERO_LOW_COUNT = TIMER_COMP_VALUE(100);

/****************************************************************************
* Public Functions
****************************************************************************/

/****************************************************************************
 * NAME
 *     dcc_hardware_setup
 *
 * DESCRIPTION
 *     Configure the chip hardware to generate two complementary outputs using
 *     Timer 1 and the overflow interrupt.
 *
 * PARAMETERS
 *     None
 *
 * RETURNS
 *     Nothing
 ****************************************************************************/
void dcc_hardware_setup()
{
    //Set the OC1A and OC1B pins (Timer1 output pins A and B) to output mode
    DDRB |= OC1_OUTPUT_DIR;

    // Configure timer1 in CTC mode, for waveform generation, set to toggle
    // OC1A, OC1B, at /8 prescalar, interupt at CTC
    TCCR1A = (0 << COM1A1) | (1 << COM1A0) | (0 << COM1B1) | (1 << COM1B0) |
        (0 << WGM11) | (0 << WGM10);
    TCCR1B = (0 << ICNC1)  | (0 << ICES1)  | (0 << WGM13)  | (1 << WGM12)  |
        (0 << CS12)  | (1 << CS11) | (0 << CS10);

    // Start by outputting a '1'
    // Whenever we set OCR1A, we must also set OCR1B, or else pin OC1B will get
    // out of sync with OC1A!
    OCR1A = OCR1B = ONE_COUNT;

    // Finally, force a toggle on OC1B so that pin OC1B will always complement
    // pin OC1A
    TCCR1C |= (1 << FOC1B);

    TIMSK1 |= (1 << OCIE1A);
}

/****************************************************************************
 * NAME
 *     dcc_hardware_need_packet
 *
 * DESCRIPTION
 *     Check if a packet is required.
 *
 * PARAMETERS
 *     None
 *
 * RETURNS
 *     true if packet required.
 ****************************************************************************/
bool dcc_hardware_need_packet(void)
{
    return (byte_counter == 0);
}

/****************************************************************************
 * NAME
 *     dcc_hardware_supply_packet
 *
 * DESCRIPTION
 *     Supply a new packet.
 *
 * PARAMETERS
 *     None
 *
 * RETURNS
 *     Nothing
 ****************************************************************************/
void dcc_hardware_supply_packet(const uint8_t* p_packet, size_t num_bytes)
{
    if (num_bytes <= sizeof(current_packet))
    {
        for(size_t i = 0; i < num_bytes; i++)
        {
            current_packet[i] = p_packet[i];
        }
        packet_size = num_bytes;
        byte_counter = packet_size;
    }
}


/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * NAME
 *     ISR(TIMER1_COMPA_vect)
 *
 * DESCRIPTION
 *     This is the Interrupt Service Routine (ISR) for Timer1 compare match.
 *
 * PARAMETERS
 *     None
 *
 * RETURNS
 *     Nothing
 ****************************************************************************/
ISR(TIMER1_COMPA_vect)
{
    // in CTC mode, timer TCINT1 automatically resets to 0 when it matches
    // OCR1A. Depending on the next bit to output, we may have to alter the
    // value in OCR1A, maybe. To switch between "one" waveform and "zero"
    // waveform, we assign a value to OCR1A.

    // remember, anything we set for OCR1A takes effect IMMEDIATELY, so we are
    // working within the cycle we are setting. First, check to see if we're
    // in the second half of a byte; only act on the first half of a byte if
    // the pin is low, we need to use a different zero counter to enable
    // stretched-zero DC operation
    if (!OC1A_OUTPUT_PIN)
    {
        //if the pin is low and outputting a zero, we need to be using
        // ZERO_LOW_COUNT
        if (OCR1A == ZERO_HIGH_COUNT)
        {
            OCR1A = OCR1B = ZERO_LOW_COUNT;
        }
    }
    else
    {
        // The pin is high. New cycle is begining. Here's where the real work
        // goes.
        // Time to switch things up, maybe. send the current bit in the current
        // packet. If this is the last bit to send, queue up another packet
        // (might be the idle packet).
        switch (dcc_hw_state)
        {
        /// Idle: Check if a new packet is ready. If it is, fall through to
        /// DCC_HW_STATE_SEND_PREMABLE. Otherwise just stick a '1' out there.
        case DCC_HW_STATE_IDLE:
            if (byte_counter == 0)
            {
                // If no new packet, just send ones if we don't know what else
                // to do. safe bet.
                OCR1A = OCR1B = ONE_COUNT;
                break;
            }

            dcc_hw_state = DCC_HW_STATE_SEND_PREAMBLE;
            //and fall through to DCC_HW_STATE_SEND_PREAMBLE

        /// Preamble: In the process of producing 14 '1's, counter by
        /// bit_counter; when complete, move to
        /// DCC_HW_STATE_SEND_BSTART
        case DCC_HW_STATE_SEND_PREAMBLE:
            OCR1A = OCR1B = ONE_COUNT;

            if (bit_counter == 0)
            {
                dcc_hw_state = DCC_HW_STATE_SEND_BSTART;
            }
            else
            {
                bit_counter--;
            }

            break;

        /// About to send a data byte, but have to peceed the data with a '0'.
        /// Send that '0', then move to DCC_HW_STATE_SEND_BYTE
        case DCC_HW_STATE_SEND_BSTART:
            OCR1A = OCR1B = ZERO_HIGH_COUNT;
            dcc_hw_state = DCC_HW_STATE_SEND_BYTE;
            // Counter runs 7..0 inclusive, i.e. 8 bits
            bit_counter = 7;
            break;

        /// Sending a data byte; current bit is tracked with
        /// bit_counter, and current byte with byte_counter
        case DCC_HW_STATE_SEND_BYTE:
            if (((current_packet[packet_size - byte_counter]) >> bit_counter) & 1)
            {
                // Current bit is a '1'
                OCR1A = OCR1B = ONE_COUNT;
            }
            else
            {
                // Or it is a '0'
                OCR1A = OCR1B = ZERO_HIGH_COUNT;
            }

            if (bit_counter == 0)
            {
                // Out of bits! time to either send a new byte, or end the
                // packet
                byte_counter--;
                if (byte_counter == 0)
                {
                    // If not more bytes, move to DCC_HW_STATE_END_BIT
                    dcc_hw_state = DCC_HW_STATE_END_BIT;
                }
                else
                {
                    // There are more bytes..., so, go back to
                    // DCC_HW_STATE_SEND_BSTART
                    dcc_hw_state = DCC_HW_STATE_SEND_BSTART;
                }
            }
            else
            {
                bit_counter--;
            }

            break;

        /// Done with the packet. Send out a final '1', then head back to
        /// DCC_HW_STATE_IDLE to check for a new packet.
        case DCC_HW_STATE_END_BIT:
            OCR1A = OCR1B = ONE_COUNT;
            dcc_hw_state = DCC_HW_STATE_IDLE;
            bit_counter = PREAMBLE_BITS;
            break;
        }
    }
}

/****************************************************************************
* End of file
****************************************************************************/
