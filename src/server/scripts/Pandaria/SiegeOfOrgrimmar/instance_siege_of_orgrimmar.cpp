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
#include "siege_of_orgrimmar.h"

DoorData const doorData[] =
{
    { 0, 0, DOOR_TYPE_ROOM },// END
};

class instance_siege_of_orgrimmar : public InstanceMapScript
{
    public:
        instance_siege_of_orgrimmar() : InstanceMapScript("instance_siege_of_orgrimmar", 1136) { }

        struct instance_siege_of_orgrimmar_InstanceMapScript : public InstanceScript
        {
            instance_siege_of_orgrimmar_InstanceMapScript(Map* map) : InstanceScript(map) {}

            void Initialize()
            {
				SetHeaders(DataHeader);
                SetBossNumber(MAX_BOSS_DATA);
                LoadDoorData(doorData);

				InstanceProgress = INSTANCE_PROGRESS_NONE;
				GarroshHellscreamIntroState = NOT_STARTED;
            }

			void OnPlayerEnter(Player* player) override
			{
				SetEntranceLocation(WSL_PRE_GOLDEN_LOTUS);
				SetTemporaryEntranceLocation(WSL_PRE_GARROSH);
			}

			void OnCreatureCreate(Creature* creature) override
			{
				InstanceScript::OnCreatureCreate(creature);

				switch (creature->GetEntry())
				{
					case NPC_GARROSH_THRALL:
						GarroshThrallGUID = creature->GetGUID();
						break;
					case NPC_GARROSH_HELLSCREAM:
						GarroshHellscreamGUID = creature->GetGUID();
						break;
					case NPC_HEART_OF_YSHAARJ_MAIN_ROOM:
						HeartOfYshaarjMainRoomCreatureGUID = creature->GetGUID();
						break;
					default:
						break;
				}
			}

			void OnGameObjectCreate(GameObject* go) override
			{
				switch (go->GetEntry())
				{
					case GO_HEART_OF_YSHAARJ:
						HeartOfYshaarjMainRoomGameobjectGUID = go->GetGUID();
						break;
				}
			}

            bool SetBossState(uint32 id, EncounterState state)
            {
                if (!InstanceScript::SetBossState(id, state))
                    return false;

				switch (id)
				{
					case BOSS_IMMERSEUS:
						if (state == DONE)
						{
							SetEntranceLocation(WSL_PRE_GOLDEN_LOTUS);
							SaveToDB();
						}
						break;
					case BOSS_FALLEN_PROTECTORS:
						break;
					case BOSS_NORUSHEN:
						if (state == DONE)
						{
							SetEntranceLocation(WSL_PRE_SHA);
							SaveToDB();
						}
						break;
					case BOSS_SHA_OF_PRIDE:
						break;
					case BOSS_GALAKRAS:
						break;
					case BOSS_IRON_JUGGERNAUT:
						if (state == DONE)
						{
							SetEntranceLocation(WSL_PRE_DARK_SHAMAN);
							SaveToDB();
						}
						break;
					case BOSS_KORKRON_DARK_SHAMAN:
						break;
					case BOSS_GENERAL_NAZGRIM:
						if (state == DONE)
						{
							SetEntranceLocation(WSL_PRE_MALKOROK);
							SaveToDB();
						}
						break;
					case BOSS_MALKOROK:
						break;
					case BOSS_SPOILS_OF_PANDARIA:
						break;
					case BOSS_THOK_THE_BLOODTHIRSTY:
						break;
					case BOSS_SIEGECRAFTER_BLACKFUSE:
						break;
					case BOSS_PARAGONS_OF_THE_KLAXXI:
						if (state == DONE)
						{
							SetEntranceLocation(WSL_PRE_GARROSH);
							SaveToDB();
						}
						break;
					case BOSS_GARROSH_HELLSCREAM:
						break;
				}

                return true;
            }

			ObjectGuid GetGuidData(uint32 data) const override
			{
				switch (data)
				{
					case DATA_GARROSH_THRALL:
						return GarroshThrallGUID;
					case DATA_GARROSH_HELLSCREAM:
						return GarroshHellscreamGUID;
					case DATA_HEART_OF_YSHAARJ_CREATURE_MAIN_ROOM:
						return HeartOfYshaarjMainRoomCreatureGUID;
					case DATA_HEART_OF_YSHAARJ_GAMEOBJECT_MAIN_ROOM:
						return HeartOfYshaarjMainRoomGameobjectGUID;
				}

				return ObjectGuid::Empty;
			}

			void SetData(uint32 type, uint32 data) override
			{
				switch (type)
				{
					case DATA_INSTANCE_PROGRESS:
						if (InstanceProgress < data)
							break;

						InstanceProgress = data;
						switch (type)
						{
							case INSTANCE_PROGRESS_REACHED_NORUSHEN:
								SetTemporaryEntranceLocation(WSL_PRE_SHA);
								break;
							case INSTANCE_PROGRESS_ORGRIMMAR_GATES:
								SetTemporaryEntranceLocation(WSL_PRE_DARK_SHAMAN);
								break;
							case INSTANCE_PROGRESS_REACHED_GAMON:
								SetTemporaryEntranceLocation(WSL_THE_DRAG);
								break;
							case INSTANCE_PROGRESS_REACHED_UNDERHOLD:
								SetTemporaryEntranceLocation(WSL_THE_UNDERHOLD);
								break;
							default:
								break;
						}
						break;
					case DATA_GARROSH_HELLSCREAM_INTRO:
						if (GarroshHellscreamIntroState == data)
							break;

						GarroshHellscreamIntroState = data;
						switch (GarroshHellscreamIntroState)
						{
							case IN_PROGRESS:
								if (Creature* heartOfYshaarj = instance->GetCreature(HeartOfYshaarjMainRoomCreatureGUID))
									heartOfYshaarj->CastSpell(heartOfYshaarj, SPELL_HEARTBEAT_SOUND, true);

								if (GameObject* heartOfYshaarj = instance->GetGameObject(HeartOfYshaarjMainRoomGameobjectGUID))
									heartOfYshaarj->SetGoState(GO_STATE_READY);
								break;
							case DONE:
								if (Creature* garrosh = instance->GetCreature(GarroshHellscreamGUID))
									if (garrosh->IsAIEnabled)
										garrosh->GetAI()->DoAction(ACTION_GARROSH_INTRO);
								break;
							default:
								break;
						}
						break;
					default:
						break;
				}
			}

			uint32 GetData(uint32 type) const override
			{
				switch (type)
				{
					case DATA_GARROSH_HELLSCREAM_INTRO:
						return GarroshHellscreamIntroState;
					default:
						return 0;
				}
			}

			void Load(const char* in) override
			{
				InstanceScript::Load(in);

				if (GetBossState(BOSS_SPOILS_OF_PANDARIA == DONE))
					SetData(DATA_INSTANCE_PROGRESS, INSTANCE_PROGRESS_REACHED_UNDERHOLD);
				else if (GetBossState(BOSS_NORUSHEN) == DONE)
					SetData(DATA_INSTANCE_PROGRESS, INSTANCE_PROGRESS_REACHED_NORUSHEN);
			}

            /*bool CheckRequiredBosses(uint32 bossId, Player const* player = NULL) const
            {
                if (!InstanceScript::CheckRequiredBosses(bossId, player))
                    return false;

                switch (bossId)
                {
                    case DATA_GARROSH_HELLSCREAM:
                    case DATA_PARAGONS_OF_THE_KLAXXI:
                    case DATA_SIEGECRAFTER_BLACKFUSE:
                    case DATA_THOK_THE_BLOODTHIRSTY:
                    case DATA_SPOILS_OF_PANDARIA:
                    case DATA_MALKOROK:
                    case DATA_GENERAL_NAZGRIM:
                    case DATA_KORKRON_DARK_SHAMAN:
                    case DATA_IRON_JUGGERNAUT:
                    case DATA_GALAKRAS:
                    case DATA_SHA_OF_PRIDE:
                    case DATA_NORUSHEN:
                    case DATA_FALLEN_PROTECTORS:
                        if (GetBossState(bossId - 1) != DONE)
                            return false;
                    default:
                        break;
                }

                return true;
            }*/

		protected:
			ObjectGuid GarroshHellscreamGUID;
			ObjectGuid GarroshThrallGUID;
			ObjectGuid HeartOfYshaarjMainRoomCreatureGUID;
			ObjectGuid HeartOfYshaarjMainRoomGameobjectGUID;
			
			uint32 InstanceProgress;
			uint32 GarroshHellscreamIntroState;
			EventMap events;
        };

        InstanceScript* GetInstanceScript(InstanceMap* map) const
        {
            return new instance_siege_of_orgrimmar_InstanceMapScript(map);
        }
};

void AddSC_instance_siege_of_orgrimmar()
{
    new instance_siege_of_orgrimmar();
}