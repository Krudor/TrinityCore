/* ScriptData
SDName: boss_garrosh_hellscream
SD%Complete: 0
SDComment: Placeholder
SDCategory: Siege of Orgrimmar
EndScriptData */

/*---Comments---
    Kor'kron Warbringer:
    "Grunts stream in from the sides of the room and join the battle more frequently as the phase progresses."
    Not enough sample size from phase 1 to set timers accordingly.
    Wave 1 spawn as soon as the fight starts.
    Wave 2 spawns 45 seconds after the first wave.
    Wave 3 spawns 45 seconds after the second wave.
    Wave 4 and onwards is unknown.

	Getting pulled into the Heart of Y'shaarj plays a scene with the following information:
	SceneID: 270
	PlaybackFlags: 20
	SceneInstanceID: 6
	SceneScriptPackageID: 560
	Players cast spell 144868 to jump to the Heart of Y'shaarj but it also applies a currently unknown aura type (430) with misc value 270 indicating the scene id.
	Source of PlaybackFlags, SceneInstanceID and SceneScriptPackageID are currently unknown.

	Going into the heroic only phase, the following scene is sent to players:
	SceneID: 271
	PlaybackFlags: 16
	SceneInstanceID: 7
	SceneScriptPackageID: 561
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "siege_of_orgrimmar.h"
#include "PassiveAI.h"

#define HEALTH_PHASE_2_3          10
#define HEALTH_PHASE_4          2

enum Spells
{
    //Thrall
    SPELL_CALL_OF_THE_ELEMENTS      = 147815,
    SPELL_ATTACK_GARROSH            = 147892,
    SPELL_ASTRAL_RECALL             = 147811,
    SPELL_EXHAUSTED                 = 147861,

    //Garrosh Hellscream
    SPELL_ZERO_ENERGY_ZERO_REGEN    = 72242,

    //Intro
    SPELL_DEFEND_AGAINST_THRALL     = 147899,
    SPELL_ATTACK_THRALL             = 147831,

    //Phase 1
    SPELL_SUMMON_ADDS               = 144489,
    SPELL_DESECRATE                 = 144745,
    SPELL_HELLSCREAMS_WARSONG       = 144821,
    SPELL_THROW_AXE_AT_HEART        = 145235,
    SPELL_TRANSITION_VISUAL         = 146756,
    SPELL_PHASE_TWO_TRANSFORM       = 144842,
    SPELL_JUMP_TO_GROUND            = 144956,
    SPELL_JUMP_TO_GROUND_2          = 145231,

    //Phase 2 & 3 (Same shit, phase 3 is just phase 2 forced garrosh to 100 power)
    SPELL_GRIPPING_DESPAIR          = 145183,
    SPELL_EMPOWERED_GRIPPING_DESPAIR = 145831,
    SPELL_EMPOWERED_GRIPPING_DESPAIR_2 = 145195,
    SPELL_GRIPPING_DESPAIR_SCRIPT   = 145207,
    SPELL_TOUCH_OF_YSHAARJ          = 145065,
    SPELL_TOUCH_OF_YSHAARJ_UNK      = 145163,
    SPELL_EMPOWERED_TOUCH_OF_YSHAARJ = 145171,
    SPELL_EMPOWERED_TOUCH_OF_YSHAARJ_2 = 145832,
    SPELL_WHIRLING_CORRUPTION       = 144985,
    SPELL_WHIRLING_CORRUPTION_UNK   = 145036,
    SPELL_WHIRLING_CORRUPTION_3 = 144994,
    SPELL_EMPOWERED_WHIRLING_CORRUPTION = 145023,
    SPELL_EMPOWERED_WHIRLING_CORRUPTION_2 = 145833,
    SPELL_EMPOWERED_DESECRATE_2     = 145829,

    SPELL_CANCEL_REALM_OF_YSHAARJ   = 145647,
    SPELL_TRANSITION_VISUAL_2       = 146845,
    SPELL_PHASE_THREE_TRANSFORM     = 145222, //Garrosh > Heart of Y'Shaarj
    SPELL_PHASE_THREE_TRANSFORM_2   = 145598,

    //Transition
    SPELL_ENTER_REALM_OF_YSHAARJ    = 144866,
	SPELL_ENTER_REALM_OF_YSHAARJ_AOE	= 144867,
    SPELL_WEAK_MINDED               = 145331,
    SPELL_ENTER_REALM_OF_YSHAARJ_2  = 144870,
    SPELL_TELEPORT_JADE_SERPENT		= 144880, //Temple of the Jade Serpent
    SPELL_TELEPORT_RED_CRANE        = 144883, //Temple of the Red Crane
    SPELL_TELEPORT_ENLESS_SPRING    = 144885, //Terrace of Endless Spring
    SPELL_REALM_OF_YSHAARJ          = 144954,
    SPELL_REALM_OF_YSHAARJ_VISUAL   = 144958,
    SPELL_ENTER_REALM_OF_YSHAARJ_3  = 144878,
    SPELL_BREAK_PLAYER_TARGETTING   = 140562,
    SPELL_YSHAARJ_PROTECTION        = 144945,
    SPELL_ABSORB_POWER              = 144946,
    SPELL_CONSUMED_COURAGE          = 149011, //Terrace of Endless Spring Scenario
    SPELL_ANNIHILATE                = 144970,

    //Heroic (TODO)
    //

    //Outro
    SPELL_SUMMON_FADING_BREATH      = 147296,
    SPELL_PHASE_THREE_DETRANSFORM   = 147893,
    SPELL_PHASE_THREE_DETRANSFORM_2 = 147608,
    SPELL_PHASE_THREE_DETRANSFORM_3 = 148806,
    SPELL_PHASE_THREE_DETRANSFORM_4 = 146966,
    SPELL_PHASE_THREE_DETRANSFORM_5 = 148185,
    //Garrosh Hellscream END

    //Siege Engineer
    SPELL_POWER_IRON_STAR           = 144616,
    
    //Iron Star
    SPELL_IRON_STAR_IMPACT_PLAYER   = 144645,
    SPELL_IRON_STAR_IMPACT_CREATURE = 149468,
	SPELL_IRON_STAR_EXPLOSION		= 144798,
};

enum Events
{
    EVENT_NONE,
	EVENT_INTRO_CALL_ELEMENTS,
	EVENT_SET_EMOTESTATE_TALKING,
	EVENT_SET_UNARMED,
	EVENT_SET_ARMED,
	EVENT_RESET_EMOTESTATE,
	EVENT_SET_SPECIAL_AI_KIT,
	EVENT_RESET_AI_KIT,
	EVENT_INTRO_1,
	EVENT_INTRO_2,
	EVENT_INTRO_3,
	EVENT_INTRO_4,
	EVENT_INTRO_5,
	EVENT_INTRO_6,
	EVENT_INTRO_7,
	EVENT_INTRO_8,
	EVENT_INTRO_9,
	EVENT_INTRO_10,
    EVENT_SUMMON_SIEGE_ENGINEER,
    EVENT_SUMMON_KORKRON_WARBRINGER,
    EVENT_SUMMON_FARSEER_WOLF_RIDER,
    EVENT_HELLSCREAMS_WARSONG,
    EVENT_DESECRATE,

    EVENT_UNLOCK_VOCAL_CORDS,
    EVENT_BERSERK,

    EVENT_GROUP_NORMAL = 1,
};

enum Actions
{
	ACTION_NONE,
	
	ACTION_INTRO_1,
	ACTION_INTRO_2,
	ACTION_INTRO_3,
	ACTION_ACTIVATE_IRON_STAR,
};

enum Texts
{
	// Thrall
	SAY_THRALL_INTRO_1				= 0,
	SAY_THRALL_INTRO_2				= 1,
	SAY_THRALL_INTRO_3				= 2,
	SAY_THRALL_INTRO_4				= 3,

	// Garrosh Hellscream
	// Intro
	SAY_INTRO_1						= 0,
	SAY_INTRO_2						= 1,
	SAY_INTRO_3						= 2,
	SAY_INTRO_4						= 3,
	SAY_INTRO_5						= 4,

	SAY_UNK_1						= 5,

	SAY_ENTER_COMBAT				= 6,
	SAY_KILL						= 7,
	SAY_WIPE						= 8,
	SAY_BERSERK						= 9,
	SAY_DEFEAT_NORMAL				= 10,
	SAY_DEFEAT_HEROIC				= 11,

	SAY_KORKRON_IRON_STAR			= 12,
	SAY_KORKRON_WARBRINGER			= 13,
	SAY_KORKRON_FARSEER				= 14,
	SAY_HELLSCREAMS_WARSONG			= 15,
	SAY_WHIRLING_CORRUPTION			= 16,
	SAY_E_WHIRLING_CORRUPION		= 17,

	SAY_PHASE_2_1					= 18,
	SAY_PHASE_2_2					= 19,
	SAY_REALM_OF_YSHAARJ			= 20,

	SAY_PHASE_3						= 21,

	SAY_PHASE_4_1					= 22,
	SAY_PHASE_4_2					= 23,

	SAY_MANIFEST_RAGE				= 24,
	SAY_CALL_BOMBARDMENT			= 25,

	EMOTE_SIEGE_ENGINEERS			= 26,
	EMOTE_WHIRLING_CORRUPTION		= 27,
	EMOTE_E_WHIRLING_CORRUPTION		= 28,
	EMOTE_WHIRLING_CORRUPTION_TIP	= 29,
	EMOTE_MANIFEST_RAGE				= 30,
	EMOTE_BOMBARDMENT				= 31,
	EMOTE_UNSTABLE_IRON_STAR		= 32,
	EMOTE_MALICE					= 33,


 //   TALK_AGGRO							= 5, //
	//TALK_IRON_STAR						= 6,
 //   TALK_HELLSCREAM_WARSONG				= 7,
	//TALK_FARSEERS						= 8,

	//TALK_PHASE_2_1						= 9,
	//TALK_PHASE_2_2						= 10,

	//TALK_REALM_OF_YSHAARJ				= 11,

	//TALK_WHIRLING_CORRUPTION			= 12,
	//TALK_EMPOWERED_WHIRLING_CORRUPTION	= 13,

	//TALK_PHASE_3						= 14,
	//
	//TALK_PHASE_4_1						= 15,
	//TALK_PHASE_4_2						= 16,

	//TALK_CALL_BOMBARDMENT				= 17,
	//TALK_MANIFEST_RAGE					= 18,
	//TALK_BERSERK						= 19,
	//TALK_KILL							= 20,
	//TALK_WIPE							= 21,
	//TALK_DEATH_NORMAL					= 22,
	//TALK_DEATH_ALTERNATE				= 23,

 //   EMOTE_SIEGE_ENGINEERS				= 24,
};

enum CreatureGroups
{
	CG_WARBRINGERS		= 0,
	CG_FARSEER_1		= 1,
	CG_FARSEER_2		= 2,
	CG_SIEGE_ENGINEERS	= 3,
	CG_IRON_STARS		= 4,
};

enum Phases
{
    PHASE_NONE,
	PHASE_INTRO,
    PHASE_ONE,
    PHASE_TWO,
    PHASE_THREE,
    PHASE_FOUR,
    PHASE_INTERMISSION,

	PHASE_MASK_INTRO	= 1 << PHASE_INTRO,
    PHASE_MASK_ONE		= 1 << PHASE_ONE,
    PHASE_MASK_TWO		= 1 << PHASE_TWO,
    PHASE_MASK_THREE	= 1 << PHASE_THREE,
};

enum Points
{
	POINT_INTRO,
};

enum AnimKits
{
	ANIM_KIT_4448 = 4448,
};

struct SiegeEngineerInfo {
	Position Spawn;
	Position Target;
};

Position SiegeEngineerSpawns[] = {
	{ 1085.594f, -5516.162f, -304.4025f, 4.552298f },
	{ 1059.965f, -5763.319f, -301.9454f, 0.9641288f },
};

Position SiegeEngineerTargets[] = {
	{ 1085.269f, -5518.175f, -304.4649f, 4.552298f },
	{ 1060.599f, -5762.407f, -303.6003f, 0.9641288f }
};

Position GarroshIntroPosition = { 1028.61f, -5633.94f, -317.6976f };
Position ThrallIntroPosition = { 1035.373f, -5635.088f, -317.7258f };

Position RealmOfYshaarjExitPosition = { 1068.59f, -5640.31f, -277.636f };
Position RealmOfYshaarjJumpTarget = { 1072.880615f, -5639.467773f, -317.4f }; //Not taken from sniff, custm

class boss_garrosh_hellscream : public CreatureScript
{
    public:
        boss_garrosh_hellscream() : CreatureScript("boss_garrosh_hellscream") {}

        struct boss_garrosh_hellscreamAI : public BossAI
        {
            boss_garrosh_hellscreamAI(Creature* creature) : BossAI(creature, BOSS_GARROSH_HELLSCREAM) { }

            void Reset()
            {
				me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
                _Reset();
                SetEquipmentSlots(true);
				me->SetCurrentEquipmentId(1);
				me->SetSheath(SHEATH_STATE_MELEE);
                DoCastAOE(SPELL_ZERO_ENERGY_ZERO_REGEN, true);
                warbringerWaveCnt = 0, farseerWaveCnt = 0;
				farseerSpawn = urand(CG_FARSEER_1, CG_FARSEER_2);
                canYell = true;
				atIntroFight = false;
				yellIronStar = false;
				me->SummonCreatureGroup(CG_IRON_STARS);
            }

			void SpellHitTarget(Unit* target, const SpellInfo* spellInfo) override
			{
				switch (spellInfo->Id)
				{
					case SPELL_ENTER_REALM_OF_YSHAARJ:
						DoCastAOE(SPELL_WEAK_MINDED, true);
						break;
				}
			}

			void DoAction(int32 action) override
			{
				switch (action)
				{
					case ACTION_GARROSH_INTRO:
						if (!events.IsInPhase(PHASE_NONE))
							break;

						if (Creature* thrall = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_GARROSH_THRALL)))
							if (thrall->IsAIEnabled)
								thrall->GetAI()->DoAction(ACTION_INTRO_1);

						events.SetPhase(PHASE_INTRO);
						events.ScheduleEvent(EVENT_INTRO_1, Milliseconds(12400), 0, PHASE_INTRO);
						break;
					case ACTION_ACTIVATE_IRON_STAR:
						if (!yellIronStar || !events.IsInPhase(PHASE_ONE))
							break;

						yellIronStar = false;
						Talk(SAY_KORKRON_IRON_STAR);
						break;
					default:
						break;
				}
			}

            void EnterCombat(Unit* /*who*/)
            {
                _EnterCombat();
                
                Talk(SAY_ENTER_COMBAT);

				Talk(EMOTE_SIEGE_ENGINEERS);
				me->SummonCreatureGroup(CG_SIEGE_ENGINEERS);

                if (instance)
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
            }

			void EnterEvadeMode(EvadeReason why) override
			{
				if (why == EVADE_REASON_NO_HOSTILES)
					Talk(SAY_WIPE);

				summons.DespawnAll();
				BossAI::EnterEvadeMode();
				_DespawnAtEvade();
			}

            void KilledUnit(Unit* who)
            {
                if (who->GetTypeId() == TYPEID_PLAYER && canYell)
                {
                    Talk(SAY_KILL);
                    canYell = false;
                    events.ScheduleEvent(EVENT_UNLOCK_VOCAL_CORDS, 8 * IN_MILLISECONDS);
                }
            }

            void DamageTaken(Unit* /*attacker*/, uint32& damage)
            {
                if (events.GetPhaseMask() & PHASE_MASK_ONE && me->HealthBelowPctDamaged(HEALTH_PHASE_2_3, damage))
                    PreparePhase(PHASE_THREE, PHASE_ONE);
            }

            void OnAddThreat(Unit* /*victim*/, float& fThreat, SpellSchoolMask /*schoolMask*/, SpellInfo const* /*threatSpell*/)
            {
                if (me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE))
                    fThreat = 0;
            }

            void JustSummoned(Creature* summon)
            {
                summons.Summon(summon);

                switch (summon->GetEntry())
                {
                    case NPC_SIEGE_ENGINEER:
                        instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, summon);
                        break;
                    default:
                        break;
                }
            }

            void SummonedCreatureDies(Creature* summon, Unit* /*killer*/)
            {
                if (summon->GetEntry() == NPC_SIEGE_ENGINEER)
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, summon);
            }

            void SummonedCreatureDespawn(Creature* summon)
            {
                summons.Despawn(summon);

                if (summon->GetEntry() == NPC_SIEGE_ENGINEER)
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, summon);
            }

            void PreparePhase(Phases _phase, Phases _previousPhase = PHASE_NONE)
            {
                events.SetPhase(_phase);

                switch (_phase)
                {
                    case PHASE_ONE:
                        DoCastAOE(SPELL_SUMMON_ADDS, true);
                        events.ScheduleEvent(EVENT_DESECRATE, 11 * IN_MILLISECONDS, EVENT_GROUP_NORMAL);
                        events.ScheduleEvent(EVENT_SUMMON_SIEGE_ENGINEER, 20 * IN_MILLISECONDS, 0, PHASE_ONE);
                        events.ScheduleEvent(EVENT_HELLSCREAMS_WARSONG, 22 * IN_MILLISECONDS, 0, PHASE_ONE);
                        events.ScheduleEvent(EVENT_SUMMON_FARSEER_WOLF_RIDER, 30 * IN_MILLISECONDS, 0, PHASE_ONE);
                        events.ScheduleEvent(EVENT_SUMMON_KORKRON_WARBRINGER, 45 * IN_MILLISECONDS, 0, PHASE_ONE);

                        if (GetDifficulty() == DIFFICULTY_LFR)
                            events.ScheduleEvent(EVENT_BERSERK, 25 * MINUTE * IN_MILLISECONDS);
                        else
                            events.ScheduleEvent(EVENT_BERSERK, 18 * MINUTE * IN_MILLISECONDS);
                        break;
                    case PHASE_TWO:
                        break;
                    case PHASE_THREE:
                        break;
                    case PHASE_FOUR:
                        break;
                    default:
                        break;
                }
            }

            bool CanAIAttack(Unit const* target) const
            {
                return !me->HasAura(SPELL_REALM_OF_YSHAARJ_VISUAL);
            }

			void UpdateAI(uint32 diff) override
			{
				UpdateVictim();

				events.Update(diff);

				while (uint32 eventId = events.ExecuteEvent())
				{
					switch (eventId)
					{
						case EVENT_INTRO_1:
							Talk(SAY_INTRO_1);
							me->SetAIAnimKitId(ANIM_KIT_4448);
							me->SetSheath(SHEATH_STATE_UNARMED);
							events.ScheduleEvent(EVENT_INTRO_2, Milliseconds(21500), 0, PHASE_INTRO);
							break;
						case EVENT_INTRO_2:
							Talk(SAY_INTRO_2);
							events.ScheduleEvent(EVENT_INTRO_3, Milliseconds(13500), 0, PHASE_INTRO);
							break;
						case EVENT_INTRO_3:
							Talk(SAY_INTRO_3);
							me->SetAIAnimKitId(0);
							me->HandleEmoteCommand(EMOTE_ONESHOT_ROAR);
							events.ScheduleEvent(EVENT_SET_SPECIAL_AI_KIT, Seconds(6), 0, PHASE_INTRO);
							events.ScheduleEvent(EVENT_INTRO_4, Milliseconds(18300), 0, PHASE_INTRO);
							events.ScheduleEvent(EVENT_INTRO_5, Seconds(33), 0, PHASE_INTRO);
							break;
						case EVENT_INTRO_4:
							me->SetAIAnimKitId(0);
							me->SetSheath(SHEATH_STATE_MELEE);
							if (Creature* thrall = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_GARROSH_THRALL)))
								if (thrall->IsAIEnabled)
									thrall->GetAI()->DoAction(ACTION_INTRO_2);
							break;
						case EVENT_INTRO_5:
							Talk(SAY_INTRO_4);
							me->SetAIAnimKitId(ANIM_KIT_4448);
							me->SetSheath(SHEATH_STATE_UNARMED);
							events.ScheduleEvent(EVENT_RESET_AI_KIT, Milliseconds(21500), 0, PHASE_INTRO);
							events.ScheduleEvent(EVENT_INTRO_6, Seconds(29), 0, PHASE_INTRO);
							break;
						case EVENT_INTRO_6:
							Talk(SAY_INTRO_5);
							me->SetAIAnimKitId(ANIM_KIT_4448);
							events.ScheduleEvent(EVENT_INTRO_7, Milliseconds(1200), 0, PHASE_INTRO);
							break;
						case EVENT_INTRO_7:
							me->SetAIAnimKitId(0);
							me->SetSheath(SHEATH_STATE_MELEE);
							me->GetMotionMaster()->MovePoint(POINT_INTRO, GarroshIntroPosition, true);
							events.ScheduleEvent(EVENT_INTRO_8, Milliseconds(7500), 0, PHASE_INTRO);
							break;
						case EVENT_INTRO_8:
							me->CastSpell(me, SPELL_DEFEND_AGAINST_THRALL);
							if (Creature* thrall = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_GARROSH_THRALL)))
								thrall->CastSpell(thrall, SPELL_ATTACK_GARROSH);
							events.ScheduleEvent(EVENT_INTRO_9, Milliseconds(2800), 0, PHASE_INTRO);
							break;
						case EVENT_INTRO_9:
							me->CastSpell(me, SPELL_ATTACK_THRALL, true);
							events.ScheduleEvent(EVENT_INTRO_10, Seconds(3), 0, PHASE_INTRO);
							break;
						case EVENT_INTRO_10:
							me->HandleEmoteCommand(EMOTE_ONESHOT_LAUGH);
							me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
							break;
						case EVENT_SET_SPECIAL_AI_KIT:
							me->SetAIAnimKitId(ANIM_KIT_4448);
							me->SetSheath(SHEATH_STATE_UNARMED);
							break;
						case EVENT_RESET_AI_KIT:
							me->SetAIAnimKitId(0);
							me->SetSheath(SHEATH_STATE_MELEE);
							break;
						case EVENT_DESECRATE:
							DoCastAOE(SPELL_DESECRATE, true);
							events.ScheduleEvent(EVENT_DESECRATE, 41 * IN_MILLISECONDS, EVENT_GROUP_NORMAL);
							break;
						case EVENT_SUMMON_KORKRON_WARBRINGER:
						{
							++warbringerWaveCnt;
							me->SummonCreatureGroup(CG_WARBRINGERS);
							int _timer = 45;
							if (warbringerWaveCnt > 3)
								_timer = _timer - ((warbringerWaveCnt - 3) * 5) > 5 ? (warbringerWaveCnt - 3) * 5 : 5;
							events.ScheduleEvent(EVENT_SUMMON_KORKRON_WARBRINGER, _timer * IN_MILLISECONDS, 0, PHASE_ONE);
						}
						break;
						case EVENT_SUMMON_SIEGE_ENGINEER:
							Talk(EMOTE_SIEGE_ENGINEERS);
							me->SummonCreatureGroup(CG_SIEGE_ENGINEERS);
							events.ScheduleEvent(EVENT_SUMMON_SIEGE_ENGINEER, 45 * IN_MILLISECONDS, 0, PHASE_ONE);
							break;
						case EVENT_HELLSCREAMS_WARSONG:
							DoCastAOE(SPELL_HELLSCREAMS_WARSONG);
							events.ScheduleEvent(EVENT_HELLSCREAMS_WARSONG, 41 * IN_MILLISECONDS, 0, PHASE_ONE);
							break;
						case EVENT_SUMMON_FARSEER_WOLF_RIDER:
						{
							++farseerWaveCnt;
							me->SummonCreatureGroup(farseerSpawn);
							if (farseerSpawn == CG_FARSEER_1)
								farseerSpawn = CG_FARSEER_2;
							else
								farseerSpawn = CG_FARSEER_1;

							int _timer = 50;
							if (farseerWaveCnt > 3)
								_timer = _timer - ((farseerWaveCnt - 3) * 10) > 10 ? (farseerWaveCnt - 3) * 10 : 10;

							events.ScheduleEvent(EVENT_SUMMON_FARSEER_WOLF_RIDER, _timer * IN_MILLISECONDS, 0, PHASE_ONE);
						}
						break;
						default:
							break;
					}
                }

				DoMeleeAttackIfReady();
            }

        private:
            int32 warbringerWaveCnt, farseerWaveCnt;
			int8 farseerSpawn;
            bool atIntroFight, canYell, yellIronStar;
        };

		CreatureAI* GetAI(Creature* creature) const override
		{
			return new boss_garrosh_hellscreamAI(creature);
		}
};

// http://www.wowhead.com/npc=73483/thrall
class npc_garrosh_thrall : public CreatureScript
{
	public:
		npc_garrosh_thrall() : CreatureScript("npc_garrosh_thrall") { }

		struct npc_garrosh_thrallAI : public NullCreatureAI
		{
			npc_garrosh_thrallAI(Creature* creature) : NullCreatureAI(creature)
			{
				creature->SetCurrentEquipmentId(1);
			}

			void DoAction(int32 action) override
			{
				switch (action)
				{
					case ACTION_INTRO_1:
						Talk(SAY_THRALL_INTRO_1);
						events.SetPhase(PHASE_INTRO);
						events.ScheduleEvent(EVENT_SET_EMOTESTATE_TALKING, Milliseconds(400));
						events.ScheduleEvent(EVENT_RESET_EMOTESTATE, Milliseconds(12100));
						break;
					case ACTION_INTRO_2:
						Talk(SAY_THRALL_INTRO_2);
						events.ScheduleEvent(EVENT_SET_EMOTESTATE_TALKING, Milliseconds(400));
						events.ScheduleEvent(EVENT_INTRO_CALL_ELEMENTS, Milliseconds(8400));
						break;
					default:
						break;
				}
			}

			void SpellHit(Unit* caster, const SpellInfo* spell) override
			{
				if (spell->Id == SPELL_CALL_OF_THE_ELEMENTS)
				{
					events.ScheduleEvent(EVENT_INTRO_1, 16 * IN_MILLISECONDS, 0, PHASE_INTRO);
				}
			}

			void MovementInform(uint32 type, uint32 pointId) override
			{
			}

			void UpdateAI(uint32 diff) override
			{
				UpdateVictim();

				events.Update(diff);

				while (uint32 eventId = events.ExecuteEvent())
				{
					switch (eventId)
					{
						case EVENT_INTRO_CALL_ELEMENTS:
							me->CastSpell(me, SPELL_CALL_OF_THE_ELEMENTS);
							break;
						case EVENT_INTRO_1:
							Talk(SAY_THRALL_INTRO_3);
							me->SetSheath(SHEATH_STATE_MELEE);
							me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_READY1H);
							events.ScheduleEvent(EVENT_INTRO_2, Seconds(9), 0, PHASE_INTRO);
							break;
						case EVENT_INTRO_2:
							me->GetMotionMaster()->MovePoint(POINT_INTRO, ThrallIntroPosition, true);
							break;
						case EVENT_SET_EMOTESTATE_TALKING:
							me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_TALK);
							break;
						case EVENT_RESET_EMOTESTATE:
							me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_ONESHOT_NONE);
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
			return new npc_garrosh_thrallAI(creature);
		}
};

// http://www.wowhead.com/npc=72215/heart-of-yshaarj
class npc_heart_of_yshaarj_main_room : public CreatureScript
{
	public:
		npc_heart_of_yshaarj_main_room() : CreatureScript("npc_heart_of_yshaarj_main_room") { }

		struct npc_heart_of_yshaarj_main_roomAI : public NullCreatureAI
		{
			npc_heart_of_yshaarj_main_roomAI(Creature* creature) : NullCreatureAI(creature) { }

			void SpellHit(Unit* caster, const SpellInfo* spell) override
			{
				switch (spell->Id)
				{
					case SPELL_ENTER_REALM_OF_YSHAARJ:
						DoCastAOE(SPELL_ENTER_REALM_OF_YSHAARJ_AOE, true);
						break;
				}
			}
		};

		CreatureAI* GetAI(Creature* creature) const override
		{
			return new npc_heart_of_yshaarj_main_roomAI(creature);
		}
};

// http://www.wowhead.com/npc=71985/kokron-iron-star
class npc_korkron_iron_star : public CreatureScript
{
public:
	npc_korkron_iron_star() : CreatureScript("npc_korkron_iron_star") { }

	struct npc_korkron_iron_starAI : public NullCreatureAI
	{
		npc_korkron_iron_starAI(Creature* creature) : NullCreatureAI(creature) { }

		void JustDied(Unit* killer) override
		{
			me->DespawnOrUnsummon(3 * IN_MILLISECONDS);
			me->SetRespawnTime(10 * IN_MILLISECONDS);
		}

		void DoAction(int32 action) override
		{
			switch (action)
			{
				case ACTION_ACTIVATE_IRON_STAR:
					me->CastSpell(me, SPELL_IRON_STAR_IMPACT_PLAYER, true);
					me->CastSpell(me, SPELL_IRON_STAR_IMPACT_CREATURE, true);

					if (InstanceScript* instance = me->GetInstanceScript())
						if (Creature* garrosh = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_GARROSH_HELLSCREAM)))
							if (garrosh->IsAIEnabled)
								garrosh->GetAI()->DoAction(ACTION_ACTIVATE_IRON_STAR);

					me->GetMotionMaster()->MovePoint(0, me->GetNearPosition(200.0f, 0.0f), false);
					break;
				default:
					break;
			}
		}

		void MovementInform(uint32 type, uint32 pointId) override
		{
			if (type != POINT_MOTION_TYPE)
				return;

			DoCastAOE(SPELL_IRON_STAR_EXPLOSION);
			me->KillSelf();
		}
	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return new npc_korkron_iron_starAI(creature);
	}
};

// http://www.wowhead.com/npc=71984/siege-engineer
class npc_garrosh_siege_engineer : public CreatureScript
{
	public:
		npc_garrosh_siege_engineer() : CreatureScript("npc_garrosh_siege_engineer") { }

		struct npc_garrosh_siege_engineerAI : public NullCreatureAI
		{
			npc_garrosh_siege_engineerAI(Creature* creature) : NullCreatureAI(creature)
			{
				me->SetWalk(false);
			}

			void Reset() override
			{
				Position toPos = NULL;
				for (Position pos : SiegeEngineerTargets)
				{
					if (toPos == NULL || me->GetDistance2d(toPos.GetPositionX(), toPos.GetPositionY()) > me->GetDistance2d(pos.GetPositionX(), pos.GetPositionY()))
						toPos = pos;
				}

				if (toPos == NULL)
				{
					me->DespawnOrUnsummon();
					return;
				}

				me->GetMotionMaster()->MovePoint(0, toPos);
			}

			void JustReachedHome() override
			{
				me->DespawnOrUnsummon();
			}

			void MovementInform(uint32 type, uint32 pointId) override
			{
				if (type == POINT_MOTION_TYPE)
					DoCastAOE(SPELL_POWER_IRON_STAR);
			}

			void DoAction(int32 action) override
			{
				switch (action)
				{
					case ACTION_ACTIVATE_IRON_STAR:
						me->GetMotionMaster()->MoveTargetedHome();
						break;
					default:
						break;
				}
			}
		};

		CreatureAI* GetAI(Creature* creature) const override
		{
			return new npc_garrosh_siege_engineerAI(creature);
		}
};


// http://www.wowhead.com/spell=144745
class spell_garrosh_hellscream_desecrate : public SpellScriptLoader
{
public:
    spell_garrosh_hellscream_desecrate() : SpellScriptLoader("spell_garrosh_hellscream_desecrate") {}

    class spell_garrosh_hellscream_desecrate_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_garrosh_hellscream_desecrate_SpellScript);

        bool Validate(SpellInfo const* /*spellInfo*/)
        {
            if (!sSpellMgr->GetSpellInfo(SPELL_DESECRATE))
                return false;

            return true;
        }

        void HandleScript(SpellEffIndex effIndex)
        {
			if (GetCaster()->HasAura(SPELL_EMPOWERED_DESECRATE_2))
			    GetCaster()->CastSpell(GetHitUnit(), GetSpellInfo()->GetEffect(EFFECT_1)->BasePoints, true);
			else
			    GetCaster()->CastSpell(GetHitUnit(), GetSpellInfo()->GetEffect(effIndex)->BasePoints, true);
        }

        void FilterTargets(std::list<WorldObject*>& targets)
        {
            std::list<WorldObject*> targetsBackup = targets;
            targets.remove_if(RangedClassOrSpecCheck());

            if (targets.empty())
            {
                if (WorldObject* target = Trinity::Containers::SelectRandomContainerElement(targetsBackup))
                    targets.push_back(target);
            }
            else
				Trinity::Containers::RandomResizeList(targets, 1);
        }

        void Register()
        {
            OnEffectHitTarget += SpellEffectFn(spell_garrosh_hellscream_desecrate_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_DUMMY);
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_garrosh_hellscream_desecrate_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_garrosh_hellscream_desecrate_SpellScript();
    }
};

// http://www.wowhead.com/spell=145331/weak-minded
class spell_garrosh_weak_minded : public SpellScriptLoader
{
public:
	spell_garrosh_weak_minded() : SpellScriptLoader("spell_garrosh_weak_minded") {}

	class spell_garrosh_weak_minded_SpellScript : public SpellScript
	{
		PrepareSpellScript(spell_garrosh_weak_minded_SpellScript);

		void FilterTargets(std::list<WorldObject*>& targets)
		{
			targets.remove_if([](WorldObject* target)
			{
				if (target->GetTypeId() != TYPEID_UNIT)
					return true;
				
				switch (target->GetEntry())
				{
					case NPC_KORKRON_WARBRINGER:
					case NPC_FARSEER_WOLF_RIDER:
						return false;
					default:
						return true;
				}
			});
		}

		void Register()
		{
			OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_garrosh_weak_minded_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
			OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_garrosh_weak_minded_SpellScript::FilterTargets, EFFECT_1, TARGET_UNIT_SRC_AREA_ENTRY);
		}
	};

	SpellScript* GetSpellScript() const
	{
		return new spell_garrosh_weak_minded_SpellScript();
	}
};

// http://www.wowhead.com/spell=144616/power-iron-star
class spell_power_iron_star : public SpellScriptLoader
{
public:
	spell_power_iron_star() : SpellScriptLoader("spell_power_iron_star") {}

	class spell_power_iron_star_SpellScript : public SpellScript
	{
		PrepareSpellScript(spell_power_iron_star_SpellScript);

		bool Validate(SpellInfo const* /*spellInfo*/) override
		{
			if (!sSpellMgr->GetSpellInfo(SPELL_POWER_IRON_STAR))
				return false;
			return true;
		}

		void GetIronStar(WorldObject*& target)
		{
			target = nullptr;
			if (Creature* ironStar = GetCaster()->FindNearestCreature(NPC_KORKRON_IRON_STAR, GetSpellInfo()->RangeEntry->MaxRangeFriend))
				target = ironStar;
		}

		void Register()
		{
			OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_power_iron_star_SpellScript::GetIronStar, EFFECT_0, TARGET_UNIT_NEARBY_ENTRY);
		}
	};

	SpellScript* GetSpellScript() const
	{
		return new spell_power_iron_star_SpellScript();
	}

	class spell_power_iron_star_AuraScript : public AuraScript
	{
		PrepareAuraScript(spell_power_iron_star_AuraScript);

		void HandleRemove(AuraEffect const* aurEff, AuraEffectHandleModes mode)
		{
			if (GetTargetApplication()->GetRemoveMode() == AURA_REMOVE_BY_EXPIRE)
			{
				if (GetCaster()->IsAIEnabled)
					GetCaster()->GetAI()->DoAction(ACTION_ACTIVATE_IRON_STAR);

				if (GetTarget()->IsAIEnabled)
					GetTarget()->GetAI()->DoAction(ACTION_ACTIVATE_IRON_STAR);
			}
		}

		void Register() override
		{
			OnEffectRemove += AuraEffectRemoveFn(spell_power_iron_star_AuraScript::HandleRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
		}
	};

	AuraScript* GetAuraScript() const override
	{
		return new spell_power_iron_star_AuraScript();
	}
};

// http://www.wowhead.com/spell=144866/enter-realm-of-yshaarj
class spell_enter_realm_of_yshaarj : public SpellScriptLoader
{
public:
	spell_enter_realm_of_yshaarj() : SpellScriptLoader("spell_enter_realm_of_yshaarj") {}

	class spell_enter_realm_of_yshaarj_SpellScript : public SpellScript
	{
		PrepareSpellScript(spell_enter_realm_of_yshaarj_SpellScript);

		void GetHeartOfYshaarj(WorldObject*& target)
		{
			target = nullptr;
			if (InstanceScript* instance = GetCaster()->GetInstanceScript())
				target = ObjectAccessor::GetCreature(*GetCaster(), instance->GetGuidData(DATA_HEART_OF_YSHAARJ_CREATURE_MAIN_ROOM));
		}

		void Register()
		{
			OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_enter_realm_of_yshaarj_SpellScript::GetHeartOfYshaarj, EFFECT_0, TARGET_UNIT_NEARBY_ENTRY);
		}
	};

	SpellScript* GetSpellScript() const
	{
		return new spell_enter_realm_of_yshaarj_SpellScript();
	}
};

class PlayerOrPetCheck
{
	public:
		bool operator()(WorldObject* object) const
		{
			if (object->GetTypeId() == TYPEID_PLAYER)
				return false;

			if (Creature* creature = object->ToCreature())
				return !creature->IsPet();

			return true;
		}
};

// http://www.wowhead.com/spell=144867/enter-realm-of-yshaarj
class spell_enter_realm_of_yshaarj_aoe : public SpellScriptLoader
{
public:
	spell_enter_realm_of_yshaarj_aoe() : SpellScriptLoader("spell_enter_realm_of_yshaarj_aoe") {}

	class spell_enter_realm_of_yshaarj_aoe_SpellScript : public SpellScript
	{
		PrepareSpellScript(spell_enter_realm_of_yshaarj_aoe_SpellScript);

		void FilterTarget(std::list<WorldObject*>& targets)
		{
			targets.remove_if(PlayerOrPetCheck());
		}

		void SelectGarrosh(std::list<WorldObject*>& targets)
		{
			targets.remove_if([](WorldObject* target)
			{
				return target->GetTypeId() != TYPEID_UNIT || target->GetEntry() != NPC_GARROSH_HELLSCREAM;
			});
		}

		void Register()
		{
			OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_enter_realm_of_yshaarj_aoe_SpellScript::FilterTarget, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
			OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_enter_realm_of_yshaarj_aoe_SpellScript::SelectGarrosh, EFFECT_1, TARGET_UNIT_SRC_AREA_ENTRY);
			OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_enter_realm_of_yshaarj_aoe_SpellScript::FilterTarget, EFFECT_2, TARGET_UNIT_SRC_AREA_ENEMY);
			OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_enter_realm_of_yshaarj_aoe_SpellScript::FilterTarget, EFFECT_3, TARGET_UNIT_SRC_AREA_ENEMY);
		}
	};

	SpellScript* GetSpellScript() const
	{
		return new spell_enter_realm_of_yshaarj_aoe_SpellScript();
	}
};

// http://www.wowhead.com/spell=144868/enter-realm-of-yshaarj
// http://www.wowhead.com/spell=144870/enter-realm-of-yshaarj
class spell_enter_realm_of_yshaarj_jump : public SpellScriptLoader
{
public:
	spell_enter_realm_of_yshaarj_jump() : SpellScriptLoader("spell_enter_realm_of_yshaarj_jump") { }

	class spell_enter_realm_of_yshaarj_jump_SpellScript : public SpellScript
	{
		PrepareSpellScript(spell_enter_realm_of_yshaarj_jump_SpellScript);

		bool Load() override
		{
			return GetCaster()->GetInstanceScript() != nullptr;
		}



		void SelectHeartOfYshaarj(WorldObject*& target)
		{
			target = nullptr;
			if (InstanceScript* instance = GetCaster()->GetInstanceScript())
				if (Creature* heartOfYshaarj = ObjectAccessor::GetCreature(*GetCaster(), instance->GetGuidData(DATA_HEART_OF_YSHAARJ_CREATURE_MAIN_ROOM)))
					target = heartOfYshaarj;
		}

		void Register() override
		{
			OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_enter_realm_of_yshaarj_jump_SpellScript::SelectHeartOfYshaarj, EFFECT_0, TARGET_DEST_NEARBY_ENTRY);
		}
	};

	SpellScript* GetSpellScript() const override
	{
		return new spell_enter_realm_of_yshaarj_jump_SpellScript();
	}
};


// http://www.wowhead.com/spell=144956/jump-to-ground
class spell_garrosh_jump_to_ground : public SpellScriptLoader
{
public:
	spell_garrosh_jump_to_ground() : SpellScriptLoader("spell_garrosh_jump_to_ground") { }

	class spell_garrosh_jump_to_ground_SpellScript : public SpellScript
	{
		PrepareSpellScript(spell_garrosh_jump_to_ground_SpellScript);

		bool Load() override
		{
			return GetCaster()->GetInstanceScript() != nullptr;
		}

		void SelectHeart(WorldObject*& target)
		{
			target = nullptr;
			//if (InstanceScript* instance = GetCaster()->GetInstanceScript())
			//	if (Creature* heartOfYshaarj = ObjectAccessor::GetCreature(*GetCaster(), instance->GetGuidData(DATA_HEART_OF_YSHAARJ_CREATURE_MAIN_ROOM)))
			//		target = heartOfYshaarj;
		}

		void RelocateDest(SpellEffIndex /*effIndex*/)
		{
			GetHitDest()->Relocate(RealmOfYshaarjJumpTarget);
			GetHitDest()->RelocateOffset({ 0.0f, 0.0f, 0.0f, GetCaster()->GetAngle(&RealmOfYshaarjJumpTarget) });
			/*if (GetHitDest())
				GetHitDest()->m_positionZ = -317.4f;*/
		}

		void Register() override
		{
			OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_garrosh_jump_to_ground_SpellScript::SelectHeart, EFFECT_0, TARGET_DEST_NEARBY_ENTRY);
			OnEffectLaunch += SpellEffectFn(spell_garrosh_jump_to_ground_SpellScript::RelocateDest, EFFECT_0, SPELL_EFFECT_JUMP_DEST);
		}
	};

	SpellScript* GetSpellScript() const override
	{
		return new spell_garrosh_jump_to_ground_SpellScript();
	}
};

// http://www.wowhead.com/spell=147831/attack-thrall
class spell_garrosh_attack_thrall : public SpellScriptLoader
{
public:
	spell_garrosh_attack_thrall() : SpellScriptLoader("spell_garrosh_attack_thrall") {}

	class spell_garrosh_attack_thrall_SpellScript : public SpellScript
	{
		PrepareSpellScript(spell_garrosh_attack_thrall_SpellScript);

		void GetThrall(WorldObject*& target)
		{
			target = nullptr;
			if (InstanceScript* instance = GetCaster()->GetInstanceScript())
				target = ObjectAccessor::GetCreature(*GetCaster(), instance->GetGuidData(DATA_GARROSH_THRALL));
		}

		void Register()
		{
			OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_garrosh_attack_thrall_SpellScript::GetThrall, EFFECT_0, TARGET_UNIT_NEARBY_ENTRY);
		}
	};

	SpellScript* GetSpellScript() const
	{
		return new spell_garrosh_attack_thrall_SpellScript();
	}
};

// http://www.wowhead.com/spell=144954/realm-of-yshaarj
class spell_garrosh_realm_of_yshaarj : public SpellScriptLoader
{
public:
	spell_garrosh_realm_of_yshaarj() : SpellScriptLoader("spell_garrosh_realm_of_yshaarj") {}

	class spell_garrosh_realm_of_yshaarj_AuraScript : public AuraScript
	{
		PrepareAuraScript(spell_garrosh_realm_of_yshaarj_AuraScript);

		void HandleRemove(AuraEffect const* aurEff, AuraEffectHandleModes mode)
		{
			if (GetTarget()->isDead())
			{
				Position position = RealmOfYshaarjJumpTarget;
				position.SetOrientation((float)rand_norm() * static_cast<float>(2 * M_PI));
				position.m_positionX = position.m_positionX + (8.0f * (float)rand_norm()) * std::cos((float)rand_norm() * static_cast<float>(2 * M_PI));
				position.m_positionY = position.m_positionY + (8.0f * (float)rand_norm()) * std::sin((float)rand_norm() * static_cast<float>(2 * M_PI));
				GetTarget()->NearTeleportTo(position.GetPositionX(), position.GetPositionY(), GetTarget()->GetMap()->GetHeight(position.GetPositionX(), position.GetPositionY(), position.GetPositionZ()), GetTarget()->GetOrientation());
				return;
			}

			switch (GetTargetApplication()->GetRemoveMode())
			{
				case AURA_REMOVE_BY_DEATH:
					GetTarget()->NearTeleportTo(RealmOfYshaarjJumpTarget.GetPositionX(), RealmOfYshaarjJumpTarget.GetPositionY(), GetTarget()->GetMap()->GetHeight(RealmOfYshaarjJumpTarget.GetPositionX(), RealmOfYshaarjJumpTarget.GetPositionY(), RealmOfYshaarjJumpTarget.GetPositionZ()), GetTarget()->GetOrientation());
					break;
				case AURA_REMOVE_BY_EXPIRE:
				case AURA_REMOVE_BY_ENEMY_SPELL:
				case AURA_REMOVE_BY_DEFAULT: // REMOVE LATER
					GetTarget()->NearTeleportTo(RealmOfYshaarjExitPosition.GetPositionX(), RealmOfYshaarjExitPosition.GetPositionY(), RealmOfYshaarjExitPosition.GetPositionZ(), GetTarget()->GetOrientation());
					//GetTarget()->CastSpell((Unit*)NULL, SPELL_JUMP_TO_GROUND, true);
					break;
				default:
					break;
			}
		}

		void Register() override
		{
			OnEffectRemove += AuraEffectRemoveFn(spell_garrosh_realm_of_yshaarj_AuraScript::HandleRemove, EFFECT_1, SPELL_AURA_SCREEN_EFFECT, AURA_EFFECT_HANDLE_REAL);
		}
	};

	AuraScript* GetAuraScript() const override
	{
		return new spell_garrosh_realm_of_yshaarj_AuraScript();
	}
};

// 221792
class go_garrosh_hellscream_sanctum_door : public GameObjectScript
{
public:
	go_garrosh_hellscream_sanctum_door() : GameObjectScript("go_garrosh_hellscream_sanctum_door") { }

	bool OnGossipHello(Player* /*player*/, GameObject* go) override
	{
		InstanceScript* instance = go->GetInstanceScript();
		if (!instance || instance->GetData(DATA_GARROSH_HELLSCREAM_INTRO) != NOT_STARTED)
			return false;

		instance->SetData(DATA_GARROSH_HELLSCREAM_INTRO, IN_PROGRESS);
		return false;
	}
};

class at_soo_garrosh_intro : public AreaTriggerScript
{
	public:
		at_soo_garrosh_intro() : AreaTriggerScript("at_soo_garrosh_intro") { }

		bool OnTrigger(Player* player, AreaTriggerEntry const* /*areaTrigger*/, bool /*entered*/) override
		{
			InstanceScript* instance = player->GetInstanceScript();
			if (!instance || instance->GetData(DATA_GARROSH_HELLSCREAM_INTRO) != IN_PROGRESS)
				return true;

			instance->SetData(DATA_GARROSH_HELLSCREAM_INTRO, DONE);
			return true;
		}
};

void AddSC_boss_garrosh_hellscream()
{
    new boss_garrosh_hellscream();

	new npc_garrosh_thrall();
	new npc_heart_of_yshaarj_main_room();

	new npc_korkron_iron_star();
	new npc_garrosh_siege_engineer();

	new spell_garrosh_weak_minded();
    new spell_garrosh_hellscream_desecrate();
	new spell_enter_realm_of_yshaarj();
	new spell_power_iron_star();
	new spell_enter_realm_of_yshaarj_jump();
	new spell_garrosh_attack_thrall();
	new spell_garrosh_jump_to_ground();
	new spell_garrosh_realm_of_yshaarj();

	new go_garrosh_hellscream_sanctum_door();
	new at_soo_garrosh_intro();
}
