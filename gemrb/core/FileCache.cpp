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
#include "ResourceCache.h"

DataStream* CacheCompressedStream(DataStream *stream, const char* filename, int length, bool overwrite)
{
	if (!core->IsAvailable(PLUGIN_COMPRESSION_ZLIB)) {
		print( "No Compression Manager Available.\nCannot Load Compressed File.\n" );
		return NULL;
	}

	char fname[_MAX_PATH];
	ExtractFileFromPath(fname, filename);

	if (!overwrite) {
		DataStream* out = core->CacheDir->OpenFile(fname);
		if (out) {
			return out;
		}
	}

	DataStream* out = core->CacheDir->CreateFile(fname, overwrite);
	if (!out) {
		printMessage("FileCache", "Cannot write %s.\n", RED, filename);
		return NULL;
	}

	PluginHolder<Compressor> comp(PLUGIN_COMPRESSION_ZLIB);
	if (comp->Decompress(out, stream, length) != GEM_OK)
		return NULL;

	delete out;

	return core->CacheDir->OpenFile(fname);
}

DataStream* CacheStream(DataStream* src)
{
	src->Seek(0, GEM_STREAM_START);
	if (!core->SlowBIFs)
		return src;

	DataStream* dest = core->CacheDir->OpenFile(src->filename);
	if (dest) {
		delete src;
		return dest;
	}

	dest = core->CacheDir->CreateFile(src->filename);
	if (!dest) {
		error("Cache", "CachedFile failed to write to cached file (from '%s')\n", src->originalfile);
	}

	size_t blockSize = 1024 * 1000;
	char buff[1024 * 1000];
	do {
		if (blockSize > src->Remains())
			blockSize = src->Remains();
		size_t len = src->Read(buff, blockSize);
		size_t c = dest->Write(buff, len);
		if (c != len) {
			error("Cache", "CacheFile failed to write to cached file (from '%s')\n", src->originalfile);
		}
	} while (src->Remains());

	delete dest;

	dest = core->CacheDir->OpenFile(src->filename);

	delete src;
	return dest;
}
