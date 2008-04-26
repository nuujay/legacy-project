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

#ifndef __LEGACY_UPDATEFIELDS_H
#define __LEGACY_UPDATEFIELDS_H

enum EObjectFields
{
	OBJECT_FIELD_GUID              = 0x0000, // Size: 2 Type: LONG, PUBLIC
	OBJECT_FIELD_TYPE              = 0x0002, // Size: 1 Type: INT, PUBLIC
	OBJECT_FIELD_ENTRY             = 0x0003, // Size: 1 Type: INT, PUBLIC
	OBJECT_END                     = 0x0006
};

enum EItemFields
{
	ITEM_FIELD_OWNER          = OBJECT_END + 0x0000, // Size: 2, LONG, PUBLIC
	ITEM_FIELD_CONTAINED      = OBJECT_END + 0x0002, // Size: 2, LONG, PUBLIC

	ITEM_FIELD_STACK_COUNT    = OBJECT_END + 0x0008, // Size: 1, INT, OWNER_ONLY
	ITEM_FIELD_DURATION       = OBJECT_END + 0x0009, // Size: 1, INT, OWNER_ONLY

	ITEM_FIELD_FLAGS          = OBJECT_END + 0x000F, // Size: 1, INT, PUBLIC

	ITEM_END                  = OBJECT_END + 0x0036,
};

enum EContainerFields
{
	CONTAINER_FIELD_NUM_SLOTS = ITEM_END + 0x0000, // Size: 1, INT, PUBLIC
	CONTAINER_END             = ITEM_END + 0x004A,
};


enum EUnitFields
{
	UNIT_FIELD_SUMMON         = OBJECT_END + 0x0000, // Size: 2 LONG PUBLIC
	UNIT_FIELD_SUMMONEDBY     = OBJECT_END + 0x0002, // Size: 2 LONG PUBLIC
	UNIT_FIELD_REBORN         = OBJECT_END + 0x0004, // Size: 1 INT PUBLIC
	UNIT_FIELD_ELEMENT        = OBJECT_END + 0x0005, // Size: 1 INT PUBLIC

	UNIT_FIELD_EQ_HEAD        = OBJECT_END + 0x0006, // Size: 2 INT PUBLIC
	UNIT_FIELD_EQ_BODY        = OBJECT_END + 0x0008, // Size: 2 INT PUBLIC
	UNIT_FIELD_EQ_WRIST       = OBJECT_END + 0x000A, // Size: 2 INT PUBLIC
	UNIT_FIELD_EQ_WEAPON      = OBJECT_END + 0x000C, // Size: 2 INT PUBLIC
	UNIT_FIELD_EQ_SHOE        = OBJECT_END + 0x000E, // Size: 2 INT PUBLIC
	UNIT_FIELD_EQ_SPECIAL     = OBJECT_END + 0x0010, // Size: 2 INT PUBLIC

	UNIT_FIELD_HP             = OBJECT_END + 0x0012, // Size: 2 INT PUBLIC
	UNIT_FIELD_SP             = OBJECT_END + 0x0014, // Size: 2 INT PUBLIC

	UNIT_FIELD_HP_MAX         = OBJECT_END + 0x0016, // Size: 2 INT PUBLIC
	UNIT_FIELD_SP_MAX         = OBJECT_END + 0x0018, // Size: 2 INT PUBLIC

	UNIT_FIELD_INT            = OBJECT_END + 0x001A, // Size: 2 INT PUBLIC
	UNIT_FIELD_ATK            = OBJECT_END + 0x001C, // Size: 2 INT PUBLIC
	UNIT_FIELD_DEF            = OBJECT_END + 0x001E, // Size: 2 INT PUBLIC
	UNIT_FIELD_AGI            = OBJECT_END + 0x0020, // Size: 2 INT PUBLIC
	UNIT_FIELD_HPX            = OBJECT_END + 0x0022, // Size: 2 INT PUBLIC
	UNIT_FIELD_SPX            = OBJECT_END + 0x0024, // Size: 2 INT PUBLIC

	UNIT_FIELD_INT_MOD        = OBJECT_END + 0x0026, // Size: 2 INT PUBLIC
	UNIT_FIELD_ATK_MOD        = OBJECT_END + 0x0028, // Size: 2 INT PUBLIC
	UNIT_FIELD_DEF_MOD        = OBJECT_END + 0x002A, // Size: 2 INT PUBLIC
	UNIT_FIELD_AGI_MOD        = OBJECT_END + 0x002C, // Size: 2 INT PUBLIC
	UNIT_FIELD_HPX_MOD        = OBJECT_END + 0x002E, // Size: 2 INT PUBLIC
	UNIT_FIELD_SPX_MOD        = OBJECT_END + 0x0030, // Size: 2 INT PUBLIC

	UNIT_FIELD_LEVEL          = OBJECT_END + 0x0032, // Size: 1 INT PUBLIC
	UNIT_FIELD_XP             = OBJECT_END + 0x0033, // Size: 4 INT PUBLIC
	UNIT_FIELD_NEXT_LEVEL_XP  = OBJECT_END + 0x0037, // Size: 4 INT PUBLIC

	UNIT_FIELD_FLAGS          = OBJECT_END + 0x003B, // Size: 1 INT PUBLIC
	UNIT_DYNAMIC_FLAGS        = OBJECT_END + 0x003C, // Size: 1 INT OWNER_ONLY
	UNIT_NPC_FLAGS            = OBJECT_END + 0x003D, // Size: 1 INT DYNAMIC
	UNIT_FIELD_DISPLAYID      = OBJECT_END + 0x003E, // Size: 2 INT PUBLIC

	UNIT_FIELD_SPELL_POINT    = OBJECT_END + 0x0040, // Size: 2 INT PUBLIC
	UNIT_FIELD_STAT_POINT     = OBJECT_END + 0x0042, // Size: 2 INT PUBLIC

	UNIT_MORAL_DONGWU         = OBJECT_END + 0x0044, // Size: 2 INT PUBLIC
	UNIT_MORAL_2              = OBJECT_END + 0x0046, // Size: 2 INT PUBLIC
	UNIT_MORAL_3              = OBJECT_END + 0x0048, // Size: 2 INT PUBLIC
	UNIT_MORAL_4              = OBJECT_END + 0x004A, // Size: 2 INT PUBLIC
	UNIT_MORAL_5              = OBJECT_END + 0x004C, // Size: 2 INT PUBLIC

	UNIT_END                  = OBJECT_END + 0x004E, // Size: 1 INT PUBLIC

	PLAYER_GENDER                       = UNIT_END + 0x0000, // 1 INT PUBLIC
	PLAYER_FACE                         = UNIT_END + 0x0001, // 1 INT PUBLIC
	PLAYER_HAIR                         = UNIT_END + 0x0002, // 1 INT PUBLIC

	PLAYER_HAIR_COLOR_R                 = UNIT_END + 0x0003, // 1 INT PUBLIC
	PLAYER_HAIR_COLOR_G                 = UNIT_END + 0x0004, // 1 INT PUBLIC
	PLAYER_HAIR_COLOR_B                 = UNIT_END + 0x0005, // 1 INT PUBLIC

	PLAYER_SKIN_COLOR_R                 = UNIT_END + 0x0006, // 1 INT PUBLIC
	PLAYER_SKIN_COLOR_G                 = UNIT_END + 0x0007, // 1 INT PUBLIC
	PLAYER_SKIN_COLOR_B                 = UNIT_END + 0x0008, // 1 INT PUBLIC

	PLAYER_SHIRT_COLOR                  = UNIT_END + 0x0009, // 1 INT PUBLIC
	PLAYER_MISC_COLOR                   = UNIT_END + 0x000A, // 1 INT PUBLIC
	PLAYER_GOLD_INHAND                  = UNIT_END + 0x000E, // 4 INT PUBLIC
	PLAYER_GOLD_INBANK                  = UNIT_END + 0x0012, // 4 INT PUBLIC

	PLAYER_EMOTE                        = UNIT_END + 0x0016, // 1 INT PUBLIC
	PLAYER_EMOTE_ACTION                 = UNIT_END + 0x0017, // 1 INT PUBLIC

	PLAYER_HERALD_CONSUMPTION           = UNIT_END + 0x0018, // 4 INT PUBLIC

	PLAYER_END                          = UNIT_END + 0x001C,

	/*
	PLAYER_DUEL_ARBITER                 = UNIT_END + 0x0000, // 2 LONG  PUBLIC
	PLAYER_FLAGS                        = UNIT_END + 0x0002, // 1 INT   PUBLIC
	PLAYER_GUILDID                      = UNIT_END + 0x0003, // 1 INT PUBLIC
	PLAYER_GUILDRANK                    = UNIT_END + 0x0004, // 1 INT PUBLIC
	PLAYER_BYTES                        = UNIT_END + 0x0005, // 1 BYTES PUBLIC
	PLAYER_BYTES_2                      = UNIT_END + 0x0006, // 1 BYTES PUBLIC
	PLAYER_BYTES_3                      = UNIT_END + 0x0007, // 1 BYTES PUBLIC
	PLAYER_DUEL_TEAM                    = UNIT_END + 0x0008, // 1 INT PUBLIC
	PLAYER_GUILD_TIMESTAMP              = UNIT_END + 0x0009, // 1 INT PUBLIC
	PLAYER_QUEST_LOG_1_1                = UNIT_END + 0x000A, // 1 INT GROUP_ONLY
	PLAYER_QUEST_LOG_1_2                = UNIT_END + 0x000B, // 2 INT PRIVATE
	PLAYER_QUEST_LOG_2_1                = UNIT_END + 0x000D, // 1 INT GROUP_ONLY
	PLAYER_QUEST_LOG_2_2                = UNIT_END + 0x000E, // 2 INT PRIVATE
	PLAYER_QUEST_LOG_3_1                = UNIT_END + 0x0010, // 1 INT GROUP_ONLY
	PLAYER_QUEST_LOG_3_2                = UNIT_END + 0x0011, // 2 INT PRIVATE
	PLAYER_QUEST_LOG_4_1                = UNIT_END + 0x0013, // 1 INT GROUP_ONLY
	PLAYER_QUEST_LOG_4_2                = UNIT_END + 0x0014, // 2 INT PRIVATE
	PLAYER_QUEST_LOG_5_1                = UNIT_END + 0x0016, // 1 INT GROUP_ONLY
	PLAYER_QUEST_LOG_5_2                = UNIT_END + 0x0017, // 2 INT PRIVATE
	PLAYER_QUEST_LOG_6_1                = UNIT_END + 0x0019, // 1 INT GROUP_ONLY
	PLAYER_QUEST_LOG_6_2                = UNIT_END + 0x001A, // 2 INT PRIVATE
	PLAYER_QUEST_LOG_7_1                = UNIT_END + 0x001C, // 1 INT GROUP_ONLY
	PLAYER_QUEST_LOG_7_2                = UNIT_END + 0x001D, // 2 INT PRIVATE
	PLAYER_QUEST_LOG_8_1                = UNIT_END + 0x001F, // 1 INT GROUP_ONLY
	PLAYER_QUEST_LOG_8_2                = UNIT_END + 0x0020, // 2 INT PRIVATE
	PLAYER_QUEST_LOG_9_1                = UNIT_END + 0x0022, // 1 INT GROUP_ONLY
	PLAYER_QUEST_LOG_9_2                = UNIT_END + 0x0023, // 2 INT PRIVATE
	PLAYER_QUEST_LOG_10_1               = UNIT_END + 0x0025, // 1 INT GROUP_ONLY
	PLAYER_QUEST_LOG_10_2               = UNIT_END + 0x0026, // 2 INT PRIVATE
	PLAYER_QUEST_LOG_11_1               = UNIT_END + 0x0028, // 1 INT GROUP_ONLY
	PLAYER_QUEST_LOG_11_2               = UNIT_END + 0x0029, // 2 INT PRIVATE
	PLAYER_QUEST_LOG_12_1               = UNIT_END + 0x002B, // 1 INT GROUP_ONLY
	PLAYER_QUEST_LOG_12_2               = UNIT_END + 0x002C, // 2 INT PRIVATE
	PLAYER_QUEST_LOG_13_1               = UNIT_END + 0x002E, // 1 INT GROUP_ONLY
	PLAYER_QUEST_LOG_13_2               = UNIT_END + 0x002F, // 2 INT PRIVATE
	PLAYER_QUEST_LOG_14_1               = UNIT_END + 0x0031, // 1 INT GROUP_ONLY
	PLAYER_QUEST_LOG_14_2               = UNIT_END + 0x0032, // 2 INT PRIVATE
	PLAYER_QUEST_LOG_15_1               = UNIT_END + 0x0034, // 1 INT GROUP_ONLY
	PLAYER_QUEST_LOG_15_2               = UNIT_END + 0x0035, // 2 INT PRIVATE
	PLAYER_QUEST_LOG_16_1               = UNIT_END + 0x0037, // 1 INT GROUP_ONLY
	PLAYER_QUEST_LOG_16_2               = UNIT_END + 0x0038, // 2 INT PRIVATE
	PLAYER_QUEST_LOG_17_1               = UNIT_END + 0x003A, // 1 INT GROUP_ONLY
	PLAYER_QUEST_LOG_17_2               = UNIT_END + 0x003B, // 2 INT PRIVATE
	PLAYER_QUEST_LOG_18_1               = UNIT_END + 0x003D, // 1 INT GROUP_ONLY
	PLAYER_QUEST_LOG_18_2               = UNIT_END + 0x003E, // 2 INT PRIVATE
	PLAYER_QUEST_LOG_19_1               = UNIT_END + 0x0040, // 1 INT GROUP_ONLY
	PLAYER_QUEST_LOG_19_2               = UNIT_END + 0x0041, // 2 INT PRIVATE
	PLAYER_QUEST_LOG_20_1               = UNIT_END + 0x0043, // 1 INT GROUP_ONLY
	PLAYER_QUEST_LOG_20_2               = UNIT_END + 0x0044, // 2 INT PRIVATE
	PLAYER_QUEST_LOG_21_1               = UNIT_END + 0x0046, // 1 INT GROUP_ONLY
	PLAYER_QUEST_LOG_21_2               = UNIT_END + 0x0047, // 2 INT PRIVATE
	PLAYER_QUEST_LOG_22_1               = UNIT_END + 0x0049, // 1 INT GROUP_ONLY
	PLAYER_QUEST_LOG_22_2               = UNIT_END + 0x004A, // 2 INT PRIVATE
	PLAYER_QUEST_LOG_23_1               = UNIT_END + 0x004C, // 1 INT GROUP_ONLY
	PLAYER_QUEST_LOG_23_2               = UNIT_END + 0x004D, // 2 INT PRIVATE
	PLAYER_QUEST_LOG_24_1               = UNIT_END + 0x004F, // 1 INT GROUP_ONLY
	PLAYER_QUEST_LOG_24_2               = UNIT_END + 0x0050, // 2 INT PRIVATE
	PLAYER_QUEST_LOG_25_1               = UNIT_END + 0x0052, // 1 INT GROUP_ONLY
	PLAYER_QUEST_LOG_25_2               = UNIT_END + 0x0053, // 2 INT PRIVATE
	PLAYER_VISIBLE_ITEM_1_CREATOR       = UNIT_END + 0x0055, // 2 LONG PUBLIC
	PLAYER_VISIBLE_ITEM_1_0             = UNIT_END + 0x0057, // 12 INT PUBLIC
	PLAYER_VISIBLE_ITEM_1_PROPERTIES    = UNIT_END + 0x0063, // 1 2_SHORT PUBLIC
	PLAYER_VISIBLE_ITEM_1_PAD           = UNIT_END + 0x0064, // 1 INT PUBLIC
	PLAYER_VISIBLE_ITEM_2_CREATOR       = UNIT_END + 0x0065, // 2 LONG PUBLIC
	PLAYER_VISIBLE_ITEM_2_0             = UNIT_END + 0x0067, // 12 INT PUBLIC
	PLAYER_VISIBLE_ITEM_2_PROPERTIES    = UNIT_END + 0x0073, // 1 2_SHORT PUBLIC
	PLAYER_VISIBLE_ITEM_2_PAD           = UNIT_END + 0x0074, // 1 INT PUBLIC
	PLAYER_VISIBLE_ITEM_3_CREATOR       = UNIT_END + 0x0075, // 2 LONG PUBLIC
	PLAYER_VISIBLE_ITEM_3_0             = UNIT_END + 0x0077, // 12 INT PUBLIC
	PLAYER_VISIBLE_ITEM_3_PROPERTIES    = UNIT_END + 0x0083, // 1 2_SHORT PUBLIC
	PLAYER_VISIBLE_ITEM_3_PAD           = UNIT_END + 0x0084, // 1 INT PUBLIC
	PLAYER_VISIBLE_ITEM_4_CREATOR       = UNIT_END + 0x0085, // 2 LONG PUBLIC
	PLAYER_VISIBLE_ITEM_4_0             = UNIT_END + 0x0087, // 12 INT PUBLIC
	PLAYER_VISIBLE_ITEM_4_PROPERTIES    = UNIT_END + 0x0093, // 1 2_SHORT PUBLIC
	PLAYER_VISIBLE_ITEM_4_PAD           = UNIT_END + 0x0094, // 1 INT PUBLIC
	PLAYER_VISIBLE_ITEM_5_CREATOR       = UNIT_END + 0x0095, // 2 LONG PUBLIC
	PLAYER_VISIBLE_ITEM_5_0             = UNIT_END + 0x0097, // 12 INT PUBLIC
	PLAYER_VISIBLE_ITEM_5_PROPERTIES    = UNIT_END + 0x00A3, // 1 2_SHORT PUBLIC
	PLAYER_VISIBLE_ITEM_5_PAD           = UNIT_END + 0x00A4, // 1 INT PUBLIC
	PLAYER_VISIBLE_ITEM_6_CREATOR       = UNIT_END + 0x00A5, // 2 LONG PUBLIC
	PLAYER_VISIBLE_ITEM_6_0             = UNIT_END + 0x00A7, // 12 INT PUBLIC
	PLAYER_VISIBLE_ITEM_6_PROPERTIES    = UNIT_END + 0x00B3, // 1 2_SHORT PUBLIC
	PLAYER_VISIBLE_ITEM_6_PAD           = UNIT_END + 0x00B4, // 1 INT PUBLIC
	PLAYER_VISIBLE_ITEM_7_CREATOR       = UNIT_END + 0x00B5, // 2 LONG PUBLIC
	PLAYER_VISIBLE_ITEM_7_0             = UNIT_END + 0x00B7, // 12 INT PUBLIC
	PLAYER_VISIBLE_ITEM_7_PROPERTIES    = UNIT_END + 0x00C3, // 1 2_SHORT PUBLIC
	PLAYER_VISIBLE_ITEM_7_PAD           = UNIT_END + 0x00C4, // 1 INT PUBLIC
	PLAYER_VISIBLE_ITEM_8_CREATOR       = UNIT_END + 0x00C5, // 2 LONG PUBLIC
	PLAYER_VISIBLE_ITEM_8_0             = UNIT_END + 0x00C7, // 12 INT PUBLIC
	PLAYER_VISIBLE_ITEM_8_PROPERTIES    = UNIT_END + 0x00D3, // 1 2_SHORT PUBLIC
	PLAYER_VISIBLE_ITEM_8_PAD           = UNIT_END + 0x00D4, // 1 INT PUBLIC
	PLAYER_VISIBLE_ITEM_9_CREATOR       = UNIT_END + 0x00D5, // 2 LONG PUBLIC
	PLAYER_VISIBLE_ITEM_9_0             = UNIT_END + 0x00D7, // 12 INT PUBLIC
	PLAYER_VISIBLE_ITEM_9_PROPERTIES    = UNIT_END + 0x00E3, // 1 2_SHORT PUBLIC
	PLAYER_VISIBLE_ITEM_9_PAD           = UNIT_END + 0x00E4, // 1 INT PUBLIC
	PLAYER_VISIBLE_ITEM_10_CREATOR      = UNIT_END + 0x00E5, // 2 LONG PUBLIC
	PLAYER_VISIBLE_ITEM_10_0            = UNIT_END + 0x00E7, // 12 INT PUBLIC
	PLAYER_VISIBLE_ITEM_10_PROPERTIES   = UNIT_END + 0x00F3, // 1 2_SHORT PUBLIC
	PLAYER_VISIBLE_ITEM_10_PAD          = UNIT_END + 0x00F4, // 1 INT PUBLIC
	PLAYER_VISIBLE_ITEM_11_CREATOR      = UNIT_END + 0x00F5, // 2 LONG PUBLIC
	PLAYER_VISIBLE_ITEM_11_0            = UNIT_END + 0x00F7, // 12 INT PUBLIC
	PLAYER_VISIBLE_ITEM_11_PROPERTIES   = UNIT_END + 0x0103, // 1 2_SHORT PUBLIC
	PLAYER_VISIBLE_ITEM_11_PAD          = UNIT_END + 0x0104, // 1 INT PUBLIC
	PLAYER_VISIBLE_ITEM_12_CREATOR      = UNIT_END + 0x0105, // 2 LONG PUBLIC
	PLAYER_VISIBLE_ITEM_12_0            = UNIT_END + 0x0107, // 12 INT PUBLIC
	PLAYER_VISIBLE_ITEM_12_PROPERTIES   = UNIT_END + 0x0113, // 1 2_SHORT PUBLIC
	PLAYER_VISIBLE_ITEM_12_PAD          = UNIT_END + 0x0114, // 1 INT PUBLIC
	PLAYER_VISIBLE_ITEM_13_CREATOR      = UNIT_END + 0x0115, // 2 LONG PUBLIC
	PLAYER_VISIBLE_ITEM_13_0            = UNIT_END + 0x0117, // 12 INT PUBLIC
	PLAYER_VISIBLE_ITEM_13_PROPERTIES   = UNIT_END + 0x0123, // 1 2_SHORT PUBLIC
	PLAYER_VISIBLE_ITEM_13_PAD          = UNIT_END + 0x0124, // 1 INT PUBLIC
	PLAYER_VISIBLE_ITEM_14_CREATOR      = UNIT_END + 0x0125, // 2 LONG PUBLIC
	PLAYER_VISIBLE_ITEM_14_0            = UNIT_END + 0x0127, // 12 INT PUBLIC
	PLAYER_VISIBLE_ITEM_14_PROPERTIES   = UNIT_END + 0x0133, // 1 2_SHORT PUBLIC
	PLAYER_VISIBLE_ITEM_14_PAD          = UNIT_END + 0x0134, // 1 INT PUBLIC
	PLAYER_VISIBLE_ITEM_15_CREATOR      = UNIT_END + 0x0135, // 2 LONG PUBLIC
	PLAYER_VISIBLE_ITEM_15_0            = UNIT_END + 0x0137, // 12 INT PUBLIC
	PLAYER_VISIBLE_ITEM_15_PROPERTIES   = UNIT_END + 0x0143, // 1 2_SHORT PUBLIC
	PLAYER_VISIBLE_ITEM_15_PAD          = UNIT_END + 0x0144, // 1 INT PUBLIC
	PLAYER_VISIBLE_ITEM_16_CREATOR      = UNIT_END + 0x0145, // 2 LONG PUBLIC
	PLAYER_VISIBLE_ITEM_16_0            = UNIT_END + 0x0147, // 12 INT PUBLIC
	PLAYER_VISIBLE_ITEM_16_PROPERTIES   = UNIT_END + 0x0153, // 1 2_SHORT PUBLIC
	PLAYER_VISIBLE_ITEM_16_PAD          = UNIT_END + 0x0154, // 1 INT PUBLIC
	PLAYER_VISIBLE_ITEM_17_CREATOR      = UNIT_END + 0x0155, // 2 LONG PUBLIC
	PLAYER_VISIBLE_ITEM_17_0            = UNIT_END + 0x0157, // 12 INT PUBLIC
	PLAYER_VISIBLE_ITEM_17_PROPERTIES   = UNIT_END + 0x0163, // 1 2_SHORT PUBLIC
	PLAYER_VISIBLE_ITEM_17_PAD          = UNIT_END + 0x0164, // 1 INT PUBLIC
	PLAYER_VISIBLE_ITEM_18_CREATOR      = UNIT_END + 0x0165, // 2 LONG PUBLIC
	PLAYER_VISIBLE_ITEM_18_0            = UNIT_END + 0x0167, // 12 INT PUBLIC
	PLAYER_VISIBLE_ITEM_18_PROPERTIES   = UNIT_END + 0x0173, // 1 2_SHORT PUBLIC
	PLAYER_VISIBLE_ITEM_18_PAD          = UNIT_END + 0x0174, // 1 INT PUBLIC
	PLAYER_VISIBLE_ITEM_19_CREATOR      = UNIT_END + 0x0175, // 2 LONG PUBLIC
	PLAYER_VISIBLE_ITEM_19_0            = UNIT_END + 0x0177, // 12 INT PUBLIC
	PLAYER_VISIBLE_ITEM_19_PROPERTIES   = UNIT_END + 0x0183, // 1 2_SHORT PUBLIC
	PLAYER_VISIBLE_ITEM_19_PAD          = UNIT_END + 0x0184, // 1 INT PUBLIC
	PLAYER_CHOSEN_TITLE                 = UNIT_END + 0x0185, // 1 INT PUBLIC
	PLAYER_FIELD_INV_SLOT_HEAD          = UNIT_END + 0x0186, // 46 LONG PRIVATE
	PLAYER_FIELD_PACK_SLOT_1            = UNIT_END + 0x01B4, // 32 LONG PRIVATE
	PLAYER_FIELD_BANK_SLOT_1            = UNIT_END + 0x01D4, // 56 LONG PRIVATE
	PLAYER_FIELD_BANKBAG_SLOT_1         = UNIT_END + 0x020C, // 14 LONG PRIVATE
	PLAYER_FIELD_VENDORBUYBACK_SLOT_1   = UNIT_END + 0x021A, // 24 LONG PRIVATE
	PLAYER_FIELD_KEYRING_SLOT_1         = UNIT_END + 0x0232, // 64 LONG PRIVATE

	PLAYER_END                          = UNIT_END + 0x04C0,
	*/
};

enum UpdateFieldsFlags
{
	UPD_FLAG_HP                         = 0x19,
	UPD_FLAG_SP                         = 0x1A,
	UPD_FLAG_INT                        = 0x1B,
	UPD_FLAG_ATK                        = 0x1C,
	UPD_FLAG_DEF                        = 0x1D,
	UPD_FLAG_AGI                        = 0x1E,
	UPD_FLAG_HPX                        = 0x1F,
	UPD_FLAG_SPX                        = 0x20,

	UPD_FLAG_LEVEL                      = 0x23,
	UPD_FLAG_XP                         = 0x24,
	UPD_FLAG_SPELL_POINT                = 0x25,
	UPD_FLAG_STAT_POINT                 = 0x26,

	UPD_FLAG_LOYALTY                    = 0x40,

	UPD_FLAG_HPX_MOD                    = 0xCF,
	UPD_FLAG_SPX_MOD                    = 0xD0,

	UPD_FLAG_ATK_MOD                    = 0xD2,
	UPD_FLAG_DEF_MOD                    = 0xD3,
	UPD_FLAG_INT_MOD                    = 0xD4,

	UPD_FLAG_AGI_MOD                    = 0xD6,


	UPD_FLAG_ADD_SPELL                  = 0x6E,
};

#endif
