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

#ifndef NETPACKET_H
#define NETPACKET_H



#include "cMSstruct.h"
#include <stdint.h>

// Wire format: fixed-width fields, no padding, little endian (asserted below). Changing a layout needs a GAME_VERSION bump.
#pragma pack(push, 1)

// Quand le client recois un ping, il renvoit un pong
#define NET_CLSV_PONG 1
struct net_clsv_pong
{
	int8_t playerID; // Le ID du joueur concern�
//	char bidon[31];
};

// Le client est pret � spawner (apparaitre)
// Il va envoyer une request au server, et ce dernier va d�cider O� il spawn
#define NET_CLSV_SPAWN_REQUEST 2
struct net_clsv_spawn_request
{
	int8_t playerID; // Le ID du joueur concern�
	int8_t weaponID; // Le ID du gun avec lequel spawner
	int8_t meleeID;

	// Skin info
	char skin[7];

	//--- Les couleurs custom du babo
	uint8_t redDecal[3];
	uint8_t greenDecal[3];
	uint8_t blueDecal[3];
};

// Le client tire du fusil (activit� commune chez les cocassien)
// Le server et les clients connaissent le fusil utilis� par le joueur, donc pas besoin de l'envoyer
#define NET_CLSV_PLAYER_SHOOT 3
struct net_clsv_player_shoot
{
	int8_t playerID; // Le ID du joueur
	int8_t weaponID; // Le ID du type de gun
	int8_t nuzzleID; // le ID du nuzzle du fusil qui l'a tir�
	int16_t p1[3]; // Le point du d�but du ray
	int16_t p2[3]; // Le point de la fin du ray
};

// La version du server est accept� par le client
#define NET_CLSV_GAMEVERSION_ACCEPTED 4
struct net_clsv_gameversion_accepted
{
	int8_t playerID;		// Le ID du joueur
	char password[16];	// Password
};

// On demande au server de pickuper un item par terre
#define NET_CLSV_PICKUP_REQUEST 5
struct net_clsv_pickup_request
{
	int8_t playerID; // Le ID du joueur en question
};

// On demande au server d'�tre admin!
#define NET_CLSV_ADMIN_REQUEST 6
struct net_clsv_admin_request
{
	char login[33];		//md5
	char password[33];	//md5
};

// Vote
#define NET_CLSV_VOTE 7
struct net_clsv_vote
{
	bool value;
	int8_t playerID;
};

// On map list request
#define NET_CLSV_MAP_LIST_REQUEST 8
struct net_clsv_map_list_request
{
	int8_t playerID;
	bool all;
};

// Le server accept une new connection, il envoit � tout le monde le ID du joueur
#define NET_SVCL_NEWPLAYER 101
struct net_svcl_newplayer
{
	int8_t newPlayerID; // Le ID du nouveau Joueur (de 0 � 31)
	int32_t baboNetID;
};

// Le server envoit ses info aux nouveaux clients
// Suivi de �a, il va lui envoyer la liste de tout les joueurs
// � partir de ce moment l�, le joueur devra recevoir tout les messages du server
// Pour garder l'�tat du server � jour
#define NET_SVCL_SERVER_INFO 102
struct net_svcl_server_info
{
	int32_t mapSeed; // Le seed de la map, pour le random
	char mapName[16]; // 15 + '\0'

	// Le type de parti
	int8_t gameType;

	// Les score
	int16_t blueScore;
	int16_t redScore;
	int16_t blueWin;
	int16_t redWin;
};

// Le server fou le camp, il le dit � tout le monde (en moins quil plante l�)
#define NET_SVCL_SERVER_DISCONNECT 103
/*struct net_svcl_server_disconnect
{
	// Pas grand truc � mettre ici ! Me semble que le message est clair ;)
};*/

// Le client fou le camp, le server le sait tout suite, et le shoot au autres 
// (en moins quil plante l�)
#define NET_SVCL_PLAYER_DISCONNECT 104
struct net_svcl_player_disconnect
{
	int8_t playerID; // Le ID du joueur
};

// Le client fou le camp, le server le sait tout suite, et le shoot au autres 
// (en moins quil plante l�)
#define NET_SVCL_PLAYER_ENUM_STATE 105
struct net_svcl_player_enum_state
{
	int8_t playerID; // Le ID du joueur
	char playerName[31+1]; // Le nom du joueur, 31 + \0 caract�res
	int8_t teamID; // Son team
	int8_t status; // Son status
	int16_t kills;
	int16_t deaths;
	int16_t score; // Son score
	int16_t returns;
	int16_t flagAttempts;
	int16_t damage;
	float life; // Sa vie
	float dmg;
	int8_t weaponID; // Le gun qu'il a
	char playerIP[16];
	int32_t babonetID;

	// Skin info
	char skin[7];

	//--- Les couleurs custom du babo
	uint8_t redDecal[3];
	uint8_t greenDecal[3];
	uint8_t blueDecal[3];
};

// Le server envoit un ping � toute les seconde � tout les joueurs
#define NET_SVCL_PING 106
struct net_svcl_ping
{
	int8_t playerID; // Le ID du joueur concerv�
//	char bidon[31];
};

// Le server envoit � chaque seconde le ping de toute le monde � tout le monde
#define NET_SVCL_PLAYER_PING 107
struct net_svcl_player_ping
{
	int8_t playerID; // Le ID du joueur
	int16_t ping; // Son ping avec le server, en miliseconde
};

// Le server re�ois la request de spawner du joueur, et renvoit � TOUT le monde
// sa position de spawn
#define NET_SVCL_PLAYER_SPAWN 108
struct net_svcl_player_spawn
{
	int8_t playerID; // Le ID du joueur
	int8_t weaponID; // Le ID du gun avec lequel spawner
	int8_t meleeID; // Le ID du melee gun avec lequel spawner
	int16_t position[3]; // La position o� il spawn

	// Skin info
	char skin[7];

	//--- Les couleurs custom du babo
	uint8_t redDecal[3];
	uint8_t greenDecal[3];
	uint8_t blueDecal[3];
};

// Le server modifie une variable sv_, il va l'envoyer � tout le monde
// Les sv ce sont les seuls qui affectent le gameplay et que le server garde l'exclusivit�e
#define NET_SVCL_SV_CHANGE 109
struct net_svcl_sv_change
{
	char svChange[79+1]; // 80 c'est assez
};

// Un client a tir�, le server a effectu� la collision et renvoi le r�sultat aux autres joueurs
#define NET_SVCL_PLAYER_SHOOT 110
struct net_svcl_player_shoot
{
	int8_t playerID; // Le ID du joueur qui l'a tir�
	int8_t hitPlayerID; // Le ID du joueur qu'on a touch�, si -1 on a touch� un mur
	int8_t nuzzleID; // le ID du nuzzle du fusil qui l'a tir� (pour savoir o� spawner le feu)
	int8_t weaponID; // Le ID du type de gun
	int16_t p1[3]; // Le point du d�but du ray
	int16_t p2[3]; // Le point de la fin du ray (point d'impact)
	char normal[3]; // La normal de l'impact
};

// Pour dire qu'on supprime un projectile
#define NET_SVCL_DELETE_PROJECTILE 111
struct net_svcl_delete_projectile
{
	int32_t projectileID; // Son ID dans le vector
};

// La position d'un projectile (uniquement control� par le server)
#define NET_SVCL_PROJECTILE_COORD_FRAME 112
struct net_svcl_projectile_coord_frame
{
	int32_t uniqueID; // Le ID unique du projectile
	int16_t projectileID; // Le ID du projectile concern�
	int32_t frameID; // Le frame auquel �a a �t� envoy� (on en a de besoin pour cr�er des belles interpolations)
	int16_t position[3]; // Sa position
	char vel[3]; // Sa velocity
//	int32_t uniqueProjectileID;
	// Sa rotation sur l'axe est calcul� c�t� client, vu que c uniquement visuel
};

// Pour spawner une explosion
#define NET_SVCL_EXPLOSION 113
struct net_svcl_explosion
{
	float position[3]; // La position de l'explosion dans map
	float normal[3]; // L'orientation de l'explosion
	float radius; // Sa puissance !! (�a va aussi faire shaker la vue :P)
	int8_t playerID;
	// On ne dit pas qui l'a provoqu� et tout, c'est le server qui va faire les hits
};

// Si on a touch� un joueur l'hors d'une explosion par exemple
#define NET_SVCL_PLAYER_HIT 114
struct net_svcl_player_hit
{
	int8_t playerID; // Le joueur touch�
	int8_t fromID; // De qui �a vient
	int8_t weaponID; // Le type d'arme utilis�
	float damage; // ne pas oublier le damage inflig� ! ** New, la vie restante **
	char vel[3]; // La velocity qu'on recoit par le coup
};

// Le server emet un son et veut que les clients le jou
#define NET_SVCL_PLAY_SOUND 115
struct net_svcl_play_sound
{
	int8_t soundID;
	uint8_t volume;
	int8_t range;
	uint8_t position[3];
};

// La version du server
#define NET_SVCL_GAMEVERSION 116
struct net_svcl_gameversion
{
	uint32_t gameVersion;
};

// Pour synchroniser le temps des horloges du jeu
#define NET_SVCL_SYNCHRONIZE_TIMER 117
struct net_svcl_synchronize_timer
{
	int32_t frameID;
	float gameTimeLeft;
	float roundTimeLeft;
};

// Pour Changer l'�tat d'un flag
#define NET_SVCL_CHANGE_FLAG_STATE 118
struct net_svcl_change_flag_state
{
	int8_t flagID; // 0 ou 1
	int8_t newFlagState; // Son nouvel �tat
	int8_t playerID; // Le ID du player qui a effectu� l'action
};

// Un joueur est mort ou disconnect�, il laisse tomber le flag
// Le server communique alors la position exacte aux autres players
#define NET_SVCL_DROP_FLAG 119
struct net_svcl_drop_flag
{
	int8_t flagID;
	float position[3];
};

// Un joueur join la game, on send juste le enum state
#define NET_SVCL_FLAG_ENUM 120
struct net_svcl_flag_enum
{
	int8_t flagState[2];
	float positionBlue[3];
	float positionRed[3];
};

// On change le state du round
#define NET_SVCL_GAME_STATE 121
struct net_svcl_round_state
{
	int8_t newState;
	int8_t reInit; // Pour restarter le round � neuf ou en parti (trace de sang, vie, etc)
};

// On change le type de game
#define NET_SVCL_CHANGE_GAME_TYPE 122
struct net_svcl_change_game_type
{
	int8_t newGameType;
};

// Le server change de map, il le dit aux autres !
#define NET_SVCL_MAP_CHANGE 123
struct net_svcl_map_change
{
	char mapName[16]; // 15 + '\0'
	int8_t gameType; // Le type dla game
};

// Un joueur ramasse un item, on le dit � tout le monde
#define NET_SVCL_PICKUP_ITEM 124
struct net_svcl_pickup_item
{
	int8_t playerID;
	int8_t itemType;
	int8_t itemFlag;
};

// Un joueur passe sur une flame, la flame se colle sur lui
#define NET_SVCL_FLAME_STICK_TO_PLAYER 125
struct net_svcl_flame_stick_to_player
{
	int16_t projectileID; // Le ID unique du projectile
	int8_t playerID; // Le ID du joueur sur qui �a stick
};

// La console envoit les messages console aux admin
#define NET_SVCL_CONSOLE 126

// Le server accept le user/pass du admin
#define NET_SVCL_ADMIN_ACCEPTED 127

// Le server averti qu'il va y avoir un auto-balance dans 15secondes.
#define NET_SVCL_AUTOBALANCE 128

// Le server met fin au vote
#define NET_SVCL_END_VOTE 129

// Le server update le voting status
#define NET_SVCL_UPDATE_VOTE 130
struct net_svcl_update_vote
{
	int8_t nbYes;
	int8_t nbNo;
};

// Le server shoot le r�sultat des votes
#define NET_SVCL_VOTE_RESULT 131
struct net_svcl_vote_result
{
	bool passed;
};

#define NET_SVCL_MSG 132
struct net_svcl_msg
{
	int8_t msgDest; // where msg should be displayed, 0 - chat
	int8_t teamID; // -2 - all, -1 - spectators, 1 - blue, 2 - red
	char message[49+80+1]; // Null terminated string. De 79 + \0 caract�res
};

#define NET_SVCL_PLAYER_UPDATE_STATS 133
struct net_svcl_player_update_stats
{
	int8_t playerID;
	int16_t kills;
	int16_t deaths;
	int16_t score; // Son score
	int16_t returns;
	int16_t flagAttempts;
	float timePlayedCurGame;
};

// Le client recois son ID, il envoit ses info (player name, etc), 
// et le server le renvois aux autres
#define NET_CLSV_SVCL_PLAYER_INFO 201
struct net_clsv_svcl_player_info
{
	int8_t playerID; // Le ID du joueur
	char playerIP[16];
	char playerName[31+1]; // Le nom du joueur, 31 + \0 caract�res
	char username[21];		// Account username
	char password[32];		// Account password (MD5)
	char macAddr[20]; // player's mac adress, mouhouha
};

// Le client �cris un message, l'envoit au server
#define NET_CLSV_SVCL_CHAT 202
struct net_clsv_svcl_chat
{
	int8_t teamID; // -1 for all, >= 0 for team ID
	char message[49+80+1]; // Null terminated string. De 79 + \0 caract�res
};

// Le client veux changer de team, il le demande d'abords au server
#define NET_CLSV_SVCL_TEAM_REQUEST 203
struct net_clsv_svcl_team_request
{
	int8_t playerID; // Son ID
	int8_t teamRequested; // L'etat quil demande
};

// La position du joueur
#define NET_CLSV_SVCL_PLAYER_COORD_FRAME 204
struct net_clsv_svcl_player_coord_frame
{
	int8_t playerID; // Le ID du joueur concern�
	int32_t frameID; // Le frame auquel �a a �t� envoy� (on en a de besoin pour cr�er des belles interpolations)
//	float angle; // Par o� il regarde
	int16_t position[3]; // Sa position
	char vel[3]; // Sa velocity
	int16_t mousePos[3]; // La position o� il vise
	int32_t babonetID;
	int32_t camPosZ;
	// Son orientation sera calcul� client side, vu que c pas full important c une boule
};

//--- mini bot creation
#define NET_SVCL_CREATE_MINIBOT 1001
struct net_svcl_create_minibot
{
	int8_t playerID; //--- Player ID owning that bot
	int16_t position[3]; //--- Bot position
	int16_t mousePos[3]; //--- Where it aims
};

#define NET_SVCL_MINIBOT_COORD_FRAME 1002
struct net_svcl_minibot_coord_frame
{
	int8_t playerID; // Le ID du joueur concern�
	int32_t frameID; // Le frame auquel �a a �t� envoy� (on en a de besoin pour cr�er des belles interpolations)
//	float angle; // Par o� il regarde
	int16_t position[3]; // Sa position
	char vel[3]; // Sa velocity
	int16_t mousePos[3]; // La position o� il vise
	int32_t babonetID; // ?? don't need that
};


// On change le nom du joueur pendant le round (devra attendre la fin, ou au prochain round)
// Techniquement �a ne devrait pas �tre allow�, mais �a va �tre hot
#define NET_CLSV_SVCL_PLAYER_CHANGE_NAME 205
struct net_clsv_svcl_player_change_name
{
	int8_t playerID; // Le ID du joueur
	char playerName[31+1]; // Le nom du joueur, 31 + \0 caract�res
};

// Le player shoot un projectile, on demande au server de cr�er l'instance
#define NET_CLSV_SVCL_PLAYER_PROJECTILE 206
struct net_clsv_svcl_player_projectile
{
	int8_t playerID; // Le ID du joueur
	int8_t weaponID; // Le ID du type de gun qui a shoot� le projectile
	int8_t nuzzleID; // Le ID du nuzzle du fusil qui a l'a tir�
	int8_t projectileType; // Le type du projectile
	int16_t position[3]; // La position initial du projectile
	char vel[3]; // La velocit�e initial du projectile
	int32_t uniqueID;
//	int32_t uniqueProjectileID;
};

// Le player shoot avec son melee
#define NET_CLSV_SVCL_PLAYER_SHOOT_MELEE 207
struct net_clsv_svcl_player_shoot_melee
{
	int8_t playerID;
};

// On request un vote
#define NET_CLSV_SVCL_VOTE_REQUEST 208
struct net_clsv_svcl_vote_request
{
	char vote[79+1]; // La commande
	int8_t playerID; // Le player ID
};

// On request map
#define NET_CLSV_MAP_REQUEST 209
struct net_clsv_map_request
{
	char mapName[16]; // 15 + '\0'
	uint32_t uniqueClientID;
};

// On request map
#define NET_SVCL_MAP_CHUNK 210
struct net_svcl_map_chunk
{
	uint16_t	size;
	char			data[250]; //250 bytes chunks
};

// On map list request
#define NET_SVCL_MAP_LIST 211
struct net_svcl_map_list
{
	char mapName[16]; // 15 + '\0'
};

// Le player shoot avec son melee
#define NET_CLSV_SVCL_PLAYER_UPDATE_SKIN 212
struct net_clsv_svcl_player_update_skin
{
	int8_t playerID;

	// Skin info
	char skin[7];

	//--- Les couleurs custom du babo
	uint8_t redDecal[3];
	uint8_t greenDecal[3];
	uint8_t blueDecal[3];
};

// BROADCAST MESSAGE
#define UNIQUE_BV2_KEY 3333113333
#define NET_BROADCAST -1
#define NET_BROADCAST_QUERY 301
#define NET_BROADCAST_GAME_INFO 302

struct net_clsv_broadcast_query
{
	char key[12];	//unique bv2 key for broadcasting
};

struct net_svcl_broadcast_game_info
{
	char		key[16];	//unique bv2 key for broadcasting
	stBV2row	GameInfo;	
};



static_assert(sizeof(net_clsv_pong) == 1, "net_clsv_pong wire size");
static_assert(sizeof(net_clsv_spawn_request) == 19, "net_clsv_spawn_request wire size");
static_assert(sizeof(net_clsv_player_shoot) == 15, "net_clsv_player_shoot wire size");
static_assert(sizeof(net_clsv_gameversion_accepted) == 17, "net_clsv_gameversion_accepted wire size");
static_assert(sizeof(net_clsv_pickup_request) == 1, "net_clsv_pickup_request wire size");
static_assert(sizeof(net_clsv_admin_request) == 66, "net_clsv_admin_request wire size");
static_assert(sizeof(net_clsv_vote) == 2, "net_clsv_vote wire size");
static_assert(sizeof(net_clsv_map_list_request) == 2, "net_clsv_map_list_request wire size");
static_assert(sizeof(net_svcl_newplayer) == 5, "net_svcl_newplayer wire size");
static_assert(sizeof(net_svcl_server_info) == 29, "net_svcl_server_info wire size");
static_assert(sizeof(net_svcl_player_disconnect) == 1, "net_svcl_player_disconnect wire size");
static_assert(sizeof(net_svcl_player_enum_state) == 92, "net_svcl_player_enum_state wire size");
static_assert(sizeof(net_svcl_ping) == 1, "net_svcl_ping wire size");
static_assert(sizeof(net_svcl_player_ping) == 3, "net_svcl_player_ping wire size");
static_assert(sizeof(net_svcl_player_spawn) == 25, "net_svcl_player_spawn wire size");
static_assert(sizeof(net_svcl_sv_change) == 80, "net_svcl_sv_change wire size");
static_assert(sizeof(net_svcl_player_shoot) == 19, "net_svcl_player_shoot wire size");
static_assert(sizeof(net_svcl_delete_projectile) == 4, "net_svcl_delete_projectile wire size");
static_assert(sizeof(net_svcl_projectile_coord_frame) == 19, "net_svcl_projectile_coord_frame wire size");
static_assert(sizeof(net_svcl_explosion) == 29, "net_svcl_explosion wire size");
static_assert(sizeof(net_svcl_player_hit) == 10, "net_svcl_player_hit wire size");
static_assert(sizeof(net_svcl_play_sound) == 6, "net_svcl_play_sound wire size");
static_assert(sizeof(net_svcl_gameversion) == 4, "net_svcl_gameversion wire size");
static_assert(sizeof(net_svcl_synchronize_timer) == 12, "net_svcl_synchronize_timer wire size");
static_assert(sizeof(net_svcl_change_flag_state) == 3, "net_svcl_change_flag_state wire size");
static_assert(sizeof(net_svcl_drop_flag) == 13, "net_svcl_drop_flag wire size");
static_assert(sizeof(net_svcl_flag_enum) == 26, "net_svcl_flag_enum wire size");
static_assert(sizeof(net_svcl_round_state) == 2, "net_svcl_round_state wire size");
static_assert(sizeof(net_svcl_change_game_type) == 1, "net_svcl_change_game_type wire size");
static_assert(sizeof(net_svcl_map_change) == 17, "net_svcl_map_change wire size");
static_assert(sizeof(net_svcl_pickup_item) == 3, "net_svcl_pickup_item wire size");
static_assert(sizeof(net_svcl_flame_stick_to_player) == 3, "net_svcl_flame_stick_to_player wire size");
static_assert(sizeof(net_svcl_update_vote) == 2, "net_svcl_update_vote wire size");
static_assert(sizeof(net_svcl_vote_result) == 1, "net_svcl_vote_result wire size");
static_assert(sizeof(net_svcl_msg) == 132, "net_svcl_msg wire size");
static_assert(sizeof(net_svcl_player_update_stats) == 15, "net_svcl_player_update_stats wire size");
static_assert(sizeof(net_clsv_svcl_player_info) == 122, "net_clsv_svcl_player_info wire size");
static_assert(sizeof(net_clsv_svcl_chat) == 131, "net_clsv_svcl_chat wire size");
static_assert(sizeof(net_clsv_svcl_team_request) == 2, "net_clsv_svcl_team_request wire size");
static_assert(sizeof(net_clsv_svcl_player_coord_frame) == 28, "net_clsv_svcl_player_coord_frame wire size");
static_assert(sizeof(net_svcl_create_minibot) == 13, "net_svcl_create_minibot wire size");
static_assert(sizeof(net_svcl_minibot_coord_frame) == 24, "net_svcl_minibot_coord_frame wire size");
static_assert(sizeof(net_clsv_svcl_player_change_name) == 33, "net_clsv_svcl_player_change_name wire size");
static_assert(sizeof(net_clsv_svcl_player_projectile) == 17, "net_clsv_svcl_player_projectile wire size");
static_assert(sizeof(net_clsv_svcl_player_shoot_melee) == 1, "net_clsv_svcl_player_shoot_melee wire size");
static_assert(sizeof(net_clsv_svcl_vote_request) == 81, "net_clsv_svcl_vote_request wire size");
static_assert(sizeof(net_clsv_map_request) == 20, "net_clsv_map_request wire size");
static_assert(sizeof(net_svcl_map_chunk) == 252, "net_svcl_map_chunk wire size");
static_assert(sizeof(net_svcl_map_list) == 16, "net_svcl_map_list wire size");
static_assert(sizeof(net_clsv_svcl_player_update_skin) == 17, "net_clsv_svcl_player_update_skin wire size");
static_assert(sizeof(net_clsv_broadcast_query) == 12, "net_clsv_broadcast_query wire size");
static_assert(sizeof(net_svcl_broadcast_game_info) == 148, "net_svcl_broadcast_game_info wire size");

#pragma pack(pop)

#endif

