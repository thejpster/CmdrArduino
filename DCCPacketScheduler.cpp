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

/****************************************************************************
* Includes
****************************************************************************/
#include <Arduino.h>
#include <stdint.h>
#include <HardwareSerial.h>

#include "DCCPacketScheduler.h"
#include "DCCHardware.h"

/****************************************************************************
 * Defines
 ****************************************************************************/

#define SPEED_REPEAT      3
#define FUNCTION_REPEAT   3
#define E_STOP_REPEAT     5
#define OPS_MODE_PROGRAMMING_REPEAT 3
#define OTHER_REPEAT      2

#define E_STOP_QUEUE_SIZE           2
#define HIGH_PRIORITY_QUEUE_SIZE    10
#define LOW_PRIORITY_QUEUE_SIZE     10
#define REPEAT_QUEUE_SIZE           10
//#define PERIODIC_REFRESH_QUEUE_SIZE 10

#define LOW_PRIORITY_INTERVAL     5
#define REPEAT_INTERVAL           11
#define PERIODIC_REFRESH_INTERVAL 23

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

DCCPacketScheduler::DCCPacketScheduler(void) :
    default_speed_steps(128),
    last_packet_address(255),
    packet_counter(1)
{
    e_stop_queue.setup(E_STOP_QUEUE_SIZE);
    high_priority_queue.setup(HIGH_PRIORITY_QUEUE_SIZE);
    low_priority_queue.setup(LOW_PRIORITY_QUEUE_SIZE);
    repeat_queue.setup(REPEAT_QUEUE_SIZE);
    //periodic_refresh_queue.setup(PERIODIC_REFRESH_QUEUE_SIZE);
}

//for configuration
void DCCPacketScheduler::setDefaultSpeedSteps(uint8_t new_speed_steps)
{
    default_speed_steps = new_speed_steps;
}

void DCCPacketScheduler::setup(void) //for any post-constructor initialization
{
    dcc_hardware_setup();

    //Following RP 9.2.4, begin by putting 20 reset packets and 10 idle packets on the rails.
    //use the e_stop_queue to do this, to ensure these packets go out first!

    DCCPacket p;
    uint8_t data[] = {0x00};

    //reset packet: address 0x00, data 0x00, XOR 0x00; S 9.2 line 75
    p.addData(data, 1);
    p.setAddress(0x00, DCCPacket::DCC_SHORT_ADDRESS);
    p.setRepeat(20);
    p.setKind(RESET_PACKET_KIND);
    e_stop_queue.insertPacket(p);

    //WHy in the world is it that what gets put on the rails is 4 reset packets, followed by
    //10 god know's what, followed by something else?
    // C0 FF 00 FF
    // 00 FF FF   what are these?

    //idle packet: address 0xFF, data 0x00, XOR 0xFF; S 9.2 line 90
    p.setAddress(0xFF, DCCPacket::DCC_SHORT_ADDRESS);
    p.setRepeat(10);
    p.setKind(IDLE_PACKET_KIND);
    e_stop_queue.insertPacket(p); //e_stop_queue will be empty, so no need to check if insertion was OK.

}

//helper functions
void DCCPacketScheduler::repeatPacket(const DCCPacket& p)
{
    switch (p.getKind())
    {
    case IDLE_PACKET_KIND:
    case ESTOP_PACKET_KIND: //e_stop packets automatically repeat without having to be put in a special queue
        break;

    case SPEED_PACKET_KIND: //speed packets go to the periodic_refresh queue

    //  periodic_refresh_queue.insertPacket(p);
    //  break;
    case FUNCTION_PACKET_1_KIND: //all other packets go to the repeat_queue
    case FUNCTION_PACKET_2_KIND: //all other packets go to the repeat_queue
    case FUNCTION_PACKET_3_KIND: //all other packets go to the repeat_queue
    case ACCESSORY_PACKET_KIND:
    case RESET_PACKET_KIND:
    case OPS_MODE_PROGRAMMING_KIND:
    case OTHER_PACKET_KIND:
    default:
        repeat_queue.insertPacket(p);
    }
}

//for enqueueing packets

//setSpeed* functions:
//new_speed contains the speed and direction.
// a value of 0 = estop
// a value of 1/-1 = stop
// a value >1 (or <-1) means go.
// valid non-estop speeds are in the range [1,127] / [-127,-1] with 1 = stop
bool DCCPacketScheduler::setSpeed(DCCPacket::address_t address, DCCPacket::address_kind_t address_kind, int8_t new_speed, uint8_t steps)
{
    uint8_t num_steps = steps;

    //steps = 0 means use the default; otherwise use the number of steps specified
    if (!steps)
    {
        num_steps = default_speed_steps;
    }

    switch (num_steps)
    {
    case 14:
        return (setSpeed14(address, address_kind, new_speed));

    case 28:
        return (setSpeed28(address, address_kind, new_speed));

    case 128:
        return (setSpeed128(address, address_kind, new_speed));
    }

    return false; //invalid number of steps specified.
}

bool DCCPacketScheduler::setSpeed14(DCCPacket::address_t address, DCCPacket::address_kind_t address_kind, int8_t new_speed, bool F0)
{
    DCCPacket p(address, address_kind);
    uint8_t dir = 1;
    uint8_t speed_data_bytes[] = {0x40};
    uint16_t abs_speed = new_speed;

    if (new_speed < 0)
    {
        dir = 0;
        abs_speed = new_speed * -1;
    }

    if (!new_speed) //estop!
    {
        return eStop(address, address_kind);  //speed_data_bytes[0] |= 0x01; //estop
    }

    else if (abs_speed == 1) //regular stop!
    {
        speed_data_bytes[0] |= 0x00;  //stop
    }
    else //movement
    {
        speed_data_bytes[0] |= map(abs_speed, 2, 127, 2, 15);  //convert from [2-127] to [1-14]
    }

    speed_data_bytes[0] |= (0x20 * dir); //flip bit 3 to indicate direction;
    p.addData(speed_data_bytes, 1);

    p.setRepeat(SPEED_REPEAT);

    p.setKind(SPEED_PACKET_KIND);

    //speed packets get refreshed indefinitely, and so the repeat doesn't need to be set.
    //speed packets go to the high proirity queue
    return (high_priority_queue.insertPacket(p));
}

bool DCCPacketScheduler::setSpeed28(DCCPacket::address_t address, DCCPacket::address_kind_t address_kind, int8_t new_speed)
{
    DCCPacket p(address, address_kind);
    uint8_t dir = 1;
    uint8_t speed_data_bytes[] = {0x40};
    uint16_t abs_speed = new_speed;

    if (new_speed < 0)
    {
        dir = 0;
        abs_speed = new_speed * -1;
    }

    if (new_speed == 0) //estop!
    {
        return eStop(address, address_kind);  //speed_data_bytes[0] |= 0x01; //estop
    }
    else if (abs_speed == 1) //regular stop!
    {
        speed_data_bytes[0] |= 0x00;  //stop
    }
    else //movement
    {
        speed_data_bytes[0] |= map(abs_speed, 2, 127, 2, 0X1F); //convert from [2-127] to [2-31]
        //most least significant bit has to be shufled around
        speed_data_bytes[0] = (speed_data_bytes[0] & 0xE0) | ((speed_data_bytes[0] & 0x1F) >> 1) | ((speed_data_bytes[0] & 0x01) << 4);
    }

    speed_data_bytes[0] |= (0x20 * dir); //flip bit 3 to indicate direction;
    p.addData(speed_data_bytes, 1);

    p.setRepeat(SPEED_REPEAT);

    p.setKind(SPEED_PACKET_KIND);

    //speed packets get refreshed indefinitely, and so the repeat doesn't need to be set.
    //speed packets go to the high proirity queue
    //return(high_priority_queue.insertPacket(p));
    return (high_priority_queue.insertPacket(p));
}

bool DCCPacketScheduler::setSpeed128(DCCPacket::address_t address, DCCPacket::address_kind_t address_kind, int8_t new_speed)
{
    //why do we get things like this?
    // 03 3F 16 15 3F (speed packet addressed to loco 03)
    // 03 3F 11 82 AF  (speed packet addressed to loco 03, speed hex 0x11);
    DCCPacket p(address, address_kind);
    uint8_t dir = 1;
    uint16_t abs_speed = new_speed;
    uint8_t speed_data_bytes[] = {0x3F, 0x00};

    if (new_speed < 0)
    {
        dir = 0;
        abs_speed = new_speed * -1;
    }

    if (!new_speed) //estop!
    {
        return eStop(address, address_kind);  //speed_data_bytes[0] |= 0x01; //estop
    }
    else if (abs_speed == 1) //regular stop!
    {
        speed_data_bytes[1] = 0x00;  //stop
    }
    else //movement
    {
        speed_data_bytes[1] = abs_speed;  //no conversion necessary.
    }

    speed_data_bytes[1] |= (0x80 * dir); //flip bit 7 to indicate direction;
    p.addData(speed_data_bytes, 2);

    p.setRepeat(SPEED_REPEAT);

    p.setKind(SPEED_PACKET_KIND);

    //speed packets get refreshed indefinitely, and so the repeat doesn't need to be set.
    //speed packets go to the high proirity queue
    return (high_priority_queue.insertPacket(p));
}

bool DCCPacketScheduler::setFunctions(DCCPacket::address_t address, DCCPacket::address_kind_t address_kind, uint16_t functions)
{
    return setFunctions(address, address_kind, functions & 0x1F, (functions >> 5) & 0x0F, (functions >> 9) & 0x0F);
}

bool DCCPacketScheduler::setFunctions(DCCPacket::address_t address, DCCPacket::address_kind_t address_kind, uint8_t F0to4, uint8_t F5to8, uint8_t F9to12)
{
    bool retval = false;
    if (setFunctions0to4(address, address_kind, F0to4))
    {
        if (setFunctions5to8(address, address_kind, F5to8))
        {
            if (setFunctions9to12(address, address_kind, F9to12))
            {
                retval = true;
            }
        }
    }

    return retval;
}

bool DCCPacketScheduler::setFunctions0to4(DCCPacket::address_t address, DCCPacket::address_kind_t address_kind, uint8_t functions)
{
    DCCPacket p(address, address_kind);
    uint8_t data[] = {0x80};

    //Obnoxiously, the headlights (F0, AKA FL) are not controlled
    //by bit 0, but by bit 4. Really?

    //get functions 1,2,3,4
    data[0] |= (functions >> 1) & 0x0F;
    //get functions 0
    data[0] |= (functions & 0x01) << 4;

    p.addData(data, 1);
    p.setKind(FUNCTION_PACKET_1_KIND);
    p.setRepeat(FUNCTION_REPEAT);
    return low_priority_queue.insertPacket(p);
}


bool DCCPacketScheduler::setFunctions5to8(DCCPacket::address_t address, DCCPacket::address_kind_t address_kind, uint8_t functions)
{
    DCCPacket p(address, address_kind);
    uint8_t data[] = {0xB0};

    data[0] |= functions & 0x0F;

    p.addData(data, 1);
    p.setKind(FUNCTION_PACKET_2_KIND);
    p.setRepeat(FUNCTION_REPEAT);
    return low_priority_queue.insertPacket(p);
}

bool DCCPacketScheduler::setFunctions9to12(DCCPacket::address_t address, DCCPacket::address_kind_t address_kind, uint8_t functions)
{
    DCCPacket p(address, address_kind);
    uint8_t data[] = {0xA0};

    //least significant four functions (F5--F8)
    data[0] |= functions & 0x0F;

    p.addData(data, 1);
    p.setKind(FUNCTION_PACKET_3_KIND);
    p.setRepeat(FUNCTION_REPEAT);
    return low_priority_queue.insertPacket(p);
}


//other cool functions to follow. Just get these working first, I think.

//bool DCCPacketScheduler::setTurnout(DCCPacket::address_t address)
//bool DCCPacketScheduler::unsetTurnout(DCCPacket::address_t address)

bool DCCPacketScheduler::opsProgramCV(DCCPacket::address_t address, DCCPacket::address_kind_t address_kind, uint16_t CV, uint8_t CV_data)
{
    //format of packet:
    // {preamble} 0 [ AAAAAAAA ] 0 111011VV 0 VVVVVVVV 0 DDDDDDDD 0 EEEEEEEE 1 (write)
    // {preamble} 0 [ AAAAAAAA ] 0 111001VV 0 VVVVVVVV 0 DDDDDDDD 0 EEEEEEEE 1 (verify)
    // {preamble} 0 [ AAAAAAAA ] 0 111010VV 0 VVVVVVVV 0 DDDDDDDD 0 EEEEEEEE 1 (bit manipulation)
    // only concerned with "write" form here.

    DCCPacket p(address, address_kind);
    uint8_t data[] = {0xEC, 0x00, 0x00};

    // split the CV address up among data bytes 0 and 1
    data[0] |= ((CV - 1) & 0x3FF) >> 8;
    data[1] = (CV - 1) & 0xFF;
    data[2] = CV_data;

    p.addData(data, 3);
    p.setKind(OPS_MODE_PROGRAMMING_KIND);
    p.setRepeat(OPS_MODE_PROGRAMMING_REPEAT);

    return low_priority_queue.insertPacket(p);
}

//more specific functions

//broadcast e-stop command
bool DCCPacketScheduler::eStop(void)
{
    // 111111111111 0 00000000 0 01DC0001 0 EEEEEEEE 1
    DCCPacket e_stop_packet(0); //address 0
    uint8_t data[] = {0x71}; //01110001
    e_stop_packet.addData(data, 1);
    e_stop_packet.setKind(ESTOP_PACKET_KIND);
    e_stop_packet.setRepeat(10);
    e_stop_queue.insertPacket(e_stop_packet);
    //now, clear all other queues
    high_priority_queue.clear();
    low_priority_queue.clear();
    repeat_queue.clear();
    return true;
}

bool DCCPacketScheduler::eStop(DCCPacket::address_t address, DCCPacket::address_kind_t address_kind)
{
    // 111111111111 0 0AAAAAAA 0 01001001 0 EEEEEEEE 1
    // or
    // 111111111111 0 0AAAAAAA 0 01000001 0 EEEEEEEE 1
    DCCPacket e_stop_packet(address, address_kind);
    uint8_t data[] = {0x41}; //01000001
    e_stop_packet.addData(data, 1);
    e_stop_packet.setKind(ESTOP_PACKET_KIND);
    e_stop_packet.setRepeat(10);
    e_stop_queue.insertPacket(e_stop_packet);
    //now, clear this packet's address from all other queues
    high_priority_queue.forget(address, address_kind);
    low_priority_queue.forget(address, address_kind);
    repeat_queue.forget(address, address_kind);
}

bool DCCPacketScheduler::setBasicAccessory(DCCPacket::address_t address, uint8_t function)
{
    DCCPacket p(address);

    uint8_t data[] = { 0x01 | ((function & 0x03) << 1) };
    p.addData(data, 1);
    p.setKind(BASIC_ACCESSORY_PACKET_KIND);
    p.setRepeat(OTHER_REPEAT);

    return low_priority_queue.insertPacket(p);
}

bool DCCPacketScheduler::unsetBasicAccessory(DCCPacket::address_t address, uint8_t function)
{
    DCCPacket p(address);

    uint8_t data[] = { ((function & 0x03) << 1) };
    p.addData(data, 1);
    p.setKind(BASIC_ACCESSORY_PACKET_KIND);
    p.setRepeat(OTHER_REPEAT);

    return low_priority_queue.insertPacket(p);
}


//to be called periodically within loop()
void DCCPacketScheduler::update(void) //checks queues, puts whatever's pending on the rails via global current_packet. easy-peasy
{
    //TODO ADD POM QUEUE?
    if (dcc_hardware_need_packet()) //if the ISR needs a packet:
    {
        DCCPacket p;

        //Take from e_stop queue first, then high priority queue.
        //every fifth packet will come from low priority queue.
        //every 20th packet will come from periodic refresh queue. (Why 20? because. TODO reasoning)
        //if there's a packet ready, and the counter is not divisible by 5
        //first, we need to know which queues have packets ready, and the state of the this->packet_counter.
        if (!e_stop_queue.isEmpty())   //if there's an e_stop packet, send it now!
        {
            //e_stop
            e_stop_queue.readPacket(p); //nothing more to do. e_stop_queue is a repeat_queue, so automatically repeats where necessary.
        }
        else
        {
            bool doHigh = high_priority_queue.notEmpty() && high_priority_queue.notRepeat(last_packet_address);
            bool doLow = low_priority_queue.notEmpty() && low_priority_queue.notRepeat(last_packet_address) &&
                         !((packet_counter % LOW_PRIORITY_INTERVAL) && doHigh);
            bool doRepeat = repeat_queue.notEmpty() && repeat_queue.notRepeat(last_packet_address) &&
                            !((packet_counter % REPEAT_INTERVAL) && (doHigh || doLow));

            //bool doRefresh = periodic_refresh_queue.notEmpty() && periodic_refresh_queue.notRepeat(last_packet_address) &&
            //            !((packet_counter % PERIODIC_REFRESH_INTERVAL) && (doHigh || doLow || doRepeat));
            //examine queues in order from lowest priority to highest.
            //if(doRefresh)
            //{
            //  periodic_refresh_queue.readPacket(p);
            //  ++packet_counter;
            //}
            //else if(doRepeat)
            if (doHigh)
            {
                high_priority_queue.readPacket(p);
                ++packet_counter;
            }
            else if (doLow)
            {
                low_priority_queue.readPacket(p);
                ++packet_counter;
            }
            else if (doRepeat)
            {
                repeat_queue.readPacket(p);
                ++packet_counter;
            }

            //if none of these conditions hold, DCCPackets initialize to the idle packet, so that's what'll get sent.
            //++packet_counter; //it's a byte; let it overflow, that's OK.
            //enqueue the packet for repitition, if necessary:
            repeatPacket(p);
        }

        last_packet_address = p.getAddress(); //remember the address to compare with the next packet
        uint8_t buffer[DCC_PACKET_MAX_LEN];
        size_t count = p.getBitstream(buffer);

        dcc_hardware_supply_packet(buffer, count); //feed to the starving ISR.
    }
}

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* None */

/****************************************************************************
 * End of file
 ****************************************************************************/