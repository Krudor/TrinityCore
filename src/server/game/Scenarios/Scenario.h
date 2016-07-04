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

struct InstanceScenario
{
    StringVector OptionText;
    StringVector BoxText;
};

enum ScenarioDifficulty
{
    SCENARIO_DIFFICULTY_NONE,
    SCENARIO_DIFFICULTY_BRONZE,
    SCENARIO_DIFFICULTY_SILVER,
    SCENARIO_DIFFICULTY_GOLD,
    SCENARIO_DIFFICULTY_ENDLESS
};

struct ScenarioData
{
    ScenarioEntry const* Entry;
    std::map<uint8, ScenarioStepEntry const*> Steps;
};

class ScenarioWaveGroup
{
    public:
        ScenarioWaveGroup(ScenarioDifficulty difficulty, std::vector<uint32> waves, bool repeat);

        uint32 GetDifficulty() { return uint32(_difficulty); }
        uint32 GetWavesCount() { return _waves.size(); }
        uint32 GetCurrentWave() { return _currentWave; }
        void SetCurrentWave(uint32 wave) { _currentWave = wave; }

        uint32 GetWaveTime(uint32 wave)
        {
            return _waves.at((wave - 1) % _waves.size());
        }

        bool IsRepeating() { return _repeat; }

    private:
        ScenarioDifficulty _difficulty;
        std::vector<uint32 /*seconds*/> _waves;
        bool _repeat;
        uint32 _currentWave;
};

class Scenario : public CriteriaHandler
{
    public:
        Scenario(Map* map, ScenarioData const* scenarioData);
        ~Scenario();

        void Reset() override;

        void OnSpellCriteria(Criteria const* criteria, Unit const* caster);

        void SendScenarioState(Player* player);
        void OnPlayerEnter(Player* player) const;
		void OnPlayerExit(Player* player) const;
        bool IsComplete() { return _complete; }

        void SaveToDB();
        void LoadInstanceData(uint32 instanceId);

        void Update(uint32 diff);

        ChallengeMode* GetChallengeMode() { return challengeMode; }

protected:
    void SendAllData(Player const* receiver) const override;

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
    Map* _map;
    ScenarioData const* _data;
    ScenarioWaveGroup* waveGroup;
    ChallengeMode* challengeMode;

private:
    void SendScenarioState();
    void SetStep(int8 step);
    void Boot(Player* player);
    //std::map<uint8, CriteriaProgressMap> StepsCriteriaProgress;
    int8 _step;
    bool _complete;
};

#endif
