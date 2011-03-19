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

#include "GameScript/IEScript.h"

#include "GameScript/GSUtils.h"
#include "GameScript/Matching.h"

IEScript::IEScript()
{
	lastAction = (unsigned int) ~0;
	script = NULL;
}

IEScript::~IEScript(void)
{
	delete script;
}

bool IEScript::Open(DataStream* stream)
{
	if (!stream) {
		return false;
	}
	if (script)
		delete script;

	char line[10];
	stream->ReadLine( line, 10 );
	if (strncmp( line, "SC", 2 ) != 0) {
		printMessage( "GameScript","Not a Compiled Script file\n", YELLOW );
		delete( stream );
		return NULL;
	}
	script = new Script( );

	while (true) {
		ResponseBlock* rB = ReadResponseBlock( stream );
		if (!rB)
			break;
		script->responseBlocks.push_back( rB );
		stream->ReadLine( line, 10 );
	}
	delete( stream );
	return true;
}

static int ParseInt(const char*& src)
{
	char number[33];

	char* tmp = number;
	while (isdigit(*src) || *src=='-') {
		*tmp = *src;
		tmp++;
		src++;
	}
	*tmp = 0;
	if (*src)
		src++;
	return atoi( number );
}

static void ParseString(const char*& src, char* tmp)
{
	while (*src != '"' && *src) {
		*tmp = *src;
		tmp++;
		src++;
	}
	*tmp = 0;
	if (*src)
		src++;
}

static Object* DecodeObject(const char* line)
{
	int i;
	const char *origline = line; // for debug below

	Object* oB = new Object();
	for (i = 0; i < ObjectFieldsCount; i++) {
		oB->objectFields[i] = ParseInt( line );
	}
	for (i = 0; i < MaxObjectNesting; i++) {
		oB->objectFilters[i] = ParseInt( line );
	}
	//iwd tolerates the missing rectangle, so we do so too
	if (HasAdditionalRect && (*line=='[') ) {
		line++; //Skip [
		for (i = 0; i < 4; i++) {
			oB->objectRect[i] = ParseInt( line );
		}
		if (*line == ' ')
			line++; //Skip ] (not really... it skips a ' ' since the ] was skipped by the ParseInt function
	}
	if (*line == '"')
		line++; //Skip "
	ParseString( line, oB->objectName );
	if (*line == '"')
		line++; //Skip " (the same as above)
	//this seems to be needed too
	if (ExtraParametersCount && *line) {
		line++;
	}
	for (i = 0; i < ExtraParametersCount; i++) {
		oB->objectFields[i + ObjectFieldsCount] = ParseInt( line );
	}
	if (*line != 'O' || *(line + 1) != 'B') {
		printMessage( "GameScript","Got confused parsing object line: ", YELLOW );
		printf("%s\n", origline);
	}
	//let the object realize it has no future (in case of null objects)
	if (oB->isNull()) {
		delete oB;
		return NULL;
	}
	return oB;
}

static Trigger* ReadTrigger(DataStream* stream)
{
	char* line = ( char* ) malloc( 1024 );
	stream->ReadLine( line, 1024 );
	if (strncmp( line, "TR", 2 ) != 0) {
		free( line );
		return NULL;
	}
	stream->ReadLine( line, 1024 );
	Trigger* tR = new Trigger();
	//this exists only in PST?
	if (HasTriggerPoint) {
		sscanf( line, "%hu %d %d %d %d [%hd,%hd] \"%[^\"]\" \"%[^\"]\" OB",
			&tR->triggerID, &tR->int0Parameter, &tR->flags,
			&tR->int1Parameter, &tR->int2Parameter, &tR->pointParameter.x,
			&tR->pointParameter.y, tR->string0Parameter, tR->string1Parameter );
	} else {
		sscanf( line, "%hu %d %d %d %d \"%[^\"]\" \"%[^\"]\" OB",
			&tR->triggerID, &tR->int0Parameter, &tR->flags,
			&tR->int1Parameter, &tR->int2Parameter, tR->string0Parameter,
			tR->string1Parameter );
	}
	strlwr(tR->string0Parameter);
	strlwr(tR->string1Parameter);
	tR->triggerID &= 0x3fff;
	stream->ReadLine( line, 1024 );
	tR->objectParameter = DecodeObject( line );
	stream->ReadLine( line, 1024 );
	free( line );
	return tR;
}

static Condition* ReadCondition(DataStream* stream)
{
	char line[10];

	stream->ReadLine( line, 10 );
	if (strncmp( line, "CO", 2 ) != 0) {
		return NULL;
	}
	Condition* cO = new Condition();
	while (true) {
		Trigger* tR = ReadTrigger( stream );
		if (!tR)
			break;
		cO->triggers.push_back( tR );
	}
	return cO;
}

/*
 * if you pass non-NULL parameters, continuing is set to whether we Continue()ed
 * (should start false and be passed to next script's Update),
 * and done is set to whether we processed a block without Continue()
 */
bool IEScript::Update(bool *continuing, bool *done)
{
	if (!MySelf)
		return false;

	if (!script)
		return false;

	//ieDword thisTime = core->GetGame()->Ticks;
	//if (( thisTime - lastRunTime ) < scriptRunDelay) {
	//	return false;
	//}

	//lastRunTime = thisTime;

	if(!(MySelf->GetInternalFlag()&IF_ACTIVE) ) {
		return true;
	}

	bool continueExecution = false;
	if (continuing) continueExecution = *continuing;

	RandomNumValue=rand();
	for (size_t a = 0; a < script->responseBlocks.size(); a++) {
		ResponseBlock& rB = *script->responseBlocks[a];
		if (rB.condition->Evaluate(MySelf)) {
			//if this isn't a continue-d block, we have to clear the queue
			//we cannot clear the queue and cannot execute the new block
			//if we already have stuff on the queue!
			if (!continueExecution) {
				if (MySelf->GetCurrentAction() || MySelf->GetNextAction()) {
					if (MySelf->GetInternalFlag()&IF_NOINT) {
						// we presumably don't want any further execution?
						if (done) *done = true;
						return true;
					}

					if (lastAction==a) {
						// we presumably don't want any further execution?
						// this one is a bit more complicated, due to possible
						// interactions with Continue() (lastAction here is always
						// the first block encountered), needs more testing
						//if (done) *done = true;
						return true;
					}

					//movetoobjectfollow would break if this isn't called
					//(what is broken if it is here?)
					MySelf->ClearActions();
					//IE even clears the path, shall we?
					//yes we must :)
					if (MySelf->Type == ST_ACTOR) {
						((Movable *)MySelf)->ClearPath();
					}
				}
				lastAction=a;
			}
			continueExecution = (rB.responseSet->Execute(this, MySelf) != 0);
			if (continuing) *continuing = continueExecution;
			//clear triggers after response executed
			//MySelf->ClearTriggers();
			if (!continueExecution) {
				if (done) *done = true;
				break;
			}
			//continueExecution = false;
		}
	}
	return true;
}

//IE simply takes the first action's object for cutscene object
//then adds these actions to its queue:
// SetInterrupt(false), <actions>, SetInterrupt(true)

void IEScript::EvaluateAllBlocks()
{
	if (!MySelf || !(MySelf->GetInternalFlag()&IF_ACTIVE) ) {
		return;
	}

	if (!script) {
		return;
	}

#ifdef GEMRB_CUTSCENES
	// this is the (unused) more logical way of executing a cutscene, which
	// evaluates conditions and doesn't just use the first response
	for (size_t a = 0; a < script->responseBlocks.size(); a++) {
		ResponseBlock& rB = *script->responseBlocks[a];
		if (rB.Condition->Evaluate(MySelf)) {
			// TODO: this no longer works since the cutscene changes
			rB.Execute(this, MySelf);
		}
	}
#else
	// this is the original IE behaviour:
	// cutscenes don't evaluate conditions - they just choose the
	// first response, take the object from the first action,
	// and then add the actions to that object's queue.
	for (size_t a = 0; a < script->responseBlocks.size(); a++) {
		ResponseBlock& rB = *script->responseBlocks[a];
		ResponseSet& rS = *rB.responseSet;
		if (rS.responses.size()) {
			Response& response = *rS.responses[0];
			if (response.actions.size()) {
				Holder<Action> action = response.actions[0];
				Scriptable *target = GetActorFromObject(MySelf, action->objects[1]);
				if (target) {
					// TODO: sometimes SetInterrupt(false) and SetInterrupt(true) are added before/after?
					rS.responses[0]->Execute(this, target);
					// TODO: this will break blocking instants, if there are any
					target->ReleaseCurrentAction();
				} else if (InDebug&ID_CUTSCENE) {
					printMessage("GameScript","Failed to find CutSceneID target!\n",YELLOW);
					if (action->objects[1]) {
						action->objects[1]->Dump();
					}
				}
			}
		}
	}
#endif
}

ResponseBlock* IEScript::ReadResponseBlock(DataStream* stream)
{
	char line[10];

	stream->ReadLine( line, 10 );
	if (strncmp( line, "CR", 2 ) != 0) {
		return NULL;
	}
	ResponseBlock* rB = new ResponseBlock();
	rB->condition = ReadCondition( stream );
	rB->responseSet = ReadResponseSet( stream );
	return rB;
}

ResponseSet* IEScript::ReadResponseSet(DataStream* stream)
{
	char line[10];

	stream->ReadLine( line, 10 );
	if (strncmp( line, "RS", 2 ) != 0) {
		return NULL;
	}
	ResponseSet* rS = new ResponseSet();
	while (true) {
		Response* rE = ReadResponse( stream );
		if (!rE)
			break;
		rS->responses.push_back( rE );
	}
	return rS;
}

//this is the border of the GameScript object (all subsequent functions are library functions)
//we can't make this a library function, because scriptlevel is set here
Response* IEScript::ReadResponse(DataStream* stream)
{
	char* line = ( char* ) malloc( 1024 );
	stream->ReadLine( line, 1024 );
	if (strncmp( line, "RE", 2 ) != 0) {
		free( line );
		return NULL;
	}
	Response* rE = new Response();
	rE->weight = 0;
	stream->ReadLine( line, 1024 );
	char *poi;
	rE->weight = (unsigned char)strtoul(line,&poi,10);
	if (strncmp(poi,"AC",2)==0)
	while (true) {
		//not autofreed, because it is referenced by the Script
		Action* aC = new Action();
		stream->ReadLine( line, 1024 );
		aC->actionID = (unsigned short)strtoul(line, NULL,10);
		for (int i = 0; i < 3; i++) {
			stream->ReadLine( line, 1024 );
			Object* oB = DecodeObject( line );
			aC->objects[i] = oB;
			if (i != 2)
				stream->ReadLine( line, 1024 );
		}
		stream->ReadLine( line, 1024 );
		sscanf( line, "%d %hd %hd %d %d\"%[^\"]\" \"%[^\"]\" AC",
			&aC->int0Parameter, &aC->pointParameter.x, &aC->pointParameter.y,
			&aC->int1Parameter, &aC->int2Parameter, aC->string0Parameter,
			aC->string1Parameter );
		strlwr(aC->string0Parameter);
		strlwr(aC->string1Parameter);
		if (aC->actionID>=MAX_ACTIONS) {
			aC->actionID=0;
			printMessage("GameScript","Invalid script action ID!",LIGHT_RED);
		}
		rE->actions.push_back( aC );
		stream->ReadLine( line, 1024 );
		if (strncmp( line, "RE", 2 ) == 0)
			break;
	}
	free( line );
	return rE;
}


int ResponseSet::Execute(IEScript* Script, Scriptable* Sender)
{
	size_t i;

	switch(responses.size()) {
		case 0:
			return 0;
		case 1:
			return responses[0]->Execute(Script, Sender);
	}
	/*default*/
	int randWeight;
	int maxWeight = 0;

	for (i = 0; i < responses.size(); i++) {
		maxWeight += responses[i]->weight;
	}
	if (maxWeight) {
		randWeight = rand() % maxWeight;
	}
	else {
		randWeight = 0;
	}

	for (i = 0; i < responses.size(); i++) {
		Response& rE = *responses[i];
		if (rE.weight > randWeight) {
			return rE.Execute(Script, Sender);
			/* this break is only symbolic */
			break;
		}
		randWeight-=rE.weight;
	}
	return 0;
}

//continue is effective only as the last action in the block
int Response::Execute(IEScript* Script, Scriptable* Sender)
{
	int ret = 0; // continue or not
	for (size_t i = 0; i < actions.size(); i++) {
		Holder<Action> aC = actions[i];
		if (actionflags[aC->actionID] & AF_SCRIPTLEVEL) {
			aC = ParamCopy(aC);
			aC->int0Parameter = Script->scriptlevel;
		}
		switch (actionflags[aC->actionID] & AF_MASK) {
			case AF_IMMEDIATE:
				GameScript::ExecuteAction( Sender, aC );
				ret = 0;
				break;
			case AF_NONE:
				Sender->AddAction( aC );
				ret = 0;
				break;
			case AF_CONTINUE:
			case AF_MASK:
				ret = 1;
				break;
		}
	}
	return ret;
}

#define STATIC_LINK
#include "plugindef.h"

const TypeID GameScript::ID = { "Script" };

GEMRB_PLUGIN(_, _)
PLUGIN_IE_RESOURCE(IEScript, ".bs", (ieWord)IE_BCS_CLASS_ID)
PLUGIN_IE_RESOURCE(IEScript, ".bcs", (ieWord)IE_BCS_CLASS_ID)
END_PLUGIN()
