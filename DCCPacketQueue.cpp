/*
 * CmdrArduino
 *
 * DCC Packet Queue
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

#include "DCCPacketQueue.h"

/****************************************************************************
 * Defines
 ****************************************************************************/

/* None */

/****************************************************************************
 * Data Types
 ****************************************************************************/

/* None */

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

DCCPacketQueue::DCCPacketQueue(void) : read_pos(0), write_pos(0), size(10), written(0)
{
    return;
}

void DCCPacketQueue::setup(size_t length)
{
    size = length;
    queue = new DCCPacket[size];
}

bool DCCPacketQueue::insertPacket(const DCCPacket& packet)
{
    //First: Overwrite any packet with the same address and kind; if no such packet THEN hitup the packet at write_pos
    byte i = read_pos;

    while (i != (read_pos + written) % (size))  //(size+1) ) //size+1 so we can check the last slot, too…
    {
        if ((queue[i].getAddress() == packet.getAddress()) && (queue[i].getKind() == packet.getKind()))
        {
            queue[i] = packet;
            //do not increment written or modify write_pos
            return true;
        }

        i = (i + 1) % size;
    }

    //else, tack it on to the end
    if (!isFull())
    {
        //else, just write it at the end of the queue.
        queue[write_pos] = packet;
        write_pos = (write_pos + 1) % size;
        ++written;
        return true;
    }

    return false;
}

bool DCCPacketQueue::readPacket(DCCPacket& packet)
{
    if (!isEmpty())
    {
        packet = queue[read_pos];
        read_pos = (read_pos + 1) % size;
        --written;
        return true;
    }

    return false;
}

bool DCCPacketQueue::forget(DCCPacket::address_t address, DCCPacket::address_kind_t address_kind)
{
    bool found = false;

    for (int i = 0; i < size; ++i)
    {
        if ((queue[i].getAddress() == address) && (queue[i].getAddressKind() == address_kind))
        {
            found = true;
            queue[i] = DCCPacket(); //revert to default value
        }
    }

    --written;
    return found;
}

void DCCPacketQueue::clear(void)
{
    read_pos = 0;
    write_pos = 0;
    written = 0;

    for (int i = 0; i < size; ++i)
    {
        queue[i] = DCCPacket();
    }
}

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* None */

/****************************************************************************
 * End of file
 ****************************************************************************/