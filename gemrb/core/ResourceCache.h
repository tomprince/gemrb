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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef RESOURCECACHE_H
#define RESOURCECACHE_H

#include "Plugin.h"

class DataStream;
class ResourceSource;

#ifdef _MSC_VER // No SFINAE
#include "ResourceSource.h"
#endif

class ResourceCache : public Plugin {
public:
	ResourceCache();
	virtual ~ResourceCache();

	virtual bool Open(const char *filename, const char *description) = 0;
	virtual ResourceSource* GetSource() = 0;
	virtual DataStream* CreateFile(const char* resname, bool force = false) = 0;
	virtual DataStream* OpenFile(const char* resname) = 0;
	virtual void EmptyCache() = 0;
};

#endif
