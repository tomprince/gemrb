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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Id$
 *
 */

#include "../../includes/win32def.h"
#include "TISImp.h"
#include "../../includes/RGBAColor.h"
#include "../Core/Interface.h"
#include "../Core/Video.h"

TISImp::TISImp(void)
{
	str = NULL;
	autoFree = false;
}

TISImp::~TISImp(void)
{
	if (str && autoFree) {
		delete( str );
	}
}

bool TISImp::Open(DataStream* stream, bool autoFree)
{
	if (stream == NULL) {
		return false;
	}
	if (str && this->autoFree) {
		delete( str );
	}
	str = stream;
	this->autoFree = autoFree;
	char Signature[8];
	str->Read( Signature, 1 );
	str->Seek( -1, GEM_CURRENT_POS );
	headerShift = 0;
	if (Signature[0] == 'T') {
		str->Read( Signature, 8 );	
		if (strncmp( Signature, "TIS V1  ", 8 ) != 0) {
			printf( "[TISImporter]: Not a Valid TIS File.\n" );
			return false;
		}
		str->ReadDword( &TilesCount );
		str->ReadDword( &TilesSectionLen );
		str->ReadDword( &headerShift );
		str->ReadDword( &TileSize );
	}
	return true;
}

Tile* TISImp::GetTile(unsigned short* indexes, int count,
	unsigned short* secondary)
{
	Animation* ani = new Animation( count );
	ani->x = ani->y = 0;
	//pause key stops animation
	ani->gameAnimation = true;
	for (int i = 0; i < count; i++) {
		ani->AddFrame( GetTile( indexes[i] ), i );
	}
	if (secondary) {
		Animation* sec = new Animation( count );
		sec->x = sec->y = 0;
		for (int i = 0; i < count; i++) {
			sec->AddFrame( GetTile( secondary[i] ), i );
		}
		return new Tile( ani, sec );
	}
	return new Tile( ani );
}

Sprite2D* TISImp::GetTile(int index)
{
	RevColor RevCol[256];
	Color Palette[256];
	void* pixels = malloc( 4096 );
	unsigned long pos = index *(1024+4096) + headerShift;
	if(str->Size()<pos) {
		printf("Invalid tile index: %d\n",index);
		printf("FileSize: %ld\n", str->Size() );
		printf("Position: %ld\n", pos);
		printf("Shift: %d\n", headerShift);
	}
	str->Seek( ( index * ( 1024 + 4096 ) + headerShift ), GEM_STREAM_START );
	str->Read( &RevCol, 1024 );
	int transindex = 0;
	bool transparent = false;
	for (int i = 0; i < 256; i++) {
		Palette[i].r = RevCol[i].r;
		Palette[i].g = RevCol[i].g;
		Palette[i].b = RevCol[i].b;
		Palette[i].a = RevCol[i].a;
		if (Palette[i].g==255 && !Palette[i].r && !Palette[i].b) {
			if (transparent) {
				printMessage( "TISImp", "Tile has two green (transparent) palette entries\n", LIGHT_RED );
			} else {
				transparent = true;
				transindex = i;
			}
		}
	}
	str->Read( pixels, 4096 );
	Sprite2D* spr = core->GetVideoDriver()->CreateSprite8( 64, 64, 8, pixels, Palette, transparent, transindex );
	spr->XPos = spr->YPos = 0;
	return spr;
}
