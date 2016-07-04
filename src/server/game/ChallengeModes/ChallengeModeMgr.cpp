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

#include "Common.h"
#include "ObjectMgr.h"
#include "World.h"
#include "WorldPacket.h"

#include "Player.h"
#include "SharedDefines.h"
#include "DisableMgr.h"
#include "Opcodes.h"
#include "ChallengeModeMgr.h"
#include "ChallengeModePackets.h"
#include "GuildMgr.h"

/*********************************************************/
/***              CHALLENGE MODE MANAGER               ***/
/*********************************************************/

ChallengeModeMgr::ChallengeModeMgr()
{
    _resetChallengeModeCooldown = sWorld->getIntConfig(CONFIG_CHALLENGE_MODE_RESET_COOLDOWN);
}

void ChallengeModeMgr::LoadChallengeModes()
{
    LoadGroups();
    UpdateRealmLeaderboards();
    UpdateGuildLeaderboards();
    //LoadRealmLeaderboards();
    //LoadGuildLeaderboards();
}

MapChallengeModeEntry const* ChallengeModeMgr::GetChallengeModeEntry(uint32 mapId) const
{
    for (uint32 i = 0; i < sMapChallengeModeStore.GetNumRows(); ++i)
    {
        MapChallengeModeEntry const* entry = sMapChallengeModeStore.LookupEntry(i);
        if (!entry || entry->MapID != mapId)
            continue;

        return entry;
    }

    return nullptr;
}

uint32 ChallengeModeMgr::GetOutOfBoundsAreatriggerId(uint32 mapId) const
{
    return uint32();
}

void ChallengeModeMgr::BuildMapStatsResult(WorldPackets::ChallengeMode::ChallengeModeRequestMapStatsResult &result, Player* receiver)
{
    std::list<WorldPackets::ChallengeMode::ChallengeModeGroup*> groups;
    for (auto group : _groups)
    {
        for (auto player : group->Players)
        {
            if (player->Guid != receiver->GetGUID())
                continue;

            groups.push_back(group);
            break;
        }
    }

    for (auto group : groups)
    {
        std::map<uint32, WorldPackets::ChallengeMode::ChallengeModeMapStats*>::iterator itr = result.AllMapStats.find(group->MapId);
        if (itr != result.AllMapStats.end())
        {
            if (itr->second->LastCompletionDate < group->CompletionDate)
            {
                itr->second->LastCompletionTime = group->CompletionTime;
                itr->second->LastCompletionDate = group->CompletionDate;
            }

            if (itr->second->BestCompletionTime && itr->second->BestCompletionTime < group->CompletionTime)
                continue;

            itr->second->BestCompletionTime = group->CompletionTime;
            itr->second->Medal = group->MedalEarned;
            itr->second->BestCompletionDate = group->CompletionDate;
            for (auto player : group->Players)
            {
                itr->second->Specs.push_back(uint16(player->SpecializationId));
            }
        }
        else
        {
            std::pair<uint32, WorldPackets::ChallengeMode::ChallengeModeMapStats*> map;
            map.second = new WorldPackets::ChallengeMode::ChallengeModeMapStats();
            map.first = group->MapId;
            map.second->MapId = group->MapId;
            map.second->LastCompletionTime = group->CompletionTime;
            map.second->BestCompletionTime = group->CompletionTime;
            map.second->Medal = group->MedalEarned;
            map.second->BestCompletionDate = group->CompletionDate;
            map.second->LastCompletionDate = group->CompletionDate;
            for (auto player : group->Players)
            {
                map.second->Specs.push_back(uint16(player->SpecializationId));
            }

            result.AllMapStats.insert(map);
        }
    }
}

void ChallengeModeMgr::BuildLeadersResult(WorldPackets::ChallengeMode::ChallengeModeRequestLeaders const* request, WorldPackets::ChallengeMode::ChallengeModeRequestLeadersResult &result, Player* source)
{
    result.MapId = request->MapID;
	result.RealmLeaderboards = new WorldPackets::ChallengeMode::ChallengeModeLeader();
    result.RealmLeaderboards->LastUpdated = 0;
	result.GuildLeaderboards = new WorldPackets::ChallengeMode::ChallengeModeLeader();
    result.GuildLeaderboards->LastUpdated = 0;

    WorldPackets::ChallengeMode::ChallengeModeLeader* realmLeaders = _realmLeaderboard[request->MapID];
	if (realmLeaders && realmLeaders->LastUpdated > request->LastRealmUpdate)
	{
		result.RealmLeaderboards->Groups = realmLeaders->Groups;
        result.RealmLeaderboards->LastUpdated = realmLeaders->LastUpdated;
	}

    WorldPackets::ChallengeMode::ChallengeModeLeader* guildLeaders = _guildsLeaderboard[source->GetGuildId()][request->MapID];
	if (guildLeaders && guildLeaders->LastUpdated > request->LastGuildUpdate)
	{
		result.GuildLeaderboards->Groups = guildLeaders->Groups;
		result.GuildLeaderboards->LastUpdated = guildLeaders->LastUpdated;
    }
}

bool cm_by_time(WorldPackets::ChallengeMode::ChallengeModeGroup* a, WorldPackets::ChallengeMode::ChallengeModeGroup* b)
{
    return a->CompletionTime < b->CompletionTime;
}

void ChallengeModeMgr::UpdateRealmLeaderboards()
{
    std::set<uint32> maps;
    for (auto group : _groups)
        maps.insert(group->MapId);

    for (auto map : maps)
        UpdateRealmLeaderboards(map);
}

void ChallengeModeMgr::UpdateRealmLeaderboards(uint32 mapId)
{
    std::list<WorldPackets::ChallengeMode::ChallengeModeGroup*> groups;
    for (auto group : _groups)
    {
        if (group->MapId != mapId)
            continue;

        groups.push_back(group);
    }

    if (groups.size() < 1)
        return;

    groups.sort(cm_by_time);
    groups.resize(groups.size() > MAX_REALM_LEADERS ? MAX_REALM_LEADERS : groups.size());

    if (!_realmLeaderboard[mapId])
        _realmLeaderboard[mapId] = new WorldPackets::ChallengeMode::ChallengeModeLeader();

    _realmLeaderboard[mapId]->Groups = groups;
    _realmLeaderboard[mapId]->LastUpdated = time_t(time(NULL));
}

void ChallengeModeMgr::UpdateGuildLeaderboards()
{
    std::set<uint32> maps;
    for (auto group : _groups)
        maps.insert(group->MapId);

    std::set<uint64> guilds;
    for (auto group : _groups)
        if (group->GuildId)
            guilds.insert(group->GuildId);

    for (auto guild : guilds)
        for (auto map : maps)
            UpdateGuildLeaderboards(guild, map);
}

void ChallengeModeMgr::UpdateGuildLeaderboards(uint64 guildId, uint32 mapId)
{
    std::list<WorldPackets::ChallengeMode::ChallengeModeGroup*> groups;
    for (auto group : _groups)
    {
        if (group->MapId != mapId || group->GuildId != guildId)
            continue;

        groups.push_back(group);
    }

    if (groups.size() < 1)
        return;

    groups.sort(cm_by_time);
    groups.resize(groups.size() > MAX_GUILD_LEADERS ? MAX_GUILD_LEADERS : groups.size());

    if (!_guildsLeaderboard[guildId][mapId])
        _guildsLeaderboard[guildId][mapId] = new WorldPackets::ChallengeMode::ChallengeModeLeader();

    _guildsLeaderboard[guildId][mapId]->Groups = groups;
    _guildsLeaderboard[guildId][mapId]->LastUpdated = time_t(time(NULL));
}

//WorldPackets::ChallengeMode::ChallengeModeLeader* ChallengeModeMgr::GetRealmLeaderboards(uint32 mapId, uint32 count)
//{
    //std::map<uint32, WorldPackets::ChallengeMode::ChallengeModeLeader*>::const_iterator itr = _realmLeaderboard.find(mapId);
    //if (itr == _realmLeaderboard.end())
    //    return nullptr;

    //return itr->second;
//}

std::list<WorldPackets::ChallengeMode::PlayerEntry*> ChallengeModeMgr::LoadPlayersForAttempt(uint32 attemptId)
{
    PreparedStatement* playersStmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHALLENGE_MODE_MEMBERS_FOR_ATTEMPT);
	playersStmt->setUInt32(0, attemptId);

    PreparedQueryResult playersResult = CharacterDatabase.Query(playersStmt);

    std::list<WorldPackets::ChallengeMode::PlayerEntry*> players;
    if (playersResult)
    {
		do
		{
			Field* fields = playersResult->Fetch();
			WorldPackets::ChallengeMode::PlayerEntry* player = new WorldPackets::ChallengeMode::PlayerEntry();
			player->AttemptId = attemptId;
			player->Guid = ObjectGuid::Create<HighGuid::Player>(fields[0].GetUInt64());
			player->SpecializationId = fields[1].GetUInt32();

			players.push_back(player);

		} while (playersResult->NextRow());
    }

    return players;
}

//std::list<WorldPackets::ChallengeMode::PlayerEntry*> ChallengeModeMgr::LoadPlayerRecords(bool reload)
//{
//    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHALLENGE_MODE_MEMBERS);
//    PreparedQueryResult result = CharacterDatabase.Query(stmt);
//
//    std::list<WorldPackets::ChallengeMode::PlayerEntry*> players;
//    if (result)
//    {
//        do
//        {
//            Field* fields = result->Fetch();
//
//            WorldPackets::ChallengeMode::PlayerEntry* player = new WorldPackets::ChallengeMode::PlayerEntry();
//            player->AttemptId = fields[0].GetUInt32();
//            player->Guid = ObjectGuid::Create<HighGuid::Player>(fields[1].GetUInt64());
//            player->SpecializationId = fields[2].GetUInt32();
//            players.push_back(player);
//        }
//        while (result->NextRow());
//    }
//
//    return players;
//}

void ChallengeModeMgr::LoadGroups()
{
	_groups.clear();

	PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHALLENGE_MODE_GROUPS);
	PreparedQueryResult result = CharacterDatabase.Query(stmt);

	if (result)
	{
		do
		{
			Field* fields = result->Fetch();

			WorldPackets::ChallengeMode::ChallengeModeGroup* group = new WorldPackets::ChallengeMode::ChallengeModeGroup();
			group->AttemptId = fields[0].GetUInt32();
            group->MapId = fields[1].GetUInt32();
			group->CompletionTime = fields[2].GetUInt32();
			group->CompletionDate = fields[3].GetUInt32();
			group->MedalEarned = fields[4].GetUInt32();
			group->GuildId = fields[5].GetUInt64();
            group->Players = LoadPlayersForAttempt(group->AttemptId);
            _groups.push_back(group);
		}
        while (result->NextRow());
	}
}

void ChallengeModeMgr::LoadOutOfBoundsAreatriggerData()
{
    QueryResult result = WorldDatabase.Query("SELECT garrPlotInstanceId, hordeGameObjectId, hordeX, hordeY, hordeZ, hordeO, hordeAnimKitId, "
        //                      7          8          9         10         11                 12
        "allianceGameObjectId, allianceX, allianceY, allianceZ, allianceO, allianceAnimKitId FROM garrison_plot_finalize_info");
    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 garrison follower class spec abilities. DB table `garrison_plot_finalize_info` is empty.");
        return;
    }
}

//void ChallengeModeMgr::LoadRealmLeaderboards(bool reload)
//{
//    if (reload)
//        _realmLeaderboard.clear();
//
//    std::list<uint32> challengeModeMaps;
//
//    for (uint32 i = 0; i < sMapChallengeModeStore.GetNumRows(); ++i)
//    {
//        MapChallengeModeEntry const* entry = sMapChallengeModeStore.LookupEntry(i);
//        if (!entry)
//            continue;
//
//        challengeModeMaps.push_back(entry->MapID);
//    }
//
//    for (auto map : challengeModeMaps)
//        LoadRealmLeaderboardsForMap(map);
//}

//void ChallengeModeMgr::LoadRealmLeaderboardsForMap(uint32 mapId, bool reload)
//{
//    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHALLENGE_MODE_REALM_LEADERBOARDS);
//    stmt->setUInt32(0, mapId);
//    stmt->setUInt32(1, MAX_GUILD_LEADERS);
//    PreparedQueryResult result = CharacterDatabase.Query(stmt);
//
//    if (result)
//    {
//        do
//        {
//            Field* fields = result->Fetch();
//
//            WorldPackets::ChallengeMode::ChallengeModeGroupEntry* group = new WorldPackets::ChallengeMode::ChallengeModeGroupEntry();
//            group->AttemptId = fields[0].GetUInt32();
//            group->CompletionTime = fields[1].GetUInt32();
//            group->CompletionDate = fields[2].GetUInt32();
//            group->MedalEarned = fields[3].GetUInt32();
//            group->GuildId = fields[4].GetUInt64();
//            group->Players = LoadPlayersForAttempt(group->AttemptId);
//            AddToRealmLeaderboard(mapId, group);
//
//        } while (result->NextRow());
//    }
//}

//void ChallengeModeMgr::LoadGuildLeaderboards(bool reload)
//{
//    if (reload)
//        _guildsLeaderboard.clear();
//
//    std::list<uint32> challengeModeMaps;
//
//    for (uint32 i = 0; i < sMapChallengeModeStore.GetNumRows(); ++i)
//    {
//        MapChallengeModeEntry const* entry = sMapChallengeModeStore.LookupEntry(i);
//        if (!entry)
//            continue;
//
//        challengeModeMaps.push_back(entry->MapID);
//    }
//
//    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHALLENGE_MODE_GUILDS);
//    PreparedQueryResult result = CharacterDatabase.Query(stmt);
//
//    std::list<uint64> guilds;
//    if (result)
//    {
//        do
//        {
//            Field* fields = result->Fetch();
//            guilds.push_back(fields[0].GetUInt64());
//        }
//        while (result->NextRow());
//    }
//
//    for (auto guild : guilds)
//        for (auto map : challengeModeMaps)
//            LoadGuildLeaderboardForMap(guild, map);
//}

//void ChallengeModeMgr::LoadGuildLeaderboardForMap(uint64 guildId, uint32 mapId)
//{
//    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHALLENGE_MODE_GUILD_LEADERBOARDS);
//    stmt->setUInt64(0, guildId);
//    stmt->setUInt32(1, mapId);
//    stmt->setUInt32(2, MAX_GUILD_LEADERS);
//    PreparedQueryResult result = CharacterDatabase.Query(stmt);
//
//    if (result)
//    {
//        do
//        {
//            Field* guildFields = result->Fetch();
//
//            WorldPackets::ChallengeMode::ChallengeModeGroupEntry* group = new WorldPackets::ChallengeMode::ChallengeModeGroupEntry();
//            group->AttemptId = guildFields[0].GetUInt32();
//            group->CompletionTime = guildFields[1].GetUInt32();
//            group->CompletionDate = guildFields[2].GetUInt32();
//            group->MedalEarned = guildFields[3].GetUInt32();
//            group->Players = LoadPlayersForAttempt(group->AttemptId);
//            AddToGuildLeaderboard(mapId, guildId, group);
//
//        } while (result->NextRow());
//    }
//}

void ChallengeModeMgr::RegisterToLeaderboards(uint32 attemptId)
{
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHALLENGE_MODE_GROUP);
    stmt->setUInt32(0, attemptId);

    PreparedQueryResult result = CharacterDatabase.Query(stmt);

    if (result)
    {
        Field* fields = result->Fetch();

        WorldPackets::ChallengeMode::ChallengeModeGroup* group = new WorldPackets::ChallengeMode::ChallengeModeGroup();
        group->MapId = fields[0].GetUInt32();
        group->CompletionTime = fields[1].GetUInt32();
        group->CompletionDate = fields[2].GetUInt32();
        group->MedalEarned = fields[3].GetUInt32();
        group->GuildId = fields[4].GetUInt64();
        group->Players = LoadPlayersForAttempt(group->AttemptId);
        _groups.push_back(group);

        UpdateRealmLeaderboards(group->MapId);
        if (group->GuildId)
            UpdateGuildLeaderboards(group->GuildId, group->MapId);
    }

    // Previous leaderboard structure
    //std::map<uint32, WorldPackets::ChallengeMode::ChallengeModeLeader*>::iterator realmItr = _realmLeaderboard.find(mapId);
    //if (realmItr == _realmLeaderboard.end() || _realmLeaderboard.size() < MAX_REALM_LEADERS || ValidForLeaderboards(mapId, time, realmItr->second))
    //    LoadRealmLeaderboardsForMap(mapId, true);

    //if (guildId)
    //{
    //    std::map<uint64, std::map<uint32, WorldPackets::ChallengeMode::ChallengeModeLeader*>>::iterator guildItr = _guildsLeaderboard.find(guildId);
    //    std::map<uint32, WorldPackets::ChallengeMode::ChallengeModeLeader*>::iterator itr = _realmLeaderboard.find(mapId);
    //    if (guildItr == _guildsLeaderboard.end() || itr == guildItr->second.end() || guildItr->second.size() <  MAX_GUILD_LEADERS || ValidForLeaderboards(mapId, time, itr->second))
    //        LoadGuildLeaderboardForMap(guildId, mapId);
    //}
}

//ChallengeModeGroupEntry* ChallengeModeMgr::LoadAndGetChallengeModeGroup(uint32 attemptId)
//{
//    PreparedStatement* attemptStmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHALLENGE_MODE_RECORD_BY_ID);
//    attemptStmt->setUInt32(0, attemptId);
//    PreparedQueryResult attemptResult = CharacterDatabase.Query(attemptStmt);
//
//    PreparedStatement* membersStmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHALLENGE_MODE_RECORD_MEMBERS_FOR_ATTEMPT);
//    attemptStmt->setUInt32(0, attemptId);
//    PreparedQueryResult membersResult = CharacterDatabase.Query(membersStmt);
//
//    if (!attemptResult)
//    {
//        TC_LOG_WARN("challengemodes", "ChallengeModeMgr::LoadAndGetChallengeModeGroup couldn't find challenge mode attempt with Id = %u in db.", attemptId);
//        return nullptr;
//    }
//
//    if (!membersResult)
//    {
//        TC_LOG_WARN("challengemodes", "ChallengeModeMgr::LoadAndGetChallengeModeGroup couldn't find members for challenge mode attempt with Id = %u in db.", attemptId);
//        return nullptr;
//    }
//
//    Field* attemptFields = attemptResult->Fetch();
//    ChallengeModeGroupEntry* group;
//    group->AttemptId = attemptFields[0].GetUInt32();
//    uint32 mapId = attemptFields[1].GetUInt32();
//    group->CompletionTime = attemptFields[2].GetUInt32();
//    group->CompletionDate = attemptFields[3].GetUInt32();
//    group->MedalEarned = attemptFields[4].GetUInt32();
//    group->GuildId = attemptFields[5].GetUInt64();
//
//    std::list<PlayerEntry*> players;
//    do
//    {
//        Field* membersFields = membersResult->Fetch();
//        PlayerEntry* player;
//        player->AttemptId = membersFields[0].GetUInt32();
//        player->Guid = ObjectGuid::Create<HighGuid::Player>(membersFields[1].GetUInt64());
//        player->SpecializationId = membersFields[2].GetUInt32();
//
//        players.push_back(player);
//    }
//    while (membersResult->NextRow());
//
//    group->Players = players;
//    return group;
//}

//WorldPackets::ChallengeMode::ChallengeModeLeader* ChallengeModeMgr::GetGuildLeaderboards(uint64 guildId, uint32 mapId)
//{
//    std::map<uint64, std::map<uint32, WorldPackets::ChallengeMode::ChallengeModeLeader*>>::const_iterator itr = _guildsLeaderboard.find(guildId);
//    if (itr == _guildsLeaderboard.end())
//        return nullptr;
//
//    std::map<uint32, WorldPackets::ChallengeMode::ChallengeModeLeader*>::const_iterator _itr = itr->second.find(mapId);
//    if (_itr == itr->second.end())
//        return nullptr;
//
//    return _itr->second;
//}



//void ChallengeModeMgr::UpdateLeaderboards(uint32 mapId, std::list<Player*> players, ChallengeModeGroupEntry* group)
//{
//    for (ChallengeModeRealmLeaderboardMap::iterator itr = _realmLeaderboard.begin(); itr != _realmLeaderboard.end(); itr++)
//    {
//        if (IsBetterThanAnyOf(group, itr->second))
//        {
//            UpdateRealmLeaderboards();
//            break;
//        }
//    }
//
//    uint64 guildId = GetGuildIdFromGroup(players, DIFFICULTY_CHALLENGE);
//    if (guildId)
//    {
//        ChallengeModeLeader* leaders = GetGuildLeaderboards(mapId, guildId);
//        for (auto guildGroup : leaders->Groups)
//        {
//            if (IsBetterThanAnyOf(group, leaders))
//            {
//                UpdateGuildLeaderboards(guildId);
//                break;
//            }
//        }
//    }
//}

uint32 ChallengeModeMgr::GenerateAttemptId()
{
    if (NextAttemptId >= 0xFFFFFFFE)
    {
        TC_LOG_ERROR("challenge_mode_attempt", "Challenge Mode Attempt Ids overflow!! Can't continue, shutting down server.");
        World::StopNow(ERROR_EXIT_CODE);
    }
    return NextAttemptId++;
}