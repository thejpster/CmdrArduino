/*
 * CmdrArduino
 *
 * DCC Repeating Packet Queue
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

#ifndef INC_DCCREPEATQUEUE_H
#define INC_DCCREPEATQUEUE_H

#include "DCCPacketQueue.h"

//A queue that, when a packet is read, puts that packet back in the queue if it requires repeating.
class DCCRepeatQueue: public DCCPacketQueue
{
public:
  DCCRepeatQueue(void);
  //void setup(byte length);
  bool insertPacket(DCCPacket* packet);
  bool readPacket(DCCPacket* packet);
};

#endif // INC_DCCREPEATQUEUE_H


/****************************************************************************
 * End of file
 ****************************************************************************/
