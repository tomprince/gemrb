/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2007
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef LRUCACHE2_H
#define LRUCACHE2_H 

#include <string>
#include <map>
#include <list>
#include "exports.h"

class Resource;

class GEM_EXPORT LRUCache2 {
public:
	LRUCache2(size_t depth = 10);
	~LRUCache2();

	/// key must not be in table.
	bool Add(const char* key, Resource* value);
	Resource* Lookup(const char* key);
	bool Touch(const char* key);
private:
	size_t depth;
	size_t size;
	typedef std::map<std::string, Resource*> table_t;
	table_t table;
	typedef std::list<table_t::iterator> queue_t;
	queue_t queue;
};



#endif
