/*
 * Copyright (C) 2008-2008 LeGACY <http://www.legacy-project.org/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "Common.h"
#include "Database/DatabaseEnv.h"
#include "Log.h"
#include "Opcodes.h"
#include "WorldSocket.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Player.h"
#include "ObjectMgr.h"
#include "World.h"
#include "NameTables.h"
#include "MapManager.h"
#include "ObjectAccessor.h"
#include "BattleSystem.h"

/// WorldSession constructor
WorldSession::WorldSession(uint32 id, WorldSocket *sock, uint32 sec) :
	_socket(sock), _security(sec), _accountId(id), _player(NULL), m_playerLoading(false), m_playerLogout(false), m_playerRecentlyLogout(false)
{
	FillOpcodeHandlerHashTable();
}

/// WorldSession destructor
WorldSession::~WorldSession()
{
	///- unload player if not unloaded
	if(_player)
		LogoutPlayer(true);
	
	///- if have unclosed socket, close it
	if(_socket)
		_socket->CloseSocket();

	_socket = NULL;

	///- empty incoming packet queue
	while(!_recvQueue.empty())
	{
		WorldPacket *packet = _recvQueue.next();
		delete packet;
	}
}

void WorldSession::FillOpcodeHandlerHashTable()
{
	sLog.outDetail("WorldSession: Load Opcode Handler Hash Table");
	// if table is already filled
	if (!objmgr.opcodeTable.empty())
		return;

	// fill table if empty
	objmgr.opcodeTable[ CMSG_AUTH_RESPONSE ] = OpcodeHandler(STATUS_NOT_LOGGEDIN,
			&WorldSession::HandlePlayerLoginOpcode );

	objmgr.opcodeTable[ CMSG_CHAR_CREATE ] = OpcodeHandler(STATUS_LOGGEDIN,
			&WorldSession::HandleCharCreateOpcode );

	objmgr.opcodeTable[ CMSG_PLAYER_MOVE ] = OpcodeHandler(STATUS_LOGGEDIN,
			&WorldSession::HandleMovementOpcodes );

	objmgr.opcodeTable[ CMSG_PLAYER_ACTION ] = OpcodeHandler(STATUS_LOGGEDIN,
			&WorldSession::HandlePlayerActionOpcode );

	objmgr.opcodeTable[ CMSG_PLAYER_ENTER_MAP_COMPLETED ] = OpcodeHandler(STATUS_LOGGEDIN,
			&WorldSession::HandlePlayerEnterMapCompletedOpcode );

	objmgr.opcodeTable[ CMSG_PLAYER_CHAT_MESSAGE ] = OpcodeHandler(STATUS_LOGGEDIN,
			&WorldSession::HandleMessagechatOpcode );

	objmgr.opcodeTable[ CMSG_PLAYER_EXPRESSION ] = OpcodeHandler(STATUS_LOGGEDIN,
			&WorldSession::HandlePlayerExpressionOpcode );

	objmgr.opcodeTable[ CMSG_UNKNOWN_14 ] = OpcodeHandler(STATUS_LOGGEDIN,
			&WorldSession::HandleUnknownRequest14Opcode );

	objmgr.opcodeTable[ CMSG_PLAYER_ATTACK ] = OpcodeHandler(STATUS_LOGGEDIN,
			&WorldSession::HandlePlayerAttackOpcode );

	objmgr.opcodeTable[ CMSG_PET_COMMAND ] = OpcodeHandler(STATUS_LOGGEDIN,
			&WorldSession::HandlePetCommandOpcodes );

	objmgr.opcodeTable[ CMSG_PET_COMMAND_RELEASE ] = OpcodeHandler(STATUS_LOGGEDIN,
			&WorldSession::HandlePetReleaseOpcode );

	objmgr.opcodeTable[ CMSG_USE_INVENTORY_ITEM ] = OpcodeHandler(STATUS_LOGGEDIN, 
			&WorldSession::HandleUseItemOpcodes );

	objmgr.opcodeTable[ CMSG_GROUP_COMMAND ] = OpcodeHandler(STATUS_LOGGEDIN,
			&WorldSession::HandleGroupOpcodes );

	objmgr.opcodeTable[ CMSG_PLAYER_STAT_ADD ] = OpcodeHandler(STATUS_LOGGEDIN,
			&WorldSession::HandlePlayerStatAddOpcodes );

	objmgr.opcodeTable[ CMSG_PLAYER_SPELL_ADD ] = OpcodeHandler(STATUS_LOGGEDIN,
			&WorldSession::HandlePlayerSpellAddOpcodes );

	objmgr.opcodeTable[ CMSG_PLAYER_TRANSAC_ITEM ] = OpcodeHandler(STATUS_LOGGEDIN,
			&WorldSession::HandlePlayerTransacItemOpcodes );

	objmgr.opcodeTable[ CMSG_PLAYER_BATTLE_COMMAND ] = OpcodeHandler(STATUS_LOGGEDIN,
			&WorldSession::HandlePlayerBattleCommandOpcodes );

	objmgr.opcodeTable[ CMSG_PLAYER_SETUP_SHORTKEY ] = OpcodeHandler(STATUS_LOGGEDIN,
			&WorldSession::HandlePlayerSetupShortkeyOpcodes );

	objmgr.opcodeTable[ CMSG_PLAYER_ENABLE_OPTION ] = OpcodeHandler(STATUS_LOGGEDIN,
			&WorldSession::HandlePlayerEnableOptionsOpcodes );
}

void WorldSession::SizeError(WorldPacket const& packet, uint32 size) const
{
	sLog.outError("Client (account %u) send packet %s (%u) with size %u but expected %u (attempt crash server?), skipped",
		GetAccountId(), LookupNameClient(packet.GetOpcode(), g_clntOpcodeNames),packet.GetOpcode(),packet.size(),size);
}


/// Set the WorldSocket associated with this session
void WorldSession::SetSocket(WorldSocket *sock)
{
	_socket = sock;
}

/// Add an incoming packet to the queue
void WorldSession::QueuePacket(WorldPacket& packet)
{
	WorldPacket *pck = new WorldPacket(packet);
	_recvQueue.add(pck);
}

/// Send a packet to the client
void WorldSession::SendPacket(WorldPacket* packet, bool log)
{
	if(!_socket)
		return;

	#ifdef LEGACY_DEBUG
	// Code for network use statistic
	#endif

	_socket->SendPacket(packet, log);
}

/// Update the WorldSession (triggered by World update)
bool WorldSession::Update(uint32 /*diff*/)
{
	WorldPacket *packet;
	WorldPacket  data(1);
	while(!_recvQueue.empty())
	{
		//sLog.outString("WorldSession::Update processing queue");
		packet = _recvQueue.next();

		OpcodeTableMap::const_iterator iter = objmgr.opcodeTable.find( packet->GetOpcode() );

		if (iter == objmgr.opcodeTable.end())
		{
			//sLog.outError( "SESSION: received unhandled opcode %s (0x%.2X)",
			//	LookupNameClient(packet->GetOpcode(), g_clntOpcodeNames),
			//	packet->GetOpcode());

				//GetPlayer()->EndOfRequest();
		}
		else
		{
			if (iter->second.status == STATUS_NOT_LOGGEDIN)
			{
				//DEBUG_LOG("STATUS_NOT_LOGGEDIN");
				(this->*iter->second.handler)(*packet);
			}
			else if (iter->second.status == STATUS_LOGGEDIN && _player)
			{
				//DEBUG_LOG("STATUS_LOGGEDIN && _player");
				(this->*iter->second.handler)(*packet);
			} else if (iter->second.status == STATUS_LOGGEDIN) {
				m_playerRecentlyLogout = false;
				(this->*iter->second.handler)(*packet);
			}
			else
				// skip STATUS_LOGGEDIN opcode unexpected errors if player logout sometime ago - this can be network lag delayed packets
			if(!m_playerRecentlyLogout)
			{
				sLog.outError( "SESSION: received unexpected opcode %s (0x%.2X)",
				LookupNameClient(packet->GetOpcode(), g_clntOpcodeNames),
				packet->GetOpcode());

				//GetPlayer()->EndOfRequest();
					
			}
			
		}
		delete packet;
	}

	///- If necessray, log the player out
	time_t currTime = time(NULL);
	//if (!_socket || (ShouldLogOut(currTime) && !m_playerLoading))
	if (!_socket && !m_playerLoading )
	{
		/*
		///- If Battle Master, find new battle master
		if( GetPlayer()->isBattleInProgress() )
		{
			sLog.outDebug("SESSION: '%s' is battle master, find new master", GetPlayer()->GetName());
			if( !GetPlayer()->PlayerBattleClass->FindNewMaster() )
			{
				delete GetPlayer()->PlayerBattleClass;
				GetPlayer()->PlayerBattleClass = NULL;
			}

		}

		///- Leave from battle if any
		GetPlayer()->LeaveBattle();
		*/
		LogoutPlayer(true);
		return false;
	}

	//if (!_socket)
	//	return false;  // Will remove this session from the world session map

	return true;

}

///- Log the player out
void WorldSession::LogoutPlayer(bool Save)
{
	sLog.outDebug("SESSION: Logging out Player");
	m_playerLogout = true;

	///- Reset the online field in the account table
	//No SQL injection as AccountId is uint32
	loginDatabase.PExecute("UPDATE accounts SET online = 0 WHERE accountid = '%u'", GetAccountId());

	if(Save && _player)
	{
		_player->SaveToDB();
	}

	///- Remove the player from the world
	if( _player )
	{
		ObjectAccessor::Instance().RemoveObject(_player);
		MapManager::Instance().GetMap(_player->GetMapId(), _player)->Remove(_player, false);

		delete _player;
		_player = NULL;
	}

	CharacterDatabase.PExecute("UPDATE characters SET online = 0 WHERE accountid = '%u'", GetAccountId());
	sLog.outDebug( "SESSION: Send SMSG_LOGOUT_COMPLETE Message" );

	m_playerLogout = false;
	m_playerRecentlyLogout = true;
	LogoutRequest(0);

}

void WorldSession::SetLogging(bool newvalue)
{
	if( _socket == NULL )
		return;

	_socket->SetLogging(newvalue);
}
