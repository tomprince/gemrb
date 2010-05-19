#include "Script.h"

#include "DataStream.h"
#include "GameScript.h"
#include "GSUtils.h"
#include "ActorBlock.h"

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
	if (oB->ReadyToDie()) {
		oB = NULL;
	}
	return oB;
}


static Response* ReadResponse(DataStream* stream)
{
	char* line = ( char* ) malloc( 1024 );
	stream->ReadLine( line, 1024 );
	if (strncmp( line, "RE", 2 ) != 0) {
		free( line );
		return NULL;
	}
	Response* rE = new Response();
	rE->weight = 0;
	int count = stream->ReadLine( line, 1024 );
	char *poi;
	rE->weight = (unsigned char)strtoul(line,&poi,10);
	std::vector< Action*> aCv;
	if (strncmp(poi,"AC",2)==0)
	while (true) {
		//not autofreed, because it is referenced by the Script
		Action* aC = new Action(false);
		count = stream->ReadLine( line, 1024 );
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
		aCv.push_back( aC );
		stream->ReadLine( line, 1024 );
		if (strncmp( line, "RE", 2 ) == 0)
			break;
	}
	free( line );
	rE->actionsCount = ( unsigned char ) aCv.size();
	rE->actions = new Action* [rE->actionsCount];
	for (int i = 0; i < rE->actionsCount; i++) {
		rE->actions[i] = aCv.at( i );
	}
	return rE;
}

static ResponseSet* ReadResponseSet(DataStream* stream)
{
	char line[10];

	stream->ReadLine( line, 10 );
	if (strncmp( line, "RS", 2 ) != 0) {
		return NULL;
	}
	ResponseSet* rS = new ResponseSet();
	std::vector< Response*> rEv;
	while (true) {
		Response* rE = ReadResponse( stream );
		if (!rE)
			break;
		rEv.push_back( rE );
	}
	rS->responsesCount = ( unsigned short ) rEv.size();
	rS->responses = new Response * [rS->responsesCount];
	for (int i = 0; i < rS->responsesCount; i++) {
		rS->responses[i] = rEv.at( i );
	}
	return rS;
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
	std::vector< Trigger*> tRv;
	while (true) {
		Trigger* tR = ReadTrigger( stream );
		if (!tR)
			break;
		tRv.push_back( tR );
	}
	cO->triggersCount = ( unsigned short ) tRv.size();
	cO->triggers = new Trigger * [cO->triggersCount];
	for (int i = 0; i < cO->triggersCount; i++) {
		cO->triggers[i] = tRv.at( i );
	}
	return cO;
}

static ResponseBlock* ReadResponseBlock(DataStream* stream)
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



bool Script::Open(DataStream *stream)
{
	char line[10];

	stream->ReadLine( line, 10 );
	if (strncmp( line, "SC", 2 ) != 0) {
		printMessage( "GameScript","Not a Compiled Script file\n", YELLOW );
		delete( stream );
		return false;
	}
	if (InDebug&ID_REFERENCE) {
		//printf("Caching %s for the %d. time\n", ResRef, BcsCache.RefCount(ResRef) );
	}

	std::vector< ResponseBlock*> rBv;
	while (true) {
		ResponseBlock* rB = ReadResponseBlock( stream );
		if (!rB)
			break;
		rBv.push_back( rB );
		stream->ReadLine( line, 10 );
	}
	AllocateBlocks( ( unsigned int ) rBv.size() );
	for (unsigned int i = 0; i < responseBlocksCount; i++) {
		responseBlocks[i] = rBv.at( i );
	}
	delete( stream );
	return true;
}
