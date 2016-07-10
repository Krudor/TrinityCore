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

#include "ChallengeModePackets.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "ChallengeModeMgr.h"

void WorldSession::HandleResetChallengeModeOpcode(WorldPackets::ChallengeMode::ResetChallengeMode& /*packet*/)
{
    if (Group* group = _player->GetGroup())
        if (!group->IsLeader(_player->GetGUID()))
            return;

    if (InstanceMap* instanceMap = _player->GetMap()->ToInstanceMap())
        if (Scenario* scenario = instanceMap->GetScenario())
            if (ChallengeMode* challengeMode = scenario->GetChallengeMode())
                challengeMode->Reset(_player);
}

void WorldSession::HandleChallengeModeRequestLeaders(WorldPackets::ChallengeMode::ChallengeModeRequestLeaders& request)
{
    WorldPackets::ChallengeMode::ChallengeModeRequestLeadersResult result;
    sChallengeModeMgr->BuildLeadersResult(&request, result, GetPlayer());
    SendPacket(result.Write());
} 

void WorldSession::HandleChallengeModeRequestMapStats(WorldPackets::ChallengeMode::ChallengeModeRequestMapStats& /*request*/)
{
    WorldPackets::ChallengeMode::ChallengeModeRequestMapStatsResult result;
    sChallengeModeMgr->BuildMapStatsResult(result, GetPlayer());
    SendPacket(result.Write());
}

void WorldSession::HandleGetChallengeModeRewards(WorldPackets::ChallengeMode::GetChallengeModeRewards& /*request*/)
{
    WorldPackets::ChallengeMode::ChallengeModeRewards result;
    sChallengeModeMgr->BuildRewardsResult(result);
    SendPacket(result.Write());
}