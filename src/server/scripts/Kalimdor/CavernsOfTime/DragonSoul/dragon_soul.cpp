#include "ScriptMgr.h"
#include "dragon_soul.h"
#include "ScriptedEscortAI.h"
#include "PassiveAI.h"
#include "PhasingHandler.h"
#include "GameObject.h"

enum Events
{
    EVENT_CHANNEL_DS_GUNSHIP
};

enum Spells
{
    SPELL_YSERA_CHARGE_DS_GUNSHIP       = 108833,
    SPELL_KALECGOS_CHARGE_DS_GUNSHIP    = 108836,
    SPELL_ALEXSTRASZA_CHARGE_DS_GUNSHIP = 108837,
    SPELL_NOZDORMU_CHARGE_DS_GUNSHIP    = 108838,
};

// http://www.wowhead.com/npc=56630
class npc_ds_alexstrasza_56630 : public CreatureScript
{
public:
    npc_ds_alexstrasza_56630() : CreatureScript("npc_ds_alexstrasza_56630") {}

    struct npc_ds_alexstrasza_56630AI : public ScriptedAI
    {
        npc_ds_alexstrasza_56630AI(Creature* creature) : ScriptedAI(creature), instance(creature->GetInstanceScript()) { }

        void InitializeAI() override
        {
            JustRespawned();
        }

        void EnterEvadeMode(EvadeReason reason) override
        {
            events.Reset();

            uint32 corpseDelay = me->GetCorpseDelay();
            uint32 respawnDelay = me->GetRespawnDelay();

            me->SetCorpseDelay(1);
            me->SetRespawnDelay(29);

            me->DespawnOrUnsummon();

            me->SetCorpseDelay(corpseDelay);
            me->SetRespawnDelay(respawnDelay);

            Position home = me->GetHomePosition();
            me->NearTeleportTo(home.GetPositionX(), home.GetPositionY(), home.GetPositionZ(), home.GetOrientation());
        }

        void JustRespawned() override
        {
            if (me->GetAreaId() == AREA_ABOVE_THE_FROZEN_SEA)
            {
                DoCastAOE(SPELL_ALEXSTRASZA_CHARGE_DS_GUNSHIP);
                events.ScheduleEvent(EVENT_CHANNEL_DS_GUNSHIP, 1 * IN_MILLISECONDS);
            }

            CreatureAI::JustRespawned();
        }

        /*void MovementInform(uint32 type, uint32 id) override
        {
        }*/

        void UpdateAI(uint32 diff) override
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_CHANNEL_DS_GUNSHIP:
                    if (!me->HasUnitState(UNIT_STATE_CASTING))
                        DoCastAOE(SPELL_ALEXSTRASZA_CHARGE_DS_GUNSHIP);
                    events.ScheduleEvent(EVENT_CHANNEL_DS_GUNSHIP, 1 * IN_MILLISECONDS);
                    break;
                default:
                    break;
                }
            }
        }

    private:
        EventMap events;
        InstanceScript* instance;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_ds_alexstrasza_56630AI(creature);
    }
};

// http://www.wowhead.com/npc=56664
class npc_ds_kalecgos_56664 : public CreatureScript
{
    public:
        npc_ds_kalecgos_56664() : CreatureScript("npc_ds_kalecgos_56664") {}

        struct npc_ds_kalecgos_56664AI : public ScriptedAI
        {
            npc_ds_kalecgos_56664AI(Creature* creature) : ScriptedAI(creature), instance(creature->GetInstanceScript()) { }

            void InitializeAI() override
            {
                JustRespawned();
            }

            void EnterEvadeMode(EvadeReason reason) override
            {
                events.Reset();

                uint32 corpseDelay = me->GetCorpseDelay();
                uint32 respawnDelay = me->GetRespawnDelay();

                me->SetCorpseDelay(1);
                me->SetRespawnDelay(29);

                me->DespawnOrUnsummon();

                me->SetCorpseDelay(corpseDelay);
                me->SetRespawnDelay(respawnDelay);

                Position home = me->GetHomePosition();
                me->NearTeleportTo(home.GetPositionX(), home.GetPositionY(), home.GetPositionZ(), home.GetOrientation());
            }

            void JustRespawned() override
            {
                if (me->GetAreaId() == AREA_ABOVE_THE_FROZEN_SEA)
                {
                    DoCastAOE(SPELL_KALECGOS_CHARGE_DS_GUNSHIP);
                    events.ScheduleEvent(EVENT_CHANNEL_DS_GUNSHIP, 1 * IN_MILLISECONDS);
                }

                CreatureAI::JustRespawned();
            }

            /*void MovementInform(uint32 type, uint32 id) override
            {
            }*/

            void UpdateAI(uint32 diff) override
            {
                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case EVENT_CHANNEL_DS_GUNSHIP:
                        if (!me->HasUnitState(UNIT_STATE_CASTING))
                            DoCastAOE(SPELL_KALECGOS_CHARGE_DS_GUNSHIP);
                        events.ScheduleEvent(EVENT_CHANNEL_DS_GUNSHIP, 1 * IN_MILLISECONDS);
                        break;
                    default:
                        break;
                    }
                }
            }

        private:
            EventMap events;
            InstanceScript* instance;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_ds_kalecgos_56664AI(creature);
        }
};

// http://www.wowhead.com/npc=56665
class npc_ds_ysera_56665 : public CreatureScript
{
    public:
        npc_ds_ysera_56665() : CreatureScript("npc_ds_ysera_56665") {}

        struct npc_ds_ysera_56665AI : public ScriptedAI
        {
            npc_ds_ysera_56665AI(Creature* creature) : ScriptedAI(creature), instance(creature->GetInstanceScript()) { }

            void InitializeAI() override
            {
                JustRespawned();
            }

            void EnterEvadeMode(EvadeReason reason) override
            {
                events.Reset();

                uint32 corpseDelay = me->GetCorpseDelay();
                uint32 respawnDelay = me->GetRespawnDelay();

                me->SetCorpseDelay(1);
                me->SetRespawnDelay(29);

                me->DespawnOrUnsummon();

                me->SetCorpseDelay(corpseDelay);
                me->SetRespawnDelay(respawnDelay);

                Position home = me->GetHomePosition();
                me->NearTeleportTo(home.GetPositionX(), home.GetPositionY(), home.GetPositionZ(), home.GetOrientation());
            }

            void JustRespawned() override
            {
                if (me->GetAreaId() == AREA_ABOVE_THE_FROZEN_SEA)
                {
                    DoCastAOE(SPELL_YSERA_CHARGE_DS_GUNSHIP);
                    events.ScheduleEvent(EVENT_CHANNEL_DS_GUNSHIP, 1 * IN_MILLISECONDS);
                }

                CreatureAI::JustRespawned();
            }

            /*void MovementInform(uint32 type, uint32 id) override
            {
            }*/

            void UpdateAI(uint32 diff) override
            {
                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case EVENT_CHANNEL_DS_GUNSHIP:
                        if (!me->HasUnitState(UNIT_STATE_CASTING))
                            DoCastAOE(SPELL_YSERA_CHARGE_DS_GUNSHIP);
                        events.ScheduleEvent(EVENT_CHANNEL_DS_GUNSHIP, 1 * IN_MILLISECONDS);
                        break;
                    default:
                        break;
                    }
                }
            }

        private:
            EventMap events;
            InstanceScript* instance;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_ds_ysera_56665AI(creature);
        }
};

// http://www.wowhead.com/npc=56666
class npc_ds_nozdormu_56666 : public CreatureScript
{
    public:
        npc_ds_nozdormu_56666() : CreatureScript("npc_ds_nozdormu_56666") {}

        struct npc_ds_nozdormu_56666AI : public ScriptedAI
        {
            npc_ds_nozdormu_56666AI(Creature* creature) : ScriptedAI(creature), instance(creature->GetInstanceScript()) { }

            void InitializeAI() override
            {
                JustRespawned();
            }

            void EnterEvadeMode(EvadeReason reason) override
            {
                events.Reset();

                uint32 corpseDelay = me->GetCorpseDelay();
                uint32 respawnDelay = me->GetRespawnDelay();

                me->SetCorpseDelay(1);
                me->SetRespawnDelay(29);

                me->DespawnOrUnsummon();

                me->SetCorpseDelay(corpseDelay);
                me->SetRespawnDelay(respawnDelay);

                Position home = me->GetHomePosition();
                me->NearTeleportTo(home.GetPositionX(), home.GetPositionY(), home.GetPositionZ(), home.GetOrientation());
            }

            void JustRespawned() override
            {
                if (me->GetAreaId() == AREA_ABOVE_THE_FROZEN_SEA)
                {
                    DoCastAOE(SPELL_NOZDORMU_CHARGE_DS_GUNSHIP);
                    events.ScheduleEvent(EVENT_CHANNEL_DS_GUNSHIP, 1 * IN_MILLISECONDS);
                }

                CreatureAI::JustRespawned();
            }

            /*void MovementInform(uint32 type, uint32 id) override
            {
            }*/

            void UpdateAI(uint32 diff) override
            {
                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case EVENT_CHANNEL_DS_GUNSHIP:
                        if (!me->HasUnitState(UNIT_STATE_CASTING))
                            DoCastAOE(SPELL_NOZDORMU_CHARGE_DS_GUNSHIP);
                        events.ScheduleEvent(EVENT_CHANNEL_DS_GUNSHIP, 1 * IN_MILLISECONDS);
                        break;
                    default:
                        break;
                    }
                }
            }

        private:
            EventMap events;
            InstanceScript* instance;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_ds_nozdormu_56666AI(creature);
        }
};

// http://www.wowhead.com/spell=108242
// http://www.wowhead.com/spell=108243
// http://www.wowhead.com/spell=108471
// http://www.wowhead.com/spell=108472
// http://www.wowhead.com/spell=108473
// http://www.wowhead.com/spell=108833
// http://www.wowhead.com/spell=108836
// http://www.wowhead.com/spell=108837
// http://www.wowhead.com/spell=108838
class spell_ds_charge_dragon_soul : public SpellScriptLoader
{
    public:
        spell_ds_charge_dragon_soul() : SpellScriptLoader("spell_ds_charge_dragon_soul") {}

        class spell_ds_charge_dragon_soul_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_ds_charge_dragon_soul_SpellScript);

            void FindTarget(WorldObject*& target)
            {
                target = (WorldObject*)NULL;
                switch (GetSpellInfo()->Id)
                {
                    case SPELL_YSERA_CHARGE_DS_GUNSHIP:
                    case SPELL_KALECGOS_CHARGE_DS_GUNSHIP:
                    case SPELL_ALEXSTRASZA_CHARGE_DS_GUNSHIP:
                    case SPELL_NOZDORMU_CHARGE_DS_GUNSHIP:
                        if (Creature* DragonSoul = GetCaster()->FindNearestCreature(NPC_DRAGON_SOUL, 200.0f))
                            target = DragonSoul;
                        break;
                    default:
                        break;
                }
            }

            void Register() override
            {
                OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_ds_charge_dragon_soul_SpellScript::FindTarget, EFFECT_0, TARGET_UNIT_NEARBY_ENTRY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_ds_charge_dragon_soul_SpellScript();
        }
};

void AddSC_dragon_soul()
{
    new npc_ds_alexstrasza_56630();
    new npc_ds_kalecgos_56664();
    new npc_ds_ysera_56665();
    new npc_ds_nozdormu_56666();

    new spell_ds_charge_dragon_soul();
}
