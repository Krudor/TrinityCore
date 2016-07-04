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

#include "ChallengeModeMgr.h"
#include "ChallengeModePackets.h"

ByteBuffer& operator<<(ByteBuffer& data, WorldPackets::ChallengeMode::ChallengeModeGroup const* group)
{
    data << uint32(group->InstanceRealmAddress);
    data << uint32(group->AttemptId);
    data << uint32(group->CompletionTime);
    data << uint32(group->CompletionDate);
    data << uint32(group->MedalEarned);

    std::list<WorldPackets::ChallengeMode::PlayerEntry*> players = group->Players;
    data << uint32(players.size());
    for (auto player : players)
    {
        data << uint32(player->VirtualRealmAddress);
        data << uint32(player->NativeRealmAddress);
        data << player->Guid;
        data << uint32(player->SpecializationId);
    }

    return data;
}

WorldPacket const* WorldPackets::ChallengeMode::ChallengeModeStart::Write()
{
    _worldPacket << MapId;
    return &_worldPacket;
}

WorldPacket const* WorldPackets::ChallengeMode::ChallengeModeReset::Write()
{
    _worldPacket << MapId;
    return &_worldPacket;
}

void WorldPackets::ChallengeMode::ChallengeModeRequestLeaders::Read()
{
    _worldPacket >> MapID;
    _worldPacket >> LastRealmUpdate;
    _worldPacket >> LastGuildUpdate;
}

WorldPacket const* WorldPackets::ChallengeMode::ChallengeModeRequestLeadersResult::Write()
{
    _worldPacket << MapId;
    
    _worldPacket << uint32(RealmLeaderboards->LastUpdated);
    _worldPacket << uint32(GuildLeaderboards->LastUpdated);

    _worldPacket << uint32(RealmLeaderboards->Groups.size());
    _worldPacket << uint32(GuildLeaderboards->Groups.size());

    for (ChallengeModeGroup const* realmGroup : RealmLeaderboards->Groups)
        _worldPacket << realmGroup;

    for (ChallengeModeGroup const* guildGroup : GuildLeaderboards->Groups)
        _worldPacket << guildGroup;

    return &_worldPacket;
}

ByteBuffer& operator<<(ByteBuffer& data, WorldPackets::ChallengeMode::ChallengeModeMapStats const* map)
{
    data << uint32(map->MapId);
    data << uint32(map->BestCompletionTime);
    data << uint32(map->LastCompletionTime);
    data << uint32(map->Medal);
    data << uint32(map->BestCompletionDate);
    data << uint32(map->Specs.size());
    for (auto spec : map->Specs)
        data << uint16(spec);

    return data;
}

WorldPacket const* WorldPackets::ChallengeMode::ChallengeModeRequestMapStatsResult::Write()
{
    _worldPacket << uint32(AllMapStats.size());
    for (auto map : AllMapStats)
        _worldPacket << map.second;

    return &_worldPacket;
}

WorldPacket const* WorldPackets::ChallengeMode::ChallengeModeRequestMapStatsUpdateResult::Write()
{
    _worldPacket << MapStats;
	return &_worldPacket;
}

WorldPacket const* WorldPackets::ChallengeMode::ChallengeModeNewPlayerRecord::Write()
{
    _worldPacket << MapID;
    _worldPacket << Time;
    _worldPacket << Medal;
	return &_worldPacket;
}