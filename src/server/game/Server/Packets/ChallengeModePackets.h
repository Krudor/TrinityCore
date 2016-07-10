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

#ifndef ChallengeModePackets_h__
#define ChallengeModePackets_h__

#include "Packet.h"
#include "ObjectGuid.h"
#include "WorldSession.h"
#include "ItemPackets.h"

namespace WorldPackets
{
    namespace ChallengeMode
    {
        struct PlayerEntry
        {
            uint32 AttemptId = 0;
            uint32 VirtualRealmAddress = 0;
            uint32 NativeRealmAddress = 0;
            ObjectGuid Guid;
            uint32 SpecializationId = 0;
        };

        //struct ChallengeModeGroupEntry
        //{
        //    uint32 AttemptId = 0;
        //    uint32 MapId = 0;
        //    uint32 InstanceRealmAddress = 0;
        //    uint64 GuildId = 0;
        //    uint32 CompletionTime = 0;
        //    uint32 CompletionDate = 0;
        //    uint32 MedalEarned = 0;
        //    std::list<PlayerEntry*> Players;
        //};

		struct ChallengeModeGroup
		{
			uint32 AttemptId = 0;
			uint32 MapId = 0;
			uint32 InstanceRealmAddress = 0;
			uint64 GuildId = 0;
			uint32 CompletionTime = 0;
			uint32 CompletionDate = 0;
			uint32 MedalEarned = 0;
			std::list<PlayerEntry*> Players;
		};

        struct ChallengeModeLeader
        {
            std::list<ChallengeModeGroup*> Groups;
            time_t LastUpdated = time_t(NULL);
        };

        class ChallengeModeStart final : public WorldPackets::ServerPacket
        {
            public:
                ChallengeModeStart() : ServerPacket(SMSG_CHALLENGE_MODE_START, 4) { }

                WorldPacket const* Write() override;

                uint32 MapId = 0;
        };

        class ChallengeModeReset final : public WorldPackets::ServerPacket
        {
            public:
                ChallengeModeReset() : ServerPacket(SMSG_CHALLENGE_MODE_RESET, 4) { }

                WorldPacket const* Write() override;

                uint32 MapId = 0;
        };

        class ChallengeModeRequestLeaders final : public ClientPacket
        {
            public:
                ChallengeModeRequestLeaders(WorldPacket&& packet) : ClientPacket(CMSG_CHALLENGE_MODE_REQUEST_LEADERS, std::move(packet)) { }

                void Read() override;

                uint32 MapID = 0;
                uint32 LastRealmUpdate = 0;
                uint32 LastGuildUpdate = 0;
        };

        class ChallengeModeRequestLeadersResult final : public WorldPackets::ServerPacket
        {
            public:
                ChallengeModeRequestLeadersResult() : ServerPacket(SMSG_CHALLENGE_MODE_REQUEST_LEADERS_RESULT) { }

                WorldPacket const* Write() override;

                uint32 MapId = 0;
                ChallengeModeLeader* RealmLeaderboards;
                ChallengeModeLeader* GuildLeaderboards;
        };

		class ChallengeModeRequestMapStats final : public ClientPacket
		{
			public:
				ChallengeModeRequestMapStats(WorldPacket&& packet) : ClientPacket(CMSG_CHALLENGE_MODE_REQUEST_MAP_STATS, std::move(packet)) { }

                void Read() override { }
		};

		struct ChallengeModeMapStats
		{
            uint32 MapId = 0;
			uint32 BestCompletionTime = 0;
			uint32 LastCompletionTime = 0;
			uint32 Medal = 0;
            uint32 BestCompletionDate = 0;
            uint32 LastCompletionDate = 0;
			std::list<uint16> Specs;
		};

		class ChallengeModeRequestMapStatsResult final : public WorldPackets::ServerPacket
		{
			public:
				ChallengeModeRequestMapStatsResult() : ServerPacket(SMSG_CHALLENGE_MODE_ALL_MAP_STATS) { }

				WorldPacket const* Write() override;

				std::map<uint32, ChallengeModeMapStats> AllMapStats;
		};

		class ChallengeModeMapStatsUpdate final : public WorldPackets::ServerPacket
		{
			public:
                ChallengeModeMapStatsUpdate() : ServerPacket(SMSG_CHALLENGE_MODE_MAP_STATS_UPDATE) { }

				WorldPacket const* Write() override;

				ChallengeModeMapStats MapStats;
		};

		class ChallengeModeNewPlayerRecord final : public WorldPackets::ServerPacket
		{
			public:
				ChallengeModeNewPlayerRecord(ChallengeModeGroup const* group) : ServerPacket(SMSG_CHALLENGE_MODE_NEW_PLAYER_RECORD)
				{
                    MapID = group->MapId;
                    Time = group->CompletionTime;
                    Medal = group->MedalEarned;
				}

				WorldPacket const* Write() override;

				uint32 MapID = 0;
				uint32 Time = 0;
				uint32 Medal = 0;
		};

        class ResetChallengeMode final : public ClientPacket
        {
            public:
                ResetChallengeMode(WorldPacket&& packet) : ClientPacket(CMSG_RESET_CHALLENGE_MODE, std::move(packet)) { }

                void Read() override { }
        };

        struct CurrencyReward
        {
            uint32 CurrencyId = 0;
            uint32 Quantity = 0;
        };

        struct ItemReward
        {
            WorldPackets::Item::ItemInstance Item;
            uint32 Quantity = 0;
        };

        class ChallengeModeComplete final : public WorldPackets::ServerPacket
        {
            public:
                ChallengeModeComplete() : ServerPacket(SMSG_CHALLENGE_MODE_COMPLETE) { }

                WorldPacket const* Write() override;

                uint32 MoneyReward = 0;
                std::list<ItemReward> ItemRewards;
                std::list<CurrencyReward> CurrencyRewards;
                uint32 Time = 0;
                uint32 MapID = 0;
                uint32 Medal = 0;
        };

        class GetChallengeModeRewards final : public ClientPacket
        {
            public:
                GetChallengeModeRewards(WorldPacket&& packet) : ClientPacket(CMSG_GET_CHALLENGE_MODE_REWARDS, std::move(packet)) { }

                void Read() override { }
        };

        struct ChallengeModeReward
        {
            uint32 MapId = 0;
            std::list<ItemReward> ItemRewards;
            std::list<CurrencyReward> CurrencyRewards;
            uint32 MoneyReward = 0;
        };

        class ChallengeModeRewards final : public ServerPacket
        {
            public:
                ChallengeModeRewards() : ServerPacket(SMSG_CHALLEGE_MODE_REWARDS) { }

                WorldPacket const* Write() override;

                std::list<ChallengeModeReward> CMRewards;
                std::list<ItemReward> ItemRewards;
        };
    }
}
#endif // ChallengeModePackets_h__
