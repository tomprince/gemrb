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
#include "TISImporter.h"
#include "RGBAColor.h"
#include "Interface.h"
#include "Video.h"
#include "TISTile.h"

TISImporter::TISImporter(void)
{
}

TISImporter::~TISImporter(void)
{
}

bool TISImporter::Open(DataStream* stream, bool autoFree)
{
	if (!Resource::Open(stream, autoFree))
		return false;

	char Signature[8];
	str->Read( Signature, 8 );
	headerShift = 0;
	if (Signature[0] == 'T' && Signature[1] == 'I' && Signature[2] == 'S') {
		if (strncmp( Signature, "TIS V1  ", 8 ) != 0) {
			printf( "[TISImporter]: Not a Valid TIS File.\n" );
			return false;
		}
		str->ReadDword( &TilesCount );
		str->ReadDword( &TilesSectionLen );
		str->ReadDword( &headerShift );
		str->ReadDword( &TileSize );
	} else {
		str->Seek( -8, GEM_CURRENT_POS );
	}
	return true;
}

Sprite2D* TISImporter::GetTile(int index)
{
	RevColor RevCol[256];
	void* pixels = malloc( 4096 );
	unsigned long pos = index *(1024+4096) + headerShift;
	if(str->Size()<pos+1024+4096) {
		// try to only report error once per file
		static TISImporter *last_corrupt = NULL;
		if (last_corrupt != this) {
			/*printf("Invalid tile index: %d\n",index);
			printf("FileSize: %ld\n", str->Size() );
			printf("Position: %ld\n", pos);
			printf("Shift: %d\n", headerShift);*/
			printf("Corrupt WED file encountered; couldn't find any more tiles at tile %d\n", index);
			last_corrupt = this;
		}
	
		// original PS:T AR0609 and AR0612 report far more tiles than are actually present :(
		return EmptySprite();
	}
	str->Seek( ( index * ( 1024 + 4096 ) + headerShift ), GEM_STREAM_START );
	str->Read( &RevCol, 1024 );
	str->Read( pixels, 4096 );
	Sprite2D* spr = TileToSprite( RevCol, pixels );
	return spr;
}

#include "plugindef.h"

GEMRB_PLUGIN(0x19F91578, "TIS File Importer")
PLUGIN_IE_RESOURCE(&TileSetMgr::ID, TISImporter, ".tis", (ieWord)IE_TIS_CLASS_ID)
END_PLUGIN()
