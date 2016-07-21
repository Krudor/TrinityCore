/*
 * Copyright (C) 2008-2015 TrinityCore <http://www.trinitycore.org/>
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

#include "Scenario.h"
#include "DB2Store.h"
#include "Player.h"
#include "ScenarioMgr.h"
#include "InstanceSaveMgr.h"
#include "ChallengeModeMgr.h"
#include "ObjectMgr.h"

Scenario::Scenario(Map* map, ScenarioData const* scenarioData) : _map(map), _data(scenarioData), _complete(false), _lastStep(0)
{
    ASSERT(_map);
    ASSERT(_data);

    LoadInstanceData(_map->GetInstanceId());
    challengeMode = sChallengeModeMgr->CreateChallengeMode(_map);

    SendScenarioState();

    ASSERT(!_data->Steps.empty());
    ScenarioStepEntry const* step = _data->Steps.at(0);
    if (step)
        SetStep(step->Step);

    for (auto itr = _data->Steps.begin(); itr != _data->Steps.end(); ++itr)
        if (_lastStep < itr->second->Step)
            _lastStep = itr->second->Step;
}

void Scenario::SaveToDB()
{
    //if (_data->Entry->Flags & SCENARIO_ENTRY_FLAG_CHALLENGE_MODE)
    //    return;

    if (_criteriaProgress.empty())
        return;

    uint32 id = _map->GetInstanceId();
    if (!id)
    {
        TC_LOG_DEBUG("scenario", "Scenario::SaveToDB: Can not save scenario progress without an instance save. Map::GetInstanceId() did not return an instance save.");
        return;
    }

    for (auto iter = _criteriaProgress.begin(); iter != _criteriaProgress.end(); ++iter)
    {
        if (!iter->second.Changed)
            continue;

        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_SCENARIO_INSTANCE_CRITERIA_BY_CRITERIA);
        SQLTransaction trans = CharacterDatabase.BeginTransaction();

        stmt->setUInt32(0, iter->first);
        trans->Append(stmt);

        if (iter->second.Counter)
        {
            stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_SCENARIO_INSTANCE_CRITERIA);
            stmt->setUInt32(0, id);
            stmt->setUInt32(1, iter->first);
            stmt->setUInt64(2, iter->second.Counter);
            stmt->setUInt32(3, uint32(iter->second.Date));
            trans->Append(stmt);
        }

        iter->second.Changed = false;
    }
}

void Scenario::LoadInstanceData(uint32 instanceId)
{
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_SCENARIO_INSTANCE_CRITERIA_FOR_INSTANCE);
    stmt->setUInt32(0, instanceId);

    PreparedQueryResult result = CharacterDatabase.Query(stmt);
    if (result)
    {
        time_t now = time(NULL);
        do
        {
            Field* fields = result->Fetch();
            uint32 id = fields[0].GetUInt32();
            uint64 counter = fields[1].GetUInt64();
            time_t date = time_t(fields[2].GetUInt32());

            Criteria const* criteria = sCriteriaMgr->GetCriteria(id);
            if (!criteria)
            {
                // Removing non-existing criteria data for all instances
                TC_LOG_ERROR("criteria.achievement", "Non-existing achievement criteria %u data has been removed from the table `instance_scenario_progress`.", id);

                PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_INVALID_SCENARIO_INSTANCE_CRITERIA);
                stmt->setUInt32(0, uint32(id));
                CharacterDatabase.Execute(stmt);

                continue;
            }

            if (criteria->Entry->StartTimer && time_t(date + criteria->Entry->StartTimer) < now)
                continue;

            CriteriaProgress& criteriaProgress = _criteriaProgress[id];
            criteriaProgress.Counter = counter;
            criteriaProgress.Date = date;
        }
        while (result->NextRow());

        SendScenarioState();
    }
}

void Scenario::Update(uint32 diff)
{
    if (challengeMode)
        challengeMode->Update(diff);
}

void Scenario::Reset()
{
    CriteriaHandler::Reset();
    _complete = false;
    SetStep(0);
}

void Scenario::OnSpellCriteria(Criteria const * criteria, Unit const * caster)
{
}

void Scenario::CompleteStep(uint8 step)
{
    std::map<uint8, ScenarioStepEntry const*>::const_iterator itr = _data->Steps.find(step);
    if (itr == _data->Steps.end())
    {
        TC_LOG_ERROR("scenarios", "Attempted to complete step (Id: %u), but step was not found!", step);
        return;
    }

    CompleteStep(itr->second);
}

void Scenario::CompleteStep(ScenarioStepEntry const* step)
{
    std::list<Player*> players;
    Map::PlayerList const& playerList = _map->GetPlayers();
    for (auto itr = playerList.begin(); itr != playerList.end(); ++itr)
    {
        if (Player* player = itr->GetSource())
            if (Quest const* quest = quest = sObjectMgr->GetQuestTemplate(reward->firstQuest))

    }
}

void Scenario::OnPlayerEnter(Player* player) const
{
    //WorldPackets::Scenario::ScenarioState scenarioState;
    //BuildScenarioState(&scenarioState);
    //player->SendDirectMessage(scenarioState.Write());
    //
    if (challengeMode)
        challengeMode->OnPlayerEnter(player);
}

void Scenario::OnPlayerExit(Player* player) const
{
	if (challengeMode)
		challengeMode->OnPlayerExit(player);
}

void Scenario::SetStep(int8 step)
{
    _step = step;
    SendScenarioState();

    if (_step > _lastStep)
        if (!IsComplete())
            CompleteScenario();
}

void Scenario::CompleteScenario()
{
    if (IsComplete())
        return;

    _complete = true;
    SendPacket(WorldPackets::Scenario::ScenarioCompleted(_data->Entry->ID).Write());
    if (challengeMode)
        challengeMode->Complete();
}

void Scenario::SendAllData(Player const * receiver) const
{
}

void Scenario::SendCriteriaUpdate(Criteria const * criteria, CriteriaProgress const * progress, uint32 timeElapsed, bool timedCompleted) const
{
    WorldPackets::Scenario::ScenarioProgressUpdate progressUpdate;
    WorldPackets::Scenario::CriteriaProgress criteriaProgress;
    criteriaProgress.Id = criteria->ID;
    criteriaProgress.Quantity = progress->Counter;
    criteriaProgress.Player = progress->PlayerGUID;
    criteriaProgress.Date = progress->Date;
    if (criteria->Entry->StartTimer)
        criteriaProgress.Flags = timedCompleted ? 1 : 0;

    criteriaProgress.TimeStart = timeElapsed;
    criteriaProgress.TimeCreate = 0;

    progressUpdate.criteriaProgress = criteriaProgress;
    _map->SendToPlayers(progressUpdate.Write());
}

void Scenario::SendCriteriaProgressRemoved(uint32 criteriaId)
{
}

bool Scenario::CanUpdateCriteriaTree(Criteria const * /*criteria*/, CriteriaTree const * tree, Player * /*referencePlayer*/)
{
    return CanCompleteCriteriaTree(tree);
}

bool Scenario::CanCompleteCriteriaTree(CriteriaTree const * tree)
{
    if (!tree->ScenarioStep)
        return false;

    std::map<uint8, ScenarioStepEntry const*>::const_iterator itr = _data->Steps.find(_step);
    if (itr != _data->Steps.end())
    {
        if (tree->ScenarioStep->ScenarioID != itr->second->ScenarioID)
            return false;

        if (itr->second->Step != tree->ScenarioStep->Step && !(tree->ScenarioStep->BonusStepFlag & SCENARIO_STEP_FLAG_BONUS_OBJECTIVE))
            return false;

        return true;
    }

    return false;
}

void Scenario::CompletedCriteriaTree(CriteriaTree const * tree, Player * referencePlayer)
{
    if (!tree->ScenarioStep || _complete)
        return;

    if (tree->ScenarioStep->BonusStepFlag & SCENARIO_STEP_FLAG_BONUS_OBJECTIVE)
        CompleteStep()

    SetStep(_step + 1);
    
    //for (std::map<uint8, const ScenarioStepEntry*>::const_iterator itr = _data->Steps.begin(); itr != _data->Steps.end(); ++itr)
    //{
    //    CriteriaTree const* tree = sCriteriaMgr->GetCriteriaTree(itr->second->CriteriaTreeID);
    //    if (!IsCompletedCriteriaTree(tree))
    //        return;
    //}
}

void Scenario::SendPacket(WorldPacket const * data) const
{
    _map->SendToPlayers(data);
}

void Scenario::BuildScenarioState(WorldPackets::Scenario::ScenarioState* scenarioState)
{
    scenarioState->ScenarioId = _data->Entry->ID;
    scenarioState->CurrentStep = _step;
    if (waveGroup)
    {
        scenarioState->DifficultyId = waveGroup->GetDifficulty();
        scenarioState->WaveCurrent = waveGroup->GetCurrentWave();
        scenarioState->WaveMax = waveGroup->GetWavesCount();
        scenarioState->TimerDuration = waveGroup->GetWaveTime(waveGroup->GetCurrentWave());
    }
    scenarioState->CriteriaProgress = GetCriteriasProgress();
    scenarioState->BonusObjectiveData = GetBonusObjectivesData();
    scenarioState->ScenarioCompleted = _complete;
}

std::vector<WorldPackets::Scenario::BonusObjectiveData> Scenario::GetBonusObjectivesData()
{
    std::vector<WorldPackets::Scenario::BonusObjectiveData> bonusObjectivesData;
    for (auto itr = _data->Steps.begin(); itr != _data->Steps.end(); ++itr)
    {
        if (!(itr->second->BonusStepFlag & SCENARIO_STEP_FLAG_BONUS_OBJECTIVE))
            continue;

        CriteriaTree const* tree = sCriteriaMgr->GetCriteriaTree(itr->second->CriteriaTreeID);
        if (tree)
        {
            WorldPackets::Scenario::BonusObjectiveData bonusObjectiveData;
            bonusObjectiveData.Id = itr->second->ID;
            bonusObjectiveData.ObjectiveCompleted = IsCompletedCriteriaTree(tree);
            bonusObjectivesData.push_back(bonusObjectiveData);
        }
    }

    return bonusObjectivesData;
}

std::vector<WorldPackets::Scenario::CriteriaProgress> Scenario::GetCriteriasProgress()
{
    std::vector<WorldPackets::Scenario::CriteriaProgress> criteriasProgress;

    if (!_criteriaProgress.empty())
    {
        for (auto critItr = _criteriaProgress.begin(); critItr != _criteriaProgress.end(); ++critItr)
        {
            //if (!critItr->second.Changed)
            //    continue;

            WorldPackets::Scenario::CriteriaProgress criteriaProgress;
            criteriaProgress.Id = critItr->first;
            criteriaProgress.Quantity = critItr->second.Counter;
            criteriaProgress.Date = critItr->second.Date;
            criteriaProgress.Player = critItr->second.PlayerGUID;
            /*criteriaProgress.TimeStart = ;
            criteriaProgress.TimeCreate = ;
            criteriaProgress.Flags = ;*/
            criteriasProgress.push_back(criteriaProgress);
        }
    }

    return criteriasProgress;
}

std::string Scenario::GetOwnerInfo() const
{
    return std::string();
}

CriteriaList const & Scenario::GetCriteriaByType(CriteriaTypes type) const
{
    return sCriteriaMgr->GetScenarioCriteriaByType(type);
}

void Scenario::Boot(Player* player)
{
    player->SendDirectMessage(WorldPackets::Scenario::ScenarioBoot().Write());
}

ScenarioWaveGroup::ScenarioWaveGroup(ScenarioDifficulty difficulty, std::vector<uint32> waves, bool repeat) : _difficulty(difficulty), _waves(waves), _repeat(repeat)
{
}

void Scenario::SendScenarioState()
{
    WorldPackets::Scenario::ScenarioState scenarioState;
    BuildScenarioState(&scenarioState);
    _map->SendToPlayers(scenarioState.Write());
}

void Scenario::SendScenarioState(Player* player)
{
    WorldPackets::Scenario::ScenarioState scenarioState;
    BuildScenarioState(&scenarioState);
    player->SendDirectMessage(scenarioState.Write());
}