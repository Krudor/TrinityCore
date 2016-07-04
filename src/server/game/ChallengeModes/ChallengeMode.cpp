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

#include "ChallengeMode.h"
#include "ChallengeModePackets.h"
#include "ChallengeModeMgr.h"
#include "MiscPackets.h"
#include "InstanceScript.h"
#include "SpellInfo.h"
#include "ScriptedCreature.h"

class InstanceResetWorker
{
public:
    InstanceResetWorker() { }

    void Visit(std::unordered_map<ObjectGuid, Creature*>& creatureMap)
    {
        for (auto const& p : creatureMap)
        {
            if (p.second->IsInWorld())
            {
                if (NewReset_BossAI* ai = dynamic_cast<NewReset_BossAI*>(p.second->AI()))
                    ai->FullReset();
                else if (NewReset_ScriptedAI* ai = dynamic_cast<NewReset_ScriptedAI*>(p.second->AI()))
                    ai->FullReset();
            }
        }
    }

    void Visit(std::unordered_map<ObjectGuid, GameObject*>& gameObjectMap)
    {
        for (auto const& p : gameObjectMap)
        {
            if (GameObject* gameobject = p.second)
            {
                if (!gameobject->IsInWorld())
                    continue;

                switch (gameobject->GetEntry())
                {
                    case 211674:
                        gameobject->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                    case 212387:
                    case 211988:
                    case 211992:
                    case 211989:
                    case 211991:
                    case 211972:
                        gameobject->SetGoState(GO_STATE_READY);
                        break;
                }

                gameobject->Respawn();
            }
        }
    }

    template<class T>
    void Visit(std::unordered_map<ObjectGuid, T*>&) { }
};

class ChallengeModeStartWorker
{
public:
    ChallengeModeStartWorker() { }

    void Visit(std::unordered_map<ObjectGuid, GameObject*>& gameObjectMap)
    {
        for (auto const& p : gameObjectMap)
        {
            if (GameObject* gameobject = p.second)
            {
                if (!gameobject->IsInWorld())
                    continue;

                switch (gameobject->GetEntry())
                {
                    case 211674:
                        gameobject->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                    case 212387:
                    case 211988:
                    case 211992:
                    case 211989:
                    case 211991:
                    case 211972:
                        gameobject->SetGoState(GO_STATE_ACTIVE);
                        break;
                }
            }
        }
    }

    template<class T>
    void Visit(std::unordered_map<ObjectGuid, T*>&) { }
};

ChallengeMode::ChallengeMode(Map* map) : instance(map), data(nullptr), /*timeStart(0),*/ timeComplete(NULL), lastReset(NULL), outOfBoundsAreatriggerId(0), _state(0), _startedByTimer(false), elapsedTimerId(0), _elapsedTimers(NULL)
{
    if (!map || map->GetDifficultyID() != DIFFICULTY_CHALLENGE)
        return;

    MapChallengeModeEntry const* entry = sChallengeModeMgr->GetChallengeModeEntry(map->GetEntry()->ID);
    data = entry;

    ChallengeModeData const* cmData = sObjectMgr->GetChallengeModeData(data->ID);
    if (cmData)
    {
        outOfBoundsAreatriggerId = cmData->OutOfBoundsId;
        //More data?
    }

    _elapsedTimers = new ElapsedTimers(instance);
}
 
void ChallengeMode::Start()
{
    if (!CanStart())
        return;

    _state = 1;

    if (InstanceMap* instanceMap = instance->ToInstanceMap())
        if (InstanceScript* instanceScript = instanceMap->GetInstanceScript())
            instanceScript->OnChallengeModeStart(data);

    std::list<Player*> players;
    Map::PlayerList const& playerlist = instance->GetPlayers();
    for (Map::PlayerList::const_iterator itr = playerlist.begin(); itr != playerlist.end(); ++itr)
        if (Player* player = itr->GetSource())
            player->SetInPhase(170, true, false);

    ChallengeModeStartWorker worker;
    TypeContainerVisitor<ChallengeModeStartWorker, MapStoredObjectTypesContainer> visitor(worker);
    visitor.Visit(instance->GetObjectsStore());

    WorldPackets::ChallengeMode::ChallengeModeStart packet;
    packet.MapId = data->MapID;
    instance->SendToPlayers(packet.Write());

    elapsedTimerId = _elapsedTimers->CreateElapsedTimer();
    _elapsedTimers->Start(elapsedTimerId);
    //timeStart = getMSTime();
}

void ChallengeMode::Start(uint32 countdownSeconds)
{
    if (countdownSeconds < 1)
    {
        Start();
        return;
    }

    if (!CanStart())
        return;

    WorldPackets::Misc::StartTimer timer;
    timer.Type = timer.ChallengeMode;
    timer.Time = countdownSeconds;
    timer.TotalTime = countdownSeconds;
    instance->SendToPlayers(timer.Write());

    ChallengeModeStartEvent* startEvent = new ChallengeModeStartEvent(this);
    _events.AddEvent(startEvent, _events.CalculateTime(countdownSeconds * IN_MILLISECONDS));
}

void ChallengeMode::Reset(Player* source)
{
    if (_state != 1 && _state != 3)
        return;

	Complete();
	return;

    uint32 sinceReset = GetMSTimeDiffToNow(lastReset) / IN_MILLISECONDS;
    if (sinceReset < sChallengeModeMgr->GetChallengeModeResetCooldown())
    {
        WorldPackets::Misc::DisplayGameError error;
        error.Error = 858;
        error.Argument1 = sChallengeModeMgr->GetChallengeModeResetCooldown() - sinceReset;
        source->SendDirectMessage(error.Write());
        return;
    }

    lastReset = getMSTime();
    _state = 0;
    uint32 attemptTime = _elapsedTimers->Stop(elapsedTimerId);
    //timeStart = 0;

    if (InstanceMap* instanceMap = instance->ToInstanceMap())
    {
        if (InstanceScript* instanceScript = instanceMap->GetInstanceScript())
            instanceScript->OnChallengeModeReset(data, attemptTime);
        if (Scenario* scenario = instanceMap->GetScenario())
            scenario->Reset();
    }


    std::list<Player*> players;
    Map::PlayerList const& playerlist = instance->GetPlayers();
    for (Map::PlayerList::const_iterator itr = playerlist.begin(); itr != playerlist.end(); ++itr)
    {
        if (Player* player = itr->GetSource())
        {
            player->SetInPhase(170, true, true);
            player->GetSpellHistory()->ResetCooldowns([](SpellHistory::CooldownStorageType::iterator itr) -> bool
            {
                SpellInfo const* spellInfo = sSpellMgr->AssertSpellInfo(itr->first);
                return spellInfo->GetCategory() == 1407;
            }, true);
        }
    }

    bool teleported = false;
    if (InstanceMap* instanceMap = instance->ToInstanceMap())
    {
        if (InstanceMapData const* mapData = instanceMap->GetMapData())
        {
            if (WorldSafeLocsEntry const* entrance = sWorldSafeLocsStore.LookupEntry(mapData->EntranceId))
            {
                teleported = true;
                std::list<Player*> players;
                Map::PlayerList const& playerlist = instance->GetPlayers();
                for (Map::PlayerList::const_iterator itr = playerlist.begin(); itr != playerlist.end(); ++itr)
                    if (Player* player = itr->GetSource())
                        player->TeleportTo(entrance->MapID, entrance->Loc.X, entrance->Loc.Y, entrance->Loc.Z, entrance->Facing * M_PI / 180);
            }
        }
    }

    if (!teleported)
    {
        instance->RemoveAllPlayers();
    }

    for (Map::PlayerList::const_iterator itr = playerlist.begin(); itr != playerlist.end(); ++itr)
    {
        if (Player* player = itr->GetSource())
        {
            player->GetSpellHistory()->ResetCooldowns([](SpellHistory::CooldownStorageType::iterator itr) -> bool
            {
                SpellInfo const* spellInfo = sSpellMgr->AssertSpellInfo(itr->first);
                return spellInfo->RecoveryTime >= 5 * MINUTE * IN_MILLISECONDS
                    && spellInfo->CategoryRecoveryTime >= 5 * MINUTE * IN_MILLISECONDS
                    && !itr->second.OnHold;
            }, true);
            player->GetSpellHistory()->ResetCooldown(SPELL_LESSER_INVISIBILITY, true);
            player->GetSpellHistory()->ResetCooldown(SPELL_INVISIBILITY, true);
        }
    }

    InstanceResetWorker worker;
    TypeContainerVisitor<InstanceResetWorker, MapStoredObjectTypesContainer> visitor(worker);
    visitor.Visit(instance->GetObjectsStore());

    WorldPackets::ChallengeMode::ChallengeModeReset reset;
    reset.MapId = data->MapID;
    instance->SendToPlayers(reset.Write());
}

void ChallengeMode::Complete()
{
    _state = 3;
    timeComplete = GetMSTimeDiffToNow(_elapsedTimers->Stop(elapsedTimerId));
    uint32 medal = GetMedal(timeComplete);

    std::list<Player*> players;
    Map::PlayerList const& playerlist = instance->GetPlayers();
    for (Map::PlayerList::const_iterator itr = playerlist.begin(); itr != playerlist.end(); ++itr)
    {
        if (Player* player = itr->GetSource())
        {
            player->UpdateCriteria(CRITERIA_TYPE_COMPLETE_CHALLENGE_MODE, medal);
            player->GetSpellHistory()->ResetCooldowns([](SpellHistory::CooldownStorageType::iterator itr) -> bool
            {
                SpellInfo const* spellInfo = sSpellMgr->AssertSpellInfo(itr->first);
                return spellInfo->GetCategory() == 1407;
            }, true);
        }
    }
    if (InstanceMap* instanceMap = instance->ToInstanceMap())
        if (InstanceScript* instanceScript = instanceMap->GetInstanceScript())
            instanceScript->OnChallengeModeComplete(data, medal);

    SaveAttemptToDb();
}

void ChallengeMode::OnPlayerEnter(Player * player) const
{
    if (_state == 1)
        return;

    player->SetInPhase(170, true, true);
}

void ChallengeMode::OnPlayerExit(Player* player) const
{
	player->SetInPhase(170, true, false);
}

void ChallengeMode::HandleAreaTrigger(Player * source, uint32 trigger, bool entered)
{
    if (trigger == outOfBoundsAreatriggerId && !entered)
        OnPlayerOutOfBounds(source);
}

void ChallengeMode::OnPlayerOutOfBounds(Player * player) const
{
    if (_state != 0 || player->IsGameMaster())
        return;

    TC_LOG_INFO("maps.challengemode", "Detected player out of bounds in challenge mode (map id: %u), returning player to instance entrance.", instance->GetId());
    WorldSafeLocsEntry const* entrance = NULL;
    if (InstanceMap* instanceMap = instance->ToInstanceMap())
        if (InstanceMapData const* mapData = instanceMap->GetMapData())
            entrance = sWorldSafeLocsStore.LookupEntry(mapData->EntranceId);

    if (!entrance)
    {
        TC_LOG_WARN("maps.challengemode", "Tried to teleport an out of bounds player to the instance entrance, but the instance entrance (id: %u) was not found! Removing player from instance.");
        instance->RemovePlayerFromMap(player, true);
        return;
    }

    player->TeleportTo(entrance->MapID, entrance->Loc.X, entrance->Loc.Y, entrance->Loc.Z, entrance->Facing * M_PI / 180);
}

void ChallengeMode::Update(uint32 diff)
{
    _events.Update(diff);
}

void ChallengeMode::SaveAttemptToDb()
{
	WorldPackets::ChallengeMode::ChallengeModeGroup* group = new WorldPackets::ChallengeMode::ChallengeModeGroup();
    group->AttemptId = sChallengeModeMgr->GenerateAttemptId();
    group->CompletionDate = getMSTime();
    group->MedalEarned = GetMedal(timeComplete);

    SQLTransaction trans = CharacterDatabase.BeginTransaction();

    std::list<Player*> players;
    Map::PlayerList const& playerlist = instance->GetPlayers();
    for (Map::PlayerList::const_iterator itr = playerlist.begin(); itr != playerlist.end(); ++itr)
    {
        Player* player = itr->GetSource();
        if (!player || player->IsGameMaster())
            continue;

        WorldPackets::ChallengeMode::PlayerEntry* pEntry = new WorldPackets::ChallengeMode::PlayerEntry();
        pEntry->Guid = player->GetGUID();
        pEntry->SpecializationId = player->GetSpecId(player->GetActiveTalentGroup());

        players.push_back(player);
        group->Players.push_back(pEntry);

        PreparedStatement* memberStmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHALLENGE_MODE_MEMBERS);
        memberStmt->setUInt32(0, group->AttemptId);
        memberStmt->setUInt64(1, pEntry->Guid.GetCounter());
        memberStmt->setUInt32(2, pEntry->SpecializationId);
        trans->Append(memberStmt);
    }

    group->GuildId = sChallengeModeMgr->GetGuildIdFromGroup(players, instance->GetDifficultyID());

    PreparedStatement* attemptStmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHALLENGE_MODE_GROUP);
    attemptStmt->setUInt32(0, group->AttemptId);
    attemptStmt->setUInt32(1, data->MapID);
    attemptStmt->setUInt32(2, group->CompletionTime);
    attemptStmt->setUInt32(3, group->CompletionDate);
    attemptStmt->setUInt32(4, group->MedalEarned);
    attemptStmt->setUInt64(5, group->GuildId);
    trans->Append(attemptStmt);

    CharacterDatabase.CommitTransaction(trans);

    sChallengeModeMgr->RegisterToLeaderboards(group->AttemptId);
}

bool ChallengeModeStartEvent::Execute(uint64 e_time, uint32 p_time)
{
    _ChallengeMode->Start();
    return true;
}

ElapsedTimers::ElapsedTimers(Map* map) : _map(map)
{
    Timers = new std::map<int32, time_t>();
}

int32 ElapsedTimers::CreateElapsedTimer()
{
    int32 id = CreateId();
    if (id < 1)
        return id;

    Timers->insert(std::pair<int32, time_t>(id, 0));
    return id;
}

time_t ElapsedTimers::GetElapsedTimer(uint32 id)
{
    std::map<int32, time_t>::iterator itr = Timers->find(id);
    if (itr == Timers->end())
        return 0;

    return itr->second;
}

void ElapsedTimers::Start(int32 id)
{
    std::map<int32, time_t>::iterator itr = Timers->find(id);
    if (itr == Timers->end())
        return;

    itr->second = getMSTime();

    if (!_map)
    {
        TC_LOG_DEBUG("misc.elapsedtimers", "Unable to broadcast SMSG_START_ELAPSED_TIMER opcode, Map object is null!");
        return;
    }

    WorldPackets::Misc::StartElapsedTimer elapsedTimer;
    elapsedTimer.ElapsedTimer.TimerID = itr->first;
    elapsedTimer.ElapsedTimer.CurrentDuration = GetMSTimeDiffToNow(itr->second) / IN_MILLISECONDS;
    _map->SendToPlayers(elapsedTimer.Write());
}

uint32 ElapsedTimers::Stop(int32 id, bool keepTimer)
{
    std::map<int32, time_t>::iterator itr = Timers->find(id);
    if (itr == Timers->end())
        return 0;

    time_t time = itr->second;
    itr->second = 0;

    if (!_map)
    {
        TC_LOG_DEBUG("misc.elapsedtimer", "Unable to broadcast SMSG_STOP_ELAPSED_TIMER opcode, Map object is null! TimerID: %u, KeepTimer = %s", itr->first, keepTimer ? "true" : "false");
        return time;
    }

    WorldPackets::Misc::StopElapsedTimer elapsedTimer;
    elapsedTimer.TimerID = itr->first;
    elapsedTimer.KeepTimer = keepTimer;
    _map->SendToPlayers(elapsedTimer.Write());

    if (!keepTimer)
        Timers->erase(itr);

    return time;
}

void ElapsedTimers::RemoveTimer(int32 id)
{
    std::map<int32, time_t>::iterator itr = Timers->find(id);
    if (itr == Timers->end())
        return;

    Timers->erase(itr);
}

void ElapsedTimers::BuildElapsedTimers(WorldPackets::Misc::StartElapsedTimers* startElapsedTimers)
{
    std::list<WorldPackets::Misc::ElapsedTimer> elapsedTimers;
    for (auto timer : *Timers)
    {
        WorldPackets::Misc::ElapsedTimer elapsedTimer;
        elapsedTimer.TimerID = timer.first;
        elapsedTimer.CurrentDuration = timer.second;
        elapsedTimers.push_back(elapsedTimer);
    }

    startElapsedTimers->ElapsedTimers = elapsedTimers;
}

int32 ElapsedTimers::CreateId()
{
    std::map<int32, time_t>::iterator itr;
    for (int i = 0; i < std::numeric_limits<int8>::max(); i++)
    {
        itr = Timers->find(i);
        if (itr != Timers->end())
            continue;

        return i + 1;
    }

    TC_LOG_ERROR("misc.elapsedtimers", "ElapsedTimers::CreateId attempted to create timer id over %u, which was a limit set by the developer.", std::numeric_limits<int8>::max());
    return -1;
}
