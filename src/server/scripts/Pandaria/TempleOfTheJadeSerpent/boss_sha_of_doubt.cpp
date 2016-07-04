/* ScriptData
SDName: boss_sha_of_doubt
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

class boss_sha_of_doubt : public CreatureScript
{
    public:
        boss_sha_of_doubt() : CreatureScript("boss_sha_of_doubt") {}

        struct boss_sha_of_doubtAI : public BossAI
        {
            boss_sha_of_doubtAI(Creature* creature) : BossAI(creature, BOSS_SHA_OF_DOUBT) { }
        };

		CreatureAI* GetAI(Creature* creature) const override
		{
			return new boss_sha_of_doubtAI(creature);
		}
};

void AddSC_boss_sha_of_doubt()
{
    new boss_sha_of_doubt();
}
