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

#ifndef __CHALLENGEMODEMGR_H
#define __CHALLENGEMODEMGR_H

#include "Common.h"
#include "DBCEnums.h"
#include "DB2Stores.h"
#include "Player.h"

namespace WorldPackets
{
    namespace ChallengeMode
    {
        struct PlayerEntry;
        struct ChallengeModeGroup;
        struct ChallengeModeLeader;

        class ChallengeModeRequestLeaders;
        class ChallengeModeRequestLeadersResult;
        class ChallengeModeRequestMapStats;
        class ChallengeModeRequestMapStatsResult;
        class ChallengeModeMapStatsUpdate;
        class ChallengeModeNewPlayerRecord;
        class ChallengeModeMapStats;
    }
}

struct ChallengeModeQuestReward
{
    Quest const* First = nullptr;
    Quest const* Consecutive = nullptr;
};

struct ChallengeModeData
{
    uint32 OutOfBoundsId = 0;
    ChallengeModeQuestReward const* QuestRewards[CM_MEDAL_MAX];
    SpellEntry const* TeleportSpellReward;
    AchievementEntry const* RealmBestAchievement;
    CharTitlesEntry const* RealmBestTitleReward;
};

typedef std::unordered_map<uint32, ChallengeModeData const*> ChallengeModeInfo;

typedef std::list<WorldPackets::ChallengeMode::ChallengeModeGroup*> ChallengeModeRecords;
typedef std::unordered_map<ObjectGuid, std::map<uint32, std::list<WorldPackets::ChallengeMode::ChallengeModeGroup*>>> ChallengeModeRecordsByPlayerGuid;
typedef std::map<uint32, WorldPackets::ChallengeMode::ChallengeModeLeader*> ChallengeModeRecordsByMap;
typedef std::map<uint64, ChallengeModeRecordsByMap> ChallengeModeGuildRecords;

int const MAX_GUILD_LEADERS = 5;
int const MAX_REALM_LEADERS = 1;

class TC_GAME_API ChallengeModeMgr
{
private:
    ChallengeModeMgr();
    ~ChallengeModeMgr() { }

public:
    static ChallengeModeMgr* instance()
    {
        static ChallengeModeMgr instance;
        return &instance;
    }

    static uint64 GetGuildIdFromGroup(std::list<Player*> players, Difficulty difficulty)
    {
        uint8 playersRequired;
        double pctRequired;
        switch (difficulty)
        {
            case DIFFICULTY_NORMAL:
            case DIFFICULTY_HEROIC:
            case DIFFICULTY_MYTHIC:
            case DIFFICULTY_TIMEWALKER:
                pctRequired = 0.6;
                break;
            case DIFFICULTY_N_SCENARIO:
            case DIFFICULTY_HC_SCENARIO:
                pctRequired = 1;
                break;
            case DIFFICULTY_40:
            case DIFFICULTY_10_N:
            case DIFFICULTY_10_HC:
            case DIFFICULTY_25_N:
            case DIFFICULTY_25_HC:
            case DIFFICULTY_NORMAL_RAID:
            case DIFFICULTY_HEROIC_RAID:
            case DIFFICULTY_MYTHIC_RAID:
                pctRequired = 0.8;
                break;
            default:
                pctRequired = 1;
                break;
        }

        playersRequired = ceil(players.size() * pctRequired);

        std::map<uint64, uint8> GuildCount;
        for (Player* player : players)
        {
            uint64 guildId = player->GetGuildId();
            if (!guildId)
                continue;

            ++GuildCount[guildId];
        }

        for (std::pair<uint64, uint8> guild : GuildCount)
        {
            if (guild.second >= playersRequired)
                return guild.first;
        }

        return NULL;
    }
    MapChallengeModeEntry const* GetChallengeModeEntry(uint32 mapId) const;

    void LoadChallengeModeRecords();

    void LoadChallengeModeData();
    ChallengeModeData const* GetChallengeModeData(uint32 challengeModeId);

    void BuildMapStatsResult(WorldPackets::ChallengeMode::ChallengeModeRequestMapStatsResult &result, Player* receiver);
    void BuildLeadersResult(WorldPackets::ChallengeMode::ChallengeModeRequestLeaders const* request, WorldPackets::ChallengeMode::ChallengeModeRequestLeadersResult &result, Player* receiver);
    void BuildRewardsResult(WorldPackets::ChallengeMode::ChallengeModeRewards &result);
    void BuildMapUpdateResult(WorldPackets::ChallengeMode::ChallengeModeMapStats &result, Player* receiver, uint32 map);

    std::list<WorldPackets::ChallengeMode::PlayerEntry*> LoadPlayersForAttempt(uint32 attemptId);

    std::list<WorldPackets::ChallengeMode::PlayerEntry*> LoadPlayerRecords(bool reload = false);
    void LoadOutOfBoundsAreatriggerData();

    void UpdatePlayerRecords(WorldPackets::ChallengeMode::ChallengeModeGroup* group);
    void UpdateRealmLeaderboards(WorldPackets::ChallengeMode::ChallengeModeGroup* group);
    void UpdateGuildLeaderboards(WorldPackets::ChallengeMode::ChallengeModeGroup* group);

    void RegisterToLeaderboards(WorldPackets::ChallengeMode::ChallengeModeGroup* group);

    uint32 GenerateAttemptId();
    void SetNextAttemptId(uint32 Id) { NextAttemptId = Id; }

    uint32 NextAttemptId;

    uint32 GetChallengeModeResetCooldown() { return _resetChallengeModeCooldown; }

    Quest const* GetRewardQuest(uint32 challengeModeId, uint32 medal, bool first) const;
    WorldPackets::ChallengeMode::ChallengeModeGroup* GetMapRecordForPlayer(uint32 map, ObjectGuid playerGuid);
    WorldPackets::ChallengeMode::ChallengeModeGroup* GetRealmRecordForMap(uint32 map);
    void UpdateRealmBestTitles(Player* player);

    void Update(uint32 diff);

private:
    uint32 _resetChallengeModeCooldown;
    ChallengeModeInfo _challengeModeData;

    ChallengeModeRecords _challengeModeRecords;
    ChallengeModeRecordsByPlayerGuid _challengeModePlayerRecords;
    ChallengeModeGuildRecords _challengeModeGuildRecords;
    ChallengeModeRecordsByMap _challengeModeRealmRecords;
};

#define sChallengeModeMgr ChallengeModeMgr::instance()

#endif // __CHALLENGEMODEMGR_H
