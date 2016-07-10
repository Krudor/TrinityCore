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
//#include "InstanceScript.h"

namespace WorldPackets
{
    namespace Misc
    {
        class StartElapsedTimers;
    }
}

struct ChallengeModeData;

enum ChallengeModeMedals
{
    CM_MEDAL_NONE   = 0,
    CM_MEDAL_BRONZE = 1,
    CM_MEDAL_SILVER = 2,
    CM_MEDAL_GOLD   = 3,
    CM_MEDAL_MAX    = 4,
};

enum ChallengeModeStatus
{
    CM_STATUS_NOT_STARTED,
    CM_STATUS_PREPARING,
    CM_STATUS_IN_PROGRESS,
    CM_STATUS_DONE
};

enum ChallengeModeSpells
{
    SPELL_LESSER_INVISIBILITY   = 3680,
    SPELL_INVISIBILITY          = 11392,
};

enum CMSpellCategories
{
    SPELLCATEGORY_CHALLENGERS_PATH  = 1407
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

        Map* _instance;
        MapChallengeModeEntry const* _entry;
        ChallengeModeData const* _data;

    public:
        void Start();
        void Start(uint32 countdownSeconds);
        void Reset(Player* source);
        void Complete();

        void OnPlayerEnter(Player* player) const;
		void OnPlayerExit(Player* player) const;
        void HandleAreaTrigger(Player* source, uint32 trigger, bool entered);
        void OnPlayerOutOfBounds(Player* player) const;
        bool IsCompleted() const { return _status == CM_STATUS_DONE; }
        bool InProgress() const { return _status == CM_STATUS_IN_PROGRESS; }
        bool CanStart() const { return _status == CM_STATUS_NOT_STARTED; }
        uint32 GetState() const { return _status; }

        uint32 GetMedal(uint32 timeInMilliseconds) const;
        //uint32 GetTime() { return GetMSTimeDiffToNow(timeStart); }
        //uint32 GetTime() { return GetMSTimeDiffToNow(_elapsedTimers->GetElapsedTimer(elapsedTimerId)); }
        
        void Update(uint32 diff);

private:
        ElapsedTimers* _elapsedTimers;
        int32 elapsedTimerId;
        bool _startedByTimer;
        ChallengeModeStatus _status;
        EventProcessor _events;
        //uint32 timeStart;
        uint32 timeComplete;
        time_t lastReset;

        // returns attemptId
        uint32 SaveAttemptToDb() const;
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
