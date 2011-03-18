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

#ifndef IESCRIPT_H
#define IESCRIPT_H

#include "GameScript/GS.h"

class GEM_EXPORT Response {
public:
	Response()
	{
		weight = 0;
	}
	int Execute(Scriptable* Sender);
public:
	unsigned char weight;
	std::vector<Holder<Action> > actions;
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

#endif
