/*
 * CmdrArduino
 *
 * DCC Packet Scheduler
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

#ifndef INC_DCCPACKETSCHEDULER_H
#define INC_DCCPACKETSCHEDULER_H

#include "DCCPacket.h"
#include "DCCPacketQueue.h"
#include "DCCEmergencyQueue.h"
#include "DCCRepeatQueue.h"
#include "DCCHardware.h"

#define E_STOP_QUEUE_SIZE           2
#define HIGH_PRIORITY_QUEUE_SIZE    10
#define LOW_PRIORITY_QUEUE_SIZE     10
#define REPEAT_QUEUE_SIZE           10
//#define PERIODIC_REFRESH_QUEUE_SIZE 10

#define LOW_PRIORITY_INTERVAL     5
#define REPEAT_INTERVAL           11
#define PERIODIC_REFRESH_INTERVAL 23

#define SPEED_REPEAT      3
#define FUNCTION_REPEAT   3
#define E_STOP_REPEAT     5
#define OPS_MODE_PROGRAMMING_REPEAT 3
#define OTHER_REPEAT      2

class DCCPacketScheduler
{
  public:

    DCCPacketScheduler(void);

    //for configuration
    void setDefaultSpeedSteps(uint8_t new_speed_steps);
    void setup(void); //for any post-constructor initialization

    //for enqueueing packets
    bool setSpeed(DCCPacket::address_t address, DCCPacket::address_kind_t address_kind, int8_t new_speed, uint8_t steps = 0); //new_speed: [-127,127]
    bool setSpeed14(DCCPacket::address_t address, DCCPacket::address_kind_t address_kind, int8_t new_speed, bool F0=true); //new_speed: [-13,13], and optionally F0 settings.
    bool setSpeed28(DCCPacket::address_t address, DCCPacket::address_kind_t address_kind, int8_t new_speed); //new_speed: [-28,28]
    bool setSpeed128(DCCPacket::address_t address, DCCPacket::address_kind_t address_kind, int8_t new_speed); //new_speed: [-127,127]

    //the function methods are NOT stateful; you must specify all functions each time you call one
    //keeping track of function state is the responsibility of the calling program.
    bool setFunctions(DCCPacket::address_t address, DCCPacket::address_kind_t address_kind, uint8_t F0to4, uint8_t F5to9=0x00, uint8_t F9to12=0x00);
    bool setFunctions(DCCPacket::address_t address, DCCPacket::address_kind_t address_kind, uint16_t functions);
    bool setFunctions0to4(DCCPacket::address_t address, DCCPacket::address_kind_t address_kind, uint8_t functions);
    bool setFunctions5to8(DCCPacket::address_t address, DCCPacket::address_kind_t address_kind, uint8_t functions);
    bool setFunctions9to12(DCCPacket::address_t address, DCCPacket::address_kind_t address_kind, uint8_t functions);
    //other cool functions to follow. Just get these working first, I think.

    bool setBasicAccessory(DCCPacket::address_t address, uint8_t function);
    bool unsetBasicAccessory(DCCPacket::address_t address, uint8_t function);

    bool opsProgramCV(DCCPacket::address_t address, DCCPacket::address_kind_t address_kind, uint16_t CV, uint8_t CV_data);

    //more specific functions
    bool eStop(void); //all locos
    bool eStop(DCCPacket::address_t address, DCCPacket::address_kind_t address_kind); //just one specific loco
    
    //to be called periodically within loop()
    void update(void); //checks queues, puts whatever's pending on the rails via global current_packet. easy-peasy

  private:

    void repeatPacket(DCCPacket *p); //insert into the appropriate repeat queue
    uint8_t default_speed_steps;
    uint16_t last_packet_address;

    uint8_t packet_counter;
    
    DCCEmergencyQueue e_stop_queue;
    DCCPacketQueue high_priority_queue;
    DCCPacketQueue low_priority_queue;
    DCCRepeatQueue repeat_queue;
};

#endif // INC_DCCPACKETSCHEDULER_H
