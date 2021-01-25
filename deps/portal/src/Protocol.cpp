/*
 portal
 Copyright (C) 2018 Will Townsend <will@townsend.io>

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along
 with this program. If not, see <https://www.gnu.org/licenses/>
 */

#include <cstdint>
#include <iostream>

#ifdef WIN32
#include <winsock2.h>
#endif

#include "Protocol.hpp"

namespace portal {

SimpleDataPacketProtocol::SimpleDataPacketProtocol()
{
	std::cout << "SimpleDataPacketProtocol created\n";
}

SimpleDataPacketProtocol::~SimpleDataPacketProtocol()
{
	buffer.clear();
	std::cout << "SimpleDataPacketProtocol destroyed\n";
}

std::vector<SimpleDataPacketProtocol::DataPacket>
SimpleDataPacketProtocol::processData(std::vector<char> data)
{
	if (data.size() > 0) {
		// Add data recieved to the end of buffer.
		buffer.insert(buffer.end(), data.data(),
			      data.data() + data.size());
	}

	auto headerLength = sizeof(_PortalFrame);

	auto packets = std::vector<DataPacket>();

	uint32_t length = 0;

	while (buffer.size() >= headerLength) {

		// Read the portal frame out
		_PortalFrame frame;

		memcpy(&frame, &buffer[0], sizeof(_PortalFrame));
		length = ntohl(length);

		frame.version = ntohl(frame.version);
		frame.type = ntohl(frame.type);
		frame.tag = ntohl(frame.tag);
		frame.payloadSize = ntohl(frame.payloadSize);

		if (headerLength + frame.payloadSize > buffer.size()) {
			// We don't yet have all of the payload in the buffer yet.
			break;
		}

		std::vector<char>::const_iterator first =
			buffer.begin() + sizeof(_PortalFrame);
		std::vector<char>::const_iterator last = buffer.begin() +
							 sizeof(_PortalFrame) +
							 frame.payloadSize;
		std::vector<char> newVec(first, last);

		auto packet = DataPacket();
		packet.version = frame.version;
		packet.type = frame.type;
		packet.tag = frame.tag;
		packet.data = newVec;

		packets.push_back(packet);

		// Remove the data from buffer
		buffer.erase(buffer.begin(), buffer.begin() +
						     sizeof(_PortalFrame) +
						     frame.payloadSize);
	}

	return packets;
}

void SimpleDataPacketProtocol::reset()
{
	buffer.clear();
}

}
