#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "temple_of_the_jade_serpent.h"

class TerraceFighterPredicate
{
	public:
		bool operator() (Unit const* object, bool sourceIsTerraceFighter) const
		{
			switch (object->GetTypeId())
			{
				case TYPEID_UNIT:
					switch (object->GetEntry())
					{
						case NPC_LESSER_SHA:
						case NPC_MINION_OF_DOUBT_TERRACE:
						case NPC_SERPENT_WARRIOR:
							return sourceIsTerraceFighter;
						default:
							return !sourceIsTerraceFighter;
					}
				default:
					return true;
			}
		}
};

struct terraceFighterAI : public NewReset_ScriptedAI
{
    terraceFighterAI(Creature* creature) : NewReset_ScriptedAI(creature) { }

    void FullReset(bool initialize = false) override
    {
        NewReset_ScriptedAI::FullReset(initialize);
    }

	bool CanAIAttack(Unit const* target) const override
	{
		TerraceFighterPredicate pred;
		return pred(target, true);
	}

    void DamageTaken(Unit* attacker, uint32& damage) override
    {
        if (!attacker->IsControlledByPlayer() && me->HealthBelowPctDamaged(75, damage))
            damage = 0;
    }
};

// http://www.wowhead.com/npc=62171/serpent-warrior
class npc_totjs_serpent_warrior : public CreatureScript
{
public:
	npc_totjs_serpent_warrior() : CreatureScript("npc_totjs_serpent_warrior") { }

	struct npc_totjs_serpent_warriorAI : public terraceFighterAI
	{
		npc_totjs_serpent_warriorAI(Creature* creature) : terraceFighterAI(creature) { }

		void DoAction(int32 action) override
		{
			switch (action)
			{
				case ACTION_TERRACE_FIGHTERS_RETREAT:
					me->DespawnOrUnsummon();
					break;
				default:
					break;
			}
		}

	private:
		TaskScheduler scheduler;
	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return new npc_totjs_serpent_warriorAI(creature);
	}
};

// http://www.wowhead.com/npc=58319/lesser-sha
class npc_totjs_lesser_sha_terrace : public CreatureScript
{
public:
	npc_totjs_lesser_sha_terrace() : CreatureScript("npc_totjs_lesser_sha_terrace") { }

	struct npc_totjs_lesser_sha_terraceAI : public terraceFighterAI
	{
		npc_totjs_lesser_sha_terraceAI(Creature* creature) : terraceFighterAI(creature) { }

	private:
		TaskScheduler scheduler;
	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return new npc_totjs_lesser_sha_terraceAI(creature);
	}
};

// http://www.wowhead.com/npc=57109/minion-of-doubt
class npc_totjs_minion_of_doubt_terrace : public CreatureScript
{
public:
	npc_totjs_minion_of_doubt_terrace() : CreatureScript("npc_totjs_minion_of_doubt_terrace") { }

	struct npc_totjs_minion_of_doubt_terraceAI : public terraceFighterAI
	{
		npc_totjs_minion_of_doubt_terraceAI(Creature* creature) : terraceFighterAI(creature) { }

	private:
		TaskScheduler scheduler;
	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return new npc_totjs_minion_of_doubt_terraceAI(creature);
	}
};

// http://www.wowhead.com/npc=65362/minion-of-doubt
class npc_totjs_minion_of_doubt_temple : public CreatureScript
{
public:
	npc_totjs_minion_of_doubt_temple() : CreatureScript("npc_totjs_minion_of_doubt_temple") { }

    struct npc_totjs_minion_of_doubt_templeAI : public NewReset_ScriptedAI
    {
		npc_totjs_minion_of_doubt_templeAI(Creature* creature) : NewReset_ScriptedAI(creature) { }

		bool CanAIAttack(Unit const* target) const override
		{
			TerraceFighterPredicate pred;
			return pred(target, false);
		}

    private:
        TaskScheduler scheduler;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_totjs_minion_of_doubt_templeAI(creature);
    }
};

void AddSC_temple_of_the_jade_serpent()
{
	new npc_totjs_serpent_warrior();
	new npc_totjs_lesser_sha_terrace();
	new npc_totjs_minion_of_doubt_terrace();
	new npc_totjs_minion_of_doubt_temple();
}
