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

#include "Holder.h"
#include "Owner.h"
#include "Opcode.h"
#include "SymbolMgr.h"
#include "Variables.h"
#include "System/DataStream.h"

#include <cstdio>
#include <vector>

class Action;
class Actor;
class GameScript;
class Scriptable;

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

	bool isNull();
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
	Owner<Object> objectParameter;
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
	bool Evaluate(Scriptable* Sender);
public:
	std::vector<Owner<Trigger> > triggers;
};

class GEM_EXPORT Action : public Held<Action> {
public:
	static const TypeID ID;
public:
	Action()
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
	}
public:
	unsigned short actionID;
	Owner<Object> objects[3];
	int int0Parameter;
	Point pointParameter;
	int int1Parameter;
	int int2Parameter;
	char string0Parameter[65];
	char string1Parameter[65];
public:
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
	}
};

typedef int (* TriggerFunction)(Scriptable*, Trigger*);
typedef void (* ActionFunction)(Scriptable*, Action*);
typedef Targets* (* ObjectFunction)(Scriptable *, Targets*, int ga_flags);
typedef int (* IDSFunction)(Actor *, int parameter);

typedef OpcodeRegistry<TriggerFunction>::Description TriggerDesc;
extern GEM_EXPORT OpcodeRegistry<TriggerFunction> TriggerRegistry;

typedef OpcodeRegistry<ActionFunction>::Description ActionDesc;
extern GEM_EXPORT OpcodeRegistry<ActionFunction> ActionRegistry;

typedef OpcodeRegistry<ObjectFunction>::Description ObjectDesc;
extern GEM_EXPORT OpcodeRegistry<ObjectFunction> ObjectRegistry;

#define TF_NONE 	0
#define TF_CONDITION    1 //this isn't a trigger, just a condition (0x4000)
#define TF_MERGESTRINGS 8 //same value as actions' mergestring

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
	GEM_EXPORT int False(Scriptable* Sender, Trigger* parameters);
	GEM_EXPORT void NoActionAtAll(Scriptable* Sender, Action* parameters);
	GEM_EXPORT void SetLeavePartyDialogFile(Scriptable* Sender, Action* parameters);
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
}

#endif
