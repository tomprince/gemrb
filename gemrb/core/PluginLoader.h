/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#ifndef PLUGINLOADER_H
#define PLUGINLOADER_H

#include "SClassID.h" // For PluginID
#include "exports.h"
#include "globals.h"
#include "win32def.h"

#include <map>

#ifdef WIN32
typedef HINSTANCE LibHandle;
#else
typedef void *LibHandle;
#endif

class PluginMgr;

/**
 * @class PluginLoader
 * Class for loading GemRB plugins from shared libraries or DLLs.
 * It goes over all appropriately named files in PluginPath directory
 * and tries to load them one after another.
 */

class GEM_EXPORT PluginLoader {
private:
	struct PluginDesc {
		LibHandle handle;
		PluginID ID;
		const char *Description;
		bool (*Register)(PluginMgr*);
	};
	std::map<PluginID, PluginDesc> libs;
public:
	PluginLoader();
	~PluginLoader();

	void LoadPlugins(char* pluginpath);
};

#endif
