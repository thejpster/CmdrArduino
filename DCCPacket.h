/*
 * CmdrArduino
 *
 * DCC Packet
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

#ifndef  INC_DCCPACKET_H
#define  INC_DCCPACKET_H

/****************************************************************************
 * Defines
 ****************************************************************************/

#define MULTIFUNCTION_PACKET_KIND_MASK 0x10
#define IDLE_PACKET_KIND               0x10
#define ESTOP_PACKET_KIND              0x11
#define SPEED_PACKET_KIND              0x12
#define FUNCTION_PACKET_1_KIND         0x13
#define FUNCTION_PACKET_2_KIND         0x14
#define FUNCTION_PACKET_3_KIND         0x15
#define ACCESSORY_PACKET_KIND          0x16
#define RESET_PACKET_KIND              0x17
#define OPS_MODE_PROGRAMMING_KIND      0x18

#define ACCESSORY_PACKET_KIND_MASK     0x40
#define BASIC_ACCESSORY_PACKET_KIND    0x40
#define EXTENDED_ACCESSORY_PACKET_KIND 0x41

#define OTHER_PACKET_KIND              0x00

#define DCC_PACKET_MAX_LEN             6

/****************************************************************************
 * Data Types
 ****************************************************************************/

class DCCPacket
{
public:
    typedef uint16_t address_t;

    typedef enum address_kind_t
    {
        DCC_SHORT_ADDRESS = 0,
        DCC_LONG_ADDRESS = 1
    } address_kind_t;

    DCCPacket(address_t decoder_address = 0xFF, address_kind_t decoder_address_kind = DCC_SHORT_ADDRESS);

    uint8_t getBitstream(uint8_t rawbytes[]); //returns size of array.
    uint8_t getSize(void);

    inline address_t getAddress(void)
    {
        return address;
    }

    inline uint8_t getAddressKind(void)
    {
        return address_kind;
    }

    inline void setAddress(address_t new_address)
    {
        address = new_address;
    }

    inline void setAddress(address_t new_address, address_kind_t new_address_kind)
    {
        address = new_address;
        address_kind = new_address_kind;
    }

    void addData(uint8_t new_data[], uint8_t new_size); //insert freeform data.

    inline void setKind(uint8_t new_kind)
    {
        kind = new_kind;
    }

    inline uint8_t getKind(void)
    {
        return kind;
    }

    inline void setRepeat(uint8_t new_repeat)
    {
        size_repeat = ((size_repeat & 0xC0) | (new_repeat & 0x3F)) ;
    }

    inline uint8_t getRepeat(void)
    {
        return size_repeat & 0x3F;
    }

private:
    //A DCC packet is at most 6 bytes: 2 of address, three of data, one of XOR
    address_t address;
    address_kind_t address_kind;
    uint8_t data[3];
    uint8_t size_repeat;  //a bit field! 0b11000000 = 0xC0 = size; 0x00111111 = 0x3F = repeat
    uint8_t kind;
};

/****************************************************************************
 * Function Prototypes
 ****************************************************************************/

/* None */

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* None */

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* None */

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* None */


/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* None */

#endif // INC_DCCPACKET_H

/****************************************************************************
 * End of file
 ****************************************************************************/

