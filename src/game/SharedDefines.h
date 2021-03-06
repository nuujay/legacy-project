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

#ifndef __LEGACY_SHAREDDEFINES_H
#define __LEGACY_SHAREDDEFINES_H

#include "Platform/Define.h"
#include <cassert>

enum Gender
{
	GENDER_MALE                      = 0,
	GENDER_FEMALE                    = 1,
	GENDER_NONE                      = 2
};

enum ElementTypes
{
	ELEMENT_HEART                    = 0,
	ELEMENT_EARTH                    = 1,
	ELEMENT_WATER                    = 2,
	ELEMENT_FIRE                     = 3,
	ELEMENT_WIND                     = 4,
	ELEMENT_NONE                     = 5,
};

enum HeraldItemId
{
	ITEM_ROCKY_GOLEM                 = 23086,
	ITEM_WATER_GODDES,
	ITEM_PHOENIX,
	ITEM_GREEN_DRAGON
};

#define MAX_ELEMENT_ID                 6

enum MapTypes
{
	MAP_COMMON         = 0,
	MAP_ARENA          = 1
};

enum Stats
{
	STAT_INT   = 0,
	STAT_ATK   = 1,
	STAT_DEF   = 2
};

enum CharacterSlot
{
	SLOT_HEAD           = 0,
	SLOT_WRISTS         = 1,
	SLOT_BODY           = 2
};

enum Emote
{
	EMOTE_SMILE = 0,
	EMOTE_CRY   = 1
};

enum Anim
{
	ANIM_STAND     = 0x0,
	ANIM_WALK      = 0x1
};

enum ChatMsg
{
	CHAT_MSG_NULL                 = 0x00,
	CHAT_MSG_ALL                  = 0x01,
	CHAT_MSG_LIGHT                = 0x02,
	CHAT_MSG_WHISPER              = 0x03,
	CHAT_MSG_GM                   = 0x04,
	CHAT_MSG_PARTY                = 0x05,
	CHAT_MSG_ARMY                 = 0x06,
	CHAT_MSG_ALLY                 = 0x07,
	CHAT_MSG_UNK_0                = 0x08,
	CHAT_MSG_UNK_1                = 0x09,
	CHAT_MSG_UNK_2                = 0x0A,
	CHAT_MSG_SYSTEM               = 0x0B
};

#endif
