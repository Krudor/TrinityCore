/* ScriptData
SDName: boss_liu_flameheart
SD%Complete: 0
SDComment: Placeholder
SDCategory: Temple of the Jade Serpent
EndScriptData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "temple_of_the_jade_serpent.h"
#include "PassiveAI.h"

enum Spells
{
    SPELL_POSSESED_BY_SHA       = 110164,
    
    // Probably triggered by a server-side jump spell, haven't found it yet though.
    SPELL_DUST_VISUAL           = 110518,
    SPELL_JADE_ESSENCE          = 106797,
    SPELL_SUMMON_JADE_SERPENT   = 106895,
    SPELL_JADE_SOUL             = 106909,
    SPELL_SUICIDE               = 43388,

    // Yu'lon
    SPELL_JADE_SERPENT_HEALTH   = 106924,
    SPELL_SHARED_HEALTH         = 114711,
    SPELL_SHRINK                = 92078,
    SPELL_TRANSFORM_VISUAL      = 74620,
    SPELL_CLEANSING_BREATH      = 132387,
};

enum Events
{
    EVENT_NONE,
};

enum Actions
{
	ACTION_NONE,
};

enum Texts
{
    SAY_INTRO           = 0,
    SAY_AGGRO           = 1,
    SAY_PHASE_2         = 2,
    SAY_PHASE_3         = 3,
    SAY_KILL            = 4,
    SAY_DEATH           = 5,

    SAY_YULON_DEFEAT    = 0,
};

enum CreatureGroups
{
};

enum Phases
{
    PHASE_NONE,
	PHASE_INTRO,
    PHASE_ONE,
    PHASE_TWO,
    PHASE_THREE,

	PHASE_MASK_INTRO	= 1 << PHASE_INTRO,
    PHASE_MASK_ONE		= 1 << PHASE_ONE,
    PHASE_MASK_TWO		= 1 << PHASE_TWO,
    PHASE_MASK_THREE    = 1 << PHASE_THREE,
};

enum AnimKits
{
};

enum Points
{
    POINT_LANDING,
    POINT_PHASE_3,
    POINT_YULON_DEFEAT,
    POINT_YULON_OUT
};

Position const CenterPosition = { 930.005f, -2560.36f, 180.069f, 1.258908f };
Position const YulonDefeatPosition = { 926.828f, -2571.02f, 180.0731f, 1.27409f };

uint32 const YulonPathSize = 3;
G3D::Vector3 const YulonPath[YulonPathSize] =
{
    { 938.1285f, -2542.773f, 226.7539f },
    { 965.3004f, -2489.177f, 256.0829f },
    { 978.6389f, -2419.773f, 299.6773f }
};

class boss_liu_flameheart : public CreatureScript
{
    public:
        boss_liu_flameheart() : CreatureScript("boss_liu_flameheart") {}

        struct boss_liu_flameheartAI : public NewReset_BossAI
        {
            boss_liu_flameheartAI(Creature* creature) : NewReset_BossAI(creature, BOSS_LIU_FLAMEHEART), _onGround(false){ }

            void Reset() override
            {
                DoCast(SPELL_POSSESED_BY_SHA);
                me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_READY_UNARMED);
                me->SetReactState(_onGround ? REACT_AGGRESSIVE : REACT_DEFENSIVE);
            }

            void JustSummoned(Creature* summon) override
            {
                switch (summon->GetEntry())
                {
                    case NPC_YULON:
                        DoCast(SPELL_JADE_SOUL);
                        break;
                    default:
                        break;
                }
            }

            void EnterCombat(Unit* who) override
            {
                BossAI::Talk(SAY_AGGRO);
                me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_ONESHOT_NONE);
				events.SetPhase(PHASE_ONE);
                scheduler.Schedule(Seconds(1), [this](TaskContext context) // Blizzard doesn't like using damage taken hook
                {
                    if (me->HealthBelowPct(70 + 1) && events.IsInPhase(PHASE_ONE))
                        PreparePhase(PHASE_TWO);
                    else if (me->HealthBelowPct(30 + 1) && events.IsInPhase(PHASE_TWO))
                        PreparePhase(PHASE_THREE);
					context.Repeat();
                });
            }

            void JustDied(Unit* killer) override
            {
                BossAI::Talk(SAY_DEATH);
                scheduler.CancelAll();
                instance->SetBossState(DATA_LIU_FLAMEHEART, DONE);
            }

            void KilledUnit(Unit* victim) override
            {
                if (victim->GetTypeId() == TYPEID_PLAYER)
                    Talk(SAY_KILL, victim, Seconds(8));
            }

            void EnterEvadeMode(EvadeReason why) override
            {
                scheduler.CancelAll();
                NewReset_BossAI::EnterEvadeMode(why);
            }

            void DamageTaken(Unit* source, uint32& damage) override
            {
                if (damage > me->GetHealth())
                    damage = me->GetHealth() - 1;
            }

            void SpellHit(Unit* source, const SpellInfo* spell) override
            {
                switch (spell->Id)
                {
                    case SPELL_CLEANSING_BREATH:
                        me->CastStop();
                        scheduler.Schedule(Milliseconds(1200), [this](TaskContext context)
                        {
                            DoCast(SPELL_SUICIDE);
                        });
                        break;
                }
            }

            void PreparePhase(Phases phase)
            {
				events.SetPhase(phase);

                switch (phase)
                {
                    case PHASE_ONE:
                        break;
                    case PHASE_TWO:
                        BossAI::Talk(SAY_PHASE_2);
                        break;
                    case PHASE_THREE:
                        BossAI::Talk(SAY_PHASE_3);
                        DoStopAttack();
                        me->SetReactState(REACT_PASSIVE);
                        me->RemoveAurasDueToSpell(SPELL_JADE_ESSENCE);
                        me->GetMotionMaster()->MovePoint(POINT_PHASE_3, me->GetHomePosition());
                        break;
                    default:
                        break;
                }
            }

            void MovementInform(uint32 type, uint32 id) override
            {
                if (type == EFFECT_MOTION_TYPE && id == POINT_LANDING)
                {
                    BossAI::Talk(SAY_INTRO);
                    DoCastAOE(SPELL_DUST_VISUAL);
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                    me->SetHomePosition(me->GetPosition());
				}
                else if (type == POINT_MOTION_TYPE && id == POINT_PHASE_3)
                {
					_onGround = true;
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                    me->SetOrientation(me->GetHomePosition().GetOrientation() + M_PI);
                    me->SetFacingTo(me->GetHomePosition().GetOrientation() + M_PI);
                    DoCast(SPELL_SUMMON_JADE_SERPENT);
                }
            }

            void DoAction(int32 action) override
            {
                switch (action)
                {
                    case ACTION_TERRACE_LIU_FLAMEHEART_ENTRANCE:
                        me->SetReactState(REACT_DEFENSIVE);

                        // Probably hack, pretty sure it's triggered by server-side spell but haven't found it.
                        me->GetMotionMaster()->MoveJump(CenterPosition, 25, 25, POINT_LANDING);
                        break;
                    default:
                        break;
                }

                NewReset_BossAI::DoAction(action);
            }

			void UpdateAI(uint32 diff) override
			{
				NewReset_BossAI::UpdateAI(diff);
				scheduler.Update(diff);
			}

            private:
                bool _onGround;
        };

		CreatureAI* GetAI(Creature* creature) const override
		{
			return new boss_liu_flameheartAI(creature);
		}
};

// http://www.wowhead.com/npc=56762/yulon
class npc_liu_flameheart_yulon : public CreatureScript
{
public:
    npc_liu_flameheart_yulon() : CreatureScript("npc_liu_flameheart_yulon") { }

    struct npc_liu_flameheart_yulonAI : public NewReset_ScriptedAI
    {
        npc_liu_flameheart_yulonAI(Creature* creature) : NewReset_ScriptedAI(creature)
        {
            creature->SetReactState(REACT_PASSIVE);
        }

        void IsSummonedBy(Unit* summoner) override
        {
            DoCast(SPELL_JADE_SERPENT_HEALTH); // We don't really need this...
            me->SetHealth(std::max((uint32)1, CalculatePct(me->GetMaxHealth(), summoner->GetHealthPct())));

            DoCast(SPELL_SHARED_HEALTH);

            if (InstanceScript* instance = me->GetInstanceScript())
                instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);

            scheduler.Schedule(Milliseconds(1200), [this, summoner](TaskContext context)
            {
                if (!summoner)
                {
                    me->DespawnOrUnsummon();
                    return;
                }

                me->SetReactState(REACT_AGGRESSIVE);
                if (Creature* boss = summoner->ToCreature())
                    if (boss->IsAIEnabled)
                        boss->AI()->DoZoneInCombat(me);
            });
        }

        void EnterCombat(Unit* who) override
        {
            scheduler.Schedule(Seconds(1), [this](TaskContext context)
            {
                if (me->HealthBelowPct(1) && me->GetReactState() == REACT_AGGRESSIVE)
                {
                    DoStopAttack();
                    me->SetReactState(REACT_PASSIVE);
                    scheduler.CancelAll();
                    me->GetMotionMaster()->MovePoint(POINT_YULON_DEFEAT, YulonDefeatPosition);
                }

				context.Repeat();
            });
            scheduler.Schedule(Milliseconds(1200), [this](TaskContext context)
            {
                me->RemoveAurasDueToSpell(SPELL_SHRINK);
            });
        }

        void DamageTaken(Unit* source, uint32& damage) override
        {
            if (damage > me->GetHealth())
                damage = me->GetHealth() - 1;
        }

        //void SpellHitTarget(Unit* target, const SpellInfo* spell) override
        //{
        //    if (spell->Id != SPELL_CLEANSING_BREATH)
        //        return;

        //    scheduler.Schedule(Milliseconds(2400), [this](TaskContext context)
        //    {
        //        // Needs flying flag
        //        me->GetMotionMaster()->MoveSmoothPath(POINT_YULON_OUT, YulonPath, YulonPathSize, true);
        //        if (InstanceScript* instance = me->GetInstanceScript())
        //            instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
        //    }); // Blizzard really seems to like using and multiplying 1.2 seconds instead of 1 flat second.
        //}

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type != POINT_MOTION_TYPE)
                return;

            switch (id)
            {
                case POINT_YULON_DEFEAT:
                    me->SetOrientation(YulonDefeatPosition.GetOrientation());
                    me->SetFacingTo(YulonDefeatPosition.GetOrientation());
                    Talk(SAY_YULON_DEFEAT);
                    scheduler.Schedule(Milliseconds(1200), [this](TaskContext context)
                    {
                        DoCast(SPELL_CLEANSING_BREATH);
                    });
                    break;
            }
        }

        void UpdateAI(uint32 diff) override
        {
            scheduler.Update(diff);
            NewReset_ScriptedAI::UpdateAI(diff);
        }

    private:
        TaskScheduler scheduler;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_liu_flameheart_yulonAI(creature);
    }
};

// http://www.wowhead.com/spell=106909/jade-soul
class spell_liu_flameheart_jade_soul : public SpellScriptLoader
{
public:
    spell_liu_flameheart_jade_soul() : SpellScriptLoader("spell_liu_flameheart_jade_soul") {}

    class spell_liu_flameheart_jade_soul_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_liu_flameheart_jade_soul_SpellScript);

        void GetYulon(WorldObject*& target)
        {
            target = nullptr;
            if (InstanceScript* instance = GetCaster()->GetInstanceScript())
                target = ObjectAccessor::GetCreature(*GetCaster(), instance->GetGuidData(DATA_YULON));
        }

        void Register()
        {
            OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_liu_flameheart_jade_soul_SpellScript::GetYulon, EFFECT_0, TARGET_UNIT_NEARBY_ENTRY);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_liu_flameheart_jade_soul_SpellScript();
    }
};

// http://www.wowhead.com/spell=114711/shared-health
class spell_liu_flameheart_shared_health : public SpellScriptLoader
{
public:
    spell_liu_flameheart_shared_health() : SpellScriptLoader("spell_liu_flameheart_shared_health") {}

    class spell_liu_flameheart_shared_health_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_liu_flameheart_shared_health_SpellScript);

        void GetLiu(WorldObject*& target)
        {
            target = nullptr;
            if (InstanceScript* instance = GetCaster()->GetInstanceScript())
                target = ObjectAccessor::GetCreature(*GetCaster(), instance->GetGuidData(DATA_LIU_FLAMEHEART));
        }

        void Register()
        {
            OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_liu_flameheart_shared_health_SpellScript::GetLiu, EFFECT_0, TARGET_UNIT_NEARBY_ENTRY);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_liu_flameheart_shared_health_SpellScript();
    }
};

void AddSC_boss_liu_flameheart()
{
    new boss_liu_flameheart();
    new npc_liu_flameheart_yulon();

    new spell_liu_flameheart_jade_soul();
    new spell_liu_flameheart_shared_health();
}
