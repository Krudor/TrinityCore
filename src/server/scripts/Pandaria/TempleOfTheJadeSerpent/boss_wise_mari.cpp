/* ScriptData
SDName: boss_wise_mari
SD%Complete: 0
SDComment: Hydrolance seems a bit bugged (barely noticable) on retail.
    Cannot be 100% sure, but will fix this potential (not 100% sure) bug.
    Known bugs:
        Water Bubble (106062) seems to block periodic ticks from spell id 106098 from going off, this prevents Wise Mari from performing Hydrolance on the top positions. Caused by the spells aura immunity?
SDCategory: Temple of the Jade Serpent
EndScriptData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "temple_of_the_jade_serpent.h"
#include "PassiveAI.h"
#include "TaskScheduler.h"

enum Spells
{
    SPELL_CORRUPTED_WATERS                  = 115165,

    SPELL_DRY                               = 128437,
    SPELL_WATER_BUBBLE                      = 106062,
    SPELL_BUBBLE_BURST                      = 106612,

    SPELL_HYDROLANCE_PRECAST_BOTTOM         = 115229,
    SPELL_HYDROLANCE_PRECAST_TOP_VISUAL     = 115220,
    SPELL_HYDROLANCE_PRECAST_TOP            = 115227,
    SPELL_HYDROLANCE                        = 106055,


    SPELL_WISE_MARI_BOTTOM_HYDROLANCE_AURA  = 106097,
    SPELL_WISE_MARI_TOP_HYDROLANCE_AURA     = 106098,
    SPELL_SMALL_HYDROLANCE_TICK             = 106104, // 2.6f distance increment
    SPELL_SPLASH_STALKER_HYDROLANCE_AURA    = 106250,
    SPELL_MEDIUM_HYDROLANCE_TICK            = 106319,
    SPELL_LARGE_HYDROLANCE_TICK             = 106267,

    SPELL_CALL_WATER                        = 106526,

    SPELL_CORRUPTED_FOUNTAIN                = 106518,
    SPELL_CORRUPT_FOUNTAIN                  = 106519, // Triggers Corrupted Fountain

    SPELL_WASH_AWAY                         = 106331,
    SPELL_WASH_AWAY_TURN_TRIGGER            = 115575,

    SPELL_KNEEL                             = 115368,
    SPELL_BLESSING_OF_THE_WATERSPEAKER      = 121483,
    SPELL_RELEASED_FROM_SHA                 = 106770,
    SPELL_CLEANSING_WATERS                  = 106771,
    SPELL_CLAIMING_WATERS                   = 115622,
    SPELL_QUIET_SUICIDE                     = 115372,
};

enum Events
{
    EVENT_NONE,
    EVENT_CHECK_IF_DEAD,
    EVENT_OUTRO_1,
    EVENT_OUTRO_2,
};

enum Data
{
    DATA_INTRO,
    DATA_IN_WATERS,
};

enum Actions
{
    ACTION_NONE,
    ACTION_INTRO,
    ACTION_AT_WATERS,
};

enum Texts
{
    SAY_INTRO = 0,
    SAY_AGGRO = 1,
    SAY_WATERS = 2,
    SAY_KILL = 3,
    SAY_CALL_WATER = 4,
    SAY_PHASE_2 = 5,
    SAY_75_PCT = 6,
    SAY_50_PCT = 7,
    SAY_25_PCT = 8,

    SAY_DEATH_1 = 9,
    SAY_DEATH_2 = 10,
    SAY_DEATH_3 = 11,
    SAY_WASH_AWAY = 12,

    SAY_WATER_SUMMON = 0,
};

enum CreatureGroups
{
    CG_FOUNTAIN_STALKERS        = 0,
    CG_SPLASH_STALKERS          = 1,
};

enum Phases
{
    PHASE_NONE,
    PHASE_ONE,
    PHASE_TWO,

    PHASE_MASK_ONE		= 1 << PHASE_ONE,
    PHASE_MASK_TWO		= 1 << PHASE_TWO,
};

enum NextAbility
{
    NEXT_ABILITY_NULL,
    NEXT_ABILITY_HYDROLANCE,
    NEXT_ABILITY_CALL_WATER,
    NEXT_ABILITY_BURST_BUBBLE
};

enum HydrolanceGroup : uint32
{
    HYDROLANCE_NULL,
    HYDROLANCE_BOTTOM,
    HYDROLANCE_TOP
};

enum HydrolanceLocation : uint32
{
    LOCATION_BOTTOM,
    LOCATION_TOP_1,
    LOCATION_TOP_2,

    LOCATION_MAX
};

struct HydrolanceInfo
{
    float Orientation;
    HydrolanceGroup Group;
    std::chrono::milliseconds CastDelay;
};

uint32 const HydrolanceLocationsMax = 3;
HydrolanceInfo const HydrolanceLocations[HydrolanceLocationsMax] =
{
    { 1.256637f, HYDROLANCE_BOTTOM, Milliseconds(3300) },
    { 3.577925f, HYDROLANCE_TOP, Seconds(2) },
    { 5.256206f, HYDROLANCE_TOP, Seconds(2) }
};

enum Points
{
};

enum AnimKits
{
};

Position const CorruptedWatersStalkerPos = { 1046.941f, -2560.606f, 174.9552f, 1.285821f };
Position const SplashStalkerSpecialPos = { 1051.655f, -2545.786f, 170.2738f, 1.256637f };


class boss_wise_mari : public CreatureScript
{
    public:
        boss_wise_mari() : CreatureScript("boss_wise_mari") {}

        struct boss_wise_mariAI : public NewReset_BossAI
        {
            explicit boss_wise_mariAI(Creature* creature) : NewReset_BossAI(creature, BOSS_WISE_MARI), _introDone(false), _aggroByWater(false), _sayKill(false), _sayBelow75(false), _sayBelow50(false), _sayBelow25(false), hydrolanceGroup(), _lastHydrolanceDirection(0)
            { }

            void InitializeAI() override
            {
                NewReset_BossAI::InitializeAI();
                FullReset(true);
            }

            void ResetVariables() override
            {
                _introDone = false;
                _aggroByWater = false;
                _sayKill = false;
                _sayBelow75, _sayBelow50, _sayBelow25 = false;
                hydrolanceGroup = NULL;
                _lastHydrolanceDirection = NULL;
            }

            void FullReset(bool initialize = false) override
            {
                _introDone = false;

                DespawnCreature(CorruptedWatersStalkerGUID);
                DespawnCreatures(FountainStalkerGUIDs);
                DespawnCreatures(SplashStalkerGUIDs);
                // Our workaround until TrinityCore finds a way to standardize a proper way to get specific creature guids, this creature and the following creaturegroup share the same entry id but have different tasks.
                me->SummonCreatureGroup(CG_SPLASH_STALKERS, &SplashStalkerGUIDs);
                if (TempSummon* summon = me->SummonCreature(NPC_CORRUPTED_WATERS_STALKER, CorruptedWatersStalkerPos))
                    SplashStalkerSpecialGUID = summon->GetGUID();

                NewReset_BossAI::FullReset(initialize);
            }

            void Reset() override
            {
                _aggroByWater = false;
                _sayKill = true;
                _sayBelow75 = _sayBelow50 = _sayBelow25 = true;
                me->SetReactState(REACT_DEFENSIVE);
                me->RemoveAurasDueToSpell(SPELL_CLEANSING_WATERS);
                me->RemoveAurasDueToSpell(SPELL_QUIET_SUICIDE);
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                me->RemoveFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_DISABLE_TURN);
                me->SummonCreatureGroup(CG_FOUNTAIN_STALKERS, &FountainStalkerGUIDs);
                me->LoadEquipment(1);
            }

            void DoAction(int32 action) override
            {
                switch (action)
                {
                    case ACTION_INTRO:
                        if (_introDone)
                            break;

                        _introDone = true;
                        BossAI::Talk(SAY_INTRO);
                        break;
                    case ACTION_AT_WATERS:
                        if (me->IsInCombat())
                            break;

                        _aggroByWater = true;
                        DoZoneInCombat();
                        break;
                    default:
                        break;
                }
            }

            void EnterEvadeMode(EvadeReason why) override
            {
                scheduler.CancelAll();
                DespawnCreatures(FountainStalkerGUIDs);
                NewReset_BossAI::EnterEvadeMode(why);
                _DespawnAtEvade(10);
            }

            void JustDied(Unit* killer) override
            {
            }

            void KilledUnit(Unit* victim) override
            {
                if (victim->GetTypeId() == TYPEID_PLAYER)
                    Talk(SAY_KILL, victim, Seconds(8));

                //if (victim->GetTypeId() != TYPEID_PLAYER || !_sayKill)
                //    return;

                //_sayKill = false;
                //Talk(SAY_KILL);
                //scheduler.Schedule(Seconds(8), [this](TaskContext context)
                //{
                //    _sayKill = true;
                //});
            }

            void JustSummoned(Creature* summon) override
            {
                switch (summon->GetEntry())
                {
                    case NPC_CORRUPT_LIVING_WATER:
                        if (!summon->IsAIEnabled)
                            break;

                        summon->AI()->Talk(SAY_WATER_SUMMON, me);
                        summon->AI()->DoCastAOE(SPELL_CORRUPT_FOUNTAIN);
                        summon->AI()->DoZoneInCombat();
                        break;
                    default:
                        return;
                }

                summons.Summon(summon);
            }

            void SummonedCreatureDies(Creature* summon, Unit* /*killer*/) override
            {
                summons.Despawn(summon);
            }

            void MoveInLineOfSight(Unit* /*who*/) override { }

            void AttackStart(Unit* who) override
            {
                if (me->Attack(who, false))
                    DoStartNoMovement(who);
            }

            void EnterCombat(Unit* who) override
            {
                NewReset_BossAI::EnterCombat(who);
                DoStartNoMovement(who);
                me->SetReactState(REACT_PASSIVE);
                me->AttackStop();

                BossAI::Talk(SAY_AGGRO, who);
                if (_aggroByWater)
                    BossAI::Talk(SAY_WATERS, who);

                DoCast(SPELL_WATER_BUBBLE);
                CastHydrolance(LOCATION_BOTTOM);
            }

            void DamageTaken(Unit* attacker, uint32& damage) override
            {
                if (damage > me->GetHealth())
                    damage = me->GetHealth() - 1;
            }

            void UpdateAI(uint32 diff) override
            {
                UpdateVictim();
                scheduler.Update(diff);
            }

            void SpellCast(SpellInfo const* spell)
            {
                switch (spell->Id)
                {
                    case SPELL_HYDROLANCE:
                        switch (hydrolanceGroup)
                        {
                            case HYDROLANCE_BOTTOM:
                                me->Say("Apply bottom hydrolance aura", LANG_UNIVERSAL);
                                me->AddAura(SPELL_WISE_MARI_BOTTOM_HYDROLANCE_AURA, me);
                                if (Creature* stalker = ObjectAccessor::GetCreature(*me, SplashStalkerSpecialGUID))
                                    me->AddAura(SPELL_SPLASH_STALKER_HYDROLANCE_AURA, stalker);
                                break;
                            case HYDROLANCE_TOP:
                                me->Say("Apply top hydrolance aura", LANG_UNIVERSAL);
                                me->AddAura(SPELL_WISE_MARI_TOP_HYDROLANCE_AURA, me);
                                break;
                            default:
                                break;
                        }
                        break;
                    case SPELL_BUBBLE_BURST:
                        scheduler.Schedule(Seconds(1), [this](TaskContext context)
                        {
                            BossAI::Talk(SAY_WASH_AWAY);
                            me->CastSpell(me, SPELL_WASH_AWAY_TURN_TRIGGER, true);
                            scheduler.Schedule(Milliseconds(2400), [this](TaskContext context)
                            {
                                me->CastSpell(me, SPELL_WASH_AWAY, true);
                            });
                            ScheduleTasks();
                        });
                        break;
                    default:
                        break;
                }
            }

            NextAbility GetNextAbility()
            {
                if (!summons.HasEntry(NPC_CORRUPT_LIVING_WATER))
                {
                    std::list<ObjectGuid> uncorruptedFountains;
                    for (auto guid : FountainStalkerGUIDs)
                        if (Creature* stalker = ObjectAccessor::GetCreature(*me, guid))
                            if (!stalker->HasAura(SPELL_CORRUPTED_FOUNTAIN))
                                uncorruptedFountains.push_back(guid);
                    if (!uncorruptedFountains.empty())
                    {
                        Creature* fountain = ObjectAccessor::GetCreature(*me, Trinity::Containers::SelectRandomContainerElement(uncorruptedFountains));
                        if (fountain)
                            return NEXT_ABILITY_CALL_WATER;
                    }

                    return NEXT_ABILITY_BURST_BUBBLE;
                }

                return NEXT_ABILITY_HYDROLANCE;
            }

            void CastNextAbility(NextAbility ability)
            {
                switch (ability)
                {
                    case NEXT_ABILITY_CALL_WATER:
                    {
                        std::list<ObjectGuid> uncorruptedFountains;
                        for (auto guid : FountainStalkerGUIDs)
                            if (Creature* stalker = ObjectAccessor::GetCreature(*me, guid))
                                if (!stalker->HasAura(SPELL_CORRUPTED_FOUNTAIN))
                                    uncorruptedFountains.push_back(guid);

                        Creature* fountain = ObjectAccessor::GetCreature(*me, Trinity::Containers::SelectRandomContainerElement(uncorruptedFountains));
                        if (!fountain)
                        {
                            EnterEvadeMode(EVADE_REASON_OTHER);
                            return;
                        }

                        me->SetFacingToObject(fountain);
                        BossAI::Talk(SAY_CALL_WATER);
                        DoCast(fountain, SPELL_CALL_WATER);
                        scheduler.Schedule(Milliseconds(3600), [this](TaskContext context)
                        {
                             CastNextAbility(NEXT_ABILITY_HYDROLANCE);
                        });
                    }
                        break;
                    case NEXT_ABILITY_HYDROLANCE:
                        CastHydrolance(static_cast<HydrolanceLocation>(urand(0, LOCATION_MAX - 1)));
                        break;
                    case NEXT_ABILITY_BURST_BUBBLE:
                        PreparePhase(PHASE_TWO);
                        return;
                    default:
                        break;
                }
            }

            void CastHydrolance(HydrolanceLocation location)
            {
                HydrolanceInfo const info = HydrolanceLocations[location];
                me->SetOrientation(info.Orientation); // At the time of scripting this, there's still an orientation bug that has to be fixed that will not be addressed here.
                me->SetFacingTo(info.Orientation);
                hydrolanceGroup = info.Group;

                switch (info.Group)
                {
                    case HYDROLANCE_BOTTOM:
                        DoCast(SPELL_HYDROLANCE_PRECAST_BOTTOM);
                        break;
                    case HYDROLANCE_TOP:
                        DoCast(SPELL_HYDROLANCE_PRECAST_TOP);
                        break;
                    default:
                        break;
                }

                DoCast(SPELL_HYDROLANCE);
                
                scheduler.Schedule(Seconds(4), [this, info](TaskContext context)
                {
                    NextAbility ability = GetNextAbility();
                    context.Schedule(info.CastDelay, [this, ability](TaskContext)
                    {
                        CastNextAbility(ability);
                    });
                });
            }

            void PreparePhase(Phases phase)
            {
                switch (phase)
                {
                    case PHASE_ONE:
                        break;
                    case PHASE_TWO:
                        BossAI::Talk(SAY_PHASE_2);
                        DoCast(SPELL_BUBBLE_BURST);
                        break;
                    default:
                        break;
                }
            }

            void ScheduleTasks()
            {
                scheduler.Schedule(Seconds(1), [this](TaskContext task)
                {
                    if (_sayBelow75 && me->GetHealthPct() <= 75)
                    {
                        BossAI::Talk(SAY_75_PCT);
                        _sayBelow75 = !_sayBelow75;
                    }
                    else if (_sayBelow50 && me->GetHealthPct() <= 50)
                    {
                        BossAI::Talk(SAY_50_PCT);
                        _sayBelow50 = !_sayBelow50;
                    }
                    else if (_sayBelow25 && me->GetHealthPct() <= 25)
                    {
                        BossAI::Talk(SAY_25_PCT);
                        _sayBelow25 = !_sayBelow25;
                    }

                    if (me->GetHealth() != 1)
                        task.Repeat(Seconds(1));
                    else
                    {
                        me->SetOrientation(me->GetHomePosition().GetOrientation());
                        me->SetFacingTo(me->GetHomePosition().GetOrientation());
                        BossAI::Talk(SAY_DEATH_1);
                        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                        me->SetFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_DISABLE_TURN);
                        DoCast(SPELL_KNEEL);
                        DoCastAOE(SPELL_BLESSING_OF_THE_WATERSPEAKER);
                        instance->SetBossState(BOSS_WISE_MARI, DONE);
                        task.Schedule(Milliseconds(3600), [this](TaskContext task)
                        {
                            switch (task.GetRepeatCounter())
                            {
                                case 0:
                                    BossAI::Talk(SAY_DEATH_2);
                                    DoCast(SPELL_RELEASED_FROM_SHA);
                                    DoCast(SPELL_CLEANSING_WATERS);
                                    task.Repeat(Milliseconds(7500));
                                    break;
                                case 1:
                                    BossAI::Talk(SAY_DEATH_3);
                                    DoCast(SPELL_CLAIMING_WATERS);
                                    task.Repeat(Seconds(4));
                                    break;
                                case 2:
                                    DoCast(SPELL_QUIET_SUICIDE);
                                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                                    break;
                            }
                        });
                        task.Schedule(Seconds(4), [this](TaskContext)
                        {
                            DespawnCreature(CorruptedWatersStalkerGUID);
                            DespawnCreatures(FountainStalkerGUIDs);
                        });
                    }
                });
            }

            std::list<Position> GetHydrolanceTopSplashPositions()
            {
                // Values taken from sniffs, helper method (GetAnglesInCone) made for future reference in case people need a similiar function
                uint32 count = 5;
                float degreeDistance = 12.0f;
                float distanceFromCaster = 18.0f;

                std::list<Position> positions;
                for (auto angle : GetAnglesInCone(count, degreeDistance, false))
                {
                    Position pos = me->GetNearPosition(distanceFromCaster, angle);
                    positions.push_back(pos);
                }

                return positions;
            }

            ObjectGuid GetGUID(int32 id) const override
            {
                switch (id)
                {
                    case DATA_SPLASH_STALKER_ROOT:
                        return SplashStalkerSpecialGUID;
                    default:
                        break;
                }

                return NewReset_BossAI::GetGUID(id);
            }

        private:
            GuidList FountainStalkerGUIDs;
            ObjectGuid CorruptedWatersStalkerGUID;
            ObjectGuid SplashStalkerSpecialGUID;
            GuidList SplashStalkerGUIDs;
            TaskScheduler scheduler;
            bool _introDone;
            bool _aggroByWater;
            bool _sayKill;
            bool _sayBelow75, _sayBelow50, _sayBelow25;
            uint32 hydrolanceGroup;
            uint32 _lastHydrolanceDirection;
            void DespawnCreature(ObjectGuid guid)
            {
                if (Creature* creature = ObjectAccessor::GetCreature(*me, guid))
                    creature->DespawnOrUnsummon();
            }
            void DespawnCreatures(GuidList list)
            {
                for (auto guid : list)
                    DespawnCreature(guid);
            }
            std::list<float> GetAnglesInCone(int32 count, float distanceApartInDegrees, bool outputDegrees) // If not degrees, then radians
            {
                std::list<float> angles;
                for (int32 i = 0; i < count; i++)
                {
                    float val = (i - (count / 2))*distanceApartInDegrees;
                    if (count % 2 == 0)
                        val += distanceApartInDegrees / 2;

                    if (!outputDegrees)
                        val *= M_PI / 180;

                    angles.push_back(val);
                }

                return angles;
            }
        };

		CreatureAI* GetAI(Creature* creature) const override
		{
			return new boss_wise_mariAI(creature);
		}
};


// http://www.wowhead.com/npc=65393/east-temple-corrupted-waters-stalker-mw
class npc_corrupted_waters_stalker : public CreatureScript
{
public:
    npc_corrupted_waters_stalker() : CreatureScript("npc_corrupted_waters_stalker") { }

    struct npc_corrupted_waters_stalkerAI : public NewReset_ScriptedAI
    {
        npc_corrupted_waters_stalkerAI(Creature* creature) : NewReset_ScriptedAI(creature)
        {
        }

        void InitializeAI() override
        {
            NewReset_ScriptedAI::InitializeAI();
            FullReset(true);
        }

        void FullReset(bool initialize = false) override
        {
            if (!initialize)
                NewReset_ScriptedAI::FullReset();
        }

        void Reset() override
        {
            _playersInWater.clear();
            scheduler.Schedule(Milliseconds(1200), [this](TaskContext context)
            {
                DoCast(SPELL_CORRUPTED_WATERS);
                context.Repeat(); // Aura never expires, still repeats on retail.
            });
        }

        void UpdateAI(uint32 diff) override
        {
            scheduler.Update(diff);
        }

        void SetUnitInWater(ObjectGuid guid, bool apply)
        {
            if (apply)
                _playersInWater.push_back(guid);
            else
                _playersInWater.remove(guid);
        }

        GuidList GetPlayersInWater() { return _playersInWater; }

        void MoveInLineOfSight(Unit*) override { }
        void AttackStart(Unit*) override { }
        void EnterEvadeMode(EvadeReason /*why*/) override { }
        void OnCharmed(bool /*apply*/) override { }

        static int Permissible(const Creature*) { return PERMIT_BASE_IDLE; }

    private:
        GuidList _playersInWater;
        TaskScheduler scheduler;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_corrupted_waters_stalkerAI(creature);
    }
};

// http://www.wowhead.com/npc=56574/firehose-target
class npc_wise_mari_fire_hose : public CreatureScript
{
public:
    npc_wise_mari_fire_hose() : CreatureScript("npc_wise_mari_fire_hose") { }

    struct npc_wise_mari_fire_hoseAI : public NewReset_ScriptedAI
    {
        npc_wise_mari_fire_hoseAI(Creature* creature) : NewReset_ScriptedAI(creature) { }

        void FullReset(bool initialize = false) override
        {
            NewReset_ScriptedAI::FullReset(initialize);
        }

        void Reset() override
        {
            me->SetSpeedRate(MOVE_WALK, 1.5f);
            me->SetWalk(true);
            if (InstanceScript* instance = me->GetInstanceScript())
                if (Creature* wiseMari = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_WISE_MARI)))
                    me->GetMotionMaster()->MoveCirclePath(wiseMari->GetPositionX(), wiseMari->GetPositionY(), wiseMari->GetPositionZ(), 14.0f, true, 16);
        }

        void MoveInLineOfSight(Unit*) override { }
        void AttackStart(Unit*) override { }
        void UpdateAI(uint32) override { }
        void EnterEvadeMode(EvadeReason /*why*/) override { }
        void OnCharmed(bool /*apply*/) override { }

        static int Permissible(const Creature*) { return PERMIT_BASE_IDLE; }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_wise_mari_fire_hoseAI(creature);
    }
};

// http://www.wowhead.com/spell=115167/corrupted-waters
class spell_wise_mari_corrupted_waters : public SpellScriptLoader
{
public:
    spell_wise_mari_corrupted_waters() : SpellScriptLoader("spell_wise_mari_corrupted_waters") {}

    class spell_wise_mari_corrupted_waters_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_wise_mari_corrupted_waters_SpellScript);

        bool Load() override
        {
            _targets.clear();
            if (Creature* caster = GetCaster()->ToCreature())
            {
                casterAI = dynamic_cast<npc_corrupted_waters_stalker::npc_corrupted_waters_stalkerAI*>(caster->AI());
                if (casterAI)
                    return true;
            }

            return false;
        }

        void FilterTargets(std::list<WorldObject*>& targets)
        {
            targets.clear();
            GuidList players = casterAI->GetPlayersInWater();
            for (auto player : players)
            {
                if (Unit* target = ObjectAccessor::GetUnit(*GetCaster(), player))
                    targets.push_back(target);
            }

            _targets = targets;
        }

        void FilterTargetsCopy(std::list<WorldObject*>& targets)
        {
            targets = _targets;
        }

        void Register()
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_wise_mari_corrupted_waters_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_wise_mari_corrupted_waters_SpellScript::FilterTargetsCopy, EFFECT_1, TARGET_UNIT_SRC_AREA_ENTRY);
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_wise_mari_corrupted_waters_SpellScript::FilterTargetsCopy, EFFECT_2, TARGET_UNIT_SRC_AREA_ENTRY);
        }

    private:
        npc_corrupted_waters_stalker::npc_corrupted_waters_stalkerAI* casterAI;
        std::list<WorldObject*> _targets;
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_wise_mari_corrupted_waters_SpellScript();
    }
};

// http://www.wowhead.com/spell=115229/hydrolance-precast-bottom
class spell_hydrolance_precast_bottom : public SpellScriptLoader
{
public:
    spell_hydrolance_precast_bottom() : SpellScriptLoader("spell_hydrolance_precast_bottom") {}

    class spell_hydrolance_precast_bottom_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_hydrolance_precast_bottom_SpellScript);

        void FilterTargets(std::list<WorldObject*>& targets)
        {
            targets.remove_if([](WorldObject* target)
            {
                return !(target->GetTypeId() == TYPEID_UNIT && target->GetEntry() == NPC_SPLASH_STALKER && target->ToCreature()->GetSpawnId() != GUID_SPLASH_STALKER_ROOT);
            });
        }

        void Register()
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_hydrolance_precast_bottom_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_hydrolance_precast_bottom_SpellScript();
    }
};

// http://www.wowhead.com/spell=115227/hydrolance-precast-top
class spell_hydrolance_precast_top : public SpellScriptLoader
{
public:
    spell_hydrolance_precast_top() : SpellScriptLoader("spell_hydrolance_precast_top") {}

    class spell_hydrolance_precast_top_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_hydrolance_precast_top_SpellScript);

        void HandleDummy(SpellEffIndex /*index*/)
        {
            if (Creature* caster = GetCaster()->ToCreature())
                if (caster->IsAIEnabled)
                {
                    std::list<Position> positions = ENSURE_AI(boss_wise_mari::boss_wise_mariAI, caster->AI())->GetHydrolanceTopSplashPositions();
                    for (auto position : positions)
                    {
                        //std::cout << "\nX: " << position->GetPositionX() << ", Y:" << position->GetPositionY() << ", Z: " << position->GetPositionZ();
                        GetCaster()->CastSpell(position.GetPositionX(), position.GetPositionY(), position.GetPositionZ(), SPELL_HYDROLANCE_PRECAST_TOP_VISUAL, true);
                    }
                }
        }

        void Register()
        {
            OnEffectHit += SpellEffectFn(spell_hydrolance_precast_top_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_hydrolance_precast_top_SpellScript();
    }
};

// http://www.wowhead.com/spell=106250/hydrolance-precast-bottom
class spell_splash_stalker_hydrolance_aura : public SpellScriptLoader
{
public:
    spell_splash_stalker_hydrolance_aura() : SpellScriptLoader("spell_splash_stalker_hydrolance_aura") {}

    class spell_splash_stalker_hydrolance_aura_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_splash_stalker_hydrolance_aura_SpellScript);

        void GetSplashStalker(WorldObject*& target)
        {
            target = nullptr;
            Creature* caster = GetCaster()->ToCreature();
            if (!caster || !caster->IsAIEnabled)
                return;

            if (InstanceScript* instance = GetCaster()->GetInstanceScript())
                if (Creature* creature = ObjectAccessor::GetCreature(*caster, caster->AI()->GetGUID(DATA_SPLASH_STALKER_ROOT)))
                    target = creature;
        }

        void Register()
        {
            OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_splash_stalker_hydrolance_aura_SpellScript::GetSplashStalker, EFFECT_0, TARGET_UNIT_NEARBY_ENTRY);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_splash_stalker_hydrolance_aura_SpellScript();
    }

    class spell_splash_stalker_hydrolance_aura_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_splash_stalker_hydrolance_aura_AuraScript);

        void DummyTick(AuraEffect const* aurEff)
        {
            if (GetCaster()->HasAura(SPELL_WISE_MARI_BOTTOM_HYDROLANCE_AURA))
                return;

            Position pos = GetTarget()->GetNearPosition(aurEff->GetTickNumber() * 2.4f, 0.0f);
            GetTarget()->CastSpell(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), SPELL_SMALL_HYDROLANCE_TICK, true);
        }

        void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            std::list<Creature*> splashStalkers;
            GetCaster()->GetCreatureListWithEntryInGrid(splashStalkers, NPC_SPLASH_STALKER, 50.0f);
            for (auto stalker : splashStalkers)
                stalker->CastSpell((Unit*)NULL, SPELL_LARGE_HYDROLANCE_TICK, true);
        }

        void Register() override
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_splash_stalker_hydrolance_aura_AuraScript::DummyTick, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
            AfterEffectRemove += AuraEffectRemoveFn(spell_splash_stalker_hydrolance_aura_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_splash_stalker_hydrolance_aura_AuraScript();
    }
};

// http://www.wowhead.com/spell=106097/hydrolance // 1.250s
// http://www.wowhead.com/spell=106098/hydrolance // 1.750s
class spell_wise_mari_hydrolance_aura : public SpellScriptLoader
{
public:
    spell_wise_mari_hydrolance_aura() : SpellScriptLoader("spell_wise_mari_hydrolance_aura") { }

    class spell_wise_mari_hydrolance_aura_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_wise_mari_hydrolance_aura_AuraScript);

        void DummyTick(AuraEffect const* aurEff)
        {
            Position pos = GetTarget()->GetNearPosition(1.0f + (aurEff->GetTickNumber() * (m_scriptSpellId == SPELL_WISE_MARI_TOP_HYDROLANCE_AURA ? 2.6f : 2.4f)), 0.0f);
            if (aurEff->GetTickNumber() > 5)
            {
                if (m_scriptSpellId == SPELL_WISE_MARI_TOP_HYDROLANCE_AURA && aurEff->GetTickNumber() == aurEff->GetTotalTicks())
                    GetTarget()->CastSpell(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), SPELL_MEDIUM_HYDROLANCE_TICK, true);
                else if (Creature* target = GetTarget()->ToCreature())
                {
                    if (!target->IsAIEnabled)
                        return;

                    std::list<Position> positions = ENSURE_AI(boss_wise_mari::boss_wise_mariAI, target->AI())->GetHydrolanceTopSplashPositions();
                    for (auto position : positions)
                        GetCaster()->CastSpell(position.GetPositionX(), position.GetPositionY(), position.GetPositionZ(), SPELL_MEDIUM_HYDROLANCE_TICK, true);
                }

            }
            else
                GetTarget()->CastSpell(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), SPELL_SMALL_HYDROLANCE_TICK, true);
        }

        void Register() override
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_wise_mari_hydrolance_aura_AuraScript::DummyTick, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_wise_mari_hydrolance_aura_AuraScript();
    }
};

// http://www.wowhead.com/spell=106519/corrupt-fountain
class spell_corrupt_fountain : public SpellScriptLoader
{
public:
    spell_corrupt_fountain() : SpellScriptLoader("spell_corrupt_fountain") {}

    class spell_corrupt_fountain_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_corrupt_fountain_SpellScript);

        void HandleScript(SpellEffIndex /*effIndex*/)
        {
            GetHitUnit()->CastSpell(GetHitUnit(), SPELL_CORRUPTED_FOUNTAIN, true);
        }

        void FilterTargets(std::list<WorldObject*>& targets)
        {
            targets.remove_if([](WorldObject* target)
            {
                return !(target->GetTypeId() == TYPEID_UNIT && target->GetEntry() == NPC_FOUNTAIN_STALKER);
            });
        }

        void Register()
        {
            OnEffectHitTarget += SpellEffectFn(spell_corrupt_fountain_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_DUMMY);
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_corrupt_fountain_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_corrupt_fountain_SpellScript();
    }
};

// http://www.wowhead.com/spell=115575/wash-away
class spell_wise_mari_wash_away : public SpellScriptLoader
{
public:
    spell_wise_mari_wash_away() : SpellScriptLoader("spell_wise_mari_wash_away") {}

    class spell_wise_mari_wash_away_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_wise_mari_wash_away_SpellScript);

        void GetFireHose(WorldObject*& target)
        {
            target = nullptr;
            if (InstanceScript* instance = GetCaster()->GetInstanceScript())
            {
                target = ObjectAccessor::GetCreature(*GetCaster(), instance->GetGuidData(DATA_FIRE_HOSE));
                GetCaster()->Say("Target: " + target->GetName(), LANG_UNIVERSAL);
            }
        }

        void Register()
        {
            OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_wise_mari_wash_away_SpellScript::GetFireHose, EFFECT_0, TARGET_UNIT_NEARBY_ENTRY);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_wise_mari_wash_away_SpellScript();
    }
};

// 7084
class at_wise_mari_intro : public AreaTriggerScript
{
    public:
        at_wise_mari_intro() : AreaTriggerScript("at_wise_mari_intro") { }

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*areaTrigger*/, bool /*entered*/) override
        {
            if (player->IsGameMaster())
                return true;

            if (InstanceScript* instance = player->GetInstanceScript())
                if (Creature* wiseMari = ObjectAccessor::GetCreature(*player, instance->GetGuidData(DATA_WISE_MARI)))
                    if (wiseMari->IsAIEnabled)
                        wiseMari->GetAI()->DoAction(ACTION_INTRO);

            return true;
        }
};

// 7085, 7708, 8538, 8539, 8540
class at_wise_mari_waters : public AreaTriggerScript
{
public:
    at_wise_mari_waters() : AreaTriggerScript("at_wise_mari_waters") { }

    bool OnTrigger(Player* player, AreaTriggerEntry const* /*areaTrigger*/, bool entered) override
    {
        if (player->IsGameMaster())
            return true;

        if (InstanceScript* instance = player->GetInstanceScript())
        {
            if (Creature* wiseMari = ObjectAccessor::GetCreature(*player, instance->GetGuidData(DATA_WISE_MARI)))
                if (wiseMari->IsAIEnabled)
                    wiseMari->GetAI()->DoAction(ACTION_AT_WATERS);

            if (Creature* stalker = ObjectAccessor::GetCreature(*player, instance->GetGuidData(DATA_CORRUPTED_WATERS_STALKER)))
                if (npc_corrupted_waters_stalker::npc_corrupted_waters_stalkerAI* ai = ENSURE_AI(npc_corrupted_waters_stalker::npc_corrupted_waters_stalkerAI, stalker->AI()))
                    ai->SetUnitInWater(player->GetGUID(), entered);
        }


        return true;
    }
};

void AddSC_boss_wise_mari()
{
    new boss_wise_mari();
    new npc_corrupted_waters_stalker();
    new npc_wise_mari_fire_hose();

    new spell_wise_mari_corrupted_waters();
    new spell_hydrolance_precast_bottom();
    new spell_hydrolance_precast_top();
    new spell_splash_stalker_hydrolance_aura();
    new spell_wise_mari_hydrolance_aura();
    new spell_corrupt_fountain();
    new spell_wise_mari_wash_away();

    new at_wise_mari_intro();
    new at_wise_mari_waters();
}
