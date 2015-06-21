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

#include <Arduino.h>
#include <stdint.h>

#include "DCCPacket.h"

DCCPacket::DCCPacket(address_t new_address, address_kind_t new_address_kind) : address(new_address), address_kind(new_address_kind), kind(IDLE_PACKET_KIND), size_repeat(0x40) //size(1), repeat(0)
{
	address = new_address;
	address_kind = new_address_kind;
	data[0] = 0x00; //default to idle packet
	data[1] = 0x00;
	data[2] = 0x00;
}

uint8_t DCCPacket::getBitstream(uint8_t rawbytes[]) //returns size of array.
{
	size_t total_size = 1; //minimum size
	uint8_t cs_byte = 0;

	if (kind & MULTIFUNCTION_PACKET_KIND_MASK)
	{

		if (kind == IDLE_PACKET_KIND) //idle packets work a bit differently:
			// since the "address" field is 0xFF, the logic below will produce C0 FF 00 3F instead of FF 00 FF
		{
			rawbytes[0] = 0xFF;
			cs_byte = rawbytes[0];
		}
		else if (address_kind == DCC_LONG_ADDRESS)   //This is a 14-bit address
		{
			rawbytes[0] = (uint8_t)((address >> 8) | 0xC0);
			rawbytes[1] = (uint8_t)(address & 0xFF);
			cs_byte = rawbytes[0] ^ rawbytes[1];
			total_size = 2;
		}
		else   //we have an 7-bit address
		{
			rawbytes[0] = (uint8_t)(address & 0x7F);
			cs_byte = rawbytes[0];
		}

		for (size_t i = 0; i < getSize(); ++i)
		{
			rawbytes[total_size++] = data[i];
			cs_byte ^= data[i];
		}

		rawbytes[total_size] = cs_byte;

		return total_size + 1;
	}
	else if (kind & ACCESSORY_PACKET_KIND_MASK)
	{
		if (kind == BASIC_ACCESSORY_PACKET_KIND)
		{
			// Basic Accessory Packet looks like this:
			// {preamble} 0 10AAAAAA 0 1AAACDDD 0 EEEEEEEE 1
			// or this:
			// {preamble} 0 10AAAAAA 0 1AAACDDD 0 (1110CCVV 0 VVVVVVVV 0 DDDDDDDD) 0 EEEEEEEE 1 (if programming)

			rawbytes[0] = 0x80; //set up address byte 0
			rawbytes[1] = 0x88; //set up address byte 1

			rawbytes[0] |= address & 0x03F;
			rawbytes[1] |= (~(address >> 2) & 0x70)
			               | (data[0] & 0x07);

			cs_byte = rawbytes[0] ^ rawbytes[1];
			total_size = 2;

			//now, add any programming bytes (skipping first data byte, of course)
			for (size_t i = 1; i < getSize(); ++i)
			{
				rawbytes[total_size++] = data[i];
				cs_byte ^= data[i];
			}

			//and, finally, the XOR

			for (size_t i = 0; i < total_size; ++i)
			{
				cs_byte ^= rawbytes[i];
			}

			rawbytes[total_size] = cs_byte;

			return total_size + 1;
		}
	}

	return 0; //ERROR! SHOULD NEVER REACH HERE! do something useful, like transform it into an idle packet or something! TODO
}

byte DCCPacket::getSize(void)
{
	return (size_repeat >> 6);
}

void DCCPacket::addData(byte new_data[], byte new_size) //insert freeform data.
{
	for (int i = 0; i < new_size; ++i)
	{
		data[i] = new_data[i];
	}

	size_repeat = (size_repeat & 0x3F) | (new_size << 6);
}