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

#ifndef GAMESCRIPT_H
#define GAMESCRIPT_H

#include "exports.h"
#include "ie_types.h"

#include "Holder.h"
#include "Resource.h"

#include <cstddef>

class Action;
class Script;
class Trigger;
class DataStream;
class Scriptable;

class GEM_EXPORT GameScript : public Resource {
public:
	static const TypeID ID;
public:
	GameScript();
	void SetStuff(const ieResRef ResRef, Scriptable* Myself, int ScriptLevel);
	const char *GetName() { return this?Name:"NONE\0\0\0\0"; }
	static void ExecuteString(Scriptable* Sender, char* String);
	static int EvaluateString(Scriptable* Sender, char* String);
	static void ExecuteAction(Scriptable* Sender, Holder<Action> aC);
public:
	virtual bool Update(bool *continuing = NULL, bool *done = NULL) = 0;
	virtual void EvaluateAllBlocks() = 0;
protected: //Internal variables
	Scriptable* MySelf;
	ieResRef Name;
	int scriptlevel;
};

class IEScript : public GameScript {
public:
	IEScript();
	~IEScript();
	virtual bool Open(DataStream* str);
public:
	virtual bool Update(bool *continuing = NULL, bool *done = NULL);
	virtual void EvaluateAllBlocks();
private: //Internal Functions
	ResponseBlock* ReadResponseBlock(DataStream* stream);
	ResponseSet* ReadResponseSet(DataStream* stream);
	Response* ReadResponse(DataStream* stream);
	Trigger* ReadTrigger(DataStream* stream);
	static int ParseInt(const char*& src);
	static void ParseString(const char*& src, char* tmp);
private: //Internal variables
	Script* script;
	unsigned int lastAction;
};

GEM_EXPORT Holder<Action> GenerateAction(char* String);
Holder<Action> GenerateActionDirect(char* String, Scriptable *object);
GEM_EXPORT Trigger* GenerateTrigger(char* String);

void InitializeIEScript();
extern void SetScriptDebugMode(int arg);

#endif
