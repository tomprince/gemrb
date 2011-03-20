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

#include "GameScript/GameScript.h"

#include "GameScript/GSUtils.h"
#include "GameScript/Matching.h"

#include "win32def.h"

#include "Game.h"
#include "GameData.h"
#include "Interface.h"
#include "Opcode.h"
#include "PluginMgr.h"

//debug flags
// 1 - cache
// 2 - cutscene ID
// 4 - globals
// 8 - action execution
//16 - trigger evaluation

OpcodeRegistry<TriggerFunction> TriggerRegistry;
OpcodeRegistry<ActionFunction> ActionRegistry;
OpcodeRegistry<ObjectFunction> ObjectRegistry;

static const IDSLink idsnames[] = {
	{"align", GS::ID_Alignment},
	{"alignmen", GS::ID_Alignment},
	{"alignmnt", GS::ID_Alignment},
	{"class20", GS::ID_AVClass},
	{"class", GS::ID_Class},
	{"classmsk", GS::ID_ClassMask},
	{"ea", GS::ID_Allegiance},
	{"faction", GS::ID_Faction},
	{"gender", GS::ID_Gender},
	{"general", GS::ID_General},
	{"race", GS::ID_Race},
	{"specific", GS::ID_Specific},
	{"subrace", GS::ID_Subrace},
	{"team", GS::ID_Team},
	{ NULL,NULL}
};

static const TriggerDesc* FindTrigger(const char* triggername)
{
	if (!triggername) {
		return NULL;
	}
	int len = strlench( triggername, '(' );
	char* name = strndup(triggername, len);
	TriggerDesc* desc = TriggerRegistry.Find(name);
	free(name);
	return desc;
}

static const ActionDesc* FindAction(const char* actionname)
{
	if (!actionname) {
		return NULL;
	}
	int len = strlench( actionname, '(' );
	char* name = strndup(actionname, len);
	ActionDesc* desc = ActionRegistry.Find(name);
	free(name);
	return desc;
}

static const ObjectDesc* FindObject(const char* objectname)
{
	if (!objectname) {
		return NULL;
	}
	int len = strlench( objectname, '(' );
	char* name = strndup(objectname, len);
	ObjectDesc* desc = ObjectRegistry.Find(name);
	free(name);
	return desc;
}

static const IDSLink* FindIdentifier(const char* idsname)
{
	if (!idsname) {
		return NULL;
	}
	int len = (int)strlen( idsname );
	for (int i = 0; idsnames[i].Name; i++) {
		if (!strnicmp( idsnames[i].Name, idsname, len )) {
			return idsnames + i;
		}
	}
	
	printMessage( "GameScript"," ", YELLOW );
	printf( "Couldn't assign ids target: %.*s\n", len, idsname );
	return NULL;
}

void SetScriptDebugMode(int arg)
{
	InDebug=arg;
}



/********************** Targets **********************************/

int Targets::Count() const
{
	return (int)objects.size();
}

targettype *Targets::RemoveTargetAt(targetlist::iterator &m)
{
	m=objects.erase(m);
	if (m!=objects.end() ) {
		return &(*m);
	}
	return NULL;
}

const targettype *Targets::GetLastTarget(int Type)
{
	targetlist::const_iterator m = objects.end();
	while (m--!=objects.begin() ) {
		if ( (Type==-1) || ((*m).actor->Type==Type) ) {
			return &(*(m));
		}
	}
	return NULL;
}

const targettype *Targets::GetFirstTarget(targetlist::iterator &m, int Type)
{
	m=objects.begin();
	while (m!=objects.end() ) {
		if ( (Type!=-1) && ( (*m).actor->Type!=Type)) {
			m++;
			continue;
		}
		return &(*m);
	}
	return NULL;
}

const targettype *Targets::GetNextTarget(targetlist::iterator &m, int Type)
{
	m++;
	while (m!=objects.end() ) {
		if ( (Type!=-1) && ( (*m).actor->Type!=Type)) {
			m++;
			continue;
		}
		return &(*m);
	}
	return NULL;
}

Scriptable *Targets::GetTarget(unsigned int index, int Type)
{
	targetlist::iterator m = objects.begin();
	while(m!=objects.end() ) {
		if ( (Type==-1) || ((*m).actor->Type==Type)) {
			if (!index) {
				return (*m).actor;
			}
			index--;
		}
		m++;
	}
	return NULL;
}

//this stuff should be refined, dead actors are sometimes targetable by script?
void Targets::AddTarget(Scriptable* target, unsigned int distance, int ga_flags)
{
	if (!target) {
		return;
	}

	switch (target->Type) {
	case ST_ACTOR:
		//i don't know if unselectable actors are targetable by script
		//if yes, then remove GA_SELECT
		if (ga_flags) {
			if (!((Actor *) target)->ValidTarget(ga_flags) ) {
				return;
			}
		}
		break;
	case ST_GLOBAL:
		// this doesn't seem a good idea to allow
		return;
	default:
		break;
	}
	targettype Target = {target, distance};
	targetlist::iterator m;
	for (m = objects.begin(); m != objects.end(); ++m) {
		if ( (*m).distance>distance) {
			objects.insert( m, Target);
			return;
		}
	}
	objects.push_back( Target );
}

void Targets::Clear()
{
	objects.clear();
}

/** releasing global memory */
static void CleanupIEScript()
{
	triggersTable.release();
	actionsTable.release();
	objectsTable.release();
	overrideActionsTable.release();
	if (ObjectIDSTableNames)
		free(ObjectIDSTableNames);
	ObjectIDSTableNames = NULL;
}

void printFunction(Holder<SymbolMgr> table, int index)
{
	const char *str = table->GetStringIndex(index);
	int value = table->GetValueIndex(index);

	int len = strchr(str,'(')-str;
	if (len<0) {
		printf("%d %s\n", value, str);
	} else {
		printf("%d %.*s\n", value, len, str);
	}
}

void InitializeIEScript()
{
	std::list<int> missing_triggers;
	std::list<int> missing_actions;
	std::list<int> missing_objects;
	std::list<int>::iterator l;

	PluginMgr::Get()->RegisterCleanup(CleanupIEScript);

	NoCreate = core->HasFeature(GF_NO_NEW_VARIABLES);
	HasKaputz = core->HasFeature(GF_HAS_KAPUTZ);

	InitScriptTables();
	int tT = core->LoadSymbol( "trigger" );
	int aT = core->LoadSymbol( "action" );
	int oT = core->LoadSymbol( "object" );
	int gaT = core->LoadSymbol( "gemact" );
	AutoTable objNameTable("script");
	if (tT < 0 || aT < 0 || oT < 0 || !objNameTable) {
		printMessage( "GameScript","A critical scripting file is missing!\n",LIGHT_RED );
		abort();
	}
	triggersTable = core->GetSymbol( tT );
	actionsTable = core->GetSymbol( aT );
	objectsTable = core->GetSymbol( oT );
	overrideActionsTable = core->GetSymbol( gaT );
	if (!triggersTable || !actionsTable || !objectsTable || !objNameTable) {
		printMessage( "GameScript","A critical scripting file is damaged!\n",LIGHT_RED );
		abort();
	}

	int i;

	/* Loading Script Configuration Parameters */

	ObjectIDSCount = atoi( objNameTable->QueryField() );
	if (ObjectIDSCount<0 || ObjectIDSCount>MAX_OBJECT_FIELDS) {
		printMessage("GameScript","The IDS Count shouldn't be more than 10!\n",LIGHT_RED);
		abort();
	}

	ObjectIDSTableNames = (ieResRef *) malloc( sizeof(ieResRef) * ObjectIDSCount );
	for (i = 0; i < ObjectIDSCount; i++) {
		const char *idsname;
		idsname=objNameTable->QueryField( 0, i + 1 );
		const IDSLink *poi=FindIdentifier( idsname );
		if (poi==NULL) {
			idtargets[i]=NULL;
		}
		else {
			idtargets[i]=poi->Function;
		}
		strnlwrcpy(ObjectIDSTableNames[i], idsname, 8 );
	}
	MaxObjectNesting = atoi( objNameTable->QueryField( 1 ) );
	if (MaxObjectNesting<0 || MaxObjectNesting>MAX_NESTING) {
		printMessage("GameScript","The Object Nesting Count shouldn't be more than 5!\n", LIGHT_RED);
		abort();
	}
	HasAdditionalRect = ( atoi( objNameTable->QueryField( 2 ) ) != 0 );
	ExtraParametersCount = atoi( objNameTable->QueryField( 3 ) );
	HasTriggerPoint = ( atoi( objNameTable->QueryField( 4 ) ) != 0 );
	ObjectFieldsCount = ObjectIDSCount - ExtraParametersCount;

	/* Initializing the Script Engine */

	memset( triggers, 0, sizeof( triggers ) );
	memset( triggerflags, 0, sizeof( triggerflags ) );
	memset( actions, 0, sizeof( actions ) );
	memset( actionflags, 0, sizeof( actionflags ) );
	memset( objects, 0, sizeof( objects ) );

	int j;

	j = triggersTable->GetSize();
	while (j--) {
		i = triggersTable->GetValueIndex( j );
		const TriggerDesc* poi = FindTrigger(triggersTable->GetStringIndex( j ));
		//maybe we should watch for this bit?
		//bool triggerflag = i & 0x4000;
		i &= 0x3fff;
		if (i >= MAX_TRIGGERS) {
			printMessage("GameScript"," ", RED);
			printf("trigger %d (%s) is too high, ignoring\n", i, triggersTable->GetStringIndex( j ) );
			continue;
		}
		if (triggers[i]) {
			if (poi && triggers[i]!=poi->Function) {
				printMessage("GameScript"," ", YELLOW);
				printf("%s is in collision with ", triggersTable->GetStringIndex( j ) );
				printFunction(triggersTable,triggersTable->FindValue(triggersTable->GetValueIndex( j )));
				//printFunction(triggersTable->GetStringIndex(triggersTable->FindValue(triggersTable->GetValueIndex( j )) ));
			} else {
				if (InDebug&ID_TRIGGERS) {
					printMessage("GameScript"," ", WHITE);
					printf("%s is a synonym of ", triggersTable->GetStringIndex( j ) );
					printFunction(triggersTable,triggersTable->FindValue(triggersTable->GetValueIndex( j )));
					//printFunction(triggersTable->GetStringIndex(triggersTable->FindValue(triggersTable->GetValueIndex( j ) ) ) );
				}
			}
			continue; //we already found an alternative
		}
		if (poi == NULL) {
			triggers[i] = NULL;
			triggerflags[i] = 0;
			missing_triggers.push_back(j);
			continue;
		}
		triggers[i] = poi->Function;
		triggerflags[i] = poi->Flags;
	}

	for (l = missing_triggers.begin(); l!=missing_triggers.end();l++) {
		j = *l;
		// found later as a different name
		int ii = triggersTable->GetValueIndex( j ) & 0x3fff;
		if (ii >= MAX_TRIGGERS) {
			continue;
		}
		
		if (InDebug&ID_TRIGGERS) {
			TriggerFunction f = triggers[ii];
			if (f) {
				// This only shows the first instance now.
				TriggerDesc *desc = TriggerRegistry.Find(f);
				if (desc) {
					printMessage("GameScript"," ", WHITE);
					printf("%s is a synonym of %s\n", triggersTable->GetStringIndex( j ), desc->Name );
					break;
				}
				continue;
			}
		}
		printMessage("GameScript","Couldn't assign function to trigger: ", YELLOW);
		printFunction(triggersTable,j);
//->GetStringIndex(j) );
	}

	j = actionsTable->GetSize();
	while (j--) {
		i = actionsTable->GetValueIndex( j );
		if (i >= MAX_ACTIONS) {
			printMessage("GameScript"," ", RED);
			printf("action %d (%s) is too high, ignoring\n", i, actionsTable->GetStringIndex( j ) );
			continue;
		}
		const ActionDesc* poi = FindAction( actionsTable->GetStringIndex( j ));
		if (actions[i]) {
			if (poi && actions[i]!=poi->Function) {
				printMessage("GameScript"," ", YELLOW);
				printf("%s is in collision with ", actionsTable->GetStringIndex( j ) );
				printFunction(actionsTable, actionsTable->FindValue(actionsTable->GetValueIndex(j)));
//->GetStringIndex(actionsTable->FindValue(actionsTable->GetValueIndex( j )) ) );
			} else {
				if (InDebug&ID_ACTIONS) {
					printMessage("GameScript"," ", WHITE);
					printf("%s is a synonym of ", actionsTable->GetStringIndex( j ) );
					printFunction(actionsTable, actionsTable->FindValue(actionsTable->GetValueIndex( j )));
//actionsTable->GetStringIndex(actionsTable->FindValue(actionsTable->GetValueIndex( j )) ) );
				}
			}
			continue; //we already found an alternative
		}
		if (poi == NULL) {
			actions[i] = NULL;
			actionflags[i] = 0;
			missing_actions.push_back(j);
			continue;
		}
		actions[i] = poi->Function;
		actionflags[i] = poi->Flags;
	}

	if (overrideActionsTable) {
		/*
		 * we add/replace some actions from gemact.ids
		 * right now you can't print or generate these actions!
		 */
		j = overrideActionsTable->GetSize();
		while (j--) {
			i = overrideActionsTable->GetValueIndex( j );
			if (i >= MAX_ACTIONS) {
				printMessage("GameScript"," ", RED);
				printf("action %d (%s) is too high, ignoring\n", i, overrideActionsTable->GetStringIndex( j ) );
				continue;
			}
			const ActionDesc *poi = FindAction( overrideActionsTable->GetStringIndex( j ));
			if (!poi) {
				continue;
			}
			if (actions[i]) {
				printMessage("GameScript"," ", WHITE);
				printf("%s overrides existing action ", overrideActionsTable->GetStringIndex( j ) );
				printFunction( actionsTable, actionsTable->FindValue(overrideActionsTable->GetValueIndex( j )));
				//printFunction( actionsTable->GetStringIndex(actionsTable->FindValue(overrideActionsTable->GetValueIndex( j )) ) );
			}
			actions[i] = poi->Function;
			actionflags[i] = poi->Flags;
		}
	}

	for (l = missing_actions.begin(); l!=missing_actions.end();l++) {
		j = *l;
		// found later as a different name
		int ii = actionsTable->GetValueIndex( j );
		if (ii>=MAX_ACTIONS) {
			continue;
		}

		if (InDebug&ID_ACTIONS) {
			ActionFunction f = actions[ii];
			if (f) {
				// This only shows the first instance now.
				ActionDesc *desc = ActionRegistry.Find(f);
				if (desc) {
					printMessage("GameScript"," ", WHITE);
					printf("%s is a synonym of %s\n", actionsTable->GetStringIndex( j ), desc->Name );
					break;
				}
				continue;
			}
		}
		printMessage("GameScript","Couldn't assign function to action: ", YELLOW);
		printFunction(actionsTable,j);
		//printFunction(actionsTable->GetStringIndex(j) );
	}

	j = objectsTable->GetSize();
	while (j--) {
		i = objectsTable->GetValueIndex( j );
		if (i >= MAX_OBJECTS) {
			printMessage("GameScript"," ", RED);
			printf("object %d (%s) is too high, ignoring\n", i, objectsTable->GetStringIndex( j ) );
			continue;
		}
		const ObjectDesc* poi = FindObject( objectsTable->GetStringIndex( j ));
		if (objects[i]) {
			if (poi && objects[i]!=poi->Function) {
				printMessage("GameScript"," ", YELLOW);
				printf("%s is in collision with ", objectsTable->GetStringIndex( j ) );
				printFunction(objectsTable,objectsTable->FindValue(objectsTable->GetValueIndex( j )));
				//printFunction(objectsTable->GetStringIndex(objectsTable->FindValue(objectsTable->GetValueIndex( j )) ) );
			} else {
				printMessage("GameScript"," ", WHITE);
				printf("%s is a synonym of ", objectsTable->GetStringIndex( j ) );
				printFunction(objectsTable, objectsTable->FindValue(objectsTable->GetValueIndex( j )));
				//printFunction(objectsTable->GetStringIndex(objectsTable->FindValue(objectsTable->GetValueIndex( j )) ) );
			}
			continue;
		}
		if (poi == NULL) {
			objects[i] = NULL;
			missing_objects.push_back(j);
		} else {
			objects[i] = poi->Function;
		}
	}

	for (l = missing_objects.begin(); l!=missing_objects.end();l++) {
		j = *l;
		// found later as a different name
		int ii = objectsTable->GetValueIndex( j );
		if (ii>=MAX_ACTIONS) {
			continue;
		}

		ObjectFunction f = objects[ii];
		if (f) {
			ObjectDesc* desc = ObjectRegistry.Find(f);
			if (desc) {
				printMessage("GameScript"," ", WHITE);
				printf("%s is a synonym of %s\n", objectsTable->GetStringIndex( j ), desc->Name );
				break;
			}
			continue;
		}
		printMessage("GameScript","Couldn't assign function to object: ", YELLOW);
		printFunction(objectsTable,j);
		//printFunction(objectsTable->GetStringIndex(j) );
	}

	int instantTableIndex = core->LoadSymbol("instant");
	if (instantTableIndex < 0) {
		printMessage("GameScript", "Couldn't find instant symbols!\n", LIGHT_RED);
		abort();
	}
	Holder<SymbolMgr> instantTable = core->GetSymbol(instantTableIndex);
	if (!instantTable) {
		printMessage("GameScript", "Couldn't load instant symbols!\n", LIGHT_RED);
		abort();
	}
	j = instantTable->GetSize();
	while (j--) {
		i = instantTable->GetValueIndex( j );
		if (i >= MAX_ACTIONS) {
			printMessage("GameScript"," ", RED);
			printf("instant action %d (%s) is too high, ignoring\n", i, instantTable->GetStringIndex( j ) );
			continue;
		}
		if (!actions[i]) {
			printMessage("GameScript"," ", YELLOW);
			printf("instant action %d (%s) doesn't exist, ignoring\n", i, instantTable->GetStringIndex( j ) );
			continue;
		}
		actionflags[i] |= AF_INSTANT;
	}
}

/********************** GameScript *******************************/
GameScript::GameScript()
{
	MySelf = NULL;
	scriptlevel = 0;
	Name[0] = 0;
}

void GameScript::SetStuff(const ieResRef Name, Scriptable* MySelf, int ScriptLevel)
{
	this->MySelf = MySelf;
	strnlwrcpy(this->Name, Name, 8);
	this->scriptlevel = ScriptLevel;
}

void GameScript::ExecuteString(Scriptable* Sender, char* String)
{
	if (String[0] == 0) {
		return;
	}
	Holder<Action> act = GenerateAction( String );
	if (!act) {
		return;
	}
	Sender->AddActionInFront(act);
}

//This must return integer because Or(3) returns 3
int GameScript::EvaluateString(Scriptable* Sender, char* String)
{
	if (String[0] == 0) {
		return 0;
	}
	Trigger* tri = GenerateTrigger( String );
	if (tri) {
		int ret = tri->Evaluate(Sender);
		delete tri;
		return ret;
	}
	return 0;
}

bool Condition::Evaluate(Scriptable* Sender)
{
	int ORcount = 0;
	unsigned int result = 0;
	bool subresult = true;

	for (size_t i = 0; i < triggers.size(); i++) {
		Trigger* tR = triggers[i];
		//do not evaluate triggers in an Or() block if one of them
		//was already True()
		if (!ORcount || !subresult) {
			result = tR->Evaluate(Sender);
		}
		if (result > 1) {
			//we started an Or() block
			if (ORcount) {
				printMessage( "GameScript","Unfinished OR block encountered!\n",YELLOW );
			}
			ORcount = result;
			subresult = false;
			continue;
		}
		if (ORcount) {
			subresult |= ( result != 0 );
			if (--ORcount) {
				continue;
			}
			result = subresult;
		}
		if (!result) {
			return 0;
		}
	}
	if (ORcount) {
		printMessage( "GameScript","Unfinished OR block encountered!\n",YELLOW );
	}
	return 1;
}

/* this may return more than a boolean, in case of Or(x) */
int Trigger::Evaluate(Scriptable* Sender)
{
	if (!this) {
		printMessage( "GameScript","Trigger evaluation fails due to NULL trigger.\n",LIGHT_RED );
		return 0;
	}
	TriggerFunction func = triggers[triggerID];
	const char *tmpstr=triggersTable->GetValue(triggerID);
	if (!tmpstr) {
		tmpstr=triggersTable->GetValue(triggerID|0x4000);
	}
	if (!func) {
		triggers[triggerID] = GS::False;
		printMessage("GameScript"," ",YELLOW);
		printf("Unhandled trigger code: 0x%04x %s\n",
			triggerID, tmpstr );
		return 0;
	}
	if (InDebug&ID_TRIGGERS) {
		printMessage("GameScript"," ",YELLOW);
		printf( "Executing trigger code: 0x%04x %s\n",
				triggerID, tmpstr );
	}
	int ret = func( Sender, this );
	if (flags & NEGATE_TRIGGER) {
		return !ret;
	}
	return ret;
}

void PrintAction(int actionID)
{
	printf("Action: %d %s\n", actionID , actionsTable->GetValue(actionID) );
}

void GameScript::ExecuteAction(Scriptable* Sender, Holder<Action> aC)
{
	int actionID = aC->actionID;

	if (aC->objects[0]) {
		Scriptable *scr = GetActorFromObject(Sender, aC->objects[0]);

		Sender->ReleaseCurrentAction();

		if (scr) {
			if (InDebug&ID_ACTIONS) {
				printMessage("GameScript"," ",YELLOW);
				printf("Sender: %s-->override: %s\n",Sender->GetScriptName(), scr->GetScriptName() );
			}
			scr->ReleaseCurrentAction();
			scr->AddAction(ParamCopyNoOverride(aC));
			if (!(actionflags[actionID] & AF_INSTANT)) {
				assert(scr->GetNextAction());
				// TODO: below was written before i added instants, this might be unnecessary now

				// there are plenty of places where it's vital that ActionOverride is not interrupted and if
				// there are actions left on the queue after the release above, we can't instant-execute,
				// so this is my best guess for now..
				scr->CurrentActionInterruptable = false;
			}
		} else {
			printMessage("GameScript","Actionoverride failed for object: \n",LIGHT_RED);
			aC->objects[0]->Dump();
		}
		return;
	}
	if (InDebug&ID_ACTIONS) {
		printMessage("GameScript"," ",YELLOW);
		PrintAction(actionID);
		printf("Sender: %s\n",Sender->GetScriptName() );
	}
	ActionFunction func = actions[actionID];
	if (func) {
		//turning off interruptable flag
		//uninterruptable actions will set it back
		if (Sender->Type==ST_ACTOR) {
			Sender->Activate();
			if (actionflags[actionID]&AF_ALIVE) {
				if (Sender->GetInternalFlag()&IF_STOPATTACK) {
					printMessage("GameScript", "Aborted action due to death\n", YELLOW);
					Sender->ReleaseCurrentAction();
					return;
				}
			}
		}
		func( Sender, aC.get() );
	} else {
		actions[actionID] = GS::NoActionAtAll;
		printMessage("GameScript", "Unknown ", YELLOW);
		textcolor(YELLOW);
		PrintAction(actionID);
		Sender->ReleaseCurrentAction();
		textcolor(WHITE);
		return;
	}

	//don't bother with special flow control actions
	if (actionflags[actionID] & AF_IMMEDIATE) {
		return;
	}

	//Releasing nonblocking actions, blocking actions will release themselves
	if (!( actionflags[actionID] & AF_BLOCKING )) {
		Sender->ReleaseCurrentAction();
		//aC is invalid beyond this point, so we return!
		return;
	}
}

Trigger* GenerateTrigger(char* String)
{
	strlwr( String );
	if (InDebug&ID_TRIGGERS) {
		printMessage("GameScript"," ",YELLOW);
		printf("Compiling:%s\n",String);
	}
	int negate = 0;
	if (*String == '!') {
		String++;
		negate = 1;
	}
	int len = strlench(String,'(')+1; //including (
	int i = triggersTable->FindString(String, len);
	if (i<0) {
		printMessage("GameScript"," ",LIGHT_RED);
		printf("Invalid scripting trigger: %s\n", String);
		return NULL;
	}
	char *src = String+len;
	char *str = triggersTable->GetStringIndex( i )+len;
	Trigger *trigger = GenerateTriggerCore(src, str, i, negate);
	if (!trigger) {
		printMessage("GameScript"," ",LIGHT_RED);
		printf("Malformed scripting trigger: %s\n", String);
		return NULL;
	}
	return trigger;
}

Holder<Action> GenerateAction(char* String)
{
	strlwr( String );
	if (InDebug&ID_ACTIONS) {
		printMessage("GameScript"," ",YELLOW);
		printf("Compiling:%s\n",String);
	}
	int len = strlench(String,'(')+1; //including (
	char *src = String+len;
	int i = -1;
	char *str;
	unsigned short actionID;
	if (overrideActionsTable) {
		i = overrideActionsTable->FindString(String, len);
		if (i >= 0) {
			str = overrideActionsTable->GetStringIndex( i )+len;
			actionID = overrideActionsTable->GetValueIndex(i);
		}
	}
	if (i<0) {
		i = actionsTable->FindString(String, len);
		if (i < 0) {
			printMessage("GameScript"," ",LIGHT_RED);
			printf("Invalid scripting action: %s\n", String);
			return NULL;
		}
		str = actionsTable->GetStringIndex( i )+len;
		actionID = actionsTable->GetValueIndex(i);
	}
	Holder<Action> action = GenerateActionCore( src, str, actionID);
	if (!action) {
		printMessage("GameScript"," ",LIGHT_RED);
		printf("Malformed scripting action: %s\n", String);
		return NULL;
	}
	return action;
}

Holder<Action> GenerateActionDirect(char *String, Scriptable *object)
{
	Holder<Action> action = GenerateAction(String);
	Object *tmp = action->objects[1];
	if (tmp && tmp->objectFields[0]==-1) {
		tmp->objectFields[1] = object->GetGlobalID();
	}
	action->pointParameter.empty();
	return action;
}

/** Return true if object is null */
bool Object::isNull()
{
	if (objectName[0]!=0) {
		return false;
	}
	if (objectFilters[0]) {
		return false;
	}
	for (int i=0;i<ObjectFieldsCount;i++) {
		if (objectFields[i]) {
			return false;
		}
	}
	return true;
}
