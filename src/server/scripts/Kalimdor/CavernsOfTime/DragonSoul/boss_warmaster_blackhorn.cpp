/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "DisableMgr.h"
#include "CreatureTextMgr.h"
#include "ScriptedCreature.h"
#include "GridNotifiers.h"
#include "dragon_soul.h"
#include "Vehicle.h"
#include "ScriptedGossip.h"
#include "PassiveAI.h"
#include "Common.h"
#include "SpellAuraEffects.h"
#include "SpellScript.h"
#include <G3D/Vector3.h>
#include "MoveSplineInitArgs.h"
#include "MoveSplineInit.h"
#include "Log.h"
#include "SpellMgr.h"

/*"You won'get near the master, dragonriders attack!"
Will trigger the spawn of 2x Twilight Assault Drakes, one with entry: 56587 and the other with 56855*/
/*Twilight Barrage and Twilight Onslaught in between: (X: 13415 and 13465) (Y: -12110 and -12155) (Not blizzlike but close enough)
Blizzard use serverside spells for doing some tasks like spawning npcs sometimes, the dummy npc for Twilight Onslaught targeting is such an example,
the best we can do is just spawn the npc as we normally do.
Twilight Onslaught is spawned by: http://www.wowhead.com/spell=107587 (no entry in dbc which means wowhead has no data on the spell)*/

enum Other
{
    MAX_POINTS_GORIONA_INTRO = 14,
    MAX_POINTS_ASSAULT_DRAKE = 10,

    POINT_INSIDE = 1,
    POINT_OUTSIDE = 2,
    POINT_GORIONA_INTRO = 1,
    POINT_GORIONA_DROPOFF,
    POINT_PHASE_TWO,
    POINT_GORIONA_RETREAT,
    POINT_DRAKE_DROPOFF,
    POINT_DRAKE_ASSAULT,
    POINT_DRAKE_HARPOON,
    POINT_DRAKE_RETURN,
    POINT_FALL_GROUND,
    POINT_GORIONA_GROUND,
    POINT_DECKFIRE,

    ID_DRAKE_NONE,
    ID_EAST_DRAKE_ONE,
    ID_WEST_DRAKE_ONE,
    ID_EAST_DRAKE_TWO,
    ID_WEST_DRAKE_TWO,
    ID_EAST_DRAKE_THREE,
    ID_WEST_DRAKE_THREE,

    DATA_DRAKE_WAVE,
    DATA_TURRET_STATE,
    DATA_TURRET_TARGET,
    STATE_EMPTY,
    STATE_OCCUPIED,
    ID_TURRET_SET_TARGET,
    ID_TURRET_REMOVE_TARGET,
    DATA_ASSAULT_DRAKE_HARPOONABLE,
    STATE_OTHER,
    STATE_ASSAULTING,
    DATA_DOUSE_FIRE,
};

enum Spells
{
    //Other
    SPELL_TELEPORT_ALL_TO_GUNSHIP = 106051,
    SPELL_PLAY_SPINE_OF_DEATHWING_CINEMATIC = 106085,

    //Warmaster Blackhorn
    SPELL_DEVASTATE = 108042,
    SPELL_DISRUPTING_ROAR = 108044,
    SPELL_SHOCKWAVE = 110137,
    SPELL_SHOCKWAVE_2 = 108046,
    SPELL_VENGEANCE = 108045,
    SPELL_BERSERK = 26662,

    SPELL_DECK_FIRE_PERIODIC = 110092,
    SPELL_SHIP_FIRE = 109245,
    SPELL_ENGINE_FIRE = 107799,
    SPELL_EJECT_PASSENGER_1 = 60603,
    SPELL_CSA_DUMMY_EFFECT_1 = 56685, //Not a clue...
    SPELL_SMOKE_BOMB = 107752,
    SPELL_EVADE = 107761,
    SPELL_DETONATE = 107518,
    SPELL_CLEAR_ALL_DEBUFFS = 34098,
    SPELL_ROCKET_PACK = 110054,
    SPELL_GENERAL_TRIGGER_1 = 72760, //Target summoner
    SPELL_HATE_TO_ZERO = 63984,
    SPELL_ENGINE_SOUND = 109654,
    SPELL_GAINING_SPEED = 107514, //This sends the SMSG_MAP_OBJ_EVENTS opcode that changes the animation of the gunship
    //(The ship skybox starts to wobble, indicating that the ship is moving forward)
    SPELL_LOSING_SPEED = 107513,  //This sends the SMSG_MAP_OBJ_EVENTS opcode that changes the animation of the gunship
    //(The ship skybox stops moving, indicating that the ship has stopped)

    SPELL_TWILIGHT_BARRAGE = 107286,
    SPELL_TWILIGHT_BARRAGE_AOE = 107439,
    SPELL_TWILIGHT_SKYFIRE_BARRAGE = 107501,
    SPELL_HEAVY_SLUG = 108010, //On the drakes 1 second cooldown after cast
    SPELL_ARTILLERY_BARRAGE = 108041,
    SPELL_HARPOON = 108038,
    SPELL_RELOAD = 108039,
    SPELL_MASSIVE_EXPLOSION = 108132,
    SPELL_DECK_FIRE = 109445,
    SPELL_DECK_FIRE_SPAWN = 109470,
    SPELL_DEGENERATION = 107558,
    SPELL_BRUTAL_STRIKE = 107567,
    SPELL_BLADE_RUSH = 107594,
    SPELL_TWILIGHT_FLAMES = 108050,
    SPELL_TWILIGHT_BREATH = 110212,
    SPELL_CONSUMING_SHROUD = 110214,
    SPELL_TWILIGHT_BLAST = 107583,
    SPELL_TWILIGHT_ONSLAUGHT_CALL = 107586,
    SPELL_TWILIGHT_ONSLAUGHT_DUMMY = 107587, //Targets http://www.wowhead.com/npc=57920/deck-fire-controller, unknown effect so we skip
    SPELL_TWILIGHT_ONSLAUGHT = 107588,
    SPELL_TWILIGHT_ONSLAUGHT_AOE = 106401,
    SPELL_TWILIGHT_ONSLAUGHT_DAMAGE = 107589,
    SPELL_TWILIGHT_ONSLAUGHT_AURA = 107927,
    SPELL_BROADSIDE = 110153,
    SPELL_BROADSIDE_DAMAGE = 110157,
    SPELL_DECK_FIRE_DAMAGE_PERIODIC = 110092,
    SPELL_DECK_FIRE_DAMAGE = 110095,
    SPELL_WATER_JET = 110061,
    SPELL_DECK_FIRE_TARGET = 110081,
};

enum Events
{
    EVENT_NONE,
    EVENT_INTRO_TALK_1,
    EVENT_INTRO_TALK_2,
    EVENT_COMBAT_START,
    EVENT_TWILIGHT_ONSLAUGHT,
    EVENT_BROADSIDE,
    EVENT_DEVASTATE,
    EVENT_DISRUPTING_ROAR,
    EVENT_SHOCKWAVE,
    EVENT_CAN_YELL_KILL,
    EVENT_BERSERK,
    EVENT_TWILIGHT_BARRAGE,
    EVENT_HARPOON,
    EVENT_HEAVY_SLUG,
    EVENT_ARTILLERY_BARRAGE,
    EVENT_BREAK_FREE,
    EVENT_TWILIGHT_ASSAULT_GROUP,
    EVENT_TWILIGHT_INFILTRATOR,
    EVENT_TWILIGHT_INFILTRATOR_ESCAPE,
    EVENT_TWILIGHT_ASSAULT_DRAKE_ASSAULT,
    EVENT_START_ATTACK,
    EVENT_BLADE_RUSH,
    EVENT_DEGENERATION,
    EVENT_BRUTAL_STRIKE,
    EVENT_FLY_TO_PHASE_2_DROPOFF,
    EVENT_FLY_TO_PHASE_2_ASSAULT,
    EVENT_TWILIGHT_FLAMES,
    EVENT_TWILIGHT_BREATH,
    EVENT_CONSUMING_SHROUD,
    EVENT_SIPHON_VITALITY,
    EVENT_RETREAT,
    EVENT_ENGINE_TWILIGHT_BARRAGE,
    EVENT_EVADE,

    EVENT_PRE_COMBAT_1,
    EVENT_PRE_COMBAT_2,
    EVENT_SWAYZE_WARMASTER_VICTORY,
    EVENT_DECK_FIRE,
    EVENT_DOUSE_FIRE,

    EVENT_GROUP_DELAYABLE,
};

enum Actions
{
    ACTION_PHASE_TWO = 1,
    ACTION_LAND,
    ACTION_RETREAT,
    ACTION_RELOAD_FINISH,
    ACTION_SPAWN_DRAKES,
    ACTION_REEVS_START,
    ACTION_BATTLE_STATIONS,
    ACTION_WARMASTER_VICTORY,
};

enum Phases
{
    PHASE_ALL,
    PHASE_ONE,
    PHASE_TWO,

    PHASE_75_HEALTH,
    PHASE_50_HEALTH,
    PHASE_25_HEALTH,
    PHASE_20_HEALTH,

    PHASE_AIR,
    PHASE_GROUND,
    PHASE_FLEE,
};

enum Talk
{
    TALK_INTRO_1                    = 0,
    TALK_INTRO_2                    = 1,
    TALK_TWILIGHT_ONSLAUGHT         = 2,
    TALK_PHASE_2                    = 3,
    TALK_SHOCKWAVE                  = 4,
    TALK_KILL                       = 5,
    TALK_SIPHON                     = 6,
    TALK_BERSERK                    = 7,
    TALK_DEATH                      = 8,
    EMOTE_BERSERK                   = 9,

    SOUND_DISRUPTING_ROAR           = 26220,
    SOUND_KILL                      = 26216,

    TALK_SWAYZE_WELCOME             = 0,
    TALK_SWAYZE_WARMASTER_START     = 1,
    TALK_SWAYZE_WARMASTER_1         = 2,
    TALK_SWAYZE_WARMASTER_2         = 3,
    TALK_SWAYZE_SKYFIRE_DAMAGED     = 4,
    TALK_SWAYZE_SKYFIRE_DEAD        = 5,
    TALK_SWAYZE_WARMASTER_PHASE_2   = 6,
    TALK_SWAYZE_DEATHWING_START     = 7,
    TALK_SWAYZE_DEATHWING_1         = 8,

    EMOTE_GORIONA_ONSLAUGHT         = 0,
    EMOTE_GORIONA_BROADSIDE         = 1,
    EMOTE_GORIONA_RETREAT           = 2,
    EMOTE_DRAKE_BREAK_TETHER        = 0,
    EMOTE_INFILTRATOR_SAPPER        = 0,
    EMOTE_SAPPER_BREACH             = 0,
    EMOTE_SKYFIRE_DECKFIRE          = 0,
    EMOTE_SKYFIRE_LFR_TIP           = 1,
};

/*
    Deckfire Notes

    The deckfire has a specific behaviour.
    They spawn in exactly 5 yard distances from eachother, positions being determined by the Deck Fire Controller. (entry: 57920)
    Example:

    13447.4 Y: -12141.9 Z: 150.8    Base
    13452.4 Y: -12141.9 Z: 150.8    Offset = X:  5.0f, Y:  0.0f
    13442.4 Y: -12136.9 Z: 150.8    Offset = X: -5.0f, Y: -5.0f

    So to determine possible spawn positions it could be as easy as to just choose a base point
    based on the position examples in the sniffs and then calculate and determine if the offsets
    have a fire within their proximity, treat it like a grid. This way we can spawn fires wherever
    we want on the ship. As long as we check that the target Z coordinate and the map floor
    coordinate are not too far apart we can also check that we don't spread fires in the air.
    The fires may look weird if the raid happens to be standing on the stairs leading up to the
    top of the cabin since the coordinate calculation is slightly clunky and the passageway is small
    but it should be fine.
*/

Position const FireBaseCoordinates = { 13467.4f, -12136.9f, 0.0f };
Position const SkyfireDeckhandExitPos = { 13492.07f, -12135.25f, 155.6054f };

uint32 const SwayzeBattleStationsPathSize = 13;
G3D::Vector3 const SwayzeBattleStationsPath[SwayzeBattleStationsPathSize] =
{
    { 13468.19f, -12139.11f, 150.8992f },
    { 13468.78f, -12136.71f, 151.1432f },
    { 13469.03f, -12135.46f, 151.1432f },
    { 13469.28f, -12133.96f, 151.1432f },
    { 13472.53f, -12134.21f, 151.1432f },
    { 13473.78f, -12134.21f, 151.1432f },
    { 13475.12f, -12134.03f, 151.2588f },
    { 13481.87f, -12134.53f, 154.0088f },
    { 13483.38f, -12134.55f, 154.0052f },
    { 13483.38f, -12133.3f, 154.0052f },
    { 13483.63f, -12128.8f, 153.7552f },
    { 13483.88f, -12127.3f, 153.7552f },
    { 13481.37f, -12127.35f, 153.682f }
};

uint32 const SwayzeVictoryPathSize = 12;
G3D::Vector3 const SwayzeVictoryPath[SwayzeVictoryPathSize] =
{
    { 13481.37f, -12127.35f, 153.682f },
    { 13482.28f, -12129.98f, 153.9983f },
    { 13482.28f, -12130.48f, 153.9983f },
    { 13482.28f, -12131.23f, 153.9983f },
    { 13481.78f, -12131.98f, 153.9983f },
    { 13475.28f, -12134.23f, 151.2483f },
    { 13474.03f, -12134.23f, 151.2483f },
    { 13472.53f, -12134.23f, 151.2483f },
    { 13471.28f, -12135.73f, 151.7483f },
    { 13469.78f, -12135.73f, 151.7483f },
    { 13469.78f, -12136.48f, 151.7483f },
    { 13468.19f, -12139.11f, 150.8146f }
};

uint32 const ReevsBattleStationsPathSize = 14;
G3D::Vector3 const ReevsBattleStationsPath[ReevsBattleStationsPathSize] =
{
    { 13468.2f, -12142.13f, 150.897f },
    { 13469.33f, -12137.4f, 151.1571f },
    { 13469.33f, -12135.9f, 151.1571f },
    { 13472.58f, -12135.15f, 151.1571f },
    { 13475.08f, -12134.65f, 151.1571f },
    { 13476.08f, -12134.65f, 151.6571f },
    { 13480.95f, -12134.66f, 153.4171f },
    { 13481.07f, -12134.76f, 153.5072f },
    { 13481.82f, -12135.26f, 154.0072f },
    { 13482.57f, -12136.01f, 153.7572f },
    { 13483.57f, -12136.86f, 153.7051f },
    { 13482.99f, -12140.36f, 153.9178f },
    { 13482.99f, -12141.36f, 153.9178f },
    { 13480.34f, -12142.16f, 153.6717f }
};

uint32 const ReevsVictoryPathSize = 10;
G3D::Vector3 const ReevsVictoryPath[ReevsVictoryPathSize] =
{
    { 13480.34f, -12142.16f, 153.6717f },
    { 13481.27f, -12139.65f, 153.9872f },
    { 13481.52f, -12138.65f, 153.9872f },
    { 13481.52f, -12136.9f, 153.7372f },
    { 13475.27f, -12135.4f, 151.2372f },
    { 13472.52f, -12135.9f, 151.2372f },
    { 13472.52f, -12135.9f, 151.7372f },
    { 13469.77f, -12135.9f, 151.7372f },
    { 13469.77f, -12136.65f, 151.9872f },
    { 13468.2f, -12142.13f, 150.8028f }
};

uint32 const GorionaIntroPathSize = 14;
G3D::Vector3 const GorionaIntroPath[GorionaIntroPathSize] =
{
    { 13622.46f, -12100.59f, 170.5641f },
    { 13622.46f, -12100.59f, 170.5641f },
    { 13621.46f, -12100.59f, 170.5641f },
    { 13614.3f, -12102.46f, 167.7289f },
    { 13584.09f, -12089.64f, 167.4736f },
    { 13527.58f, -12066.98f, 167.4736f },
    { 13485.61f, -12057.89f, 169.8847f },
    { 13454.18f, -12052.43f, 172.5224f },
    { 13422.94f, -12053.89f, 176.6686f },
    { 13371.22f, -12099.46f, 187.0653f },
    { 13356.59f, -12135.83f, 180.2729f },
    { 13360.49f, -12173.27f, 180.2729f },
    { 13391.76f, -12203.25f, 188.6195f },
    { 13391.76f, -12203.25f, 188.6195f }
};

Position const GorionaSpawnPos = { 13617.98f, -12102.97f, 168.3606f, 3.243683f };
Position const GorionaDropOffPos = { 13422.2f, -12133.19f, 171.3667f };
Position const GorionaPhaseTwoPos = { 13408.5f, -12090.55f, 168.4647f, 5.358161f };
Position const GorionaLandPos = { 13423.41f, -12132.23f, 151.0395f };
Position const GorionaRetreatPos = { 13426.39f, -11926.0f, 167.744f };

uint32 const FirstAssaultDrakeEastPathSize = 10;
Position const FirstAssaultDrakeEastPath[FirstAssaultDrakeEastPathSize] =
{
    { 13622.73f, -12070.78f, 159.0655f },
    { 13624.19f, -12066.98f, 157.871f },
    { 13623.19f, -12066.95f, 157.871f },
    { 13616.54f, -12104.06f, 169.9852f },
    { 13587.36f, -12084.23f, 170.014f },
    { 13541.3f, -12070.96f, 166.7399f },
    { 13462.05f, -12056.04f, 170.3707f },
    { 13441.7f, -12065.96f, 172.2309f },
    { 13434.73f, -12123.99f, 171.8846f },
    { 13434.73f, -12123.99f, 171.8846f }
};

uint32 const SecondAssaultDrakeEastPathSize = 10;
Position const SecondAssaultDrakeEastPath[SecondAssaultDrakeEastPathSize] =
{
    { 13612.63f, -12098.65f, 159.0633f },
    { 13612.98f, -12097.91f, 157.871f },
    { 13611.98f, -12097.88f, 157.871f },
    { 13616.54f, -12104.06f, 169.9852f },
    { 13587.36f, -12084.23f, 170.014f },
    { 13541.3f, -12070.96f, 166.7399f },
    { 13476.69f, -12084.94f, 170.3707f },
    { 13451.92f, -12103.81f, 172.2309f },
    { 13434.73f, -12123.99f, 171.8846f },
    { 13434.73f, -12123.99f, 171.8846f }
};

uint32 const ThirdAssaultDrakeEastPathSize = 10;
Position const ThirdAssaultDrakeEastPath[ThirdAssaultDrakeEastPathSize] =
{
    { 13604.34f, -12096.14f, 170.4129f },
    { 13630.78f, -12103.63f, 166.6226f },
    { 13629.78f, -12103.6f, 166.6226f },
    { 13616.54f, -12104.06f, 169.9852f },
    { 13587.36f, -12084.23f, 170.014f },
    { 13546.61f, -12060.52f, 166.7399f },
    { 13443.16f, -12035.43f, 170.3707f },
    { 13407.02f, -12065.64f, 172.2309f },
    { 13434.73f, -12123.99f, 171.8846f },
    { 13434.73f, -12123.99f, 171.8846f }
};

uint32 const FirstAssaultDrakeWestPathSize = 11;
Position const FirstAssaultDrakeWestPath[FirstAssaultDrakeWestPathSize] =
{
    { 13605.21f, -12168.22f, 158.1617f },
    { 13607.5f, -12171.0f, 155.4982f },
    { 13606.53f, -12170.76f, 155.4982f },
    { 13595.93f, -12161.82f, 166.2525f },
    { 13548.54f, -12167.88f, 170.3928f },
    { 13522.3f, -12171.48f, 168.2348f },
    { 13478.16f, -12183.98f, 173.9983f },
    { 13445.84f, -12183.3f, 172.7414f },
    { 13438.69f, -12166.11f, 169.0544f },
    { 13435.55f, -12138.29f, 171.0571f },
    { 13435.55f, -12138.29f, 171.0571f }
};

uint32 const SecondAssaultDrakeWestPathSize = 11;
Position const SecondAssaultDrakeWestPath[SecondAssaultDrakeWestPathSize] =
{
    { 13606.83f, -12162.04f, 163.6648f },
    { 13608.74f, -12162.41f, 163.3623f },
    { 13607.77f, -12162.17f, 163.3623f },
    { 13595.93f, -12161.82f, 166.2525f },
    { 13548.54f, -12167.88f, 170.3928f },
    { 13537.97f, -12177.91f, 168.2348f },
    { 13484.74f, -12185.09f, 173.9983f },
    { 13461.34f, -12168.94f, 172.7414f },
    { 13450.08f, -12154.88f, 169.0544f },
    { 13435.55f, -12138.29f, 171.0571f },
    { 13435.55f, -12138.29f, 171.0571f }
};

uint32 const ThirdAssaultDrakeWestPathSize = 11;
Position const ThirdAssaultDrakeWestPath[ThirdAssaultDrakeWestPathSize] =
{
    { 13605.08f, -12167.89f, 158.5066f },
    { 13607.5f, -12171.0f, 155.4982f },
    { 13606.53f, -12170.76f, 155.4982f },
    { 13595.93f, -12161.82f, 166.2525f },
    { 13548.54f, -12167.88f, 170.3928f },
    { 13522.3f, -12171.48f, 168.2348f },
    { 13457.16f, -12190.59f, 173.9983f },
    { 13414.94f, -12191.7f, 172.7414f },
    { 13410.94f, -12171.14f, 176.379f },
    { 13435.55f, -12138.29f, 171.0571f },
    { 13435.55f, -12138.29f, 171.0571f }
};

Position const EastDrakeDropOffPoint = { 13434.72f, -12124.66f, 151.1979f };
Position const EastDrakeAssaultPointOne = { 13433.3f, -12205.2f, 176.183f, 1.361357f };
Position const EastDrakeAssaultPointTwo = { 13410.3f, -12205.2f, 176.183f, 1.029744f };
Position const EastDrakeAssaultPointThree = { 13453.3f, -12205.2f, 176.183f, 2.076942f };
Position const EastDrakeHarpoonPointOne = { 13430.44f, -12158.96f, 159.0908f };
Position const EastDrakeHarpoonPointTwo = { 13428.49f, -12158.71f, 159.1456f };
Position const EastDrakeHarpoonPointThree = { 13432.06f, -12158.63f, 159.663f };
Position const WestDrakeDropOffPoint = { 13435.24f, -12138.27f, 151.1738f };
Position const WestDrakeAssaultPointOne = { 13433.3f, -12060.7f, 176.183f, 4.625123f };
Position const WestDrakeAssaultPointTwo = { 13410.1f, -12060.7f, 176.183f, 5.183628f };
Position const WestDrakeAssaultPointThree = { 13453.3f, -12060.7f, 176.183f, 4.171337f };
Position const WestDrakeHarpoonPointOne = { 13432.43f, -12106.11f, 159.5907f };
Position const WestDrakeHarpoonPointTwo = { 13430.48f, -12106.43f, 158.7551f };
Position const WestDrakeHarpoonPointThree = { 13434.11f, -12106.4f, 158.7371f };

/*struct DrakeInfo
{
    Position const IntroPath;
    uint32 IntroPathSize;
    Position AssaultPoint, HarpoonPoint;

    static void FillSplinePath(Position const movementArr[], int32 numPoints, Movement::PointsArray& path)
    {
        for (uint8 i = 0; i < numPoints; i++)
        {
            G3D::Vector3 point;
            point.x = movementArr[i].GetPositionX();
            point.y = movementArr[i].GetPositionY();
            point.z = movementArr[i].GetPositionZ();
            path.push_back(point);
        }
    }
};*/

//DrakeInfo const AssaultDrakeInfo[6] =
//{
//};

uint32 const InfiltratorPathSize = 10;
Position const InfiltratorPath[InfiltratorPathSize] =
{
    { 13628.87f, -12130.33f, 166.8361f },
    { 13630.61f, -12132.06f, 166.6226f },
    { 13629.61f, -12132.02f, 166.6226f },
    { 13616.54f, -12104.06f, 169.9852f },
    { 13587.36f, -12084.23f, 170.014f },
    { 13538.41f, -11989.17f, 166.7399f },
    { 13396.21f, -12030.91f, 182.2028f },
    { 13334.26f, -12095.19f, 196.0419f },
    { 13406.59f, -12133.06f, 169.6312f },
    { 13406.59f, -12133.06f, 169.6312f }
};

Position const InfiltratorSpawnPoint = { 13626.52f, -12127.27f, 166.7005f };
Position const InfiltratorExitPoint = { 13649.39f, -12249.78f, 176.1371f };
Position const TwilightSapperTargetPoint = { 13476.67f, -12134.71f, 151.4972f };

uint32 const WestSkyfireFireBrigadeSize = 7;

G3D::Vector3 const WestSkyfireFireBrigade[WestSkyfireFireBrigadeSize] =
{
    { 13434.97f, -12088.83f, 111.5319f },
    { 13435.97f, -12088.83f, 111.5319f },
    { 13437.29f, -12093.58f, 145.0945f },
    { 13436.68f, -12097.96f, 160.1569f },
    { 13435.83f, -12113.01f, 163.7356f },
    { 13435.75f, -12117.79f, 162.3688f },
    { 13435.75f, -12117.79f, 162.3688f }
};

class DelayedExplosionEvent : public BasicEvent
{
    public:
        DelayedExplosionEvent(Creature* owner)
            : _owner(owner)
        {
        }

        bool Execute(uint64 /*time*/, uint32 /*diff*/)
        {
            _owner->CastSpell((Unit*)NULL, SPELL_MASSIVE_EXPLOSION, true);
            return true;
        }

    private:
        Creature* _owner;
};

class ResetEncounterEvent : public BasicEvent
{
    public:
        ResetEncounterEvent(Creature* source) : _source(source) {}

        bool Execute(uint64, uint32) override
        {
            if (_source->IsAIEnabled)
                _source->AI()->EnterEvadeMode();

            return true;
        }

    private:
        Creature* _source;
};

// http://www.wowhead.com/npc=55891
class npc_kaanu_reevs : public CreatureScript
{
    public:
        npc_kaanu_reevs() : CreatureScript("npc_kaanu_reevs") {}

        struct npc_kaanu_reevsAI : public ScriptedAI
        {
            npc_kaanu_reevsAI(Creature* creature) : ScriptedAI(creature), instance(creature->GetInstanceScript()) { }

            void DoAction(int32 actionId) override
            {
                switch (actionId)
                {
                    case ACTION_REEVS_START:
                        events.ScheduleEvent(EVENT_PRE_COMBAT_1, 27.5 * IN_MILLISECONDS);
                        break;
                    case ACTION_WARMASTER_VICTORY:
                    {
                        Position pos;
                        pos.Relocate(ReevsVictoryPath[ReevsVictoryPathSize - 1].x, ReevsVictoryPath[ReevsVictoryPathSize - 1].y, ReevsVictoryPath[ReevsVictoryPathSize - 1].z);
                        me->GetMotionMaster()->MovePoint(POINT_OUTSIDE, pos, false);

                        Movement::PointsArray path(ReevsVictoryPath, ReevsVictoryPath + ReevsVictoryPathSize);

                        Movement::MoveSplineInit init(me);
                        init.MovebyPath(path, 0);
                        init.SetWalk(false);
                        init.SetFacing(me->GetHomePosition().GetOrientation());
                        init.Launch();
                    }
                        break;
                    default:
                        break;
                }
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

            void UpdateAI(uint32 diff) override
            {
                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_PRE_COMBAT_1:
                        {
                            Position pos;
                            pos.Relocate(ReevsBattleStationsPath[ReevsBattleStationsPathSize - 1].x, ReevsBattleStationsPath[ReevsBattleStationsPathSize - 1].y, ReevsBattleStationsPath[ReevsBattleStationsPathSize - 1].z);
                            me->GetMotionMaster()->MovePoint(POINT_INSIDE, pos, false);

                            Movement::PointsArray path(ReevsBattleStationsPath, ReevsBattleStationsPath + ReevsBattleStationsPathSize);

                            Movement::MoveSplineInit init(me);
                            init.MovebyPath(path, 0);
                            init.SetWalk(false);
                            init.SetFacing(me->GetHomePosition().GetOrientation());
                            init.Launch();
                        }
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
            return new npc_kaanu_reevsAI(creature);
        }
};

// http://www.wowhead.com/npc=55870
class npc_sky_captain_swayze : public CreatureScript
{
    public:
        npc_sky_captain_swayze() : CreatureScript("npc_sky_captain_swayze") {}

        struct npc_sky_captain_swayzeAI : public ScriptedAI
        {
            npc_sky_captain_swayzeAI(Creature* creature) : ScriptedAI(creature), instance(creature->GetInstanceScript()) { }

            void InitializeAI() override
            {
                if (!me->IsAlive())
                    return;

                JustRespawned();
            }

            void DoAction(int32 actionId) override
            {
                switch (actionId)
                {
                    case ACTION_PHASE_TWO:
                        break;
                    case ACTION_WARMASTER_VICTORY:
                            events.ScheduleEvent(EVENT_SWAYZE_WARMASTER_VICTORY, 1 * IN_MILLISECONDS);
                            break;
                        default:
                            break;
                }
            }

            void JustSummoned(Creature* summon) override
            {
                if (summon->GetEntry() != NPC_GORIONA)
                    return;

                Talk(TALK_SWAYZE_WARMASTER_START);

                /*if (Creature* controller = ObjectAccessor::GetCreature(*me, instance->GetGuidData(NPC_DECK_FIRE_CONTROLLER)))
                    if (controller->IsAIEnabled)
                        controller->AI()->DoAction(ACTION_BATTLE_STATIONS);*/

                if (Creature* reevs = ObjectAccessor::GetCreature(*me, instance->GetGuidData(NPC_KAANU_REEVS)))
                    if (reevs->IsAIEnabled)
                        reevs->AI()->DoAction(ACTION_REEVS_START);

                events.ScheduleEvent(EVENT_PRE_COMBAT_1, 13.25 * IN_MILLISECONDS);
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

                std::list<Creature*> skyfireCrew;
                me->GetCreatureListWithEntryInGrid(skyfireCrew, NPC_SKYFIRE_DECKHAND, 200.0f);
                me->GetCreatureListWithEntryInGrid(skyfireCrew, NPC_SKYFIRE_COMMANDO, 200.0f);
                for (auto crewMember : skyfireCrew)
                    if (crewMember->IsAIEnabled)
                        crewMember->AI()->EnterEvadeMode(reason);
            }

            void JustRespawned() override
            {
                if (me->GetAreaId() == AREA_THE_DRAGON_WASTES_1 ||
                    (me->GetAreaId() == AREA_ABOVE_THE_FROZEN_SEA && instance->GetBossState(DATA_SPINE_OF_DEATHWING) != DONE))
                        me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);

                CreatureAI::JustRespawned();
            }

            void Reset() override
            {
                if (me->GetAreaId() == AREA_THE_DRAGON_WASTES_1)
                    Talk(TALK_SWAYZE_WELCOME);
            }

            void MovementInform(uint32 type, uint32 id) override
            {
                if (type != POINT_MOTION_TYPE || id != POINT_OUTSIDE)
                    return;

                me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            }

            void UpdateAI(uint32 diff) override
            {
                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_PRE_COMBAT_1:
                            Talk(TALK_SWAYZE_WARMASTER_1);
                            events.ScheduleEvent(EVENT_PRE_COMBAT_2, 13.25 * IN_MILLISECONDS);
                            break;
                        case EVENT_PRE_COMBAT_2:
                        {
                            Talk(TALK_SWAYZE_WARMASTER_2);
                            Position pos;
                            pos.Relocate(SwayzeBattleStationsPath[SwayzeBattleStationsPathSize - 1].x, SwayzeBattleStationsPath[SwayzeBattleStationsPathSize - 1].y, SwayzeBattleStationsPath[SwayzeBattleStationsPathSize - 1].z);
                            me->GetMotionMaster()->MovePoint(POINT_INSIDE, pos, false);

                            Movement::PointsArray path(SwayzeBattleStationsPath, SwayzeBattleStationsPath + SwayzeBattleStationsPathSize);

                            Movement::MoveSplineInit init(me);
                            init.MovebyPath(path, 0);
                            init.SetWalk(false);
                            init.Launch();

                            std::list<Creature*> skyfireCrew;
                            me->GetCreatureListWithEntryInGrid(skyfireCrew, NPC_SKYFIRE_DECKHAND, 200.0f);
                            me->GetCreatureListWithEntryInGrid(skyfireCrew, NPC_SKYFIRE_COMMANDO, 200.0f);
                            for (auto crewMember : skyfireCrew)
                                if (crewMember->IsAIEnabled)
                                    crewMember->AI()->DoAction(ACTION_BATTLE_STATIONS);
                        }
                            break;
                        case EVENT_SWAYZE_WARMASTER_VICTORY:
                        {
                            Position pos;
                            pos.Relocate(SwayzeVictoryPath[SwayzeVictoryPathSize - 1].x, SwayzeVictoryPath[SwayzeVictoryPathSize - 1].y, SwayzeVictoryPath[SwayzeVictoryPathSize - 1].z);
                            me->GetMotionMaster()->MovePoint(POINT_OUTSIDE, pos, false);

                            Movement::PointsArray path(SwayzeVictoryPath, SwayzeVictoryPath + SwayzeVictoryPathSize);

                            Movement::MoveSplineInit init(me);
                            init.MovebyPath(path, 0);
                            init.SetWalk(true);
                            init.SetFacing(me->GetHomePosition().GetOrientation());
                            init.Launch();
                        }
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

        bool OnGossipHello(Player* player, Creature* creature) override
        {
            InstanceScript* instance = creature->GetInstanceScript();
            if (!instance)
                return true;

            switch (creature->GetAreaId())
            {
                case AREA_THE_DRAGON_WASTES_1:
                    switch (player->GetTeam())
                    {
                        case ALLIANCE:
                            AddGossipItemFor(player, GOSSIP_SKYFIRE_DRAGONBLIGHT, GOSSIP_OPTION_ALLIANCE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
                            break;
                        case HORDE:
                            AddGossipItemFor(player, GOSSIP_SKYFIRE_DRAGONBLIGHT, GOSSIP_OPTION_HORDE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
                            break;
                        default:
                            break;
                    }
                    SendGossipMenuFor(player, player->GetGossipTextId(GOSSIP_SKYFIRE_DRAGONBLIGHT, creature), creature);
                    break;
                case AREA_ABOVE_THE_FROZEN_SEA:
                    uint32 state = instance->GetBossState(DATA_WARMASTER_BLACKHORN);
                    switch (state)
                    {
                        case NOT_STARTED:
                            AddGossipItemFor(player, GOSSIP_WARMASTER_BLACKHORN, GOSSIP_OPTION_DEFAULT, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
                            SendGossipMenuFor(player, player->GetGossipTextId(GOSSIP_SKYFIRE_DRAGONBLIGHT, creature), creature);
                            break;
                        case DONE:
                            if (instance->GetBossState(DATA_SPINE_OF_DEATHWING) != NOT_STARTED)
                                break;

                            AddGossipItemFor(player, GOSSIP_SPINE_OF_DEATHWING, GOSSIP_OPTION_DEFAULT, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
                            SendGossipMenuFor(player, player->GetGossipTextId(GOSSIP_SKYFIRE_DRAGONBLIGHT, creature), creature);
                            break;
                        default:
                            break;
                    }
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
                    creature->CastSpell(player, SPELL_TELEPORT_ALL_TO_GUNSHIP, true);
                    if (GameObject* skyfire = ObjectAccessor::GetGameObject(*creature, instance->GetGuidData(GO_ALLIANCE_SHIP_1)))
                        break;
                case GOSSIP_ACTION_INFO_DEF + 2:
                    creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                    creature->SummonCreature(NPC_GORIONA, GorionaSpawnPos, TEMPSUMMON_MANUAL_DESPAWN);
                    break;
                case GOSSIP_ACTION_INFO_DEF + 3:
                    if (instance->GetBossState(DATA_SPINE_OF_DEATHWING) != NOT_STARTED)
                        break;

                    creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                    creature->AI()->DoCastAOE(SPELL_PLAY_SPINE_OF_DEATHWING_CINEMATIC, true);
                    break;
                default:
                    break;
            }

            return true;
        }

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_sky_captain_swayzeAI(creature);
        }
};

// http://www.wowhead.com/npc=58174
class npc_skyfire_fire_brigade : public CreatureScript
{
public:
    npc_skyfire_fire_brigade() : CreatureScript("npc_skyfire_fire_brigade") {}

    struct npc_skyfire_fire_brigadeAI : public ScriptedAI
    {
        npc_skyfire_fire_brigadeAI(Creature* creature) : ScriptedAI(creature) { }

        void SpellHit(Unit* caster, SpellInfo const* spell) override
        {
            switch (spell->Id)
            {
                case SPELL_GENERAL_TRIGGER_1:
                    me->SetTarget(caster->GetGUID());
                    me->GetMotionMaster()->MovePoint(POINT_DECKFIRE, caster->GetPosition());
                    break;
                case SPELL_WATER_JET:
                    if (InstanceScript* instance = me->GetInstanceScript())
                        if (Creature* controller = ObjectAccessor::GetCreature(*me, instance->GetGuidData(NPC_DECK_FIRE_CONTROLLER)))
                            controller->AI()->SetGUID(me->GetGUID(), DATA_DOUSE_FIRE);
                    if (Unit* target = ObjectAccessor::GetCreature(*me, me->GetTarget()))
                        if (Creature* stalker = target->ToCreature())
                            if (TempSummon* tempSum = stalker->ToTempSummon())
                                if (me == tempSum->GetSummoner())
                                    tempSum->DespawnOrUnsummon();

                    events.ScheduleEvent(EVENT_DOUSE_FIRE, 4 * IN_MILLISECONDS);
                    break;
                default:
                    break;
            }
        }

        void MovementInform(uint32 type, uint32 pointId) override
        {
            if (type != POINT_MOTION_TYPE)
                return;

            switch (pointId)
            {
                case POINT_DECKFIRE:
                    DoCastAOE(SPELL_WATER_JET);
                    break;
                case POINT_OUTSIDE:
                    events.ScheduleEvent(EVENT_DOUSE_FIRE, 1 * IN_MILLISECONDS);
                    break;
                default:
                    break;
            }
        }

        void UpdateAI(uint32 diff) override
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_DOUSE_FIRE:
                        DoCastAOE(SPELL_DECK_FIRE_TARGET, true);
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
        return new npc_skyfire_fire_brigadeAI(creature);
    }
};

// http://www.wowhead.com/npc=57265
class npc_skyfire_deckhand : public CreatureScript
{
    public:
        npc_skyfire_deckhand() : CreatureScript("npc_skyfire_deckhand") {}

        struct npc_skyfire_deckhandAI : public ScriptedAI
        {
            npc_skyfire_deckhandAI(Creature* creature) : ScriptedAI(creature) { }

            void InitializeAI() override
            {
                if (!me->IsAlive())
                    me->Respawn();
            }

            void DoAction(int32 action) override
            {
                if (action != ACTION_BATTLE_STATIONS || !me->IsAlive())
                    return;

                me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_ONESHOT_NONE);
                me->SetWalk(false);
                me->GetMotionMaster()->MovePoint(POINT_INSIDE, SkyfireDeckhandExitPos);
            }

            void MovementInform(uint32 type, uint32 id) override
            {
                if (type != POINT_MOTION_TYPE || id != POINT_INSIDE)
                    return;

                me->DespawnOrUnsummon();
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_skyfire_deckhandAI(creature);
        }
};

// http://www.wowhead.com/npc=57264
class npc_skyfire_commando : public CreatureScript
{
    public:
        npc_skyfire_commando() : CreatureScript("npc_skyfire_commando") {}

        struct npc_skyfire_commandoAI : public ScriptedAI
        {
            npc_skyfire_commandoAI(Creature* creature) : ScriptedAI(creature) { }

            void Reset() override
            {
                _turretGuid = ObjectGuid::Empty;
            }

            void EnterEvadeMode(EvadeReason reason) override
            {
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

            void DoAction(int32 action) override
            {
                if (action != ACTION_BATTLE_STATIONS)
                    return;

                std::list<Creature*> skyfireTurrets;
                me->GetCreatureListWithEntryInGrid(skyfireTurrets, NPC_SKYFIRE_HARPOON_GUN, 200.0f);
                me->GetCreatureListWithEntryInGrid(skyfireTurrets, NPC_SKYFIRE_CANNON, 200.0f);
                for (auto turret : skyfireTurrets)
                    if (turret->IsAIEnabled)
                        if (turret->AI()->GetData(DATA_TURRET_STATE) == STATE_EMPTY)
                        {
                            _turretGuid = turret->GetGUID();
                            turret->AI()->SetData(DATA_TURRET_STATE, STATE_OCCUPIED);
                            Position pos = turret->GetNearPosition(2.400391f, M_PI);
                            me->SetWalk(false);
                            me->GetMotionMaster()->MovePoint(POINT_OUTSIDE, pos);
                            break;
                        }
            }

            void MovementInform(uint32 type, uint32 id) override
            {
                if (type != POINT_MOTION_TYPE || id != POINT_OUTSIDE)
                    return;

                if (Creature* turret = ObjectAccessor::GetCreature(*me, _turretGuid))
                    if (Vehicle* vehicle = turret->GetVehicleKit())
                        me->EnterVehicle(turret, 0);
            }

        private:
            ObjectGuid _turretGuid;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_skyfire_commandoAI(creature);
        }
};

struct skyfire_turretAI : public ScriptedAI
{
    skyfire_turretAI(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        if (me->GetVehicleKit()->HasEmptySeat(0))
            _state = STATE_EMPTY;
        else
            _state = STATE_OCCUPIED;
    }

    void PassengerBoarded(Unit* who, int8 /*seatId*/, bool apply) override
    {
        if (!apply)
            _state = STATE_EMPTY;
    }

    void SetData(uint32 type, uint32 data) override
    {
        if (type != DATA_TURRET_STATE)
            return;

        _state = (Other)data;
    }

    uint32 GetData(uint32 type) const override
    {
        if (type == DATA_TURRET_STATE)
            return _state;

        return 0;
    }

    protected:
        Other _state;
};

// http://www.wowhead.com/npc=56681
class npc_skyfire_harpoon_gun : public CreatureScript
{
    public:
        npc_skyfire_harpoon_gun() : CreatureScript("npc_skyfire_harpoon_gun") {}

        struct npc_skyfire_harpoon_gunAI : public skyfire_turretAI
        {
            npc_skyfire_harpoon_gunAI(Creature* creature) : skyfire_turretAI(creature) { }

            void Reset() override
            {
                skyfire_turretAI::Reset();

                _eligibleDrakes.clear();
            }

            void DoAction(int32 action) override
            {
                switch (action)
                {
                    case ACTION_RELOAD_FINISH:
                        if (_eligibleDrakes.empty())
                            return;

                        DoCastAOE(SPELL_HARPOON);
                        break;
                    default:
                        break;
                }
            }

            void SetGUID(ObjectGuid guid, int32 id) override
            {
                if (id == ID_TURRET_SET_TARGET)
                {
                    _eligibleDrakes.push_back(guid);
                    if (me->HasUnitState(UNIT_STATE_CASTING))
                        return;

                    DoCastAOE(SPELL_HARPOON);
                }
                else if (id == ID_TURRET_REMOVE_TARGET)
                {
                    _eligibleDrakes.remove(guid);
                    if (!me->GetCurrentSpell(CURRENT_CHANNELED_SPELL)) // If we're not channeling harpoon on a drake
                        if (Spell* harpoon = me->GetCurrentSpell(CURRENT_GENERIC_SPELL)) // But we're trying to cast either Reload or Harpoon
                            if (harpoon->m_spellInfo->Id == SPELL_HARPOON && harpoon->m_targets.GetUnitTargetGUID() == guid) // Check to see if it's harpoon
                            {
                                me->InterruptSpell(CURRENT_GENERIC_SPELL); // Spell will never land since the target is either dead or gone, cancel the spell and reschule it
                                if (!_eligibleDrakes.empty())
                                    DoCastAOE(SPELL_HARPOON);
                            }
                }
            }

            ObjectGuid GetGUID(int32 id) const override
            {
                if (id != DATA_TURRET_TARGET)
                    return ObjectGuid::Empty;

                Creature* _me = me;
                std::list<Creature*> assaultDrakes;
                for (auto guid : _eligibleDrakes)
                    if (Creature* drake = ObjectAccessor::GetCreature(*me, guid))
                        assaultDrakes.push_back(drake);

                assaultDrakes.sort(Trinity::HealthPctOrderPred());
                if (assaultDrakes.empty())
                    return ObjectGuid::Empty;

                return assaultDrakes.front()->GetGUID();
            }

        private:
            EventMap _events;
            std::list<ObjectGuid> _eligibleDrakes;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_skyfire_harpoon_gunAI(creature);
        }
};

// http://www.wowhead.com/npc=57260
class npc_skyfire_cannon : public CreatureScript
{
    public:
        npc_skyfire_cannon() : CreatureScript("npc_skyfire_cannon") {}

        struct npc_skyfire_cannonAI : public skyfire_turretAI
        {
            npc_skyfire_cannonAI(Creature* creature) : skyfire_turretAI(creature) { }

            void Reset() override
            {
                skyfire_turretAI::Reset();

                _target = ObjectGuid::Empty;
                if (me->HasUnitState(UNIT_STATE_CASTING))
                    if (me->GetCurrentSpell(CURRENT_GENERIC_SPELL))
                        me->InterruptSpell(CURRENT_GENERIC_SPELL);
                _events.Reset();
                me->SetFacingTo(me->GetHomePosition().GetOrientation());
            }

            void SetGUID(ObjectGuid guid, int32 id) override
            {
                if (id != DATA_TURRET_TARGET)
                    return;

                _target = guid;
                if (_target == ObjectGuid::Empty)
                {
                    Reset();
                }
                else if (Creature* target = ObjectAccessor::GetCreature(*me, _target))
                {
                    if (target->GetEntry() == NPC_GORIONA)
                    {
                        _events.CancelEvent(EVENT_HEAVY_SLUG);

                        if (Creature* target = ObjectAccessor::GetCreature(*me, _target))
                            me->SetFacingToObject(target);
                        DoCastAOE(SPELL_ARTILLERY_BARRAGE);
                        _events.ScheduleEvent(EVENT_ARTILLERY_BARRAGE, 1 * IN_MILLISECONDS);
                    }
                    else
                    {
                        _events.CancelEvent(EVENT_ARTILLERY_BARRAGE); //Never going to happen but makes sense to cancel this

                        DoCastAOE(SPELL_HEAVY_SLUG);
                        _events.ScheduleEvent(EVENT_HEAVY_SLUG, 3 * IN_MILLISECONDS);
                    }
                }
            }

            ObjectGuid GetGUID(int32 id) const override
            {
                if (id != DATA_TURRET_TARGET)
                    return ObjectGuid::Empty;

                return _target;
            }

            void UpdateAI(uint32 diff) override
            {
                _events.Update(diff);

                switch (_events.ExecuteEvent())
                {
                    case EVENT_HEAVY_SLUG:
                        DoCastAOE(SPELL_HEAVY_SLUG);
                        _events.ScheduleEvent(EVENT_HEAVY_SLUG, 3 * IN_MILLISECONDS);
                        break;
                    case EVENT_ARTILLERY_BARRAGE:
                        if (Creature* target = ObjectAccessor::GetCreature(*me, _target))
                            me->SetFacingToObject(target);
                        DoCastAOE(SPELL_ARTILLERY_BARRAGE);
                        _events.ScheduleEvent(EVENT_ARTILLERY_BARRAGE, 1 * IN_MILLISECONDS);
                        break;
                    default:
                        break;
                }
            }

        private:
            EventMap _events;
            ObjectGuid _target;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_skyfire_cannonAI(creature);
        }
};

// http://www.wowhead.com/npc=57920
class npc_deck_fire_controller : public CreatureScript
{
    public:
        npc_deck_fire_controller() : CreatureScript("npc_deck_fire_controller") {}

        struct npc_deck_fire_controllerAI : public ScriptedAI
        {
            npc_deck_fire_controllerAI(Creature* creature) : ScriptedAI(creature) { }

            void DoAction(int32 action) override
            {
                if (action != ACTION_BATTLE_STATIONS)
                    return;

                DoCastAOE(SPELL_DECK_FIRE_DAMAGE_PERIODIC, true);
                if (Player* player = me->SelectNearestPlayer(100.0f))
                    me->CastSpell(player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), SPELL_DECK_FIRE, true);
                events.ScheduleEvent(EVENT_DECK_FIRE, 1 * IN_MILLISECONDS);
            }

            void UpdateAI(uint32 diff) override
            {
                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_DECK_FIRE:
                            if (Player* player = me->SelectNearestPlayer(100.0f))
                                me->CastSpell(player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), SPELL_DECK_FIRE, true);
                            events.ScheduleEvent(EVENT_DECK_FIRE, 1 * IN_MILLISECONDS);
                            break;
                        default:
                            break;
                    }
                }
            }

            void SetGUID(ObjectGuid guid, int32 data) override
            {
                if (data != DATA_DOUSE_FIRE)
                    return;

                std::vector<DynamicObject*> deckfires = me->GetDynObjects(SPELL_DECK_FIRE);
                std::vector<DynamicObject*> dynobjs = me->GetDynObjects(SPELL_DECK_FIRE_SPAWN);
                deckfires.insert(deckfires.end(), dynobjs.begin(), dynobjs.end());

                if (Creature* stalker = ObjectAccessor::GetCreature(*me, guid))
                    for (auto fire : deckfires)
                        if (fire->IsWithinDist2d(stalker, 2.5f))
                        {
                            fire->Remove();
                            me->_UnregisterDynObject(fire);
                        }
            }

        private:
            EventMap events;
            Position GetEligibleFireLocation(Unit* caster, Position target, uint32 spellId1, uint32 spellId2)
            {
                Position pos = GetConvertedFireGridCoordinate(target);
                std::vector<DynamicObject*> deckfires = caster->GetDynObjects(spellId1);
                std::vector<DynamicObject*> dynobjs = caster->GetDynObjects(spellId2);
                deckfires.insert(deckfires.end(), dynobjs.begin(), dynobjs.end());

                std::list<Position> invalidPositions;

                for (auto fire : deckfires)
                {
                    invalidPositions.push_back(fire->GetPosition());
                }

                bool valid = true;
                for (auto ipos : invalidPositions)
                    if (pos.IsInDist(&ipos, 5.0f))
                        valid = false;

                if (valid)
                    return pos;
                else
                    pos = GetRandomGeneratedFirePosition(invalidPositions, pos);


                return pos;
            }

            Position GetConvertedFireGridCoordinate(Position base)
            {
                Position pos = FireBaseCoordinates;
                uint32 itr = 0;
                while (pos.m_positionX + 5.0f > base.m_positionX || pos.m_positionX - 5.0f < base.m_positionX && itr++ < 500)
                    pos.m_positionX += pos.m_positionX + 5.0f > base.m_positionX ? 5.0f : -5.0f;

                if (itr >= 100)
                    TC_LOG_DEBUG("scripts", "Warmaster Blackhorn [Deck Fire Controller]: Unable to generate valid Deck Fire coordinate after 500 iterations, player X Coordinate > 2500.0f yards from source.");

                itr = 0;
                while (pos.m_positionY + 5.0f > base.m_positionY || pos.m_positionY - 5.0f < base.m_positionY && itr++ < 500)
                    pos.m_positionY += pos.m_positionY + 5.0f > base.m_positionY ? 5.0f : -5.0f;

                if (itr >= 100)
                    TC_LOG_DEBUG("scripts", "Warmaster Blackhorn [Deck Fire Controller]: Unable to generate valid Deck Fire coordinate after 500 iterations, player Y Coordinate > 2500.0f yards from source.");

                return pos;
            }

            Position GetRandomGeneratedFirePosition(std::list<Position> invalidPositions, Position pos, uint32 maxIterations = 20)
            {
                std::list<Position> validPositions;
                Position defaultPos = pos;
                float X = 10.0f;
                float Y = 10.0f;
                while (--maxIterations > 0)
                {
                    float defaultX = X;
                    float defaultY = Y;

                    for (float _X = 0.0f; _X <= X; _X += 5.0f)
                    {
                        for (float _Y = 0.0f; _Y <= Y; _Y += 5.0f)
                        {
                            Position attemptPos = pos;
                            attemptPos.m_positionX += _X;
                            if (IsValidFireCoordinate(invalidPositions, attemptPos))
                                return attemptPos;

                            attemptPos.m_positionY += _Y;
                            if (IsValidFireCoordinate(invalidPositions, attemptPos))
                                return attemptPos;

                            attemptPos.m_positionX -= _X + _X;
                            if (IsValidFireCoordinate(invalidPositions, attemptPos))
                                return attemptPos;

                            attemptPos.m_positionY -= _Y + _Y;
                            if (IsValidFireCoordinate(invalidPositions, attemptPos))
                                return attemptPos;
                        }
                    }

                    X = defaultX + 5.0f;
                    Y = defaultY + 5.0f;
                }

                Position toReturn = (Position)NULL;
                float range = 0.0f;

                while (toReturn == (Position)NULL && range < 50.0f)
                {
                    std::list<Position> possibleLocs;
                    range += 5.0f;
                    for (auto position : validPositions)
                        if (pos.IsInDist2d(&position, range))
                            possibleLocs.push_back(position);

                    if (!possibleLocs.empty())
                    {
                        toReturn = possibleLocs.size() > 1 ? Trinity::Containers::SelectRandomContainerElement(possibleLocs) : possibleLocs.front();
                        break;
                    }
                }

                return toReturn;
            }

            bool IsValidFireCoordinate(std::list<Position> invalidPositions, Position testPos)
            {
                bool valid = true;
                for (auto pos : invalidPositions)
                    if (pos.IsInDist(&testPos, 5.0f))
                        valid = false;

                return valid;
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_deck_fire_controllerAI(creature);
        }
};

// http://www.wowhead.com/npc=56427
class boss_warmaster_blackhorn : public CreatureScript
{
    public:
        boss_warmaster_blackhorn() : CreatureScript("boss_warmaster_blackhorn") {}

        struct boss_warmaster_blackhornAI : public BossAI
        {
            boss_warmaster_blackhornAI(Creature* creature) : BossAI(creature, DATA_WARMASTER_BLACKHORN)
            {
                me->setRegeneratingHealth(false);
            }

            void Reset()
            {
                BossAI::Reset();
                _canYell = true;
                _introDone = false;
                _phaseTwo = false;
            }

            bool CanAIAttack(Unit const* /*target*/) const override
            {
                return _phaseTwo;
            }

            void IsSummonedBy(Unit* summoner) override
            {
                events.SetPhase(PHASE_ONE);
                events.ScheduleEvent(EVENT_INTRO_TALK_1, 7 * IN_MILLISECONDS); //6.5 or 7 seconds
                events.ScheduleEvent(EVENT_INTRO_TALK_2, 20 * IN_MILLISECONDS);
                if (InstanceScript* instance = me->GetInstanceScript())
                    if (Creature* skyfire = ObjectAccessor::GetCreature(*me, instance->GetGuidData(NPC_THE_SKYFIRE)))
                    {
                        skyfire->setActive(true);
                        instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, skyfire);
                    }
            }

            void EnterCombat(Unit* who) override
            {
                _EnterCombat();
            }

            void KilledUnit(Unit* victim) override
            {
                if (victim->GetTypeId() == TYPEID_PLAYER && _canYell && !me->IsInEvadeMode())
                {
                    if (urand(1,4) == 1)
                        sCreatureTextMgr->SendSound(me, SOUND_KILL, CHAT_MSG_MONSTER_YELL, 0, TEXT_RANGE_NORMAL, TEAM_OTHER, false);
                    else
                        Talk(TALK_KILL);
                    _canYell = false;
                    events.ScheduleEvent(EVENT_CAN_YELL_KILL, 8 * IN_MILLISECONDS);
                }
            }

            void DamageTaken(Unit* source, uint32& damage) override
            {
                if (damage > me->GetHealth() && !events.IsInPhase(PHASE_TWO))
                    damage = me->GetHealth() - 1;
            }

            void JustDied(Unit* /*killer*/) override
            {
                _JustDied();
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
                Talk(TALK_DEATH);

                if (Creature* goriona = ObjectAccessor::GetCreature(*me, instance->GetGuidData(NPC_GORIONA)))
                    if (goriona->IsAIEnabled)
                        goriona->AI()->DoAction(ACTION_RETREAT);

                if (Creature* pursuitController = ObjectAccessor::GetCreature(*me, instance->GetGuidData(NPC_GUNSHIP_PURSUIT_CONTROLLER)))
                    if (pursuitController->IsAIEnabled)
                        pursuitController->AI()->DoCastAOE(SPELL_GAINING_SPEED, true);

                if (Creature* skyfire = ObjectAccessor::GetCreature(*me, instance->GetGuidData(NPC_THE_SKYFIRE)))
                    if (skyfire->IsAIEnabled)
                        skyfire->AI()->EnterEvadeMode();

                if (Creature* swayze = ObjectAccessor::GetCreature(*me, instance->GetGuidData(NPC_SKY_CAPTAIN_SWAYZE)))
                    if (swayze->IsAIEnabled)
                        swayze->AI()->DoAction(ACTION_WARMASTER_VICTORY);

                if (Creature* kaanu = ObjectAccessor::GetCreature(*me, instance->GetGuidData(NPC_KAANU_REEVS)))
                    if (kaanu->IsAIEnabled)
                        kaanu->AI()->DoAction(ACTION_WARMASTER_VICTORY);
            }

            void EnterEvadeMode(EvadeReason reason) override
            {
                if (Creature* pursuitController = ObjectAccessor::GetCreature(*me, instance->GetGuidData(NPC_GUNSHIP_PURSUIT_CONTROLLER)))
                if (pursuitController->IsAIEnabled)
                    pursuitController->AI()->DoCastAOE(SPELL_GAINING_SPEED, true);

                if (Creature* skyfire = ObjectAccessor::GetCreature(*me, instance->GetGuidData(NPC_THE_SKYFIRE)))
                if (skyfire->IsAIEnabled)
                    skyfire->AI()->EnterEvadeMode();

                if (Creature* goriona = ObjectAccessor::GetCreature(*me, instance->GetGuidData(NPC_GORIONA)))
                if (goriona->IsAIEnabled)
                    goriona->AI()->EnterEvadeMode();

                if (Creature* swayze = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_SKY_CAPTAIN_SWAYZE_GUNSHIP)))
                if (swayze->IsAIEnabled)
                    swayze->AI()->EnterEvadeMode();

                if (Creature* kaanu = ObjectAccessor::GetCreature(*me, instance->GetGuidData(NPC_KAANU_REEVS)))
                if (kaanu->IsAIEnabled)
                    kaanu->AI()->EnterEvadeMode();

                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
                instance->SetBossState(DATA_WARMASTER_BLACKHORN, FAIL);
                BossAI::EnterEvadeMode();
                summons.DespawnAll();
                _DespawnAtEvade();
            }

            void JustSummoned(Creature* summon) override
            {
                summons.Summon(summon);
            }

            void SummonedCreatureDespawn(Creature* summon) override
            {
                summons.Despawn(summon);
            }

            void MovementInform(uint32 type, uint32 id) override
            {
                if (type != EFFECT_MOTION_TYPE || id != POINT_PHASE_TWO)
                    return;

                _phaseTwo = true;
                Talk(TALK_PHASE_2);
                DoCastAOE(SPELL_VENGEANCE, true);
                instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
                events.SetPhase(PHASE_TWO);
                events.ScheduleEvent(EVENT_DEVASTATE, 7 * IN_MILLISECONDS, EVENT_GROUP_DELAYABLE, PHASE_TWO);
                events.ScheduleEvent(EVENT_DISRUPTING_ROAR, 13 * IN_MILLISECONDS, 0, PHASE_TWO);
                events.ScheduleEvent(EVENT_SHOCKWAVE, 14.5 * IN_MILLISECONDS, 0, PHASE_TWO);
                events.ScheduleEvent(EVENT_BERSERK, 4 * MINUTE * IN_MILLISECONDS, 0, PHASE_TWO); //Does not enrage on LFR, we've got no local variables to check for this though.
                if (Player* player = me->SelectNearestPlayer(100.0f))
                    AttackStart(player);
                else
                    EnterEvadeMode(EVADE_REASON_NO_HOSTILES);
            }

            void UpdateAI(uint32 diff) override
            {
                if (_introDone)
                if (!me->IsFalling())
                    UpdateVictim();

                events.Update(diff);

                switch (events.ExecuteEvent())
                {
                    case EVENT_INTRO_TALK_1:
                        Talk(TALK_INTRO_1);
                        break;
                    case EVENT_INTRO_TALK_2:
                        Talk(TALK_INTRO_2);
                        events.ScheduleEvent(EVENT_COMBAT_START, 0.5*IN_MILLISECONDS);
                        break;
                    case EVENT_COMBAT_START:
                        me->SetInCombatWithZone();
                        if (TempSummon* summon = me->ToTempSummon())
                        if (summon->GetSummoner() && summon->GetSummoner()->GetTypeId() == TYPEID_UNIT)
                            summon->GetSummoner()->ToCreature()->SetInCombatWithZone();
                        _introDone = true;
                        break;
                    case EVENT_CAN_YELL_KILL:
                        _canYell = true;
                        break;
                    case EVENT_DEVASTATE:
                        DoCastVictim(SPELL_DEVASTATE);
                        events.ScheduleEvent(EVENT_DEVASTATE, 8.4 * IN_MILLISECONDS, EVENT_GROUP_DELAYABLE);
                        break;
                    case EVENT_DISRUPTING_ROAR:
                        sCreatureTextMgr->SendSound(me, SOUND_DISRUPTING_ROAR, CHAT_MSG_MONSTER_YELL, 0, TEXT_RANGE_NORMAL, TEAM_OTHER, false);
                        DoCastAOE(SPELL_DISRUPTING_ROAR);
                        events.DelayEvents(1 * IN_MILLISECONDS, EVENT_GROUP_DELAYABLE);
                        events.ScheduleEvent(EVENT_DISRUPTING_ROAR, urand(19 * IN_MILLISECONDS, 24 * IN_MILLISECONDS));
                        break;
                    case EVENT_SHOCKWAVE:
                        Talk(TALK_SHOCKWAVE);
                        DoCastAOE(SPELL_SHOCKWAVE, true);
                        events.ScheduleEvent(EVENT_SHOCKWAVE, urand(21 * IN_MILLISECONDS, 26 * IN_MILLISECONDS));
                        break;
                    case EVENT_BERSERK:
                        Talk(TALK_BERSERK);
                        Talk(EMOTE_BERSERK);
                        DoCastAOE(SPELL_BERSERK, true);
                        break;
                    default:
                        break;
                }

                DoMeleeAttackIfReady();
            }

            bool UpdateVictim()
            {
                if (!me->IsInCombat())
                {
                    EnterEvadeMode(EVADE_REASON_NO_HOSTILES);
                    return false;
                }

                if (!me->HasReactState(REACT_PASSIVE))
                {
                    if (Unit* victim = me->SelectVictim())
                        AttackStart(victim);
                    else if (me->getThreatManager().isThreatListEmpty())
                    {
                        EnterEvadeMode(EVADE_REASON_NO_HOSTILES);
                        return false;
                    }

                    return me->GetVictim() != nullptr;
                }

                return true;
            }

        private:
            bool _canYell;
            bool _introDone;
            bool _phaseTwo;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new boss_warmaster_blackhornAI(creature);
        }
};

// http://www.wowhead.com/npc=56781
class npc_goriona : public CreatureScript
{
    public:
        npc_goriona() : CreatureScript("npc_goriona") { }

        struct npc_gorionaAI : public ScriptedAI
        {
            npc_gorionaAI(Creature* creature) : ScriptedAI(creature), _vehicle(creature->GetVehicleKit()), _instance(creature->GetInstanceScript())
            {
                creature->setActive(true);
                ASSERT(_vehicle);
            }

            void Reset() override
            {
                _events.SetPhase(PHASE_ONE);
                _engineBarrages =_drakesKilled = _assaultWave = 0;
                _retreat = _onGround = false;
            }

            bool CanAIAttack(Unit const* /*target*/) const override
            {
                return _onGround;
            }

            void EnterCombat(Unit* who) override
            {
                DoZoneInCombat();
            }

            void DoAction(int32 action) override
            {
                switch (action)
                {
                    case ACTION_RETREAT:
                        _retreat = true;
                        _events.ScheduleEvent(EVENT_RETREAT, 0);
                        break;
                    default:
                        break;
                }
            }

            void JustSummoned(Creature* summon) override
            {
                switch (summon->GetEntry())
                {
                    case NPC_WARMASTER_BLACKHORN:
                        break;
                    default:
                        if (Creature* blackhorn = ObjectAccessor::GetCreature(*me, _instance->GetGuidData(NPC_WARMASTER_BLACKHORN)))
                            if (blackhorn->IsAIEnabled)
                                blackhorn->AI()->JustSummoned(summon);
                        break;
                }
            }

            void SummonedCreatureDies(Creature* summon, Unit* /*who*/) override
            {
                if (summon->GetEntry() == NPC_TWILIGHT_ASSAULT_DRAKE_1
                    || summon->GetEntry() == NPC_TWILIGHT_ASSAULT_DRAKE_2)
                    if (++_drakesKilled == 6)
                    {
                        _events.SetPhase(PHASE_TWO);
                        _events.ScheduleEvent(EVENT_FLY_TO_PHASE_2_DROPOFF, 3 * IN_MILLISECONDS, 0, PHASE_TWO);
                        if (Creature* swayze = ObjectAccessor::GetCreature(*me, _instance->GetGuidData(NPC_SKY_CAPTAIN_SWAYZE)))
                            if (swayze->IsAIEnabled)
                                swayze->AI()->Talk(TALK_SWAYZE_WARMASTER_PHASE_2);
                    }
            }

            void DamageTaken(Unit* source, uint32& damage) override
            {
                if (me->HealthBelowPctDamaged(25, damage) && !_retreat)
                {
                    _retreat = true;
                    if (_events.IsInPhase(PHASE_TWO))
                        DoAction(ACTION_RETREAT);
                }

                if (damage > me->GetHealth())
                    damage = me->GetHealth() - 1;
            }

            void EnterEvadeMode(EvadeReason reason) override
            {
                _events.Reset();
                ScriptedAI::EnterEvadeMode();
                me->DespawnOrUnsummon();
            }

            uint32 GetData(uint32 data) const override
            {
                if (data != DATA_DRAKE_WAVE)
                    return 0;

                return _assaultWave;
            }

            void MovementInform(uint32 type, uint32 id) override
            {
                if (type != POINT_MOTION_TYPE)
                    return;

                switch (id)
                {
                    case POINT_GORIONA_INTRO:
                        DoCastAOE(SPELL_LOSING_SPEED, true);
                        _events.ScheduleEvent(EVENT_TWILIGHT_ONSLAUGHT, 35 * IN_MILLISECONDS, 0, PHASE_ONE);
                        me->SummonCreatureGroup(_assaultWave++);
                        _events.ScheduleEvent(EVENT_TWILIGHT_ASSAULT_GROUP, 1 * MINUTE * IN_MILLISECONDS + 1 * IN_MILLISECONDS, 0, PHASE_ONE); //1 minute and 1 second.

                        if (IsHeroic())
                            _events.ScheduleEvent(EVENT_BROADSIDE, 45 * IN_MILLISECONDS, 0, PHASE_ONE);
                        break;
                    case POINT_DRAKE_DROPOFF:
                        //DoCastAOE(SPELL_EJECT_PASSENGER_1, true);
                        if (Unit* passenger = me->GetVehicleKit()->GetPassenger(0))
                        {
                            passenger->ExitVehicle();
                            passenger->GetMotionMaster()->MoveFall(POINT_PHASE_TWO);
                        }
                        _events.ScheduleEvent(EVENT_FLY_TO_PHASE_2_ASSAULT, 0);
                        break;
                    case POINT_PHASE_TWO:
                        if (_retreat)
                            DoAction(ACTION_RETREAT);
                        else
                        {
                            _events.SetPhase(PHASE_TWO);
                            me->SetFacingTo(GorionaPhaseTwoPos.GetOrientation());
                            _events.ScheduleEvent(EVENT_TWILIGHT_FLAMES, 10 * IN_MILLISECONDS);
                        }
                        break;
                    case POINT_GORIONA_GROUND:
                        _onGround = true;
                        _events.SetPhase(PHASE_GROUND);
                        break;
                    case POINT_GORIONA_RETREAT:
                        me->DespawnOrUnsummon();
                        break;
                    default:
                        break;
                }
            }

            void IsSummonedBy(Unit* /*summoner*/)
            {
                me->SetReactState(REACT_PASSIVE);
                Position pos;
                pos.Relocate(GorionaIntroPath[GorionaIntroPathSize - 1].x, GorionaIntroPath[GorionaIntroPathSize - 1].y, GorionaIntroPath[GorionaIntroPathSize - 1].z);
                //me->GetMotionMaster()->MovePoint(POINT_GORIONA_INTRO, pos);

                Movement::PointsArray path(GorionaIntroPath, GorionaIntroPath + GorionaIntroPathSize);

                Movement::MoveSplineInit init(me);
                init.MovebyPath(path);
                init.SetUncompressed();
                init.SetVelocity(35.0f);
                init.SetFly();
                init.SetSmooth();
                init.SetFacing(0.8203048f);
                init.Launch();
                _events.ScheduleEvent(EVENT_TWILIGHT_INFILTRATOR, 53 * IN_MILLISECONDS, 0, PHASE_ONE);
                _events.ScheduleEvent(EVENT_ENGINE_TWILIGHT_BARRAGE, 4 * IN_MILLISECONDS, 0, PHASE_ONE);
            }

            void UpdateAI(uint32 diff) override
            {
                _events.Update(diff);

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_ENGINE_TWILIGHT_BARRAGE:
                            DoCastAOE(SPELL_TWILIGHT_BLAST);
                            ++_engineBarrages;
                            if (_engineBarrages < 4)
                                _events.ScheduleEvent(EVENT_ENGINE_TWILIGHT_BARRAGE, 4 * IN_MILLISECONDS, 0, PHASE_ONE);
                            break;
                        case EVENT_TWILIGHT_ASSAULT_GROUP:
                            if (_assaultWave > 2)
                                break;
                            me->SummonCreatureGroup(_assaultWave++);
                            _events.ScheduleEvent(EVENT_TWILIGHT_ASSAULT_GROUP, 1 * MINUTE * IN_MILLISECONDS, 0, PHASE_ONE);
                            break;
                        case EVENT_TWILIGHT_INFILTRATOR:
                            me->SummonCreature(NPC_TWILIGHT_INFILTRATOR, InfiltratorSpawnPoint);
                            _events.ScheduleEvent(EVENT_TWILIGHT_INFILTRATOR, 40 * IN_MILLISECONDS, 0, PHASE_ONE);
                            break;
                        case EVENT_TWILIGHT_ONSLAUGHT:
                        {
                            float x = frand(13425.0f, 13465.0f);
                            float y = frand(-12140.0f, -12120.0f);
                            float z = 152.0f;
                            z = me->GetMap()->GetHeight(me->GetPhaseShift(), x, y, z);
                            Creature* dummy = me->SummonCreature(NPC_ONSLAUGHT_TARGET, x, y, z, 0.0f, TEMPSUMMON_TIMED_DESPAWN, 10 * IN_MILLISECONDS);
                            if (dummy)
                            {
                                dummy->SetDisplayId(dummy->GetCreatureTemplate()->Modelid2);
                                DoCastAOE(SPELL_TWILIGHT_ONSLAUGHT);
                                Talk(EMOTE_GORIONA_ONSLAUGHT);
                                if (Creature* blackhorn = ObjectAccessor::GetCreature(*me, _instance->GetGuidData(NPC_WARMASTER_BLACKHORN)))
                                    if (blackhorn->IsAIEnabled)
                                        blackhorn->AI()->Talk(TALK_TWILIGHT_ONSLAUGHT);
                            }
                            _events.ScheduleEvent(EVENT_TWILIGHT_ONSLAUGHT, 38 * IN_MILLISECONDS, 0, PHASE_ONE);
                            break;
                        }
                        case EVENT_BROADSIDE:
                            DoCastAOE(SPELL_BROADSIDE);
                            me->m_Events.AddEvent(new DelayedTalkEvent(me, EMOTE_GORIONA_BROADSIDE), me->m_Events.CalculateTime(2.5 * IN_MILLISECONDS));
                            _events.ScheduleEvent(EVENT_BROADSIDE, 70 * IN_MILLISECONDS, 0, PHASE_ONE);
                            break;
                        case EVENT_FLY_TO_PHASE_2_DROPOFF:
                        {
                            me->CastStop();
                            std::list<Creature*> skyfireCannons;
                            me->GetCreatureListWithEntryInGrid(skyfireCannons, NPC_SKYFIRE_CANNON, 200.0f);
                            for (auto cannon : skyfireCannons)
                                if (cannon->IsAIEnabled)
                                    cannon->AI()->SetGUID(me->GetGUID(), DATA_TURRET_TARGET);

                            me->GetMotionMaster()->MovePoint(POINT_DRAKE_DROPOFF, GorionaDropOffPos);
                        }
                            break;
                        case EVENT_FLY_TO_PHASE_2_ASSAULT:
                            me->GetMotionMaster()->MovePoint(POINT_PHASE_TWO, GorionaPhaseTwoPos);
                            break;
                        case EVENT_RETREAT:
                        {
                            _events.SetPhase(PHASE_FLEE);
                            _onGround = false;
                            Talk(EMOTE_GORIONA_RETREAT);
                            me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, true);
                            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, true);
                            me->SetReactState(REACT_PASSIVE);
                            me->AttackStop();
                            me->GetMotionMaster()->MovePoint(POINT_GORIONA_RETREAT, GorionaRetreatPos);
                            Movement::MoveSplineInit init(me);
                            init.MoveTo(GorionaRetreatPos.GetPositionX(), GorionaRetreatPos.GetPositionY(), GorionaRetreatPos.GetPositionZ());
                            init.SetUncompressed();
                            init.SetVelocity(35.0f);
                            init.SetFly();
                            init.Launch();
                            std::list<Creature*> skyfireCannons;
                            me->GetCreatureListWithEntryInGrid(skyfireCannons, NPC_SKYFIRE_CANNON, 200.0f);
                            for (auto cannon : skyfireCannons)
                                if (cannon->IsAIEnabled)
                                    cannon->AI()->Reset();
                            break;
                        }
                        case EVENT_TWILIGHT_FLAMES:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, RangedClassTargetSelector()))
                                me->CastSpell(target, SPELL_TWILIGHT_FLAMES);
                            else if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                                me->CastSpell(target, SPELL_TWILIGHT_FLAMES);
                            _events.ScheduleEvent(EVENT_TWILIGHT_FLAMES, 8 * IN_MILLISECONDS);
                            break;
                        case EVENT_TWILIGHT_BREATH:
                            DoCastAOE(SPELL_TWILIGHT_BREATH);
                            _events.ScheduleEvent(EVENT_TWILIGHT_BREATH, 20.5 * IN_MILLISECONDS);
                            break;
                        case EVENT_CONSUMING_SHROUD:
                            DoCastAOE(SPELL_CONSUMING_SHROUD, true);
                            _events.ScheduleEvent(EVENT_CONSUMING_SHROUD, 30 * IN_MILLISECONDS);
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            Vehicle* _vehicle;
            InstanceScript* _instance;
            EventMap _events;
            bool _retreat;
            bool _onGround;
            uint32 _engineBarrages;
            uint32 _drakesKilled;
            uint8 _assaultWave;
            void FillSplinePath(Position const movementArr[], Movement::PointsArray& path)
            {
                for (uint8 i = 0; i < MAX_POINTS_GORIONA_INTRO; i++)
                {
                    G3D::Vector3 point;
                    point.x = movementArr[i].GetPositionX();
                    point.y = movementArr[i].GetPositionY();
                    point.z = movementArr[i].GetPositionZ();
                    path.push_back(point);
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_gorionaAI(creature);
        }
};

// http://www.wowhead.com/npc=56598
class npc_blackhorn_the_skyfire : public CreatureScript
{
    public:
        npc_blackhorn_the_skyfire() : CreatureScript("npc_blackhorn_the_skyfire") {}

        struct npc_blackhorn_the_skyfireAI : public NullCreatureAI
        {
            npc_blackhorn_the_skyfireAI(Creature* creature) : NullCreatureAI(creature), _instance(creature->GetInstanceScript())
            {
                me->setRegeneratingHealth(false);
                me->SetDisplayId(me->GetCreatureTemplate()->Modelid1);
            }

            void InitializeAI() override
            {
                if (!me->isDead())
                    JustRespawned();
            }

            void JustRespawned() override
            {
                _died = _75PercentDeckfire = _50PercentDeckfire = _25PercentDeckfire = _20PercentWarning = false;
                //_lfrTipGiven = false;
                _phase = PHASE_75_HEALTH;
                me->SetHealth(me->GetMaxHealth());
                _instance->SetData(DATA_DECK_DEFENDER, TRUE);
                if (_instance->GetBossState(DATA_WARMASTER_BLACKHORN) != DONE)
                    _instance->SetBossState(DATA_WARMASTER_BLACKHORN, NOT_STARTED); //Reset Warmaster Blackhorn boss state since he never reappears after a wipe to reset the variable.
            }

            void EnterEvadeMode(EvadeReason reason) override
            {
                std::list<Creature*> creatures;
                me->GetCreatureListWithEntryInGrid(creatures, NPC_FIRE_STALKER, 200.0f);
                creatures.remove_if(Trinity::UnitAuraCheck(false, SPELL_SHIP_FIRE));
                for (auto trigger : creatures)
                    trigger->RemoveAura(SPELL_SHIP_FIRE);

                me->GetCreatureListWithEntryInGrid(creatures, NPC_ENGINE_STALKER, 200.0f);
                creatures.remove_if(Trinity::UnitAuraCheck(false, SPELL_ENGINE_FIRE));
                for (auto trigger : creatures)
                    trigger->RemoveAura(SPELL_ENGINE_FIRE);

                me->GetCreatureListWithEntryInGrid(creatures, NPC_SKYFIRE_HARPOON_GUN, 200.0f);
                me->GetCreatureListWithEntryInGrid(creatures, NPC_SKYFIRE_CANNON, 200.0f);
                for (auto trigger : creatures)
                    if (trigger->IsAIEnabled)
                        trigger->AI()->Reset();

                _instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);

                me->setActive(false);
                CreatureAI::EnterEvadeMode();

                uint32 corpseDelay = me->GetCorpseDelay();
                uint32 respawnDelay = me->GetRespawnDelay();

                me->SetCorpseDelay(1);
                if (_instance->GetBossState(DATA_WARMASTER_BLACKHORN) == DONE)
                    me->SetRespawnDelay(WEEK);
                else
                    me->SetRespawnDelay(29);

                me->DespawnOrUnsummon();

                me->SetCorpseDelay(corpseDelay);
                me->SetRespawnDelay(respawnDelay);
            }

            void DamageTaken(Unit* /*source*/, uint32& damage) override
            {
                if (damage >= me->GetHealth())
                {
                    JustDied(NULL);
                    damage = me->GetHealth() - 1;
                    return;
                }
                if (me->HealthBelowPctDamaged(20, damage) && !_20PercentWarning)
                    if (Creature* swayze = ObjectAccessor::GetCreature(*me, _instance->GetGuidData(NPC_SKY_CAPTAIN_SWAYZE)))
                        if (swayze->IsAIEnabled)
                        {
                            _20PercentWarning = true;
                            swayze->AI()->Talk(TALK_SWAYZE_SKYFIRE_DAMAGED);
                        }

                /*if (!me->GetMap()->IsHeroic())
                    return;*/

                while ((me->HealthBelowPctDamaged(75, damage) && !_75PercentDeckfire) ||
                    (me->HealthBelowPctDamaged(50, damage) && !_50PercentDeckfire) ||
                    (me->HealthBelowPctDamaged(25, damage) && !_25PercentDeckfire))
                {
                    std::list<Creature*> fireTriggers;
                    me->GetCreatureListWithEntryInGrid(fireTriggers, NPC_FIRE_STALKER, 200.0f);
                    fireTriggers.remove_if(Trinity::UnitAuraCheck(true, SPELL_SHIP_FIRE));
                    fireTriggers.sort(Trinity::ObjectDistanceOrderPred(me, false));

                    if (me->HealthBelowPctDamaged(75, damage) && !_75PercentDeckfire)
                    {
                        _75PercentDeckfire = true;
                        fireTriggers.resize(9);
                    }
                    else if (me->HealthBelowPctDamaged(50, damage) && !_50PercentDeckfire)
                    {
                        _50PercentDeckfire = true;
                        fireTriggers.resize(2);
                    }
                    else if (me->HealthBelowPctDamaged(25, damage) && !_25PercentDeckfire)
                    {
                        _25PercentDeckfire = true;
                        fireTriggers.resize(1);
                    }

                    for (auto trigger : fireTriggers)
                        trigger->CastSpell((Unit*)NULL, SPELL_SHIP_FIRE, true);

                    Talk(EMOTE_SKYFIRE_DECKFIRE);
                }
            }

            void SpellHit(Unit* /*caster*/, SpellInfo const* spell) override
            {
                if (spell->Id != SPELL_TWILIGHT_SKYFIRE_BARRAGE)
                    return;

                _instance->SetData(DATA_DECK_DEFENDER, FALSE);
                /*if (!_lfrTipGiven && me->GetMap()->GetSpawnMode() == RAID_DIFFICULTY_RAID_FINDER) //Uh, no variables implemented for LFR? LFR was implemented in 4.3...
                {
                    Talk(EMOTE_SKYFIRE_LFR_TIP);
                    _lfrTipGiven = true;
                }*/
            }

            void JustDied(Unit* /*killer*/) override
            {
                if (_died)
                    return;

                _died = true;

                if (Creature* swayze = ObjectAccessor::GetCreature(*me, _instance->GetGuidData(NPC_SKY_CAPTAIN_SWAYZE)))
                    swayze->AI()->Talk(TALK_SWAYZE_SKYFIRE_DEAD);

                std::list<Creature*> explosionTriggers;
                me->GetCreatureListWithEntryInGrid(explosionTriggers, NPC_MASSIVE_EXPLOSION, 150.0f);
                uint32 delay = 500;
                for (auto trigger : explosionTriggers)
                {
                    trigger->m_Events.AddEvent(new DelayedExplosionEvent(trigger), trigger->m_Events.CalculateTime(delay));
                    delay += 500;
                }

                me->m_Events.AddEvent(new ResetEncounterEvent(me), me->m_Events.CalculateTime(5 * IN_MILLISECONDS));
                if (Creature* warmaster = ObjectAccessor::GetCreature(*me, _instance->GetGuidData(NPC_WARMASTER_BLACKHORN)))
                    warmaster->m_Events.AddEvent(new ResetEncounterEvent(warmaster), warmaster->m_Events.CalculateTime(5 * IN_MILLISECONDS));
            }

        private:
            InstanceScript* _instance;
            bool _died, _75PercentDeckfire, _50PercentDeckfire, _25PercentDeckfire, _20PercentWarning;
            //bool _lfrTipGiven;
            Phases _phase;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_blackhorn_the_skyfireAI(creature);
        }
};

// http://www.wowhead.com/npc=56587
// http://www.wowhead.com/npc=56855
class npc_blackhorn_twilight_assault_drake : public CreatureScript
{
    public:
        npc_blackhorn_twilight_assault_drake() : CreatureScript("npc_blackhorn_twilight_assault_drake") {}

        struct npc_blackhorn_twilight_assault_drakeAI : public ScriptedAI
        {
            npc_blackhorn_twilight_assault_drakeAI(Creature* creature) : ScriptedAI(creature), _vehicle(creature->GetVehicleKit()), _instance(creature->GetInstanceScript())
            {
                me->setActive(true);
                ASSERT(_vehicle);
            }

            void EnterCombat(Unit* who) override
            {
                DoZoneInCombat();
            }

            void EnterEvadeMode(EvadeReason reason) override
            {
                _events.Reset();
                ScriptedAI::EnterEvadeMode();
                std::list<Creature*> harpoons;
                me->GetCreatureListWithEntryInGrid(harpoons, NPC_SKYFIRE_HARPOON_GUN, 200.0f);
                for (auto harpoon : harpoons)
                    if (harpoon->IsAIEnabled)
                        harpoon->AI()->SetGUID(me->GetGUID(), ID_TURRET_REMOVE_TARGET);

                me->DespawnOrUnsummon();
            }

            void JustDied(Unit* /*killer*/) override
            {
                _events.Reset();
                me->DespawnOrUnsummon(5 * IN_MILLISECONDS);
                std::list<Creature*> harpoons;
                me->GetCreatureListWithEntryInGrid(harpoons, NPC_SKYFIRE_HARPOON_GUN, 200.0f);
                for (auto harpoon : harpoons)
                    if (harpoon->IsAIEnabled)
                        harpoon->AI()->SetGUID(me->GetGUID(), ID_TURRET_REMOVE_TARGET);
            }

            void DamageTaken(Unit* source, uint32& damage) override
            {
                if (damage >= me->GetHealth())
                    if (!me->GetVehicleKit()->HasEmptySeat(0))
                        damage = me->GetHealth() - 1;
            }

            void SpellHit(Unit* caster, SpellInfo const* spell)
            {
                if (spell->Id != SPELL_HARPOON || !caster)
                    return;

                Movement::MoveSplineInit init(me);
                Position goPosition = { 0.0f, 0.0f, 0.0f, 0.0f };

                switch (_drakeID)
                {
                    case ID_EAST_DRAKE_ONE:
                        goPosition = EastDrakeHarpoonPointOne;
                        break;
                    case ID_EAST_DRAKE_TWO:
                        goPosition = EastDrakeHarpoonPointTwo;
                        break;
                    case ID_EAST_DRAKE_THREE:
                        goPosition = EastDrakeHarpoonPointThree;
                        break;
                    case ID_WEST_DRAKE_ONE:
                        goPosition = WestDrakeHarpoonPointOne;
                        break;
                    case ID_WEST_DRAKE_TWO:
                        goPosition = WestDrakeHarpoonPointTwo;
                        break;
                    case ID_WEST_DRAKE_THREE:
                        goPosition = WestDrakeHarpoonPointThree;
                        break;
                    default:
                        break;
                }

                me->GetMotionMaster()->MovePoint(POINT_DRAKE_HARPOON, goPosition);
                init.MoveTo(goPosition.GetPositionX(), goPosition.GetPositionY(), goPosition.GetPositionZ());
                init.SetFacing(caster);
                init.SetUncompressed();
                init.SetVelocity(10.0f); //Needs tuning
                init.SetFly();
                init.Launch();
            }

            void MovementInform(uint32 type, uint32 id) override
            {
                if (type != POINT_MOTION_TYPE)
                    return;

                switch (id)
                {
                    case POINT_DRAKE_DROPOFF:
                        if (me->GetVehicleKit())
                        if (Unit* passenger = me->GetVehicleKit()->GetPassenger(0))
                        {
                            passenger->ExitVehicle();
                            passenger->GetMotionMaster()->MoveFall(POINT_FALL_GROUND);
                        }
                        _events.ScheduleEvent(EVENT_TWILIGHT_ASSAULT_DRAKE_ASSAULT, 0);
                        break;
                    case POINT_DRAKE_ASSAULT:
                    case POINT_DRAKE_RETURN:
                        DoCastAOE(SPELL_TWILIGHT_BARRAGE);
                        _events.ScheduleEvent(EVENT_TWILIGHT_BARRAGE, 5 * IN_MILLISECONDS);
                        if (Creature* harpoon = me->FindNearestCreature(NPC_SKYFIRE_HARPOON_GUN, 200.0f))
                            if (harpoon->IsAIEnabled)
                                harpoon->AI()->SetGUID(me->GetGUID(), ID_TURRET_SET_TARGET);
                        break;
                    case POINT_DRAKE_HARPOON:
                        _events.ScheduleEvent(EVENT_BREAK_FREE, 25 * IN_MILLISECONDS);
                        break;
                    default:
                        break;
                }
            }

            void UpdateAI(uint32 diff) override
            {
                _events.Update(diff);

                switch (_events.ExecuteEvent())
                {
                    case EVENT_TWILIGHT_ASSAULT_DRAKE_ASSAULT:
                    {
                        Position AssaultPoint = me->GetPosition();
                        switch (_drakeID)
                        {
                            case ID_EAST_DRAKE_ONE:
                                AssaultPoint = EastDrakeAssaultPointOne;
                                break;
                            case ID_EAST_DRAKE_TWO:
                                AssaultPoint = EastDrakeAssaultPointTwo;
                                break;
                            case ID_EAST_DRAKE_THREE:
                                AssaultPoint = EastDrakeAssaultPointThree;
                                break;
                            case ID_WEST_DRAKE_ONE:
                                AssaultPoint = WestDrakeAssaultPointOne;
                                break;
                            case ID_WEST_DRAKE_TWO:
                                AssaultPoint = WestDrakeAssaultPointTwo;
                                break;
                            case ID_WEST_DRAKE_THREE:
                                AssaultPoint = WestDrakeAssaultPointThree;
                                break;
                            default:
                                return;
                        }
                        Movement::MoveSplineInit init(me);
                        me->GetMotionMaster()->MovePoint(POINT_DRAKE_ASSAULT, AssaultPoint);
                        init.MoveTo(AssaultPoint.GetPositionX(), AssaultPoint.GetPositionY(), AssaultPoint.GetPositionZ(), true);
                        init.SetFacing(AssaultPoint.GetOrientation());
                        init.SetUncompressed();
                        init.SetVelocity(15.0f); //Needs tuning
                        init.SetFly();
                        init.Launch();
                        break;
                    }
                    case EVENT_TWILIGHT_BARRAGE:
                        DoCastAOE(SPELL_TWILIGHT_BARRAGE);
                        _events.ScheduleEvent(EVENT_TWILIGHT_BARRAGE, 5 * IN_MILLISECONDS);
                        break;
                    case EVENT_BREAK_FREE:
                    {
                        _events.CancelEvent(EVENT_TWILIGHT_BARRAGE);
                        Talk(EMOTE_DRAKE_BREAK_TETHER);
                        if (Aura const* harpoon = me->GetAura(SPELL_HARPOON))
                            if (Unit* harpoonGun = harpoon->GetCaster())
                            if (harpoonGun->GetTypeId() == TYPEID_UNIT && harpoonGun->IsAIEnabled)
                            {
                                harpoonGun->CastStop();
                                harpoonGun->ToCreature()->AI()->DoCastAOE(SPELL_RELOAD);
                            }
                            Position AssaultPoint = me->GetPosition();
                            switch (_drakeID)
                            {
                                case ID_EAST_DRAKE_ONE:
                                    AssaultPoint = EastDrakeAssaultPointOne;
                                    break;
                                case ID_EAST_DRAKE_TWO:
                                    AssaultPoint = EastDrakeAssaultPointTwo;
                                    break;
                                case ID_EAST_DRAKE_THREE:
                                    AssaultPoint = EastDrakeAssaultPointThree;
                                    break;
                                case ID_WEST_DRAKE_ONE:
                                    AssaultPoint = WestDrakeAssaultPointOne;
                                    break;
                                case ID_WEST_DRAKE_TWO:
                                    AssaultPoint = WestDrakeAssaultPointTwo;
                                    break;
                                case ID_WEST_DRAKE_THREE:
                                    AssaultPoint = WestDrakeAssaultPointThree;
                                    break;
                                default:
                                    return;
                            }
                            Movement::MoveSplineInit init(me);
                            me->GetMotionMaster()->MovePoint(POINT_DRAKE_RETURN, AssaultPoint);
                            init.MoveTo(AssaultPoint.GetPositionX(), AssaultPoint.GetPositionY(), AssaultPoint.GetPositionZ(), true);
                            init.SetFacing(AssaultPoint.GetOrientation());
                            init.SetUncompressed();
                            init.SetVelocity(15.0f); //Needs tuning
                            init.SetFly();
                            init.Launch();
                    }
                        break;
                    default:
                        break;
                }
            }

            void IsSummonedBy(Unit* /*summoner*/)
            {
                if (!_instance)
                    return;

                me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, true);
                me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, true);
                me->SetReactState(REACT_PASSIVE);

                _drakeID = ID_DRAKE_NONE;
                if (Creature* goriona = ObjectAccessor::GetCreature(*me, _instance->GetGuidData(NPC_GORIONA)))
                {
                    if (CreatureAI* ai = goriona->AI())
                    {
                        uint32 wave = ai->GetData(DATA_DRAKE_WAVE);
                        if (wave == 1)
                        {
                            if (me->GetDistance(FirstAssaultDrakeEastPath[0]) < me->GetDistance(FirstAssaultDrakeWestPath[0]))
                                _drakeID = ID_EAST_DRAKE_ONE;
                            else
                                _drakeID = ID_WEST_DRAKE_ONE;
                        }
                        else if (wave == 2)
                        {
                            if (me->GetDistance(SecondAssaultDrakeEastPath[0]) < me->GetDistance(SecondAssaultDrakeWestPath[0]))
                                _drakeID = ID_EAST_DRAKE_TWO;
                            else
                                _drakeID = ID_WEST_DRAKE_TWO;
                        }
                        else if (me->GetDistance(ThirdAssaultDrakeEastPath[0]) < me->GetDistance(ThirdAssaultDrakeWestPath[0]))
                            _drakeID = ID_EAST_DRAKE_THREE;
                        else
                            _drakeID = ID_WEST_DRAKE_THREE;
                    }
                }

                Movement::MoveSplineInit init(me);

                switch (_drakeID)
                {
                    case ID_EAST_DRAKE_ONE:
                        AssaultPos = EastDrakeAssaultPointOne;
                        me->GetMotionMaster()->MovePoint(POINT_DRAKE_DROPOFF, FirstAssaultDrakeEastPath[FirstAssaultDrakeEastPathSize - 1]);
                        FillSplinePath(FirstAssaultDrakeEastPath, int32(MAX_POINTS_ASSAULT_DRAKE), init.Path());
                        break;
                    case ID_EAST_DRAKE_TWO:
                        AssaultPos = EastDrakeAssaultPointTwo;
                        me->GetMotionMaster()->MovePoint(POINT_DRAKE_DROPOFF, SecondAssaultDrakeEastPath[SecondAssaultDrakeEastPathSize - 1]);
                        FillSplinePath(SecondAssaultDrakeEastPath, int32(MAX_POINTS_ASSAULT_DRAKE), init.Path());
                        break;
                    case ID_EAST_DRAKE_THREE:
                        AssaultPos = EastDrakeAssaultPointThree;
                        me->GetMotionMaster()->MovePoint(POINT_DRAKE_DROPOFF, ThirdAssaultDrakeEastPath[ThirdAssaultDrakeEastPathSize - 1]);
                        FillSplinePath(ThirdAssaultDrakeEastPath, int32(MAX_POINTS_ASSAULT_DRAKE), init.Path());
                        break;
                    case ID_WEST_DRAKE_ONE:
                        AssaultPos = WestDrakeAssaultPointOne;
                        me->GetMotionMaster()->MovePoint(POINT_DRAKE_DROPOFF, FirstAssaultDrakeWestPath[FirstAssaultDrakeWestPathSize - 1]);
                        FillSplinePath(FirstAssaultDrakeWestPath, int32(MAX_POINTS_ASSAULT_DRAKE + 1), init.Path());
                        break;
                    case ID_WEST_DRAKE_TWO:
                        AssaultPos = WestDrakeAssaultPointTwo;
                        me->GetMotionMaster()->MovePoint(POINT_DRAKE_DROPOFF, SecondAssaultDrakeWestPath[SecondAssaultDrakeWestPathSize - 1]);
                        FillSplinePath(SecondAssaultDrakeWestPath, int32(MAX_POINTS_ASSAULT_DRAKE + 1), init.Path());
                        break;
                    case ID_WEST_DRAKE_THREE:
                        AssaultPos = WestDrakeAssaultPointThree;
                        me->GetMotionMaster()->MovePoint(POINT_DRAKE_DROPOFF, ThirdAssaultDrakeWestPath[ThirdAssaultDrakeWestPathSize - 1]);
                        FillSplinePath(ThirdAssaultDrakeWestPath, int32(MAX_POINTS_ASSAULT_DRAKE + 1), init.Path());
                        break;
                    default:
                        me->DespawnOrUnsummon();
                        return;
                }

                init.SetUncompressed();
                if (_drakeID == ID_EAST_DRAKE_ONE || _drakeID == ID_EAST_DRAKE_TWO)
                    init.SetVelocity(25.0f);
                else if (_drakeID == ID_WEST_DRAKE_THREE || _drakeID == ID_EAST_DRAKE_THREE)
                    init.SetVelocity(35.0f);
                else
                    init.SetVelocity(20.0f);

                init.SetFly();
                init.SetSmooth();
                init.Launch();
            }

        private:
            Vehicle* _vehicle;
            InstanceScript* _instance;
            EventMap _events;
            Other _drakeID;
            Position AssaultPos;
            void FillSplinePath(Position const movementArr[], int32 numPoints, Movement::PointsArray& path)
            {
                for (uint8 i = 0; i < numPoints; i++)
                {
                    G3D::Vector3 point;
                    point.x = movementArr[i].GetPositionX();
                    point.y = movementArr[i].GetPositionY();
                    point.z = movementArr[i].GetPositionZ();
                    path.push_back(point);
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_blackhorn_twilight_assault_drakeAI(creature);
        }
};

// http://www.wowhead.com/npc=56922
class npc_blackhorn_twilight_infiltrator : public CreatureScript
{
    public:
        npc_blackhorn_twilight_infiltrator() : CreatureScript("npc_blackhorn_twilight_infiltrator") {}

        struct npc_blackhorn_twilight_infiltratorAI : public ScriptedAI
        {
            npc_blackhorn_twilight_infiltratorAI(Creature* creature) : ScriptedAI(creature), _vehicle(creature->GetVehicleKit()), _instance(creature->GetInstanceScript()), _summons(creature)
            {
                me->setActive(true);
                ASSERT(_vehicle);
            }

            void EnterEvadeMode(EvadeReason /*reason*/) override
            {
                _events.Reset();
                _summons.DespawnAll();
                ScriptedAI::EnterEvadeMode();
                me->DespawnOrUnsummon();
            }

            void DamageTaken(Unit* source, uint32& damage) override
            {
                if (damage > me->GetHealth())
                    damage = me->GetHealth() - 1;
            }

            void MovementInform(uint32 type, uint32 id) override
            {
                if (type != POINT_MOTION_TYPE)
                    return;

                switch (id)
                {
                    case POINT_DRAKE_DROPOFF:
                        if (Unit* passenger = me->GetVehicleKit()->GetPassenger(0))
                        {
                            passenger->ExitVehicle();
                            passenger->GetMotionMaster()->MoveFall(POINT_FALL_GROUND);
                        }
                        _events.ScheduleEvent(EVENT_TWILIGHT_INFILTRATOR_ESCAPE, 0);
                        break;
                    case POINT_DRAKE_ASSAULT:
                        _events.ScheduleEvent(EVENT_TWILIGHT_INFILTRATOR, 0);
                        break;
                    case POINT_DRAKE_RETURN:
                        me->DespawnOrUnsummon();
                        break;
                    default:
                        break;
                }
            }

            void IsSummonedBy(Unit* /*summoner*/)
            {
                me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, true);
                me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, true);
                me->SetReactState(REACT_PASSIVE);
                me->GetMotionMaster()->MovePoint(POINT_DRAKE_ASSAULT, InfiltratorPath[InfiltratorPathSize - 1]);
                Movement::MoveSplineInit init(me);
                FillSplinePath(InfiltratorPath, InfiltratorPathSize, init.Path());
                init.SetUncompressed();
                init.SetVelocity(35.0f);
                init.SetFly();
                init.SetSmooth();
                init.Launch();
            }

            void UpdateAI(uint32 diff) override
            {
                _events.Update(diff);

                switch (_events.ExecuteEvent())
                {
                    case EVENT_TWILIGHT_INFILTRATOR:
                    {
                        Talk(EMOTE_INFILTRATOR_SAPPER);
                        Position pos = me->GetPosition();
                        pos.m_positionX += 10 + frand(-2.5f, 2.5f); //Offset by 10 coordinates and then randomize it.
                        pos.m_positionY += frand(-15.0f, 15.0f);
                        me->GetMotionMaster()->MovePoint(POINT_DRAKE_DROPOFF, pos);
                    }
                        break;
                    case EVENT_TWILIGHT_INFILTRATOR_ESCAPE:
                    {
                        me->GetMotionMaster()->MovePoint(POINT_DRAKE_RETURN, InfiltratorExitPoint);
                        Movement::MoveSplineInit init(me);
                        init.MoveTo(InfiltratorExitPoint.GetPositionX(), InfiltratorExitPoint.GetPositionY(), InfiltratorExitPoint.GetPositionZ());
                        init.SetUncompressed();
                        init.SetVelocity(35.0f);
                        init.SetFly();
                        init.SetSmooth();
                        init.Launch();
                    }
                        break;
                    default:
                        break;
                }
            }

        private:
            Vehicle* _vehicle;
            InstanceScript* _instance;
            EventMap _events;
            Other _drakeID;
            SummonList _summons;
            void FillSplinePath(Position const movementArr[], int32 numPoints, Movement::PointsArray& path)
            {
                for (uint8 i = 0; i < numPoints; i++)
                {
                    G3D::Vector3 point;
                    point.x = movementArr[i].GetPositionX();
                    point.y = movementArr[i].GetPositionY();
                    point.z = movementArr[i].GetPositionZ();
                    path.push_back(point);
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_blackhorn_twilight_infiltratorAI(creature);
        }
};

// http://www.wowhead.com/npc=56923
class npc_twilight_sapper : public CreatureScript
{
    public:
        npc_twilight_sapper() : CreatureScript("npc_twilight_sapper") {}

        struct npc_twilight_sapperAI : public ScriptedAI
        {
            npc_twilight_sapperAI(Creature* creature) : ScriptedAI(creature)
            {
                me->setActive(true);
                me->SetReactState(REACT_PASSIVE);
                me->SetWalk(false);
            }

            void IsSummonedBy(Unit* /*summoner*/) override
            {
                if (InstanceScript* instance = me->GetInstanceScript())
                    if (Creature* blackhorn = ObjectAccessor::GetCreature(*me, instance->GetGuidData(NPC_WARMASTER_BLACKHORN)))
                        if (blackhorn->IsAIEnabled)
                            blackhorn->AI()->JustSummoned(me);


                me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, true);
                me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, true);
                _landed = _evaded = false;
                _evadeHealthPercent = frand(30, 80); //Everything about the Evade ability is completely made up, no information on how it actually works
            }

            void JustDied(Unit* killer) override
            {
                me->DespawnOrUnsummon(5 * IN_MILLISECONDS);
            }

            void DamageTaken(Unit* source, uint32& damage) override
            {
                if (_landed && !_evaded && damage < me->GetHealth() && _evadeHealthPercent >= me->GetHealthPct())
                {
                    _evaded = true;
                    DoCastAOE(SPELL_EVADE, true);
                }
            }

            void MovementInform(uint32 type, uint32 id) override
            {
                if (type == EFFECT_MOTION_TYPE && id == POINT_FALL_GROUND)
                {

                    DoCastAOE(SPELL_SMOKE_BOMB, true);
                    DoCastAOE(SPELL_CLEAR_ALL_DEBUFFS, true);

                    _landed = true;
                    //Find a better way to handle the dragon soul buff and remove this
                    if (InstanceScript* instance = me->GetInstanceScript())
                        if (instance->GetData(DATA_DS_BUFF_STATE) != DONE)
                        {
                            me->AddAura(SPELL_POWER_OF_THE_ASPECTS_35, me);
                            /*if (instance->GetSpawnMode() == RAID_DIFFICULTY_RAID_FINDER)
                                me->AddAura(SPELL_PRESENCE_OF_THE_DRAGON_SOUL, me);*/
                        }
                }
                else if (type == POINT_MOTION_TYPE && id == POINT_INSIDE)
                {
                    Talk(EMOTE_SAPPER_BREACH);
                    DoCastAOE(SPELL_DETONATE);
                }
            }

            void UpdateAI(uint32 diff) override
            {
                if (_landed && !me->isMoving() && me->CanFreeMove()) //Test
                    me->GetMotionMaster()->MovePoint(POINT_INSIDE, TwilightSapperTargetPoint);
            }

        private:
            float _evadeHealthPercent;
            bool _landed, _evaded;
            EventMap _events;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_twilight_sapperAI(creature);
        }
};

// http://www.wowhead.com/npc=56848
// http://www.wowhead.com/npc=56854
class npc_twilight_elite_vrykul : public CreatureScript
{
public:
    npc_twilight_elite_vrykul() : CreatureScript("npc_twilight_elite_vrykul") {}

    struct npc_twilight_elite_vrykulAI : public ScriptedAI
    {
        npc_twilight_elite_vrykulAI(Creature* creature) : ScriptedAI(creature)
        {
            me->setActive(true);
        }

        void MoveInLineOfSight(Unit* who) override
        {
            if (!_droppedOff)
                return;

            ScriptedAI::MoveInLineOfSight(who);
        }

        void AttackStart(Unit* who) override
        {
            if (!_droppedOff)
                return;

            ScriptedAI::AttackStart(who);
        }

        void IsSummonedBy(Unit* /*summoner*/) override
        {
            _droppedOff = false;
            me->SetReactState(REACT_PASSIVE);
            if (InstanceScript* instance = me->GetInstanceScript())
                if (Creature* blackhorn = ObjectAccessor::GetCreature(*me, instance->GetGuidData(NPC_WARMASTER_BLACKHORN)))
                    if (blackhorn->IsAIEnabled)
                        blackhorn->AI()->JustSummoned(me);
        }

        void JustDied(Unit* killer) override
        {
            me->DespawnOrUnsummon(5 * IN_MILLISECONDS);
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type != EFFECT_MOTION_TYPE || id != POINT_FALL_GROUND)
                return;

            me->SetHomePosition(me->GetPosition());
            _events.ScheduleEvent(EVENT_START_ATTACK, 1 * IN_MILLISECONDS);
            _events.ScheduleEvent(EVENT_CHARGE, urand(8.5 * IN_MILLISECONDS, 9.5 * IN_MILLISECONDS));
            if (me->GetEntry() == NPC_TWILIGHT_ELITE_DREADBLADE)
                _events.ScheduleEvent(EVENT_DEGENERATION, urand(8.5 * IN_MILLISECONDS, 9.5 * IN_MILLISECONDS));
            else
                _events.ScheduleEvent(EVENT_BRUTAL_STRIKE, urand(8.5 * IN_MILLISECONDS, 9.5 * IN_MILLISECONDS));
        }

        void UpdateAI(uint32 diff) override
        {
            UpdateVictim();

            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_START_ATTACK:
                        _droppedOff = true;
                        me->SetReactState(REACT_AGGRESSIVE);
                        if (Unit* target = me->SelectNearestHostileUnitInAggroRange())
                            me->AI()->AttackStart(target);
                        else if (Player* player = me->SelectNearestPlayer(200.0f)) // Extend search
                            me->AI()->AttackStart(player);
                        break;
                    case EVENT_DEGENERATION:
                        if (me->IsWithinMeleeRange(me->GetVictim()))
                        {
                            DoCastAOE(SPELL_DEGENERATION);
                            _events.ScheduleEvent(EVENT_DEGENERATION, urand(8.5 * IN_MILLISECONDS, 9.5 * IN_MILLISECONDS));
                        }
                        else
                            _events.ScheduleEvent(EVENT_DEGENERATION, 1 * IN_MILLISECONDS);
                        break;
                    case EVENT_BRUTAL_STRIKE:
                        if (me->IsWithinMeleeRange(me->GetVictim()))
                        {
                            DoCastVictim(SPELL_BRUTAL_STRIKE);
                            _events.ScheduleEvent(EVENT_BRUTAL_STRIKE, urand(8.5 * IN_MILLISECONDS, 9.5 * IN_MILLISECONDS));
                        }
                        else
                            _events.ScheduleEvent(EVENT_BRUTAL_STRIKE, 1 * IN_MILLISECONDS);
                        break;
                    case EVENT_BLADE_RUSH:
                        if (Unit* target = SelectTarget(SELECT_TARGET_FARTHEST, 0, 50.0f, true))
                            me->CastSpell(target, SPELL_BLADE_RUSH);
                        _events.ScheduleEvent(EVENT_BLADE_RUSH, 15.5 * IN_MILLISECONDS);
                        break;
                    default:
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }

        bool CanAIAttack(Unit const* target) const override
        {
            return _droppedOff;
        }

    private:
        EventMap _events;
        bool _droppedOff;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_twilight_elite_vrykulAI(creature);
    }
};

// http://www.wowhead.com/spell=108045
class spell_blackhorn_vengeance : public SpellScriptLoader
{
    public:
        spell_blackhorn_vengeance() : SpellScriptLoader("spell_blackhorn_vengeance") {}

        class spell_blackhorn_vengeance_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_blackhorn_vengeance_AuraScript);

            void HandlePeriodic(AuraEffect const* aurEff)
            {
                if (Unit* caster = GetCaster())
                if (AuraEffect* effect = GetAura()->GetEffect(EFFECT_0))
                    effect->RecalculateAmount(caster);
            }

            void RecalculateDamage(AuraEffect const* aurEff, int32& amount, bool& canBeRecalculated) //Does not send visual % update in the tooltip to the client?
            {
                canBeRecalculated = true;
                amount = 0;
                if (Unit* caster = GetCaster())
                    amount = int32(100 - caster->GetHealthPct());
            }

            void Register() override
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_blackhorn_vengeance_AuraScript::HandlePeriodic, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY);
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_blackhorn_vengeance_AuraScript::RecalculateDamage, EFFECT_0, SPELL_AURA_MOD_DAMAGE_PERCENT_DONE);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_blackhorn_vengeance_AuraScript();
        }
};

// http://www.wowhead.com/spell=110137
class spell_blackhorn_shockwave : public SpellScriptLoader
{
public:
    spell_blackhorn_shockwave() : SpellScriptLoader("spell_blackhorn_shockwave") {}

    class spell_blackhorn_shockwave_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_blackhorn_shockwave_SpellScript);

        bool Validate(SpellInfo const* /*spell*/) override
        {
            return ValidateSpellInfo({ SPELL_SHOCKWAVE });
        }

        void HandleDummy(SpellEffIndex /*effIndex*/)
        {
            Creature* caster = GetCaster()->ToCreature();
            if (!caster)
                return;

            caster->CastSpell(GetHitUnit(), uint32(GetEffectValue()));
        }

        void FilterTargets(std::list<WorldObject*>& targets)
        {
            if (!targets.empty())
                Trinity::Containers::RandomResize(targets, 1);
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_blackhorn_shockwave_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_blackhorn_shockwave_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_blackhorn_shockwave_SpellScript();
    }
};

// http://www.wowhead.com/spell=108046
class spell_blackhorn_shockwave_two : public SpellScriptLoader
{
public:
    spell_blackhorn_shockwave_two() : SpellScriptLoader("spell_blackhorn_shockwave_two") {}

    class spell_blackhorn_shockwave_two_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_blackhorn_shockwave_two_SpellScript);

        void HandleCast()
        {
            Creature* caster = GetCaster()->ToCreature();
            if (!caster)
                return;

            caster->SetReactState(REACT_AGGRESSIVE);
            //caster->Attack(caster->AI()->SelectTarget(SELECT_TARGET_TOPAGGRO), true);
        }

        void Register() override
        {
            AfterCast += SpellCastFn(spell_blackhorn_shockwave_two_SpellScript::HandleCast);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_blackhorn_shockwave_two_SpellScript();
    }
};

// http://www.wowhead.com/spell=107513
// http://www.wowhead.com/spell=107514
class spell_blackhorn_change_speed : public SpellScriptLoader
{
public:
    spell_blackhorn_change_speed() : SpellScriptLoader("spell_blackhorn_change_speed") {}

    class spell_blackhorn_change_speed_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_blackhorn_change_speed_SpellScript);

        void HandleScript()
        {
            if (InstanceScript* instance = GetCaster()->GetInstanceScript())
            {
                if (Creature* pursuitController = ObjectAccessor::GetCreature(*GetCaster(), instance->GetGuidData(NPC_GUNSHIP_PURSUIT_CONTROLLER)))
                {
                    if (GetSpellInfo()->Id == SPELL_LOSING_SPEED)
                    {
                        //SendMapObjEvent(ANIM_HANDS_CLOSED, CL_GUID_SKYFIRE);
                        pursuitController->RemoveAurasDueToSpell(SPELL_ENGINE_SOUND);
                    }
                    else
                    {
                        //SendMapObjEvent(ANIM_SLEEP, CL_GUID_SKYFIRE);
                        pursuitController->CastSpell((Unit*)NULL, SPELL_ENGINE_SOUND, true);
                    }
                }
            }
        }

        //void SendMapObjEvent(uint32 animId, uint32 objGuid, uint8 length = 6, uint32 unkFlag = 1)
        //{
        //    WorldPacket* packet = new WorldPacket(SMSG_MAP_OBJ_EVENTS, 14);
        //    *packet << uint32(objGuid);
        //    *packet << uint32(length);
        //    *packet << uint8(1);
        //    *packet << uint8(unkFlag);
        //    *packet << uint32(animId);
        //    GetCaster()->GetMap()->SendToPlayers(packet);
        //}

        void Register() override
        {
            OnCast += SpellCastFn(spell_blackhorn_change_speed_SpellScript::HandleScript);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_blackhorn_change_speed_SpellScript();
    }
};

// http://www.wowhead.com/spell=107286
class spell_blackhorn_twilight_barrage : public SpellScriptLoader
{
    public:
        spell_blackhorn_twilight_barrage() : SpellScriptLoader("spell_blackhorn_twilight_barrage") {}

        class spell_blackhorn_twilight_barrage_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_blackhorn_twilight_barrage_SpellScript);

            void SetDestination(SpellEffIndex /*effIndex*/)
            {
                //Works really well although can never be 100% blizzlike as we dont know the coordinates they use.
                float x = frand(13415.0f, 13465.0f);
                float y = frand(-12155.0f, -12110.0f);
                float z = 152.0f;
                z = GetCaster()->GetMap()->GetHeight(GetCaster()->GetPhaseShift(), x, y, z);
                const_cast<WorldLocation*>(GetExplTargetDest())->Relocate(x, y, z);
                GetHitDest()->Relocate(x, y, z);
            }

            void Register() override
            {
                OnEffectLaunch += SpellEffectFn(spell_blackhorn_twilight_barrage_SpellScript::SetDestination, EFFECT_0, SPELL_EFFECT_TRIGGER_MISSILE);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_blackhorn_twilight_barrage_SpellScript();
        }
};

// http://www.wowhead.com/spell=107439
// http://www.wowhead.com/spell=109203
// http://www.wowhead.com/spell=109204
// http://www.wowhead.com/spell=109205
class spell_blackhorn_twilight_barrage_aoe : public SpellScriptLoader
{
public:
    spell_blackhorn_twilight_barrage_aoe() : SpellScriptLoader("spell_blackhorn_twilight_barrage_aoe") {}

    class spell_blackhorn_twilight_barrage_aoe_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_blackhorn_twilight_barrage_aoe_SpellScript);

        bool Load() override
        {
            _targets = 0;
            return true;
        }

        void HandleEffect()
        {
            SetHitDamage(GetHitDamage() / _targets);
        }

        void CheckPlayers()
        {
            if (!_targets)
            if (GetCaster()->IsAIEnabled)
                GetCaster()->GetAI()->DoCastAOE(SPELL_TWILIGHT_SKYFIRE_BARRAGE, true);
        }

        void FilterTargets(std::list<WorldObject*>& targets)
        {
            targets.remove(GetExplTargetWorldObject());
            targets.remove_if([](WorldObject* target)
            {
                return target->GetTypeId() != TYPEID_PLAYER;
            });

            _targets = targets.size();
        }

        void Register() override
        {
            OnHit += SpellHitFn(spell_blackhorn_twilight_barrage_aoe_SpellScript::HandleEffect);
            AfterCast += SpellCastFn(spell_blackhorn_twilight_barrage_aoe_SpellScript::CheckPlayers);
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_blackhorn_twilight_barrage_aoe_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_DEST_AREA_ENEMY);
        }

    private:
        uint32 _targets;
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_blackhorn_twilight_barrage_aoe_SpellScript();
    }
};

// http://www.wowhead.com/spell=107501
class spell_blackhorn_twilight_skyfire_barrage : public SpellScriptLoader
{
public:
    spell_blackhorn_twilight_skyfire_barrage() : SpellScriptLoader("spell_blackhorn_twilight_skyfire_barrage") {}

    class spell_blackhorn_twilight_skyfire_barrage_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_blackhorn_twilight_skyfire_barrage_SpellScript);

        bool Validate(SpellInfo const* /*spell*/) override
        {
            return ValidateSpellInfo({ SPELL_TWILIGHT_BARRAGE_AOE });
        }

        void HandleEffect()
        {
            SetHitDamage(sSpellMgr->GetSpellInfo(SPELL_TWILIGHT_BARRAGE_AOE)->GetEffect(EFFECT_0)->BasePoints);
        }

        void SelectTarget(WorldObject*& target)
        {
            if (InstanceScript* instance = target->GetInstanceScript())
                if (Creature* skyfire = ObjectAccessor::GetCreature(*GetCaster(), instance->GetGuidData(NPC_THE_SKYFIRE)))
                    target = skyfire;
        }

        void Register() override
        {
            OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_blackhorn_twilight_skyfire_barrage_SpellScript::SelectTarget, EFFECT_0, TARGET_UNIT_NEARBY_ENTRY);
            OnHit += SpellHitFn(spell_blackhorn_twilight_skyfire_barrage_SpellScript::HandleEffect);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_blackhorn_twilight_skyfire_barrage_SpellScript();
    }
};

// http://www.wowhead.com/spell=108038
class spell_blackhorn_harpoon : public SpellScriptLoader
{
    public:
        spell_blackhorn_harpoon() : SpellScriptLoader("spell_blackhorn_harpoon") {}

        class spell_blackhorn_harpoon_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_blackhorn_harpoon_SpellScript);

            void SelectDrake(WorldObject*& target)
            {
                target = (WorldObject*)NULL;
                if (Creature* caster = GetCaster()->ToCreature())
                    if (caster->IsAIEnabled)
                        if (Creature* _target = ObjectAccessor::GetCreature(*caster, caster->AI()->GetGUID(DATA_TURRET_TARGET)))
                            target = _target;
            }

            void Register() override
            {
                OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_blackhorn_harpoon_SpellScript::SelectDrake, EFFECT_0, TARGET_UNIT_NEARBY_ENTRY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_blackhorn_harpoon_SpellScript();
        }

        class spell_blackhorn_harpoon_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_blackhorn_harpoon_AuraScript);

            void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (!GetCaster())
                    return;

                std::list<Creature*> cannons;
                GetCaster()->GetCreatureListWithEntryInGrid(cannons, NPC_SKYFIRE_CANNON, 30.0f);
                for (auto cannon : cannons)
                    if (cannon->IsAIEnabled)
                        cannon->AI()->SetGUID(GetTarget()->GetGUID(), DATA_TURRET_TARGET);

                if (Creature* harpoon = GetCaster()->ToCreature())
                    if (harpoon->IsAIEnabled)
                        harpoon->AI()->SetGUID(GetTarget()->GetGUID(), ID_TURRET_REMOVE_TARGET);
            }

            void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (!GetCaster())
                    return;

                if (!GetCaster())
                    return;

                std::list<Creature*> cannons;
                GetCaster()->GetCreatureListWithEntryInGrid(cannons, NPC_SKYFIRE_CANNON, 30.0f);
                for (auto cannon : cannons)
                    if (cannon->IsAIEnabled)
                        cannon->AI()->SetGUID(ObjectGuid::Empty, DATA_TURRET_TARGET);

                if (Creature* caster = GetCaster()->ToCreature())
                    if (caster->IsAIEnabled)
                        caster->AI()->DoCastAOE(SPELL_RELOAD);
            }

            void Register() override
            {
                AfterEffectApply += AuraEffectRemoveFn(spell_blackhorn_harpoon_AuraScript::OnApply, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
                AfterEffectRemove += AuraEffectRemoveFn(spell_blackhorn_harpoon_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_blackhorn_harpoon_AuraScript();
        }
};

// http://www.wowhead.com/spell=108039
class spell_blackhorn_reloading : public SpellScriptLoader
{
    public:
        spell_blackhorn_reloading() : SpellScriptLoader("spell_blackhorn_reloading") {}

        class spell_blackhorn_reloading_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_blackhorn_reloading_SpellScript);

            bool Load() override
            {
                return GetCaster()->GetTypeId() == TYPEID_UNIT;
            }

            void HandlePostCast()
            {
                if (!GetCaster())
                    return;

                if (Creature* caster = GetCaster()->ToCreature())
                    caster->AI()->DoAction(ACTION_RELOAD_FINISH);
            }

            void Register()
            {
                AfterCast += SpellCastFn(spell_blackhorn_reloading_SpellScript::HandlePostCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_blackhorn_reloading_SpellScript();
        }
};

// http://www.wowhead.com/spell=108010
// http://www.wowhead.com/spell=108041
class spell_skyfire_cannon_artillery : public SpellScriptLoader
{
public:
    spell_skyfire_cannon_artillery() : SpellScriptLoader("spell_skyfire_cannon_artillery") {}

    class spell_skyfire_cannon_artillery_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_skyfire_cannon_artillery_SpellScript);

        void SelectDrake(WorldObject*& target)
        {
            target = (WorldObject*)NULL;
            if (Creature* caster = GetCaster()->ToCreature())
                if (caster->IsAIEnabled)
                    if (Creature* _target = ObjectAccessor::GetCreature(*caster, caster->AI()->GetGUID(DATA_TURRET_TARGET)))
                        target = _target;
        }

        void Register() override
        {
            OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_skyfire_cannon_artillery_SpellScript::SelectDrake, EFFECT_0, TARGET_UNIT_NEARBY_ENTRY);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_skyfire_cannon_artillery_SpellScript();
    }
};

// http://www.wowhead.com/spell=107586
class spell_goriona_call_onslaught : public SpellScriptLoader
{
    public:
        spell_goriona_call_onslaught() : SpellScriptLoader("spell_goriona_call_onslaught") {}

        class spell_goriona_call_onslaught_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_goriona_call_onslaught_SpellScript);

            void SelectTarget(WorldObject*& target)
            {
                if (InstanceScript* instance = target->GetInstanceScript())
                    if (Creature* goriona = ObjectAccessor::GetCreature(*GetCaster(), instance->GetGuidData(NPC_GORIONA)))
                        target = goriona;
            }

            void Register() override
            {
                OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_goriona_call_onslaught_SpellScript::SelectTarget, EFFECT_0, TARGET_UNIT_NEARBY_ENTRY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_goriona_call_onslaught_SpellScript();
        }
};

// http://www.wowhead.com/spell=107583
class spell_goriona_twilight_blast : public SpellScriptLoader
{
    public:
        spell_goriona_twilight_blast() : SpellScriptLoader("spell_goriona_twilight_blast") {}

        class spell_goriona_twilight_blast_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_goriona_twilight_blast_SpellScript);

            bool Validate(SpellInfo const* /*spell*/) override
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_TWILIGHT_BLAST))
                    return false;
                return true;
            }

            void EngineSelector(WorldObject*& target)
            {
                target = (WorldObject*)NULL;
                std::list<Creature*> engineTriggers;
                GetCaster()->GetCreatureListWithEntryInGrid(engineTriggers, NPC_ENGINE_STALKER, 150.0f);
                engineTriggers.remove_if(Trinity::UnitAuraCheck(true, SPELL_ENGINE_FIRE));
                engineTriggers.sort(Trinity::ObjectDistanceOrderPred(GetCaster()));
                if (Creature* trigger = engineTriggers.front())
                    target = trigger;
            }

            void SpellHit()
            {
                GetHitUnit()->CastSpell(GetHitUnit(), SPELL_ENGINE_FIRE, true);
            }

            void Register() override
            {
                OnHit += SpellHitFn(spell_goriona_twilight_blast_SpellScript::SpellHit);
                OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_goriona_twilight_blast_SpellScript::EngineSelector, EFFECT_0, TARGET_UNIT_NEARBY_ENTRY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_goriona_twilight_blast_SpellScript();
        }
};

// http://www.wowhead.com/spell=107588
class spell_goriona_twilight_onslaught_missile : public SpellScriptLoader
{
    public:
        spell_goriona_twilight_onslaught_missile() : SpellScriptLoader("spell_goriona_twilight_onslaught_missile") {}

        class spell_goriona_twilight_onslaught_missile_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_goriona_twilight_onslaught_missile_SpellScript);

            void SelectTarget(WorldObject*& target)
            {
                std::cout << "\nTest Missile Target Filter\n";
                if (Creature* onslaughtTarget = GetCaster()->FindNearestCreature(NPC_ONSLAUGHT_TARGET, 200.0f))
                    target = onslaughtTarget;
            }
            void Register() override
            {
                OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_goriona_twilight_onslaught_missile_SpellScript::SelectTarget, EFFECT_0, TARGET_DEST_NEARBY_ENTRY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_goriona_twilight_onslaught_missile_SpellScript();
        }
};

// http://www.wowhead.com/spell=106401
class spell_goriona_twilight_onslaught : public SpellScriptLoader
{
    public:
        spell_goriona_twilight_onslaught() : SpellScriptLoader("spell_goriona_twilight_onslaught") {}

        class spell_goriona_twilight_onslaught_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_goriona_twilight_onslaught_SpellScript);

            bool Load() override
            {
                _targets = 0;
                return true;
            }

            void HandleEffect()
            {
                SetHitDamage(GetHitDamage() / _targets);
            }

            void CheckPlayers()
            {
                int basePoints0 = int32(GetHitDamage() / _targets);
                if (GetCaster())
                {
                    GetCaster()->CastCustomSpell((Unit*)NULL, SPELL_TWILIGHT_ONSLAUGHT_DAMAGE, &basePoints0, NULL, NULL, true);
                    if (Creature* onslaughtTarget = GetCaster()->FindNearestCreature(NPC_ONSLAUGHT_TARGET, 200.0f))
                        onslaughtTarget->RemoveAurasDueToSpell(SPELL_TWILIGHT_ONSLAUGHT_AURA);
                }
            }

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                targets.remove(GetExplTargetWorldObject());
                targets.remove_if([](WorldObject* target)
                {
                    return target->GetTypeId() != TYPEID_PLAYER;
                });

                _targets = targets.size() + 1; // + 1 for the gunship
            }

            void Register() override
            {
                OnHit += SpellHitFn(spell_goriona_twilight_onslaught_SpellScript::HandleEffect);
                AfterCast += SpellCastFn(spell_goriona_twilight_onslaught_SpellScript::CheckPlayers);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_goriona_twilight_onslaught_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_DEST_AREA_ENEMY);
            }

        private:
            uint32 _targets;
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_goriona_twilight_onslaught_SpellScript();
        }
};

// http://www.wowhead.com/spell=107589
class spell_goriona_twilight_onslaught_damage : public SpellScriptLoader
{
    public:
        spell_goriona_twilight_onslaught_damage() : SpellScriptLoader("spell_goriona_twilight_onslaught_damage") {}

        class spell_goriona_twilight_onslaught_damage_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_goriona_twilight_onslaught_damage_SpellScript);

            bool Validate(SpellInfo const* /*spell*/) override
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_TWILIGHT_ONSLAUGHT_DAMAGE))
                    return false;
                return true;
            }

            void HandleEffect()
            {
                SetHitDamage(sSpellMgr->GetSpellInfo(SPELL_TWILIGHT_BARRAGE_AOE)->GetEffect(EFFECT_0)->BasePoints);
            }

            void SelectTarget(WorldObject*& target)
            {
                if (InstanceScript* instance = target->GetInstanceScript())
                    if (Creature* skyfire = ObjectAccessor::GetCreature(*GetCaster(), instance->GetGuidData(NPC_THE_SKYFIRE)))
                        target = skyfire;
            }

            void Register() override
            {
                OnHit += SpellHitFn(spell_goriona_twilight_onslaught_damage_SpellScript::HandleEffect);
                OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_goriona_twilight_onslaught_damage_SpellScript::SelectTarget, EFFECT_0, TARGET_UNIT_NEARBY_ENTRY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_goriona_twilight_onslaught_damage_SpellScript();
        }
};

// http://www.wowhead.com/spell=110153
class spell_goriona_broadside : public SpellScriptLoader
{
    public:
        spell_goriona_broadside() : SpellScriptLoader("spell_goriona_broadside") {}

        class spell_goriona_broadside_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_goriona_broadside_SpellScript);

            bool Load() override
            {
                _targets = 0;
                return true;
            }

            void HandleEffect()
            {
                if (!GetCaster())
                    return;

                SetHitDamage(GetHitDamage() / _targets);
                if (InstanceScript* instance = GetCaster()->GetInstanceScript())
                    if (Creature* skyfire = ObjectAccessor::GetCreature(*GetCaster(), instance->GetGuidData(NPC_THE_SKYFIRE)))
                    {
                        int basePoints0 = (skyfire->GetHealth() / (100 / GetSpellInfo()->GetEffect(EFFECT_0)->BasePoints)) / _targets;
                        GetCaster()->CastCustomSpell((Unit*)NULL, SPELL_BROADSIDE_DAMAGE, &basePoints0, NULL, NULL, true);
                    }
            }

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                targets.remove(GetExplTargetWorldObject());
                targets.remove_if([](WorldObject* target)
                {
                    return target->GetEntry() != NPC_GENERAL_PURPOSE_DUMMY_JMF;
                });

                _targets = targets.size();
            }

            void Register() override
            {
                OnHit += SpellHitFn(spell_goriona_broadside_SpellScript::HandleEffect);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_goriona_broadside_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
            }

        private:
            uint32 _targets;
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_goriona_broadside_SpellScript();
        }
};

// http://www.wowhead.com/spell=110157
class spell_goriona_broadside_damage : public SpellScriptLoader
{
    public:
        spell_goriona_broadside_damage() : SpellScriptLoader("spell_goriona_broadside_damage") {}

        class spell_goriona_broadside_damage_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_goriona_broadside_damage_SpellScript);

            bool Validate(SpellInfo const* /*spell*/) override
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_BROADSIDE_DAMAGE))
                    return false;
                return true;
            }

            void SelectTarget(WorldObject*& target)
            {
                if (InstanceScript* instance = target->GetInstanceScript())
                    if (Creature* skyfire = ObjectAccessor::GetCreature(*GetCaster(), instance->GetGuidData(NPC_THE_SKYFIRE)))
                        target = skyfire;
            }

            void Register() override
            {
                OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_goriona_broadside_damage_SpellScript::SelectTarget, EFFECT_0, TARGET_UNIT_NEARBY_ENTRY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_goriona_broadside_damage_SpellScript();
        }
};

// http://www.wowhead.com/spell=107518
class spell_twilight_sapper_detonate : public SpellScriptLoader
{
    public:
        spell_twilight_sapper_detonate() : SpellScriptLoader("spell_twilight_sapper_detonate") {}

        class spell_twilight_sapper_detonate_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_twilight_sapper_detonate_SpellScript);

            bool Validate(SpellInfo const* /*spell*/) override
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_TWILIGHT_BLAST))
                    return false;
                return true;
            }

            void EngineSelector(WorldObject*& target)
            {
                target = (WorldObject*)NULL;
                if (InstanceScript* instance = GetCaster()->GetInstanceScript())
                    if (Creature* skyfire = ObjectAccessor::GetCreature(*GetCaster(), instance->GetGuidData(NPC_THE_SKYFIRE)))
                        target = skyfire;
            }

            void SpellHit()
            {
                GetHitUnit()->CastSpell(GetHitUnit(), SPELL_ENGINE_FIRE, true);
            }

            void Register() override
            {
                OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_twilight_sapper_detonate_SpellScript::EngineSelector, EFFECT_1, TARGET_UNIT_NEARBY_ENTRY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_twilight_sapper_detonate_SpellScript();
        }
};

// http://www.wowhead.com/spell=106085
class spell_play_movie_deathwing_2 : public SpellScriptLoader
{
    public:
        spell_play_movie_deathwing_2() : SpellScriptLoader("spell_play_movie_deathwing_2") {}

        class spell_play_movie_deathwing_2_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_play_movie_deathwing_2_SpellScript);

            bool Validate(SpellInfo const* /*spell*/) override
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_PLAY_SPINE_OF_DEATHWING_CINEMATIC))
                    return false;
                return true;
            }

            void SpellHit()
            {
                GetHitPlayer()->SendMovieStart(GetSpellInfo()->GetEffect(EFFECT_0)->BasePoints);
            }

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                targets.remove_if([](WorldObject* target)
                {
                    return target->GetTypeId() != TYPEID_PLAYER;
                });
            }

            void Register() override
            {
                OnHit += SpellHitFn(spell_play_movie_deathwing_2_SpellScript::SpellHit);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_play_movie_deathwing_2_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_DEST_AREA_ENEMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_play_movie_deathwing_2_SpellScript();
        }
};

// http://www.wowhead.com/spell=109445/deck-fire
// http://www.wowhead.com/spell=109470/deck-fire-spawn
class spell_warmaster_blackhorn_deck_fire : public SpellScriptLoader
{
    public:
        spell_warmaster_blackhorn_deck_fire() : SpellScriptLoader("spell_warmaster_blackhorn_deck_fire") { }

        class spell_warmaster_blackhorn_deck_fire_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warmaster_blackhorn_deck_fire_SpellScript);

            void SpellHit(SpellEffIndex effIndex)
            {
                /*if (!GetCaster())
                    PreventHitEffect(EFFECT_0);*/

                WorldLocation* dest = const_cast<WorldLocation*>(GetExplTargetDest());
                Position fireDest = GetEligibleFireLocation(GetCaster(), dest->GetPosition(), SPELL_DECK_FIRE, SPELL_DECK_FIRE_SPAWN);
                fireDest.m_positionZ = GetCaster()->GetMap()->GetHeight(GetCaster()->GetPhaseShift(), fireDest.m_positionX, fireDest.m_positionY, GetCaster()->GetPositionZ());
                GetHitDest()->Relocate(fireDest);
            }

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                targets.remove_if([](WorldObject* target){
                    return target->GetTypeId() != TYPEID_PLAYER;
                });

                if (targets.empty())
                    return;

                Trinity::Containers::RandomResize(targets, 1);
            }

            void Register() override
            {
                OnEffectHit += SpellEffectFn(spell_warmaster_blackhorn_deck_fire_SpellScript::SpellHit, EFFECT_0, SPELL_EFFECT_PERSISTENT_AREA_AURA);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_warmaster_blackhorn_deck_fire_SpellScript::FilterTargets, EFFECT_1, TARGET_UNIT_DEST_AREA_ENEMY);
                //OnEffectHitTarget += SpellEffectFn(spell_warmaster_blackhorn_deck_fire_SpellScript::HandleDummy, EFFECT_1, SPELL_EFFECT_DUMMY);
            }

        private:
            Position GetEligibleFireLocation(Unit* caster, Position target, uint32 spellId1, uint32 spellId2)
            {
                Position pos = GetConvertedFireGridCoordinate(target);
                std::vector<DynamicObject*> deckfires = caster->GetDynObjects(spellId1);
                std::vector<DynamicObject*> dynobjs = caster->GetDynObjects(spellId2);
                deckfires.insert(deckfires.end(), dynobjs.begin(), dynobjs.end());
                target.m_positionZ = 150.0f;

                std::list<Position> invalidPositions;

                for (auto fire : deckfires)
                {
                    invalidPositions.push_back(fire->GetPosition());
                }

                bool valid = true;
                for (auto ipos : invalidPositions)
                    if (pos.IsInDist2d(&ipos, 5.0f))
                        valid = false;

                if (valid)
                    return pos;
                else
                    pos = GetRandomGeneratedFirePosition(invalidPositions, pos);


                return pos;
            }

            Position GetConvertedFireGridCoordinate(Position base)
            {
                Position pos = FireBaseCoordinates;
                uint32 itr = 0;
                if (pos.m_positionX < 0.0f)
                    while (pos.m_positionX - 5.0f > base.m_positionX)
                        pos.m_positionX -= 5.0f;
                else
                    while (pos.m_positionX + 5.0f < base.m_positionX)
                        pos.m_positionX += 5.0f;
                /*while ((pos.m_positionX + 5.0f - base.m_positionX) || pos.m_positionX - 5.0f < base.m_positionX && itr < 500)
                {
                    itr++;
                    pos.m_positionX += pos.m_positionX + 5.0f > base.m_positionX ? -5.0f : 5.0f;
                }*/

                if (itr >= 100)
                    TC_LOG_DEBUG("scripts", "Warmaster Blackhorn [Deck Fire Controller]: Unable to generate valid Deck Fire coordinate after 100 iterations, player X Coordinate > 2500.0f yards from source.");

                itr = 0;
                if (pos.m_positionY < 0.0f)
                    while (pos.m_positionY - 5.0f > base.m_positionY)
                        pos.m_positionY -= 5.0f;
                else
                    while (pos.m_positionY + 5.0f < base.m_positionY)
                        pos.m_positionY += 5.0f;
               /* while (pos.m_positionY + 5.0f > base.m_positionY || pos.m_positionY - 5.0f < base.m_positionY && itr < 500)
                {
                    itr++;
                    pos.m_positionY += pos.m_positionY + 5.0f > base.m_positionY ? -5.0f : 5.0f;
                }*/

                if (itr >= 100)
                    TC_LOG_DEBUG("scripts", "Warmaster Blackhorn [Deck Fire Controller]: Unable to generate valid Deck Fire coordinate after 100 iterations, player Y Coordinate > 2500.0f yards from source.");

                return pos;
            }

            Position GetRandomGeneratedFirePosition(std::list<Position> invalidPositions, Position pos, uint32 maxIterations = 20)
            {
                std::list<Position> validPositions;
                Position defaultPos = pos;
                float X = 10.0f;
                float Y = 10.0f;
                while (--maxIterations > 0)
                {
                    float defaultX = X;
                    float defaultY = Y;

                    for (float _X = 0.0f; _X <= X; _X += 5.0f)
                    {
                        for (float _Y = 0.0f; _Y <= Y; _Y += 5.0f)
                        {
                            Position attemptPos = pos;
                            attemptPos.m_positionX += _X;
                            if (IsValidFireCoordinate(invalidPositions, attemptPos))
                                return attemptPos;

                            attemptPos.m_positionY += _Y;
                            if (IsValidFireCoordinate(invalidPositions, attemptPos))
                                return attemptPos;

                            attemptPos.m_positionX -= _X + _X;
                            if (IsValidFireCoordinate(invalidPositions, attemptPos))
                                return attemptPos;

                            attemptPos.m_positionY -= _Y + _Y;
                            if (IsValidFireCoordinate(invalidPositions, attemptPos))
                                return attemptPos;
                        }
                    }

                    X = defaultX + 5.0f;
                    Y = defaultY + 5.0f;
                }

                Position toReturn = (Position)NULL;
                float range = 0.0f;

                while (toReturn == (Position)NULL && range < 50.0f)
                {
                    std::list<Position> possibleLocs;
                    range += 5.0f;
                    for (auto position : validPositions)
                        if (pos.IsInDist2d(&position, range))
                            possibleLocs.push_back(position);

                    if (!possibleLocs.empty())
                    {
                        toReturn = possibleLocs.size() > 1 ? Trinity::Containers::SelectRandomContainerElement(possibleLocs) : possibleLocs.front();
                        break;
                    }
                }

                return toReturn;
            }

            bool IsValidFireCoordinate(std::list<Position> invalidPositions, Position testPos)
            {
                bool valid = true;
                for (auto pos : invalidPositions)
                    if (pos.IsInDist2d(&testPos, 5.0f))
                        valid = false;

                return valid;
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_warmaster_blackhorn_deck_fire_SpellScript();
        }
};

//// http://www.wowhead.com/spell=110094/deck-fire-damage-periodic
//class spell_deck_fire_damage_periodic : public SpellScriptLoader
//{
//    public:
//        spell_deck_fire_damage_periodic() : SpellScriptLoader("spell_deck_fire_damage_periodic") {}
//
//        class spell_deck_fire_damage_periodic_SpellScript : public SpellScript
//        {
//            PrepareSpellScript(spell_deck_fire_damage_periodic_SpellScript);
//
//            void SpellHit()
//            {
//                GetCaster()->CastSpell(GetHitUnit(), SPELL_DECK_FIRE_DAMAGE, true);
//            }
//
//            void FilterTargets(std::list<WorldObject*>& targets)
//            {
//                Creature* caster = GetCaster()->ToCreature();
//                if (!caster || _dynObjects.empty())
//                    return;
//
//                GetCaster()->GetAllDynObjects(_dynObjects, SPELL_DECK_FIRE);
//                GetCaster()->GetAllDynObjects(_dynObjects, SPELL_DECK_FIRE_SPAWN);
//
//                targets.remove_if([caster](WorldObject* target)
//                {
//                    std::list<DynamicObject*> dynObjects;
//                    caster->GetAllDynObjects(dynObjects, SPELL_DECK_FIRE);
//                    caster->GetAllDynObjects(dynObjects, SPELL_DECK_FIRE_SPAWN);
//                    for (auto dynObj : dynObjects)
//                        if (target->IsWithinDist2d(dynObj, 6.0f))
//                            return false;
//
//                    return true;
//                });
//            }
//
//            void Register() override
//            {
//                OnHit += SpellHitFn(spell_deck_fire_damage_periodic_SpellScript::SpellHit);
//                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_deck_fire_damage_periodic_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
//            }
//
//            std::list<DynamicObject*> _dynObjects;
//        };
//
//        SpellScript* GetSpellScript() const override
//        {
//            return new spell_deck_fire_damage_periodic_SpellScript();
//        }
//};

// http://www.wowhead.com/spell=110092/deck-fire-damage-periodic
class spell_deck_fire_damage_periodic : public SpellScriptLoader
{
public:
    spell_deck_fire_damage_periodic() : SpellScriptLoader("spell_deck_fire_damage_periodic") {}

    class spell_deck_fire_damage_periodic_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_deck_fire_damage_periodic_AuraScript);

        void HandlePeriodic(AuraEffect const* /*aurEff*/)
        {
            PreventDefaultAction();
            Creature* caster = GetCaster()->ToCreature();
            if (!caster)
                return;

            Map::PlayerList const& players = GetCaster()->GetMap()->GetPlayers();

            std::vector<DynamicObject*> deckfires;
            deckfires = caster->GetDynObjects(SPELL_DECK_FIRE);
            std::vector<DynamicObject*> dynObjs = caster->GetDynObjects(SPELL_DECK_FIRE_SPAWN);
            deckfires.insert(deckfires.end(), dynObjs.begin(), dynObjs.end());

            std::set<Player*> targets;
            for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                for (auto dynObj : deckfires)
                    if (itr->GetSource()->IsWithinDist3d(dynObj, 3.0f))
                    {
                        targets.insert(itr->GetSource());
                        break;
                    }

            for (auto target : targets)
                GetCaster()->CastSpell(target, SPELL_DECK_FIRE_DAMAGE, true);
        }

        void Register() override
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_deck_fire_damage_periodic_AuraScript::HandlePeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_deck_fire_damage_periodic_AuraScript();
    }

    //class spell_deck_fire_damage_periodic_SpellScript : public SpellScript
    //{
    //    PrepareSpellScript(spell_deck_fire_damage_periodic_SpellScript);

    //    void OnSpellHitTarget(SpellEffIndex /*eff*/)
    //    {
    //        GetCaster()->CastSpell(GetHitUnit(), SPELL_DECK_FIRE_DAMAGE, true);
    //    }

    //    void FilterTargets(std::list<WorldObject*>& targets)
    //    {
    //        Creature* caster = GetCaster()->ToCreature();
    //        if (!caster || _dynObjects.empty())
    //            return;

    //        GetCaster()->GetAllDynObjects(_dynObjects, SPELL_DECK_FIRE);
    //        GetCaster()->GetAllDynObjects(_dynObjects, SPELL_DECK_FIRE_SPAWN);

    //        targets.remove_if([caster](WorldObject* target)
    //        {
    //            std::list<DynamicObject*> dynObjects;
    //            caster->GetAllDynObjects(dynObjects, SPELL_DECK_FIRE);
    //            caster->GetAllDynObjects(dynObjects, SPELL_DECK_FIRE_SPAWN);
    //            for (auto dynObj : dynObjects)
    //                if (target->IsWithinDist2d(dynObj, 6.0f))
    //                    return false;

    //            return true;
    //        });
    //    }

    //    void Register() override
    //    {
    //        OnEffectHitTarget += SpellEffectFn(spell_deck_fire_damage_periodic_SpellScript::OnSpellHitTarget, EFFECT_0, SPELL_EFFECT_DUMMY);
    //        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_deck_fire_damage_periodic_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
    //    }

    //    std::list<DynamicObject*> _dynObjects;
    //};

    //SpellScript* GetSpellScript() const override
    //{
    //    return new spell_deck_fire_damage_periodic_SpellScript();
    //}
};

// No DBC info
class spell_skyfire_fire_brigade_target : public SpellScriptLoader
{
    public:
        spell_skyfire_fire_brigade_target() : SpellScriptLoader("spell_skyfire_fire_brigade_target") {}

        class spell_skyfire_fire_brigade_target_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_skyfire_fire_brigade_target_SpellScript);

            void SpellHit(SpellEffIndex /*effIndex*/)
            {
                Position offset = { 0.0f, 0.0f, 15.0f, 0.0f };
                GetHitDest()->RelocateOffset(offset);
            }

            void FilterTargets(WorldObject*& target)
            {
                Creature* caster = GetCaster()->ToCreature();
                if (!caster)
                    return;

                std::list<DynamicObject*> deckfires;
                if (InstanceScript* instance = caster->GetInstanceScript())
                {
                    if (Creature* controller = ObjectAccessor::GetCreature(*caster, instance->GetGuidData(NPC_DECK_FIRE_CONTROLLER)))
                    {
                        std::vector<DynamicObject*> dynObjs = controller->GetDynObjects(SPELL_DECK_FIRE);
                        deckfires.insert(deckfires.end(), dynObjs.begin(), dynObjs.end());
                        dynObjs = controller->GetDynObjects(SPELL_DECK_FIRE_SPAWN);
                        deckfires.insert(deckfires.end(), dynObjs.begin(), dynObjs.end());
                    }
                }

                deckfires.remove_if([](DynamicObject* dynObj)
                {
                    std::list<Creature*> stalkers;
                    dynObj->GetCreatureListWithEntryInGrid(stalkers, NPC_FIRE_BRIGADE_TARGET_STALKER, 30.0f);
                    for (auto stalker : stalkers)
                        if (stalker->IsInDist2d(dynObj, 2.5f))
                            return true;

                    return false;
                });

                if (deckfires.empty())
                    return;

                target = Trinity::Containers::SelectRandomContainerElement(deckfires);
            }

            void Register() override
            {
                OnEffectHit += SpellEffectFn(spell_skyfire_fire_brigade_target_SpellScript::SpellHit, EFFECT_0, SPELL_EFFECT_SUMMON);
                OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_skyfire_fire_brigade_target_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_skyfire_fire_brigade_target_SpellScript();
        }
};

//18444
class achievement_deck_defender : public AchievementCriteriaScript
{
public:
    achievement_deck_defender() : AchievementCriteriaScript("achievement_deck_defender") {}

    bool OnCheck(Player* /*player*/, Unit* target) override
    {
        if (!target)
            return false;

        if (InstanceScript* instance = target->GetInstanceScript())
            return instance->GetData(DATA_DECK_DEFENDER) == TRUE;

        return false;
    }
};

void AddSC_boss_warmaster_blackhorn()
{
    new npc_kaanu_reevs();
    new npc_sky_captain_swayze();
    new npc_skyfire_deckhand();
    new npc_skyfire_commando();
    new npc_skyfire_harpoon_gun();
    new npc_skyfire_cannon();
    new boss_warmaster_blackhorn();
    new npc_goriona();
    new npc_blackhorn_the_skyfire();
    new npc_blackhorn_twilight_assault_drake();
    new npc_twilight_sapper();
    new npc_blackhorn_twilight_infiltrator();
    new npc_twilight_elite_vrykul();
    new npc_deck_fire_controller();

    new spell_blackhorn_vengeance();
    new spell_blackhorn_shockwave();
    new spell_blackhorn_shockwave_two();
    new spell_blackhorn_change_speed();
    new spell_blackhorn_twilight_barrage();
    new spell_blackhorn_twilight_barrage_aoe();
    new spell_blackhorn_twilight_skyfire_barrage();
    new spell_blackhorn_harpoon();
    new spell_blackhorn_reloading();
    new spell_skyfire_cannon_artillery();
    new spell_goriona_call_onslaught();
    new spell_goriona_twilight_blast();
    new spell_goriona_twilight_onslaught_missile();
    new spell_goriona_twilight_onslaught();
    new spell_goriona_twilight_onslaught_damage();
    new spell_goriona_broadside();
    new spell_goriona_broadside_damage();
    new spell_twilight_sapper_detonate();
    new spell_warmaster_blackhorn_deck_fire();
    new spell_deck_fire_damage_periodic();
    new spell_play_movie_deathwing_2();
    new spell_skyfire_fire_brigade_target();

    new achievement_deck_defender();
}
