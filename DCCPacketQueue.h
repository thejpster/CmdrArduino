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

#ifndef __DCCPACKETQUEUE_H__
#define __DCCPACKETQUEUE_H__


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
  byte read_pos;
  byte write_pos;
  byte size;
  byte written; //how many cells have valid data? used for determining full status.
public:
  DCCPacketQueue(void);

  virtual void setup(byte);

  ~DCCPacketQueue(void)
  {
    free(queue);
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

  virtual inline bool notRepeat(unsigned int address)
  {
    return (address != queue[read_pos].getAddress());
  }

  //void printQueue(void);

  virtual bool insertPacket(DCCPacket* packet); //makes a local copy, does not take over memory management!
  virtual bool readPacket(DCCPacket* packet); //does not hand off memory management of packet. used immediately.

  bool forget(uint16_t address, byte address_kind);
  void clear(void);
};

//A queue that, when a packet is read, puts that packet back in the queue if it requires repeating.
class DCCRepeatQueue: public DCCPacketQueue
{
public:
  DCCRepeatQueue(void);
  //void setup(byte length);
  bool insertPacket(DCCPacket* packet);
  bool readPacket(DCCPacket* packet);
};

//A queue that repeats the topmost packet as many times as is indicated by the packet before moving on
class DCCEmergencyQueue: public DCCPacketQueue
{
public:
  DCCEmergencyQueue(void);
  bool readPacket(DCCPacket* packet);
};

#endif //__DCCPACKETQUEUE_H__
