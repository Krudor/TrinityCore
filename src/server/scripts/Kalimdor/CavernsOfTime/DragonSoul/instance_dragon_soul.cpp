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
    { NPC_DEATHWING },
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

ObjectData const creatureData[] =
{
    { NPC_SPINE_OF_DEATHWING, DATA_SPINE_OF_DEATHWING }
};

class instance_dragon_soul : public InstanceMapScript
{
    public:
        instance_dragon_soul() : InstanceMapScript("instance_dragon_soul", 967) { }

        struct instance_dragon_soul_InstanceMapScript : public InstanceScript
        {
            instance_dragon_soul_InstanceMapScript(Map* map) : InstanceScript(map)
            {
                LoadObjectData(creatureData, nullptr);
                SetBossNumber(MAX_ENCOUNTER);
				_disabledDSBuff = NOT_STARTED; //Untouched (ON)
                _ultraxionTrashState = NOT_STARTED;

                _eventsUnfoldHagara = NOT_STARTED;

                _dragonSoulTeleporters.clear();
                _morchokEventGroupOne.clear();
                memset(PlatformGUIDs, 0, sizeof(PlatformGUIDs));

                _deckDefended = true;
				_dragonSoulEventProgress = EVENT_PROGRESS_NONE;
                SetBossState(DATA_SPINE_OF_DEATHWING, NOT_STARTED);
                SetBossState(DATA_MADNESS_OF_DEATHWING, NOT_STARTED);
                _spineLooted = true;
            }

            void OnPlayerEnter(Player* player) override
            {
                if (GetBossState(DATA_MADNESS_OF_DEATHWING) == DONE)
                    player->CastSpell((Unit*)NULL, SPELL_MADNESS_SKYBOX_SOOTHE, true);
            }

            void OnPlayerLeave(Player* player)
            {
                player->RemoveAura(SPELL_DEGRADATION);
                player->RemoveAura(SPELL_BLOOD_CORRUPTION_DEATH);
                player->RemoveAura(SPELL_BLOOD_CORRUPTION_EARTH);
                player->RemoveAura(SPELL_BLOOD_OF_NELTHARION);
                player->RemoveAura(106527);
            }

            void OnCreatureCreate(Creature* creature) override
            {
                switch (creature->GetEntry())
                {
                    case NPC_MORCHOK:
                        if (!instance->GetCreature(_morchokGUID))
                            _morchokGUID = creature->GetGUID();
                        break;
                    case NPC_LORD_AFRASASTRASZ:
                        if (!instance->GetCreature(_lordAfrasastraszGUID))
                        _lordAfrasastraszGUID = creature->GetGUID();
                        break;
                    case NPC_TRAVEL_TO_EYE_OF_ETERNITY:
                        _travelToEyeOfEternityGUID = creature->GetGUID();
                        _dragonSoulTeleporters.push_back(creature->GetGUID());
                        creature->SetInPhase(PHASE_DUNGEON_ALTERNATE, true, true);
                        break;
                    case NPC_TRAVEL_TO_SKYFIRE:
                    case NPC_TRAVEL_TO_THE_MAELSTROM:
                    case NPC_TRAVEL_TO_WYRMREST_BASE:
                    case NPC_TRAVEL_TO_WYRMREST_SUMMIT:
                    case NPC_TRAVEL_TO_WYRMREST_TEMPLE:
                        _dragonSoulTeleporters.push_back(creature->GetGUID());
                        break;
                    case NPC_ANDORGOS:
                        //creature->m_Events.AddEvent(new DelayedSpellEvent(creature, creature, SPELL_ARCANE_CHANNELING, false), creature->m_Events.CalculateTime(500));
                        break;
                    case NPC_DEATHWING:
                        if (!instance->GetCreature(_deathwingGUID))
                            _deathwingGUID = creature->GetGUID();
                        break;
                    case NPC_DRAGON_SOUL_WT:
                        _dragonSoulGUID = creature->GetGUID();
                        if (_dragonSoulEventProgress >= EVENT_PROGRESS_GUNSHIP)
                            creature->SetInPhase(PHASE_DUNGEON_ALTERNATE, true, true);
                        break;
                    case NPC_ALEXSTRASZA_WYRMREST_TEMPLE:
                        _alexstraszaWtGUID = creature->GetGUID();
                        /*if (_dragonSoulEventProgress != EVENT_PROGRESS_ULTRAXION)*/
                            creature->SetInPhase(PHASE_DUNGEON_ALTERNATE, true, true);
                        break;
                    case NPC_YSERA_WYRMREST_TEMPLE:
                        _yseraWtGUID = creature->GetGUID();
                        /*if (_dragonSoulEventProgress != EVENT_PROGRESS_ULTRAXION)*/
                            creature->SetInPhase(PHASE_DUNGEON_ALTERNATE, true, true);
                        break;
                    case NPC_KALECGOS_WYRMREST_TEMPLE:
                        _kalecgosWtGUID = creature->GetGUID();
                        /*if (_dragonSoulEventProgress != EVENT_PROGRESS_ULTRAXION)*/
                            creature->SetInPhase(PHASE_DUNGEON_ALTERNATE, true, true);
                        break;
                    case NPC_THRALL_WYRMREST_TEMPLE:
                        _thrallWtGUID = creature->GetGUID();
                        /*if (_dragonSoulEventProgress != EVENT_PROGRESS_ULTRAXION)*/
                            creature->SetInPhase(PHASE_DUNGEON_ALTERNATE, true, true);
                        break;
                    case NPC_NOZDORMU_WYRMREST_TEMPLE:
                        _nozdormuWtGUID = creature->GetGUID();
                        /*if (_dragonSoulEventProgress != EVENT_PROGRESS_ULTRAXION)*/
                            creature->SetInPhase(PHASE_DUNGEON_ALTERNATE, true, true);
                        break;
                    case NPC_ULTRAXION_GAUNTLET:
                        if (!instance->GetCreature(_ultraxionGauntletGUID))
                            _ultraxionGauntletGUID = creature->GetGUID();
                        break;
                    case NPC_SKY_CAPTAIN_SWAYZE:
                        if (creature->GetAreaId() == AREA_ABOVE_THE_FROZEN_SEA && !instance->GetCreature(_skyCaptainSwayzeGUID))
                            _skyCaptainSwayzeGUID = creature->GetGUID();
                        break;
                    case NPC_KAANU_REEVS:
                        if (!instance->GetCreature(_kaanuReevsGUID))
                            _kaanuReevsGUID = creature->GetGUID();
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
                        if (!instance->GetCreature(_deckFireControllerGUID))
                            _deckFireControllerGUID = creature->GetGUID();
                        break;
                    case NPC_SPINE_OF_DEATHWING:
                        if (!instance->GetCreature(_spineOfDeathwingGUID))
                            _spineOfDeathwingGUID = creature->GetGUID();
                        break;
                    case NPC_MADNESS_OF_DEATHWING:
                        if (!instance->GetCreature(_modDeathwingGUID))
                            _modDeathwingGUID = creature->GetGUID();
                        break;
                    case NPC_MADNESS_OF_DEATHWING_HEAD_10_N:
                    case NPC_MADNESS_OF_DEATHWING_HEAD_25_N:
                    case NPC_MADNESS_OF_DEATHWING_HEAD_10_H:
                    case NPC_MADNESS_OF_DEATHWING_HEAD_25_H:
                        if (!instance->GetCreature(_modDeathwingHeadGUID))
                            _modDeathwingHeadGUID = creature->GetGUID();
                        break;
                    case NPC_MOD_THRALL:
                        _modThrallGUID = creature->GetGUID();
                        break;
                    case NPC_MOD_KALECGOS:
                        _modKalecgosGUID = creature->GetGUID();
                        break;
                    case NPC_MOD_YSERA:
                        _modYseraGUID = creature->GetGUID();
                        break;
                    case NPC_MOD_NOZDORMU:
                        _modNozdormuGUID = creature->GetGUID();
                        break;
                    case NPC_MOD_ALEXSTRASZA:
                        _modAlexstraszaGUID = creature->GetGUID();
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
                        break;
                    case NPC_MOD_DRAGON_SOUL:
                        if (!instance->GetCreature(_modDragonSoulGUID))
                            _modDragonSoulGUID = creature->GetGUID();
                        break;
                    case NPC_KALECGOS_OUTRO:
                        if (GetBossState(DATA_MADNESS_OF_DEATHWING) != DONE)
                            creature->SetVisible(false);
                        if (instance->GetCreature(_modKalecgosOutroGUID))
                            _modKalecgosOutroGUID = creature->GetGUID();
                        break;
                    case NPC_YSERA_OUTRO:
                        if (GetBossState(DATA_MADNESS_OF_DEATHWING) != DONE)
                            creature->SetVisible(false);
                        if (instance->GetCreature(_modYseraOutroGUID))
                            _modYseraOutroGUID = creature->GetGUID();
                        break;
                    case NPC_ALEXSTRASZA_OUTRO:
                        if (GetBossState(DATA_MADNESS_OF_DEATHWING) != DONE)
                            creature->SetVisible(false);
                        if (instance->GetCreature(_modAlexstraszaOutroGUID))
                            _modAlexstraszaOutroGUID = creature->GetGUID();
                        break;
                    case NPC_NOZDORMU_OUTRO:
                        if (GetBossState(DATA_MADNESS_OF_DEATHWING) != DONE)
                            creature->SetVisible(false);
                        if (instance->GetCreature(_modNozdormuOutroGUID))
                            _modNozdormuOutroGUID = creature->GetGUID();
                        break;
                    case NPC_THRALL_OUTRO:
                        if (GetBossState(DATA_MADNESS_OF_DEATHWING) != DONE)
                            creature->SetVisible(false);
                        if (instance->GetCreature(_modThrallOutroGUID))
                            _modThrallOutroGUID = creature->GetGUID();
                        break;
                    case NPC_AGGRA_OUTRO:
                        if (GetBossState(DATA_MADNESS_OF_DEATHWING) != DONE)
                            creature->SetVisible(false);
                        if (instance->GetCreature(_modAggraOutroGUID))
                            _modAggraOutroGUID = creature->GetGUID();
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
				if (_disabledDSBuff != DONE)
					creature->AddAura(SPELL_POWER_OF_THE_ASPECTS_35, creature);
            }

            void OnCreatureRemove(Creature* creature) override
            {
				if (creature->GetEntry() == NPC_ULTRAXION_GAUNTLET)
					_ultraxionGauntletGUID = ObjectGuid::Empty;
            }

            void OnGameObjectCreate(GameObject* go) override
            {
                switch (go->GetEntry())
                {
                    case GO_THE_FOCUSING_IRIS:
                        _theFocusingIrisGUID = go->GetGUID();
                        if (GetData(DATA_HAGARA_THE_STORMBINDER) != NOT_STARTED)
                            go->SetInPhase(PHASE_DUNGEON_ALTERNATE, true, true);
                        break;
					case GO_ALLIANCE_SHIP_1:
						_allianceShipPhaseOneGUID = go->GetGUID();
                        if (_dragonSoulEventProgress != EVENT_PROGRESS_GUNSHIP)
                            go->SetInPhase(PHASE_DUNGEON_ALTERNATE, true, true);
						break;
                    case GO_ALLIANCE_SHIP_2:
                        _allianceShipPhaseTwoGUID = go->GetGUID();
                        if (_dragonSoulEventProgress >= EVENT_PROGRESS_GUNSHIP)
                            go->SetInPhase(PHASE_DUNGEON_ALTERNATE, true, true);
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

            ObjectGuid GetGuidData(uint32 type) const override
            {
                switch (type)
                {
                    case NPC_MORCHOK:
                        return _morchokGUID;
                    case NPC_LORD_AFRASASTRASZ:
                        return _lordAfrasastraszGUID;
                    case NPC_DEATHWING:
                        return _deathwingGUID;
                    case NPC_DRAGON_SOUL_WT:
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
                    case NPC_SKY_CAPTAIN_SWAYZE:
                        return _skyCaptainSwayzeGUID;
                    case NPC_KAANU_REEVS:
                        return _kaanuReevsGUID;
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
                    case NPC_MADNESS_OF_DEATHWING_HEAD_10_N:
                    case NPC_MADNESS_OF_DEATHWING_HEAD_25_N:
                    case NPC_MADNESS_OF_DEATHWING_HEAD_10_H:
                    case NPC_MADNESS_OF_DEATHWING_HEAD_25_H:
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
                        return _disabledDSBuff;
                    case DATA_DRAGON_SOUL_EVENT_PROGRESS:
                        return (uint32)_dragonSoulEventProgress;
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
						_disabledDSBuff = data;
						//if (_disabledDSBuff == DONE)
						//{
						//	boost::shared_lock<boost::shared_mutex> lock(*HashMapHolder<Creature>::GetLock());
						//	HashMapHolder<Creature>::MapType const& m = ObjectAccessor::GetCreatures();
						//	for (auto creature : m)
						//		creature.second->RemoveAurasDueToSpell(SPELL_POWER_OF_THE_ASPECTS_35);
						//}
						break;
                    case DATA_DRAGON_SOUL_EVENT_PROGRESS:
                        SetDragonSoulProgress((DragonSoulEventProgress)data);
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
                for (auto guid : _dragonSoulTeleporters)
                if (Creature* teleporter = instance->GetCreature(guid))
                {
                    if (teleporter->HasAura(SPELL_TELEPORTER_ACTIVE))
                    //switch (teleporter->GetEntry())
                    //{
                    //    case NPC_TRAVEL_TO_EYE_OF_ETERNITY:
                    //        if (_dragonSoulEventProgress != EVENT_PROGRESS_HAGARA)
                    //            return;
                    //        break;
                    //    case NPC_TRAVEL_TO_SKYFIRE:
                    //        if (_dragonSoulEventProgress < EVENT_PROGRESS_SIEGE_ENDED)
                    //            return;
                    //        break;
                    //    /*case NPC_TRAVEL_TO_THE_MAELSTROM:
                    //        break;*/
                    //    case NPC_TRAVEL_TO_WYRMREST_BASE:
                    //    case NPC_TRAVEL_TO_WYRMREST_TEMPLE:
                    //        if (_dragonSoulEventProgress < EVENT_PROGRESS_MORCHOK_END)
                    //            return;
                    //        break;
                    //    case NPC_TRAVEL_TO_WYRMREST_SUMMIT:
                    //        if (_dragonSoulEventProgress < EVENT_PROGRESS_HAGARA_INTRO)
                    //            return;
                    //        break;
                    //    default:
                    //        break;
                    //}

      //              if (enable)
      //              {
      //                  teleporter->AddAura(SPELL_TELEPORTER_ACTIVE, teleporter);
      //                  teleporter->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
						//if (Creature* afrastrasz = GetCreature(_lordAfrasastraszGUID))
						//	afrastrasz->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
      //              }
      //              else
      //              {
      //                  teleporter->RemoveAura(SPELL_TELEPORTER_ACTIVE);
      //                  teleporter->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
						//if (Creature* afrastrasz = GetCreature(_lordAfrasastraszGUID)) //Do not allow players to switch the dragon soul buff off during an encounter
						//	afrastrasz->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
      //              }
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
                        if (state == DONE)
                            SetDragonSoulProgress(EVENT_PROGRESS_GENERALS);
                        break;
                    case DATA_WARLORD_ZONOZZ:
                        if (state == DONE && GetBossState(DATA_YORSAHJ_THE_UNSLEEPING))
                            SetDragonSoulProgress(EVENT_PROGRESS_HAGARA);
                    case DATA_YORSAHJ_THE_UNSLEEPING:
                        if (state == DONE && GetBossState(DATA_WARLORD_ZONOZZ))
                            SetDragonSoulProgress(EVENT_PROGRESS_HAGARA);
                        break;
                    case DATA_HAGARA_THE_STORMBINDER:
                        if (state == DONE)
                            SetDragonSoulProgress(EVENT_PROGRESS_ULTRAXION);
                    case DATA_ULTRAXION:
                        if (state == DONE)
                            SetDragonSoulProgress(EVENT_PROGRESS_GUNSHIP);
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

                        if (state == DONE)
                            SetDragonSoulProgress(EVENT_PROGRESS_SPINE);
                    case DATA_SPINE_OF_DEATHWING:
                        if (state == DONE)
                        {
                            SetDragonSoulProgress(EVENT_PROGRESS_MADNESS);
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

            void UpdateInstanceProgress(uint32 eventId)
            {
                boost::shared_lock<boost::shared_mutex> lock(*HashMapHolder<Creature>::GetLock());
                HashMapHolder<Creature>::MapType const& m = ObjectAccessor::GetCreatures();
                for (auto creature : m)
                    if (creature.second->IsAIEnabled)
                        creature.second->AI()->SetData(DATA_INSTANCE_PROGRESS, eventId);
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
				data << _disabledDSBuff;
			}

			void ReadSaveDataMore(std::istringstream& data) override
			{
				data >> _disabledDSBuff;

				/*if (GetBossState(DATA_MORCHOK) == NOT_STARTED)
					SetDragonSoulProgress(EVENT_PROGRESS_MORCHOK_INTRO);
				else if (GetBossState(DATA_WARLORD_ZONOZZ) == NOT_STARTED && GetBossState(DATA_YORSAHJ_THE_UNSLEEPING) == NOT_STARTED)
					SetDragonSoulProgress(EVENT_PROGRESS_ASSAULT_GENERALS);
				else if (GetBossState(DATA_WARLORD_ZONOZZ) != GetBossState(DATA_YORSAHJ_THE_UNSLEEPING))
					SetDragonSoulProgress(EVENT_PROGRESS_FIRST_GENERAL_DEAD);
				else if (GetBossState(DATA_HAGARA_THE_STORMBINDER) == NOT_STARTED)
					SetDragonSoulProgress(EVENT_PROGRESS_HAGARA);
				else if (GetBossState(DATA_ULTRAXION) == NOT_STARTED)
					SetDragonSoulProgress(EVENT_PROGRESS_ULTRAXION_INTRO);
				else
					SetDragonSoulProgress(EVENT_PROGRESS_SIEGE_ENDED);*/

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
            uint32 _disabledDSBuff;
            bool _spineLooted;

            uint32 _eventsUnfoldHagara;

            //Creatures
            ObjectGuid _yseraHagaraIntroGUID;
            ObjectGuid _alexstraszaHagaraIntroGUID;
            ObjectGuid _nozdormuHagaraIntroGUID;
            ObjectGuid _thrallHagaraIntroGUID;
            ObjectGuid _kalecgosHagaraIntroGUID;
            ObjectGuid _travelToEyeOfEternityGUID;

            ObjectGuid _morchokGUID;
            ObjectGuid _yorsahjGUID;
            ObjectGuid _zonozzGUID;
            ObjectGuid _lordAfrasastraszGUID;
            ObjectGuid _deathwingGUID;
            ObjectGuid _dragonSoulGUID;
            ObjectGuid _alexstraszaWtGUID;
            ObjectGuid _yseraWtGUID;
            ObjectGuid _kalecgosWtGUID;
            ObjectGuid _thrallWtGUID;
            ObjectGuid _nozdormuWtGUID;
            ObjectGuid _ultraxionGauntletGUID;
            ObjectGuid _skyCaptainSwayzeGUID;
            ObjectGuid _kaanuReevsGUID;
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
