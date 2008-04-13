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
#include "ObjectMgr.h"
#include "World.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Player.h"

#include "GossipDef.h"
#include "BattleSystem.h"

#include "MapManager.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "ObjectMgr.h"
#include "ObjectAccessor.h"
#include "Util.h"
#include "Database/DatabaseImpl.h"

#include <cmath>

Player::Player (WorldSession *session): Unit( 0 )
{
	m_session = session;

	m_objectType |= TYPE_PLAYER;
	m_objectTypeId = TYPEID_PLAYER;

	m_valuesCount = PLAYER_END;

	m_GMFlags = 0;

	memset(m_items, 0, sizeof(Item*)*PLAYER_SLOTS_COUNT);

	memset(m_pets, 0, sizeof(Pet*)*MAX_PET_SLOT);

	//memset(m_spells, 0, sizeof(Spell*)*MAX_PLAYER_SPELL);

	m_battlePet = NULL;

	m_nextSave = sWorld.getConfig(CONFIG_INTERVAL_SAVE);

	// randomize first save time in range [CONFIG_INTERVAL_SAVE] around [CONFIG_INTERVAL_SAVE]
	// this must help in case next save after mass player load after server startup
	m_nextSave = rand32(m_nextSave/2,m_nextSave*3/2);

	m_dontMove = false;

	PlayerTalkClass = new PlayerMenu( GetSession() );
	//PlayerBattleClass = new BattleSystem( GetSession() );
	PlayerBattleClass = NULL;
	i_battleMaster = NULL;

	m_talkedSequence = 0;
	m_talkedCreatureGuid = 0;

	m_leaderGuid = 0;
}

Player::~Player ()
{
	///- if battle in progress, if in group assign battle engine to others
	if( isBattleInProgress() )
	{
		sLog.outDebug("PLAYER: '%s' is battle master, find new master", GetName());
		if( !PlayerBattleClass->FindNewMaster() )
		{
			delete PlayerBattleClass;
			PlayerBattleClass = NULL;
		}

		/*
		if( isTeamLeader() && !m_team.empty() )
		{
			Player* pteam = m_team.front();

			pteam->PlayerBattleClass = PlayerBattleClass;
		}
		*/
	}

	///- Leave from battle if any
	LeaveBattle();

	///- Leave from team if joined
	if(m_leaderGuid)
	{
		Player* leader = ObjectAccessor::FindPlayer(MAKE_GUID(m_leaderGuid, HIGHGUID_PLAYER));
		if( leader )
		{
			leader->LeaveTeam(this);
		}
	}

	///- Dismiss Team if team leader
	//for(TeamList::const_iterator itr = m_team.begin(); itr != m_team.end(); ++itr)
	while( !m_team.empty() )
	{
		Player* pteam = m_team.front();
		sLog.outDebug("PLAYER: '%s' as leader, dismiss team for '%s'", GetName(), pteam->GetName());
		LeaveTeam((pteam));
	}

	if( m_session && m_session->PlayerLogout() )
	{
		sLog.outDebug( "WORLD: Player '%s' is logged out", GetName());
		WorldPacket data;
		data.Initialize( 0x01 );
		data << (uint8 ) 0x01;
		data << GetAccountId();
		SendMessageToSet(&data, false);
	}
}

bool Player::Create( uint32 accountId, WorldPacket& data)
{
	uint8 element, gender, skin, face, hair, hairColor;

	Object::_Create(accountId, HIGHGUID_PLAYER);

	data >> m_name;
	data >> element;
	data >> gender;
	// later on
	
	return true;
}

void Player::Update( uint32 p_time )
{
	if(!IsInWorld())
		return;

	Unit::Update( p_time );

	if( isBattleInProgress() )
	{
		if( PlayerBattleClass->isActionTimedOut() )
		{
			PlayerBattleClass->BuildActions();
		}

		PlayerBattleClass->UpdateBattleAction();

		///- After UpdateBattleAction, possible the battle is ended
		if( PlayerBattleClass->BattleConcluded() )
		{
			PlayerBattleClass->BattleStop();
			delete PlayerBattleClass;
			PlayerBattleClass = NULL;
		}
	}

	if(m_nextSave > 0)
	{
		if(p_time >= m_nextSave)
		{
			// m_nextSave reseted in SaveToDB call
			SaveToDB();
	//		sLog.outDetail("Player '%u' '%s' Saved", GetAccountId(), GetName());
		}
		else
		{
			m_nextSave -= p_time;
		}
	}
}

void Player::SendMessageToSet(WorldPacket *data, bool self)
{
//	sLog.outString("Player::SendMessageToSet");
	MapManager::Instance().GetMap(GetMapId(), this)->MessageBroadcast(this, data, self);
}

bool Player::LoadFromDB( uint32 guid, SqlQueryHolder *holder)
{
	QueryResult *result = holder->GetResult(PLAYER_LOGIN_QUERY_LOADFROM);
	
	if(!result)
	{
		sLog.outError("ERROR: Player (GUID: %u) not found in table `characters`,can't load.", guid);
		return false;
	}

	Field *f = result->Fetch();

	uint32 dbAccountId = f[1].GetUInt32();

	// check if the character's account in the db and the logged in account match.
	// player should be able to load/delete only with correct account!
	if( dbAccountId != GetSession()->GetAccountId() )
	{
		sLog.outError("ERROR: Player (GUID: %u) loading from wrong account (is: %u, should be: %u)", guid, GetSession()->GetAccountId(),dbAccountId);
		delete result;
		return false;
	}

	Object::_Create( guid, HIGHGUID_PLAYER );

	// overwrite possible wrong/corrupted guid
	SetUInt64Value(OBJECT_FIELD_GUID,MAKE_GUID(guid,HIGHGUID_PLAYER));

	// cleanup inventory related item value fields (its will be filled correctly in _LoadInvetory
	for(uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
	{
		if (m_items[slot])
		{
			delete m_items[slot];
			m_items[slot] = NULL;
		}
	}

	m_name = f[FD_CHARNAME].GetCppString();

	sLog.outDebug(">> Load Basic value of player %s is: ", m_name.c_str());

	m_element = f[FD_ELEMENT].GetUInt8();
	SetUInt8Value(UNIT_FIELD_ELEMENT, f[FD_ELEMENT].GetUInt8());
	m_gender  = f[FD_GENDER].GetUInt8();
	m_reborn  = f[FD_REBORN].GetUInt8();

	Relocate(f[FD_POSX].GetUInt16(), f[FD_POSY].GetUInt16());

	SetMapId(f[FD_MAPID].GetUInt16());

	m_face       = f[FD_FACE].GetUInt8();
	m_hair       = f[FD_HAIR].GetUInt8();
	m_hair_color_R = f[FD_HAIR_COLOR_R].GetUInt8();
	m_hair_color_G = f[FD_HAIR_COLOR_G].GetUInt8();
	m_hair_color_B = f[FD_HAIR_COLOR_B].GetUInt8();
	m_skin_color_R = f[FD_SKIN_COLOR_R].GetUInt8();
	m_skin_color_G = f[FD_SKIN_COLOR_G].GetUInt8();
	m_skin_color_B = f[FD_SKIN_COLOR_B].GetUInt8();
	m_shirt_color = f[FD_SHIRT_COLOR].GetUInt8();
	m_misc_color = f[FD_MISC_COLOR].GetUInt8();

	m_hp        = f[FD_HP].GetUInt16();
	SetUInt16Value(UNIT_FIELD_HP, f[FD_HP].GetUInt16());
	m_sp        = f[FD_SP].GetUInt16();
	SetUInt16Value(UNIT_FIELD_SP, f[FD_SP].GetUInt16());

	m_stat_int  = f[FD_ST_INT].GetUInt16();
	SetUInt16Value(UNIT_FIELD_INT, f[FD_ST_INT].GetUInt16());

	m_stat_atk  = f[FD_ST_ATK].GetUInt16();
	SetUInt16Value(UNIT_FIELD_ATK, f[FD_ST_ATK].GetUInt16());

	m_stat_def  = f[FD_ST_DEF].GetUInt16();
	SetUInt16Value(UNIT_FIELD_DEF, f[FD_ST_DEF].GetUInt16());

	m_stat_agi  = f[FD_ST_AGI].GetUInt16();
	SetUInt16Value(UNIT_FIELD_AGI, f[FD_ST_AGI].GetUInt16());

	m_stat_hpx  = f[FD_ST_HPX].GetUInt16();
	SetUInt16Value(UNIT_FIELD_HPX, f[FD_ST_HPX].GetUInt16());

	m_stat_spx  = f[FD_ST_SPX].GetUInt16();
	SetUInt16Value(UNIT_FIELD_SPX, f[FD_ST_SPX].GetUInt16());

	m_level     = f[FD_LEVEL].GetUInt8();
	SetUInt8Value(UNIT_FIELD_LEVEL, f[FD_LEVEL].GetUInt8());

	m_rank      = f[FD_RANK].GetUInt8();

	m_xp_gain   = f[FD_XP_GAIN].GetUInt32();
	SetUInt32Value(UNIT_FIELD_XP, f[FD_XP_GAIN].GetUInt32());

	SetUInt32Value(UNIT_FIELD_NEXT_LEVEL_XP, f[FD_XP_TNL].GetUInt32());

	m_skill     = f[FD_SKILL_GAIN].GetUInt16();
	SetUInt16Value(UNIT_FIELD_SPELL_POINT, f[FD_SKILL_GAIN].GetUInt16());

	m_stat      = f[FD_STAT_GAIN].GetUInt16();
	SetUInt16Value(UNIT_FIELD_STAT_POINT, f[FD_STAT_GAIN].GetUInt16());

	///- TODO: Calculate from equipment weared
	m_atk_mod   = 0;
	m_def_mod   = 0;
	m_int_mod   = 0;
	m_agi_mod   = 0;
	m_hpx_mod   = 0;
	m_spx_mod   = 0;

	///- Calculation must be after hpx & spx modifier applied
	m_hp_max    = ((f[FD_ST_HPX].GetUInt16() + m_hpx_mod) * 4) + 80 + m_level;
	SetUInt16Value(UNIT_FIELD_HP_MAX, ((f[FD_ST_HPX].GetUInt16() + m_hpx_mod) * 4) + 80 + GetUInt8Value(UNIT_FIELD_LEVEL));
	m_sp_max    = ((f[FD_ST_SPX].GetUInt16() + m_spx_mod) * 2) + 60 + m_level;
	SetUInt16Value(UNIT_FIELD_SP_MAX, ((f[FD_ST_SPX].GetUInt16() + m_spx_mod) * 4) + 80 + GetUInt8Value(UNIT_FIELD_LEVEL));

	m_gold_hand = f[FD_GOLD_IN_HAND].GetUInt32();
	m_gold_bank = f[FD_GOLD_IN_BANK].GetUInt32();

	m_unk1      = f[FD_UNK1].GetUInt16();
	m_unk2      = f[FD_UNK2].GetUInt16();
	m_unk3      = f[FD_UNK3].GetUInt16();
	m_unk4      = f[FD_UNK4].GetUInt16();
	m_unk5      = f[FD_UNK5].GetUInt16();



	_LoadPet(holder->GetResult(PLAYER_LOGIN_QUERY_LOADPET));
	_LoadInventory(holder->GetResult(PLAYER_LOGIN_QUERY_LOADINVENTORY)); // must be called after _LoadPet, for equiped Pet equipments

	_LoadSpell(holder->GetResult(PLAYER_LOGIN_QUERY_LOADSPELL));

	sLog.outDebug(" - Name    : %s", GetName());
	sLog.outDebug(" - Level   : %u", m_level);
	sLog.outDebug(" - Element : %u", m_element);
	sLog.outDebug(" - HP & SP : %u/%u %u/%u", m_hp, m_hp_max, m_sp, m_sp_max);
	sLog.outDebug(" - Map Id  : %u [%u,%u]", GetMapId(), GetPositionX(), GetPositionY());

	//DumpPlayer();
	return true;
}

void Player::DumpPlayer(const char* section)
{
	sLog.outDebug("");
	if( section == "equip" )
		for(uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; slot++)
			if(m_items[slot])
				sLog.outDebug(" @@ Equipment slot %3u equiped [%-20.20s] (%u piece)", slot, m_items[slot]->GetProto()->Name, m_items[slot]->GetCount());

	if( section == "inventory" )
		for(uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
			if (m_items[slot])
				sLog.outDebug(" @@ Inventory slot %2u contain [%-20.20s] (%2u piece) GUID(%u)", slot, m_items[slot]->GetProto()->Name, m_items[slot]->GetCount(), m_items[slot]->GetGUIDLow());

	if( section == "spell" )
		for(SpellMap::iterator it = m_spells.begin(); it != m_spells.end(); ++it)
			sLog.outDebug( " ** Player '%s' has spell <%-20.20s> level [%2u]", GetName(), (*it).second->GetProto()->Name, (*it).second->GetLevel());

	if( section == "pet" )
		for(uint8 slot = 0; slot < MAX_PET_SLOT; slot++)
			if( m_pets[slot] )
			{
				sLog.outDebug(" @@ Pet slot %3u '%s' is %s", slot + 1, m_pets[slot]->GetName(), (m_pets[slot]->isBattle() ? "Battle" : "Resting"));
				m_pets[slot]->DumpPet();
			}

	sLog.outDebug("");
}

bool Player::_LoadInventory(QueryResult* result)
{
	if( result )
	{
		do
		{
			Field *f = result->Fetch();
			uint32 pet_guid  = f[1].GetUInt32();
			uint8  slot      = f[2].GetUInt8();
			uint32 item_guid = f[3].GetUInt32();
			uint32 item_id   = f[4].GetUInt32();

			sLog.outDebug("STORAGE: Loading inventory slot %u, item_guid %u, item_id %u", slot, item_guid, item_id);

			ItemPrototype const * proto = objmgr.GetItemPrototype(item_id);

			if(!proto)
			{
				sLog.outError("STORAGE: Player %s has an unknown item (id: #%u) in inventory, deleted.", GetName(), item_id );
				//CharacterDatabase.PExecute("DELETE FROM character_inventory WHERE item = '%u'", item_guid);
				//CharacterDatabase.PExecute("DELETE FROM item_instance WHERE guid = '%u'", item_guid);
				continue;
			}

			Item *item = new Item;

			if(!item->LoadFromDB(item_guid, GetGUID(), result))
			{
				sLog.outError("STORAGE: Player %s has broken item (id: #%u) in inventory, deleted.", GetName(), item_id);
				//CharacterDatabase.PExecute("DELETE FROM character_inventory WHERE item = '%u'", item_guid);
				//CharacterDatabase.PExecute("DELETE FROM item_instance WHERE guid = '%u'", item_guid);
				//item->FSetState(ITEM_REMOVED);
				item->SaveToDB();       // it alse deletes item object !
				continue;
			}

			///- TODO not allow have in perticular state

			bool success = true;

			if (!pet_guid)
			{
				// the item is not equiped by pet
				item->SetContainer( NULL );
				item->SetSlot(slot);

				// destinated position
				uint8 dest = slot;

				if( IsInventoryPos( dest ) )
				{
					if( CanStoreItem( slot, dest, item, false ) == EQUIP_ERR_OK )
						item = StoreItem(dest, item, true);
					else
						success = false;
				}
				else if( IsEquipmentPos( dest ) )
				{
					if( CanEquipItem( slot, dest, item, false, false ) == EQUIP_ERR_OK )
						QuickEquipItem(dest, item);
					else
						success = false;
				}
				else if( IsBankPos( dest ) )
				{
					if( CanBankItem( slot, dest, item, false, false ) == EQUIP_ERR_OK )
						item = BankItem(dest, item, true);
					else
						success = false;
				}

				//sLog.outDebug("STORAGE: _LoadInventory Destination slot for %u is %u", item_guid, dest);
			}
			else
			{
				Pet* pet = GetPetByGuid(pet_guid);

				if(!pet)
				{
					sLog.outDebug("STORAGE: Player %s has item (id: %u) belongs to Pet, but can't be equipped to pet, deleted.", GetName(), item_id );
					//CharacterDatabase.PExecute("DELETE FROM character_inventory WHERE item = '%u'", item_guid);
					//CharacterDatabase.PExecute("DELETE FROM item_instance WHERE guid = '%u'", item_guid);
					//item->FSetState(ITEM_REMOVED);
					//item->SaveToDB();       // it alse deletes item object !
					continue;
				}
				uint8 dest = slot;
				// the item is used by pet, equip it to pet
				if( IsEquipmentPos( dest ) )
				{
					if( CanEquipItem( slot, dest, item, false, false ) == EQUIP_ERR_OK )
						QuickPetEquipItem(pet_guid, dest, item);
					else
						success = false;
				}
				else
				{
					//CharacterDatabase.PExecute("DELETE FROM character_inventory WHERE item = '%u'", item_guid);
					//CharacterDatabase.PExecute("DELETE FROM item_instance WHERE guid = '%u'", item_guid);
					sLog.outDebug("STORAGE: Player %s has item (id: %u) belongs to Pet, but can't be equipped to pet, deleted.", GetName(), item_id );
					success = false;
				}
			}
				
			// item's state may have changed after stored
			if( success )
				item->SetState(ITEM_UNCHANGED, this);
			else
			{
				sLog.outError("STORAGE: Player %s has item (GUID: %u Entry: %u) can't be loaded to inventory (Slot: %u) by some reason", GetName(), item_guid, item_id, slot);
				//CharacterDatabase.PExecute("DELETE FROM character_inventory WHERE item = '%u'", item_guid);
				//problematicItems.push_back(item);
			}

		} while (result->NextRow());

		delete result;

	}

	_ApplyAllItemMods();
				
}

void Player::_ApplyAllItemMods()
{
	sLog.outDebug("MODSTAT: _ApplyAllItemMods start.");

}

void Player::SaveToDB()
{
	// delay auto save at any saves (manual, in code, or autosave)
	m_nextSave = sWorld.getConfig(CONFIG_INTERVAL_SAVE);

	CharacterDatabase.BeginTransaction();
	CharacterDatabase.PExecute("UPDATE characters SET mapid = '%u', pos_x = '%u', pos_y = '%u' WHERE accountid = '%u'", GetMapId(), GetPositionX(), GetPositionY(), GetAccountId());

	//std::ostringstream ss;
	//ss << "INSERT INTO characters (guid, accountid, charname, mapid, pos_x, pos_y, online
	CharacterDatabase.CommitTransaction();
}

bool Player::_LoadPet(QueryResult* result)
{
	bool delete_result = false;
	if( !result )
	{
		QueryResult *result = CharacterDatabase.PQuery("SELECT id, petslot FROM character_pet WHERE owner = '%u' ORDER BY petslot", GetGUIDLow());
		delete_result = true;
	}

	if( !result )
		return false;

	uint8 count = 0;
	do
	{
		Field *f = result->Fetch();
		uint32 petguid = f[0].GetUInt32();
		uint8  slot    = f[1].GetUInt8();

		if( slot > MAX_PET_SLOT )
			continue;

		Pet *pet = new Pet(this);
		if(!pet->LoadPetFromDB(this, petguid))
		{
			delete pet;
			continue;
		}

		count++;

		m_pets[slot] = pet;

		if( pet->isBattle() )
			m_battlePet = pet;

		//sLog.outString(" - Slot %u pet Entry %u Model %u '%s' is %s", slot + 1, pet->GetEntry(), pet->GetModelId(), pet->GetName(), pet->isBattle() ? "Battle" : "Resting"); 

		if(count > MAX_PET_SLOT)
			break;

	} while( result->NextRow() );

	if (delete_result) delete result;

	return true;
}

bool Player::_LoadSpell(QueryResult* result)
{
	bool delete_result = false;
	if( !result )
	{
		result = CharacterDatabase.PQuery("SELECT entry, level FROM character_spell WHERE owner = '%u' ORDER BY entry", GetGUIDLow());
		delete_result = true;
	}

	if( !result )
		return false;

	uint8 count = 0;
	do
	{
		Field *f = result->Fetch();
		uint16 entry = f[0].GetUInt16();
		uint8  level = f[1].GetUInt8();

		if( !AddSpell(entry, level) )
		{
			sLog.outError("SPELL: Player %s has an invalid spell (id: #%u), deleted.", GetName(), entry );
			continue;
		}

		count++;

		if(count > MAX_PLAYER_SPELL)
			break;

	} while( result->NextRow() );

	if (delete_result) delete result;

	return true;
}

void Player::UpdateBattlePet()
{
	//sLog.outDebug("PLAYER: Update Battle Pet");
	uint16 modelid = 0;

	if( m_battlePet )
		modelid = m_battlePet->GetModelId();

	WorldPacket data;
	data.Initialize( 0x13 );
	data << (uint8 ) (modelid ? 0x01 : 0x02);
	data << (uint16) modelid;
	data << (uint16) 0;

	if( m_session )
		m_session->SendPacket(&data);
}

void Player::SetBattlePet(Pet* pet)
{
	m_battlePet = pet;
}

Pet* Player::GetPetByGuid(uint32 pet_guid) const
{
	for(uint8 slot = 0; slot < MAX_PET_SLOT; slot++)
	{
		if( !m_pets[slot] )
			continue;

		if( m_pets[slot]->GetGUIDLow() == pet_guid )
			return m_pets[slot];
	}
	return NULL;
}

Pet* Player::GetPetByModelId(uint16 modelid) const
{
	for(uint8 slot = 0; slot < MAX_PET_SLOT; slot++)
	{
		if( !m_pets[slot] )
			continue;

		if( m_pets[slot]->GetModelId() == modelid )
			return m_pets[slot];
	}
	return NULL;
}

uint8 Player::GetPetSlot(Pet* pet) const
{
	for(uint8 slot = 0; slot < MAX_PET_SLOT; slot++)
	{
		if( !m_pets[slot] )
			continue;

		if( m_pets[slot] == pet )
			return slot + 1;
	}

	return 0;
}

/*Spell* Player::GetSpellById(uint16 entry) const
{
}*/

void Player::BuildUpdateBlockVisibilityPacket(WorldPacket *data)
{
	data->Initialize( 0x03, 1 );

	*data << GetAccountId();
	*data << m_gender;
	*data << (uint16) 0x0000; // unknown fix from aming
	*data << GetMapId();
	*data << GetPositionX();
	*data << GetPositionY();

	*data << (uint8) 0x00;
	*data << m_hair;
	*data << m_face;
	*data << m_hair_color_R;
	*data << m_hair_color_G;
	*data << m_hair_color_B;
	*data << m_skin_color_R;
	*data << m_skin_color_G;
	*data << m_skin_color_B;
	*data << m_shirt_color;
	*data << m_misc_color;

	uint8  equip_cnt = 0;
	uint16 equip[EQUIPMENT_SLOT_END];

	if (m_items[EQUIPMENT_SLOT_HEAD])
		equip[equip_cnt++] = m_items[EQUIPMENT_SLOT_HEAD]->GetModelId();

	if (m_items[EQUIPMENT_SLOT_BODY])
		equip[equip_cnt++] = m_items[EQUIPMENT_SLOT_BODY]->GetModelId();

	if (m_items[EQUIPMENT_SLOT_WEAPON])
		equip[equip_cnt++] = m_items[EQUIPMENT_SLOT_WEAPON]->GetModelId();

	if (m_items[EQUIPMENT_SLOT_WRISTS])
		equip[equip_cnt++] = m_items[EQUIPMENT_SLOT_WRISTS]->GetModelId();

	if (m_items[EQUIPMENT_SLOT_FEET])
		equip[equip_cnt++] = m_items[EQUIPMENT_SLOT_FEET]->GetModelId();

	if (m_items[EQUIPMENT_SLOT_SPECIAL])
		equip[equip_cnt++] = m_items[EQUIPMENT_SLOT_SPECIAL]->GetModelId();

	*data << (uint8 ) equip_cnt;
	for(uint8 i = 0; i < equip_cnt; i++) *data << equip[i];

	// unknown
	*data << (uint16) 0x00 << (uint16) 0x00 << (uint16) 0x00;
	*data << m_reborn;
	*data << m_name.c_str();

}

void Player::BuildUpdateBlockVisibilityForOthersPacket(WorldPacket *data)
{
	data->Initialize( 0x03, 1 );
	*data << GetAccountId();
	*data << m_gender;
	*data << m_element;
	*data << m_level;
	*data << (uint16) 0x0000;
	*data << GetMapId();
	*data << GetPositionX();
	*data << GetPositionY();
	*data << (uint8) 0x00;// << (uint16) 0x0000;
	*data << m_hair;
	*data << m_face;
	*data << m_hair_color_R;
	*data << m_hair_color_G;
	*data << m_hair_color_B;
	*data << m_skin_color_R;
	*data << m_skin_color_G;
	*data << m_skin_color_B;
	*data << m_shirt_color;
	*data << m_misc_color;

	uint8  equip_cnt = 0;
	uint16 equip[EQUIPMENT_SLOT_END];

	if (m_items[EQUIPMENT_SLOT_HEAD])
		equip[equip_cnt++] = m_items[EQUIPMENT_SLOT_HEAD]->GetModelId();

	if (m_items[EQUIPMENT_SLOT_BODY])
		equip[equip_cnt++] = m_items[EQUIPMENT_SLOT_BODY]->GetModelId();

	if (m_items[EQUIPMENT_SLOT_WEAPON])
		equip[equip_cnt++] = m_items[EQUIPMENT_SLOT_WEAPON]->GetModelId();

	if (m_items[EQUIPMENT_SLOT_WRISTS])
		equip[equip_cnt++] = m_items[EQUIPMENT_SLOT_WRISTS]->GetModelId();

	if (m_items[EQUIPMENT_SLOT_FEET])
		equip[equip_cnt++] = m_items[EQUIPMENT_SLOT_FEET]->GetModelId();

	if (m_items[EQUIPMENT_SLOT_SPECIAL])
		equip[equip_cnt++] = m_items[EQUIPMENT_SLOT_SPECIAL]->GetModelId();

	*data << (uint8 ) equip_cnt;
	for(uint8 i = 0; i < equip_cnt; i++) *data << equip[i];

	*data << (uint32) 0x00000000 << (uint16) 0x0000; // << (uint8) 0x02;
	*data << (uint8 ) m_reborn; //0x00;
	*data << (uint8 ) 0x00;
	*data << m_name.c_str();

}

void Player::SendInitialPacketsBeforeAddToMap()
{
	WorldPacket data;

	BuildUpdateBlockVisibilityPacket(&data);
	GetSession()->SendPacket(&data);

	UpdatePetCarried();
	UpdatePet();
	UpdateInventory();
	UpdateBattlePet();

	BuildUpdateBlockStatusPacket(&data);
	GetSession()->SendPacket(&data);

	//SendMotd();

	AllowPlayerToMove();
	SendUnknownImportant();
}

void Player::SendInitialPacketsAfterAddToMap()
{
	UpdatePlayer();
	UpdateCurrentGold();
}

void Player::UpdatePetCarried()
{
	WorldPacket data;

	data.Initialize( 0x0F );
	data << (uint8 ) 0x08;
	uint8 petcount = 0;
	for(uint8 slot = 0; slot < MAX_PET_SLOT; slot++)
	{
		Pet* pet = m_pets[slot];
		if( !pet )
			continue;

		petcount++;
		data << (uint8 ) petcount;
		data << (uint16) pet->GetModelId(); // pet npc id ;
		data << (uint32) pet->GetExpGained();//0; // pet total xp 0x010D9A19

		data << (uint8 ) pet->GetUInt8Value(UNIT_FIELD_LEVEL); // pet level

		data << pet->GetUInt16Value(UNIT_FIELD_HP);  // pet current hp
		data << pet->GetUInt16Value(UNIT_FIELD_SP);  // pet current sp
		data << pet->GetUInt16Value(UNIT_FIELD_INT); // pet base stat int
		data << pet->GetUInt16Value(UNIT_FIELD_ATK); // pet base stat atk
		data << pet->GetUInt16Value(UNIT_FIELD_DEF); // pet base stat def
		data << pet->GetUInt16Value(UNIT_FIELD_AGI); // pet base stat agi
		data << pet->GetUInt16Value(UNIT_FIELD_HPX); // pet base stat hpx
		data << pet->GetUInt16Value(UNIT_FIELD_SPX); // pet base stat spx

		data << (uint8 ) 5;    // stat point left ?
		data << (uint8 ) pet->GetLoyalty(); // pet loyalty
		data << (uint8 ) 0x01; // unknown
		data << (uint16) pet->GetSkillPoint(); //skill point

		std::string tmpname = pet->GetName();
		data << (uint8 ) tmpname.size(); //name length
		data << tmpname.c_str();

		data << (uint8 ) pet->GetSpellLevelByPos(0); //level skill #1
		data << (uint8 ) pet->GetSpellLevelByPos(1); //level skill #2
		data << (uint8 ) pet->GetSpellLevelByPos(2); //level skill #3

		uint8 durability = 0xFF;
		//data << (uint16) pet->GetUInt16Value(UNIT_FIELD_EQ_HEAD);
		data << (uint16) pet->GetEquipModelId(EQUIPMENT_SLOT_HEAD);
		data << (uint8 ) durability;
		data << (uint32) 0 << (uint16) 0 << (uint8 ) 0; // unknown 7 byte

		//data << (uint16) pet->GetUInt16Value(UNIT_FIELD_EQ_BODY);
		data << (uint16) pet->GetEquipModelId(EQUIPMENT_SLOT_BODY);
		data << (uint8 ) durability;
		data << (uint32) 0 << (uint16) 0 << (uint8 ) 0; // unknown 7 byte

		//data << (uint16) pet->GetUInt16Value(UNIT_FIELD_EQ_WRIST);
		data << (uint16) pet->GetEquipModelId(EQUIPMENT_SLOT_WRISTS);
		data << (uint8 ) durability;
		data << (uint32) 0 << (uint16) 0 << (uint8 ) 0; // unknown 7 byte

		//data << (uint16) pet->GetUInt16Value(UNIT_FIELD_EQ_WEAPON);
		data << (uint16) pet->GetEquipModelId(EQUIPMENT_SLOT_WEAPON);
		data << (uint8 ) durability;
		data << (uint32) 0 << (uint16) 0 << (uint8 ) 0; // unknown 7 byte

		//data << (uint16) pet->GetUInt16Value(UNIT_FIELD_EQ_SHOE);
		data << (uint16) pet->GetEquipModelId(EQUIPMENT_SLOT_FEET);
		data << (uint8 ) durability;
		data << (uint32) 0 << (uint16) 0 << (uint8 ) 0; // unknown 7 byte

		//data << (uint16) pet->GetUInt16Value(UNIT_FIELD_EQ_SPECIAL);
		data << (uint16) pet->GetEquipModelId(EQUIPMENT_SLOT_SPECIAL);
		data << (uint8 ) durability;
		data << (uint32) 0 << (uint16) 0 << (uint8 ) 0; // unknown 7 byte

		data << (uint16) 0x00;
	}

	if( !petcount )
		return;

	GetSession()->SendPacket(&data);
}

void Player::BuildUpdateBlockStatusPacket(WorldPacket *data)
{

	data->Initialize( 0x05, 1 );
	*data << (uint8) 0x03;                   // unknown
	*data << m_element; // uint8
	
	*data << m_hp; // uint16
	*data << m_sp; // uint16

	*data << (uint16) m_stat_int; //uint16
	*data << (uint16) m_stat_atk; //uint16
	*data << (uint16) m_stat_def; //uint16
	*data << (uint16) m_stat_agi; //uint16
	*data << (uint16) m_stat_hpx; //uint16
	*data << (uint16) m_stat_spx; //uint16

	*data << (uint8 ) getLevel(); //m_level; // uint8
	*data << (uint32) GetExpGained(); //m_xp_gain; //uint32
	*data << (uint16) m_skill; //uint16
	*data << (uint16) m_stat; // uint16

	*data << (uint32) 0; //m_rank;

	*data << (uint16) m_hp_max; // alogin use 32bit, while eXtreme use 16bit
	*data << (uint16) m_sp_max;
	*data << (uint16) 0;

	*data << (uint32) m_atk_mod;
	*data << (uint32) m_def_mod;
	*data << (uint32) m_int_mod;
	*data << (uint32) m_agi_mod;
	*data << (uint32) m_hpx_mod;
	*data << (uint32) m_spx_mod;

	*data << m_unk1; // this is Dong Wu moral 
	*data << m_unk2;
	*data << m_unk3;
	*data << m_unk4;
	*data << m_unk5;

	*data << (uint16) 0x00;
	*data << (uint16) 0x0000; //0x0101; // auto attack data ?

	*data << (uint32) 0x00 << (uint32) 0x00 << uint32(0x00) << uint32(0x00);
	*data << (uint32) 0x00 << (uint32) 0x00 << uint32(0x00) << uint32(0x00);
	*data << (uint32) 0x00 << (uint8 ) 0x00;

	///- Try to identify skill info packet
	/*
	*data << (uint16) 0x0000 << (uint8 ) 0x00;
	*data << (uint16) 11001; // Submerge
	*data << (uint8 ) 10;
	*data << (uint16) 14001; // Investigation
	*data << (uint8 ) 1;
	*/

	/*
	for( uint8 i = 0; i < MAX_PLAYER_SPELL; i++)
	{
		if(!m_spells[i])
			break;

		*data << (uint16) m_spells[i]->GetEntry();
		*data << (uint8 ) m_spells[i]->GetLevel();
	}
	*/
	for(SpellMap::iterator itr = m_spells.begin(); itr != m_spells.end(); ++itr)
	{
		*data << (uint16) (*itr).second->GetEntry();
		*data << (uint8 ) (*itr).second->GetLevel();
	}
}

void Player::UpdatePlayer()
{

	///- TODO: Update only when changes
	//_updatePlayer( 0x1B, 1, m_stat_int );
	//_updatePlayer( 0x1C, 1, m_stat_atk );
	//_updatePlayer( 0x1D, 1, m_stat_def );
	//_updatePlayer( 0x1E, 1, m_stat_agi );
	//_updatePlayer( 0x1F, 1, m_stat_hpx );
	//_updatePlayer( 0x20, 1, m_stat_spx );

	//_updatePlayer( 0x23, 1, m_level );
	//_updatePlayer( 0x24, 1, m_xp_gain );
	//_updatePlayer( 0x25, 1, m_skill );
	//_updatePlayer( 0x26, 1, m_stat );

	//_updatePlayer( 0x40, 1, 100 ); // loyalty


	///- This always updated before engaging
	_updatePlayer( 0x19, 1, GetUInt16Value(UNIT_FIELD_HP)); //m_hp );
	_updatePlayer( 0x1A, 1, GetUInt16Value(UNIT_FIELD_SP)); //m_sp );

	_updatePlayer( 0xCF, 1, m_hpx_mod );
	_updatePlayer( 0xD0, 1, m_spx_mod );
	_updatePlayer( 0xD2, 1, m_atk_mod );
	_updatePlayer( 0xD3, 1, m_def_mod );
	_updatePlayer( 0xD4, 1, m_int_mod );
	_updatePlayer( 0xD6, 1, m_agi_mod );
}

void Player::UpdateLevel()
{
	_updatePlayer( 0x23, 1, getLevel() );
	//_updatePlayer( 0x24, 1, GetExpGained() );
	_updatePlayer( 0x25, 1, GetUInt16Value(UNIT_FIELD_SPELL_POINT));
	_updatePlayer( 0x26, 1, GetUInt16Value(UNIT_FIELD_STAT_POINT));
	resetLevelUp();
}

void Player::UpdatePetLevel(Pet* pet)
{
	uint8 slot = GetPetSlot(pet);

	_updatePet(slot, 0x23, 1, pet->getLevel());
	pet->resetLevelUp();
}

void Player::_updatePlayer(uint8 updflag, uint8 modifier, uint16 value)
{
	//sLog.outDebug("PLAYER: Update player '%s' flag %3u value %u", GetName(), updflag, value);
	WorldPacket data;
	data.Initialize( 0x08 );
	data << (uint8 ) 0x01;            // flag status for main character
	data << (uint8 ) updflag;               // flag status fields
	data << (uint8 ) modifier;                 // +/- modifier
	data << (uint16) value;                    // value modifier
	data << (uint32) 0 << (uint16) 0;

	if( m_session )
		m_session->SendPacket(&data);
}

void Player::UpdatePet()
{
	for(uint8 slot = 0; slot < MAX_PET_SLOT; slot++)
	{
		Pet* pet = m_pets[slot];
		if( !pet )
			continue;
		_updatePet(slot+1, UPD_FLAG_HPX_MOD, 1, 0);
		_updatePet(slot+1, UPD_FLAG_HP, 1, pet->GetUInt16Value(UNIT_FIELD_HP));
		_updatePet(slot+1, UPD_FLAG_SPX_MOD, 1, 0);
		_updatePet(slot+1, UPD_FLAG_SP, 1, pet->GetUInt16Value(UNIT_FIELD_SP));
		_updatePet(slot+1, UPD_FLAG_ATK_MOD, 1, 0);
		_updatePet(slot+1, UPD_FLAG_DEF_MOD, 1, 0);
		_updatePet(slot+1, UPD_FLAG_INT_MOD, 1, 0);
		_updatePet(slot+1, UPD_FLAG_AGI_MOD, 1, 0);

	}
}

void Player::UpdatePet(uint8 slot)
{
	Pet* pet = m_pets[slot];
	if( !pet )
		return;
	_updatePet(slot, UPD_FLAG_HPX_MOD, 1, 0);
	_updatePet(slot, UPD_FLAG_HP, 1, pet->GetUInt16Value(UNIT_FIELD_HP));
	_updatePet(slot, UPD_FLAG_SPX_MOD, 1, 0);
	_updatePet(slot, UPD_FLAG_SP, 1, pet->GetUInt16Value(UNIT_FIELD_SP));
	_updatePet(slot, UPD_FLAG_ATK_MOD, 1, 0);
	_updatePet(slot, UPD_FLAG_DEF_MOD, 1, 0);
	_updatePet(slot, UPD_FLAG_INT_MOD, 1, 0);
	_updatePet(slot, UPD_FLAG_AGI_MOD, 1, 0);
}

void Player::UpdatePetBattle()
{
	Pet* bpet = GetBattlePet();
	for(uint8 slot = 0; slot < MAX_PET_SLOT; slot++)
		if( bpet == m_pets[slot+1])
		{
			UpdatePet(slot+1);
			break;
		}
}

void Player::_updatePet(uint8 slot, uint8 updflag, uint8 modifier, uint32 value)
{
	//sLog.outDebug("PLAYER: Update Pet slot %u flag %3u value %u", slot, updflag, value);
	WorldPacket data;
	data.Initialize( 0x08 );
	data << (uint8 ) 0x02 << (uint8 ) 0x04;
	data << (uint8 ) slot; // pet slot number
	data << (uint8 ) 0x00;
	data << (uint8 ) updflag;
	data << (uint8 ) modifier;
	data << (uint32) value;
	data << (uint32) 0;

	if( m_session )
		m_session->SendPacket(&data);
}

void Player::UpdateInventory()
{
	WorldPacket data;
	data.Initialize( 0x17 );
	data << (uint8 ) 0x05;
	uint8 count = 0;
	for(uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; slot++)
	{
		if( !m_items[slot] )
			continue;

		count++;
		data << (uint8 ) (slot - INVENTORY_SLOT_ITEM_START + 1);
		data << (uint16) m_items[slot]->GetProto()->modelid;
		data << (uint8 ) m_items[slot]->GetCount();
		data << (uint32) 0;
		data << (uint32) 0;
	}

	if( !count )
		return;

	if( m_session )
		m_session->SendPacket(&data);
}

void Player::UpdateCurrentEquip()
{
	WorldPacket data;
	data.Initialize( 0x05, 1 );
	data << (uint8) 0x00;
	data << GetAccountId();

	uint8  equip_cnt = 0;
	uint16 equip[EQUIPMENT_SLOT_END];

	if (m_items[EQUIPMENT_SLOT_HEAD])
		equip[equip_cnt++] = m_items[EQUIPMENT_SLOT_HEAD]->GetModelId();

	if (m_items[EQUIPMENT_SLOT_BODY])
		equip[equip_cnt++] = m_items[EQUIPMENT_SLOT_BODY]->GetModelId();

	if (m_items[EQUIPMENT_SLOT_WEAPON])
		equip[equip_cnt++] = m_items[EQUIPMENT_SLOT_WEAPON]->GetModelId();

	if (m_items[EQUIPMENT_SLOT_WRISTS])
		equip[equip_cnt++] = m_items[EQUIPMENT_SLOT_WRISTS]->GetModelId();

	if (m_items[EQUIPMENT_SLOT_FEET])
		equip[equip_cnt++] = m_items[EQUIPMENT_SLOT_FEET]->GetModelId();

	if (m_items[EQUIPMENT_SLOT_SPECIAL])
		equip[equip_cnt++] = m_items[EQUIPMENT_SLOT_SPECIAL]->GetModelId();

	for(uint8 i = 0; i < equip_cnt; i++) data << equip[i];

	if( m_session )
		m_session->SendPacket(&data);
}

void Player::UpdateCurrentGold()
{
	WorldPacket data;
	data.Initialize( 0x1A, 1 );
	data << (uint8 ) 0x04;
	data << (uint32) m_gold_hand; // gold
	data << (uint32) 0x00000000;

	if( m_session )
		m_session->SendPacket(&data);
}

void Player::Send0504()
{
	///- tell the client to wait for request
	WorldPacket data;
	data.Initialize( 0x05, 1 );
	data << (uint8) 0x04;
	
	if( m_session )
		m_session->SendPacket(&data);
}

void Player::Send0F0A()
{
	WorldPacket data;
	data.Initialize( 0x0F, 1 );
	data << (uint8) 0x0A;

	if( m_session )
		m_session->SendPacket(&data);
}

void Player::Send0602()
{
	WorldPacket data;
	data.Initialize( 0x06, 1 );
	data << (uint8) 0x02;

	if( m_session )
		m_session->SendPacket(&data);
}

void Player::Send1408()
{
	///- tell the client request is completed
	WorldPacket data;
	data.Initialize( 0x14, 1 );
	data << (uint8) 0x08;

	if( m_session )
		m_session->SendPacket(&data);
}

void Player::AllowPlayerToMove()
{
	Send0504(); Send0F0A(); //EndOfRequest();
}

void Player::EndOfRequest()
{
	Send1408();
}

void Player::BuildUpdateBlockTeleportPacket(WorldPacket* data)
{
	data->Initialize( 0x0C, 1 );
	*data << GetAccountId();
	*data << GetTeleportTo();
	*data << GetPositionX();
	*data << GetPositionY();
	*data << (uint8) 0x01 << (uint8) 0x00;
}


void Player::AddToWorld()
{
	///- Do not add/remove the player from the object storage
	///- It will crash when updating the ObjectAccessor
	///- The player should only be added when logging in
	Object::AddToWorld();

}

void Player::RemoveFromWorld()
{
	///- Do not add/remove the player from the object storage
	///- It will crash when updating the ObjectAccessor
	///- The player should only be removed when logging out
	Object::RemoveFromWorld();
}

void Player::SendDelayResponse(const uint32 ml_seconds)
{
}

void Player::UpdateVisibilityOf(WorldObject* target)
{
	/* this is working one */
	/*
	sLog.outString("Player::UpdateVisibilityOf '%s' to '%s'",
		GetName(), target->GetName());
	target->SendUpdateToPlayer(this);
	SendUpdateToPlayer((Player*) target);
	*/
	if(HaveAtClient(target))
	{
		//if(!target->isVisibleForInState(this, true))
		{
			//target->DestroyForPlayer(this);
			m_clientGUIDs.erase(target->GetGUID());

		}
	}
	else
	{
		//if(target->isVisibleForInState(this,false))
		{
			if(!target->isType(TYPE_PLAYER))
				return;

			target->SendUpdateToPlayer(this);
		}
	}
}

void Player::TeleportTo(uint16 mapid, uint16 pos_x, uint16 pos_y)
{
	sLog.outDebug( "PLAYER: Teleport Player '%s' to %u [%u,%u]", GetName(), mapid, pos_x, pos_y );

	WorldPacket data;

	SetDontMove(true);

	Map* map = MapManager::Instance().GetMap(GetMapId(), this);
	map->Remove(this, false);

	SetMapId(mapid);
	Relocate(pos_x, pos_y);

	// update player state for other and vice-versa
	CellPair p = LeGACY::ComputeCellPair(GetPositionX(), GetPositionY());
	Cell cell(p);
	MapManager::Instance().GetMap(GetMapId(), this)->EnsureGridLoadedForPlayer(cell, this, true);
}
	
void Player::SendMapChanged()
{
	WorldPacket data;
	data.Initialize( 0x0C, 1 );
	data << GetAccountId();
	data << GetMapId();
	data << GetPositionX();
	data << GetPositionY();
	data << (uint8) 0x01 << (uint8) 0x00;

	if( m_session )
		m_session->SendPacket(&data);


	Send0504();  // This is important, maybe tell the client to Wait Cursor

	Map* map = MapManager::Instance().GetMap(GetMapId(), this);
	map->Add(this);

	UpdateMap2Npc();

	SetDontMove(false);
}

void Player::UpdateRelocationToSet()
{
	///- Send Relocation Message to Set
	WorldPacket data;

	data.Initialize( 0x06, 1 );
	data << (uint8) 0x01;
	data << GetAccountId();
	data << (uint8) 0x05;
	data << GetPositionX() << GetPositionY();
	SendMessageToSet(&data, false);
}

bool Player::HasSpell(uint32 spell) const
{
	return false;
}

void Player::TalkedToCreature( uint32 entry, uint64 guid )
{
	m_talkedCreatureGuid = guid;    // store last talked to creature
	m_talkedSequence     = 0;       // reset sequence number
	PlayerTalkClass->CloseMenu();   // close any open menu
	PlayerTalkClass->InitTalking(); // send talk initial packet
}

uint8 Player::FindEquipSlot( ItemPrototype const* proto, uint32 slot, bool swap ) const
{
	//sLog.outDebug("STORAGE: FindEquipSlot '%s' slot %u", proto->Name, slot);
	uint8 slots;
	slots = NULL_SLOT;
	switch( proto->InventoryType )
	{
		case INVTYPE_HEAD:
			slots = EQUIPMENT_SLOT_HEAD;
			break;
		case INVTYPE_BODY:
			slots = EQUIPMENT_SLOT_BODY;
			break;
		case INVTYPE_WEAPON:
			slots = EQUIPMENT_SLOT_WEAPON;
			break;
		case INVTYPE_WRISTS:
			slots = EQUIPMENT_SLOT_WRISTS;
			break;
		case INVTYPE_FEET:
			slots = EQUIPMENT_SLOT_FEET;
			break;
		case INVTYPE_SPECIAL:
			slots = EQUIPMENT_SLOT_SPECIAL;
			break;

		default:
			return NULL_SLOT;
	}

	if( slot != NULL_SLOT )
	{
		if( swap || !GetItemByPos( slot ) )
		{
			if ( slots == slot )
			{
				//sLog.outString("STORAGE: FindEquipSlot '%s' is equipable in slot %u", proto->Name, slot);
				return slot;
			}
		}
	}
	else
	{
		if ( slots != NULL_SLOT && !GetItemByPos( slots ) )
		{
			return slots;
		}

		if ( slots != NULL_SLOT && swap )
			return slots;
	}

	sLog.outDebug("EQUIP: No Free Position");
	// no free position
	return NULL_SLOT;
}

Item* Player::CreateItem( uint32 item, uint32 count ) const
{
	ItemPrototype const *pProto = objmgr.GetItemPrototype( item );
	if( pProto )
	{
		sLog.outDebug("STORAGE: Item Prototype found <%u>, creating", item);
		Item *pItem = new Item;
		if ( count > pProto->Stackable )
			count = pProto->Stackable;
		if ( count < 1 )
			count = 1;
		if( pItem->Create(objmgr.GenerateLowGuid(HIGHGUID_ITEM), item, const_cast<Player*>(this)) )
		{
			sLog.outDebug("STORAGE: Item <%u> created", item);
			pItem->SetCount( count );
			return pItem;
		}
		else
			delete pItem;
	}
	else
		sLog.outDebug("STORAGE: Item Prototype not found <%u>", item);
	return NULL;
}

uint8 Player::CanUnequipItems( uint32 item, uint32 count ) const
{
/*
	Item *pItem;
	uint32 tempcount = 0;

	uint8 res = EQUIP_ERR_OK;

	for(int i = EQUIPMENT_SLOT_START; i < INVENTORY_SLOT_BAG_END; i++)
	{
		pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i );
		if( pItem && pItem->GetEntry() == item )
		{
			uint8 ires = CanUnequipItem(INVENTORY_SLOT_BAG_0 << 8 | i, false);
			if(ires==EQUIP_ERR_OK)
			{
				tempcount += pItem->GetCount();
				if( tempcount >= count )
					return EQUIP_ERR_OK;
			}
			else
				res = ires;
		}
	}
	for(int i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; i++)
	{
		pItem = GetItemByPos( INVENTORY_SLOT_BAG_0, i );
		if( pItem && pItem->GetEntry() == item )
		{
			tempcount += pItem->GetCount();
			if( tempcount >= count )
				return EQUIP_ERR_OK;
		}
	}
	
	// not found req. item count and have unequippable items
	return res;
*/
}

uint32 Player::GetItemCount( uint32 item, Item* eItem ) const
{
	Item *pItem;
	uint32 count = 0;
	for(int i = EQUIPMENT_SLOT_START; i < INVENTORY_SLOT_ITEM_END; i++)
	{
		pItem = GetItemByPos(i);
		if( pItem && pItem != eItem && pItem->GetEntry() == item )
			count += pItem->GetCount();
	}

	return count;
}

uint32 Player::GetBankItemCount( uint32 item, Item* eItem ) const
{
	Item *pItem;
	uint32 count = 0;
	for(int i = BANK_SLOT_ITEM_START; i < BANK_SLOT_ITEM_END; i++)
	{
		pItem = GetItemByPos(i);
		if( pItem && pItem != eItem && pItem->GetEntry() == item )
			count += pItem->GetCount();
	}
	return count;
}

Item* Player::GetItemByGuid( uint64 guid ) const
{
	for(int i = EQUIPMENT_SLOT_START; i < INVENTORY_SLOT_ITEM_END; i++)
	{
		Item *pItem = GetItemByPos( i );
		if( pItem && pItem->GetGUID() == guid )
			return pItem;
	}

	return NULL;
}

Item* Player::GetItemByPos( uint8 slot ) const
{
	if( m_items[slot] )
		return m_items[slot];
	else
		return NULL;
}

bool Player::IsInventoryPos( uint8 slot )
{
	if( slot == NULL_SLOT )
		return true;
	if( slot >= INVENTORY_SLOT_ITEM_START && slot < INVENTORY_SLOT_ITEM_END )
		return true;
	return false;
}

bool Player::IsEquipmentPos( uint8 slot )
{
	if( slot < EQUIPMENT_SLOT_END )
		return true;
	return false;
}

bool Player::IsBankPos( uint8 slot )
{
	if( slot >= BANK_SLOT_ITEM_START && slot < BANK_SLOT_ITEM_END )
		return true;
	return false;
}

bool Player::IsBagPos( uint8 slot )
{
	if( slot >= INVENTORY_SLOT_BAG_START && slot < INVENTORY_SLOT_BAG_END )
		return true;
	return false;
}

bool Player::HasItemCount( uint32 item, uint32 count ) const
{
	Item *pItem;
	uint32 tempcount = 0;
	for(int i = EQUIPMENT_SLOT_START; i < INVENTORY_SLOT_ITEM_END; i++)
	{
		pItem = GetItemByPos( i );
		if( pItem && pItem->GetEntry() == item )
		{
			tempcount += pItem->GetCount();
			if( tempcount >= count )
				return true;
		}
	}
	return false;
}

uint8 Player::CanStoreNewItem( uint8 slot, uint8 &dest, uint32 item, uint32 count, bool swap ) const
{
	dest = 0;
	Item *pItem = CreateItem( item, count );
	if( pItem )
	{
		uint8 result = CanStoreItem( slot, dest, pItem, swap );
		delete pItem;
		return result;
	}
	if( !swap )
		return EQUIP_ERR_ITEM_NOT_FOUND;
	else
		return EQUIP_ERR_ITEMS_CANT_BE_SWAPPED;
}

uint8 Player::CanTakeMoreSimilarItems(Item* pItem) const
{
	ItemPrototype const *pProto = pItem->GetProto();
	if( !pProto )
		return EQUIP_ERR_CANT_CARRY_MORE_OF_THIS;

	// no maximum
	if(pProto->MaxCount == 0)
		return EQUIP_ERR_OK;

	uint32 curcount = GetItemCount(pProto->ItemId,pItem) + GetBankItemCount(pProto->ItemId,pItem);

	if( curcount + pItem->GetCount() > pProto->MaxCount )
		return EQUIP_ERR_CANT_CARRY_MORE_OF_THIS;

	return EQUIP_ERR_OK;
}

uint8 Player::CanStoreItem( uint8 slot, uint8 &dest, Item *pItem, bool swap ) const
{
	dest = 0;
	if( pItem )
	{
		sLog.outDebug("STORAGE: CanStoreItem slot = %u, item = %u, count = %u", slot, pItem->GetEntry(), pItem->GetCount());
		ItemPrototype const *pProto = pItem->GetProto();
		if( pProto )
		{
			sLog.outDebug("STORAGE: CanStoreItem for '%s'", pProto->Name);
			Item *pItem2;
			uint16 pos;
			//if(pItem->IsBindedNotWith(GetGUID()))
			//	return EQUIP_ERR_DONT_OWN_THAT_ITEM;

			// check count of items (skip for auto move for same player from bank)
			uint8 res = CanTakeMoreSimilarItems(pItem);
			if(res != EQUIP_ERR_OK)
				return res;
			
			// search stack for merge to
			if( pProto->Stackable > 1 )
			{
				for(int i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; i++)
				{
					pos = i;
					pItem2 = GetItemByPos( pos );
					if( pItem2 && pItem2->GetEntry() == pItem->GetEntry() && pItem2->GetCount() + pItem->GetCount() <= pProto->Stackable )
					{
						sLog.outDebug("STORAGE: CanStoreItem still stackable %u <= %u", pItem2->GetCount() + pItem->GetCount(), pProto->Stackable);
						dest = pos;
						return EQUIP_ERR_OK;
					}
				}
			}

			// search free slot
			for(int i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; i++)
			{
				pItem2 = GetItemByPos( i );
				if( !pItem2 )
				{
					dest = i;
					return EQUIP_ERR_OK;
				}
			}

			sLog.outDebug("STORAGE: CanStoreItem Inventory is full");
			return EQUIP_ERR_INVENTORY_FULL;
		}
	}
	sLog.outDebug("STORAGE: CanStoreItem pItem is NULL");
	if( !swap )
		return EQUIP_ERR_ITEM_NOT_FOUND;
	else
		return EQUIP_ERR_ITEMS_CANT_BE_SWAPPED;
}

////////////////////////////////////////////////////////////////////////////
uint8 Player::CanStoreItems( Item **pItems, int count) const
{
	return EQUIP_ERR_OK;
}

///////////////////////////////////////////////////////////////////////////
uint8 Player::CanEquipNewItem( uint8 slot, uint8 &dest, uint32 item, uint32 count, bool swap ) const
{
	dest = 0;
	Item *pItem = CreateItem( item, count );
	if( pItem )
	{
		uint8 result = CanEquipItem(slot, dest, pItem, swap );
		delete pItem;
		return result;
	}

	return EQUIP_ERR_ITEM_NOT_FOUND;
}

uint8 Player::CanEquipItem( uint8 slot, uint8 &dest, Item *pItem, bool swap, bool not_loading ) const
{
	dest = 0;
	if( pItem )
	{
		//sLog.outDebug("STORAGE: CanEquipItem slot = %u, item = %u, count = %u", slot, pItem->GetEntry(), pItem->GetCount());
		ItemPrototype const *pProto = pItem->GetProto();

		if( pProto )
		{
			//if(pItem->IsBindedNotWith(GetGUID()))
			//	return EQUIP_ERR_DONT_OWN_THAT_ITEM;

			// check count of items (skip for auto move for same player from bank)
			uint8 res = CanTakeMoreSimilarItems(pItem);
			if(res != EQUIP_ERR_OK)
				return res;

			uint8 eslot = FindEquipSlot( pProto, slot, swap );
			if( eslot == NULL_SLOT )
				return EQUIP_ERR_ITEM_CANT_BE_EQUIPPED;
		
			dest = eslot;
			return EQUIP_ERR_OK;
		}
	}
	if( !swap )
		return EQUIP_ERR_ITEM_NOT_FOUND;
	else
		return EQUIP_ERR_ITEMS_CANT_BE_SWAPPED;
}

uint8 Player::CanUnequipItem( uint8 pos, bool swap ) const
{
	return EQUIP_ERR_OK;
}

uint8 Player::CanBankItem( uint8 slot, uint8 &dest, Item *pItem, bool swap, bool not_loading ) const
{
	return EQUIP_ERR_OK;
}

uint8 Player::CanUseItem( Item *pItem, bool not_loading ) const
{
	return EQUIP_ERR_OK;
}

bool Player::CanUseItem( ItemPrototype const *pProto )
{
	return true;
}

// Return stored item (if stored to stack, it can diff. from pItem). And pItem can be deleted in this case.
Item* Player::StoreNewItem( uint8 pos, uint32 item, uint32 count, bool update, int32 randomPropertyId)
{
	Item *pItem = CreateItem( item, count );
	if( pItem )
	{
		Item * retItem = StoreItem( pos, pItem, update );

		return retItem;
	}
	return NULL;
}

// Return stored item (if stored to stack, it can diff. from pItem). And pItem can be deleted in this case.
Item* Player::StoreItem( uint8 pos, Item *pItem, bool update )
{
	if( pItem )
	{
		uint8 slot = pos ;

		//sLog.outDebug( "STORAGE: StoreItem slot = %u, item = %u, count = %u", slot, pItem->GetEntry(), pItem->GetCount());

		Item *pItem2 = GetItemByPos( slot );

		if( !pItem2 )
		{
			m_items[slot] = pItem;
			SetUInt64Value( (uint16)(PLAYER_FIELD_INV_SLOT_HEAD + (slot * 2) ), pItem->GetGUID() );
			pItem->SetUInt64Value( ITEM_FIELD_CONTAINED, GetGUID() );
			pItem->SetUInt64Value( ITEM_FIELD_OWNER, GetGUID() );

			pItem->SetSlot( slot );
			pItem->SetContainer( NULL );

			if( IsInWorld() && update )
			{
				pItem->AddToWorld();
				//pItem->SendUpdateToPlayer( this );
			}

			pItem->SetState(ITEM_CHANGED, this);
		}
		else
		{
			pItem2->SetCount( pItem2->GetCount() + pItem->GetCount() );
			if( IsInWorld() && update )
				//pItem2->SendUpdateToPlayer( this );

			if( IsInWorld() && update )
			{
				pItem->RemoveFromWorld();
				pItem->DestroyForPlayer( this );
			}

			pItem->SetOwnerGUID(GetGUID());  // prevent error at next SetState in case trade/buy from vendor
			pItem->SetState(ITEM_REMOVED, this);
			pItem2->SetState(ITEM_CHANGED, this);

			return pItem2;
		}
	}

	return pItem;
}

Item* Player::EquipNewItem( uint8 pos, uint32 item, uint32 count, bool update )
{
	Item *pItem = CreateItem( item, count );
	if( pItem )
	{
		Item * retItem = EquipItem( pos, pItem, update );

		return retItem;
	}
	return NULL;
}

Item* Player::EquipItem( uint8 pos, Item *pItem, bool update )
{
	if( !pItem )
		return pItem;

	sLog.outDebug("STORAGE: EquipItem '%s' slot %u", pItem->GetProto()->Name, pos);
	Item* pItem2 = GetItemByPos( pos );
	if( !pItem2 )
	{
		VisualizeItem( pos, pItem );
	}
	else
	{
		//sLog.outDebug("STORAGE: EquipItem destination is not null");
		pItem2->SetCount( pItem2->GetCount() + pItem->GetCount() );
		pItem->SetOwnerGUID(GetGUID());
		pItem->SetState(ITEM_REMOVED, this);
		pItem2->SetState(ITEM_CHANGED, this);
		return pItem2;
	}
	return pItem;
}

void Player::RemoveItem( uint8 slot )
{
	// note: remove item does not actually change the item
	// it only takes the item out of storage temporarily
	Item *pItem = GetItemByPos( slot );
	if( !pItem )
		return;

	sLog.outDebug("STORAGE: RemoveItem slot = %u, item = %u", slot, pItem->GetEntry());
	m_items[slot] = NULL;
	pItem->SetSlot( NULL_SLOT );
	pItem->SetUInt64Value( ITEM_FIELD_CONTAINED, 0 );
}

void Player::QuickEquipItem( uint8 pos, Item *pItem)
{
	if( pItem )
	{

		VisualizeItem( pos, pItem );

		if( IsInWorld() )
		{
			pItem->AddToWorld();
			pItem->SendUpdateToPlayer( this );
		}
	}
}

void Player::QuickPetEquipItem( uint32 pet_guid, uint8 pos, Item *pItem )
{
	if( pItem )
	{

		VisualizePetItem( pet_guid, pos, pItem );

		if( IsInWorld() )
		{
			pItem->AddToWorld();
			pItem->SendUpdateToPlayer( this );
		}
	}
}

void Player::VisualizeItem( uint8 pos, Item *pItem)
{
	if(!pItem)
		return;

	uint8 slot = pos;

	//sLog.outDebug( "STORAGE: EquipItem slot = %u, item = %u", slot, pItem->GetEntry());
	
	m_items[slot] = pItem;
	SetUInt64Value( (uint16)(PLAYER_FIELD_INV_SLOT_HEAD + (slot * 2) ), pItem->GetGUID() );
	pItem->SetUInt64Value( ITEM_FIELD_CONTAINED, GetGUID() );
	pItem->SetUInt64Value( ITEM_FIELD_OWNER, GetGUID() );
	pItem->SetSlot( slot );
	pItem->SetContainer( NULL );

	if( slot < EQUIPMENT_SLOT_END )
	{
		int VisibleBase = PLAYER_VISIBLE_ITEM_1_0 + (slot * 16);
		SetUInt32Value(VisibleBase, pItem->GetEntry());

//		SetUInt32Value(VisibleBase + 8, uint32(pItem->GetItemRandomPropertyId()));
	}

	pItem->SetState(ITEM_CHANGED, this);
}

void Player::VisualizePetItem( uint32 pet_guid, uint8 pos, Item *pItem )
{
	if(!pItem)
		return;

	uint8 slot = pos;

	//sLog.outDebug( "STORAGE: EquipPetItem slot = %u, item = %u", slot, pItem->GetEntry());

	Pet* pet = GetPetByGuid( pet_guid );
	pet->SetEquip( slot, pItem, false);
	pItem->SetState(ITEM_CHANGED, this);
}




// Return stored item (if stored to stack, it can diff. from pItem). And pItem can be deleted in this case.
Item* Player::BankItem( uint8 pos, Item *pItem, bool update )
{
	return StoreItem( pos, pItem, update );
}

bool Player::AddNewInventoryItem(uint32 modelid, uint32 count)
{
	uint8 dest;

	uint32 itemid = objmgr.GetItemEntryByModelId(modelid);

	Item *item;

	if( CanStoreNewItem( NULL_SLOT, dest, itemid, count, false ) == EQUIP_ERR_OK )
	{
		sLog.outDebug("STORAGE: Add new inventory item <%u>", itemid);
		item = StoreNewItem( dest, itemid, count, false );
		return true;
	}

	sLog.outDebug("STORAGE: Can not add new item <%u> to inventory", itemid);
	return false;
}

//////////////////////////////////////////////////////////////////////////////
bool Player::isBattleInProgress()
{
	return (PlayerBattleClass != NULL ? true : false);
}

void Player::LeaveBattle()
{
	Player* battleMaster = GetBattleMaster();
	if( battleMaster )
		if( battleMaster->PlayerBattleClass )
			battleMaster->PlayerBattleClass->LeaveBattle(this);

	PlayerBattleClass = NULL;
	i_battleMaster    = NULL;
}

void Player::Engage(Creature* enemy)
{
	if( isBattleInProgress() )
		return;

	PlayerBattleClass = new BattleSystem(this);
	PlayerBattleClass->Engage( this, enemy );
}

void Player::Engage(Player* ally)
{
}

Player* Player::GetBattleMaster()
{
	if( i_battleMaster )
	{
		//sLog.outDebug("PLAYER: GetBattleEngine for '%s' is self", GetName());
		return i_battleMaster;
	}

	return NULL;
}
//////////////////////////////////////////////////////////////////////////////

bool Player::CanJoinTeam()
{
	if( !isTeamLeader() )
		return false;

	return (m_team.size() < 4 ? true : false);
}

void Player::JoinTeam(Player* member)
{
	sLog.outDebug("GROUP: '%s' is joining", member->GetName());
	member->SetLeader(GetGUIDLow());
	m_team.push_back(member);

	WorldPacket data;
	data.Initialize( 0x0D );
	data << (uint8 ) 0x05;
	data << (uint32) GetAccountId(); // leader id
	data << (uint32) member->GetAccountId(); // member id

	SendMessageToSet(&data, true);

}

void Player::LeaveTeam(Player* member)
{
	sLog.outDebug("GROUP: '%s' is leaving", member->GetName());
	m_team.remove(member);
	member->SetLeader(0);

	WorldPacket data;
	data.Initialize( 0x0D );
	data << (uint8 ) 0x04;
	data << (uint32) member->GetAccountId();

	SendMessageToSet(&data, true);
}

bool Player::isTeamLeader()
{
	if( isJoinedTeam() )
		return false;

	return true;
}

bool Player::isJoinedTeam()
{
	return (m_leaderGuid ? true : false);
}

void Player::SetSubleader(uint32 acc_id)
{
	Player* sub = ObjectAccessor::FindPlayerByAccountId(acc_id);
	if( !sub )
		return;

	for(TeamList::const_iterator itr = m_team.begin(); itr != m_team.end(); ++itr)
	{
		if( sub == (*itr) )
		{
			m_subleaderGuid = sub->GetGUIDLow();
			sLog.outDebug("GROUP: Promote '%s' as Sub Leader", sub->GetName());
		}
	}
}

uint32 Player::GetTeamGuid(uint8 index) const
{
	uint8 count = 0;
	for(TeamList::const_iterator itr = m_team.begin(); itr != m_team.end(); ++itr)
	{
		count++;

		if( count == index )
			return (*itr)->GetGUIDLow();

	}

	return 0;
}
