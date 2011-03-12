/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#ifndef GS_H
#define GS_H

#include "GameScript/GameScript.h"

#include "exports.h"

#include "SymbolMgr.h"
#include "Variables.h"
#include "Scriptable/Actor.h"
#include "System/DataStream.h"

#include <cstdio>
#include <vector>

class Action;
class GameScript;

//escapearea flags
#define EA_DESTROY 1        //destroy actor at the exit (otherwise move to new place)
#define EA_NOSEE   2        //no need to see the exit

//displaystring flags
#define DS_WAIT    1
#define DS_HEAD    2
#define DS_CONSOLE 4
#define DS_CONST   8
#define DS_NONAME  16
#define DS_SILENT  32
#define DS_SPEECH  64
#define DS_AREA    128

//diffmode (iwd2)
#define DM_EQUAL   1
#define DM_LESS    2
#define DM_GREATER 3

//markspellandobject (iwd2)
#define MSO_IGNORE_SEE     1
#define MSO_IGNORE_INVALID 2
#define MSO_RANDOM_SPELL   4
#define MSO_IGNORE_HAVE    8
#define MSO_IGNORE_RANGE   16
#define MSO_IGNORE_NULL    32

//delta (pst)
#define DM_LOWER   1
#define DM_RAISE   2
#define DM_SET     3

//attack core flags
#define AC_NO_SOUND   1
#define AC_RUNNING    2

//trigger flags stored in triggers in .bcs files
#define NEGATE_TRIGGER 1

#define MAX_OBJECT_FIELDS	10
#define MAX_NESTING		5

#define GSASSERT(f,c) \
 if(!(f))  \
 {  \
 printf("Assertion failed: %s [0x%08lX] Line %d",#f, c, __LINE__); \
	abort(); \
 }

typedef std::vector<ieDword> SrcVector;

struct targettype {
	Scriptable *actor; //hmm, could be door
	unsigned int distance;
};

typedef std::list<targettype> targetlist;

class GEM_EXPORT Targets {
private:
	targetlist objects;
public:
	int Count() const;
	targettype *RemoveTargetAt(targetlist::iterator &m);
	const targettype *GetNextTarget(targetlist::iterator &m, int Type);
	const targettype *GetLastTarget(int Type);
	const targettype *GetFirstTarget(targetlist::iterator &m, int Type);
	Scriptable *GetTarget(unsigned int index, int Type);
	void AddTarget(Scriptable* target, unsigned int distance, int flags);
	void Clear();
};

class GEM_EXPORT Object {
public:
	Object()
	{
		objectName[0] = 0;

		memset( objectFields, 0, MAX_OBJECT_FIELDS * sizeof( int ) );
		memset( objectFilters, 0, MAX_NESTING * sizeof( int ) );
		memset( objectRect, 0, 4 * sizeof( int ) );
	}
public:
	int objectFields[MAX_OBJECT_FIELDS];
	int objectFilters[MAX_NESTING];
	int objectRect[4];
	char objectName[65];
public:
	void Dump()
	{
		int i;

		if(objectName[0]) {
			printf("Object: %s\n",objectName);
			return;
		}
		printf("IDS Targeting: ");
		for(i=0;i<MAX_OBJECT_FIELDS;i++) {
			printf("%d ",objectFields[i]);
		}
		printf("\n");
		printf("Filters: ");
		for(i=0;i<MAX_NESTING;i++) {
			printf("%d ",objectFilters[i]);
		}
		printf("\n");
	}

	bool ReadyToDie();
};

class GEM_EXPORT Trigger {
public:
	Trigger()
	{
		flags = 0;
		objectParameter = NULL;
		string0Parameter[0] = 0;
		string1Parameter[0] = 0;
		int0Parameter = 0;
		int1Parameter = 0;
		pointParameter.null();
	}
	~Trigger()
	{
		delete objectParameter;
	}
	int Evaluate(Scriptable* Sender);
public:
	unsigned short triggerID;
	int int0Parameter;
	int flags;
	int int1Parameter;
	int int2Parameter;
	Point pointParameter;
	char string0Parameter[65];
	char string1Parameter[65];
	Object* objectParameter;
public:
	void Dump()
	{
		printf ("Trigger: %d\n", triggerID);
		printf ("Int parameters: %d %d %d\n", int0Parameter, int1Parameter, int2Parameter);
		printf ("Point: [%d.%d]\n", pointParameter.x, pointParameter.y);
		printf ("String0: %s\n", string0Parameter);
		printf ("String1: %s\n", string1Parameter);
		if (objectParameter) {
			objectParameter->Dump();
		} else {
			printf("No object\n");
		}
		printf("\n");
	}
};

class GEM_EXPORT Condition {
public:
	~Condition()
	{
		for (size_t c = 0; c < triggers.size(); ++c) {
			delete triggers[c];
		}
	}
	bool Evaluate(Scriptable* Sender);
public:
	std::vector<Trigger*> triggers;
};

class GEM_EXPORT Action {
public:
	Action(bool autoFree)
	{
		actionID = 0;
		objects[0] = NULL;
		objects[1] = NULL;
		objects[2] = NULL;
		string0Parameter[0] = 0;
		string1Parameter[0] = 0;
		int0Parameter = 0;
		pointParameter.null();
		int1Parameter = 0;
		int2Parameter = 0;
		//changed now
		if (autoFree) {
			RefCount = 0; //refcount will be increased by each AddAction
		} else {
			RefCount = 1; //one reference hold by the script
		}
	}
	~Action()
	{
		for (int c = 0; c < 3; c++) {
			delete objects[c];
		}
	}
public:
	unsigned short actionID;
	Object* objects[3];
	int int0Parameter;
	Point pointParameter;
	int int1Parameter;
	int int2Parameter;
	char string0Parameter[65];
	char string1Parameter[65];
private:
	int RefCount;
public:
	int GetRef() {
		return RefCount;
	}
	void Dump()
	{
		int i;

		printf("Int0: %d, Int1: %d, Int2: %d\n",int0Parameter, int1Parameter, int2Parameter);
		printf("String0: %s, String1: %s\n", string0Parameter?string0Parameter:"<NULL>", string1Parameter?string1Parameter:"<NULL>");
		for (i=0;i<3;i++) {
			if (objects[i]) {
				printf( "%d. ",i+1);
				objects[i]->Dump();
			} else {
				printf( "%d. Object - NULL\n",i+1);
			}
		}

		printf("RefCount: %d\n", RefCount);
	}

	void Release()
	{
		if (!RefCount) {
			printf( "WARNING!!! Double Freeing in %s: Line %d\n", __FILE__,
				__LINE__ );
			abort();
		}
		RefCount--;
		if (!RefCount) {
			delete this;
		}
	}
	void IncRef()
	{
		RefCount++;
		if (RefCount >= 65536) {
			printf( "Refcount increased to: %d in action %d\n", RefCount,
				actionID );
			abort();
		}
	}
};

class GEM_EXPORT Response {
public:
	Response()
	{
		weight = 0;
	}
	~Response()
	{
		for (size_t c = 0; c < actions.size(); c++) {
			if (actions[c]) {
				if (actions[c]->GetRef()>2) {
					printf("Residue action %d with refcount %d\n", actions[c]->actionID, actions[c]->GetRef());
				}
				actions[c]->Release();
			}
		}
	}
	int Execute(Scriptable* Sender);
public:
	unsigned char weight;
	std::vector<Action*> actions;
};

class GEM_EXPORT ResponseSet {
public:
	ResponseSet()
	{
	}
	~ResponseSet()
	{
		for (size_t b = 0; b < responses.size(); b++) {
			delete responses[b];
		}
	}
	int Execute(Scriptable* Sender);
public:
	std::vector<Response*> responses;
};

class GEM_EXPORT ResponseBlock {
public:
	ResponseBlock()
	{
		condition = NULL;
		responseSet = NULL;
	}
	~ResponseBlock()
	{
		delete condition;
		delete responseSet;
	}
public:
	Condition* condition;
	ResponseSet* responseSet;
};

class GEM_EXPORT Script {
public:
	~Script()
	{
		for (unsigned int i = 0; i < responseBlocks.size(); i++) {
			delete responseBlocks[i];
		}
	}
public:
	std::vector<ResponseBlock*> responseBlocks;
};

typedef int (* TriggerFunction)(Scriptable*, Trigger*);
typedef void (* ActionFunction)(Scriptable*, Action*);
typedef Targets* (* ObjectFunction)(Scriptable *, Targets*, int ga_flags);
typedef int (* IDSFunction)(Actor *, int parameter);

#define TF_NONE 	0
#define TF_CONDITION    1 //this isn't a trigger, just a condition (0x4000)
#define TF_MERGESTRINGS 8 //same value as actions' mergestring

struct TriggerLink {
	const char* Name;
	TriggerFunction Function;
	short Flags;
};

//createcreature flags
#define CC_OFFSET    1
#define CC_OBJECT    2
#define CC_OFFSCREEN 3
#define CC_MASK      3
#define CC_CHECK_IMPASSABLE  4  //adjust position (searchmap)
#define CC_PLAY_ANIM 8          //play animation
#define CC_STRING1   16         //resref is in second string
#define CC_CHECK_OVERLAP 32     //other actors
#define CC_COPY      64         //copy appearance
#define CC_SCRIPTNAME 128       //scriptname in 2nd string

//begindialog flags
#define BD_STRING0   0
#define BD_TARGET    1
#define BD_SOURCE    2
#define BD_RESERVED  3  //playerX resref
#define BD_INTERACT  4  //banter dialogs
#define BD_LOCMASK   7  //where is the dialog resref
#define BD_TALKCOUNT 8  //increases talkcount
#define BD_SETDIALOG 16 //also sets dialog (for string0)
#define BD_CHECKDIST 32 //checks distance, if needs, walks up
#define BD_OWN       64 //source == target, works for player only
#define BD_INTERRUPT 128 //interrupts action
#define BD_NUMERIC   256 //target is numeric
#define BD_ITEM      512 //talk to an item
#define BD_NOEMPTY   1024 //don't display '... has nothing to say to you'

#define AF_NONE 	 0
#define AF_IMMEDIATE     1
#define AF_CONTINUE      2
#define AF_MASK 	 3   //none, immediate or continue
#define AF_BLOCKING      4
#define AF_MERGESTRINGS  8
//we could use this flag to restrict player scripts from using dangerous
//opcodes, it would be a very useful and easy to implement feature!
#define AF_RESTRICTED    16
//#define AF_RESTRICTED_LEVEL2  32 //maybe we could use 2 bits for this???
#define AF_SCRIPTLEVEL   64  //this hack will transfer scriptlevel to int0parameter at runtime (changecurrentscript relies on it)
#define AF_INVALID       128
#define AF_DIRECT        256 //this hack will transfer target from gamecontrol to object1 at compile time
#define AF_ALIVE         512 //only alive actors can do this
#define AF_INSTANT       1024

struct ActionLink {
	const char* Name;
	ActionFunction Function;
	short Flags;
};

struct ObjectLink {
	const char* Name;
	ObjectFunction Function;
};

struct IDSLink {
	const char* Name;
	IDSFunction Function;
};

#define MAX_TRIGGERS			0xFF
#define MAX_ACTIONS			400
#define MAX_OBJECTS			128
#define AI_SCRIPT_LEVEL 4             //the script level of special ai scripts

extern int RandomNumValue;

namespace GS {
//Script Functions
	GEM_EXPORT int ID_Alignment(Actor *actor, int parameter);
	GEM_EXPORT int ID_Allegiance(Actor *actor, int parameter);
	GEM_EXPORT int ID_AVClass(Actor *actor, int parameter);
	GEM_EXPORT int ID_Class(Actor *actor, int parameter);
	GEM_EXPORT int ID_ClassMask(Actor *actor, int parameter);
	GEM_EXPORT int ID_Faction(Actor *actor, int parameter);
	GEM_EXPORT int ID_Gender(Actor *actor, int parameter);
	GEM_EXPORT int ID_General(Actor *actor, int parameter);
	GEM_EXPORT int ID_Race(Actor *actor, int parameter);
	GEM_EXPORT int ID_Specific(Actor *actor, int parameter);
	GEM_EXPORT int ID_Subrace(Actor *actor, int parameter);
	GEM_EXPORT int ID_Team(Actor *actor, int parameter);

//Triggers
	GEM_EXPORT int ActionListEmpty(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int ActuallyInCombat(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int Acquired(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int Alignment(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int Allegiance(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int AnimationID(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int AnimState(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int AnyPCOnMap(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int AnyPCSeesEnemy(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int AreaCheck(Scriptable* Sender, Trigger* parameter);
	GEM_EXPORT int AreaCheckObject(Scriptable* Sender, Trigger* parameter);
	GEM_EXPORT int AreaFlag(Scriptable* Sender, Trigger* parameter);
	GEM_EXPORT int AreaRestDisabled(Scriptable* Sender, Trigger* parameter);
	GEM_EXPORT int AreaStartsWith(Scriptable* Sender, Trigger* parameter); //InWatchersKeep
	GEM_EXPORT int AreaType(Scriptable* Sender, Trigger* parameter);
	GEM_EXPORT int AtLocation(Scriptable* Sender, Trigger* parameter);
	GEM_EXPORT int AttackedBy(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int BecameVisible(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int BitCheck(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int BitCheckExact(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int BitGlobal_Trigger(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int BreakingPoint(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int CalendarDay(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int CalendarDayGT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int CalendarDayLT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int CalledByName(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int ChargeCount(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int CharName(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int CheckDoorFlags(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int CheckPartyAverageLevel(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int CheckPartyLevel(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int CheckSkill(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int CheckSkillGT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int CheckSkillLT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int CheckSpellState(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int CheckStat(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int CheckStatGT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int CheckStatLT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int Class(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int ClassEx(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int ClassLevel(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int ClassLevelGT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int ClassLevelLT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int Clicked(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int Closed(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int CombatCounter(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int CombatCounterGT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int CombatCounterLT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int Contains(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int CreatureHidden( Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int CurrentAreaIs(Scriptable* Sender, Trigger* parameters);
	//GEM_EXPORT int DamageTaken(Scriptable* Sender, Trigger* parameters);
	//GEM_EXPORT int DamageTakenGT(Scriptable* Sender, Trigger* parameters);
	//GEM_EXPORT int DamageTakenLT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int Dead(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int Delay(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int Detect(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int Die(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int Died(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int Difficulty(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int DifficultyGT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int DifficultyLT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int Disarmed(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int DisarmFailed(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int Entered(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int EntirePartyOnMap(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int Exists(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int ExtendedStateCheck(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int ExtraProficiency(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int ExtraProficiencyGT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int ExtraProficiencyLT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int Faction(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int FallenPaladin(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int FallenRanger(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int False(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int ForceMarkedSpell_Trigger(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int Frame(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int Gender(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int General(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int G_Trigger(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int Global(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int GlobalAndGlobal_Trigger(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int GlobalBAndGlobal_Trigger(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int GlobalBAndGlobalExact(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int GlobalBitGlobal_Trigger(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int GlobalGT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int GlobalGTGlobal(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int GlobalLT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int GlobalLTGlobal(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int GlobalOrGlobal_Trigger(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int GlobalsEqual(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int GlobalsGT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int GlobalsLT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int GlobalTimerExact(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int GlobalTimerExpired(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int GlobalTimerNotExpired(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int GlobalTimerStarted(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int GGT_Trigger(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int GLT_Trigger(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int Happiness(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int HappinessGT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int HappinessLT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int HarmlessEntered(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int HasBounceEffects(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int HasImmunityEffects(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int HasInnateAbility(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int HasItem(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int HasItemEquipped(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int HasItemSlot(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int HasItemTypeSlot(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int HasWeaponEquipped(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int HaveAnySpells(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int HaveSpellParty(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int HaveSpell(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int HaveUsableWeaponEquipped(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int Heard(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int Help_Trigger(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int HelpEX(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int HitBy(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int HotKey(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int HP(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int HPGT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int HPLost(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int HPLostGT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int HPLostLT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int HPLT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int HPPercent(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int HPPercentGT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int HPPercentLT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int InActiveArea(Scriptable* Sender, Trigger* parameter);
	GEM_EXPORT int InCutSceneMode(Scriptable *Sender, Trigger* parameter);
	GEM_EXPORT int InLine(Scriptable* Sender, Trigger* parameter);
	GEM_EXPORT int InMyArea(Scriptable* Sender, Trigger* parameter);
	GEM_EXPORT int InMyGroup(Scriptable* Sender, Trigger* parameter);
	GEM_EXPORT int InParty(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int InPartyAllowDead(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int InPartySlot(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int InteractingWith(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int Internal(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int InternalGT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int InternalLT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int InTrap(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int InventoryFull(Scriptable* Sender, Trigger* parameter);
	GEM_EXPORT int InWeaponRange(Scriptable* Sender, Trigger* parameter);
	GEM_EXPORT int IsAClown(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int IsActive(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int IsCreatureAreaFlag( Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int IsCreatureHiddenInShadows( Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int IsGabber(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int IsExtendedNight(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int IsFacingObject(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int IsFacingSavedRotation(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int IsLocked(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int IsMarkedSpell(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int IsOverMe(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int IsPathCriticalObject( Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int IsPlayerNumber( Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int IsRotation(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int IsSpellTargetValid( Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int IsTeamBitOn(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int IsValidForPartyDialog(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int IsWeaponRanged(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int IsWeather(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int ItemIsIdentified(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int Joins(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int Kit(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int KnowSpell(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int LastMarkedObject_Trigger(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int LastPersonTalkedTo(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int Leaves(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int Level(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int LevelGT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int LevelLT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int LevelInClass(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int LevelInClassGT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int LevelInClassLT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int LevelParty(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int LevelPartyGT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int LevelPartyLT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int LocalsEqual(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int LocalsGT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int LocalsLT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int LOS(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int ModalState(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int Morale(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int MoraleGT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int MoraleLT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int NamelessBitTheDust(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int NearbyDialog(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int NearLocation(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int NearSavedLocation(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int NightmareModeOn(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int NotStateCheck(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int NullDialog(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int NumCreatures(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int NumCreaturesAtMyLevel(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int NumCreaturesGT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int NumCreaturesGTMyLevel(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int NumCreaturesLT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int NumCreaturesLTMyLevel(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int NumCreatureVsParty(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int NumCreatureVsPartyGT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int NumCreatureVsPartyLT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int NumDead(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int NumDeadGT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int NumDeadLT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int NumItems(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int NumItemsGT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int NumItemsLT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int NumItemsParty(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int NumItemsPartyGT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int NumItemsPartyLT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int NumTimesInteracted(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int NumTimesInteractedGT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int NumTimesInteractedLT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int NumTimesInteractedObject(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int NumTimesInteractedObjectGT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int NumTimesInteractedObjectLT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int NumTimesTalkedTo(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int NumTimesTalkedToGT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int NumTimesTalkedToLT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int ObjectActionListEmpty(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int OnCreation(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int OnIsland(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int OnScreen(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int Opened(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int OpenFailed(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int OpenState(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int Or(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int OutOfAmmo(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int OwnsFloaterMessage(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int PartyCountEQ(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int PartyCountGT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int PartyCountLT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int PartyCountAliveEQ(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int PartyCountAliveGT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int PartyCountAliveLT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int PartyGold(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int PartyGoldGT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int PartyGoldLT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int PartyHasItem(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int PartyHasItemIdentified(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int PartyMemberDied(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int PartyRested(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int PCCanSeePoint(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int PCInStore(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int PersonalSpaceDistance(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int PickLockFailed(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int PickpocketFailed(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int Proficiency(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int ProficiencyGT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int ProficiencyLT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int Race(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int RandomNum(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int RandomNumGT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int RandomNumLT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int RandomStatCheck(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int Range(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int Reaction(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int ReactionLT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int ReactionGT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int RealGlobalTimerExact(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int RealGlobalTimerExpired(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int RealGlobalTimerNotExpired(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int ReceivedOrder(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int Reputation(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int ReputationGT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int ReputationLT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int School(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int See(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int Sequence(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int SetLastMarkedObject(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int SetMarkedSpell_Trigger(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int Specifics(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int SpellCast(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int SpellCastInnate(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int SpellCastOnMe(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int SpellCastPriest(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int StateCheck(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int StealFailed(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int StoreHasItem(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int StuffGlobalRandom(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int SubRace(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int SystemVariable_Trigger(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int TargetUnreachable(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int Team(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int Time(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int TimeGT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int TimeLT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int TimeOfDay(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int TimerActive(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int TimerExpired(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int TookDamage(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int TotalItemCnt(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int TotalItemCntExclude(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int TotalItemCntExcludeGT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int TotalItemCntExcludeLT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int TotalItemCntGT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int TotalItemCntLT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int TrapTriggered(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int TriggerTrigger(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int TriggerSetGlobal(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int True(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int TurnedBy(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int Unlocked(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int UnselectableVariable(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int UnselectableVariableGT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int UnselectableVariableLT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int Unusable(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int Vacant(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int WalkedToTrigger(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int WasInDialog(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int Xor(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int XP(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int XPGT(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT int XPLT(Scriptable* Sender, Trigger* parameters);
//Actions
	GEM_EXPORT void Activate(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void ActivatePortalCursor(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void AddAreaFlag(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void AddAreaType(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void AddExperienceParty(Scriptable *Sender, Action* parameters);
	GEM_EXPORT void AddExperiencePartyCR(Scriptable *Sender, Action* parameters);
	GEM_EXPORT void AddExperiencePartyGlobal(Scriptable *Sender, Action* parameters);
	GEM_EXPORT void AddFeat(Scriptable *Sender, Action* parameters);
	GEM_EXPORT void AddGlobals(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void AddHP(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void AddJournalEntry(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void AddKit(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void AddMapnote(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void AddSpecialAbility(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void AddSuperKit(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void AddWayPoint(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void AddXP2DA(Scriptable *Sender, Action* parameters);
	GEM_EXPORT void AddXPObject(Scriptable *Sender, Action* parameters);
	GEM_EXPORT void AdvanceTime(Scriptable *Sender, Action* parameters);
	GEM_EXPORT void Ally(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void AmbientActivate(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void AnkhegEmerge(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void AnkhegHide(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void ApplyDamage(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void ApplyDamagePercent(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void ApplySpell(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void ApplySpellPoint(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void AttachTransitionToDoor(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void Attack(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void AttackNoSound(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void AttackOneRound(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void AttackReevaluate(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void BanterBlockFlag(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void BanterBlockTime(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void BashDoor(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void BattleSong(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void Berserk(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void BitClear(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void BitGlobal(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void BreakInstants(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void Calm(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void ChangeAIScript(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void ChangeAIType(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void ChangeAlignment(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void ChangeAllegiance(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void ChangeAnimation(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void ChangeAnimationNoEffect(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void ChangeClass(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void ChangeColor(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void ChangeCurrentScript(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void ChangeDestination(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void ChangeDialogue(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void ChangeGender(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void ChangeGeneral(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void ChangeRace(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void ChangeSpecifics(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void ChangeStat(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void ChangeStatGlobal(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void ChangeStoreMarkup(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void ChangeTileState(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void ClearActions(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void ClearAllActions(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void ClearPartyEffects(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void ClearSpriteEffects(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void ClickLButtonObject(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void ClickLButtonPoint(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void ClickRButtonObject(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void ClickRButtonPoint(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void CloseDoor(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void ContainerEnable(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void Continue(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void CopyGroundPilesTo(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void CreateCreature(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void CreateCreatureAtLocation(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void CreateCreatureAtFeet(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void CreateCreatureCopyPoint(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void CreateCreatureDoor(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void CreateCreatureImpassable(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void CreateCreatureImpassableAllowOverlap(Scriptable* Sender,
		Action* parameters);
	GEM_EXPORT void CreateCreatureObject(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void CreateCreatureObjectCopy(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void CreateCreatureObjectDoor(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void CreateCreatureObjectOffset(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void CreateCreatureObjectOffScreen(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void CreateCreatureOffScreen(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void CreateItem(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void CreateItemNumGlobal(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void CreatePartyGold(Scriptable *Sender, Action *parameters);
	GEM_EXPORT void CreateVisualEffect(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void CreateVisualEffectObject(Scriptable* Sender,
		Action* parameters);
	GEM_EXPORT void CreateVisualEffectObjectSticky(Scriptable* Sender,
		Action* parameters);
	GEM_EXPORT void CutSceneID(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void Damage(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void DayNight(Scriptable *Sender, Action* parameters);
	GEM_EXPORT void Deactivate(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void Debug(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void DestroyAllDestructableEquipment(Scriptable* Sender,
		Action* parameters);
	GEM_EXPORT void DestroyAllEquipment(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void DestroyGold(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void DestroyItem(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void DestroyPartyGold(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void DestroyPartyItem(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void DestroyPartyItemNum(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void DestroySelf(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void DetectSecretDoor(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void Dialogue(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void DialogueForceInterrupt(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void DialogueInterrupt(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void DisableFogDither(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void DisableSpriteDither(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void DisplayMessage(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void DisplayString(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void DisplayStringHead(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void DisplayStringHeadOwner(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void DisplayStringNoName(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void DisplayStringNoNameHead(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void DisplayStringWait(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void DoubleClickLButtonObject(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void DoubleClickLButtonPoint(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void DoubleClickRButtonObject(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void DoubleClickRButtonPoint(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void DropInventory(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void DropInventoryEX(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void DropItem(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void EnableFogDither(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void EnablePortalTravel(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void EnableSpriteDither(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void EndCredits(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void EndCutSceneMode(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void Enemy(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void EscapeArea(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void EscapeAreaDestroy(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void EscapeAreaNoSee(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void EscapeAreaObject(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void EscapeAreaObjectNoSee(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void EquipItem(Scriptable *Sender, Action *parameters);
	GEM_EXPORT void EquipMostDamagingMelee(Scriptable *Sender, Action *parameters);
	GEM_EXPORT void EquipRanged(Scriptable *Sender, Action *parameters);
	GEM_EXPORT void EquipWeapon(Scriptable *Sender, Action *parameters);
	GEM_EXPORT void ExitPocketPlane(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void ExpansionEndCredits(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void Explore(Scriptable *Sender, Action *parameters);
	GEM_EXPORT void ExploreMapChunk(Scriptable *Sender, Action *parameters);
	GEM_EXPORT void ExportParty(Scriptable *Sender, Action *parameters);
	GEM_EXPORT void Face(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void FaceObject(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void FaceSavedLocation(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void FadeFromColor(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void FadeToAndFromColor(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void FadeToColor(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void FakeEffectExpiryCheck(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void FillSlot(Scriptable *Sender, Action* parameters);
	GEM_EXPORT void FindTraps(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void FixEngineRoom(Scriptable *Sender, Action* parameters);
	GEM_EXPORT void FloatMessageFixed(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void FloatMessageFixedRnd(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void FloatMessageRnd(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void FloatRebus(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void Follow(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void FollowCreature(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void FollowObjectFormation(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void ForceAIScript(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void ForceAttack(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void ForceFacing(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void ForceHide(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void ForceLeaveAreaLUA(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void ForceMarkedSpell(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void ForceSpell(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void ForceSpellPoint(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void ForceUseContainer(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void Formation(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void FullHeal(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void GenerateMaze(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void GeneratePartyMember(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void GetItem(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void GetStat(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void GiveItem(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void GiveOrder(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void GivePartyAllEquipment(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void GivePartyGold(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void GivePartyGoldGlobal(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void GlobalAddGlobal(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void GlobalAndGlobal(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void GlobalBAnd(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void GlobalBAndGlobal(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void GlobalBitGlobal(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void GlobalBOr(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void GlobalBOrGlobal(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void GlobalMax(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void GlobalMaxGlobal(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void GlobalMin(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void GlobalMinGlobal(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void GlobalOrGlobal(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void GlobalSetGlobal(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void GlobalShL(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void GlobalShLGlobal(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void GlobalShout(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void GlobalShR(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void GlobalShRGlobal(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void GlobalSubGlobal(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void GlobalXor(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void GlobalXorGlobal(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void Help(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void Hide(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void HideAreaOnMap(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void HideCreature(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void HideGUI(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void IncInternal(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void IncMoraleAI(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void IncrementChapter(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void IncrementExtraProficiency(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void IncrementGlobal(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void IncrementGlobalOnce(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void IncrementKillStat(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void IncrementProficiency(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void Interact(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void JoinParty(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void JumpToObject(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void JumpToPoint(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void JumpToPointInstant(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void JumpToSavedLocation(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void Kill(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void KillFloatMessage(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void Leader(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void LeaveArea(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void LeaveAreaLUA(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void LeaveAreaLUAEntry(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void LeaveAreaLUAPanic(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void LeaveAreaLUAPanicEntry(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void LeaveParty(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void Lock(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void LockScroll(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void MakeGlobal(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void MakeUnselectable(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void MarkObject(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void MarkSpellAndObject(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void MatchHP(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void MoraleDec(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void MoraleInc(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void MoraleSet(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void MoveBetweenAreas(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void MoveBetweenAreasEffect(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void MoveCursorPoint(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void MoveGlobal(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void MoveGlobalObject(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void MoveGlobalObjectOffScreen(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void MoveGlobalsTo(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void MoveInventory(Scriptable *Sender, Action* parameters);
	GEM_EXPORT void MoveToCenterOfScreen(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void MoveToExpansion(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void MoveToObject(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void MoveToObjectFollow(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void MoveToObjectNoInterrupt(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void MoveToObjectUntilSee(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void MoveToOffset(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void MoveToPoint(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void MoveToPointNoInterrupt(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void MoveToPointNoRecticle(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void MoveToSavedLocation(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void MoveViewPoint(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void MoveViewObject(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void NIDSpecial1(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void NIDSpecial2(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void NoAction(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void NoActionAtAll(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void OpenDoor(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void Panic(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void PauseGame(Scriptable *Sender, Action* parameters);
	GEM_EXPORT void PermanentStatChange(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void PickLock(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void PickPockets(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void PickUpItem(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void PlayBardSong(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void PlayDead(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void PlayDeadInterruptable(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void PlayerDialogue(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void PlaySequence(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void PlaySequenceGlobal(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void PlaySequenceTimed(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void PlaySound(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void PlaySoundNotRanged(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void PlaySoundPoint(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void Plunder(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void Polymorph(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void PolymorphCopy(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void PolymorphCopyBase(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void ProtectObject(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void ProtectPoint(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void QuitGame(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void RandomFly(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void RandomRun(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void RandomTurn(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void RandomWalk(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void RandomWalkContinuous(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void RealSetGlobalTimer(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void ReallyForceSpell(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void ReallyForceSpellDead(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void ReallyForceSpellPoint(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void Recoil(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void RegainPaladinHood(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void RegainRangerHood(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void RemoveAreaFlag(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void RemoveAreaType(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void RemoveJournalEntry(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void RemoveMapnote(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void RemovePaladinHood(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void RemoveRangerHood(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void RemoveSpell(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void RemoveTraps(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void ReputationInc(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void ReputationSet(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void RestorePartyLocation(Scriptable *Sender, Action* parameters);
	GEM_EXPORT void Rest(Scriptable *Sender, Action* parameters);
	GEM_EXPORT void RestNoSpells(Scriptable *Sender, Action* parameters);
	GEM_EXPORT void RestParty(Scriptable *Sender, Action* parameters);
	GEM_EXPORT void RestUntilHealed(Scriptable *Sender, Action* parameters);
	GEM_EXPORT void ReturnToSavedLocation(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void ReturnToSavedLocationDelete(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void RevealAreaOnMap(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void RunAwayFrom(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void RunAwayFromNoInterrupt(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void RunAwayFromNoLeaveArea(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void RunFollow(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void RunningAttack(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void RunningAttackNoSound(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void RunToObject(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void RunToPoint(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void RunToPointNoRecticle(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void RunToSavedLocation(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SaveGame(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SaveLocation(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SaveObjectLocation(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void ScreenShake(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SelectWeaponAbility(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SendTrigger(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetAnimState(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetApparentName(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetAreaFlags(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetAreaRestFlag(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetArmourLevel(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetBeenInPartyFlags(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetBestWeapon(Scriptable *Sender, Action *parameters);
	GEM_EXPORT void SetCursorState(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetCreatureAreaFlag(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetCriticalPathObject(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetDialogue(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetDialogueRange(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetDoorFlag(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetDoorLocked(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetEncounterProbability(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetExtendedNight(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetFaction(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetGabber(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetGlobal(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetGlobalRandom(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetGlobalTimer(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetGlobalTimerOnce(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetGlobalTimerRandom(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetGlobalTint(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetHP(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetHPPercent(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetInternal(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetInterrupt(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetLeavePartyDialogFile(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetMarkedSpell(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetMasterArea(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetMazeEasier(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetMazeHarder(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetMoraleAI(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetMusic(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetNamelessClass(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetNamelessDeath(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetNamelessDisguise(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetNoOneOnTrigger(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetNumTimesTalkedTo(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetPlayerSound(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetQuestDone(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetRegularName(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetRestEncounterChance(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetRestEncounterProbabilityDay(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetRestEncounterProbabilityNight(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetSavedLocation(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetSavedLocationPoint(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetScriptName(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetSelection(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetStartPos(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetTeam(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetTeamBit(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetTextColor(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetToken(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetToken2DA(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetTokenGlobal(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetTokenObject(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetTrackString(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetupWish(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetupWishObject(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetVisualRange(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SG(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void Shout(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SmallWait(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SmallWaitRandom(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SoundActivate(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SpawnPtActivate(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SpawnPtDeactivate(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SpawnPtSpawn(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void Spell(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SpellCastEffect(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SpellHitEffectPoint(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SpellHitEffectSprite(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SpellNoDec(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SpellPoint(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SpellPointNoDec(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void StartCombatCounter(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void StartCutScene(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void StartCutSceneMode(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void StartDialogue(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void StartDialogueInterrupt(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void StartDialogueNoSet(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void StartDialogueNoSetInterrupt(Scriptable* Sender,
		Action* parameters);
	GEM_EXPORT void StartDialogueOverride(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void StartDialogueOverrideInterrupt(Scriptable* Sender,
		Action* parameters);
	GEM_EXPORT void StartMovie(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void StartMusic(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void StartRainNow(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void StartRandomTimer(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void StartSong(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void StartStore(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void StartTimer(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void StateOverrideFlag(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void StateOverrideTime(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void StaticPalette(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void StaticStart(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void StaticStop(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void StopMoving(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void StorePartyLocation(Scriptable *Sender, Action* parameters);
	GEM_EXPORT void Swing(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SwingOnce(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void TakeItemList(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void TakeItemListParty(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void TakeItemListPartyNum(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void TakeItemReplace(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void TakePartyGold(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void TakePartyItem(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void TakePartyItemAll(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void TakePartyItemNum(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void TakePartyItemRange(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void TeleportParty(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void TextScreen(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void ToggleDoor(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void TimedMoveToPoint(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void TransformItem(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void TransformItemAll(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void TransformPartyItem(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void TransformPartyItemAll(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void TriggerActivation(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void Turn(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void TurnAMT(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void UndoExplore(Scriptable *Sender, Action *parameters);
	GEM_EXPORT void UnhideGUI(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void Unlock(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void UnlockScroll(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void UseContainer(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void UseDoor(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void UseItem(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void UseItemPoint(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void VerbalConstant(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void VerbalConstantHead(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void Wait(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void WaitAnimation(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void WaitRandom(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void Weather(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void XEquipItem(Scriptable *Sender, Action *parameters);
//Objects
	GEM_EXPORT Targets *BestAC(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *EighthNearest(Scriptable *Sender, Targets *parameter, int ga_flagss);
	GEM_EXPORT Targets *EighthNearestDoor(Scriptable *Sender, Targets *parameter, int ga_flagss);
	GEM_EXPORT Targets *EighthNearestEnemyOf(Scriptable *Sender, Targets *parameter, int ga_flagss);
	GEM_EXPORT Targets *EighthNearestEnemyOfType(Scriptable *Sender, Targets *parameter, int ga_flagss);
	GEM_EXPORT Targets *EighthNearestMyGroupOfType(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *Farthest(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *FarthestEnemyOf(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *FifthNearest(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *FifthNearestDoor(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *FifthNearestEnemyOf(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *FifthNearestEnemyOfType(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *FifthNearestMyGroupOfType(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *FourthNearest(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *FourthNearestDoor(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *FourthNearestEnemyOf(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *FourthNearestEnemyOfType(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *FourthNearestMyGroupOfType(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *Gabber(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *GroupOf(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *LastAttackerOf(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *LastCommandedBy(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *LastHeardBy(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *LastHelp(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *LastHitter(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *LastMarkedObject(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *LastSeenBy(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *LastSummonerOf(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *LastTalkedToBy(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *LastTargetedBy(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *LastTrigger(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *LeaderOf(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *LeastDamagedOf(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *MostDamagedOf(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *Myself(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *MyTarget(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *Nearest(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *NearestDoor(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *NearestEnemyOf(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *NearestEnemyOfType(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *NearestEnemySummoned(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *NearestMyGroupOfType(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *NearestPC(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *NinthNearest(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *NinthNearestDoor(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *NinthNearestEnemyOf(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *NinthNearestEnemyOfType(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *NinthNearestMyGroupOfType(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *Nothing(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *Player1(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *Player1Fill(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *Player2(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *Player2Fill(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *Player3(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *Player3Fill(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *Player4(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *Player4Fill(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *Player5(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *Player5Fill(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *Player6(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *Player6Fill(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *Protagonist(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *ProtectedBy(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *ProtectorOf(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *SecondNearest(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *SecondNearestDoor(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *SecondNearestEnemyOf(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *SecondNearestEnemyOfType(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *SecondNearestMyGroupOfType(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *SelectedCharacter(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *SeventhNearest(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *SeventhNearestDoor(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *SeventhNearestEnemyOf(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *SeventhNearestEnemyOfType(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *SeventhNearestMyGroupOfType(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *SixthNearest(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *SixthNearestDoor(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *SixthNearestEnemyOf(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *SixthNearestEnemyOfType(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *SixthNearestMyGroupOfType(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *StrongestOf(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *StrongestOfMale(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *TenthNearest(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *TenthNearestDoor(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *TenthNearestEnemyOf(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *TenthNearestEnemyOfType(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *TenthNearestMyGroupOfType(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *ThirdNearest(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *ThirdNearestDoor(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *ThirdNearestEnemyOf(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *ThirdNearestEnemyOfType(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *ThirdNearestMyGroupOfType(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *WeakestOf(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *WorstAC(Scriptable *Sender, Targets *parameters, int ga_flags);

// GemRB extensions/actions
	GEM_EXPORT void RunAwayFromPoint(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void UnMakeGlobal(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void UnloadArea(Scriptable* Sender, Action* parameters);

// GemRB extensions/objects
	GEM_EXPORT Targets *Player7(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *Player7Fill(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *Player8(Scriptable *Sender, Targets *parameters, int ga_flags);
	GEM_EXPORT Targets *Player8Fill(Scriptable *Sender, Targets *parameters, int ga_flags);
}

#endif
