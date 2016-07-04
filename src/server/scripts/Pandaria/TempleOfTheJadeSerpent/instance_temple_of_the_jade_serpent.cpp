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

#include "ScriptMgr.h"
#include "Player.h"
#include "InstanceScript.h"
#include "CreatureGroups.h"
#include "temple_of_the_jade_serpent.h"
#include "TaskScheduler.h"

DoorData const doorData[] =
{
    { GO_WATERY_DOOR,                   BOSS_WISE_MARI,             DOOR_TYPE_ROOM      },
    { GO_FOUNTAIN_OF_EVERSEEING_EXIT,   BOSS_WISE_MARI,             DOOR_TYPE_PASSAGE   },
    { GO_FOUNTAIN_OF_EVERSEEING_EXIT,   BOSS_LOREWALKER_STONESTEP,  DOOR_TYPE_PASSAGE   },
    { GO_SCROLLKEEPERS_SANCTUM_EXIT,    BOSS_WISE_MARI,             DOOR_TYPE_PASSAGE   },
    { GO_SCROLLKEEPERS_SANCTUM_EXIT,    BOSS_LOREWALKER_STONESTEP,  DOOR_TYPE_PASSAGE   },
    { GO_DOOR_TO_COURTYARD,             BOSS_WISE_MARI,             DOOR_TYPE_PASSAGE   },
    { GO_DOOR_TO_COURTYARD,             BOSS_LOREWALKER_STONESTEP,  DOOR_TYPE_PASSAGE   },
    { GO_DOOR_TO_SHA_OF_DOUBT,          BOSS_LIU_FLAMEHEART,        DOOR_TYPE_PASSAGE   },
    { GO_DOOR_TO_SHA_OF_DOUBT,          BOSS_SHA_OF_DOUBT,          DOOR_TYPE_ROOM      },
    { 0,                                0,                          DOOR_TYPE_ROOM      },// END
};

class instance_temple_of_the_jade_serpent : public InstanceMapScript
{
    public:
        instance_temple_of_the_jade_serpent() : InstanceMapScript("instance_temple_of_the_jade_serpent", 960) { }

        struct instance_temple_of_the_jade_serpent_InstanceMapScript : public InstanceScript
        {
            instance_temple_of_the_jade_serpent_InstanceMapScript(Map* map) : InstanceScript(map)
            {
                SetHeaders(DataHeader);
                SetBossNumber(MAX_BOSS_DATA);
                LoadDoorData(doorData);
            }

            void OnCreatureCreate(Creature* creature) override
            {
                if (creature->IsAlive())
                {
                    creature->SearchFormation();
                    if (CreatureGroup* group = creature->GetFormation())
                    {
                        switch (group->GetId())
                        {
                            case CREATURE_FORMATION_SERPENT_WARRIORS_1:
                                AlliedTerraceFighterGUIDs[TERRACE_FIGHTERS_1].push_back(creature->GetGUID());
                                break;
                            case CREATURE_FORMATION_SERPENT_WARRIORS_2:
                                AlliedTerraceFighterGUIDs[TERRACE_FIGHTERS_2].push_back(creature->GetGUID());
                                break;
                            case CREATURE_FORMATION_SERPENT_WARRIORS_3:
                                AlliedTerraceFighterGUIDs[TERRACE_FIGHTERS_3].push_back(creature->GetGUID());
                                break;
                            case CREATURE_FORMATION_SHA_MINIONS_1:
                                EnemyTerraceFighterGUIDs[TERRACE_FIGHTERS_1].push_back(creature->GetGUID());
                                TerraceShaMinionsGUIDs.push_back(creature->GetGUID());
                                break;
                            case CREATURE_FORMATION_SHA_MINIONS_2:
                                EnemyTerraceFighterGUIDs[TERRACE_FIGHTERS_2].push_back(creature->GetGUID());
                                TerraceShaMinionsGUIDs.push_back(creature->GetGUID());
                                break;
                            case CREATURE_FORMATION_SHA_MINIONS_3:
                                EnemyTerraceFighterGUIDs[TERRACE_FIGHTERS_3].push_back(creature->GetGUID());
                                TerraceShaMinionsGUIDs.push_back(creature->GetGUID());
                                break;
                            case CREATURE_FORMATION_PATROLING_SHA_MINIONS:
                                TerraceShaMinionsGUIDs.push_back(creature->GetGUID());
                                break;
                            default:
                                break;
                        }
                    }
                }

                switch (creature->GetEntry())
                {
                    case NPC_WISE_MARI:
                        WiseMariGUID = creature->GetGUID();
                        break;
                    case NPC_CORRUPTED_WATERS_STALKER:
                        CorruptedWatersStalkerGUID = creature->GetGUID();
                        break;
                    case NPC_SPLASH_STALKER:
                        if (creature->GetSpawnId() != GUID_SPLASH_STALKER_ROOT)
                            break;

                        SplashStalkerRootGUID = creature->GetGUID();
                        break;
                    case NPC_FIRE_HOSE:
                        FireHoseGUID = creature->GetGUID();
                        break;
                    case NPC_LOREWALKER_STONESTEP:
                        LorewalkerStonestepGUID = creature->GetGUID();
                        break;
                    case NPC_LIU_FLAMEHEART:
                        LiuFlameheartGUID = creature->GetGUID();
                        break;
                    case NPC_YULON:
                        YulonGUID = creature->GetGUID();
                        break;
                    default:
                        break;
                }

                InstanceScript::OnCreatureCreate(creature);
            }

            void OnUnitDeath(Unit* unit) override
            {
                Creature* creature = unit->ToCreature();
                if (!creature)
                    return;

                creature->SearchFormation();
                if (CreatureGroup* group = creature->GetFormation())
                {
                    switch (group->GetId())
                    {
                        case CREATURE_FORMATION_SHA_MINIONS_1:
                            EnemyTerraceFighterGUIDs[TERRACE_FIGHTERS_1].remove(creature->GetGUID());
                            TerraceShaMinionsGUIDs.remove(creature->GetGUID());

                            creature->Say("Removed from list 1", LANG_UNIVERSAL);

                            if (!EnemyTerraceFighterGUIDs[TERRACE_FIGHTERS_1].empty() || AlliedTerraceFighterGUIDs[TERRACE_FIGHTERS_1].empty())
                                break;

                            TerraceFightersRetreat(AlliedTerraceFighterGUIDs[TERRACE_FIGHTERS_1]);
                            break;
                        case CREATURE_FORMATION_SHA_MINIONS_2:
                            EnemyTerraceFighterGUIDs[TERRACE_FIGHTERS_2].remove(creature->GetGUID());
                            TerraceShaMinionsGUIDs.remove(creature->GetGUID());

                            creature->Say("Removed from list 2", LANG_UNIVERSAL);

                            if (!EnemyTerraceFighterGUIDs[TERRACE_FIGHTERS_2].empty() || AlliedTerraceFighterGUIDs[TERRACE_FIGHTERS_2].empty())
                                break;

                            TerraceFightersRetreat(AlliedTerraceFighterGUIDs[TERRACE_FIGHTERS_2]);
                            break;
                        case CREATURE_FORMATION_SHA_MINIONS_3:
                            EnemyTerraceFighterGUIDs[TERRACE_FIGHTERS_3].remove(creature->GetGUID());
                            TerraceShaMinionsGUIDs.remove(creature->GetGUID());

                            creature->Say("Removed from list 3", LANG_UNIVERSAL);

                            if (!EnemyTerraceFighterGUIDs[TERRACE_FIGHTERS_3].empty() || AlliedTerraceFighterGUIDs[TERRACE_FIGHTERS_3].empty())
                                break;

                            TerraceFightersRetreat(AlliedTerraceFighterGUIDs[TERRACE_FIGHTERS_3]);
                            break;
                        case CREATURE_FORMATION_PATROLING_SHA_MINIONS:
                            TerraceShaMinionsGUIDs.remove(creature->GetGUID());
                            creature->Say("Removed from list", LANG_UNIVERSAL);
                            break;
                        default:
                            break;
                    }

                    switch (group->GetId())
                    {
                        case CREATURE_FORMATION_SHA_MINIONS_1:
                        case CREATURE_FORMATION_SHA_MINIONS_2:
                        case CREATURE_FORMATION_SHA_MINIONS_3:
                        case CREATURE_FORMATION_PATROLING_SHA_MINIONS:
                            if (TerraceShaMinionsGUIDs.empty())
                            {
                                std::cout << "\nList is empty";
                                if (Creature* liu = instance->GetCreature(LiuFlameheartGUID))
                                    if (liu->IsAIEnabled)
                                        liu->AI()->DoAction(ACTION_TERRACE_LIU_FLAMEHEART_ENTRANCE);
                            }
                            break;
                        default:
                            break;
                    }
                }

                InstanceScript::OnUnitDeath(unit);
            }


            void OnGameObjectCreate(GameObject* go) override
            {
                switch (go->GetEntry())
                {
                    case GO_WATERY_DOOR:
                    case GO_FOUNTAIN_OF_EVERSEEING_EXIT:
                    case GO_SCROLLKEEPERS_SANCTUM_EXIT:
                    case GO_DOOR_TO_COURTYARD:
                    case GO_DOOR_TO_SHA_OF_DOUBT:
                        AddDoor(go, true);
                        break;
                    default:
                        break;
                }
            }

            void OnGameObjectRemove(GameObject* go) override
            {
                switch (go->GetEntry())
                {
                    case GO_WATERY_DOOR:
                    case GO_FOUNTAIN_OF_EVERSEEING_EXIT:
                    case GO_SCROLLKEEPERS_SANCTUM_EXIT:
                    case GO_DOOR_TO_COURTYARD:
                    case GO_DOOR_TO_SHA_OF_DOUBT:
                        AddDoor(go, false);
                        break;
                    default:
                        break;
                }
            }

            bool SetBossState(uint32 type, EncounterState state) override
            {
                if (!InstanceScript::SetBossState(type, state))
                    return false;
            }

            ObjectGuid GetGuidData(uint32 type) const override
            {
                switch (type)
                {
                    case DATA_WISE_MARI:
                        return WiseMariGUID;
                    case DATA_CORRUPTED_WATERS_STALKER:
                        return CorruptedWatersStalkerGUID;
                    case DATA_SPLASH_STALKER_ROOT:
                        return SplashStalkerRootGUID;
                    case DATA_FIRE_HOSE:
                        return FireHoseGUID;
                    case DATA_LOREWALKER_STONESTEP:
                        return LorewalkerStonestepGUID;
                    case DATA_YULON:
                        return YulonGUID;
                    default:
                        break;
                }

                return ObjectGuid::Empty;
            }

            private:
                void TerraceFightersRetreat(GuidList guids)
                {
                    if (Creature* fighter = instance->GetCreature(guids.front()))
                    {
                        fighter->Say("I'm the leader, I say we fuck off.", LANG_UNIVERSAL);
                        if (fighter->IsAIEnabled)
                            fighter->AI()->Talk(0);
                    }

                    for (auto guid : guids)
                        if (Creature* fighter = instance->GetCreature(guid))
                        {
                            if (fighter->IsAIEnabled)
                                fighter->AI()->DoAction(ACTION_TERRACE_FIGHTERS_RETREAT);
                        }
                }

        protected:
            TaskScheduler scheduler;

            ObjectGuid WiseMariGUID;
            ObjectGuid CorruptedWatersStalkerGUID;
            ObjectGuid SplashStalkerRootGUID;
            ObjectGuid FireHoseGUID;
            ObjectGuid LorewalkerStonestepGUID;
            ObjectGuid LiuFlameheartGUID;
            GuidList AlliedTerraceFighterGUIDs[TERRACE_FIGHTERS_MAX];
            GuidList EnemyTerraceFighterGUIDs[TERRACE_FIGHTERS_MAX];
            GuidList TerraceShaMinionsGUIDs;
            ObjectGuid YulonGUID;
            
        };

        InstanceScript* GetInstanceScript(InstanceMap* map) const
        {
            return new instance_temple_of_the_jade_serpent_InstanceMapScript(map);
        }
};

void AddSC_instance_temple_of_the_jade_serpent()
{
    new instance_temple_of_the_jade_serpent();
}