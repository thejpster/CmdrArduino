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

#include "DCCEmergencyQueue.h"

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

DCCEmergencyQueue::DCCEmergencyQueue(void) : DCCPacketQueue()
{
}

/* Goes through each packet in the queue, repeats it getRepeat() times, and discards it */
bool DCCEmergencyQueue::readPacket(DCCPacket* packet)
{
  if (!isEmpty()) //anything in the queue?
  {
    queue[read_pos].setRepeat(queue[read_pos].getRepeat() - 1); //decrement the current packet's repeat count

    if (queue[read_pos].getRepeat()) //if the topmost packet needs repeating
    {
      memcpy(packet, &queue[read_pos], sizeof(DCCPacket));
      return true;
    }
    else //the topmost packet is ready to be discarded; use the DCCPacketQueue mechanism
    {
      return (DCCPacketQueue::readPacket(packet));
    }
  }

  return false;
}

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* None */

/****************************************************************************
 * End of file
 ****************************************************************************/