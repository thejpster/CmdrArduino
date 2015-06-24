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

/****************************************************************************
* Includes
****************************************************************************/
#include <Arduino.h>
#include <stdint.h>

#include "DCCRepeatQueue.h"

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

DCCRepeatQueue::DCCRepeatQueue(void) : DCCPacketQueue()
{
}

bool DCCRepeatQueue::insertPacket(DCCPacket* packet)
{
  if (packet->getRepeat())
  {
    return (DCCPacketQueue::insertPacket(packet));
  }

  return false;
}

bool DCCRepeatQueue::readPacket(DCCPacket* packet)
{
  if (!isEmpty())
  {
    memcpy(packet, &queue[read_pos], sizeof(DCCPacket));
    read_pos = (read_pos + 1) % size;
    --written;

    if (packet->getRepeat()) //the packet needs to be sent out at least one more time
    {
      packet->setRepeat(packet->getRepeat() - 1);
      insertPacket(packet);
    }

    return true;
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