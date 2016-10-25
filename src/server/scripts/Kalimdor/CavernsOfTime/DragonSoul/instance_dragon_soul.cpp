/*
* Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
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

#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "PoolMgr.h"
#include "dragon_soul.h"
#include "Player.h"
#include "GameobjectAI.h"

#define MAX_ENCOUNTER 8

/* Dragon Soul encounters:

0 - Morchok
1 - Warlord Zon'ozz
2 - Yor'sahj the Unsleeping
3 - Hagara the Stormbinder
4 - Ultraxion
5 - Warmaster Blackhorn
6 - Spine of Deathwing
7 - Madness of Deathwing

*/

Creatures const DragonSoulBuffExceptions[8] =
{
    { NPC_DEATHWING_WYRMREST_TEMPLE },
    { NPC_FORCE_OF_DESTRUCTION },
    { NPC_PORTENT_OF_TWILIGHT },
    { NPC_HARBINGER_OF_DESTRUCTION },
    { NPC_HARBINGER_OF_TWILIGHT },
    { NPC_FACELESS_DESTROYER },
    { NPC_FACELESS_CORRUPTOR },
    { NPC_RUIN_TENTACLE },
};

enum Events
{
    EVENT_NONE,
    EVENT_ENABLE_TELEPORTERS,
    EVENT_DISABLE_TELEPORTERS,
    EVENT_RESET_MADNESS,
};

enum GroupId
{
    // Wyrmrest Temple Summit
    GROUP_WTS_ASPECTS_PHASE_1,
    GROUP_WTS_ASPECTS_PHASE_2,
    GROUP_WTS_TWILIGHT_FLAMES_1,
    GROUP_WTS_TWILIGHT_FLAMES_2,
    GROUP_WTS_TWILIGHT_FLAMES_3,
    GROUP_WTS_TWILIGHT_FLAMES_4,
    GROUP_WTS_TWILIGHT_FLAMES_5,
    GROUP_WTS_TWILIGHT_FLAMES_6,
    GROUP_WTS_TWILIGHT_FLAMES_7,
    GROUP_WTS_TWILIGHT_FLAMES_8,
    GROUP_WTS_TWILIGHT_FLAMES_9,
    GROUP_WTS_TWILIGHT_FLAMES_10,
    GROUP_WTS_TWILIGHT_FLAMES_11,
    GROUP_WTS_TWILIGHT_FLAMES_12,
};

struct GroupInfo
{
    GroupId Id;
    std::list<uint64> SpawnIds;
};

GroupInfo Groups[] =
{
    {
        { GROUP_WTS_TWILIGHT_FLAMES_1 },
        {
            139,
            140,
            141,
            142,
            143,
            144,
            145,
        }
    },
    {
        { GROUP_WTS_TWILIGHT_FLAMES_2 },
        {
            146,
            147,
            148,
            149,
            150,
            151,
            152,
        }
    },
    {
        { GROUP_WTS_TWILIGHT_FLAMES_3 },
        {
            153,
            154,
            155,
            156,
            157,
            158,
            159,
            160,
            161,
        }
    },
    {
        { GROUP_WTS_TWILIGHT_FLAMES_4 },
        {
            162,
            163,
            164,
            165,
            166,
            167,
            168,
            169,
            170,
        }
    },
    {
        { GROUP_WTS_TWILIGHT_FLAMES_5 },
        {
            171,
            172,
            173,
            174,
            175,
            176,
            177,
            178,
            179,
        }
    },
    {
        { GROUP_WTS_TWILIGHT_FLAMES_6 },
        {
            180,
            181,
            182,
            183,
            184,
            185,
            186,
        }
    },
    {
        { GROUP_WTS_TWILIGHT_FLAMES_7 },
        {
            187,
            188,
            189,
            190,
            191,
            192,
            193,
        }
    },
    {
        { GROUP_WTS_TWILIGHT_FLAMES_8 },
        {
            194,
            195,
            196,
            197,
            198,
        }
    },
    {
        { GROUP_WTS_TWILIGHT_FLAMES_9 },
        {
            199,
            200,
            201,
            202,
            203,
        }
    },
    {
        { GROUP_WTS_TWILIGHT_FLAMES_10 },
        {
            204,
            205,
            206,
            207,
            208,
        }
    },
    {
        { GROUP_WTS_TWILIGHT_FLAMES_11 },
        {
            209,
            210,
            211,
            212,
            213,
            214,
            215,
            216,
            217,
        }
    },
    {
        { GROUP_WTS_TWILIGHT_FLAMES_12 },
        {
            218,
            219,
            220,
            221,
            222,
            223,
            224,
            225,
            226,
        }
    },
};

//struct DSPhaseMask
//{
//    uint64 SpawnId;
//};

std::unordered_map<uint64, uint32> CreaturePhaseMasks = 
{
    /*####################################################
    #              WYRMREST TEMPLE SUMMIT                #
    ####################################################*/
    {
        370699, // Travel to Wyrmrest Base
        (EVENT_PROGRESS_HAGARA |
            EVENT_PROGRESS_HAGARA_END |
            EVENT_PROGRESS_ULTRAXION_TRASH |
            EVENT_PROGRESS_ULTRAXION |
            EVENT_PROGRESS_ULTRAXION_END |
            EVENT_PROGRESS_GUNSHIP |
            EVENT_PROGRESS_SPINE |
            EVENT_PROGRESS_MADNESS |
            EVENT_PROGRESS_DONE)
    },
    {
        370699, // Travel to the Eye of Eternity
        (EVENT_PROGRESS_HAGARA_END |
            EVENT_PROGRESS_ULTRAXION_TRASH |
            EVENT_PROGRESS_ULTRAXION |
            EVENT_PROGRESS_ULTRAXION_END |
            EVENT_PROGRESS_GUNSHIP |
            EVENT_PROGRESS_SPINE |
            EVENT_PROGRESS_MADNESS |
            EVENT_PROGRESS_DONE)
    },
    {
        370744, // Wyrmrest Temple Summit - The Dragon Soul
        (EVENT_PROGRESS_HAGARA |
         EVENT_PROGRESS_HAGARA_END |
         EVENT_PROGRESS_ULTRAXION_TRASH |
         EVENT_PROGRESS_ULTRAXION)
    },
    {
        370746, // Wyrmrest Temple Summit - Ysera the Awakened
        (EVENT_PROGRESS_HAGARA_END |
         EVENT_PROGRESS_ULTRAXION_TRASH |
         EVENT_PROGRESS_ULTRAXION)
    },
    {
        370741, // Wyrmrest Temple Summit - Alexstrasza the Life-Binder
        (EVENT_PROGRESS_HAGARA_END |
         EVENT_PROGRESS_ULTRAXION_TRASH |
         EVENT_PROGRESS_ULTRAXION)
    },
    {
        370747, // Wyrmrest Temple Summit - Kalecgos
        (EVENT_PROGRESS_HAGARA_END |
         EVENT_PROGRESS_ULTRAXION_TRASH |
         EVENT_PROGRESS_ULTRAXION)
    },
    {
        370749, // Wyrmrest Temple Summit - Nozdormu the Timeless One
        (EVENT_PROGRESS_HAGARA_END |
         EVENT_PROGRESS_ULTRAXION_TRASH |
         EVENT_PROGRESS_ULTRAXION)
    },
    {
        370748, // Wyrmrest Temple Summit - Thrall
        (EVENT_PROGRESS_HAGARA_END |
         EVENT_PROGRESS_ULTRAXION_TRASH |
         EVENT_PROGRESS_ULTRAXION)
    },
    {
        131, // Wyrmrest Temple Alliance Gunship - Ka'anu Reevs
        EVENT_PROGRESS_ULTRAXION_END
    },
    {
        132, // Wyrmrest Temple Alliance Gunship - Sky Captain Swayze
        EVENT_PROGRESS_ULTRAXION_END
    },
    {
        133, // Wyrmrest Temple Alliance Gunship - Kalecgos
        EVENT_PROGRESS_ULTRAXION_END
    },
    {
        134, // Wyrmrest Temple Alliance Gunship - Nozdormu the Timeless One
        EVENT_PROGRESS_ULTRAXION_END
    },
    {
        135, // Wyrmrest Temple Alliance Gunship - Ysera the Awakened
        EVENT_PROGRESS_ULTRAXION_END
    },
    {
        136, // Wyrmrest Temple Alliance Gunship - Alexstrasza the Life-Binder
        EVENT_PROGRESS_ULTRAXION_END
    },
    {
        137, // Wyrmrest Temple Alliance Gunship - Thrall
        EVENT_PROGRESS_ULTRAXION_END
    },
};

std::unordered_map<uint64, uint32> GameobjectPhaseMasks =
{
    {
        224761, // The Dragon Wastes - Horde Ship (Phase 1)
        (EVENT_PROGRESS_NONE |
            EVENT_PROGRESS_GENERALS |
            EVENT_PROGRESS_HAGARA |
            EVENT_PROGRESS_HAGARA_END |
            EVENT_PROGRESS_ULTRAXION_TRASH |
            EVENT_PROGRESS_ULTRAXION)
    },
    {
        224763, // The Dragon Wastes - Alliance Ship (Phase 1)
        (EVENT_PROGRESS_NONE |
            EVENT_PROGRESS_GENERALS |
            EVENT_PROGRESS_HAGARA |
            EVENT_PROGRESS_HAGARA_END |
            EVENT_PROGRESS_ULTRAXION_TRASH |
            EVENT_PROGRESS_ULTRAXION)
    },
    {
        224764, // Wyrmrest Temple Summit - Alliance Ship (Phase 2)
        EVENT_PROGRESS_ULTRAXION_END
    }
};

class InstanceWideCreatureAuraUpdateWorker
{
public:
    InstanceWideCreatureAuraUpdateWorker(uint32 spellId, bool apply) : _spellId(spellId), _apply(apply) { }

    void Visit(std::unordered_map<ObjectGuid, Creature*>& creatureMap)
    {
        for (auto const& p : creatureMap)
        {
            if (p.second->IsInWorld())
            {
                if (!_apply && p.second->HasAura(_spellId))
                    p.second->RemoveAurasDueToSpell(_spellId);
                else if (_apply && !p.second->HasAura(_spellId))
                    p.second->AddAura(_spellId, p.second);
            }
        }
    }
    template<class T>
    void Visit(std::unordered_map<ObjectGuid, T*>&) { }

private:
    uint32 _spellId;
    bool _apply;
};

class InstanceWideSetInstanceProgressWorker
{
public:
    InstanceWideSetInstanceProgressWorker(uint32 state) : _state(state) { }

    void Visit(std::unordered_map<ObjectGuid, Creature*>& creatureMap)
    {
        for (auto const& p : creatureMap)
        {
            if (p.second->IsInWorld())
            {
                auto itr = CreaturePhaseMasks.find(p.second->GetSpawnId());
                if (itr != CreaturePhaseMasks.end())
                    p.second->SetInPhase(PHASE_DUNGEON_ALTERNATE, true, (itr->second & _state) == 0);
                if (p.second->IsAIEnabled)
                    p.second->AI()->SetData(DATA_INSTANCE_PROGRESS, _state);
            }
        }
    }
    void Visit(std::unordered_map<ObjectGuid, GameObject*>& gameObjectMap)
    {
        for (auto const& p : gameObjectMap)
        {
            if (p.second->IsInWorld())
            {
                auto itr = CreaturePhaseMasks.find(p.second->GetSpawnId());
                if (itr != CreaturePhaseMasks.end())
                    p.second->SetInPhase(PHASE_DUNGEON_ALTERNATE, true, (itr->second & _state) == 0);
                if (GameObjectAI* ai = p.second->AI())
                    ai->SetData(DATA_INSTANCE_PROGRESS, _state);
            }
        }
    }
    template<class T>
    void Visit(std::unordered_map<ObjectGuid, T*>&) { }

private:
    uint32 _state;
};

ObjectData const creatureData[] =
{
    { NPC_MORCHOK,                      DATA_MORCHOK },
    { NPC_WARMASTER_BLACKHORN,          DATA_WARMASTER_BLACKHORN },
    { NPC_SPINE_OF_DEATHWING,           DATA_SPINE_OF_DEATHWING },
    { NPC_MADNESS_OF_DEATHWING,         DATA_MADNESS_OF_DEATHWING },

    { NPC_LORD_AFRASASTRASZ,            DATA_LORD_AFRASASTRASZ },
    { NPC_DEATHWING_WYRMREST_TEMPLE,    DATA_DEATHWING_WYRMREST_TEMPLE },
    { NPC_TRAVEL_TO_EYE_OF_ETERNITY,    DATA_TRAVEL_TO_EYE_OF_ETERNITY },
    { NPC_DRAGON_SOUL_WYRMREST_TEMPLE,  DATA_DRAGON_SOUL_WYRMREST_TEMPLE },
    { NPC_ALEXSTRASZA_WYRMREST_TEMPLE,  DATA_ALEXSTRASZA_WYRMREST_TEMPLE },
    { NPC_YSERA_WYRMREST_TEMPLE,        DATA_YSERA_WYRMREST_TEMPLE },
    // ADD MORE
    { NPC_THE_SKYFIRE,                  DATA_THE_SKYFIRE },
    { NPC_GUNSHIP_PURSUIT_CONTROLLER,   DATA_GUNSHIP_PURSUIT_CONTROLLER },
    { NPC_DECK_FIRE_CONTROLLER,         DATA_DECK_FIRE_CONTROLLER },
    { NPC_GORIONA,                      DATA_GORIONA },
    { 0,                                0 } // END
};

class instance_dragon_soul : public InstanceMapScript
{
public:
    instance_dragon_soul() : InstanceMapScript("instance_dragon_soul", 967) { }

    struct instance_dragon_soul_InstanceMapScript : public InstanceScript
    {
        instance_dragon_soul_InstanceMapScript(Map* map) : InstanceScript(map)
        {
            SetBossNumber(MAX_ENCOUNTER);
            LoadObjectData(creatureData, nullptr);

            instanceBuffState = NOT_STARTED; // ON
            _ultraxionTrashState = NOT_STARTED;

            _eventsUnfoldHagara = NOT_STARTED;
            _dragonSoulEventProgress = EVENT_PROGRESS_NONE;
        }

        void OnPlayerEnter(Player* player) override
        {
            if (GetBossState(DATA_MADNESS_OF_DEATHWING) == DONE)
                player->CastSpell(static_cast<Unit*>(nullptr), SPELL_MADNESS_SKYBOX_SOOTHE, true);
        }

        void OnPlayerLeave(Player* player)
        {
            // Replace with SpellEffectMgr data fixes
            player->RemoveAura(SPELL_MADNESS_SKYBOX_SOOTHE);
            player->RemoveAura(SPELL_DEGRADATION);
            player->RemoveAura(SPELL_BLOOD_CORRUPTION_DEATH);
            player->RemoveAura(SPELL_BLOOD_CORRUPTION_EARTH);
            player->RemoveAura(SPELL_BLOOD_OF_NELTHARION);
            player->RemoveAura(SPELL_CATACLYSM_SCREEN_EFFECT);
        }

        void OnCreatureCreate(Creature* creature) override
        {
            auto itr = CreaturePhaseMasks.find(creature->GetSpawnId());
            if (itr != CreaturePhaseMasks.end())
            {
                //std::cout << "\nCreature: " << creature->GetName() << " (GUID: " << creature->GetSpawnId() << "), bitmask: " << itr->second << ", progress: " << _dragonSoulEventProgress << ", result: " << ((itr->second & _dragonSoulEventProgress) != 0);
                creature->SetInPhase(PHASE_DUNGEON_ALTERNATE, true, (itr->second & _dragonSoulEventProgress) == 0);
            }

            switch (creature->GetEntry())
            {
                case NPC_LORD_AFRASASTRASZ:
                    _lordAfrasastraszGUIDs[_dragonSoulEventProgress] = creature->GetGUID();
                    break;
                case NPC_TRAVEL_TO_EYE_OF_ETERNITY:
                case NPC_TRAVEL_TO_SKYFIRE:
                case NPC_TRAVEL_TO_THE_MAELSTROM:
                case NPC_TRAVEL_TO_WYRMREST_BASE:
                case NPC_TRAVEL_TO_WYRMREST_SUMMIT:
                case NPC_TRAVEL_TO_WYRMREST_TEMPLE:
                    _dragonSoulTeleporters.push_back(creature->GetGUID());
                    break;
                case NPC_SKY_CAPTAIN_SWAYZE:
                    _skyCaptainSwayzeGUIDs[creature->GetAreaId()] = creature->GetGUID();
                    break;
                case NPC_KAANU_REEVS:
                    _kaanuReevsGUIDs[creature->GetAreaId()] = creature->GetGUID();
                    break;
                case NPC_THE_SKYFIRE:
                    if (!instance->GetCreature(_theSkyfireGUID))
                        _theSkyfireGUID = creature->GetGUID();
                    break;
                case NPC_GUNSHIP_PURSUIT_CONTROLLER:
                    if (!instance->GetCreature(_pursuitControllerGUID))
                        _pursuitControllerGUID = creature->GetGUID();
                    creature->SetDisplayId(creature->GetCreatureTemplate()->Modelid2);
                    break;
                case NPC_GORIONA:
                    if (!instance->GetCreature(_gorionaGUID))
                        _gorionaGUID = creature->GetGUID();
                    break;
                case NPC_WARMASTER_BLACKHORN:
                    if (!instance->GetCreature(_blackhornGUID))
                        _blackhornGUID = creature->GetGUID();
                    break;
                case NPC_FIRE_STALKER:
                case NPC_FIRE_BRIGADE_TARGET_STALKER:
                case NPC_ENGINE_STALKER:
                case NPC_ONSLAUGHT_TARGET:
                case NPC_MASSIVE_EXPLOSION:
                case NPC_CATACLYSM_STALKER:
                    creature->SetDisplayId(creature->GetCreatureTemplate()->Modelid2);
                    break;
                case NPC_DECK_FIRE_CONTROLLER:
                    creature->SetDisplayId(creature->GetCreatureTemplate()->Modelid2);
                    break;
                case NPC_PLATFORM:
                    //if (creature->GetGuidScriptId() == GSID_KALECGOS_PLATFORM)
                    //    _modPlatformKalecgosGUID = creature->GetGUID();
                    //else if (creature->GetGuidScriptId() == GSID_YSERA_PLATFORM)
                    //    _modPlatformYseraGUID = creature->GetGUID();
                    //else if (creature->GetGuidScriptId() == GSID_NOZDORMU_PLATFORM)
                    //    _modPlatformNozdormuGUID = creature->GetGUID();
                    //else if (creature->GetGuidScriptId() == GSID_ALEXSTRASZA_PLATFORM)
                    //    _modPlatformAlexstraszaGUID = creature->GetGUID();
                    //break;
                case NPC_KALECGOS_OUTRO:
                    if (GetBossState(DATA_MADNESS_OF_DEATHWING) != DONE)
                        creature->SetVisible(false);
                    break;
                case NPC_YSERA_OUTRO:
                    if (GetBossState(DATA_MADNESS_OF_DEATHWING) != DONE)
                        creature->SetVisible(false);
                    break;
                case NPC_ALEXSTRASZA_OUTRO:
                    if (GetBossState(DATA_MADNESS_OF_DEATHWING) != DONE)
                        creature->SetVisible(false);
                    break;
                case NPC_NOZDORMU_OUTRO:
                    if (GetBossState(DATA_MADNESS_OF_DEATHWING) != DONE)
                        creature->SetVisible(false);
                    break;
                case NPC_THRALL_OUTRO:
                    if (GetBossState(DATA_MADNESS_OF_DEATHWING) != DONE)
                        creature->SetVisible(false);
                    break;
                case NPC_AGGRA_OUTRO:
                    if (GetBossState(DATA_MADNESS_OF_DEATHWING) != DONE)
                        creature->SetVisible(false);
                    break;
                case NPC_WING_TENTACLE_10_N:
                case NPC_WING_TENTACLE_25_N:
                case NPC_WING_TENTACLE_10_H:
                case NPC_WING_TENTACLE_25_H:
                    if (creature->GetPositionY() > 12200.0f)
                        _modAlexstraszaLimbGUID = creature->GetGUID();
                    /*else
                    _modKalecgosLimbGUID = creature->GetGUID();*/
                    break;
                case NPC_ARM_TENTACLE_YSERA_10_N:
                case NPC_ARM_TENTACLE_YSERA_25_N:
                case NPC_ARM_TENTACLE_YSERA_10_H:
                case NPC_ARM_TENTACLE_YSERA_25_H:
                    _modYseraLimbGUID = creature->GetGUID();
                    break;
                case NPC_ARM_TENTACLE_NOZDORMU_10_N:
                case NPC_ARM_TENTACLE_NOZDORMU_25_N:
                case NPC_ARM_TENTACLE_NOZDORMU_10_H:
                case NPC_ARM_TENTACLE_NOZDORMU_25_H:
                    _modNozdormuLimbGUID = creature->GetGUID();
                    break;
                case NPC_HEMORRHAGE_TARGET:
                    creature->SetReactState(REACT_PASSIVE);
                    creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_NPC);
                    creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
                    creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                    break;
                case NPC_CONGEALING_BLOOD_TARGET:
                    creature->SetReactState(REACT_PASSIVE);
                    creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_NPC);
                    creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
                    creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                    /*if (creature->GetHomePosition() == CongealingTargetOne)
                    CongealingBloodTargetOneGUID = creature->GetGUID();
                    if (creature->GetHomePosition() == CongealingTargetTwo)
                    CongealingBloodTargetTwoGUID = creature->GetGUID();
                    if (creature->GetHomePosition() == CongealingTargetThree)
                    CongealingBloodTargetThreeGUID = creature->GetGUID();
                    if (creature->GetHomePosition() == CongealingTargetFour)
                    CongealingBloodTargetFourGUID = creature->GetGUID();
                    if (creature->GetHomePosition() == CongealingDeathwingHitbox)
                    CongealingDWTargetGUID = creature->GetGUID();*/
                    break;
                case NPC_CORRUPTING_PARASITE_TENTACLE:
                    creature->SetReactState(REACT_PASSIVE);
                    creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_NPC);
                    creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
                    creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                    break;
                default:
                    break;
            }

            if (!creature->IsHostileToPlayers())
                return;

            if (std::find(std::begin(DragonSoulBuffExceptions), std::end(DragonSoulBuffExceptions),
                creature->GetEntry()) == std::end(DragonSoulBuffExceptions))
                return;

            /*for (uint8 i = 0; i < sizeof(DragonSoulBuffExceptions); i++)
            if (creature->GetEntry() == DragonSoulBuffExceptions[i])
            return;*/

            /*if (instance->GetSpawnMode() == RAID_DIFFICULTY_RAID_FINDER)
            creature->AddAura(SPELL_PRESENCE_OF_THE_DRAGON_SOUL, creature);
            else*/
            if (instanceBuffState != DONE)
                creature->AddAura(SPELL_POWER_OF_THE_ASPECTS_35, creature);
        }

        void OnCreatureRemove(Creature* creature) override
        {
            if (creature->GetEntry() == NPC_ULTRAXION_GAUNTLET)
                _ultraxionGauntletGUID = ObjectGuid::Empty;
        }

        void OnGameObjectCreate(GameObject* go) override
        {
            auto itr = GameobjectPhaseMasks.find(go->GetSpawnId());
            if (itr != GameobjectPhaseMasks.end())
            {
                //std::cout << "\Gameobject: " << go->GetName() << " (GUID: " << go->GetSpawnId() << "), bitmask: " << itr->second << ", progress: " << _dragonSoulEventProgress << ", result: " << ((itr->second & _dragonSoulEventProgress) != 0);
                go->SetInPhase(PHASE_DUNGEON_ALTERNATE, true, (itr->second & _dragonSoulEventProgress) == 0);
            }

            switch (go->GetEntry())
            {
                case GO_THE_FOCUSING_IRIS:
                    _theFocusingIrisGUID = go->GetGUID();
                    if (GetData(DATA_HAGARA_THE_STORMBINDER) != NOT_STARTED)
                        go->SetInPhase(PHASE_DUNGEON_ALTERNATE, true, true);
                    break;
                case GO_ALLIANCE_SHIP_1:
                    _allianceShipPhaseOneGUID = go->GetGUID();
                    //if (_dragonSoulEventProgress != EVENT_PROGRESS_GUNSHIP)
                    //    go->SetInPhase(PHASE_DUNGEON_ALTERNATE, true, true);
                    break;
                case GO_ALLIANCE_SHIP_2:
                    _allianceShipPhaseTwoGUID = go->GetGUID();
                    //if (_dragonSoulEventProgress >= EVENT_PROGRESS_GUNSHIP)
                    //    go->SetInPhase(PHASE_DUNGEON_ALTERNATE, true, true);
                    break;
                case GO_DEATHWING_BACK_PLATE_1:
                    _deathwingPlateOneGUID = go->GetGUID();
                    break;
                case GO_DEATHWING_BACK_PLATE_2:
                    _deathwingPlateTwoGUID = go->GetGUID();
                    break;
                case GO_DEATHWING_BACK_PLATE_3:
                    _deathwingPlateThreeGUID = go->GetGUID();
                    break;
                default:
                    break;
            }
        }

        static ObjectGuid GetObjectGuidInMapByIndex(std::unordered_map<uint32, ObjectGuid> map, uint32 index)
        {
            std::unordered_map<uint32, ObjectGuid>::const_iterator itr = map.find(index);
            if (itr != map.end())
                return itr->second;

            return ObjectGuid::Empty;
        }

        ObjectGuid GetGuidData(uint32 type) const override
        {
            switch (type)
            {
                case DATA_SKY_CAPTAIN_SWAYZE_WYRMREST_TEMPLE:
                    return GetObjectGuidInMapByIndex(_skyCaptainSwayzeGUIDs, AREA_DRAGON_SOUL);
                case DATA_SKY_CAPTAIN_SWAYZE_GUNSHIP:
                    return GetObjectGuidInMapByIndex(_skyCaptainSwayzeGUIDs, AREA_ABOVE_THE_FROZEN_SEA);
                case DATA_SKY_CAPTAIN_SWAYZE_SPINE:
                    return GetObjectGuidInMapByIndex(_skyCaptainSwayzeGUIDs, AREA_DEATHWING);
                case NPC_DRAGON_SOUL_WYRMREST_TEMPLE:
                    return _dragonSoulGUID;
                case NPC_ALEXSTRASZA_WYRMREST_TEMPLE:
                    return _alexstraszaWtGUID;
                case NPC_YSERA_WYRMREST_TEMPLE:
                    return _yseraWtGUID;
                case NPC_KALECGOS_WYRMREST_TEMPLE:
                    return _kalecgosWtGUID;
                case NPC_THRALL_WYRMREST_TEMPLE:
                    return _thrallWtGUID;
                case NPC_NOZDORMU_WYRMREST_TEMPLE:
                    return _nozdormuWtGUID;
                case NPC_TRAVEL_TO_EYE_OF_ETERNITY:
                    return _travelToEyeOfEternityGUID;
                case NPC_ULTRAXION_GAUNTLET:
                    return _ultraxionGauntletGUID;
                case NPC_THE_SKYFIRE:
                    return _theSkyfireGUID;
                case NPC_GUNSHIP_PURSUIT_CONTROLLER:
                    return _pursuitControllerGUID;
                case NPC_DECK_FIRE_CONTROLLER:
                    return _deckFireControllerGUID;
                case NPC_WARMASTER_BLACKHORN:
                    return _blackhornGUID;
                case NPC_GORIONA:
                    return _gorionaGUID;
                case NPC_SPINE_OF_DEATHWING:
                    return _spineOfDeathwingGUID;

                case GO_DEATHWING_BACK_PLATE_1:
                    return _deathwingPlateOneGUID;
                case GO_DEATHWING_BACK_PLATE_2:
                    return _deathwingPlateTwoGUID;
                case GO_DEATHWING_BACK_PLATE_3:
                    return _deathwingPlateThreeGUID;

                case NPC_MADNESS_OF_DEATHWING:
                    return _modDeathwingGUID;
                case NPC_MOD_THRALL:
                    return _modThrallGUID;
                case NPC_MOD_KALECGOS:
                    return _modKalecgosGUID;
                case NPC_MOD_YSERA:
                    return _modYseraGUID;
                case NPC_MOD_NOZDORMU:
                    return _modNozdormuGUID;
                case NPC_MOD_ALEXSTRASZA:
                    return _modAlexstraszaGUID;
                case NPC_PLATFORM:
                    /*creature->SetReactState(REACT_PASSIVE);
                    if (creature->GetHomePosition() == KalecgosPlatformPos)
                    PlatformGUIDs[0] = creature->GetGUID();
                    if (creature->GetHomePosition() == YseraPlatformPos)
                    PlatformGUIDs[1] = creature->GetGUID();
                    if (creature->GetHomePosition() == NozdormuPlatformPos)
                    PlatformGUIDs[2] = creature->GetGUID();
                    if (creature->GetHomePosition() == AlexstraszaPlatformPos)
                    PlatformGUIDs[3] = creature->GetGUID();*/
                    break;
                case NPC_MOD_DRAGON_SOUL:
                    return _modDragonSoulGUID;
                case NPC_KALECGOS_OUTRO:
                    return _modKalecgosOutroGUID;
                case NPC_YSERA_OUTRO:
                    return _modYseraOutroGUID;
                case NPC_ALEXSTRASZA_OUTRO:
                    return _modAlexstraszaOutroGUID;
                case NPC_NOZDORMU_OUTRO:
                    return _modNozdormuOutroGUID;
                case NPC_THRALL_OUTRO:
                    return _modThrallOutroGUID;
                case NPC_AGGRA_OUTRO:
                    return _modAggraOutroGUID;
                case NPC_MADNESS_OF_DEATHWING_HEAD:
                    return _modDeathwingHeadGUID;
                case DATA_MOD_PLATFORM_KALEGOS:
                    return _modPlatformKalecgosGUID;
                case DATA_MOD_PLATFORM_YSERA:
                    return _modPlatformYseraGUID;
                case DATA_MOD_PLATFORM_NOZDORMU:
                    return _modPlatformNozdormuGUID;
                case DATA_MOD_PLATFORM_ALEXSTRASZA:
                    return _modPlatformAlexstraszaGUID;
                default:
                    break;
            }

            return ObjectGuid::Empty;
        }

        uint32 GetData(uint32 type) const override
        {
            switch (type)
            {
                case DATA_DS_BUFF_STATE:
                    return instanceBuffState;
                case DATA_DRAGON_SOUL_EVENT_PROGRESS:
                    return uint32(_dragonSoulEventProgress);
                case DATA_ULTRAXION_TRASH:
                    return _ultraxionTrashState;
                case DATA_DECK_DEFENDER:
                    return uint32(_deckDefended);
                case DATA_SPINE_LOOTED:
                    return _spineLooted ? TRUE : FALSE;
                case DATA_EVENTS_UNFOLD_HAGARA:
                    return _eventsUnfoldHagara;
                default:
                    break;
            }

            return 0;
        }

        void SetData(uint32 type, uint32 data) override
        {
            switch (type)
            {
                case DATA_DS_BUFF_STATE:
                    if (instanceBuffState == data)
                        break;

                    instanceBuffState = data;
                    switch (instanceBuffState)
                    {
                        case IN_PROGRESS:
                        case DONE:
                        {
                            InstanceWideCreatureAuraUpdateWorker worker(SPELL_POWER_OF_THE_ASPECTS_35, instanceBuffState == IN_PROGRESS);
                            TypeContainerVisitor<InstanceWideCreatureAuraUpdateWorker, MapStoredObjectTypesContainer> visitor(worker);
                            visitor.Visit(instance->GetObjectsStore());
                        }
                        break;
                        default:
                            break;
                    }
                    break;
                case DATA_DRAGON_SOUL_EVENT_PROGRESS:
                    //SetDragonSoulProgress(DragonSoulEventProgress(data));
                    break;
                case DATA_ULTRAXION_TRASH:
                    _ultraxionTrashState = data;
                    break;
                case DATA_DECK_DEFENDER:
                    _deckDefended = data != 0;
                    break;
                case DATA_SPINE_LOOTED:
                    _spineLooted = data == TRUE;
                    break;
                case DATA_EVENTS_UNFOLD_HAGARA:
                    _eventsUnfoldHagara = data;
                    break;
                default:
                    break;
            }
        }

        void HandleDragonSoulTeleporters(DSAreas area = DSAreas(NULL), bool enable = true)
        {
            for (auto guid : _lordAfrasastraszGUIDs)
            {
                if (Creature* afrastrasz = instance->GetCreature(guid.second))
                    if (enable)
                        afrastrasz->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                    else
                        afrastrasz->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            }

            for (auto guid : _dragonSoulTeleporters)
                if (Creature* teleporter = instance->GetCreature(guid))
                {
                    if (enable)
                    {
                        teleporter->AddAura(SPELL_TELEPORTER_ACTIVE, teleporter);
                        teleporter->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
                    }
                    else
                    {
                        teleporter->RemoveAura(SPELL_TELEPORTER_ACTIVE);
                        teleporter->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
                    }
                }
            /*for (auto guid : _dragonSoulTeleporters)
            if (Creature* teleporter = instance->GetCreature(guid))
            if (teleporter->GetAreaId() == area || area == DSAreas(NULL))
            if (enable)
            {
            teleporter->AddAura(SPELL_TELEPORTER_ACTIVE, teleporter);
            teleporter->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
            }
            else
            {
            teleporter->RemoveAura(SPELL_TELEPORTER_ACTIVE);
            teleporter->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
            }*/

        }

        //bool SetBossState(uint32 type, EncounterState state) override
        //{
        //    if (!InstanceScript::SetBossState(type, state))
        //        return false;

        //    if (state == IN_PROGRESS)
        //    {
        //        events.CancelEvent(EVENT_ENABLE_TELEPORTERS);
        //        events.ScheduleEvent(EVENT_DISABLE_TELEPORTERS, 3 * IN_MILLISECONDS);
        //    }
        //    else if (state == FAIL)
        //    {
        //        events.CancelEvent(EVENT_DISABLE_TELEPORTERS);
        //        events.ScheduleEvent(EVENT_ENABLE_TELEPORTERS, 3 * IN_MILLISECONDS);
        //    }

        //    switch (type)
        //    {
        //        case DATA_MORCHOK:
        //            if (state == DONE)
        //                SetDragonSoulProgress(EVENT_PROGRESS_ASSAULT_GENERALS);
        //            break;
        //        case DATA_WARLORD_ZONOZZ:
        //        case DATA_YORSAHJ_THE_UNSLEEPING:
        //            if (state == DONE)
        //                SetDragonSoulProgress(EVENT_PROGRESS_GENERAL_DEAD);

        //            /*if (_dragonSoulEventProgress == EVENT_PROGRESS_ASSAULT_GENERALS)
        //                SetDragonSoulProgress(EVENT_PROGRESS_FIRST_GENERAL_DEAD);
        //            else if (_dragonSoulEventProgress = EVENT_PROGRESS_FIRST_GENERAL_DEAD)
        //                SetDragonSoulProgress(EVENT_PROGRESS_HAGARA);*/
        //            break;
        //        case DATA_HAGARA_THE_STORMBINDER:
        //            if (state == DONE)
        //                SetDragonSoulProgress(EVENT_PROGRESS_ULTRAXION_INTRO);
        //        case DATA_ULTRAXION:
        //            if (state == DONE)
        //                SetDragonSoulProgress(EVENT_PROGRESS_ULTRAXION_OUTRO);
        //        case DATA_WARMASTER_BLACKHORN:
        //            if (state == IN_PROGRESS)
        //            {
        //                instance->LoadGrid(13617.98f, -12102.97f);
        //                instance->SetUnloadLock(Trinity::ComputeGridCoord(13617.98f, -12102.97f), true);
        //            }
        //            else
        //            {
        //                instance->SetUnloadLock(Trinity::ComputeGridCoord(13617.98f, -12102.97f), false);
        //            }
        //        case DATA_SPINE_OF_DEATHWING:
        //            break;
        //        default:
        //            break;
        //    }

        //    return true;
        //}

        bool SetBossState(uint32 type, EncounterState state) override
        {
            if (!InstanceScript::SetBossState(type, state))
                return false;

            if (state == IN_PROGRESS)
            {
                events.CancelEvent(EVENT_ENABLE_TELEPORTERS);
                events.ScheduleEvent(EVENT_DISABLE_TELEPORTERS, 3 * IN_MILLISECONDS);
            }
            else if (state == FAIL || state == NOT_STARTED)
            {
                events.CancelEvent(EVENT_DISABLE_TELEPORTERS);
                events.ScheduleEvent(EVENT_ENABLE_TELEPORTERS, 3 * IN_MILLISECONDS);
            }

            switch (type)
            {
                case DATA_MORCHOK:
                    //if (state == DONE)
                        //SetDragonSoulProgress(EVENT_PROGRESS_GENERALS);
                    break;
                case DATA_WARLORD_ZONOZZ:
                    /*if (state == DONE && GetBossState(DATA_YORSAHJ_THE_UNSLEEPING))
                        SetDragonSoulProgress(EVENT_PROGRESS_HAGARA);*/
                case DATA_YORSAHJ_THE_UNSLEEPING:
                    /*if (state == DONE && GetBossState(DATA_WARLORD_ZONOZZ))
                        SetDragonSoulProgress(EVENT_PROGRESS_HAGARA);*/
                    break;
                case DATA_HAGARA_THE_STORMBINDER:
                    /*if (state == DONE)
                        SetDragonSoulProgress(EVENT_PROGRESS_ULTRAXION);*/
                case DATA_ULTRAXION:
                    /*if (state == DONE)
                        SetDragonSoulProgress(EVENT_PROGRESS_GUNSHIP);*/
                case DATA_WARMASTER_BLACKHORN:
                    if (state == IN_PROGRESS)
                    {
                        instance->LoadGrid(13617.98f, -12102.97f);
                        instance->SetUnloadLock(Trinity::ComputeGridCoord(13617.98f, -12102.97f), true);
                    }
                    else
                    {
                        instance->SetUnloadLock(Trinity::ComputeGridCoord(13617.98f, -12102.97f), false);
                    }
/*
                    if (state == DONE)
                        SetDragonSoulProgress(EVENT_PROGRESS_SPINE);*/
                case DATA_SPINE_OF_DEATHWING:
                    if (state == DONE)
                    {
                        //SetDragonSoulProgress(EVENT_PROGRESS_MADNESS);
                        if (Creature* thrall = instance->GetCreature(_modThrallGUID))
                            if (thrall->IsAIEnabled)
                                thrall->AI()->SetData(DATA_SPINE_OF_DEATHWING, TRUE);
                    }
                    break;
                case DATA_MADNESS_OF_DEATHWING:
                    break;
                default:
                    break;
            }

            return true;
        }

        void SetDragonSoulProgress(DragonSoulEventProgress eventId)
        {
            if (_dragonSoulEventProgress == eventId)
                return;

            _dragonSoulEventProgress = eventId;
            UpdateInstanceProgress(_dragonSoulEventProgress);

            switch (eventId)
            {
                default:
                    break;
            }
        }

        void UpdateInstanceProgress(uint32 eventId) const
        {
            InstanceWideSetInstanceProgressWorker worker(eventId);
            TypeContainerVisitor<InstanceWideSetInstanceProgressWorker, MapStoredObjectTypesContainer> visitor(worker);
            visitor.Visit(instance->GetObjectsStore());
        }

        /*void SetDragonSoulProgress(DragonSoulEventProgress eventId)
        {
        if (_dragonSoulEventProgress == eventId)
        return;

        _dragonSoulEventProgress = eventId;

        switch (eventId)
        {
        case EVENT_PROGRESS_MORCHOK_INTRO:
        break;
        case EVENT_PROGRESS_MORCHOK_END:
        break;
        case EVENT_PROGRESS_ASSAULT_GENERALS:
        case EVENT_PROGRESS_FIRST_GENERAL_DEAD:
        case EVENT_PROGRESS_HAGARA:
        break;
        case EVENT_PROGRESS_HAGARA_OUTRO:
        if (Creature* deathwing = instance->GetCreature(_deathwingGUID))
        if (deathwing->IsAIEnabled)
        deathwing->AI()->SetData(DATA_DEATHWING_MOVEMENT, DEATHWING_MOVEMENT_SUMMIT);
        if (Creature* kalecgos = instance->GetCreature(_kalecgosWtGUID))
        kalecgos->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
        break;
        case EVENT_PROGRESS_ULTRAXION_INTRO:
        case EVENT_PROGRESS_ULTRAXION:
        if (Creature* thrall = instance->GetCreature(_thrallWtGUID))
        thrall->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
        break;
        case EVENT_PROGRESS_ULTRAXION_OUTRO:
        if (Creature* deathwing = instance->GetCreature(_deathwingGUID))
        deathwing->DespawnOrUnsummon();
        break;
        case EVENT_PROGRESS_SIEGE_ENDED:
        break;
        default:
        break;
        }
        }*/

        void WriteSaveDataMore(std::ostringstream& data) override
        {
            data << instanceBuffState;
        }

        void ReadSaveDataMore(std::istringstream& data) override
        {
            data >> instanceBuffState;
            _spineLooted = GetBossState(DATA_SPINE_OF_DEATHWING) == DONE ? true : false;
        }

        void Update(uint32 diff) override
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_ENABLE_TELEPORTERS:
                        HandleDragonSoulTeleporters((DSAreas)0, true);
                        break;
                    case EVENT_DISABLE_TELEPORTERS:
                        HandleDragonSoulTeleporters((DSAreas)0, false);
                        break;
                    case EVENT_RESET_MADNESS:
                        if (GetBossState(DATA_MADNESS_OF_DEATHWING) == FAIL)
                            SetBossState(DATA_MADNESS_OF_DEATHWING, NOT_STARTED);
                        break;
                    default:
                        break;
                }
            }
        }

        bool IsEncounterInProgress() const override
        {
            for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
                if (GetBossState(i) == IN_PROGRESS)
                    return true;

            return false;
        }

    protected:
        DragonSoulEventProgress _dragonSoulEventProgress;
        uint32 instanceBuffState;
        bool _spineLooted;

        uint32 _eventsUnfoldHagara;

        //Creatures
        //ObjectGuid _yseraHagaraIntroGUID;
        //ObjectGuid _alexstraszaHagaraIntroGUID;
        //ObjectGuid _nozdormuHagaraIntroGUID;
        //ObjectGuid _thrallHagaraIntroGUID;
        //ObjectGuid _kalecgosHagaraIntroGUID;
        ObjectGuid _travelToEyeOfEternityGUID;

        std::unordered_map<uint32, ObjectGuid> _lordAfrasastraszGUIDs; // Indexed by instance progress
        ObjectGuid _deathwingGUID;
        ObjectGuid _dragonSoulGUID;
        ObjectGuid _alexstraszaWtGUID;
        ObjectGuid _yseraWtGUID;
        ObjectGuid _kalecgosWtGUID;
        ObjectGuid _thrallWtGUID;
        ObjectGuid _nozdormuWtGUID;
        ObjectGuid _ultraxionGauntletGUID;
        std::unordered_map<uint32, ObjectGuid> _skyCaptainSwayzeGUIDs; // Indexed by Area Id
        std::unordered_map<uint32, ObjectGuid> _kaanuReevsGUIDs; // Indexed by Area Id
        ObjectGuid _theSkyfireGUID;
        ObjectGuid _pursuitControllerGUID;
        ObjectGuid _deckFireControllerGUID;
        ObjectGuid _gorionaGUID;
        ObjectGuid _blackhornGUID;
        ObjectGuid _spineOfDeathwingGUID;

        ObjectGuid _modDeathwingGUID;
        ObjectGuid _modDeathwingHeadGUID;
        ObjectGuid _modPlatformKalecgosGUID;
        ObjectGuid _modPlatformYseraGUID;
        ObjectGuid _modPlatformNozdormuGUID;
        ObjectGuid _modPlatformAlexstraszaGUID;
        ObjectGuid _modThrallGUID;
        ObjectGuid _modKalecgosGUID;
        ObjectGuid _modYseraGUID;
        ObjectGuid _modNozdormuGUID;
        ObjectGuid _modAlexstraszaGUID;
        ObjectGuid PlatformGUIDs[4];
        ObjectGuid _modDragonSoulGUID;
        ObjectGuid _modThrallOutroGUID;
        ObjectGuid _modKalecgosOutroGUID;
        ObjectGuid _modYseraOutroGUID;
        ObjectGuid _modNozdormuOutroGUID;
        ObjectGuid _modAlexstraszaOutroGUID;
        ObjectGuid _modAggraOutroGUID;
        ObjectGuid _modKalecgosLimbGUID;
        ObjectGuid _modYseraLimbGUID;
        ObjectGuid _modNozdormuLimbGUID;
        ObjectGuid _modAlexstraszaLimbGUID;
        ObjectGuid _modCongealingBloodTargetOneGUID;
        ObjectGuid _modCongealingBloodTargetTwoGUID;
        ObjectGuid _modCongealingBloodTargetThreeGUID;
        ObjectGuid _modCongealingBloodTargetFourGUID;
        ObjectGuid _modCongealingDWTargetGUID;
        ObjectGuid _jumpPadYKGUID;
        ObjectGuid _jumpPadKYGUID;
        ObjectGuid _jumpPadNYGUID;
        ObjectGuid _jumpPadYNGUID;
        ObjectGuid _jumpPadANGUID;
        ObjectGuid _jumpPadNAGUID;

        uint32 _firstAspectAssaulted;
        uint32 _spineLootedEvent;

        //Lists of Creatures
        std::list<ObjectGuid> _dragonSoulTeleporters;
        std::list<ObjectGuid> _morchokEventGroupOne;

        //GameObjects
        ObjectGuid _theFocusingIrisGUID;
        ObjectGuid _allianceShipPhaseOneGUID;
        ObjectGuid _allianceShipPhaseTwoGUID;
        ObjectGuid _deathwingPlateOneGUID;
        ObjectGuid _deathwingPlateTwoGUID;
        ObjectGuid _deathwingPlateThreeGUID;

        uint32 _ultraxionTrashState;

        bool _deckDefended;

        EventMap events;
    };

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new instance_dragon_soul_InstanceMapScript(map);
    }
};

void AddSC_instance_dragon_soul()
{
    new instance_dragon_soul();
}
