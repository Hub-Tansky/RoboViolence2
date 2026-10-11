/*
	Copyright 2012 bitHeads inc.

	This file is part of the BaboViolent 2 source code.

	The BaboViolent 2 source code is free software: you can redistribute it and/or 
	modify it under the terms of the GNU General Public License as published by the 
	Free Software Foundation, either version 3 of the License, or (at your option) 
	any later version.

	The BaboViolent 2 source code is distributed in the hope that it will be useful, 
	but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
	FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License along with the 
	BaboViolent 2 source code. If not, see http://www.gnu.org/licenses/.
*/

// What each side accepts from the wire, checked before any handler runs (Phase B step 4). GL-free, so tests can check
// it; built into the client, the dedicated server and the test server library.

#ifndef PACKETCHECK_H
#define PACKETCHECK_H

#include <vector>

namespace packet
{
	// A client message the server accepts. size: exact payload bytes, 0 for text (at least one byte);
	// playerID: offset of the packet's playerID, which the server overwrites with the sender's slot (-1: none).
	struct ClientRule
	{
		int typeID;
		int size;
		int playerID;
	};
	// The rule for a client message type, or null for a type the server doesn't accept.
	const ClientRule * clientRule(int typeID);

	// A weaponID the game knows: indexes gameVar.weapons and the slot is filled.
	bool knownWeapon(int weaponID);

	// Checks a server message for the client: known type, exact size, index fields in range. On success `out` holds
	// the payload with every char[] field NUL-terminated (text gets a '\0' appended).
	bool checkServerMessage(const char * buffer, int size, int typeID, std::vector<char> & out);
}

#endif
