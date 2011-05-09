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

#ifndef BIFIMPORTER_H
#define BIFIMPORTER_H

#include "IndexedArchive.h"

#include "globals.h"
#include "HashMap.h"
#include "System/DataStream.h"

class BIFImporter : public IndexedArchive {
private:
	struct FileEntry {
		ieDword offset;
		ieDword size;

		FileEntry();
	};

	struct TileEntry {
		ieDword offset;
		ieDword count;
		ieDword size;

		TileEntry();
	};

	HashMap<ieDword, FileEntry> files;
	HashMap<ieDword, TileEntry> tiles;

	char path[_MAX_PATH];
	DataStream* stream;

public:
	BIFImporter(void);
	~BIFImporter(void);
	int OpenArchive(const char* filename);
	DataStream* GetStream(unsigned long Resource, unsigned long Type);

private:
	static bool DecompressBIF(DataStream* compressed, const char* path);
	void ReadBIF(void);
};

#endif
