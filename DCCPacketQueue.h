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

#ifndef INC_DCCPACKETQUEUE_H
#define INC_DCCPACKETQUEUE_H


/**
 * A FIFO queue for holding DCC packets, implemented as a circular buffer.
 * Copyright 2010 D.E. Goodman-Wilson
 * TODO
**/

#include "DCCPacket.h"

class DCCPacketQueue
{
public: //protected:
    DCCPacket* queue;
    size_t read_pos;
    size_t write_pos;
    size_t size;
    size_t written; //how many cells have valid data? used for determining full status.
public:
    DCCPacketQueue(void);

    virtual void setup(size_t length);

    ~DCCPacketQueue(void)
    {
        delete [] queue;
    }

    virtual inline bool isFull(void)
    {
        return (written == size);
    }

    virtual inline bool isEmpty(void)
    {
        return (written == 0);
    }

    virtual inline bool notEmpty(void)
    {
        return (written > 0);
    }

    virtual inline bool notRepeat(DCCPacket::address_t address)
    {
        return (address != queue[read_pos].getAddress());
    }

    virtual bool insertPacket(const DCCPacket& packet); //makes a local copy, does not take over memory management!
    virtual bool readPacket(DCCPacket& packet); //does not hand off memory management of packet. used immediately.

    bool forget(DCCPacket::address_t address, DCCPacket::address_kind_t address_kind);
    void clear(void);
};

#endif // INC_DCCPACKETQUEUE_H

/****************************************************************************
 * End of file
 ****************************************************************************/
