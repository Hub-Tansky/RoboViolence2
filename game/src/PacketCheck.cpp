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

#include "PacketCheck.h"

#include "GameVar.h"
#include "Game.h"
#include "netPacket.h"

#include <cstddef>
#include <cstring>

namespace packet
{
	namespace
	{
		const ClientRule clientRules[] = {
			{NET_CLSV_PONG, sizeof(net_clsv_pong), offsetof(net_clsv_pong, playerID)},
			{NET_CLSV_SPAWN_REQUEST, sizeof(net_clsv_spawn_request), offsetof(net_clsv_spawn_request, playerID)},
			{NET_CLSV_PLAYER_SHOOT, sizeof(net_clsv_player_shoot), offsetof(net_clsv_player_shoot, playerID)},
			{NET_CLSV_GAMEVERSION_ACCEPTED, sizeof(net_clsv_gameversion_accepted), offsetof(net_clsv_gameversion_accepted, playerID)},
			{NET_CLSV_PICKUP_REQUEST, sizeof(net_clsv_pickup_request), offsetof(net_clsv_pickup_request, playerID)},
			{NET_CLSV_ADMIN_REQUEST, sizeof(net_clsv_admin_request), -1},
			{NET_CLSV_VOTE, sizeof(net_clsv_vote), offsetof(net_clsv_vote, playerID)},
			{NET_CLSV_MAP_LIST_REQUEST, sizeof(net_clsv_map_list_request), offsetof(net_clsv_map_list_request, playerID)},
			{NET_CLSV_SVCL_PLAYER_INFO, sizeof(net_clsv_svcl_player_info), offsetof(net_clsv_svcl_player_info, playerID)},
			{NET_CLSV_SVCL_CHAT, sizeof(net_clsv_svcl_chat), -1},
			{NET_CLSV_SVCL_TEAM_REQUEST, sizeof(net_clsv_svcl_team_request), offsetof(net_clsv_svcl_team_request, playerID)},
			{NET_CLSV_SVCL_PLAYER_COORD_FRAME, sizeof(net_clsv_svcl_player_coord_frame), offsetof(net_clsv_svcl_player_coord_frame, playerID)},
			{NET_CLSV_SVCL_PLAYER_CHANGE_NAME, sizeof(net_clsv_svcl_player_change_name), offsetof(net_clsv_svcl_player_change_name, playerID)},
			{NET_CLSV_SVCL_PLAYER_PROJECTILE, sizeof(net_clsv_svcl_player_projectile), offsetof(net_clsv_svcl_player_projectile, playerID)},
			{NET_CLSV_SVCL_PLAYER_SHOOT_MELEE, sizeof(net_clsv_svcl_player_shoot_melee), offsetof(net_clsv_svcl_player_shoot_melee, playerID)},
			{NET_CLSV_SVCL_VOTE_REQUEST, sizeof(net_clsv_svcl_vote_request), offsetof(net_clsv_svcl_vote_request, playerID)},
			{NET_CLSV_MAP_REQUEST, sizeof(net_clsv_map_request), -1},
			{NET_CLSV_SVCL_PLAYER_UPDATE_SKIN, sizeof(net_clsv_svcl_player_update_skin), offsetof(net_clsv_svcl_player_update_skin, playerID)},
			{NET_SVCL_PLAY_SOUND, sizeof(net_svcl_play_sound), -1},
			{NET_SVCL_CONSOLE, 0, -1},
		};

		// What an int8 (int16 for the two marked kinds) field of a server message may hold
		enum Kind
		{
			Slot,             // 0..31
			SlotOrNone,       // -1..31 (shot that hit a wall, flame that sticks to nobody)
			Team,             // spectator..auto-assign
			Weapon,           // knownWeapon
			ShotWeapon,       // knownWeapon or the minibot's gun (100)
			ProjectileWeapon, // knownWeapon, -1 (life pack, dropped weapon or grenade) or -2 (flame), see ServerRecv.cpp
			Flag,             // 0..1
			FlagState,        // -2 at base, -1 dropped, else the carrier's slot
			ChunkSize,        // uint16 0..250
			Index16,          // int16 >= 0
		};
		struct Field
		{
			int offset;
			Kind kind;
		};
		struct ServerRule
		{
			int typeID;
			int size; // 0: no payload; -1: text, at least one byte
			std::vector<Field> fields;
			std::vector<int> ends; // last byte of each char[] field
		};

#define F(type, field, kind) Field{(int)offsetof(type, field), kind}
#define END(type, field) (int)(offsetof(type, field) + sizeof(((type *)0)->field) - 1)
		const ServerRule serverRules[] = {
			{NET_SVCL_MAP_CHUNK, sizeof(net_svcl_map_chunk), {F(net_svcl_map_chunk, size, ChunkSize)}, {}},
			{NET_SVCL_VOTE_RESULT, sizeof(net_svcl_vote_result), {}, {}},
			{NET_SVCL_UPDATE_VOTE, sizeof(net_svcl_update_vote), {}, {}},
			{NET_SVCL_MSG, sizeof(net_svcl_msg), {}, {END(net_svcl_msg, message)}},
			{NET_SVCL_PLAYER_UPDATE_STATS, sizeof(net_svcl_player_update_stats), {F(net_svcl_player_update_stats, playerID, Slot)}, {}},
			{NET_CLSV_SVCL_VOTE_REQUEST, sizeof(net_clsv_svcl_vote_request), {F(net_clsv_svcl_vote_request, playerID, Slot)}, {END(net_clsv_svcl_vote_request, vote)}},
			{NET_CLSV_SVCL_PLAYER_SHOOT_MELEE, sizeof(net_clsv_svcl_player_shoot_melee), {F(net_clsv_svcl_player_shoot_melee, playerID, Slot)}, {}},
			{NET_SVCL_ADMIN_ACCEPTED, 0, {}, {}},
			{NET_SVCL_NEWPLAYER, sizeof(net_svcl_newplayer), {F(net_svcl_newplayer, newPlayerID, Slot)}, {}},
			{NET_SVCL_GAMEVERSION, sizeof(net_svcl_gameversion), {}, {}},
			{NET_CLSV_SVCL_PLAYER_INFO, sizeof(net_clsv_svcl_player_info), {F(net_clsv_svcl_player_info, playerID, Slot)},
				{END(net_clsv_svcl_player_info, playerIP), END(net_clsv_svcl_player_info, playerName), END(net_clsv_svcl_player_info, username), END(net_clsv_svcl_player_info, macAddr)}},
			{NET_CLSV_SVCL_CHAT, sizeof(net_clsv_svcl_chat), {}, {END(net_clsv_svcl_chat, message)}},
			{NET_SVCL_PLAYER_DISCONNECT, sizeof(net_svcl_player_disconnect), {F(net_svcl_player_disconnect, playerID, Slot)}, {}},
			{NET_SVCL_SERVER_INFO, sizeof(net_svcl_server_info), {}, {END(net_svcl_server_info, mapName)}},
			{NET_SVCL_PLAYER_ENUM_STATE, sizeof(net_svcl_player_enum_state),
				{F(net_svcl_player_enum_state, playerID, Slot), F(net_svcl_player_enum_state, teamID, Team), F(net_svcl_player_enum_state, weaponID, Weapon)},
				{END(net_svcl_player_enum_state, playerName), END(net_svcl_player_enum_state, playerIP), END(net_svcl_player_enum_state, skin)}},
			{NET_CLSV_SVCL_TEAM_REQUEST, sizeof(net_clsv_svcl_team_request),
				{F(net_clsv_svcl_team_request, playerID, Slot), F(net_clsv_svcl_team_request, teamRequested, Team)}, {}},
			{NET_SVCL_PING, sizeof(net_svcl_ping), {F(net_svcl_ping, playerID, Slot)}, {}},
			{NET_SVCL_PLAYER_PING, sizeof(net_svcl_player_ping), {F(net_svcl_player_ping, playerID, Slot)}, {}},
			{NET_SVCL_PLAYER_SPAWN, sizeof(net_svcl_player_spawn),
				{F(net_svcl_player_spawn, playerID, Slot), F(net_svcl_player_spawn, weaponID, Weapon), F(net_svcl_player_spawn, meleeID, Weapon)}, {END(net_svcl_player_spawn, skin)}},
			{NET_CLSV_SVCL_PLAYER_COORD_FRAME, sizeof(net_clsv_svcl_player_coord_frame), {F(net_clsv_svcl_player_coord_frame, playerID, Slot)}, {}},
			{NET_SVCL_MINIBOT_COORD_FRAME, sizeof(net_svcl_minibot_coord_frame), {F(net_svcl_minibot_coord_frame, playerID, Slot)}, {}},
			{NET_SVCL_CREATE_MINIBOT, sizeof(net_svcl_create_minibot), {F(net_svcl_create_minibot, playerID, Slot)}, {}},
			{NET_SVCL_PROJECTILE_COORD_FRAME, sizeof(net_svcl_projectile_coord_frame), {}, {}},
			{NET_SVCL_SV_CHANGE, sizeof(net_svcl_sv_change), {}, {END(net_svcl_sv_change, svChange)}},
			{NET_CLSV_SVCL_PLAYER_CHANGE_NAME, sizeof(net_clsv_svcl_player_change_name), {F(net_clsv_svcl_player_change_name, playerID, Slot)}, {END(net_clsv_svcl_player_change_name, playerName)}},
			{NET_SVCL_PLAYER_SHOOT, sizeof(net_svcl_player_shoot),
				{F(net_svcl_player_shoot, playerID, Slot), F(net_svcl_player_shoot, hitPlayerID, SlotOrNone), F(net_svcl_player_shoot, weaponID, ShotWeapon)}, {}},
			{NET_CLSV_SVCL_PLAYER_PROJECTILE, sizeof(net_clsv_svcl_player_projectile),
				{F(net_clsv_svcl_player_projectile, playerID, Slot), F(net_clsv_svcl_player_projectile, weaponID, ProjectileWeapon)}, {}},
			{NET_SVCL_DELETE_PROJECTILE, sizeof(net_svcl_delete_projectile), {}, {}},
			{NET_SVCL_FLAME_STICK_TO_PLAYER, sizeof(net_svcl_flame_stick_to_player),
				{F(net_svcl_flame_stick_to_player, projectileID, Index16), F(net_svcl_flame_stick_to_player, playerID, SlotOrNone)}, {}},
			{NET_SVCL_EXPLOSION, sizeof(net_svcl_explosion), {F(net_svcl_explosion, playerID, Slot)}, {}},
			{NET_SVCL_PLAYER_HIT, sizeof(net_svcl_player_hit),
				{F(net_svcl_player_hit, playerID, Slot), F(net_svcl_player_hit, fromID, Slot), F(net_svcl_player_hit, weaponID, Weapon)}, {}},
			{NET_SVCL_AUTOBALANCE, 0, {}, {}},
			{NET_SVCL_PLAY_SOUND, sizeof(net_svcl_play_sound), {}, {}},
			{NET_SVCL_CONSOLE, -1, {}, {}},
			{NET_SVCL_SYNCHRONIZE_TIMER, sizeof(net_svcl_synchronize_timer), {}, {}},
			{NET_SVCL_CHANGE_FLAG_STATE, sizeof(net_svcl_change_flag_state),
				{F(net_svcl_change_flag_state, flagID, Flag), F(net_svcl_change_flag_state, newFlagState, FlagState), F(net_svcl_change_flag_state, playerID, Slot)}, {}},
			{NET_SVCL_DROP_FLAG, sizeof(net_svcl_drop_flag), {F(net_svcl_drop_flag, flagID, Flag)}, {}},
			{NET_SVCL_FLAG_ENUM, sizeof(net_svcl_flag_enum),
				{Field{(int)offsetof(net_svcl_flag_enum, flagState), FlagState}, Field{(int)offsetof(net_svcl_flag_enum, flagState) + 1, FlagState}}, {}},
			{NET_SVCL_GAME_STATE, sizeof(net_svcl_round_state), {}, {}},
			{NET_SVCL_CHANGE_GAME_TYPE, sizeof(net_svcl_change_game_type), {}, {}},
			{NET_SVCL_MAP_CHANGE, sizeof(net_svcl_map_change), {}, {END(net_svcl_map_change, mapName)}},
			{NET_SVCL_PICKUP_ITEM, sizeof(net_svcl_pickup_item), {F(net_svcl_pickup_item, playerID, Slot), F(net_svcl_pickup_item, itemFlag, Weapon)}, {}},
			{NET_SVCL_MAP_LIST, sizeof(net_svcl_map_list), {}, {END(net_svcl_map_list, mapName)}},
			{NET_CLSV_SVCL_PLAYER_UPDATE_SKIN, sizeof(net_clsv_svcl_player_update_skin), {F(net_clsv_svcl_player_update_skin, playerID, Slot)}, {END(net_clsv_svcl_player_update_skin, skin)}},
		};
#undef F
#undef END

		bool valid(const std::vector<char> & m, const Field & f)
		{
			if (f.kind == ChunkSize || f.kind == Index16)
			{
				int16_t v;
				memcpy(&v, &m[f.offset], 2);
				if (f.kind == ChunkSize) return (uint16_t)v <= 250;
				return v >= 0;
			}
			int v = (int8_t)m[f.offset];
			switch (f.kind)
			{
			case Slot: return v >= 0 && v < MAX_PLAYER;
			case SlotOrNone: return v >= -1 && v < MAX_PLAYER;
			case Team: return v >= PLAYER_TEAM_SPECTATOR && v <= PLAYER_TEAM_AUTO_ASSIGN;
			case Weapon: return knownWeapon(v);
			case ShotWeapon: return knownWeapon(v) || v == WEAPON_MINIBOT_WEAPON;
			case ProjectileWeapon: return knownWeapon(v) || v == -1 || v == -2;
			case Flag: return v == 0 || v == 1;
			case FlagState: return v >= -2 && v < MAX_PLAYER;
			default: return false;
			}
		}
	}

	const ClientRule * clientRule(int typeID)
	{
		for (const ClientRule & r : clientRules)
			if (r.typeID == typeID) return &r;
		return 0;
	}

	bool knownWeapon(int weaponID)
	{
		return weaponID >= 0 && weaponID < (int)(sizeof(gameVar.weapons) / sizeof(gameVar.weapons[0])) && gameVar.weapons[weaponID];
	}

	bool checkServerMessage(const char * buffer, int size, int typeID, std::vector<char> & out)
	{
		const ServerRule * rule = 0;
		for (const ServerRule & r : serverRules)
			if (r.typeID == typeID) rule = &r;
		if (!rule || (rule->size < 0 ? size < 1 : size != rule->size)) return false;
		out.assign(buffer, buffer + size);
		if (rule->size < 0) out.push_back('\0');
		for (const Field & f : rule->fields)
			if (!valid(out, f)) return false;
		for (int end : rule->ends) out[end] = '\0';
		return true;
	}
}
