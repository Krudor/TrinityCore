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

#ifndef __CHALLENGEMODE_H
#define __CHALLENGEMODE_H

#include "Common.h"
#include "DB2Structure.h"
#include "Map.h"
#include "EventProcessor.h"
#include "math.h"
//#include "InstanceScript.h"

namespace WorldPackets
{
    namespace Misc
    {
        class StartElapsedTimers;
    }
}

enum ChallengeModeSpells
{
    SPELL_LESSER_INVISIBILITY   = 3680,
    SPELL_INVISIBILITY          = 11392,
};

enum ChallengeModeMedals
{
    CM_MEDAL_NONE       = 0,
    CM_MEDAL_BRONZE     = 1,
    CM_MEDAL_SILVER     = 2,
    CM_MEDAL_GOLD       = 3
};

// Extent of Elapsed Timer's are currently unknown (at least to the writer of this functionality). Currently treating this with having a map contraint.
class ElapsedTimers
{
    public:
        ElapsedTimers::ElapsedTimers(Map* map);

        int32 CreateElapsedTimer();
        time_t GetElapsedTimer(uint32 id);
        std::map<int32, time_t> const* GetElapsedTimers() { return Timers; }

        void Start(int32 id);
        uint32 Stop(int32 id, bool keepTimer = false);
        void RemoveTimer(int32 id);
        
        void BuildElapsedTimers(WorldPackets::Misc::StartElapsedTimers* startElapsedTimers);

    private:
        Map* _map;
        std::map<int32, time_t>* Timers;
        int32 CreateId();
};

class TC_GAME_API ChallengeMode
{
    public:
        explicit ChallengeMode::ChallengeMode(Map* map);

        virtual ~ChallengeMode() { }

        Map* instance;
        MapChallengeModeEntry const* data;

    public:
        void Start();
        void Start(uint32 countdownSeconds);
        void Reset(Player* source);
        void Complete();

        void OnPlayerEnter(Player* player) const;
		void OnPlayerExit(Player* player) const;
        void HandleAreaTrigger(Player* source, uint32 trigger, bool entered);
        void OnPlayerOutOfBounds(Player* player) const;
        bool IsCompleted() { return _state == 3; }
        bool InProgress() { return _state == 1; }
        uint32 GetState() { return _state; }
        uint32 GetMedal(uint32 timeInMilliseconds)
        {
            ChallengeModeMedals medal = CM_MEDAL_NONE;

            if (data)
            {
                if (timeInMilliseconds > data->BronzeTime * IN_MILLISECONDS)
                    medal = CM_MEDAL_BRONZE;
                if (timeInMilliseconds > data->SilverTime * IN_MILLISECONDS)
                    medal = CM_MEDAL_SILVER;
                if (timeInMilliseconds > data->GoldTime * IN_MILLISECONDS)
                    medal = CM_MEDAL_GOLD;
            }

            return uint32(medal);
        }
        //uint32 GetTime() { return GetMSTimeDiffToNow(timeStart); }
        //uint32 GetTime() { return GetMSTimeDiffToNow(_elapsedTimers->GetElapsedTimer(elapsedTimerId)); }
        
        void Update(uint32 diff);

    private:
        bool CanStart() { return _state == 0; }

        ElapsedTimers* _elapsedTimers;
        int32 elapsedTimerId;
        bool _startedByTimer;
        uint32 _state;
        EventProcessor _events;
        //uint32 timeStart;
        uint32 timeComplete;
        time_t lastReset;
        uint32 outOfBoundsAreatriggerId;

        void SaveAttemptToDb();
};

class ChallengeModeStartEvent : public BasicEvent
{
    public:
        ChallengeModeStartEvent(ChallengeMode* challengeMode) : _ChallengeMode(challengeMode) { }
        virtual ~ChallengeModeStartEvent() { }

        virtual bool Execute(uint64 e_time, uint32 p_time) override;
        virtual void Abort(uint64 e_time) override {}

    private:
        ChallengeMode* _ChallengeMode;
};

#endif
