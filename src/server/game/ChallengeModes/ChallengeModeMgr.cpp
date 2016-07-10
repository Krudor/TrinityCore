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

void ChallengeModeMgr::LoadChallengeModeRecords()
{
    _challengeModeRecords.clear();
    _challengeModePlayerRecords.clear();
    _challengeModeRealmRecords.clear();
    _challengeModeGuildRecords.clear();

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHALLENGE_MODE_GROUPS);
    PreparedQueryResult result = CharacterDatabase.Query(stmt);

    uint32 oldMSTime = getMSTime();

    if (!result)
    {
        TC_LOG_ERROR("server.loading", ">> Loaded 0 challenge mode group records, table is empty!");
        return;
    }

    uint32 count = 0;
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
        _challengeModeRecords.push_back(group);
        ++count;
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u challenge mode groups in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
    oldMSTime = getMSTime();

    for (auto group : _challengeModeRecords)
    {
        UpdatePlayerRecords(group);
        UpdateRealmLeaderboards(group);
        if (group->GuildId)
            UpdateGuildLeaderboards(group);
    }

    TC_LOG_INFO("server.loading", ">> Processed %u challenge mode groups in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ChallengeModeMgr::LoadChallengeModeData()
{
    uint32 oldMSTime = getMSTime();

    //                                                 0               1                            2                        3                     4                     5                      6                    7                   8                  9                  10                      11                     12
    QueryResult result = WorldDatabase.Query("SELECT entry, consolidationRewardFirst, consolidationRewardConsecutive, bronzeRewardFirst, bronzeRewardConsecutive, silverRewardFirst, silverRewardConsecutive, goldRewardFirst, goldRewardConsecutive, outofboundsID, goldTeleportSpellReward, realmBestAchievement, realmBestTemporaryTitle FROM challenge_mode_data");
    if (!result)
    {
        TC_LOG_ERROR("server.loading", ">> Loaded 0 challenge mode data records, table is empty!");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();
        uint32 entry = fields[0].GetUInt16();
        uint32 consolidationRewardFirst = fields[1].GetUInt32();
        uint32 consolidationRewardConsecutive = fields[2].GetUInt32();
        uint32 bronzeRewardFirst = fields[3].GetUInt32();
        uint32 bronzeRewardConsecutive = fields[4].GetUInt32();
        uint32 silverRewardFirst = fields[5].GetUInt32();
        uint32 silverRewardConsecutive = fields[6].GetUInt32();
        uint32 goldRewardFirst = fields[7].GetUInt32();
        uint32 goldRewardConsecutive = fields[8].GetUInt32();
        uint32 outOfBoundsId = fields[9].GetUInt16();
        uint32 goldTeleportSpellReward = fields[10].GetUInt32();
        uint32 realmBestAchievement = fields[11].GetUInt32();
        uint32 realmBestTemporaryTitle = fields[12].GetUInt32();

        if (!sMapChallengeModeStore.LookupEntry(entry))
        {
            TC_LOG_ERROR("sql.sql", "Table `challenge_mode_data` column 'entry' (%u) is not a valid MapChallengMode.db2 id, skipped!", entry);
            continue;
        }

        ChallengeModeQuestReward* consolidationReward = new ChallengeModeQuestReward();
        if (consolidationRewardFirst)
        {
            consolidationReward->First = sObjectMgr->GetQuestTemplate(consolidationRewardFirst);
            if (!consolidationReward->First)
                TC_LOG_ERROR("sql.sql", "Table `challenge_mode_data` contained an invalid 'consolidationRewardFirst' Id (%u) for challenge mode entry (%u)!", consolidationRewardFirst, entry);
        }
        if (consolidationRewardConsecutive)
        {
            consolidationReward->Consecutive = sObjectMgr->GetQuestTemplate(consolidationRewardConsecutive);
            if (!consolidationReward->Consecutive)
                TC_LOG_ERROR("sql.sql", "Table `challenge_mode_data` contained an invalid 'consolidationRewardConsecutive' Id (%u) for challenge mode entry (%u)!", consolidationRewardConsecutive, entry);
        }

        ChallengeModeQuestReward* bronzeReward = new ChallengeModeQuestReward();;
        if (bronzeRewardFirst)
        {
            bronzeReward->First = sObjectMgr->GetQuestTemplate(bronzeRewardFirst);
            if (!bronzeReward->First)
                TC_LOG_ERROR("sql.sql", "Table `challenge_mode_data` contained an invalid 'bronzeRewardFirst' Id (%u) for challenge mode entry (%u)!", bronzeRewardFirst, entry);
        }
        if (bronzeRewardConsecutive)
        {
            bronzeReward->Consecutive = sObjectMgr->GetQuestTemplate(bronzeRewardConsecutive);
            if (!bronzeReward->Consecutive)
                TC_LOG_ERROR("sql.sql", "Table `challenge_mode_data` contained an invalid 'bronzeRewardConsecutive' Id (%u) for challenge mode entry (%u)!", bronzeRewardConsecutive, entry);
        }

        ChallengeModeQuestReward* silverReward = new ChallengeModeQuestReward();;
        if (silverRewardFirst)
        {
            silverReward->First = sObjectMgr->GetQuestTemplate(silverRewardFirst);
            if (!silverReward->First)
                TC_LOG_ERROR("sql.sql", "Table `challenge_mode_data` contained an invalid 'silverRewardFirst' Id (%u) for challenge mode entry (%u)!", silverRewardFirst, entry);
        }
        if (silverRewardConsecutive)
        {
            silverReward->Consecutive = sObjectMgr->GetQuestTemplate(silverRewardConsecutive);
            if (!silverReward->Consecutive)
                TC_LOG_ERROR("sql.sql", "Table `challenge_mode_data` contained an invalid 'silverRewardConsecutive' Id (%u) for challenge mode entry (%u)!", silverRewardConsecutive, entry);
        }

        ChallengeModeQuestReward* goldReward = new ChallengeModeQuestReward();;
        if (goldRewardFirst)
        {
            goldReward->First = sObjectMgr->GetQuestTemplate(goldRewardFirst);
            if (!goldReward->First)
                TC_LOG_ERROR("sql.sql", "Table `challenge_mode_data` contained an invalid 'goldRewardFirst' Id (%u) for challenge mode entry (%u)!", goldRewardFirst, entry);
        }
        if (goldRewardConsecutive)
        {
            goldReward->Consecutive = sObjectMgr->GetQuestTemplate(goldRewardConsecutive);
            if (!goldReward->Consecutive)
                TC_LOG_ERROR("sql.sql", "Table `challenge_mode_data` contained an invalid 'goldRewardConsecutive' Id (%u) for challenge mode entry (%u)!", goldRewardConsecutive, entry);
        }

        if (outOfBoundsId && !sAreaTriggerStore.LookupEntry(outOfBoundsId))
        {
            TC_LOG_ERROR("sql.sql", "Table `challenge_mode_data` column 'outofboundsID' (%u) is not a valid AreaTrigger.dbc id, setting value to 0!", outOfBoundsId);
            outOfBoundsId = 0;
        }

        SpellEntry const* spellReward = nullptr;
        if (goldTeleportSpellReward)
        {
            spellReward = sSpellStore.LookupEntry(goldTeleportSpellReward);
            if (!spellReward)
                TC_LOG_ERROR("sql.sql", "Table `challenge_mode_data` column 'goldTeleportSpellReward' (%u) is not a valid Spell.dbc id!", goldTeleportSpellReward);
        }

        AchievementEntry const* achievement = nullptr;
        if (realmBestAchievement)
        {
            achievement = sAchievementStore.LookupEntry(realmBestAchievement);
            if (!achievement)
                TC_LOG_ERROR("sql.sql", "Table `challenge_mode_data` column `realmBestAchievement` (%u) is not a valid Achievement.db2 id!", realmBestAchievement);
        }

        CharTitlesEntry const* title = nullptr;
        if (realmBestTemporaryTitle)
        {
            title = sCharTitlesStore.LookupEntry(realmBestTemporaryTitle);
            if (!title)
                TC_LOG_ERROR("sql.sql", "Table `challenge_mode_data` column 'realmBestTemporaryTitle' (%u) is not a valid CharTitles.dbc id!", realmBestTemporaryTitle);
        }

        ChallengeModeData* data = new ChallengeModeData();
        data->QuestRewards[CM_MEDAL_NONE] = consolidationReward;
        data->QuestRewards[CM_MEDAL_BRONZE] = bronzeReward;
        data->QuestRewards[CM_MEDAL_SILVER] = silverReward;
        data->QuestRewards[CM_MEDAL_GOLD] = goldReward;
        data->OutOfBoundsId = outOfBoundsId;
        data->TeleportSpellReward = spellReward;
        data->RealmBestAchievement = achievement;
        data->RealmBestTitleReward = title;
        _challengeModeData[entry] = data;

        ++count;
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u challenge mode data in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

ChallengeModeData const* ChallengeModeMgr::GetChallengeModeData(uint32 challengeModeId)
{
    ChallengeModeInfo::const_iterator itr = _challengeModeData.find(challengeModeId);
    if (itr == _challengeModeData.end())
        return nullptr;

    return itr->second;
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

void ChallengeModeMgr::BuildMapStatsResult(WorldPackets::ChallengeMode::ChallengeModeRequestMapStatsResult &result, Player* receiver)
{
    for (auto map : _challengeModePlayerRecords[receiver->GetGUID()])
    {
        if (!map.second.empty())
        {
            std::pair<uint32, WorldPackets::ChallengeMode::ChallengeModeMapStats> mapStats;
            mapStats.first = map.first;
            BuildMapUpdateResult(mapStats.second, receiver, map.first);
            result.AllMapStats.insert(mapStats);
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

    WorldPackets::ChallengeMode::ChallengeModeLeader* realmLeaders = _challengeModeRealmRecords[request->MapID];
	if (realmLeaders && realmLeaders->LastUpdated > request->LastRealmUpdate)
	{
		result.RealmLeaderboards->Groups = realmLeaders->Groups;
        result.RealmLeaderboards->LastUpdated = realmLeaders->LastUpdated;
	}

    WorldPackets::ChallengeMode::ChallengeModeLeader* guildLeaders = _challengeModeGuildRecords[source->GetGuildId()][request->MapID];
	if (guildLeaders && guildLeaders->LastUpdated > request->LastGuildUpdate)
	{
		result.GuildLeaderboards->Groups = guildLeaders->Groups;
		result.GuildLeaderboards->LastUpdated = guildLeaders->LastUpdated;
    }
}

void ChallengeModeMgr::BuildRewardsResult(WorldPackets::ChallengeMode::ChallengeModeRewards& result)
{
    for (uint32 i = 0; i < sMapChallengeModeStore.GetNumRows(); ++i)
    {
        WorldPackets::ChallengeMode::ChallengeModeReward reward;
        MapChallengeModeEntry const* entry = sMapChallengeModeStore.LookupEntry(i);
        if (!entry)
            continue;

        reward.MapId = entry->MapID;
        for (int i = 0; i < CM_MEDAL_MAX; ++i)
        {
            if (Quest const* rewardQuest = GetRewardQuest(entry->ID, i, true))
            {
                reward.MoneyReward = rewardQuest->RewardMoney;

                for (uint8 i = 0; i < QUEST_REWARD_CURRENCY_COUNT; ++i)
                {
                    if (rewardQuest->RewardCurrencyId[i])
                    {
                        WorldPackets::ChallengeMode::CurrencyReward currencyReward;
                        currencyReward.CurrencyId = rewardQuest->RewardCurrencyId[i];
                        currencyReward.Quantity = rewardQuest->RewardCurrencyCount[i];
                        reward.CurrencyRewards.push_back(currencyReward);
                    }
                }

                if (rewardQuest->GetRewItemsCount() > 0)
                {
                    for (uint32 i = 0; i < rewardQuest->GetRewItemsCount(); ++i)
                    {
                        if (uint32 itemId = rewardQuest->RewardItemId[i])
                        {
                            WorldPackets::ChallengeMode::ItemReward itemReward;
                            itemReward.Quantity = rewardQuest->RewardItemCount[i];

                            WorldPackets::Item::ItemInstance itemInstance;
                            itemInstance.ItemID = itemId;

                            reward.ItemRewards.push_back(itemReward);
                        }
                    }
                }
            }
        }
        result.CMRewards.push_back(reward);
        
        // Packet supports another set of item rewards, not specific to any challenge mode, what is this for?
    }
}

void ChallengeModeMgr::BuildMapUpdateResult(WorldPackets::ChallengeMode::ChallengeModeMapStats& result, Player* receiver, uint32 map)
{
    std::list<WorldPackets::ChallengeMode::ChallengeModeGroup*> groups = _challengeModePlayerRecords[receiver->GetGUID()][map];
    result.MapId = map;

    for (auto group : groups)
    {
        if (!result.BestCompletionTime || result.BestCompletionTime > group->CompletionTime)
        {
            result.BestCompletionTime = group->CompletionTime;
            result.BestCompletionDate = group->CompletionDate;
            result.Medal = group->MedalEarned;

            result.Specs.clear();
            for (auto player : group->Players)
                result.Specs.push_back(player->SpecializationId);
        }
        if (!result.LastCompletionDate || result.LastCompletionDate < group->CompletionDate)
        {
            result.LastCompletionTime = group->CompletionTime;
            result.LastCompletionDate = group->CompletionDate;
        }
    }
}

bool cm_by_time(WorldPackets::ChallengeMode::ChallengeModeGroup* a, WorldPackets::ChallengeMode::ChallengeModeGroup* b)
{
    return a->CompletionTime < b->CompletionTime;
}

void ChallengeModeMgr::UpdateRealmLeaderboards(WorldPackets::ChallengeMode::ChallengeModeGroup* group)
{
    ChallengeModeRecordsByMap::iterator itr = _challengeModeRealmRecords.find(group->MapId);
    if (itr == _challengeModeRealmRecords.end())
    {
        _challengeModeRealmRecords[group->MapId] = new WorldPackets::ChallengeMode::ChallengeModeLeader();
        itr = _challengeModeRealmRecords.find(group->MapId);
    }

    ASSERT(itr != _challengeModeRealmRecords.end());

    bool add = itr->second->Groups.empty();
    for (auto groupItr = itr->second->Groups.begin(); groupItr != itr->second->Groups.end(); ++groupItr)
    {
        if ((*groupItr)->CompletionTime > group->CompletionTime)
        {
            add = true;
            break;
        }
    }

    if (!add)
        return;

    itr->second->Groups.push_back(group);
    itr->second->LastUpdated = time(nullptr);
    itr->second->Groups.sort(cm_by_time);
    itr->second->Groups.resize(itr->second->Groups.size() > MAX_REALM_LEADERS ? MAX_REALM_LEADERS : itr->second->Groups.size());
}

void ChallengeModeMgr::UpdateGuildLeaderboards(WorldPackets::ChallengeMode::ChallengeModeGroup* group)
{
    ChallengeModeGuildRecords::iterator itr = _challengeModeGuildRecords.find(group->GuildId);
    if (itr == _challengeModeGuildRecords.end())
        _challengeModeGuildRecords[group->GuildId][group->MapId] = new WorldPackets::ChallengeMode::ChallengeModeLeader();
    else
    {
        ChallengeModeRecordsByMap::iterator mapItr = itr->second.find(group->MapId);
        if (mapItr == itr->second.end())
            _challengeModeGuildRecords[group->GuildId][group->MapId] = new WorldPackets::ChallengeMode::ChallengeModeLeader();
    }

    WorldPackets::ChallengeMode::ChallengeModeLeader* leaders = _challengeModeGuildRecords[group->GuildId][group->MapId];
    ASSERT(leaders);

    bool add = leaders->Groups.empty();
    for (auto groupItr = leaders->Groups.begin(); groupItr != leaders->Groups.end(); ++groupItr)
    {
        if ((*groupItr)->CompletionTime > group->CompletionTime)
        {
            add = true;
            break;
        }
    }

    if (!add)
        return;

    leaders->Groups.push_back(group);
    leaders->LastUpdated = time(nullptr);
    leaders->Groups.sort(cm_by_time);
    leaders->Groups.resize(leaders->Groups.size() > MAX_GUILD_LEADERS ? MAX_GUILD_LEADERS : leaders->Groups.size());
}

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

void ChallengeModeMgr::UpdatePlayerRecords(WorldPackets::ChallengeMode::ChallengeModeGroup* group)
{
    for (auto player : group->Players)
        _challengeModePlayerRecords[player->Guid][group->MapId].push_back(group);
}

void ChallengeModeMgr::RegisterToLeaderboards(WorldPackets::ChallengeMode::ChallengeModeGroup* group)
{
    _challengeModeRecords.push_back(group);
    UpdatePlayerRecords(group);
    UpdateRealmLeaderboards(group);
    if (group->GuildId)
        UpdateGuildLeaderboards(group);
}

uint32 ChallengeModeMgr::GenerateAttemptId()
{
    if (NextAttemptId >= 0xFFFFFFFE)
    {
        TC_LOG_ERROR("challenge_mode_attempt", "Challenge Mode Attempt Ids overflow!! Can't continue, shutting down server.");
        World::StopNow(ERROR_EXIT_CODE);
    }
    return NextAttemptId++;
}

Quest const* ChallengeModeMgr::GetRewardQuest(uint32 challengeModeId, uint32 medal, bool first) const
{
    ChallengeModeInfo::const_iterator itr = _challengeModeData.find(challengeModeId);
    if (itr == _challengeModeData.end())
        return nullptr;

    
    ChallengeModeData const* data = itr->second;
    if (!data)
        return nullptr;

    return first && data->QuestRewards[medal]->First ? data->QuestRewards[medal]->First : data->QuestRewards[medal]->Consecutive;
}

WorldPackets::ChallengeMode::ChallengeModeGroup* ChallengeModeMgr::GetMapRecordForPlayer(uint32 map, ObjectGuid playerGuid)
{
    WorldPackets::ChallengeMode::ChallengeModeGroup* record = nullptr;
    std::list<WorldPackets::ChallengeMode::ChallengeModeGroup*> groups = _challengeModePlayerRecords[playerGuid][map];
    for (auto group : groups)
        if (!record || record->CompletionTime > group->CompletionTime)
            record = group;

    return record;
}

WorldPackets::ChallengeMode::ChallengeModeGroup* ChallengeModeMgr::GetRealmRecordForMap(uint32 map)
{
    ChallengeModeRecordsByMap::iterator itr = _challengeModeRealmRecords.find(map);
     return itr != _challengeModeRealmRecords.end() ? itr->second->Groups.front() : nullptr;
}

void ChallengeModeMgr::UpdateRealmBestTitles(Player* player)
{
    for (auto map : _challengeModePlayerRecords[player->GetGUID()])
    {
        MapChallengeModeEntry const* cmEntry = GetChallengeModeEntry(map.first);
        if (!cmEntry)
            continue;

        ChallengeModeData const* cmData = GetChallengeModeData(cmEntry->ID);
        if (!cmData)
            continue;

        if (!player->HasTitle(cmData->RealmBestTitleReward))
            continue;

        WorldPackets::ChallengeMode::ChallengeModeGroup* playerRecord = GetMapRecordForPlayer(map.first, player->GetGUID());
        if (playerRecord != GetRealmRecordForMap(map.first))
            player->SetTitle(cmData->RealmBestTitleReward, true);
    }
}
