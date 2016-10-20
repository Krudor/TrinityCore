/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
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

/*
    ___Todo___/____Information___
    Corruption spawn visual effect does not trigger for some reason when it is casted by a creature, only from a player. Core related or something wrong scriptwise?
    Hideous Amalgamation's Nuclear Blast range to trigger the opening of the plate may have to be adjusted, no blizzlike values.
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SharedDefines.h"
#include "CreatureTextMgr.h"
#include "Vehicle.h"
#include "dragon_soul.h"
#include "PassiveAI.h"
#include "SpellAuras.h"
#include "SpellScript.h"

enum Spells
{
    SPELL_REDUCE_CRIT_CHANCE            = 64481,
    SPELL_ZERO_MANA_ZERO_REGEN_90_MAX   = 109121,
    SPELL_DESTROY_ALL_LIFE              = 109048,
    SPELL_ANTI_EXPLOIT_LOS_CHECK        = 109983,
    SPELL_CHECK_FOR_PLAYERS             = 109035,
    SPELL_DEATHWING_TALK                = 106300, //Using this spell to send our special opcode will complicate things for no reason.
    SPELL_DEATHWING_ROAR                = 106302, //Using this spell to send our special opcode will complicate things for no reason.
    SPELL_SPAWNING_CONTROLLER           = 105003, //No info
    SPELL_SUMMON_SLIME                  = 105537,
    SPELL_SUMMON_CORRUPTION             = 105535,
    SPELL_SUMMON_AMALGAMATION           = 105000,
    SPELL_ROLL_CONTROL_JUMP             = 105777,
    SPELL_BLOOD_OF_DEATHWING            = 106201,
    SPELL_ACTIVATE_SPAWNER              = 100341,
    SPELL_GRASPING_TENDRILS             = 105510,
    SPELL_GRASPING_TENDRILS_AURA        = 105563,
    SPELL_PLAY_MOVIE_DEATHWING_3        = 104574, //End of encounter, play movie, trigger teleport to maelstrom upon receiving opcode CMSG_COMPLETE_MOVIE from the client.
    SPELL_ROLL_CONTROL                  = 105036,
    SPELL_ROLL_TICK                     = 104621,
    SPELL_ROLL_CONTROL_KICK_OFF         = 105740,
    SPELL_FIERY_GRIP                    = 105490,
    SPELL_SEARING_PLASMA                = 109379,
    SPELL_SEARING_PLASMA_AURA           = 105479,
    SPELL_BREACH_ARMOR_LEFT             = 105363,
    SPELL_BREACH_ARMOR_RIGHT            = 105385,
    SPELL_PLATE_FLY_OFF_LEFT            = 105366,
    SPELL_PLATE_FLY_OFF_RIGHT           = 105384,
    SPELL_SEAL_ARMOR_BREACH_LEFT        = 105847,
    SPELL_SEAL_ARMOR_BREACH_RIGHT       = 105848,
    SPELL_NUCLEAR_BLAST_SCRIPT_EFFECT   = 105846,
    SPELL_NUCLEAR_BLAST                 = 105845,
    SPELL_BURST                         = 105219,
    SPELL_RESIDUE                       = 105223,
    SPELL_ENERGIZE                      = 109083,
    SPELL_ABSORBED_BLOOD                = 105248,
    SPELL_ABSORB_BLOOD                  = 105244,
    SPELL_ABSORB_BLOOD_BAR              = 109329,
    SPELL_SUPERHEATED_NUCLEUS           = 105834,
};

enum Talk
{
    TALK_TAUNT,
    TALK_ROLL_LEFT_WARNING,
    TALK_ROLL_RIGHT_WARNING,
    TALK_ROLL_LEFT,
    TALK_ROLL_RIGHT,
    TALK_LEVEL_OUT,

    SOUND_DEATHWING_ROAR_1 = 26347,
    SOUND_DEATHWING_ROAR_2 = 26348,
    SOUND_DEATHWING_ROAR_3 = 26349,

    TALK_SWAYZE_DEATHWING_1 = 7,
    TALK_SWAYZE_DEATHWING_2 = 8,

    EMOTE_AMALG_NUCLEAR_BLAST = 0,
    EMOTE_AMALG_NUCLEAR_BLAST_FAIL,
    EMOTE_AMALG_NO_RESIDUE,
};

enum Events
{
    EVENT_NONE,
    EVENT_TAUNT,
    EVENT_TAUNT_MAP_EVENT,
    EVENT_BERSERK,
    EVENT_ROLL_CONTROL,
    EVENT_ROLL_LEFT,
    EVENT_ROLL_RIGHT,
    EVENT_ROLL_KICK_OFF_UNITS,
    EVENT_SUMMON_SLIME,
    EVENT_SUMMON_AMALGAMATION,
    EVENT_SEARING_PLASMA,
    EVENT_FIERY_GRIP,
    EVENT_TENDON_SET_ATTACKABLE,
    EVENT_CAN_ATTACK,
    EVENT_CAN_ANIMATE,
    EVENT_DELAYED_ROAR,
    EVENT_AMALGAMATION_DISAPPEAR,

    EVENT_GROUP_DELAYABLE,
};

enum Actions
{
    ACTION_NONE,
    ACTION_TAUNT,
    ACTION_ROLL_LEFT,
    ACTION_ROLL_RIGHT,
    ACTION_PLATE_OFF_LEFT,
    ACTION_PLATE_OFF_RIGHT,
    ACTION_ROLL_CANCEL,
    ACTION_CORRUPTION_SPAWN,
    ACTION_ACTIVATE_SPAWNER,
    ACTION_DEACTIVATE_SPAWNER,
    ACTION_FIERY_GRIP,
    ACTION_ROLL_OFF,
};

enum FightData
{
    DATA_MAYBE_HELL_GET_DIZZY = 1,
    GUID_LAST_SPAWNER = 1,

    ROLL_STATUS_NONE = 1,
    ROLL_STATUS_LEFT,
    ROLL_STATUS_RIGHT,
};

enum CreatureGroups
{
    CREATURE_GROUP_BASE,
    CREATURE_GROUP_PLATE_ONE,
    CREATURE_GROUP_PLATE_TWO,
};

enum Phases
{
    PHASE_BLOOD = 1,
    PHASE_CLOT,
};

enum Points
{
    POINT_SPAWNER_HOLE = 1,
    POINT_THROWN_OFF = 2,
};

enum Seats
{
    SEAT_0 = 0
};

enum SpineData
{
    DATA_TENDON_BROTHER
};

class StartAttackEvent : public BasicEvent
{
    public:
        StartAttackEvent(Creature* owner)
            : _owner(owner) { }

        bool Execute(uint64 /*time*/, uint32 /*diff*/)
        {
			if (!_owner->HasAuraType(SPELL_AURA_MOD_STUN))
				_owner->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);

            if (_owner->IsAIEnabled)
                if (Unit* target = _owner->SelectNearestHostileUnitInAggroRange())
                    _owner->AI()->AttackStart(target);
                else if (Player* player = _owner->SelectNearestPlayer(200.0f)) // Extend search
                    _owner->AI()->AttackStart(player);
                else
                    _owner->AI()->EnterEvadeMode();
            return true;
        }

    private:
        Creature* _owner;
};

Position const SwayzeCyaPoint = { -13851.03f, -13720.8f, -1944.319f };
Position const KaanuCyaPoint = { -13850.95f, -13731.55f -1944.319f };

class DelayedSpineEvent : public BasicEvent
{
    public:
        DelayedSpineEvent(Creature* owner) : _owner(owner) {}

        bool Execute(uint64, uint32) override
        {
            if (Vehicle* vehicle = _owner->GetVehicleKit())
            {
                if (Unit* passenger = vehicle->GetPassenger(SEAT_0))
                {
                    passenger->ExitVehicle();
                    if (passenger->GetEntry() == NPC_SKY_CAPTAIN_SWAYZE)
                        passenger->GetMotionMaster()->MoveJump(SwayzeCyaPoint, 20.0f, 20.0f);
                    else
                        passenger->GetMotionMaster()->MoveJump(KaanuCyaPoint, 20.0f, 20.0f);

                    passenger->ToCreature()->DespawnOrUnsummon(10 * IN_MILLISECONDS);
                }
            }

            return true;
        }

    private:
        Creature* _owner;
};

// http://www.wowhead.com/npc=53879/deathwing
class boss_spine_of_deathwing : public CreatureScript
{
    public:
        boss_spine_of_deathwing() : CreatureScript("boss_spine_of_deathwing") { }

        struct boss_spine_of_deathwingAI : public BossAI
        {
            boss_spine_of_deathwingAI(Creature* creature) : BossAI(creature, DATA_SPINE_OF_DEATHWING)
            {
                SetCombatMovement(false);
            }

            void InitializeAI() override
            {
                if (!me->isDead())
                    JustRespawned();
            }

            void JustRespawned() override
            {
				_Reset();
				_lastSpawner = ObjectGuid::Empty;
				_platesOff = _rollTick = 0;
				_deathwingDizzy = _started = false;
				_canAnimate = true;
				events.Reset();
				_rollSequence.clear();
				_rollStatus = ROLL_STATUS_NONE;

				ScriptedAI::JustRespawned();
                me->SummonCreatureGroup(CREATURE_GROUP_BASE);
            }

            void KilledUnit(Unit* who) override
            {
                if (Player* player = who->ToPlayer())
                    player->UpdateCriteria(CRITERIA_TYPE_BE_SPELL_TARGET, 94644); // Triggers http://www.wowhead.com/achievement=5518/stood-in-the-fire
            }

            void MoveInLineOfSight(Unit* who) override
            {
                if (me->CanCreatureAttack(who) && me->GetDistance2d(who) < me->GetCombatReach() && !me->IsInCombat())
                    DoZoneInCombat();

                ScriptedAI::MoveInLineOfSight(who);
            }

            void SpellHitTarget(Unit* target, SpellInfo const* spell) override
            {
                switch(spell->Id)
                {
                    case SPELL_ROLL_TICK:
                        if (++_rollTick > 5)
                        {
                            if (_rollStatus == ROLL_STATUS_LEFT)
                            {
                                Talk(TALK_ROLL_LEFT);
                                _RegisterRollSequence(ACTION_ROLL_LEFT);
                                _SendMapObjEvent(ANIM_CUSTOM_SPELL_04, CL_GUID_DEATHWING);
                            }
                            else if (_rollStatus == ROLL_STATUS_RIGHT)
                            {
                                Talk(TALK_ROLL_RIGHT);
                                _RegisterRollSequence(ACTION_ROLL_RIGHT);
                                _SendMapObjEvent(ANIM_CUSTOM_SPELL_05, CL_GUID_DEATHWING);
                            }
                        }
                        break;
                }
            }

            void SpellHit(Unit* unit, SpellInfo const* spell) override
            {
                switch (spell->Id)
                {
                    case SPELL_PLATE_FLY_OFF_LEFT:
                        DoAction(ACTION_PLATE_OFF_LEFT);
                        break;
                    case SPELL_PLATE_FLY_OFF_RIGHT:
                        DoAction(ACTION_PLATE_OFF_RIGHT);
                        break;
                    default:
                        break;
                }

                if (spell->Id != SPELL_PLATE_FLY_OFF_LEFT || spell->Id != SPELL_PLATE_FLY_OFF_RIGHT)
                    return;

                events.ScheduleEvent(EVENT_DELAYED_ROAR, 1 * IN_MILLISECONDS);

                if (++_platesOff == 1)
                    me->SummonCreatureGroup(CREATURE_GROUP_PLATE_ONE);
                else if (_platesOff == 2)
                    me->SummonCreatureGroup(CREATURE_GROUP_PLATE_TWO);
                else
                    me->Kill(me); //Extra layer of fail-safe.
            }

            void EnterCombat(Unit* who) override
            {
                _started = true;
                _EnterCombat();
                summons.DoZoneInCombat();
                instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);

                events.ScheduleEvent(EVENT_TAUNT_MAP_EVENT, urand(35, 45) * IN_MILLISECONDS, EVENT_GROUP_DELAYABLE);
                events.ScheduleEvent(EVENT_SUMMON_SLIME, 9 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_ROLL_CONTROL, 16 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_BERSERK, 30 * MINUTE * IN_MILLISECONDS);

                if (Creature* corruption = me->FindNearestCreature(NPC_CORRUPTION_3, 30.0f))
                    if (Unit* swayze = corruption->GetVehicleKit()->GetPassenger(SEAT_0))
                        if (swayze->IsAIEnabled)
                        {
                            swayze->ToCreature()->AI()->Talk(TALK_SWAYZE_DEATHWING_1);
                            corruption->m_Events.AddEvent(new DelayedSpineEvent(corruption), corruption->m_Events.CalculateTime(7 * IN_MILLISECONDS));
                        }
                if (Creature* corruption = me->FindNearestCreature(NPC_CORRUPTION_2, 30.0f))
                if (Unit* swayze = corruption->GetVehicleKit()->GetPassenger(SEAT_0))
                if (swayze->IsAIEnabled)
                    corruption->m_Events.AddEvent(new DelayedSpineEvent(corruption), corruption->m_Events.CalculateTime(8 * IN_MILLISECONDS));
            }

            void JustDied(Unit* /*killer*/) override
            {
                _JustDied();
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);

                DoCastAOE(SPELL_PLAY_MOVIE_DEATHWING_3, true);
            }

            void DoAction(int32 action) override
            {
                switch (action)
                {
                    case ACTION_TAUNT:
                        if (_canAnimate) //Don't send taunt animation during a roll or damaged sequence
                            _SendMapObjEvent(ANIM_EMOTE_TALK, CL_GUID_DEATHWING);
                        events.ScheduleEvent(EVENT_TAUNT, 1.5 * IN_MILLISECONDS, EVENT_GROUP_DELAYABLE);
                        break;
                    case ACTION_ROLL_LEFT:
                        if (_rollStatus == ROLL_STATUS_LEFT)
                            return;

                        _rollTick = 0;
                        Talk(TALK_ROLL_LEFT_WARNING);
                        _rollStatus = ROLL_STATUS_LEFT;
                        break;
                    case ACTION_ROLL_RIGHT:
                        if (_rollStatus == ROLL_STATUS_RIGHT)
                            return;

                        _rollTick = 0;
                        Talk(TALK_ROLL_RIGHT_WARNING);
                        _rollStatus = ROLL_STATUS_RIGHT;
                        break;
                    case ACTION_ROLL_CANCEL:
                        if (_rollStatus == ROLL_STATUS_NONE)
                            return;

                        _rollTick = 0;
                        Talk(TALK_LEVEL_OUT);

                        _rollStatus = ROLL_STATUS_NONE;
                        break;
                    case ACTION_PLATE_OFF_RIGHT:
                        if (!_canAnimate)
                            return;

                        _canAnimate = false;
                        _SendMapObjEvent(ANIM_CUSTOM_SPELL_01, CL_GUID_DEATHWING);
                        events.ScheduleEvent(EVENT_CAN_ANIMATE, 6 * IN_MILLISECONDS);
                        break;
                    case ACTION_PLATE_OFF_LEFT:
                        if (!_canAnimate)
                            return;

                        _canAnimate = false;
                        _SendMapObjEvent(ANIM_CUSTOM_SPELL_02, CL_GUID_DEATHWING);
                        events.ScheduleEvent(EVENT_CAN_ANIMATE, 6 * IN_MILLISECONDS);
                        break;
                    default:
                        break;
                }
            }

            void EnterEvadeMode(EvadeReason reason) override
            {
                _EnterEvadeMode();
                summons.DespawnAll();

                instance->SetData(DATA_SPINE_OF_DEATHWING, FAIL);
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_DEGRADATION);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_BLOOD_CORRUPTION_DEATH);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_BLOOD_CORRUPTION_EARTH);
                if (GameObject* plate = ObjectAccessor::GetGameObject(*me, instance->GetGuidData(GO_DEATHWING_BACK_PLATE_1)))
                {
                    plate->ResetDoorOrButton();
                    plate->SetAnimKitId(1697, true);
                }
                if (GameObject* plate = ObjectAccessor::GetGameObject(*me, instance->GetGuidData(GO_DEATHWING_BACK_PLATE_2)))
                {
                    plate->ResetDoorOrButton();
                    plate->SetAnimKitId(1697, true);
                }
                if (GameObject* plate = ObjectAccessor::GetGameObject(*me, instance->GetGuidData(GO_DEATHWING_BACK_PLATE_3)))
                {
                    plate->ResetDoorOrButton();
                    plate->SetAnimKitId(1697, true);
                }

                Map::PlayerList const &players = me->GetMap()->GetPlayers();
                for (Map::PlayerList::const_iterator i = players.begin(); i != players.end(); ++i)
                if (Player* _player = i->GetSource())
                    _player->TeleportTo(me->GetMapId(), 13456.6f, -12133.8f, 151.168f, 1.643641f);

                _DespawnAtEvade();
            }

            void JustSummoned(Creature* summon) override
            {
                summons.Summon(summon);
            }

            void SummonedCreatureDies(Creature* summon, Unit* /*killer*/) override
            {
                switch (summon->GetEntry())
                {
                    case NPC_CORRUPTION:
                    case NPC_CORRUPTION_2:
                    case NPC_CORRUPTION_3:
                    {
                        if (Creature* spawner = summon->FindNearestCreature(NPC_SPAWNER, 5.0f))
                        {
                            _lastSpawner = spawner->GetGUID();
                            if (UnitAI* AI = spawner->GetAI())
                                AI->DoAction(ACTION_ACTIVATE_SPAWNER);
                        }

                        std::list<Creature*> corruptions;
                        GetCreatureListWithEntryInGrid(corruptions, me, NPC_CORRUPTION, 100.0f);
                        GetCreatureListWithEntryInGrid(corruptions, me, NPC_CORRUPTION_2, 100.0f);
                        GetCreatureListWithEntryInGrid(corruptions, me, NPC_CORRUPTION_3, 100.0f);
                        corruptions.remove(summon);
                        corruptions.remove_if([](Creature* target)
                        {
                            return !target->IsAlive();
                        });

                        if (corruptions.empty())
                            DoCastAOE(SPELL_SUMMON_CORRUPTION, true);
                    }
                        break;
                    default:
                        break;
                }

                summon->DespawnOrUnsummon(4 * IN_MILLISECONDS);
            }

            void SummonedCreatureDespawn(Creature* summon)
            {
                summons.Despawn(summon);
            }

		    ObjectGuid GetGUID(int32 data) const override
            {
                if (data == GUID_LAST_SPAWNER)
                    return _lastSpawner;

                return ObjectGuid::Empty;
            }

            uint32 GetData(uint32 data) const override
            {
                if (data == DATA_MAYBE_HELL_GET_DIZZY)
                    return _deathwingDizzy ? (uint32)true : (uint32)false;

                return 0;
            }

            void UpdateAI(uint32 diff) override
            {
                if (_started)
                    if (!me->IsInCombat() || me->getThreatManager().isThreatListEmpty()) //Should not these 2 yield the same result?
                    {
                        EnterEvadeMode(EVADE_REASON_NO_HOSTILES);
                        return;
                    }

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_TAUNT:
                            Talk(TALK_TAUNT);
                            events.ScheduleEvent(EVENT_TAUNT_MAP_EVENT, urand(35, 45) * IN_MILLISECONDS, EVENT_GROUP_DELAYABLE);
                            break;
                        case EVENT_CAN_ANIMATE:
                            _canAnimate = true;
                            break;
                        case EVENT_TAUNT_MAP_EVENT:
                            DoAction(ACTION_TAUNT);
                            break;
                        case EVENT_DELAYED_ROAR:
                            sCreatureTextMgr->SendSound(me, RAND(SOUND_DEATHWING_ROAR_1, SOUND_DEATHWING_ROAR_2, SOUND_DEATHWING_ROAR_3), CHAT_MSG_MONSTER_YELL, 0, TEXT_RANGE_NORMAL, TEAM_OTHER, false);
                            break;
                        case EVENT_SUMMON_SLIME:
                            DoCastAOE(SPELL_SUMMON_SLIME, true);
                            if (me->GetHealthPct() < 33.0f)
                                events.ScheduleEvent(EVENT_SUMMON_SLIME, 5 * IN_MILLISECONDS);
                            else if (me->GetHealthPct() < 66.0f)
                                events.ScheduleEvent(EVENT_SUMMON_SLIME, 7 * IN_MILLISECONDS);
                            else
                                events.ScheduleEvent(EVENT_SUMMON_SLIME, 9 * IN_MILLISECONDS);
                            break;
                        case EVENT_ROLL_CONTROL:
                            DoCastAOE(SPELL_ROLL_CONTROL, true);
                            break;
                        case EVENT_BERSERK:
                            DoCastAOE(SPELL_DESTROY_ALL_LIFE, true);
                            break;
                        case EVENT_ROLL_LEFT:
                            _canAnimate = false;
                            events.ScheduleEvent(EVENT_CAN_ANIMATE, 10 * IN_MILLISECONDS);
                            Talk(TALK_ROLL_LEFT);
                            _RegisterRollSequence(ACTION_ROLL_LEFT);
                            _SendMapObjEvent(ANIM_CUSTOM_SPELL_04, CL_GUID_DEATHWING);
                            break;
                        case EVENT_ROLL_RIGHT:
                            _canAnimate = false;
                            events.ScheduleEvent(EVENT_CAN_ANIMATE, 10 * IN_MILLISECONDS);
                            Talk(TALK_ROLL_RIGHT);
                            _RegisterRollSequence(ACTION_ROLL_RIGHT);
                            _SendMapObjEvent(ANIM_CUSTOM_SPELL_05, CL_GUID_DEATHWING);
                            break;
                        case EVENT_ROLL_KICK_OFF_UNITS:
                        {
                            DoCastAOE(SPELL_ROLL_CONTROL_KICK_OFF, true);
                            EntryCheckPredicate pred(NPC_HIDEOUS_AMALGAMATION);
                            summons.DoAction(ACTION_ROLL_OFF, pred);
                        }
                            break;
                        default:
                            break;
                    }
                }
            }

            void AttackStart(Unit* victim) override
            {
                ScriptedAI::AttackStartNoMove(victim);
            }

        private:
            uint8 _rollTick;
            bool _started;
            FightData _rollStatus;
            ObjectGuid _lastSpawner;
            uint8 _platesOff;
            std::deque<Actions> _rollSequence;
            bool _deathwingDizzy, _canAnimate;

            void _RegisterRollSequence(Actions rollSeq)
            {
                _canAnimate = false;
                _rollStatus = ROLL_STATUS_NONE;
                me->RemoveAurasDueToSpell(SPELL_ROLL_CONTROL);
                events.ScheduleEvent(EVENT_ROLL_KICK_OFF_UNITS, 2 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_ROLL_CONTROL, 10 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_CAN_ANIMATE, 10 * IN_MILLISECONDS);
                if (events.GetTimeUntilEvent(EVENT_TAUNT_MAP_EVENT) < 10 * IN_MILLISECONDS)
                    events.DelayEvents(10 * IN_MILLISECONDS, EVENT_GROUP_DELAYABLE);

                if (_deathwingDizzy)
                    return;

                _rollSequence.push_back(rollSeq);
                if (_rollSequence.size() > 4)
                    _rollSequence.pop_front();

                if (_rollSequence.size() >= 4)
                if (_rollSequence.at(0) == ACTION_ROLL_LEFT)
                if (_rollSequence.at(1) == ACTION_ROLL_RIGHT)
                if (_rollSequence.at(2) == ACTION_ROLL_LEFT)
                if (_rollSequence.at(3) == ACTION_ROLL_RIGHT)
                   _deathwingDizzy = true;
            }

            void _SendMapObjEvent(uint32 animId, uint32 objGuid, uint8 length = 6, uint32 unkFlag = 0)
            {
                WorldPacket* packet = new WorldPacket(SMSG_MAP_OBJ_EVENTS, 14);
                *packet << uint32(objGuid);
                *packet << uint32(length);
                *packet << uint8(1);
                *packet << uint8(unkFlag);
                *packet << uint32(animId);
                me->GetMap()->SendToPlayers(packet);
            }
        };
	
	    CreatureAI* GetAI(Creature* creature) const override
        {
            return new boss_spine_of_deathwingAI(creature);
        }
};

// http://www.wowhead.com/npc=56341
// http://www.wowhead.com/npc=56575
class npc_sod_burning_tendon : public CreatureScript
{
    public:
		npc_sod_burning_tendon() : CreatureScript("npc_sod_burning_tendon") {}

		struct npc_sod_burning_tendonAI : public PassiveAI
        {
			npc_sod_burning_tendonAI(Creature* creature) : PassiveAI(creature) { }

            void SpellHit(Unit* /*caster*/, SpellInfo const* spell) override
            {
                if (spell->Id != SPELL_NUCLEAR_BLAST_SCRIPT_EFFECT)
                    return;

                switch (me->GetEntry())
                {
                    case NPC_BURNING_TENDON_1:
                        DoCastAOE(SPELL_BREACH_ARMOR_LEFT, true);
                        DoCastAOE(SPELL_SEAL_ARMOR_BREACH_LEFT);
                        break;
                    case NPC_BURNING_TENDON_2:
                        DoCastAOE(SPELL_BREACH_ARMOR_RIGHT, true);
                        DoCastAOE(SPELL_SEAL_ARMOR_BREACH_RIGHT);
                        break;
                }

                _scheduler.Schedule(Milliseconds(2500), [this](TaskContext context)
                {
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                    if (InstanceScript* instance = me->GetInstanceScript())
                        instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
                });
            }

            void JustDied(Unit* /*killer*/) override
            {
                switch (me->GetEntry())
                {
                    case NPC_BURNING_TENDON_1:
                        DoCastAOE(SPELL_PLATE_FLY_OFF_LEFT, true);
                        break;
                    case NPC_BURNING_TENDON_2:
                        DoCastAOE(SPELL_PLATE_FLY_OFF_RIGHT, true);
                        break;
                    default:
                        break;
                }
                
                if (InstanceScript* instance = me->GetInstanceScript())
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
                if (Creature* brother = ObjectAccessor::GetCreature(*me, _brother))
                    brother->DespawnOrUnsummon();
                me->DespawnOrUnsummon();
            }

            void SetGUID(ObjectGuid guid, int32 type) override
            {
                if (type != DATA_TENDON_BROTHER)
                    return;

                if (guid != ObjectGuid::Empty)
                    return;

                _brother = guid;
                if (Creature* brother = ObjectAccessor::GetCreature(*me, _brother))
                    if (brother->IsAIEnabled)
                        brother->AI()->SetGUID(me->GetGUID(), DATA_TENDON_BROTHER);
            }

            void SpellHitTarget(Unit* /*target*/, const SpellInfo* spell) override
            {
                switch (spell->Id)
                {
                    case SPELL_SEAL_ARMOR_BREACH_LEFT:
                    case SPELL_SEAL_ARMOR_BREACH_RIGHT:
                        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                        if (InstanceScript* instance = me->GetInstanceScript())
                            instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
                        break;
                    default:
                        break;
                }
            }

        private:
            ObjectGuid _brother;
            TaskScheduler _scheduler;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
			return new npc_sod_burning_tendonAI(creature);
        }
};

// http://www.wowhead.com/npc=53890
class npc_sod_hideous_amalgamation : public CreatureScript
{
    public:
		npc_sod_hideous_amalgamation() : CreatureScript("npc_sod_hideous_amalgamation") {}

		struct npc_sod_hideous_amalgamationAI : public ScriptedAI
        {
			npc_sod_hideous_amalgamationAI(Creature* creature) : ScriptedAI(creature) { }

            void IsSummonedBy(Unit* summoner) override
            {
                me->SetReactState(REACT_PASSIVE);
                me->AddAura(SPELL_ZERO_MANA_ZERO_REGEN_90_MAX, me);
                me->AddAura(SPELL_ABSORB_BLOOD_BAR, me);
                me->AddAura(SPELL_ABSORB_BLOOD, me);
                _scheduler.Schedule(Seconds(3), [this](TaskContext)
                {
                    me->SetReactState(REACT_AGGRESSIVE);
                    if (Unit* target = me->SelectNearestHostileUnitInAggroRange())
                        me->AI()->AttackStart(target);
                });
            }

            void MovementInform(uint32 movementType, uint32 pointId) override
            {
                if (movementType != EFFECT_MOTION_TYPE || pointId != EVENT_JUMP || !me->HasAura(SPELL_ROLL_CONTROL_JUMP))
                    return;

                me->Kill(me);
            }

            void DoAction(int32 action) override
            {
                if (action != ACTION_ROLL_OFF || me->isDead())
                    return;

                me->SetReactState(REACT_PASSIVE);
                me->AttackStop();
                DoCastAOE(SPELL_ROLL_CONTROL_JUMP, true);
                _scheduler.Schedule(Seconds(3), [this](TaskContext)
                {
                    me->SetVisible(false);
                });
            }

            void SpellHit(Unit* caster, const SpellInfo* spell) override
            {
                if (spell->Id != SPELL_ABSORBED_BLOOD || me->HasAura(SPELL_SUPERHEATED_NUCLEUS))
                    return;

                if (Aura* aura = me->GetAura(SPELL_ABSORBED_BLOOD))
                {
                    if (aura->GetStackAmount() >= 9)
                    {
                        me->RemoveAurasDueToSpell(SPELL_ABSORB_BLOOD);
                        DoCastAOE(SPELL_SUPERHEATED_NUCLEUS, true);
                    }
                }
                if (caster)
                    caster->CastSpell(me, SPELL_ENERGIZE, true);
            }

            void JustDied(Unit* killer) override
            {
                if (me != killer && me->GetMap()->GetDifficultyID() == DIFFICULTY_LFR)
                    Talk(EMOTE_AMALG_NO_RESIDUE);
                if (me->GetMap()->IsHeroic())
                    DoCastAOE(SPELL_DEGRADATION, true);

                _scheduler.Schedule(Seconds(3), [this](TaskContext)
                {
                    me->SetVisible(false);
                });
            }

            void DamageTaken(Unit* /*source*/, uint32& damage) override
            {
                if (damage < me->GetHealth())
                    return;

                if (me->HasAura(SPELL_SUPERHEATED_NUCLEUS))
                {
                    damage = me->GetHealth() - 1;
                    if (Spell* spell = me->GetCurrentSpell(CURRENT_GENERIC_SPELL))
                        if (spell->getState() == SPELL_STATE_CASTING)
                            if (SpellInfo const* spellInfo = spell->GetSpellInfo())
                                if (spellInfo->Id == SPELL_NUCLEAR_BLAST)
                                    return;

                    DoCastAOE(SPELL_NUCLEAR_BLAST);
                }
            }

            bool CanAIAttack(Unit const* target) const override
            {
                return !target->HasAura(SPELL_ROLL_CONTROL_JUMP);
            }

        private:
            TaskScheduler _scheduler;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
			return new npc_sod_hideous_amalgamationAI(creature);
        }
};

// http://www.wowhead.com/npc=53889
class npc_sod_corrupted_blood : public CreatureScript
{
public:
	npc_sod_corrupted_blood() : CreatureScript("npc_sod_corrupted_blood") {}

	struct npc_sod_corrupted_bloodAI : public ScriptedAI
    {
		npc_sod_corrupted_bloodAI(Creature* creature) : ScriptedAI(creature) {}

        void Reset() override
        {
            me->SetReactState(REACT_PASSIVE);
            me->AddAura(SPELL_REDUCE_CRIT_CHANCE, me);
            _scheduler.Schedule(Seconds(1), [this](TaskContext)
            {
                me->SetReactState(REACT_AGGRESSIVE);
                if (Unit* target = me->SelectNearestHostileUnitInAggroRange())
                    me->AI()->AttackStart(target);
            });
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type != POINT_MOTION_TYPE || id != POINT_SPAWNER_HOLE)
                return;

            me->RemoveAurasDueToSpell(SPELL_RESIDUE);
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_UNK_29);
        }

        void DamageTaken(Unit* /*attacker*/, uint32& damage) override
        {
            if (damage >= me->GetHealth())
            {
                damage = me->GetHealth() - 1;
                if (!me->HasAura(SPELL_RESIDUE))
                {
                    DoCastAOE(SPELL_RESIDUE, true);
                    me->RemoveNotOwnSingleTargetAuras();
                    DoCastAOE(SPELL_BURST, true);
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_UNK_29);

                    std::list<Creature*> spawners;
                    GetCreatureListWithEntryInGrid(spawners, me, NPC_SPAWNER, 200.0f);
                    spawners.remove_if(Trinity::UnitAuraCheck(false, SPELL_GRASPING_TENDRILS));

                    if (spawners.empty())
                        return;

                    spawners.sort(Trinity::ObjectDistanceOrderPred(me));
                    Creature* spawner = spawners.front();

                    me->GetMotionMaster()->Initialize();
                    me->GetMotionMaster()->MovePoint(POINT_SPAWNER_HOLE, spawner->GetPositionX(), spawner->GetPositionY(), spawner->GetPositionZ());
                    Movement::MoveSplineInit init(me);
                    init.MoveTo(spawner->GetPositionX(), spawner->GetPositionY(), spawner->GetPositionZ());
                    init.SetVelocity(0.1f);
                    init.Launch();
                }
            }
        }
        
    private:
        TaskScheduler _scheduler;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
		return new npc_sod_corrupted_bloodAI(creature);
    }
};

// http://www.wowhead.com/npc=53888
class npc_sod_spawner : public CreatureScript
{
    public:
		npc_sod_spawner() : CreatureScript("npc_sod_spawner") {}

		struct npc_sod_spawnerAI : public PassiveAI
        {
			npc_sod_spawnerAI(Creature* creature) : PassiveAI(creature), _summons(me) { }

            void EnterEvadeMode(EvadeReason reason) override
            {
                _summons.DespawnAll();
            }

            void JustSummoned(Creature* summon) override
            {
                _summons.Summon(summon);
            }

            void SummonedCreatureDespawn(Creature* summon) override
			{
                _summons.Despawn(summon);
			}

            void SpellHit(Unit* caster, SpellInfo const* spell) override
            {
                if (spell->Id != SPELL_ACTIVATE_SPAWNER)
                    return;

                DoCastAOE(SPELL_GRASPING_TENDRILS, true);
                _scheduler.Schedule(Milliseconds(3500), [this](TaskContext)
                {
                    DoCastAOE(SPELL_SUMMON_AMALGAMATION, true);
                });
            }

            void DoAction(int32 action)
            {
                switch (action)
                {
                    /*case ACTION_ACTIVATE_SPAWNER:
                        DoCastAOE(SPELL_ACTIVATE_SPAWNER, true);
                        DoCastAOE(SPELL_GRASPING_TENDRILS, true);
                        _events.ScheduleEvent(EVENT_SUMMON_AMALGAMATION, 3.5*IN_MILLISECONDS);
                        break;*/
                    case ACTION_DEACTIVATE_SPAWNER:
                        me->RemoveAurasDueToSpell(SPELL_GRASPING_TENDRILS);
                        break;
                    default:
                        break;
                }
            }

        private:
            TaskScheduler _scheduler;
            SummonList _summons;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
			return new npc_sod_spawnerAI(creature);
        }
};

// http://www.wowhead.com/npc=53891
// http://www.wowhead.com/npc=56161
// http://www.wowhead.com/npc=56162
class npc_sod_corruption : public CreatureScript
{
    public:
		npc_sod_corruption() : CreatureScript("npc_sod_corruption") {}

		struct npc_sod_corruptionAI : public ScriptedAI
        {
			npc_sod_corruptionAI(Creature* creature) : ScriptedAI(creature) { ScriptedAI::SetCombatMovement(false); }

            void AttackStart(Unit* victim) override {}

            void Reset() override
            {
                _healthPctBreak = 0;
            }

            void IsSummonedBy(Unit* /*who*/) override
            {
                me->HandleEmoteCommand(EMOTE_ONESHOT_EMERGE);
            }

            void DamageTaken(Unit* /*source*/, uint32& damage) override
            {
                if (int64(me->GetHealth()) - int64(damage) < int64(_healthPctBreak))
                    me->CastStop(SPELL_SEARING_PLASMA);
            }

            void EnterCombat(Unit* /*who*/) override
            {
                DoCastAOE(SPELL_SEARING_PLASMA);
                _events.ScheduleEvent(EVENT_FIERY_GRIP, 20 * IN_MILLISECONDS);
                _events.ScheduleEvent(EVENT_SEARING_PLASMA, urand(8, 12) * IN_MILLISECONDS);
            }

            void JustDied(Unit* who) override
            {
                DoCastAOE(SPELL_ACTIVATE_SPAWNER);
            }

            void UpdateAI(uint32 diff) override
            {
                _events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                switch (_events.ExecuteEvent())
                {
                    case EVENT_FIERY_GRIP:
                        DoCastAOE(SPELL_FIERY_GRIP);
                        _healthPctBreak = me->GetHealth() - me->CountPctFromMaxHealth(20);
                        break;
                    case EVENT_SEARING_PLASMA:
                        DoCastAOE(SPELL_SEARING_PLASMA);
                        _events.ScheduleEvent(EVENT_SEARING_PLASMA, urand(8, 12) * IN_MILLISECONDS);
                        break;
                    default:
                        break;
                }
            }

        private:
            uint64 _healthPctBreak;
            EventMap _events;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
			return new npc_sod_corruptionAI(creature);
        }
};

// http://www.wowhead.com/spell=105490
// http://www.wowhead.com/spell=109379
class spell_searing_plasma_fiery_grip : public SpellScriptLoader
{
public:
    spell_searing_plasma_fiery_grip() : SpellScriptLoader("spell_searing_plasma_fiery_grip") {}

    class spell_searing_plasma_fiery_grip_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_searing_plasma_fiery_grip_SpellScript);

        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            if (!sSpellMgr->GetSpellInfo(SPELL_SEARING_PLASMA))
                return false;
            if (!sSpellMgr->GetSpellInfo(SPELL_FIERY_GRIP))
                return false;
            return true;
        }

        void FilterTargets(std::list<WorldObject*>& targets)
        {
            if (targets.empty() || !GetCaster() || !GetCaster()->GetMap())
                return;

            uint32 numTargets = GetCaster()->GetMap()->Is25ManRaid() ? 3 : 1;
            if (GetSpellInfo()->Id == SPELL_SEARING_PLASMA)
            {
                targets.remove_if(Trinity::UnitAuraCheck(true, SPELL_SEARING_PLASMA_AURA));
                if (targets.size() > numTargets)
                    Trinity::Containers::RandomResizeList(targets, GetCaster()->GetMap()->Is25ManRaid() ? 3 : 1);
            }
            else if (GetSpellInfo()->Id == SPELL_FIERY_GRIP)
                Trinity::Containers::RandomResizeList(targets, targets.size() <= numTargets ? targets.size()-1 : numTargets);

            _targets = targets;
        }

        void FilterTargets2(std::list<WorldObject*>& targets)
        {
            targets.clear();

            targets = _targets;
        }

        void HandleScript(SpellEffIndex /*effIndex*/)
        {
            if (Player* target = GetHitPlayer())
                GetCaster()->CastSpell(target, uint32(GetEffectValue()), true);
        }

        void Register() override
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_searing_plasma_fiery_grip_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_searing_plasma_fiery_grip_SpellScript::FilterTargets2, EFFECT_1, TARGET_UNIT_SRC_AREA_ENEMY);
            OnEffectHitTarget += SpellEffectFn(spell_searing_plasma_fiery_grip_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_DUMMY);
        }

    private:
        std::list<WorldObject*> _targets;
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_searing_plasma_fiery_grip_SpellScript();
    }
};

// http://www.wowhead.com/spell=105834
class spell_superheated_nucleus : public SpellScriptLoader
{
    public:
        spell_superheated_nucleus() : SpellScriptLoader("spell_superheated_nucleus") { }

        class spell_superheated_nucleus_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_superheated_nucleus_AuraScript);

            void HandlePeriodic(AuraEffect const* /*aurEff*/)
            {
                if (GetTarget()->HasAura(SPELL_ROLL_CONTROL_JUMP))
                    PreventDefaultAction();
            }

            void Register() override
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_superheated_nucleus_AuraScript::HandlePeriodic, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_superheated_nucleus_AuraScript();
        }
};

// http://www.wowhead.com/spell=109022
class spell_spine_of_deathwing_check_for_players : public SpellScriptLoader
{
public:
    spell_spine_of_deathwing_check_for_players() : SpellScriptLoader("spell_spine_of_deathwing_check_for_players") {}

    class spell_spine_of_deathwing_check_for_players_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_spine_of_deathwing_check_for_players_SpellScript);

        bool Load() override
        {
            _targetCount = 0;
            return GetCaster()->GetTypeId() == TYPEID_UNIT && GetCaster()->IsAIEnabled;
        }

        void CountTargets(std::list<WorldObject*>& targets)
        {
            _targetCount = targets.size();
        }

        void CheckTargets()
        {
            if (!_targetCount)
                GetCaster()->ToCreature()->AI()->EnterEvadeMode();
            else
                GetCaster()->ToCreature()->AI()->DoZoneInCombat();
        }

        void Register() override
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_spine_of_deathwing_check_for_players_SpellScript::CountTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
            AfterCast += SpellCastFn(spell_spine_of_deathwing_check_for_players_SpellScript::CheckTargets);
        }

    private:
        uint32 _targetCount;
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_spine_of_deathwing_check_for_players_SpellScript();
    }
};

// http://www.wowhead.com/spell=109984
class spell_spine_of_deathwing_anti_exploit_los_check : public SpellScriptLoader
{
    public:
        spell_spine_of_deathwing_anti_exploit_los_check() : SpellScriptLoader("spell_spine_of_deathwing_anti_exploit_los_check") {}

        class spell_spine_of_deathwing_anti_exploit_los_check_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_spine_of_deathwing_anti_exploit_los_check_SpellScript);

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                Unit* caster = GetCaster();
                targets.remove_if([caster](WorldObject* target)
                {
                    if (Unit* _target = target->ToUnit())
                        if (caster->IsWithinLOSInMap(_target) || 
                            _target->HasAura(SPELL_ROLL_CONTROL_JUMP) || 
                            _target->IsFalling() || 
                            _target->GetPositionZ() < caster->GetPositionZ())
                            return true;

                    return false;
                });
            }

            void Register() override
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_spine_of_deathwing_anti_exploit_los_check_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_spine_of_deathwing_anti_exploit_los_check_SpellScript();
        }
};

// http://www.wowhead.com/spell=105777
class spell_spine_of_deathwing_roll_control_jump : public SpellScriptLoader
{
public:
    spell_spine_of_deathwing_roll_control_jump() : SpellScriptLoader("spell_spine_of_deathwing_roll_control_jump") {}

    class spell_spine_of_deathwing_roll_control_jump_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_spine_of_deathwing_roll_control_jump_SpellScript);

        void RelocateDest(SpellDestination& dest)
        {
            const Position pos = { -13875.01f, -13769.59f, 285.1783f };
            dest.Relocate(pos);
        }

        // Spell has some proc flags for positive spell hit, and positive spell cast. Could this be catchers for spellcasts like Life Grip & Heroic Leap similiar effects, to remove the stun effect?

        void Register() override
        {
            OnDestinationTargetSelect += SpellDestinationTargetSelectFn(spell_spine_of_deathwing_roll_control_jump_SpellScript::RelocateDest, EFFECT_0, TARGET_DEST_NEARBY_ENTRY);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_spine_of_deathwing_roll_control_jump_SpellScript();
    }
};

// http://www.wowhead.com/spell=104972
class spell_summon_tentacle_dummy : public SpellScriptLoader
{
public:
    spell_summon_tentacle_dummy() : SpellScriptLoader("spell_summon_tentacle_dummy") {}

    class spell_summon_tentacle_dummy_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_summon_tentacle_dummy_SpellScript);

        void HandleHit(SpellEffIndex effIndex)
        {
            PreventHitDefaultEffect(effIndex);
            if (GetHitUnit()->IsAIEnabled)
                GetHitUnit()->GetAI()->DoAction(ACTION_CORRUPTION_SPAWN); //Notify the spawner
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_summon_tentacle_dummy_SpellScript::HandleHit, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_summon_tentacle_dummy_SpellScript();
    }
};

// http://www.wowhead.com/spell=105535
class spell_spine_of_deathwing_summon_tentacle : public SpellScriptLoader
{
public:
    spell_spine_of_deathwing_summon_tentacle() : SpellScriptLoader("spell_spine_of_deathwing_summon_tentacle") {}

    class spell_spine_of_deathwing_summon_tentacle_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_spine_of_deathwing_summon_tentacle_SpellScript);

        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            if (!sSpellMgr->GetSpellInfo(SPELL_SUMMON_CORRUPTION))
                return false;
            return true;
        }

        bool Load() override
        {
            return GetCaster()->GetTypeId() == TYPEID_UNIT && GetCaster()->IsAIEnabled;
        }

        void HandleHit(SpellEffIndex effIndex)
        {
            PreventHitDefaultEffect(effIndex);
            GetCaster()->CastSpell(GetHitUnit(), uint32(GetEffectValue()), true);
            if (UnitAI* AI = GetHitUnit()->GetAI())
                AI->DoAction(ACTION_DEACTIVATE_SPAWNER);
        }

        void FilterTargets(std::list<WorldObject*>& targets)
        {
            if (Creature* lastSpawner = ObjectAccessor::GetCreature(*GetCaster(), GetCaster()->GetAI()->GetGUID(GUID_LAST_SPAWNER)))
                targets.remove(lastSpawner);

            targets.remove_if([](WorldObject* target)
            {
                return !(target->GetTypeId() == TYPEID_UNIT && target->ToUnit()->HasAura(SPELL_GRASPING_TENDRILS));
            });

            if (!targets.empty())
                Trinity::Containers::RandomResizeList(targets, 1);
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_spine_of_deathwing_summon_tentacle_SpellScript::HandleHit, EFFECT_0, SPELL_EFFECT_DUMMY);
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_spine_of_deathwing_summon_tentacle_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_spine_of_deathwing_summon_tentacle_SpellScript();
    }
};

// http://www.wowhead.com/spell=105537
class spell_spine_of_deathwing_summon_slime : public SpellScriptLoader
{
public:
    spell_spine_of_deathwing_summon_slime() : SpellScriptLoader("spell_spine_of_deathwing_summon_slime") {}

    class spell_spine_of_deathwing_summon_slime_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_spine_of_deathwing_summon_slime_SpellScript);

        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            if (!sSpellMgr->GetSpellInfo(SPELL_SUMMON_SLIME))
                return false;
            return true;
        }

        bool Load() override
        {
            return GetCaster()->GetTypeId() == TYPEID_UNIT && GetCaster()->IsAIEnabled;
        }

        void HandleHit(SpellEffIndex effIndex)
        {
            PreventHitDefaultEffect(effIndex);
            GetCaster()->CastSpell(GetHitUnit(), uint32(GetEffectValue()), true);
        }

        void FilterTargets(std::list<WorldObject*>& targets)
        {
            targets.remove_if([](WorldObject* target)
            {
                return !(target->GetTypeId() == TYPEID_UNIT && target->ToUnit()->HasAura(SPELL_GRASPING_TENDRILS));
            });

            if (!targets.empty())
                Trinity::Containers::RandomResizeList(targets, 1);
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_spine_of_deathwing_summon_slime_SpellScript::HandleHit, EFFECT_0, SPELL_EFFECT_DUMMY);
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_spine_of_deathwing_summon_slime_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_spine_of_deathwing_summon_slime_SpellScript();
    }
};

// http://www.wowhead.com/spell=105366
// http://www.wowhead.com/spell=105384
class spell_spine_of_deathwing_plate_fly_off : public SpellScriptLoader
{
public:
    spell_spine_of_deathwing_plate_fly_off() : SpellScriptLoader("spell_spine_of_deathwing_plate_fly_off") {}

    class spell_spine_of_deathwing_plate_fly_off_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_spine_of_deathwing_plate_fly_off_SpellScript);

        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            if (!sSpellMgr->GetSpellInfo(SPELL_PLATE_FLY_OFF_LEFT))
                return false;
            if (!sSpellMgr->GetSpellInfo(SPELL_PLATE_FLY_OFF_RIGHT))
                return false;
            return true;
        }

        void RecalculateDamage()
        {
            if (!GetHitUnit())
                return;

            SetHitDamage(GetHitUnit()->CountPctFromMaxHealth(35));
        }

        void PlayAnimKit(SpellEffIndex effIndex)
        {
            GetHitGObj()->UseDoorOrButton();
			GetHitGObj()->PlayAnimKit(GetSpellInfo()->Effects[effIndex].MiscValueB);
        }

        void SelectTarget(WorldObject*& target)
        {
            target = (WorldObject*)NULL;
            if (InstanceScript* instance = GetCaster()->GetInstanceScript())
				if (Creature* deathwing = ObjectAccessor::GetCreature(*GetCaster(), instance->GetGuidData(NPC_SPINE_OF_DEATHWING)))
                target = deathwing;
        }

        void Register() override
        {
            OnHit += SpellHitFn(spell_spine_of_deathwing_plate_fly_off_SpellScript::RecalculateDamage);
            OnEffectHitTarget += SpellEffectFn(spell_spine_of_deathwing_plate_fly_off_SpellScript::PlayAnimKit, EFFECT_0, SPELL_EFFECT_ACTIVATE_OBJECT);
            OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_spine_of_deathwing_plate_fly_off_SpellScript::SelectTarget, EFFECT_1, TARGET_UNIT_NEARBY_ENTRY);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_spine_of_deathwing_plate_fly_off_SpellScript();
    }
};

// http://www.wowhead.com/spell=105363
// http://www.wowhead.com/spell=105385
class spell_spine_of_deathwing_breach_armor : public SpellScriptLoader
{
public:
    spell_spine_of_deathwing_breach_armor() : SpellScriptLoader("spell_spine_of_deathwing_breach_armor") {}

    class spell_spine_of_deathwing_breach_armor_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_spine_of_deathwing_breach_armor_SpellScript);

        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            if (!sSpellMgr->GetSpellInfo(SPELL_BREACH_ARMOR_LEFT))
                return false;
            if (!sSpellMgr->GetSpellInfo(SPELL_BREACH_ARMOR_RIGHT))
                return false;
            return true;
        }

        void PlayAnimKit(SpellEffIndex effIndex)
        {
            PreventHitDefaultEffect(effIndex);
			GetHitGObj()->PlayAnimKit(GetSpellInfo()->Effects[effIndex].MiscValueB);
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_spine_of_deathwing_breach_armor_SpellScript::PlayAnimKit, EFFECT_0, SPELL_EFFECT_ACTIVATE_OBJECT);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_spine_of_deathwing_breach_armor_SpellScript();
    }
};

// http://www.wowhead.com/spell=105847
// http://www.wowhead.com/spell=105848
class spell_spine_of_deathwing_seal_armor_breach : public SpellScriptLoader
{
public:
    spell_spine_of_deathwing_seal_armor_breach() : SpellScriptLoader("spell_spine_of_deathwing_seal_armor_breach") {}

    class spell_spine_of_deathwing_seal_armor_breach_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_spine_of_deathwing_seal_armor_breach_SpellScript);

        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            if (!sSpellMgr->GetSpellInfo(SPELL_BREACH_ARMOR_LEFT))
                return false;
            if (!sSpellMgr->GetSpellInfo(SPELL_BREACH_ARMOR_RIGHT))
                return false;
            return true;
        }

        void PlayAnimKit(SpellEffIndex effIndex)
        {
            PreventHitDefaultEffect(effIndex);
			GetHitGObj()->PlayAnimKit(GetSpellInfo()->Effects[effIndex].MiscValueB);
        }

        void ScriptEffect(SpellEffIndex effIndex)
        {
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_spine_of_deathwing_seal_armor_breach_SpellScript::PlayAnimKit, EFFECT_0, SPELL_EFFECT_ACTIVATE_OBJECT);
            OnEffectHitTarget += SpellEffectFn(spell_spine_of_deathwing_seal_armor_breach_SpellScript::ScriptEffect, EFFECT_1, SPELL_EFFECT_SCRIPT_EFFECT);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_spine_of_deathwing_seal_armor_breach_SpellScript();
    }
};

// http://www.wowhead.com/spell=104621
class spell_spine_of_deathwing_roll_control : public SpellScriptLoader
{
public:
    spell_spine_of_deathwing_roll_control() : SpellScriptLoader("spell_spine_of_deathwing_roll_control") {}

    class spell_spine_of_deathwing_roll_control_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_spine_of_deathwing_roll_control_SpellScript);

        bool Load() override
        {
            _rollCommand = ACTION_ROLL_CANCEL;
            return GetCaster()->GetTypeId() == TYPEID_UNIT && GetCaster()->IsAIEnabled;
        }

        void TriggerRollSequence()
        {
            GetCaster()->GetAI()->DoAction(_rollCommand);
        }

        void FilterTargets(std::list<WorldObject*>& targets)
        {
            std::list<WorldObject*> leftTargets;
            std::list<WorldObject*> rightTargets;

            for (auto target : targets)
            if (GetCaster()->GetAngle(target) + M_PI * 2 < GetCaster()->GetOrientation() + M_PI * 2)
                rightTargets.push_back(target);
            else
                leftTargets.push_back(target);

            if (!rightTargets.empty() && !leftTargets.empty())
            {
                if (leftTargets.size() / rightTargets.size() > 0.5 && leftTargets.size() / rightTargets.size() < 1.5)
                    _rollCommand = leftTargets.size() > rightTargets.size() ? ACTION_ROLL_LEFT : ACTION_ROLL_RIGHT;
            }
            else if (rightTargets.empty() && !leftTargets.empty())
                _rollCommand = ACTION_ROLL_LEFT;
            else if (leftTargets.empty() && !rightTargets.empty())
                _rollCommand = ACTION_ROLL_RIGHT;
        }

        void Register() override
        {
            AfterCast += SpellCastFn(spell_spine_of_deathwing_roll_control_SpellScript::TriggerRollSequence);
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_spine_of_deathwing_roll_control_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
        }

    private:
        Actions _rollCommand;
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_spine_of_deathwing_roll_control_SpellScript();
    }
};

// http://www.wowhead.com/spell=105740
class spell_spine_of_deathwing_roll_control_kick_off : public SpellScriptLoader
{
public:
    spell_spine_of_deathwing_roll_control_kick_off() : SpellScriptLoader("spell_spine_of_deathwing_roll_control_kick_off") {}

    class spell_spine_of_deathwing_roll_control_kick_off_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_spine_of_deathwing_roll_control_kick_off_SpellScript);

        void HandleScript(SpellEffIndex index)
        {
            PreventHitAura();
            GetHitUnit()->GetMotionMaster()->MoveJump(-13855.85f, -13841.3f, 300.0f, 30.0f, 20.0f);
        }

        void FilterTargets(std::list<WorldObject*>& targets)
        {
            targets.remove_if([](WorldObject* target)
            {
                if (target->GetTypeId() == TYPEID_UNIT && target->GetEntry() == NPC_HIDEOUS_AMALGAMATION)
                    return true;

                if (target->GetTypeId() == TYPEID_PLAYER && (target->ToPlayer()->HasAura(SPELL_GRASPING_TENDRILS_AURA) || target->ToPlayer()->HasAura(SPELL_FIERY_GRIP)))
                    return true;

                return false;
            });
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_spine_of_deathwing_roll_control_kick_off_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_spine_of_deathwing_roll_control_kick_off_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_spine_of_deathwing_roll_control_kick_off_SpellScript();
    }
};

// http://www.wowhead.com/spell=105845
class spell_nuclear_blast : public SpellScriptLoader
{
public:
    spell_nuclear_blast() : SpellScriptLoader("spell_nuclear_blast") {}

    class spell_nuclear_blast_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_nuclear_blast_SpellScript);

        void HandleAfterCast()
        {
            GetCaster()->CastSpell((Unit*)NULL, SPELL_NUCLEAR_BLAST_SCRIPT_EFFECT, true);
            GetCaster()->Kill(GetCaster());
        }

        void Register() override
        {
            AfterCast += SpellCastFn(spell_nuclear_blast_SpellScript::HandleAfterCast);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_nuclear_blast_SpellScript();
    }
};

// http://www.wowhead.com/spell=105846
class spell_nuclear_blast_script_effect : public SpellScriptLoader
{
public:
    spell_nuclear_blast_script_effect() : SpellScriptLoader("spell_nuclear_blast_script_effect") {}

    class spell_nuclear_blast_script_effect_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_nuclear_blast_script_effect_SpellScript);

        void FilterTargets(std::list<WorldObject*>& targets)
        {
            Unit* caster = GetCaster();
            targets.remove_if([caster](WorldObject* target)
            {
                return !(target->GetTypeId() == TYPEID_UNIT && caster->GetDistance2d(target) < 10.0f && (target->GetEntry() == NPC_BURNING_TENDON_1 || target->GetEntry() == NPC_BURNING_TENDON_2));
            });
            targets.sort(Trinity::ObjectDistanceOrderPred(GetCaster()));
            if (!targets.empty())
                targets.resize(1);
            else if (GetCaster()->GetTypeId() == TYPEID_UNIT && GetCaster()->IsAIEnabled)
                GetCaster()->ToCreature()->AI()->Talk(EMOTE_AMALG_NUCLEAR_BLAST_FAIL);
        }

        void Register() override
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_nuclear_blast_script_effect_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_nuclear_blast_script_effect_SpellScript();
    }
};

// http://www.wowhead.com/spell=106199
// http://www.wowhead.com/spell=106200
class spell_spine_of_deathwing_blood_corruption : public SpellScriptLoader
{
    public:
        spell_spine_of_deathwing_blood_corruption(char const* scriptName, uint32 spellId) : SpellScriptLoader(scriptName), _spellId(spellId) {}

        class spell_spine_of_deathwing_blood_corruption_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_spine_of_deathwing_blood_corruption_SpellScript);

        public:
            spell_spine_of_deathwing_blood_corruption_SpellScript(uint32 spellId) : SpellScript(), _spellId(spellId) { }

            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                if (!sSpellMgr->GetSpellInfo(_spellId))
                    return false;
                return true;
            }

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                std::list<WorldObject*> leftTargets;
                std::list<WorldObject*> rightTargets;
                for (auto target : targets)
                    if (target->GetPositionX() < GetCaster()->GetPositionX())
                        leftTargets.push_back(target);
                    else
                        rightTargets.push_back(target);

                targets.clear();

                if (GetSpellInfo()->Effects[EFFECT_1].BasePoints == 0) //Left
                    targets.push_back(getTarget(leftTargets, rightTargets));
                else
                    targets.push_back(getTarget(rightTargets, leftTargets));
            }

            WorldObject* getTarget(std::list<WorldObject*> targetListOne, std::list<WorldObject*> targetListTwo)
            {
                if (!targetListOne.empty())
                    return Trinity::Containers::SelectRandomContainerElement(targetListOne);
                else if (!targetListTwo.empty())
                    return Trinity::Containers::SelectRandomContainerElement(targetListTwo);
                else
                    return (WorldObject*)NULL;
            }

            void Register() override
            {
                //OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_spine_of_deathwing_blood_corruption_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
            }

            uint32 _spellId;
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_spine_of_deathwing_blood_corruption_SpellScript(_spellId);
        }

        class spell_spine_of_deathwing_blood_corruption_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_spine_of_deathwing_blood_corruption_AuraScript);

        public:
            spell_spine_of_deathwing_blood_corruption_AuraScript(uint32 spellId) : AuraScript(), _spellId(spellId) { }

            bool Validate(SpellInfo const* /*spell*/) override
            {
                if (!sSpellMgr->GetSpellInfo(_spellId))
                    return false;
                return true;
            }

            void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                switch (GetTargetApplication()->GetRemoveMode())
                {
                    case AURA_REMOVE_BY_ENEMY_SPELL:
                    {
                        InstanceScript* instance = GetCaster()->GetInstanceScript();
                        if (!GetCaster())
                            return;

                        int32 dispels = GetSpellInfo()->Effects[EFFECT_1].BasePoints + 2;
                        std::cout << "\ndispels: " << dispels << "\n";
                        if (_spellId == SPELL_BLOOD_CORRUPTION_EARTH || (_spellId == SPELL_BLOOD_CORRUPTION_DEATH && (dispels == 2 && urand(1, 2) == 1) || dispels == 4)) //50% chance to turn to earth on 2 dispels, 100% on 4.
                            GetCaster()->CastCustomSpell(SPELL_BLOOD_CORRUPTION_EARTH, SPELLVALUE_BASE_POINT1, dispels, GetTarget(), TRIGGERED_FULL_MASK, NULL, NULL, GetCasterGUID());
                        else
                            GetCaster()->CastCustomSpell(SPELL_BLOOD_CORRUPTION_DEATH, SPELLVALUE_BASE_POINT1, dispels, GetTarget(), TRIGGERED_FULL_MASK, NULL, NULL, GetCasterGUID());
                        break;
                    }
                    case AURA_REMOVE_BY_EXPIRE:
                        if (!GetCaster())
                            return;

                        if (_spellId == SPELL_BLOOD_CORRUPTION_EARTH)
                            if (GetCaster()->GetMap()->Is25ManRaid())
                                GetTarget()->SetAuraStack(SPELL_BLOOD_OF_NELTHARION, GetTarget(), 2);
                            else
                                GetTarget()->CastSpell((Unit*)NULL, SPELL_BLOOD_OF_NELTHARION, true);
                        else
                            GetTarget()->CastSpell((Unit*)NULL, SPELL_BLOOD_OF_DEATHWING, true);
                    case AURA_REMOVE_BY_DEATH:
                        if (_spellId == SPELL_BLOOD_CORRUPTION_DEATH)
                            GetTarget()->CastSpell((Unit*)NULL, SPELL_BLOOD_OF_DEATHWING, true);
                        break;
                    default:
                        return;
                }
            }

            void Register() override
            {
                AfterEffectRemove += AuraEffectRemoveFn(spell_spine_of_deathwing_blood_corruption_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }

            uint32 _spellId;
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_spine_of_deathwing_blood_corruption_AuraScript(_spellId);
        }

    private:
        uint32 _spellId;
};

/*// http://www.wowhead.com/spell=106199
// http://www.wowhead.com/spell=106200
class spell_spine_of_deathwing_blood_corruption : public SpellScriptLoader
{
    public:
        spell_spine_of_deathwing_blood_corruption(char const* scriptName, uint32 spellId) : SpellScriptLoader(scriptName), _spellId(spellId) {}

        class spell_spine_of_deathwing_blood_corruption_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_spine_of_deathwing_blood_corruption_SpellScript);

        public:
            spell_spine_of_deathwing_blood_corruption_SpellScript(uint32 spellId) : SpellScript(), _spellId(spellId) { }

            bool Validate(SpellInfo const* spellInfo) override
            {
                if (!sSpellMgr->GetSpellInfo(_spellId))
                    return false;
                return true;
            }

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                std::list<WorldObject*> leftTargets;
                std::list<WorldObject*> rightTargets;
                for (auto target : targets)
                if (target->GetPositionX() < GetCaster()->GetPositionX())
                    leftTargets.push_back(target);
                else
                    rightTargets.push_back(target);
                targets.clear();

                if (GetSpellInfo()->Effects[EFFECT_1].BasePoints == 0) //Left
                    targets.push_back(getTarget(leftTargets, rightTargets));
                else
                    targets.push_back(getTarget(rightTargets, leftTargets));
            }

            WorldObject* getTarget(std::list<WorldObject*> targetListOne, std::list<WorldObject*> targetListTwo)
            {
                if (!targetListOne.empty())
                    return Trinity::Containers::SelectRandomContainerElement(targetListOne);
                else if (!targetListTwo.empty())
                    return Trinity::Containers::SelectRandomContainerElement(targetListTwo);
                else
                    return (WorldObject*)NULL;
            }

            void Register() override
            {
                //OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_spine_of_deathwing_blood_corruption_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
            }

            uint32 _spellId;
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_spine_of_deathwing_blood_corruption_SpellScript(_spellId);
        }

        class spell_spine_of_deathwing_blood_corruption_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_spine_of_deathwing_blood_corruption_AuraScript);

        public:
            spell_spine_of_deathwing_blood_corruption_AuraScript(uint32 spellId) : AuraScript(), _spellId(spellId) { }

            bool Validate(SpellInfo const* spell) override
            {
                if (!sSpellMgr->GetSpellInfo(_spellId))
                    return false;
                return true;
            }

            void OnRemove(AuraEffect const* aurEff, AuraEffectHandleModes mode)
            {
                switch (GetTargetApplication()->GetRemoveMode())
                {
                    case AURA_REMOVE_BY_ENEMY_SPELL:
                    {
                        if (!GetCaster())
                            return;

                        int32 dispels = GetSpellInfo()->Effects[EFFECT_1].BasePoints + 2;
                        std::cout << "\ndispels: " << dispels << "\n";
                        if (_spellId == SPELL_BLOOD_CORRUPTION_EARTH || (_spellId == SPELL_BLOOD_CORRUPTION_DEATH && (dispels == 2 && urand(1, 2) == 1) || dispels == 4)) //50% chance to turn to earth on 2 dispels, 100% on 4.
                            GetCaster()->CastCustomSpell(SPELL_BLOOD_CORRUPTION_EARTH, SPELLVALUE_BASE_POINT1, dispels, GetTarget(), TRIGGERED_FULL_MASK, NULL, NULL, GetCasterGUID());
                        else
                            GetCaster()->CastCustomSpell(SPELL_BLOOD_CORRUPTION_DEATH, SPELLVALUE_BASE_POINT1, dispels, GetTarget(), TRIGGERED_FULL_MASK, NULL, NULL, GetCasterGUID());
                        break;
                    }
                    case AURA_REMOVE_BY_EXPIRE:
                        if (!GetCaster())
                            return;

                        if (_spellId == SPELL_BLOOD_CORRUPTION_EARTH)
                            if (GetCaster()->GetMap()->Is25ManRaid())
                                GetTarget()->SetAuraStack(SPELL_BLOOD_OF_NELTHARION, GetTarget(), 2);
                            else
                                GetTarget()->CastSpell((Unit*)NULL, SPELL_BLOOD_OF_NELTHARION, true);
                    case AURA_REMOVE_BY_DEATH:
                        if (_spellId == SPELL_BLOOD_CORRUPTION_DEATH)
                            GetTarget()->CastSpell((Unit*)NULL, SPELL_BLOOD_OF_DEATHWING, true);
                        break;
                    default:
                        return;
                }
            }

            void Register() override
            {
                AfterEffectRemove += AuraEffectRemoveFn(spell_spine_of_deathwing_blood_corruption_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }

            uint32 _spellId;
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_spine_of_deathwing_blood_corruption_AuraScript(_spellId);
        }

    private:
        uint32 _spellId;
};*/

// http://www.wowhead.com/spell=105241
class spell_spine_of_deathwing_absorb_blood : public SpellScriptLoader
{
public:
    spell_spine_of_deathwing_absorb_blood() : SpellScriptLoader("spell_spine_of_deathwing_absorb_blood") {}

    class spell_spine_of_deathwing_absorb_blood_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_spine_of_deathwing_absorb_blood_SpellScript);

        void HandleHit(SpellEffIndex effIndex)
        {
            GetHitUnit()->SetNativeDisplayId(11686);
            GetHitUnit()->Kill(GetHitUnit());
            GetHitUnit()->CastSpell(GetCaster(), SPELL_ABSORBED_BLOOD, true);
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_spine_of_deathwing_absorb_blood_SpellScript::HandleHit, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_spine_of_deathwing_absorb_blood_SpellScript();
    }
};

// http://www.wowhead.com/spell=109083
class spell_spine_of_deathwing_energize : public SpellScriptLoader
{
public:
    spell_spine_of_deathwing_energize() : SpellScriptLoader("spell_spine_of_deathwing_energize") {}

    class spell_spine_of_deathwing_energize_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_spine_of_deathwing_energize_SpellScript);

        void HandleHit(SpellEffIndex effIndex)
        {
            int32 _altPower = GetHitUnit()->GetPower(POWER_ALTERNATE_POWER);
            GetHitUnit()->SetPower(POWER_ALTERNATE_POWER, _altPower < 9 ? _altPower + 1 : _altPower);
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_spine_of_deathwing_energize_SpellScript::HandleHit, EFFECT_1, SPELL_EFFECT_SCRIPT_EFFECT);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_spine_of_deathwing_energize_SpellScript();
    }
};

// http://www.wowhead.com/spell=100341
class spell_sod_activate_spawner : public SpellScriptLoader
{
    public:
        spell_sod_activate_spawner() : SpellScriptLoader("spell_sod_activate_spawner") {}

        class spell_sod_activate_spawner_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sod_activate_spawner_SpellScript);

            void SelectTarget(WorldObject*& target)
            {
                target = (WorldObject*)NULL;
                if (Creature* spawner = GetCaster()->FindNearestCreature(NPC_SPAWNER, 2.5f))
                    target = spawner;
            }

            void Register() override
            {
                OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_sod_activate_spawner_SpellScript::SelectTarget, EFFECT_1, TARGET_UNIT_NEARBY_ENTRY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_sod_activate_spawner_SpellScript();
        }
};

class at_spine_of_deathwing : public AreaTriggerScript
{
    public:
        at_spine_of_deathwing() : AreaTriggerScript("at_spine_of_deathwing") {}

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*trigger*/) override
        {
            if (player->IsAlive() && !player->IsGameMaster())
                if (InstanceScript* instance = player->GetInstanceScript())
                    if (Creature* deathwing = instance->instance->GetCreature(instance->GetGuidData(NPC_SPINE_OF_DEATHWING))) //More proper way to get a creature?
                        deathwing->Kill(player);

            return true;
        }
};

// http://www.wowhead.com/achievement=6133
class achievement_maybe_hell_get_dizzy : public AchievementCriteriaScript
{
public:
    achievement_maybe_hell_get_dizzy() : AchievementCriteriaScript("achievement_maybe_hell_get_dizzy") {}

    bool OnCheck(Player* /*source*/, Unit* target) override
    {
        return target && target->GetAI()->GetData(DATA_MAYBE_HELL_GET_DIZZY);
    }
};

void AddSC_boss_spine_of_deathwing()
{
    new boss_spine_of_deathwing();
    new npc_sod_hideous_amalgamation();
	new npc_sod_corrupted_blood();
	new npc_sod_burning_tendon();
	new npc_sod_spawner();
	new npc_sod_corruption();

    new spell_searing_plasma_fiery_grip();
    new spell_superheated_nucleus();
    new spell_spine_of_deathwing_roll_control_jump();
    new spell_spine_of_deathwing_summon_tentacle();
    new spell_spine_of_deathwing_summon_slime();
    new spell_spine_of_deathwing_plate_fly_off();
    new spell_spine_of_deathwing_breach_armor();
    new spell_spine_of_deathwing_seal_armor_breach();
    new spell_spine_of_deathwing_roll_control();
    new spell_spine_of_deathwing_roll_control_kick_off();
    new spell_nuclear_blast();
    new spell_nuclear_blast_script_effect();
    //new spell_spine_of_deathwing_blood_corruption("spell_blood_corruption_death", SPELL_BLOOD_CORRUPTION_DEATH);
    //new spell_spine_of_deathwing_blood_corruption("spell_blood_corruption_earth", SPELL_BLOOD_CORRUPTION_EARTH);
    new spell_spine_of_deathwing_absorb_blood();
    new spell_spine_of_deathwing_energize();

    //new
    new spell_sod_activate_spawner();

    new at_spine_of_deathwing();

    new achievement_maybe_hell_get_dizzy();
}
