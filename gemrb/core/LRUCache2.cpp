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

#include "LRUCache2.h"
#include "Resource.h"
#include <cassert>
#include <cstdio>

LRUCache2::LRUCache2(size_t depth)
	: depth(depth>1?depth:1), size(0)
{
}

LRUCache2::~LRUCache2()
{
	for (table_t::iterator it = table.begin();
			it != table.end();
			++it) {
		it->second->release();
	}
}

bool LRUCache2::Add(const char* key, Resource* value)
{
	std::pair<table_t::iterator, bool> inserted =
		table.insert(std::make_pair(std::string(key),value));
	if (!inserted.second)
		return false;
	value->acquire();
	++size;
	if (size > depth) {
		queue_t::iterator it = queue.begin();
		while (it != queue.end() && size > depth) {
			if ((*it)->second->GetRefCount() == 1) {
				(*it)->second->release();
				table.erase(*it);
				it = queue.erase(it);
				--size;
			} else {
				++it;
			}
		}
	}
	queue.push_back(inserted.first);
	return true;
}

Resource* LRUCache2::Lookup(const char* key)
{
	table_t::iterator it = table.find(key);
	if (it == table.end())
		return NULL;
	it->second->acquire();
	return it->second;
}

bool LRUCache2::Touch(const char* key)
{
	table_t::iterator touched = table.find(key);
	for (queue_t::iterator it = queue.begin();
			it != queue.end();
			++it) {
		if (*it == touched) {
			queue.erase(it);
			queue.push_back(touched);
			return true;
		}
	}
	return false;
}
