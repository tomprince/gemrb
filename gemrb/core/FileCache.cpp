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
 */

#include "FileCache.h"

#include "Compressor.h"
#include "Interface.h"
#include "PluginMgr.h"
#include "System/FileStream.h"
#include "System/VFS.h"

DataStream* CacheCompressedStream(DataStream *stream, const char* filename, int length, bool overwrite)
{
	if (!core->IsAvailable(PLUGIN_COMPRESSION_ZLIB)) {
		printMessage("FileCache", "No Compression Manager Available. Cannot Load Compressed File.\n", RED);
		return NULL;
	}

	char fname[_MAX_PATH];
	ExtractFileFromPath(fname, filename);
	char path[_MAX_PATH];
	PathJoin(path, core->CachePath, fname, NULL);

	if (overwrite || !file_exists(path)) {
		FileStream out;
		if (!out.Create(path)) {
			printMessage("FileCache", "Cannot write %s.\n", RED, path);
			return NULL;
		}

		PluginHolder<Compressor> comp(PLUGIN_COMPRESSION_ZLIB);
		if (comp->Decompress(&out, stream, length) != GEM_OK)
			return NULL;
	} else {
		stream->Seek(length, GEM_CURRENT_POS);
	}
	return FileStream::OpenFile(path);
}
