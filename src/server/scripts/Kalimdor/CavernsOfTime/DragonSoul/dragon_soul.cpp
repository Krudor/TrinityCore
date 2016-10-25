#include "ScriptMgr.h"
#include "dragon_soul.h"
#include "SpellScript.h"
#include "ScriptedCreature.h"
#include "SharedDefines.h"
#include "Player.h"
#include "ScriptedGossip.h"
#include "GridNotifiers.h"
#include "PassiveAI.h"
#include "Spell.h"

/*
    Events Unfold - Hagara
    Timers on the events needs a look at, only have a second precision measurement from retail.
*/

enum Spells
{
	//Deathwing
	SPELL_MOLTEN_METEOR					            = 105022, //Far Circle

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
    EVENT_EVENTS_UNFOLD_HAGARA_12
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

uint32 const TwilightFlamesMapping[12] =
{
    { SPELL_TWILIGHT_FLAMES_GROUP_1 },
    { SPELL_TWILIGHT_FLAMES_GROUP_2 },
    { SPELL_TWILIGHT_FLAMES_GROUP_3 },
    { SPELL_TWILIGHT_FLAMES_GROUP_4 },
    { SPELL_TWILIGHT_FLAMES_GROUP_5 },
    { SPELL_TWILIGHT_FLAMES_GROUP_6 },
    { SPELL_TWILIGHT_FLAMES_GROUP_7 },
    { SPELL_TWILIGHT_FLAMES_GROUP_8 },
    { SPELL_TWILIGHT_FLAMES_GROUP_9 },
    { SPELL_TWILIGHT_FLAMES_GROUP_10 },
    { SPELL_TWILIGHT_FLAMES_GROUP_11 },
    { SPELL_TWILIGHT_FLAMES_GROUP_12 },
};

enum DragonSoulData123
{
    GOSSIP_MENU_KALECGOS_EVENTS_UNFOLD              = 13373,
    GOSSIP_OPTION_KALECGOS_EVENTS_UNFOLD_CONFIRM    = 0,
    GOSSIP_MENU_THRALL_ULTRAXION                    = 13322,
};

std::map<uint32, uint8> EventsUnfoldIndexMap = 
{
    {}
};

struct dragon_soul_track_instance_progressAI : public ScriptedAI
{
    dragon_soul_track_instance_progressAI(Creature* creature) : ScriptedAI(creature), _instanceProgress() { }

    void Reset() override
    {
        ScriptedAI::Reset();
        if (InstanceScript* instance = me->GetInstanceScript())
            _instanceProgress = static_cast<DragonSoulEventProgress>(instance->GetData(DATA_INSTANCE_PROGRESS));
    }

    void SetData(uint32 type, uint32 data) override
    {
        ScriptedAI::SetData(type, data);
        switch (type)
        {
            case DATA_INSTANCE_PROGRESS:
                _instanceProgress = static_cast<DragonSoulEventProgress>(data);
                OnInstanceProgressUpdate(_instanceProgress);
                break;
            default:
                break;
        }
    }

    virtual void OnInstanceProgressUpdate(DragonSoulEventProgress /*instanceProgress*/) { }

    DragonSoulEventProgress GetInstanceProgress() const
    {
        return _instanceProgress;
    }

private:
    DragonSoulEventProgress _instanceProgress;
};

// http://www.wowhead.com/npc=56630
class npc_ds_alexstrasza_part_one : public CreatureScript
{
    public:
        npc_ds_alexstrasza_part_one() : CreatureScript("npc_ds_alexstrasza_part_one") {}

        struct npc_ds_alexstrasza_part_oneAI : public dragon_soul_track_instance_progressAI
        {
            npc_ds_alexstrasza_part_oneAI(Creature* creature) : dragon_soul_track_instance_progressAI(creature) { }

            void EnterEvadeMode(EvadeReason) override
            {
                uint32 timeToDespawn;
                switch (GetInstanceProgress())
                {
                    case EVENT_PROGRESS_ULTRAXION_TRASH:
                        timeToDespawn = 3;
                        break;
                    case EVENT_PROGRESS_ULTRAXION:
                    default:
                        timeToDespawn = 30;
                        break;
                }

                _scheduler.CancelAll();

                uint32 corpseDelay = me->GetCorpseDelay();
                uint32 respawnDelay = me->GetRespawnDelay();
                me->SetCorpseDelay(1);
                me->SetRespawnDelay(timeToDespawn - 1);
                me->DespawnOrUnsummon();
                me->SetCorpseDelay(corpseDelay);
                me->SetRespawnDelay(respawnDelay);

                auto home = me->GetHomePosition();
                me->NearTeleportTo(home.GetPositionX(), home.GetPositionY(), home.GetPositionZ(), home.GetOrientation());
            }

            void Reset() override
            {
                switch (me->GetAreaId())
                {
                    case AREA_ABOVE_THE_FROZEN_SEA:
                        DoCastAOE(SPELL_ALEXSTRASZA_CHARGE_DS_GUNSHIP);
                        _scheduler.Schedule(Seconds(1), [this](TaskContext context)
                        {
                            if (!me->HasUnitState(UNIT_STATE_CASTING))
                                DoCastAOE(SPELL_ALEXSTRASZA_CHARGE_DS_GUNSHIP);
                            context.Repeat();
                        });
                        break;
                    default:
                        break;
                }
            }

        private:
            TaskScheduler _scheduler;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_ds_alexstrasza_part_oneAI(creature);
        }
};

struct wyrmrest_summit_aspectAI : public dragon_soul_track_instance_progressAI
{
    wyrmrest_summit_aspectAI(Creature* creature) : dragon_soul_track_instance_progressAI(creature) { }

    void EnterEvadeMode(EvadeReason reason) override
    {
        dragon_soul_track_instance_progressAI::EnterEvadeMode(reason);

        _scheduler.CancelAll();

        uint32 timeToDespawn;
        switch (GetInstanceProgress())
        {
            case EVENT_PROGRESS_ULTRAXION_TRASH:
                timeToDespawn = 3;
                break;
            case EVENT_PROGRESS_ULTRAXION:
            default:
                timeToDespawn = 30;
                break;
        }

        uint32 corpseDelay = me->GetCorpseDelay();
        uint32 respawnDelay = me->GetRespawnDelay();
        me->SetCorpseDelay(1);
        me->SetRespawnDelay(timeToDespawn - 1);
        me->DespawnOrUnsummon();
        me->SetCorpseDelay(corpseDelay);
        me->SetRespawnDelay(respawnDelay);

        auto home = me->GetHomePosition();
        me->NearTeleportTo(home.GetPositionX(), home.GetPositionY(), home.GetPositionZ(), home.GetOrientation());
    }

    void OnInstanceProgressUpdate(DragonSoulEventProgress progress) override
    {
        switch (progress)
        {
            case EVENT_PROGRESS_HAGARA_END:
                me->SetVisible(true);
                break;
            default:
                break;
        }
    }

private:
    TaskScheduler _scheduler;
};

// http://www.wowhead.com/npc=56664
class npc_ds_kalecgos_part_one : public CreatureScript
{
    public:
        npc_ds_kalecgos_part_one() : CreatureScript("npc_ds_kalecgos_part_one") {}

        struct npc_ds_kalecgos_part_oneAI : public wyrmrest_summit_aspectAI
        {
            npc_ds_kalecgos_part_oneAI(Creature* creature) : wyrmrest_summit_aspectAI(creature) { }

            void Reset() override
            {
                wyrmrest_summit_aspectAI::Reset();

                switch (me->GetAreaId())
                {
                    case AREA_ABOVE_THE_FROZEN_SEA:
                        DoCastAOE(SPELL_KALECGOS_CHARGE_DS_GUNSHIP);
                        _scheduler.Schedule(Seconds(1), [this](TaskContext context)
                        {
                            if (!me->HasUnitState(UNIT_STATE_CASTING))
                                DoCastAOE(SPELL_KALECGOS_CHARGE_DS_GUNSHIP);
                            context.Repeat();
                        });
                        break;
                    case AREA_WYRMREST_SUMMIT:
                        switch (GetInstanceProgress())
                        {
                            case EVENT_PROGRESS_HAGARA_END:
                                me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                                break;
                            case EVENT_PROGRESS_ULTRAXION_TRASH:
                                break;
                            case EVENT_PROGRESS_ULTRAXION:
                                break;
                            case EVENT_PROGRESS_ULTRAXION_END:
                                break;
                            default:
                                me->SetVisible(false);
                                break;
                        }
                        break;
                    default:
                        break;
                }
            }

            void EnterEvadeMode(EvadeReason reason) override
            {
                wyrmrest_summit_aspectAI::EnterEvadeMode(reason);
                me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            }

        private:
            TaskScheduler _scheduler;
        };

        bool OnGossipHello(Player* player, Creature* creature) override
        {
            InstanceScript* instance = creature->GetInstanceScript();
            if (!instance)
                return true;

            switch (creature->GetAreaId())
            {
                case AREA_THE_DRAGON_WASTES_1:
                    player->ADD_GOSSIP_ITEM_DB(GOSSIP_MENU_KALECGOS_EVENTS_UNFOLD, GOSSIP_OPTION_KALECGOS_EVENTS_UNFOLD_CONFIRM, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
                    player->SEND_GOSSIP_MENU(player->GetGossipTextId(GOSSIP_MENU_KALECGOS_EVENTS_UNFOLD, creature), creature->GetGUID());
                    break;
                default:
                    break;
            }

            return true;
        }

        bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
        {
            player->PlayerTalkClass->SendCloseGossip();

            if (!creature->HasFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP))
                return true;

            InstanceScript* instance = creature->GetInstanceScript();
            if (!instance)
                return true;

            switch (action)
            {
                case GOSSIP_ACTION_INFO_DEF + 1:
                    creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);

                    //creature->CastSpell(player, SPELL_TELEPORT_ALL_TO_GUNSHIP, true);
                    if (GameObject* skyfire = ObjectAccessor::GetGameObject(*creature, instance->GetGuidData(GO_ALLIANCE_SHIP_1)))
                        break;
                case GOSSIP_ACTION_INFO_DEF + 2:
                    creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                    //creature->SummonCreature(NPC_GORIONA, GorionaSpawnPos, TEMPSUMMON_MANUAL_DESPAWN);
                    break;
                case GOSSIP_ACTION_INFO_DEF + 3:
                    if (instance->GetBossState(DATA_SPINE_OF_DEATHWING) != NOT_STARTED)
                        break;

                    creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                    //creature->AI()->DoCastAOE(SPELL_PLAY_SPINE_OF_DEATHWING_CINEMATIC, true);
                    break;
                default:
                    break;
            }

            return true;
        }

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_ds_kalecgos_part_oneAI(creature);
        }
};

// http://www.wowhead.com/npc=56665
class npc_ds_ysera_part_one : public CreatureScript
{
    public:
        npc_ds_ysera_part_one() : CreatureScript("npc_ds_ysera_part_one") {}

        struct npc_ds_ysera_part_oneAI : public ScriptedAI
        {
            npc_ds_ysera_part_oneAI(Creature* creature) : ScriptedAI(creature) { }

            void EnterEvadeMode(EvadeReason) override
            {
                _scheduler.CancelAll();

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

            void Reset() override
            {
                switch (me->GetAreaId())
                {
                    case AREA_ABOVE_THE_FROZEN_SEA:
                        DoCastAOE(SPELL_YSERA_CHARGE_DS_GUNSHIP);
                        _scheduler.Schedule(Seconds(1), [this](TaskContext context)
                        {
                            if (!me->HasUnitState(UNIT_STATE_CASTING))
                                DoCastAOE(SPELL_YSERA_CHARGE_DS_GUNSHIP);
                            context.Repeat();
                        });
                        break;
                    default:
                        break;
                }
            }

        private:
            TaskScheduler _scheduler;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_ds_ysera_part_oneAI(creature);
        }
};

// http://www.wowhead.com/npc=56666
class npc_ds_nozdormu_part_one : public CreatureScript
{
    public:
        npc_ds_nozdormu_part_one() : CreatureScript("npc_ds_nozdormu_part_one") {}

        struct npc_ds_nozdormu_part_oneAI : public ScriptedAI
        {
            npc_ds_nozdormu_part_oneAI(Creature* creature) : ScriptedAI(creature) { }

            void EnterEvadeMode(EvadeReason) override
            {
                _scheduler.CancelAll();

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

            void Reset() override
            {
                switch (me->GetAreaId())
                {
                    case AREA_ABOVE_THE_FROZEN_SEA:
                        DoCastAOE(SPELL_NOZDORMU_CHARGE_DS_GUNSHIP);
                        _scheduler.Schedule(Seconds(1), [this](TaskContext context)
                        {
                            if (!me->HasUnitState(UNIT_STATE_CASTING))
                                DoCastAOE(SPELL_NOZDORMU_CHARGE_DS_GUNSHIP);
                            context.Repeat();
                        });
                        break;
                    default:
                        break;
                }
            }

        private:
            TaskScheduler _scheduler;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_ds_nozdormu_part_oneAI(creature);
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

// This is a personal spawn, TrinityCore does not currently support this (4881e45688c3d8f64cf2efaa7553ecbc02a0884a)
// http://www.wowhead.com/npc=57946/thrall
class npc_ds_events_unfold_thrall : public CreatureScript
{
    public:
        npc_ds_events_unfold_thrall() : CreatureScript("npc_ds_events_unfold_thrall") {}

        struct npc_ds_events_unfold_thrallAI : public ScriptedAI
        {
            npc_ds_events_unfold_thrallAI(Creature* creature) : ScriptedAI(creature) {}
        
            void Reset() override
            {
                _scheduler.Schedule(Seconds(3), [this](TaskContext context)         // After 3 seconds
                {
                    DoCastAOE(SPELL_DIALOGUE_EVENTS_UNFOLD_HAGARA_1, true);
                    context.Schedule(Seconds(8), [this](TaskContext context)    // After 3 + 8 seconds
                    {
                        me->HandleEmoteCommand(EMOTE_ONESHOT_TALK);
                    });
                    context.Schedule(Seconds(39), [this](TaskContext context)   // After 3 + 39 seconds
                    {
                        DoCastAOE(SPELL_DIALOGUE_EVENTS_UNFOLD_HAGARA_5, true);
                        context.Schedule(Seconds(20), [this](TaskContext)       // After 3 + 39 + 20 seconds
                        {
                            DoCastAOE(SPELL_DIALOGUE_EVENTS_UNFOLD_HAGARA_7, true);
                        });
                    });
                });
            }

            void EnterEvadeMode(EvadeReason) override { } // No, I don't want you to trigger Reset

            private:
                TaskScheduler _scheduler;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_ds_events_unfold_thrallAI(creature);
        }
};

// http://www.wowhead.com/npc=57943/ysera-the-awakened
class npc_ds_events_unfold_ysera : public CreatureScript
{
    public:
        npc_ds_events_unfold_ysera() : CreatureScript("npc_ds_events_unfold_ysera") {}

        struct npc_ds_events_unfold_yseraAI : public ScriptedAI
        {
            npc_ds_events_unfold_yseraAI(Creature* creature) : ScriptedAI(creature) {}

            void Reset() override
            {
                _scheduler.Schedule(Seconds(52), [this](TaskContext context)
                {
                    DoCastAOE(SPELL_DIALOGUE_EVENTS_UNFOLD_HAGARA_6, true);
                });
            }

            void EnterEvadeMode(EvadeReason) override { } // No, I don't want you to trigger Reset

            private:
                TaskScheduler _scheduler;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_ds_events_unfold_yseraAI(creature);
        }
};

// http://www.wowhead.com/npc=57944/alexstrasza-the-life-binder
class npc_ds_events_unfold_alexstrasza : public CreatureScript
{
    public:
        npc_ds_events_unfold_alexstrasza() : CreatureScript("npc_ds_events_unfold_alexstrasza") {}

        struct npc_ds_events_unfold_alexstraszaAI : public ScriptedAI
        {
            npc_ds_events_unfold_alexstraszaAI(Creature* creature) : ScriptedAI(creature) {}

            void Reset() override
            {
                _scheduler.Schedule(Seconds(17), [this](TaskContext)
                {
                    DoCastAOE(SPELL_DIALOGUE_EVENTS_UNFOLD_HAGARA_2, true);
                });
            }

            void EnterEvadeMode(EvadeReason) override { } // No, I don't want you to trigger Reset

            private:
                TaskScheduler _scheduler;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_ds_events_unfold_alexstraszaAI(creature);
        }
};

// http://www.wowhead.com/npc=57947/kalecgos
class npc_ds_events_unfold_kalecgos : public CreatureScript
{
    public:
        npc_ds_events_unfold_kalecgos() : CreatureScript("npc_ds_events_unfold_kalecgos") {}

        struct npc_ds_events_unfold_kalecgosAI : public ScriptedAI
        {
            npc_ds_events_unfold_kalecgosAI(Creature* creature) : ScriptedAI(creature) { }

            void Reset() override
            {
                _scheduler.Schedule(Seconds(21), [this](TaskContext context)
                {
                    DoCastAOE(SPELL_DIALOGUE_EVENTS_UNFOLD_HAGARA_3, true);
                });
                _scheduler.Schedule(Seconds(24), [this](TaskContext)
                {
                    me->HandleEmoteCommand(EMOTE_ONESHOT_TALK);
                });
                _scheduler.Schedule(Seconds(34), [this](TaskContext)
                {
                    DoCastAOE(SPELL_DIALOGUE_EVENTS_UNFOLD_HAGARA_4, true);
                });
                _scheduler.Schedule(Seconds(71), [this](TaskContext)
                {
                    DoCastAOE(SPELL_DIALOGUE_EVENTS_UNFOLD_HAGARA_8, true);
                });
                _scheduler.Schedule(Seconds(84), [this](TaskContext)
                {
                    if (InstanceScript* instance = me->GetInstanceScript())
                    {
                        if (Creature* portal = ObjectAccessor::GetCreature(*me, instance->GetGuidData(NPC_TRAVEL_TO_EYE_OF_ETERNITY)))
                        {
                            me->SetFacingToObject(portal);
                            me->UpdatePosition(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetAngle(portal));
                        }
                    }
                });
                _scheduler.Schedule(Seconds(86), [this](TaskContext)
                {
                    DoCastAOE(SPELL_OPEN_EYE_OF_ETERNITY_PORTAL);
                });
                _scheduler.Schedule(Milliseconds(87500), [this](TaskContext)
                {
                    if (InstanceScript* instance = me->GetInstanceScript())
                        if (Creature* portal = instance->instance->GetCreature(instance->GetGuidData(NPC_TRAVEL_TO_EYE_OF_ETERNITY)))
                            portal->SetInPhase(PHASE_DUNGEON_ALTERNATE, true, false);
                });
                _scheduler.Schedule(Seconds(94), [this](TaskContext context)
                {
                    me->SetFacingTo(me->GetHomePosition().GetOrientation());
                    me->UpdatePosition(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetHomePosition().GetOrientation());
                });
            }

            void EnterEvadeMode(EvadeReason) override { } // No, I don't want you to trigger Reset

        private:
            EventMap events;
            TaskScheduler _scheduler;
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
            return new npc_ds_events_unfold_kalecgosAI(creature);
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

        bool Load() override
        {
            sharedTargets.clear();
            return true;
        }

        void FilterTwilightFire(std::list<WorldObject*>& targets)
        {
            sharedTargets = targets;
            sharedTargets.remove_if([this](WorldObject* target)
            {
                return !(target->ToUnit() && target->ToUnit()->HasAuraEffect(m_scriptSpellId, EFFECT_1));
            });

            targets.remove_if([this](WorldObject* target)
            {
                Creature* dummy = target->ToCreature();
                if (!dummy)
                    return true;

                if (dummy->GetEntry() == NPC_TWILIGHT_ASSAULT_FIRE && dummy->IsAIEnabled)
                {
                    uint32 id = dummy->AI()->GetData(DATA_TWILIGHT_FLAMES);
                    switch (id)
                    {
                        case DATA_TWILIGHT_FLAMES_GROUP_1:
                        case DATA_TWILIGHT_FLAMES_GROUP_2:
                        case DATA_TWILIGHT_FLAMES_GROUP_3:
                        case DATA_TWILIGHT_FLAMES_GROUP_4:
                        case DATA_TWILIGHT_FLAMES_GROUP_5:
                        case DATA_TWILIGHT_FLAMES_GROUP_6:
                        case DATA_TWILIGHT_FLAMES_GROUP_7:
                        case DATA_TWILIGHT_FLAMES_GROUP_8:
                        case DATA_TWILIGHT_FLAMES_GROUP_9:
                        case DATA_TWILIGHT_FLAMES_GROUP_10:
                        case DATA_TWILIGHT_FLAMES_GROUP_11:
                        case DATA_TWILIGHT_FLAMES_GROUP_12:
                            return m_scriptSpellId != TwilightFlamesMapping[id];
                        default:
                            return true;
                    }

                    return true;
                }
            });

            targets.sort(Trinity::ObjectDistanceOrderPred(GetCaster()));
            if (WorldObject* target = targets.front())
                sharedTargets.push_back(target);

            targets = sharedTargets;
        }

        void CopyTargets(std::list<WorldObject*>& targets)
        {
            targets = sharedTargets;
        }

        void Register() override
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_ds_twilight_flames_dummy_target_SpellScript::FilterTwilightFire, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_ds_twilight_flames_dummy_target_SpellScript::CopyTargets, EFFECT_1, TARGET_UNIT_SRC_AREA_ENTRY);
        }

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

// http://www.wowhead.com/spell=109502/events-unfold
// http://www.wowhead.com/spell=109498/events-unfold
// http://www.wowhead.com/spell=109500/events-unfold
// http://www.wowhead.com/spell=109499/events-unfold
// http://www.wowhead.com/spell=109501/events-unfold
class spell_ds_events_unfold_summon_actor_one : public SpellScriptLoader
{
    public:
        spell_ds_events_unfold_summon_actor_one() : SpellScriptLoader("spell_ds_events_unfold_summon_actor_one") { }

        class spell_ds_events_unfold_summon_actor_one_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_ds_events_unfold_summon_actor_one_SpellScript);

            void SelectActor(WorldObject*& target)
            {
                target = nullptr;
                uint32 actorEntry = NULL;
                switch (m_scriptSpellId)
                {
                    case SPELL_EVENTS_UNFOLD_HAGARA_SUMMON_YSERA:
                        actorEntry = NPC_YSERA_WYRMREST_TEMPLE;
                        break;
                    case SPELL_EVENTS_UNFOLD_HAGARA_SUMMON_NOZDORMU:
                        actorEntry = NPC_NOZDORMU_WYRMREST_TEMPLE;
                        break;
                    case SPELL_EVENTS_UNFOLD_HAGARA_SUMMON_THRALL:
                        actorEntry = NPC_THRALL_WYRMREST_TEMPLE;
                        break;
                    case SPELL_EVENTS_UNFOLD_HAGARA_SUMMON_KALECGOS:
                        actorEntry = NPC_KALECGOS_WYRMREST_TEMPLE;
                        break;
                    case SPELL_EVENTS_UNFOLD_HAGARA_SUMMON_ALEXSTRASZA:
                        actorEntry = NPC_ALEXSTRASZA_WYRMREST_TEMPLE;
                        break;
                    default:
                        break;
                }


                if (InstanceScript* instance = GetCaster()->GetInstanceScript())
                    if (Creature* actor = instance->GetCreature(actorEntry))
                        target = actor;

                if (!target)
                    FinishCast(SPELL_FAILED_DONT_REPORT);
            }

            void Register() override
            {
                OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_ds_events_unfold_summon_actor_one_SpellScript::SelectActor, EFFECT_0, TARGET_DEST_NEARBY_ENTRY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_ds_events_unfold_summon_actor_one_SpellScript();
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

    bool OnTrigger(Player* player, AreaTriggerEntry const* /*at*/, bool entered) override
    {
        if (!entered || player->IsGameMaster())
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
    new npc_ds_alexstrasza_part_one();
    new npc_ds_kalecgos_part_one();
    new npc_ds_ysera_part_one();
    new npc_ds_nozdormu_part_one();

    //new npc_ds_alexstrasza_events_unfold_hagara();
    //new npc_ds_kalegos_events_unfold_hagara();
    //new npc_ds_ysera_events_unfold_hagara();
    //new npc_ds_thrall_events_unfold_hagara();

    new npc_ultraxion_gauntlet();
    new npc_twilight_assaulter_flames();

    //new spell_ds_twilight_portal_beam();
    //new spell_ds_charge_dragon_soul();
    new spell_ds_twilight_flames_dummy_target();
    new spell_ds_events_unfold_hagara_summon();
    new spell_ds_open_eye_of_eternity_portal();

    new go_ds_the_focusing_iris();

    new at_ds_wyrmrest_summit();
}
