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

#include "VarFile.h"

#include "Interface.h"
#include "Variables.h"
#include "System/FileStream.h"

void LoadInitialValues(Variables *dict, const char* name)
{
	char nPath[_MAX_PATH];
	// we only support PST's var.var for now
	PathJoin( nPath, core->GamePath, "var.var", NULL );
	FileStream fs;
	if (!fs.Open(nPath)) {
		return;
	}

	char buffer[41];
	ieDword value;
	buffer[40] = 0;
	ieVariable varname;

	// first value is useless
	if (!fs.Read(buffer, 40)) return;
	if (fs.ReadDword(&value) != 4) return;

	while (fs.Remains()) {
		// read data
		if (!fs.Read(buffer, 40)) return;
		if (fs.ReadDword(&value) != 4) return;
		// is it the type we want? if not, skip
		if (strnicmp(buffer, name, 6)) continue;
		// copy variable (types got 2 extra spaces, and the name is padded too)
		// (true = uppercase, needed for original engine save compat, see 315b8f2e)
		strnspccpy(varname,buffer+8,32, true);
		dict->SetAt(varname, value);
	}
}
