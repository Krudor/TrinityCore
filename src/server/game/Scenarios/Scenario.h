/*
 * Copyright (C) 2008-2015 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
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

#ifndef __TRINITY_SCENARIO_H
#define __TRINITY_SCENARIO_H

#include "ChallengeMode.h"
#include "CriteriaHandler.h"
#include "ScenarioPackets.h"
#include "ScenarioMgr.h"
#include "Player.h"

struct InstanceScenario
{
    StringVector OptionText;
    StringVector BoxText;
};

struct ScenarioData
{
    ScenarioEntry const* Entry;
    std::map<uint8, ScenarioStepEntry const*> Steps;
};

class Scenario : public CriteriaHandler
{
    public:
        Scenario(ScenarioData const* scenarioData);
        ~Scenario() { }

        virtual void Reset() override;

        virtual void CompleteStep(uint8 step);
        virtual void CompleteScenario();
        
        virtual void OnPlayerEnter(Player* player) { m_players.insert(player->GetGUID()); }
        virtual void OnPlayerExit(Player* player) { m_players.erase(player->GetGUID()); }
        virtual void Update(uint32) { }

        void SendScenarioState();
        void SendScenarioState(Player* player);
        bool IsComplete() const { return _complete; }

    protected:
        GuidSet m_players;

        void SendCriteriaUpdate(Criteria const* criteria, CriteriaProgress const* progress, uint32 timeElapsed, bool timedCompleted) const override;
        void SendCriteriaProgressRemoved(uint32 criteriaId) override;

        bool CanUpdateCriteriaTree(Criteria const* criteria, CriteriaTree const* tree, Player* referencePlayer) const override;
        bool CanCompleteCriteriaTree(CriteriaTree const* tree) override;
        void CompletedCriteriaTree(CriteriaTree const* tree, Player* referencePlayer) override;
        void AfterCriteriaTreeUpdate(CriteriaTree const* /*tree*/, Player* /*referencePlayer*/) override { }

        void SendPacket(WorldPacket const* data) const override;

        void BuildScenarioState(WorldPackets::Scenario::ScenarioState* scenarioState);

        std::vector<WorldPackets::Scenario::BonusObjectiveData> GetBonusObjectivesData();
        std::vector<WorldPackets::Scenario::CriteriaProgress> GetCriteriasProgress();

        std::string GetOwnerInfo() const override;
        CriteriaList const& GetCriteriaByType(CriteriaTypes type) const override;
        ScenarioData const* _data;

    private:
        void SendToPlayers(WorldPacket const* data) const;
        void SetStep(int8 step);
        void Boot(Player* player);
        int8 _step;
        int8 _lastStep;
        bool _complete;

        void SendAllData(Player const* receiver) const override;
};

#endif
