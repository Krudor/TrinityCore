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

enum Events
{
    EVENT_NONE,
    EVENT_ENABLE_TELEPORTERS,
    EVENT_DISABLE_TELEPORTERS
};

ObjectData const creatureData[] =
{
    { NPC_WARMASTER_BLACKHORN,          DATA_WARMASTER_BLACKHORN},
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

                _deckDefended = true;
            }

            uint32 GetData(uint32 type) const override
            {
                switch (type)
                {
                    case DATA_DECK_DEFENDER:
                        return uint32(_deckDefended);
                    default:
                        break;
                }

                return 0;
            }

            void SetData(uint32 type, uint32 data) override
            {
                switch (type)
                {
                    case DATA_DECK_DEFENDER:
                        _deckDefended = data != 0;
                        break;
                    default:
                        break;
                }
            }

            bool SetBossState(uint32 type, EncounterState state) override
            {
                if (!InstanceScript::SetBossState(type, state))
                    return false;

                switch (type)
                {
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
                        break;
                    default:
                        break;
                }

                return true;
            }

        protected:
            bool _deckDefended;
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
