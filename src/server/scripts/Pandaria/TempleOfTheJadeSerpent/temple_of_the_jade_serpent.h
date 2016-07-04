/*
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ObjectMgr.h"
#include "Player.h"

#ifndef DEF_TEMPLE_OF_THE_JADE_SERPENT_H_
#define DEF_TEMPLE_OF_THE_JADE_SERPENT_H_

#define DataHeader "TJS"

enum Encounter
{
    BOSS_WISE_MARI,
    BOSS_LOREWALKER_STONESTEP,
    BOSS_LIU_FLAMEHEART,
    BOSS_SHA_OF_DOUBT,

    MAX_BOSS_DATA
};

enum CreatureIds
{
    NPC_WISE_MARI                   = 56448,
    NPC_CORRUPTED_WATERS_STALKER    = 65393,
    NPC_SPLASH_STALKER              = 56542,
    NPC_CORRUPT_LIVING_WATER        = 56511,
    NPC_FOUNTAIN_STALKER            = 56586,
    NPC_FIRE_HOSE                   = 56574,
    NPC_LOREWALKER_STONESTEP        = 56843,

    NPC_LIU_FLAMEHEART              = 56732,
    NPC_YULON                       = 56762,

	NPC_SERPENT_WARRIOR				= 62171,

	NPC_LESSER_SHA					= 58319,
	NPC_MINION_OF_DOUBT_TERRACE		= 57109,
	NPC_MINION_OF_DOUBT_TEMPLE		= 65362,
};

enum GameobjectIds
{
    GO_WATERY_DOOR                  = 211280,
    GO_FOUNTAIN_OF_EVERSEEING_EXIT  = 213550,
    GO_SCROLLKEEPERS_SANCTUM_EXIT   = 213549,
    GO_DOOR_TO_COURTYARD            = 213544,
    GO_DOOR_TO_SHA_OF_DOUBT         = 213548,
};

enum CreatureGuids
{
    GUID_SPLASH_STALKER_ROOT        = 454688,
};

enum GuidData
{
    DATA_WISE_MARI,
    DATA_CORRUPTED_WATERS_STALKER,
    DATA_SPLASH_STALKER_ROOT,
    DATA_FIRE_HOSE,
    DATA_LOREWALKER_STONESTEP,
    DATA_LIU_FLAMEHEART,
    DATA_YULON,
};

enum InstanceActions
{
    ACTION_TERRACE_FIGHTERS_RETREAT = 1000,
    ACTION_TERRACE_LIU_FLAMEHEART_ENTRANCE
};

enum AreaIds
{
    AREA_TERRACE_OF_THE_TWIN_DRAGONS = 6119,
};

enum TerraceFighterGroups
{
    TERRACE_FIGHTERS_1,
    TERRACE_FIGHTERS_2,
    TERRACE_FIGHTERS_3,
    TERRACE_FIGHTERS_MAX
};

enum CreatureFormation
{
    CREATURE_FORMATION_SERPENT_WARRIORS_1       = 454693,
    CREATURE_FORMATION_SHA_MINIONS_1            = 454740,
    CREATURE_FORMATION_SERPENT_WARRIORS_2       = 454805,
    CREATURE_FORMATION_SHA_MINIONS_2            = 454787,
    CREATURE_FORMATION_SERPENT_WARRIORS_3       = 454770,
    CREATURE_FORMATION_SHA_MINIONS_3            = 454696,
    CREATURE_FORMATION_PATROLING_SHA_MINIONS    = 454838
};

#endif //DEF_TEMPLE_OF_THE_JADE_SERPENT_H_
