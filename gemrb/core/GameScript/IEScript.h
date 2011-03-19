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

#include "Owner.h"

class IEScript;

class GEM_EXPORT Response {
public:
	Response()
	{
		weight = 0;
	}
	int Execute(IEScript* Script, Scriptable* Sender);
public:
	unsigned char weight;
	std::vector<Holder<Action> > actions;
};

class GEM_EXPORT ResponseSet {
public:
	int Execute(IEScript* Script, Scriptable* Sender);
public:
	std::vector<Owner<Response> > responses;
};

class GEM_EXPORT ResponseBlock {
public:
	Owner<Condition> condition;
	Owner<ResponseSet> responseSet;
};

class GEM_EXPORT Script {
public:
	std::vector<Owner<ResponseBlock> > responseBlocks;
};

class IEScript : public GameScript {
	friend class Response; // for scriptlevel
public:
	IEScript();
	~IEScript();
	virtual bool Open(DataStream* str);
public:
	virtual bool Update(bool *continuing = NULL, bool *done = NULL);
	virtual void EvaluateAllBlocks();
private: //Internal variables
	Script* script;
	unsigned int lastAction;
};

#endif
