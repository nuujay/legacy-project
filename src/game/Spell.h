/*
 * Copyright (C) 2008-2008 LeGACY <http://code.google.com/p/legacy-project/>
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

#ifndef __LEGACY_SPELL_H
#define __LEGACY_SPELL_H

#include "GridDefines.h"

class WorldSession;
class Unit;
class Player;
class GameObject;

enum SpellUpdateState
{
	SPELL_UNCHANGED = 0,
	SPELL_CHANGED   = 1,
	SPELL_NEW       = 2,
	SPELL_REMOVED   = 3
};

enum SpellCastTargetFlags
{
	TARGET_FLAG_HURT              = 0x8000
};

enum SpellDamageType
{
	SPELL_DAMAGE_SCHOOL           = 0, // Can be pure ATK or pure INT 
	SPELL_DAMAGE_ATK_INT          = 1, // Full ATK + 0.5 INT
	SPELL_DAMAGE_INT              = 2, // Full INT
	SPELL_DAMAGE_MECH             = 3, // Full ATK ignore DEFENSE

};

enum SpellModType
{
	SPELL_MOD_NONE                = 0,
	SPELL_MOD_BUF                 = 1,
	SPELL_MOD_HURT                = 2,
	SPELL_MOD_BUF_HURT            = (SPELL_MOD_BUF | SPELL_MOD_HURT),
	SPELL_MOD_HEAL                = 6,
	SPELL_MOD_DISABLED_BUF        = 8,
	SPELL_MOD_POSITIVE_BUF        = 16,
	SPELL_MOD_NEGATIVE_BUF        = 32,

	SPELL_MOD_DISABLED_HURT       = (SPELL_MOD_DISABLED_BUF | SPELL_MOD_HURT),
};

enum SpellNumberEntry
{
	///- Basic spell
	SPELL_BASIC                   = 10000,   // Duel Tangan Kosong

	///- Earth Element spell
	SPELL_TREESPIRIT              = 10004,   // Tree Spirit
	SPELL_AURA                    = 10010,   // Aura
	SPELL_MIRROR                  = 10015,   // Mirror

	///- Water Element spell
	SPELL_ICEWALL                 = 11002,   // Ice Wall
	SPELL_FREEZING                = 11014,   // Freezing

	///- Wind Element spell
	SPELL_CYCLONE                 = 13002,   // Cyclone
	SPELL_DODGE                   = 13003,   // Dodge
	SPELL_INVISIBLE               = 13005,   // Invisible
	SPELL_STUN                    = 13007,   // Stun
	SPELL_MINIFY                  = 13011,   // Minify
	SPELL_MAGNIFY                 = 13012,   // Magnify
	SPELL_BERSERKER               = 13013,   // Berserker
	SPELL_VIGOR                   = 13014,   // Vigor

	///- Heart Element spell
	SPELL_RETREAT                 = 14002,   // Retreat
	SPELL_TEACHING                = 14027,   // Guru pembimbing
	SPELL_UNITED                  = 14028,   // Bersatu
	SPELL_CATCH                   = 15001,   // Menangkap
	SPELL_CATCH2                  = 15002,   // Menjala
	SPELL_SUCCESS_CATCH           = 15003,   // Sukses tangkap jala
	SPELL_DEFENSE                 = 17001,   // Defense
	SPELL_ESCAPE                  = 18001,   // Escape
	SPELL_ESCAPE_FAILED           = 18002,   // Escape Failed
};

/*
class SpellCastTargets
{
	public:
		SpellCastTargets() {}
		~SpellCastTargets() {}
};
*/
// from `spell_template` table
struct SpellInfo
{
	uint32 Entry;
	char*  Name;
	uint32 SP;
	uint32 Element;
	uint32 hit;
	uint32 LearnPoint;
	uint32 LevelMax;
	uint32 Type;
	uint32 DamageMod;
	uint32 Reborn;
	float  Core;
};

class Spell
{
	public:
		Spell(uint16 entry, uint8 level, SpellUpdateState state = SPELL_NEW);
		~Spell() {}

		uint16 GetEntry() { return m_entry; }
		uint8 GetLevel() { return m_level; }
		void SetLevel(uint8 level) { m_level = level; }
		void AddLevel(uint8 value) { m_level += value; }
		SpellInfo const* GetProto() const;
		SpellUpdateState GetState() { return uState; }
		void SetState(SpellUpdateState state) { uState = state; }

	private:
		uint16 m_entry;
		uint8  m_level;

		SpellUpdateState uState;
};

//typedef std::map<uint16, Spell*> SpellMap;
typedef HM_NAMESPACE::hash_map<uint16, Spell*> SpellMap;
#endif
