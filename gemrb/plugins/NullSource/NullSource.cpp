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
 */

#include "NullSource.h"

#include "globals.h"
#include "win32def.h"

#include "Interface.h"
#include "ResourceDesc.h"
#include "System/FileStream.h"

NullSource::NullSource(void)
{
	description = NULL;
}

NullSource::~NullSource(void)
{
	free(description);
}

bool NullSource::Open(const char *dir, const char *desc)
{
	return true;
}

bool NullSource::HasResource(const char* resname, SClass_ID type)
{
	return false;
}

bool NullSource::HasResource(const char* resname, const ResourceDesc &type)
{
	return false;
}

DataStream* NullSource::GetResource(const char* resname, SClass_ID type)
{
	return false;
}

DataStream* NullSource::GetResource(const char* resname, const ResourceDesc &type)
{
	return false;
}

#include "plugindef.h"

GEMRB_PLUGIN(0xAB4534, "Null Resource Source")
PLUGIN_CLASS(PLUGIN_RESOURCE_NULL, NullSource)
END_PLUGIN()
