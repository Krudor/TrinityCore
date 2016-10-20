#include "ScriptMgr.h"
#include "dragon_soul.h"
#include "ScriptedEscortAI.h"

/*
    Events Unfold - Hagara
    Timers on the events needs a look at, only have a second precision measurement from retail.
*/

enum Spells
{
	//Deathwing
	SPELL_MOLTEN_METEOR					= 105022, //Far Circle

    // Hagara Intro
    SPELL_EVENTS_UNFOLD_HAGARA                      = 110952,
    SPELL_EVENTS_UNFOLD_HAGARA_SUMMON_YSERA         = 109502,
    SPELL_EVENTS_UNFOLD_HAGARA_SUMMON_THRALL        = 109500,
    SPELL_EVENTS_UNFOLD_HAGARA_SUMMON_ALEXSTRASZA   = 109501,
    SPELL_EVENTS_UNFOLD_HAGARA_SUMMON_KALECGOS      = 109499,
    SPELL_EVENTS_UNFOLD_HAGARA_SUMMON_NOZDORMU      = 109498,

    SPELL_DIALOGUE_EVENTS_UNFOLD_HAGARA_1           = 109520,
    SPELL_DIALOGUE_EVENTS_UNFOLD_HAGARA_2           = 109524,
    SPELL_DIALOGUE_EVENTS_UNFOLD_HAGARA_3           = 109518,
    SPELL_DIALOGUE_EVENTS_UNFOLD_HAGARA_4           = 109675,
    SPELL_DIALOGUE_EVENTS_UNFOLD_HAGARA_5           = 109521,
    SPELL_DIALOGUE_EVENTS_UNFOLD_HAGARA_6           = 109523,
    SPELL_DIALOGUE_EVENTS_UNFOLD_HAGARA_7           = 109522,
    SPELL_DIALOGUE_EVENTS_UNFOLD_HAGARA_8           = 109519,
    SPELL_OPEN_EYE_OF_ETERNITY_PORTAL               = 109527,

    // Other
    SPELL_TWILIGHT_PORTAL_BEAM          = 108096,
    SPELL_COSMETIC_CHANNEL_OMNI         = 51835,
    SPELL_EARTHEN_SOLDIER_ENERGY_DRAIN  = 107849,
    SPELL_SUMMON_PURE_WATER             = 107780,
    SPELL_PURE_WATER                    = 107751,

    SPELL_EARTHEN_VORTEX_RIDE = 109615,
    SPELL_SUMMON_IMAGE_OF_TYRSAGOSA     = 108924,
    SPELL_TYRAGOSA_IMAGE_EFFECT         = 108841,

    SPELL_YSERA_CHARGE_DS_GUNSHIP       = 108833,
    SPELL_KALECGOS_CHARGE_DS_GUNSHIP    = 108836,
    SPELL_ALEXSTRASZA_CHARGE_DS_GUNSHIP = 108837,
    SPELL_NOZDORMU_CHARGE_DS_GUNSHIP    = 108838,

    SPELL_TWILIGHT_FLAMES_GROUP_1       = 105580,
    SPELL_TWILIGHT_FLAMES_GROUP_2       = 109681,
    SPELL_TWILIGHT_FLAMES_GROUP_3       = 105661,
    SPELL_TWILIGHT_FLAMES_GROUP_4       = 105657,
    SPELL_TWILIGHT_FLAMES_GROUP_5       = 105659,
    SPELL_TWILIGHT_FLAMES_GROUP_6       = 105611,
    SPELL_TWILIGHT_FLAMES_GROUP_7       = 105615,
    SPELL_TWILIGHT_FLAMES_GROUP_8       = 105612,
    SPELL_TWILIGHT_FLAMES_GROUP_9       = 105613,
    SPELL_TWILIGHT_FLAMES_GROUP_10      = 109682,
    SPELL_TWILIGHT_FLAMES_GROUP_11      = 105662,
    SPELL_TWILIGHT_FLAMES_GROUP_12      = 105663,
};

enum Events
{
    EVENT_CHANNEL_DS_GUNSHIP = 1,
    EVENT_EVENTS_UNFOLD_HAGARA_1,
    EVENT_EVENTS_UNFOLD_HAGARA_2,
    EVENT_EVENTS_UNFOLD_HAGARA_3,
    EVENT_EVENTS_UNFOLD_HAGARA_4,
    EVENT_EVENTS_UNFOLD_HAGARA_5,
    EVENT_EVENTS_UNFOLD_HAGARA_6,
    EVENT_EVENTS_UNFOLD_HAGARA_7,
    EVENT_EVENTS_UNFOLD_HAGARA_8,
    EVENT_EVENTS_UNFOLD_HAGARA_9,
    EVENT_EVENTS_UNFOLD_HAGARA_10,
    EVENT_EVENTS_UNFOLD_HAGARA_11,
    EVENT_EVENTS_UNFOLD_HAGARA_12,
    EVENT_COSMETIC_EMOTE,
};

enum DragonSoulData
{
	DATA_TWILIGHT_FLAMES_GROUP_1        = 0,
    DATA_TWILIGHT_FLAMES_GROUP_2        = 1,
    DATA_TWILIGHT_FLAMES_GROUP_3        = 2,
    DATA_TWILIGHT_FLAMES_GROUP_4        = 3,
    DATA_TWILIGHT_FLAMES_GROUP_5        = 4,
    DATA_TWILIGHT_FLAMES_GROUP_6        = 5,
    DATA_TWILIGHT_FLAMES_GROUP_7        = 6,
    DATA_TWILIGHT_FLAMES_GROUP_8        = 7,
    DATA_TWILIGHT_FLAMES_GROUP_9        = 8,
    DATA_TWILIGHT_FLAMES_GROUP_10       = 9,
    DATA_TWILIGHT_FLAMES_GROUP_11       = 10,
    DATA_TWILIGHT_FLAMES_GROUP_12       = 11,

    DATA_TWILIGHT_FLAMES                = 1,
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

        void EnterEvadeMode() override
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

            void EnterEvadeMode() override
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

// http://www.wowhead.com/npc=56305/ultraxion-gauntlet
class npc_ultraxion_gauntlet : public CreatureScript
{
    public:
	npc_ultraxion_gauntlet() : CreatureScript("npc_ultraxion_gauntlet") {}

	struct npc_ultraxion_gauntletAI : public PassiveAI
	{
		npc_ultraxion_gauntletAI(Creature* creature) : PassiveAI(creature), instance(creature->GetInstanceScript()) { }

		void InitializeAI() override
		{
			JustRespawned();
		}

		void JustRespawned() override
		{
            for (int i = 0; i < 12; i++)
            {
                std::list<TempSummon*> summonGroup;
                me->SummonCreatureGroup(i, &summonGroup);
                for(auto summon : summonGroup)
                {
                    if (summon->IsAIEnabled)
                        summon->AI()->SetData(DATA_TWILIGHT_FLAMES, i);
                }
            }
		}

	private:
		EventMap events;
		InstanceScript* instance;
	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return new npc_ultraxion_gauntletAI(creature);
	}
};

// http://www.wowhead.com/npc=57281/twilight-assaulter
class npc_twilight_assaulter_flames : public CreatureScript
{
    public:
	npc_twilight_assaulter_flames() : CreatureScript("npc_twilight_assaulter_flames") {}

	struct npc_twilight_assaulter_flamesAI : public PassiveAI
	{
        npc_twilight_assaulter_flamesAI(Creature* creature) : PassiveAI(creature) { _flamesGroup = 100; }

		void SetData(uint32 type, uint32 data) override
        {
            if (type != DATA_TWILIGHT_FLAMES)
                return;

            _flamesGroup = data;
        }

        uint32 GetData(uint32 type) const override
        {
            switch (type)
            {
                case DATA_TWILIGHT_FLAMES:
                    return _flamesGroup;
                default:
                    break;
            }

            return 0;
        }

        private:
        uint32 _flamesGroup;
	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return new npc_twilight_assaulter_flamesAI(creature);
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

            void EnterEvadeMode() override
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

            void EnterEvadeMode() override
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

class npc_ds_thrall_events_unfold_hagara : public CreatureScript
{
    public:
    npc_ds_thrall_events_unfold_hagara() : CreatureScript("npc_ds_thrall_events_unfold_hagara") {}

    struct npc_ds_thrall_events_unfold_hagaraAI : public ScriptedAI
    {
        npc_ds_thrall_events_unfold_hagaraAI(Creature* creature) : ScriptedAI(creature) {}

        void InitializeAI() override
        {
            if (!me->IsAlive())
                return;

            JustRespawned();
        }
        
        void JustRespawned() override
        {
            events.ScheduleEvent(EVENT_EVENTS_UNFOLD_HAGARA_1, 3 * IN_MILLISECONDS);
        }

        void UpdateAI(uint32 diff) override
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_EVENTS_UNFOLD_HAGARA_1:
                        DoCastAOE(SPELL_DIALOGUE_EVENTS_UNFOLD_HAGARA_1, true);
                        events.ScheduleEvent(EVENT_COSMETIC_EMOTE, 8 * IN_MILLISECONDS);
                        events.ScheduleEvent(EVENT_EVENTS_UNFOLD_HAGARA_5, 39 * IN_MILLISECONDS);
                        break;
                    case EVENT_EVENTS_UNFOLD_HAGARA_5:
                        DoCastAOE(SPELL_DIALOGUE_EVENTS_UNFOLD_HAGARA_5, true);
                        events.ScheduleEvent(EVENT_EVENTS_UNFOLD_HAGARA_7, 20 * IN_MILLISECONDS);
                        break;
                    case EVENT_EVENTS_UNFOLD_HAGARA_7:
                        DoCastAOE(SPELL_DIALOGUE_EVENTS_UNFOLD_HAGARA_7, true);
                        break;
                    case EVENT_COSMETIC_EMOTE:
                        me->HandleEmoteCommand(EMOTE_ONESHOT_TALK);
                        break;
                    default:
                        break;
                }
            }
        }

        private:
        EventMap events;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_ds_thrall_events_unfold_hagaraAI(creature);
    }
};

class npc_ds_ysera_events_unfold_hagara : public CreatureScript
{
    public:
    npc_ds_ysera_events_unfold_hagara() : CreatureScript("npc_ds_ysera_events_unfold_hagara") {}

    struct npc_ds_ysera_events_unfold_hagaraAI : public ScriptedAI
    {
        npc_ds_ysera_events_unfold_hagaraAI(Creature* creature) : ScriptedAI(creature) {}

        void InitializeAI() override
        {
            if (!me->IsAlive())
                return;

            JustRespawned();
        }

        void JustRespawned() override
        {
            events.ScheduleEvent(EVENT_EVENTS_UNFOLD_HAGARA_6, 52 * IN_MILLISECONDS);
        }

        void UpdateAI(uint32 diff) override
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_EVENTS_UNFOLD_HAGARA_6:
                        DoCastAOE(SPELL_DIALOGUE_EVENTS_UNFOLD_HAGARA_6, true);
                        break;
                    default:
                        break;
                }
            }
        }

        private:
        EventMap events;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_ds_ysera_events_unfold_hagaraAI(creature);
    }
};

class npc_ds_nozdormu_events_unfold_hagara : public CreatureScript
{
    public:
    npc_ds_nozdormu_events_unfold_hagara() : CreatureScript("npc_ds_nozdormu_events_unfold_hagara") {}

    struct npc_ds_nozdormu_events_unfold_hagaraAI : public ScriptedAI
    {
        npc_ds_nozdormu_events_unfold_hagaraAI(Creature* creature) : ScriptedAI(creature)
        {
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_ds_nozdormu_events_unfold_hagaraAI(creature);
    }
};

class npc_ds_alexstrasza_events_unfold_hagara : public CreatureScript
{
    public:
    npc_ds_alexstrasza_events_unfold_hagara() : CreatureScript("npc_ds_alexstrasza_events_unfold_hagara") {}

    struct npc_ds_alexstrasza_events_unfold_hagaraAI : public ScriptedAI
    {
        npc_ds_alexstrasza_events_unfold_hagaraAI(Creature* creature) : ScriptedAI(creature) {}

        void InitializeAI() override
        {
            if (!me->IsAlive())
                return;

            JustRespawned();
        }

        void JustRespawned() override
        {
            events.ScheduleEvent(EVENT_EVENTS_UNFOLD_HAGARA_2, 17 * IN_MILLISECONDS);
        }

        void UpdateAI(uint32 diff) override
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_EVENTS_UNFOLD_HAGARA_2:
                        DoCastAOE(SPELL_DIALOGUE_EVENTS_UNFOLD_HAGARA_2, true);
                        break;
                    default:
                        break;
                }
            }
        }

        private:
        EventMap events;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_ds_alexstrasza_events_unfold_hagaraAI(creature);
    }
};

class npc_ds_kalegos_events_unfold_hagara : public CreatureScript
{
    public:
    npc_ds_kalegos_events_unfold_hagara() : CreatureScript("npc_ds_kalegos_events_unfold_hagara") {}

    struct npc_ds_kalegos_events_unfold_hagaraAI : public ScriptedAI
    {
        npc_ds_kalegos_events_unfold_hagaraAI(Creature* creature) : ScriptedAI(creature) { }

        void InitializeAI() override
        {
            if (!me->IsAlive())
                return;

            JustRespawned();
        }

        void JustRespawned() override
        {
            events.ScheduleEvent(EVENT_EVENTS_UNFOLD_HAGARA_3, 21 * IN_MILLISECONDS);
        }

        void UpdateAI(uint32 diff) override
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_COSMETIC_EMOTE:
                        me->HandleEmoteCommand(EMOTE_ONESHOT_TALK);
                        break;
                    case EVENT_EVENTS_UNFOLD_HAGARA_3:
                        DoCastAOE(SPELL_DIALOGUE_EVENTS_UNFOLD_HAGARA_3, true);
                        events.ScheduleEvent(EVENT_COSMETIC_EMOTE, 3 * IN_MILLISECONDS);
                        events.ScheduleEvent(EVENT_EVENTS_UNFOLD_HAGARA_4, 10 * IN_MILLISECONDS);
                        break;
                    case EVENT_EVENTS_UNFOLD_HAGARA_4:
                        DoCastAOE(SPELL_DIALOGUE_EVENTS_UNFOLD_HAGARA_4, true);
                        events.ScheduleEvent(EVENT_EVENTS_UNFOLD_HAGARA_8, 37 * IN_MILLISECONDS);
                        break;
                    case EVENT_EVENTS_UNFOLD_HAGARA_8:
                        DoCastAOE(SPELL_DIALOGUE_EVENTS_UNFOLD_HAGARA_8, true);
                        events.ScheduleEvent(EVENT_EVENTS_UNFOLD_HAGARA_9, 13 * IN_MILLISECONDS);
                        break;
                    case EVENT_EVENTS_UNFOLD_HAGARA_9:
                        if (InstanceScript* instance = me->GetInstanceScript())
                        {
                            if (Creature* portal = ObjectAccessor::GetCreature(*me, instance->GetGuidData(NPC_TRAVEL_TO_EYE_OF_ETERNITY)))
                            {
                                me->SetFacingToObject(portal);
                                me->UpdatePosition(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetAngle(portal));
                            }
                        }
                        events.ScheduleEvent(EVENT_EVENTS_UNFOLD_HAGARA_10, 2 * IN_MILLISECONDS);
                        break;
                    case EVENT_EVENTS_UNFOLD_HAGARA_10:
                        DoCastAOE(SPELL_OPEN_EYE_OF_ETERNITY_PORTAL);
                        events.ScheduleEvent(EVENT_EVENTS_UNFOLD_HAGARA_11, 1.5 * IN_MILLISECONDS);
                        break;
                    case EVENT_EVENTS_UNFOLD_HAGARA_11:
                        if (InstanceScript* instance = me->GetInstanceScript())
                            if (Creature* portal = instance->instance->GetCreature(instance->GetGuidData(NPC_TRAVEL_TO_EYE_OF_ETERNITY)))
                                portal->SetInPhase(PHASE_DUNGEON_ALTERNATE, true, false);
                        events.ScheduleEvent(EVENT_EVENTS_UNFOLD_HAGARA_12, 6.5 * IN_MILLISECONDS);
                        break;
                    case EVENT_EVENTS_UNFOLD_HAGARA_12:
                        me->SetFacingTo(me->GetHomePosition().GetOrientation());
                        me->UpdatePosition(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetHomePosition().GetOrientation());
                        break;
                    default:
                        break;
                }
            }
        }

        private:
        EventMap events;
    };

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        // override default gossip
        return true;
        // load default gossip
        return false;
    }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_ds_kalegos_events_unfold_hagaraAI(creature);
    }
};

// http://www.wowhead.com/spell=108096
class spell_ds_twilight_portal_beam : public SpellScriptLoader
{
public:
    spell_ds_twilight_portal_beam() : SpellScriptLoader("spell_ds_twilight_portal_beam") {}

    class spell_ds_twilight_portal_beam_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_ds_twilight_portal_beam_SpellScript);

        void SelectPortal(WorldObject*& target)
        {
            target = (WorldObject*)NULL;
            if (Creature* portal = GetCaster()->FindNearestCreature(NPC_TWILIGHT_PORTAL, 20.0f))
                target = portal;
        }

        void Register() override
        {
            OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_ds_twilight_portal_beam_SpellScript::SelectPortal, EFFECT_0, TARGET_UNIT_NEARBY_ENTRY);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_ds_twilight_portal_beam_SpellScript();
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

// http://www.wowhead.com/spell=105580/twilight-flames
// http://www.wowhead.com/spell=109681/twilight-flames
// http://www.wowhead.com/spell=105661/twilight-flames
// http://www.wowhead.com/spell=105657/twilight-flames
// http://www.wowhead.com/spell=105659/twilight-flames
// http://www.wowhead.com/spell=105611/twilight-flames
// http://www.wowhead.com/spell=105615/twilight-flames
// http://www.wowhead.com/spell=105612/twilight-flames
// http://www.wowhead.com/spell=105613/twilight-flames
// http://www.wowhead.com/spell=109682/twilight-flames
// http://www.wowhead.com/spell=105662/twilight-flames
// http://www.wowhead.com/spell=105663/twilight-flames
class spell_ds_twilight_flames_dummy_target : public SpellScriptLoader
{
    public:
    spell_ds_twilight_flames_dummy_target() : SpellScriptLoader("spell_ds_twilight_flames_dummy_target") {}

    class spell_ds_twilight_flames_dummy_target_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_ds_twilight_flames_dummy_target_SpellScript);

        bool Load()
        {
            sharedTargets.clear();
            return true;
        }

        void FilterTwilightFire(std::list<WorldObject*>& targets)
        {
            uint32 spellId = GetSpellInfo()->Id;

            targets.remove_if([spellId](WorldObject* target)
            {
                Creature* dummy = target->ToCreature();
                if (dummy)
                {
                    if (dummy->HasAuraEffect(spellId, EFFECT_1))
                    {
                        std::cout << "\nreturn false";
                        return false;
                    }

                    if (dummy->GetEntry() == NPC_TWILIGHT_ASSAULT_FIRE && dummy->IsAIEnabled)
                    {
                        uint32 id = dummy->AI()->GetData(DATA_TWILIGHT_FLAMES);
                        switch (id)
                        {
                            case DATA_TWILIGHT_FLAMES_GROUP_1:
                                return spellId != SPELL_TWILIGHT_FLAMES_GROUP_1;
                            case DATA_TWILIGHT_FLAMES_GROUP_2:
                                return spellId != SPELL_TWILIGHT_FLAMES_GROUP_2;
                            case DATA_TWILIGHT_FLAMES_GROUP_3:
                                return spellId != SPELL_TWILIGHT_FLAMES_GROUP_3;
                            case DATA_TWILIGHT_FLAMES_GROUP_4:
                                return spellId != SPELL_TWILIGHT_FLAMES_GROUP_4;
                            case DATA_TWILIGHT_FLAMES_GROUP_5:
                                return spellId != SPELL_TWILIGHT_FLAMES_GROUP_5;
                            case DATA_TWILIGHT_FLAMES_GROUP_6:
                                return spellId != SPELL_TWILIGHT_FLAMES_GROUP_6;
                            case DATA_TWILIGHT_FLAMES_GROUP_7:
                                return spellId != SPELL_TWILIGHT_FLAMES_GROUP_7;
                            case DATA_TWILIGHT_FLAMES_GROUP_8:
                                return spellId != SPELL_TWILIGHT_FLAMES_GROUP_8;
                            case DATA_TWILIGHT_FLAMES_GROUP_9:
                                return spellId != SPELL_TWILIGHT_FLAMES_GROUP_9;
                            case DATA_TWILIGHT_FLAMES_GROUP_10:
                                return spellId != SPELL_TWILIGHT_FLAMES_GROUP_10;
                            case DATA_TWILIGHT_FLAMES_GROUP_11:
                                return spellId != SPELL_TWILIGHT_FLAMES_GROUP_11;
                            case DATA_TWILIGHT_FLAMES_GROUP_12:
                                return spellId != SPELL_TWILIGHT_FLAMES_GROUP_12;
                        }
                    }
                }

                return true;
            });

            for (auto target : targets)
                if (Unit* unitTarget = target->ToUnit())
                    if (unitTarget->HasAuraEffect(spellId, EFFECT_1))
                        sharedTargets.push_back(target);

            for (auto target : sharedTargets)
                targets.remove(target);

            targets.sort(Trinity::ObjectDistanceOrderPred(GetCaster()));
            if (!targets.empty())
                sharedTargets.push_back(targets.front());

            targets = sharedTargets;
            for (auto target : targets)
                std::cout << "\nTARGET: " << target->GetName() << ", guid: " << target->GetGUIDLow();
        }

        /*void FilterList(std::list<WorldObject*>& targets)
        {
            targets = sharedTargets;
        }*/

        void Register() override
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_ds_twilight_flames_dummy_target_SpellScript::FilterTwilightFire, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_ds_twilight_flames_dummy_target_SpellScript::FilterTwilightFire, EFFECT_1, TARGET_UNIT_SRC_AREA_ENTRY);
        }

        private:
        std::list<WorldObject*> sharedTargets;
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_ds_twilight_flames_dummy_target_SpellScript();
    }
};

// http://www.wowhead.com/spell=109498/events-unfold
// http://www.wowhead.com/spell=109499/events-unfold
// http://www.wowhead.com/spell=109500/events-unfold
// http://www.wowhead.com/spell=109501/events-unfold
// http://www.wowhead.com/spell=109502/events-unfold
class spell_ds_events_unfold_hagara_summon : public SpellScriptLoader
{
    public:
    spell_ds_events_unfold_hagara_summon() : SpellScriptLoader("spell_ds_events_unfold_hagara_summon") {}

    class spell_ds_events_unfold_hagara_summon_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_ds_events_unfold_hagara_summon_SpellScript);

        void SelectTarget(WorldObject*& target)
        {
            uint32 entry = 0;
            switch (GetSpellInfo()->Id)
            {
                case SPELL_EVENTS_UNFOLD_HAGARA_SUMMON_ALEXSTRASZA:
                    entry = NPC_ALEXSTRASZA_WYRMREST_TEMPLE;
                    break;
                case SPELL_EVENTS_UNFOLD_HAGARA_SUMMON_KALECGOS:
                    entry = NPC_KALECGOS_WYRMREST_TEMPLE;
                    break;
                case SPELL_EVENTS_UNFOLD_HAGARA_SUMMON_NOZDORMU:
                    entry = NPC_NOZDORMU_WYRMREST_TEMPLE;
                    break;
                case SPELL_EVENTS_UNFOLD_HAGARA_SUMMON_THRALL:
                    entry = NPC_THRALL_WYRMREST_TEMPLE;
                    break;
                case SPELL_EVENTS_UNFOLD_HAGARA_SUMMON_YSERA:
                    entry = NPC_YSERA_WYRMREST_TEMPLE;
                    break;
                default:
                    break;
            }

            if (InstanceScript* instance = GetCaster()->GetInstanceScript())
            {
                if (Creature* _target = ObjectAccessor::GetCreature(*GetCaster(), instance->GetGuidData(entry)))
                {
                    target = _target;
                    return;
                }
            }

            FinishCast(SPELL_FAILED_NO_VALID_TARGETS);
        }

        void Register() override
        {
            OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_ds_events_unfold_hagara_summon_SpellScript::SelectTarget, EFFECT_0, TARGET_DEST_NEARBY_ENTRY);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_ds_events_unfold_hagara_summon_SpellScript();
    }
};

// http://www.wowhead.com/spell=109527/open-eye-of-eternity-portal
class spell_ds_open_eye_of_eternity_portal : public SpellScriptLoader
{
    public:
    spell_ds_open_eye_of_eternity_portal() : SpellScriptLoader("spell_ds_open_eye_of_eternity_portal") {}

    class spell_ds_open_eye_of_eternity_portal_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_ds_open_eye_of_eternity_portal_SpellScript);

        bool Load()
        {
            if (GetCaster()->GetTypeId() != TYPEID_UNIT)
                return false;
            return true;
        }

        void FilterTarget(WorldObject*& target)
        {
            target = (WorldObject*)NULL;
            if (InstanceScript* instance = GetCaster()->GetInstanceScript())
                if (Creature* portal = instance->instance->GetCreature(instance->GetGuidData(NPC_TRAVEL_TO_EYE_OF_ETERNITY)))
                    target = portal;
        }

        void Register() override
        {
            OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_ds_open_eye_of_eternity_portal_SpellScript::FilterTarget, EFFECT_0, TARGET_UNIT_NEARBY_ENTRY);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_ds_open_eye_of_eternity_portal_SpellScript();
    }
};

class go_ds_the_focusing_iris : public GameObjectScript
{
    public:
    go_ds_the_focusing_iris() : GameObjectScript("go_ds_the_focusing_iris") {}

    bool OnGossipHello(Player* player, GameObject* go) override
    {
        InstanceScript* instance = go->GetInstanceScript();
        if (!instance || instance->GetData(DATA_HAGARA_THE_STORMBINDER) != TO_BE_DECIDED)
            return true;

        instance->SetData(DATA_HAGARA_THE_STORMBINDER, DONE);
        return false;
    }
};

// 7185
class at_ds_wyrmrest_summit : public AreaTriggerScript
{
    public:
    at_ds_wyrmrest_summit() : AreaTriggerScript("at_ds_wyrmrest_summit") {}

    bool OnTrigger(Player* player, AreaTriggerEntry const* /*at*/) override
    {
        if (player->IsGameMaster())
            return true;

        InstanceScript* instance = player->GetInstanceScript();
        if (!instance || instance->GetData(DATA_EVENTS_UNFOLD_HAGARA) != NOT_STARTED)
            return true;

        instance->SetData(DATA_EVENTS_UNFOLD_HAGARA, DONE);
        player->CastSpell((Unit*)NULL, SPELL_EVENTS_UNFOLD_HAGARA, true);
        return false;
    }
};

void AddSC_dragon_soul()
{
    new npc_ds_alexstrasza_56630();
    new npc_ds_kalecgos_56664();
    new npc_ds_ysera_56665();
    new npc_ds_nozdormu_56666();

    new npc_ds_alexstrasza_events_unfold_hagara();
    new npc_ds_kalegos_events_unfold_hagara();
    new npc_ds_ysera_events_unfold_hagara();
    new npc_ds_thrall_events_unfold_hagara();

    new npc_ultraxion_gauntlet();
    new npc_twilight_assaulter_flames();

    new spell_ds_twilight_portal_beam();
    new spell_ds_charge_dragon_soul();
    new spell_ds_twilight_flames_dummy_target();
    new spell_ds_events_unfold_hagara_summon();
    new spell_ds_open_eye_of_eternity_portal();

    new go_ds_the_focusing_iris();

    new at_ds_wyrmrest_summit();
}
