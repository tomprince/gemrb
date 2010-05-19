#ifndef SCRIPT_H
#define SCRIPT_H

#include <vector>
#include <list>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include "ie_types.h"
#include "exports.h"
#include "Region.h"

class DataStream;
class Scriptable;

#define MAX_OBJECT_FIELDS	10
#define MAX_NESTING		5
#define GSASSERT(var, var2)

typedef std::vector<ieDword> SrcVector;

struct targettype {
	Scriptable *actor; //hmm, could be door
	unsigned int distance;
};

typedef std::list<targettype> targetlist;

class GEM_EXPORT Targets {
public:
	Targets()
	{
	}

	~Targets()
	{
		Clear();
	}
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

		canary = (unsigned long) 0xdeadbeef;
	}
	~Object()
	{
	}
public:
	int objectFields[MAX_OBJECT_FIELDS];
	int objectFilters[MAX_NESTING];
	int objectRect[4];
	char objectName[65];
private:
	volatile unsigned long canary;
public:
	void Dump()
	{
		int i;

		GSASSERT( canary == (unsigned long) 0xdeadbeef, canary );
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

	void Release()
	{
		GSASSERT( canary == (unsigned long) 0xdeadbeef, canary );
		canary = 0xdddddddd;
		delete this;
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
		canary = (unsigned long) 0xdeadbeef;
	}
	~Trigger()
	{
		if (objectParameter) {
			objectParameter->Release();
			objectParameter = NULL;
		}
	}
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
private:
	volatile unsigned long canary;
public:
	void Dump()
	{
		GSASSERT( canary == (unsigned long) 0xdeadbeef, canary );
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

	void Release()
	{
		GSASSERT( canary == (unsigned long) 0xdeadbeef, canary );
		canary = 0xdddddddd;
		delete this;
	}
};

class GEM_EXPORT Condition {
public:
	Condition()
	{
		triggers = NULL;
		triggersCount = 0;
		canary = (unsigned long) 0xdeadbeef;
	}
	~Condition()
	{
		if (!triggers) {
			return;
		}
		for (int c = 0; c < triggersCount; c++) {
			if (triggers[c]) {
				triggers[c]->Release();
				triggers[c] = NULL;
			}
		}
		delete[] triggers;
	}
public:
	unsigned short triggersCount;
	Trigger** triggers;
private:
	volatile unsigned long canary;
public:
	void Release()
	{
		GSASSERT( canary == (unsigned long) 0xdeadbeef, canary );
		canary = 0xdddddddd;
		delete this;
	}
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
		canary = (unsigned long) 0xdeadbeef;
	}
	~Action()
	{
		for (int c = 0; c < 3; c++) {
			if (objects[c]) {
				objects[c]->Release();
				objects[c] = NULL;
			}
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
	volatile unsigned long canary;
public:
	int GetRef() {
		return RefCount;
	}
	void Dump()
	{
		int i;

		GSASSERT( canary == (unsigned long) 0xdeadbeef, canary );
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
		GSASSERT( canary == (unsigned long) 0xdeadbeef, canary );
		if (!RefCount) {
			printf( "WARNING!!! Double Freeing in %s: Line %d\n", __FILE__,
				__LINE__ );
			abort();
		}
		RefCount--;
		if (!RefCount) {
			canary = 0xdddddddd;
			delete this;
		}
	}
	void IncRef()
	{
		GSASSERT( canary == (unsigned long) 0xdeadbeef, canary );
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
		actions = NULL;
		weight = 0;
		actionsCount = 0;
		canary = (unsigned long) 0xdeadbeef;
	}
	~Response()
	{
		if (!actions) {
			return;
		}
		for (int c = 0; c < actionsCount; c++) {
			if (actions[c]) {
				if (actions[c]->GetRef()>2) {
					printf("Residue action %d with refcount %d\n", actions[c]->actionID, actions[c]->GetRef());
				}
				actions[c]->Release();
				actions[c] = NULL;
			}
		}
		delete[] actions;
	}
public:
	unsigned char weight;
	unsigned char actionsCount;
	Action** actions;
private:
	volatile unsigned long canary;
public:
	void Release()
	{
		GSASSERT( canary == (unsigned long) 0xdeadbeef, canary );
		canary = 0xdddddddd;
		delete this;
	}
};

class GEM_EXPORT ResponseSet {
public:
	ResponseSet()
	{
		responses = NULL;
		responsesCount = 0;
		canary = (unsigned long) 0xdeadbeef;
	}
	~ResponseSet()
	{
		if (!responses) {
			return;
		}
		for (int b = 0; b < responsesCount; b++) {
			responses[b]->Release();
			responses[b] = NULL;
		}
		delete[] responses;
	}
public:
	unsigned short responsesCount;
	Response** responses;
private:
	volatile unsigned long canary;
public:
	void Release()
	{
		GSASSERT( canary == (unsigned long) 0xdeadbeef, canary );
		canary = 0xdddddddd;
		delete this;
	}
};

class GEM_EXPORT ResponseBlock {
public:
	ResponseBlock()
	{
		condition = NULL;
		responseSet = NULL;
		canary = (unsigned long) 0xdeadbeef;
	}
	~ResponseBlock()
	{
		if (condition) {
			condition->Release();
			condition = NULL;
		}
		if (responseSet) {
			responseSet->Release();
			responseSet = NULL;
		}
	}
public:
	Condition* condition;
	ResponseSet* responseSet;
private:
	volatile unsigned long canary;
public:
	void Release()
	{
		GSASSERT( canary == (unsigned long) 0xdeadbeef, canary );
		canary = 0xdddddddd;
		delete this;
	}
};

class GEM_EXPORT Script {
public:
	Script()
	{
		canary = (unsigned long) 0xdeadbeef;
		responseBlocks = NULL;
		responseBlocksCount = 0;
	}
	~Script()
	{
		FreeResponseBlocks();
	}
	bool Open(DataStream*);
	void AllocateBlocks(unsigned int count)
	{
		if (!count) {
			FreeResponseBlocks();
			responseBlocks = NULL;
			responseBlocksCount = 0;
		}
		responseBlocks = new ResponseBlock * [count];
		responseBlocksCount = count;
	}
private:
	void FreeResponseBlocks()
	{
		if (!responseBlocks) {
			return;
		}
		for (unsigned int i = 0; i < responseBlocksCount; i++) {
			if (responseBlocks[i]) {
				responseBlocks[i]->Release();
				responseBlocks[i] = NULL;
			}
		}
		delete[] responseBlocks;
	}
public:
	unsigned int responseBlocksCount;
	ResponseBlock** responseBlocks;
private:
	volatile unsigned long canary;
public:
	void Release()
	{
		GSASSERT( canary == (unsigned long) 0xdeadbeef, canary );
		canary = 0xdddddddd;
		delete this;
	}
};

#undef GSASSERT

#endif
