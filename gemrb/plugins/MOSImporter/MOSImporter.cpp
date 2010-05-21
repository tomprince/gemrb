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
 *
 *
 */

#include "win32def.h"
#include "MOSImporter.h"
#include "RGBAColor.h"
#include "Compressor.h"
#include "FileStream.h"
#include "CachedFileStream.h"
#include "Interface.h"
#include "Video.h"

MOSImporter::MOSImporter(void)
{
	HasColorKey = true;
	ColorKey.r = 0;
	ColorKey.g = 0xff;
	ColorKey.b = 0;
	ColorKey.a = 0xff;
}

MOSImporter::~MOSImporter(void)
{
}

bool MOSImporter::Open(DataStream* stream)
{
	str = stream;
	char Signature[8];
	str->Read( Signature, 8 );
	if (strncmp( Signature, "MOSCV1  ", 8 ) == 0) {
		char cpath[_MAX_PATH];
		strcpy( cpath, core->CachePath );
		strcat( cpath, stream->filename );
		FILE* exist_in_cache = fopen( cpath, "rb" );
		if (exist_in_cache) {
			//File was previously cached, using local copy
			delete( str );
			fclose( exist_in_cache );
			FileStream* s = new FileStream();
			s->Open( cpath );
			str = s;
			str->Read( Signature, 8 );
		} else {
			//No file found in Cache, Decompressing and storing for further use
			str->Seek( 4, GEM_CURRENT_POS );

			if (!core->IsAvailable( IE_COMPRESSION_CLASS_ID )) {
				printf( "No Compression Manager Available.\nCannot Load Compressed Mos File.\n" );
				return false;
			}	
			FILE* newfile = fopen( cpath, "wb" );
			Compressor* comp = ( Compressor* )
				core->GetInterface( IE_COMPRESSION_CLASS_ID );
			comp->Decompress( newfile, str );
			comp->release();
			fclose( newfile );
			delete( str );
			FileStream* s = new FileStream();
			s->Open( cpath );
			str = s;
			str->Read( Signature, 8 );
		}
	}
	if (strncmp( Signature, "MOS V1  ", 8 ) != 0) {
		return false;
	}
	ieWord w, h;
	str->ReadWord( &w );
	Width = w;
	str->ReadWord( &h );
	Height = h;
	str->ReadWord( &Cols );
	str->ReadWord( &Rows );
	str->ReadDword( &BlockSize );
	str->ReadDword( &PalOffset );

	RevColor RevCol[256];
	pixels = new Color[Width * Height];
	unsigned char * blockpixels = ( unsigned char * )
		malloc( BlockSize * BlockSize );
	ieDword blockoffset;
	for (int y = 0; y < Rows; y++) {
		int bh = ( y == Rows - 1 ) ?
			( ( Height % 64 ) == 0 ? 64 : Height % 64 ) :
			64;
		for (int x = 0; x < Cols; x++) {
			int bw = ( x == Cols - 1 ) ?
				( ( Width % 64 ) == 0 ? 64 : Width % 64 ) :
				64;
			str->Seek( PalOffset + ( y * Cols * 1024 ) +
				( x * 1024 ), GEM_STREAM_START );
			str->Read( &RevCol[0], 1024 );
			str->Seek( PalOffset + ( Rows * Cols * 1024 ) +
				( y * Cols * 4 ) + ( x * 4 ),
				GEM_STREAM_START );
			str->ReadDword( &blockoffset );
			str->Seek( PalOffset + ( Rows * Cols * 1024 ) +
				( Rows * Cols * 4 ) + blockoffset,
				GEM_STREAM_START );
			str->Read( blockpixels, bw * bh );
			unsigned char * bp = blockpixels;
			Color* startpixel = pixels +
				( ( Width * y ) * 64 ) +
				( x * 64 );
			for (int h = 0; h < bh; h++) {
				for (int w = 0; w < bw; w++) {
					startpixel[w].a = 0xFF;
					startpixel[w].b = RevCol[*bp].b;
					startpixel[w].g = RevCol[*bp].g;
					startpixel[w].r = RevCol[*bp].r;
					bp++;
				}
				startpixel = startpixel + ( Width );
			}
		}
	}
	free( blockpixels );
	return true;
}

#include "plugindef.h"

GEMRB_PLUGIN(0x167B73E, "MOS File Importer")
PLUGIN_IE_RESOURCE(MOSImporter, ".mos", (ieWord)IE_MOS_CLASS_ID)
END_PLUGIN()
