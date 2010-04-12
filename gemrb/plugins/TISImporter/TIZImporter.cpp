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

#include "../../includes/win32def.h"
#include "TIZImporter.h"
#include "../../includes/RGBAColor.h"
#include "../Core/Interface.h"
#include "../Core/Compressor.h"
#include "../Core/Video.h"
#include "../Core/MemoryStream.h"
#include "TISTile.h"

TIZImporter::TIZImporter(void)
{
	TileIndex = NULL;
}

TIZImporter::~TIZImporter(void)
{
	free(TileIndex);
}

bool TIZImporter::Open(DataStream* stream, bool autoFree)
{
	if (!Resource::Open(stream, autoFree))
		return false;
	free(TileIndex); TileIndex = 0;

	char Signature[4];
	str->Read( Signature, 4 );
	if (strncmp( Signature, "TIZ0", 4 ) != 0) {
		printf( "[TIZImporter]: Not a Valid TIZ File.\n" );
		return false;
	}
	unsigned char word[2];
	str->Read( word, 2 );
	TilesCount = word[0] << 8 | word[1];
	TileIndex = (unsigned int*)malloc(sizeof(unsigned int)*TilesCount);
	ieWord reserved;
	str->ReadWord( &reserved );
	if (reserved) {
		printf( "[TIZImporter]: Unsupported TIZ File.\n" );
		return false;
	}
	for (int i = 0; i < TilesCount; i++) {
		if (str->Remains() < 6) {
			printf( "[TIZImporter]: Incomplete TIZ File.\n" );
		}
		str->Read(Signature, 4);
		if (strncmp( Signature, "TIL", 3 ) != 0) {
			printf( "[TIZImporter]: Invalid TIZ file TIL signature.\n" );
			return false;
		}
		TileIndex[i] = str->GetPos();
		str->Read(word, 2);
		str->Seek(word[0]<<8|word[1], GEM_CURRENT_POS);
	}
	return true;
}

static Sprite2D* TIL0Uncompress(unsigned char *buffer, unsigned int length)
{
	char tile[0x1400];
	Compressor* comp = ( Compressor* )
		core->GetInterface( IE_COMPRESSION_CLASS_ID );
	comp->Decompress(tile,0x1400,(char*)buffer,length);
	free(buffer);

	void* pixels = malloc( 4096 );
	memcpy(pixels,tile+1024, 4096 );

	Sprite2D *spr = TileToSprite((RevColor*)tile, pixels);
	return spr;
}

static Sprite2D* TIL1Uncompress(unsigned char *buffer, unsigned int length)
{
	ieWord palettelen = buffer[0] << 8 | buffer[1];
	buffer += 2; length -= 2;

	unsigned char paletteData[256*3  + 512];
	Compressor* comp = ( Compressor* )
		core->GetInterface( IE_COMPRESSION_CLASS_ID );
	comp->Decompress((char*)paletteData,sizeof(paletteData),(char*)buffer,palettelen);
	buffer += palettelen; length -= palettelen;
//	unsigned char *palette[3] = { paletteData, paletteData + 256, paletteData + 512 };

	MemoryStream stream(buffer, length, false);

	ImageMgr* im = ( ImageMgr* ) core->GetInterface( PLUGIN_IMAGE_READER_JPEG );
	if (!im->Open(&stream,false))
		return EmptySprite();

	Sprite2D *spr = im->GetSprite2D();
	//for (int y = 0; y < 64; ++y) {
	//	for (int i = 0; i < 8; ++i) {
	//	}
	//}

//	applyalpha(paletteData + 768, tile + 1024);
	core->FreeInterface(im);
	return spr;
}
static Sprite2D* TIL2Uncompress(unsigned char *buffer, unsigned int length)
{
	MemoryStream stream(buffer, length, false);

	ImageMgr* im = ( ImageMgr* ) core->GetInterface( PLUGIN_IMAGE_READER_JPEG );
	if (!im->Open(&stream,false))
		return EmptySprite();

	Sprite2D *spr = im->GetSprite2D();
	core->FreeInterface(im);
	return spr;
}

Sprite2D* TIZImporter::GetTile(int index)
{
	if(index > TilesCount) {
		// try to only report error once per file
		static TIZImporter *last_corrupt = NULL;
		if (last_corrupt != this) {
			printf("Corrupt WED file encountered; couldn't find any more tiles at tile %d\n", index);
			last_corrupt = this;
		}

		// original PS:T AR0609 and AR0612 report far more tiles than are actually present :(
		return EmptySprite();
	}
	unsigned long pos = TileIndex[index];
	str->Seek( pos-1, GEM_STREAM_START );
	char format;
	str->Read( &format, 1 );
	unsigned char word[2];
	str->Read( word, 2 );
	ieWord length = word[0] << 8 | word[1];
	unsigned char *buffer = (unsigned char*)malloc(length); // FIXME: Free this.
	str->Read(buffer, length);
	switch (format) {
	case '0':
		return TIL0Uncompress(buffer,length);
	case '1':
		return TIL1Uncompress(buffer,length);
	case '2':
		return TIL2Uncompress(buffer,length);
	default:
		printf("Unknown TIZ tile format at tile %d\n", index);
		return EmptySprite();
	}
}
