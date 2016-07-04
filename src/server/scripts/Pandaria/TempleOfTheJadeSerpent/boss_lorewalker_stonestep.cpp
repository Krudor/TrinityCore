/* ScriptData
SDName: boss_lorewalker_stonestep
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
#include "GridNotifiers.h"

enum Spells
{
    SPELL_PERMANENT_FEIGN_DEATH     = 29266,
    SPELL_SHA_CORRUPTION            = 107034,

    // Corrupted Scroll
    SPELL_FLOOR_SCROLL              = 107350,
    SPELL_JADE_ENERGY               = 111452,
    SPELL_DUMMY_NUKE                = 105898,
    SPELL_DRAW_SHA                  = 111393,
    SPELL_SHA_BURNING               = 111588,
    SPELL_SHA_EXPLOSION             = 111579,
    SPELL_DEATH                     = 98391,

    // Initially hidden encounter creatures
    SPELL_SHRINK                    = 43350,
    SPELL_INVISIBLE_MAN_TRANSFORM   = 105581,

    // Osung
    SPELL_COSMETIC_FIGHTING         = 120287,
    SPELL_SHA_CORRUPTION_OSUNG      = 123947,
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
    SAY_AREATRIGGER_1   = 0,
    SAY_AREATRIGGER_2   = 1,
    SAY_AREATRIGGER_3   = 2,
    SAY_AREATRIGGER_4   = 3
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

	PHASE_MASK_INTRO	= 1 << PHASE_INTRO,
    PHASE_MASK_ONE		= 1 << PHASE_ONE,
    PHASE_MASK_TWO		= 1 << PHASE_TWO,
};

enum Points
{
};

enum AnimKits
{
};

enum Areatriggers
{
    AREATRIGGER_LOREWALKER_1        = 7128,
    AREATRIGGER_LOREWALKER_2        = 7129,
    AREATRIGGER_LOREWALKER_3        = 7130,
    AREATRIGGER_LOREWALKER_4        = 7131,
};

enum Data
{
    DATA_AREATRIGGER_1,
    DATA_AREATRIGGER_2,
    DATA_AREATRIGGER_3,
    DATA_AREATRIGGER_4,
    DATA_AREATRIGGERS_MAX,
};

enum ScriptGuids
{
    DEAD_HEROINE_1      = 454781,
    DEAD_HEROINE_2      = 454741,
    DEAD_HEROINE_3      = 454764,
    ALIVE_HEROINE       = 454816,

    GUID_OSUNG_PASSIVE  = 454799,
    GUID_OSUNG          = 454754,
};

// http://www.wowhead.com/npc=56843/lorewalker-stonestep
class boss_lorewalker_stonestep : public CreatureScript
{
    public:
        boss_lorewalker_stonestep() : CreatureScript("boss_lorewalker_stonestep") {}

        struct boss_lorewalker_stonestepAI : public NewReset_BossAI
        {
            boss_lorewalker_stonestepAI(Creature* creature) : NewReset_BossAI(creature, BOSS_LOREWALKER_STONESTEP) { }

            void InitializeAI() override
            {
                for (uint32 i = 0; i < DATA_AREATRIGGERS_MAX; i++)
                    triggeredAreatriggers[i] = false;
            }

            void FullReset(bool initialize = false) override
            {
                triggeredAreatriggers.clear();
                for (uint32 i = 0; i < DATA_AREATRIGGERS_MAX; i++)
                    triggeredAreatriggers[i] = false;
                NewReset_BossAI::FullReset(initialize);
            }

            void SetGUID(ObjectGuid guid, int32 type = 0) override
            {
                std::map<int32, bool>::iterator itr = triggeredAreatriggers.find(type);
                if (itr == triggeredAreatriggers.end())
                    return;

                bool triggered = (*itr).second;
                if (triggered)
                    return;

                triggeredAreatriggers[type] = true;

                Player* triggeringPlayer = ObjectAccessor::GetPlayer(*me, guid);
                if (!triggeringPlayer)
                    return;

                switch (type)
                {
                    case DATA_AREATRIGGER_1:
                        BossAI::Talk(SAY_AREATRIGGER_1, triggeringPlayer);
                        break;
                    case DATA_AREATRIGGER_2:
                        BossAI::Talk(SAY_AREATRIGGER_2, triggeringPlayer);
                        break;
                    case DATA_AREATRIGGER_3:
                        BossAI::Talk(SAY_AREATRIGGER_3, triggeringPlayer);
                        break;
                    case DATA_AREATRIGGER_4:
                        BossAI::Talk(SAY_AREATRIGGER_4, triggeringPlayer);
                        break;
                }
            }

        private:
            std::map<int32, bool> triggeredAreatriggers;
        };

		CreatureAI* GetAI(Creature* creature) const override
		{
			return new boss_lorewalker_stonestepAI(creature);
		}
};

// http://www.wowhead.com/npc=56873/heroine
class npc_heroine_cosmetic : public CreatureScript
{
public:
    npc_heroine_cosmetic() : CreatureScript("npc_heroine_cosmetic") { }

    struct npc_heroine_cosmeticAI : public NewReset_ScriptedAI
    {
        npc_heroine_cosmeticAI(Creature* creature) : NewReset_ScriptedAI(creature) { }

        void FullReset(bool initialize = false) override
        {
            NewReset_ScriptedAI::FullReset(initialize);
        }

        void Reset() override
        {
            me->AddAura(SPELL_SHA_CORRUPTION, me);
            switch (me->GetSpawnId())
            {
                case DEAD_HEROINE_1:
                case DEAD_HEROINE_2:
                case DEAD_HEROINE_3:
                    me->AddAura(SPELL_PERMANENT_FEIGN_DEATH, me);
                    break;
                case ALIVE_HEROINE:
                    scheduler.Schedule(Milliseconds(1600), [this](TaskContext context)
                    {
                        me->HandleEmoteCommand(EMOTE_ONESHOT_CRY);
                        context.Schedule(Milliseconds(8500), [this](TaskContext context)
                        {
                            me->HandleEmoteCommand(EMOTE_ONESHOT_CRY);
                            context.Repeat();
                        });
                        context.Schedule(Milliseconds(3650), [this](TaskContext context)
                        {
                            DoCast(SPELL_SHA_CORRUPTION);
                            me->HandleEmoteCommand(EMOTE_ONESHOT_BEG);
                            context.Repeat(Milliseconds(8500));
                        });
                    });
                    break;
            }
        }

        void MoveInLineOfSight(Unit*) override { }
        void AttackStart(Unit*) override { }
        void UpdateAI(uint32 diff) override
        {
            scheduler.Update(diff);
        }
        void EnterEvadeMode(EvadeReason /*why*/) override { }
        void OnCharmed(bool /*apply*/) override { }

        static int Permissible(const Creature*) { return PERMIT_BASE_IDLE; }

    private:
        TaskScheduler scheduler;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_heroine_cosmeticAI(creature);
    }
};

// http://www.wowhead.com/npc=57080/corrupted-scroll
class npc_corrupted_scroll : public CreatureScript
{
public:
    npc_corrupted_scroll() : CreatureScript("npc_corrupted_scroll") { }

    struct npc_corrupted_scrollAI : public NewReset_ScriptedAI
    {
        npc_corrupted_scrollAI(Creature* creature) : NewReset_ScriptedAI(creature) { }

        void FullReset(bool initialize = false) override
        {
            NewReset_ScriptedAI::FullReset(initialize);
        }
        
        void DamageTaken(Unit* attacker, uint32& damage) override
        {
            if (damage > me->GetHealth())
                damage = me->GetHealth() - 1;
            else
                return;

            if (_popped)
                return;

            _popped = true;
            scheduler.Schedule(Seconds(2), [this](TaskContext context)
            {
                DoCastAOE(SPELL_DRAW_SHA);
                DoCast(SPELL_SHA_BURNING);
                me->RemoveAurasDueToSpell(SPELL_FLOOR_SCROLL);
                context.Schedule(Seconds(5), [this](TaskContext context)
                {
                    DoCast(SPELL_SHA_EXPLOSION);
                    DoCast(SPELL_DRAW_SHA);
                    DoCast(SPELL_DEATH);
                });
            });
        }

        void SpellHit(Unit* caster, SpellInfo const* spell) override
        {
            if (spell->Id != SPELL_DEATH)
                return;

            me->Say("FUCK", LANG_UNIVERSAL);
            me->SetVisible(false);
        }

        void Reset() override
        {
            _popped = false;
            DoCast(SPELL_FLOOR_SCROLL);
        }

        void MoveInLineOfSight(Unit*) override { }
        void AttackStart(Unit*) override { }
        void UpdateAI(uint32 diff) override
        {
            scheduler.Update(diff);
        }
        void EnterEvadeMode(EvadeReason /*why*/) override { }
        void OnCharmed(bool /*apply*/) override { }

        static int Permissible(const Creature*) { return PERMIT_BASE_IDLE; }

    private:
        bool _popped;
        TaskScheduler scheduler;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_corrupted_scrollAI(creature);
    }
};

struct libraryHiddenNpcAI : public NewReset_ScriptedAI
{
    libraryHiddenNpcAI(Creature* creature) : NewReset_ScriptedAI(creature) { }

    void FullReset(bool initialize = false) override
    {
        NewReset_ScriptedAI::FullReset(initialize);
    }

    void Reset() override
    {
        me->AddAura(SPELL_SHRINK, me);
        me->AddAura(SPELL_INVISIBLE_MAN_TRANSFORM, me);
        NewReset_ScriptedAI::Reset();
    }
};

// http://www.wowhead.com/npc=56872/osong
class npc_lorewalker_stonestep_osong : public CreatureScript
{
public:
    npc_lorewalker_stonestep_osong() : CreatureScript("npc_lorewalker_stonestep_osong") { }

    struct npc_lorewalker_stonestep_osongAI : public libraryHiddenNpcAI
    {
        npc_lorewalker_stonestep_osongAI(Creature* creature) : libraryHiddenNpcAI(creature) { }

        void Reset() override
        {
            me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_READY2HL);
            switch (me->GetSpawnId())
            {
                case GUID_OSUNG:
                    me->Say("BOO", LANG_UNIVERSAL);
                    libraryHiddenNpcAI::Reset();
                    me->AddAura(SPELL_COSMETIC_FIGHTING, me);
                    me->AddAura(SPELL_SHA_CORRUPTION_OSUNG, me);
                    break;
                case GUID_OSUNG_PASSIVE:
                    me->Say("BEE", LANG_UNIVERSAL);
                    me->AddAura(SPELL_SHA_CORRUPTION, me);
                    scheduler.Schedule(Milliseconds(1200), [this](TaskContext context)
                    {
                        DoCast(SPELL_SHA_CORRUPTION);
                        me->HandleEmoteCommand(EMOTE_ONESHOT_ATTACK2H_LOOSE);
                        scheduler.Schedule(Milliseconds(1200), [this](TaskContext)
                        {
                            me->HandleEmoteCommand(EMOTE_ONESHOT_ATTACK2H_LOOSE);
                        });
                        scheduler.Schedule(Milliseconds(2400), [this](TaskContext)
                        {
                            me->HandleEmoteCommand(EMOTE_ONESHOT_ATTACK2HTIGHT);
                        });
                        context.Repeat(Milliseconds(3600));
                    });
                    break;
            }
        }

        void SpellHit(Unit* caster, const SpellInfo* spell) override
        {
            if (spell->Id != SPELL_DRAW_SHA)
                return;

            scheduler.CancelAll();
        }

        void UpdateAI(uint32 diff) override
        {
            libraryHiddenNpcAI::UpdateAI(diff);

            scheduler.Update(diff);
        }

    private:
        TaskScheduler scheduler;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_lorewalker_stonestep_osongAI(creature);
    }
};

// http://www.wowhead.com/spell=111393/draw-sha
class spell_corrupted_scroll_draw_sha : public SpellScriptLoader
{
public:
    spell_corrupted_scroll_draw_sha() : SpellScriptLoader("spell_corrupted_scroll_draw_sha") {}

    class spell_corrupted_scroll_draw_sha_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_corrupted_scroll_draw_sha_SpellScript);

        void HandleScript(SpellEffIndex /*effIndex*/)
        {
            GetHitUnit()->CastSpell(GetCaster(), GetEffectValue(), true);
        }

        void FilterShaTargets(std::list<WorldObject*>& targets)
        {
            targets.remove_if(Trinity::UnitAuraCheck(false, SPELL_SHA_CORRUPTION));
        }

        // No idea what the purpose of this effect is, but it does hit the player.
        //void FilterPlayers(std::list<WorldObject*>& targets)
        //{
        //    targets.remove_if(Trinity::ObjectTypeIdCheck(TYPEID_PLAYER, false));
        //}

        void Register()
        {
            OnEffectHitTarget += SpellEffectFn(spell_corrupted_scroll_draw_sha_SpellScript::HandleScript, EFFECT_1, SPELL_EFFECT_SCRIPT_EFFECT);
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_corrupted_scroll_draw_sha_SpellScript::FilterShaTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_corrupted_scroll_draw_sha_SpellScript::FilterShaTargets, EFFECT_1, TARGET_UNIT_SRC_AREA_ENTRY);
            //OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_corrupted_scroll_draw_sha_SpellScript::FilterPlayers, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_corrupted_scroll_draw_sha_SpellScript();
    }
};

// 7128, 7129, 7130, 7131
class at_lorewalker_stonestep_library : public AreaTriggerScript
{
public:
    at_lorewalker_stonestep_library() : AreaTriggerScript("at_lorewalker_stonestep_library") { }

    bool OnTrigger(Player* player, AreaTriggerEntry const* areaTrigger, bool entered) override
    {
        if (player->IsGameMaster())
            return true;

        if (InstanceScript* instance = player->GetInstanceScript())
        {
            if (Creature* stonestep = ObjectAccessor::GetCreature(*player, instance->GetGuidData(DATA_LOREWALKER_STONESTEP)))
            {
                if (!stonestep->IsAIEnabled)
                    return true;

                int32 data = NULL;
                switch (areaTrigger->ID)
                {
                    case AREATRIGGER_LOREWALKER_1:
                        data = DATA_AREATRIGGER_1;
                        break;
                    case AREATRIGGER_LOREWALKER_2:
                        data = DATA_AREATRIGGER_2;
                        break;
                    case AREATRIGGER_LOREWALKER_3:
                        data = DATA_AREATRIGGER_3;
                        break;
                    case AREATRIGGER_LOREWALKER_4:
                        data = DATA_AREATRIGGER_4;
                        break;
                    default:
                        break;
                }

                stonestep->AI()->SetGUID(player->GetGUID(), data);
            }
        }

        return true;
    }
};

void AddSC_boss_lorewalker_stonestep()
{
    new boss_lorewalker_stonestep();
    new npc_heroine_cosmetic();
    new npc_corrupted_scroll();
    new npc_lorewalker_stonestep_osong();

    new spell_corrupted_scroll_draw_sha();

    new at_lorewalker_stonestep_library();
}
