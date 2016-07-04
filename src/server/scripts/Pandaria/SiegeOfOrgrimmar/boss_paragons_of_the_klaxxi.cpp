#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "siege_of_orgrimmar.h"
#include "PassiveAI.h"



enum Spells
{
	// The Klaxxi Paragons
	SPELL_SUMMON_FIGHT_STARTER	= 143552,
	SPELL_READY_TO_FIGHT		= 143542,
	SPELL_BERSERK				= 146983,
	SPELL_PING_AURA				= 144354, // Reset if no players available?
	SPELL_STRONG_LEGS			= 148650,
	SPELL_BLOODTHIRSTY			= 148655,
	SPELL_COMPOUND_EYE			= 148651,
	SPELL_WIN					= 148512,

	SPELL_JUMP_TO_CENTER		= 143545,
	SPELL_BEEN_CLICKED			= 144834,
	SPELL_DEFEATED				= 142292,

	// Kil'ruk the Wind-Reaver
	SPELL_KILRUK_THE_WIND_REAVER = 142926,

};

// http://www.wowhead.com/npc=71592/the-klaxxi-paragons
class npc_the_klaxxi_paragons : public CreatureScript
{
public:
	npc_the_klaxxi_paragons() : CreatureScript("npc_the_klaxxi_paragons") { }

	struct npc_the_klaxxi_paragonsAI : public NullCreatureAI
	{
		npc_the_klaxxi_paragonsAI(Creature* creature) : NullCreatureAI(creature) { }


	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return new npc_the_klaxxi_paragonsAI(creature);
	}
};

void AddSC_boss_paragons_of_the_klaxxi()
{
}
