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

#include "BIFImporter.h"

#include "win32def.h"

#include "Compressor.h"
#include "FileCache.h"
#include "Interface.h"
#include "PluginMgr.h"
#include "System/SlicedStream.h"
#include "System/FileStream.h"

BIFImporter::FileEntry::FileEntry()
{
}

BIFImporter::TileEntry::TileEntry()
{
}

BIFImporter::BIFImporter()
	: stream(NULL)
{
}

BIFImporter::~BIFImporter(void)
{
	if (stream) {
		delete( stream );
	}
}

DataStream* BIFImporter::DecompressBIFC(DataStream* compressed, const char* path)
{
	print( "Decompressing\n" );
	if (!core->IsAvailable( PLUGIN_COMPRESSION_ZLIB ))
		return NULL;
	PluginHolder<Compressor> comp(PLUGIN_COMPRESSION_ZLIB);
	ieDword unCompBifSize;
	compressed->ReadDword( &unCompBifSize );
	print( "\nDecompressing file: [..........]" );
	fflush(stdout);
	FileStream out;
	if (!out.Create(path)) {
		printMessage("BIFImporter", "Cannot write %s.\n", RED, path);
		return NULL;
	}
	ieDword finalsize = 0;
	int laststep = 0;
	while (finalsize < unCompBifSize) {
		ieDword complen, declen;
		compressed->ReadDword( &declen );
		compressed->ReadDword( &complen );
		if (comp->Decompress( &out, compressed, complen ) != GEM_OK) {
			return NULL;
		}
		finalsize = out.GetPos();
		if (( int ) ( finalsize * ( 10.0 / unCompBifSize ) ) != laststep) {
			laststep++;
			print( "\b\b\b\b\b\b\b\b\b\b\b" );
			int l;

			for (l = 0; l < laststep; l++)
				print( "|" );
			for (; l < 10; l++)//l starts from laststep
				print( "." );
			print( "]" );
			fflush(stdout);
		}
	}
	print( "\n" );
	return FileStream::OpenFile(path);
}

DataStream* BIFImporter::DecompressBIF(DataStream* compressed, const char* /*path*/)
{
	ieDword fnlen, complen, declen;
	compressed->ReadDword( &fnlen );
	compressed->Seek(fnlen, GEM_CURRENT_POS);
	compressed->ReadDword(&declen);
	compressed->ReadDword(&complen);
	print( "Decompressing\n" );
	return CacheCompressedStream(compressed, compressed->filename, complen);
}

int BIFImporter::OpenArchive(const char* path)
{
	if (stream) {
		delete( stream );
		stream = NULL;
	}

	char filename[_MAX_PATH];
	ExtractFileFromPath(filename, path);

	char cachePath[_MAX_PATH];
	PathJoin(cachePath, core->CachePath, filename, NULL);
	stream = FileStream::OpenFile(cachePath);

	char Signature[8];
	if (!stream) {
		FileStream* file = FileStream::OpenFile(path);
		if (!file) {
			return GEM_ERROR;
		}
		if (file->Read(Signature, 8) == GEM_ERROR) {
			delete file;
			return GEM_ERROR;
		}

		if (strncmp(Signature, "BIF V1.0", 8) == 0) {
			stream = DecompressBIF(file, cachePath);
			delete file;
		} else if (strncmp(Signature, "BIFCV1.0", 8) == 0) {
			stream = DecompressBIFC(file, cachePath);
			delete file;
		} else if (strncmp( Signature, "BIFFV1  ", 8 ) == 0) {
			stream = CacheStream(file);
		} else {
			delete file;
			return GEM_ERROR;
		}
	}

	if (!stream)
		return GEM_ERROR;

	stream->Read( Signature, 8 );

	if (strncmp( Signature, "BIFFV1  ", 8 ) != 0) {
		return GEM_ERROR;
	}

	ReadBIF();
	return GEM_OK;
}

DataStream* BIFImporter::GetStream(unsigned long Resource, unsigned long Type)
{
	if (Type == IE_TIS_CLASS_ID) {
		const TileEntry *te = tiles.get(Resource & 0xfc000);

		if (!te)
			return NULL;

		return SliceStream(stream, te->offset, te->size * te->count);
	} else {
		const FileEntry *fe = files.get(Resource & 0x3fff);

		if (!fe)
			return NULL;

		return SliceStream(stream, fe->offset, fe->size);
	}
	return NULL;
}

void BIFImporter::ReadBIF(void)
{
	files.clear();
	tiles.clear();

	ieDword fileCount, tileCount, offset;

	stream->ReadDword(&fileCount);
	stream->ReadDword(&tileCount);
	stream->ReadDword(&offset);

	stream->Seek(offset, GEM_STREAM_START);

	ieDword i, locator;
	ieWord dummy;

	files.init(fileCount, fileCount);

	FileEntry fe;
	for (i = 0; i < fileCount; ++i) {
		stream->ReadDword(&locator);
		stream->ReadDword(&fe.offset);
		stream->ReadDword(&fe.size);
		stream->ReadWord(&dummy); // type
		stream->ReadWord(&dummy); // unknown

		if (files.set(locator & 0x3fff, fe))
			printMessage("BIFImporter", "Duplicate file locator 0x%x in '%s'.", LIGHT_RED, locator, stream->filename);
	}

	tiles.init(tileCount, tileCount);

	TileEntry te;
	for (i = 0; i < tileCount; ++i) {
		stream->ReadDword(&locator);
		stream->ReadDword(&te.offset);
		stream->ReadDword(&te.count);
		stream->ReadDword(&te.size);
		stream->ReadWord(&dummy); // type
		stream->ReadWord(&dummy); // unknown

		if (tiles.set(locator & 0xfc000, te))
			printMessage("BIFImporter", "Duplicate tile locator 0x%x in '%s'.", LIGHT_RED, locator, stream->filename);
	}
}

#include "plugindef.h"

GEMRB_PLUGIN(0xC7F133C, "BIF File Importer")
PLUGIN_CLASS(IE_BIF_CLASS_ID, BIFImporter)
END_PLUGIN()
