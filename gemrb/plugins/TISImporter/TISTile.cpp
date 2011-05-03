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
 * $Id$
 *
 */

#include "TISTile.h"
#include "Interface.h"

Sprite2D* TileToSprite(RevColor RevCol[256], void* pixels) {
	Color Palette[256];
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
	Sprite2D* spr = core->GetVideoDriver()->CreateSprite8( 64, 64, 8, pixels, Palette, transparent, transindex );
	spr->XPos = spr->YPos = 0;
	return spr;
}

Sprite2D* EmptySprite()
{
	Color Palette[256];
	void* pixels = malloc( 4096 );

	memset(pixels, 0, 4096);
	memset(Palette, 0, 256 * sizeof(Color));
	Palette[0].g = 200;
	Sprite2D* spr = core->GetVideoDriver()->CreateSprite8( 64, 64, 8, pixels, Palette, false, 0 );
	spr->XPos = spr->YPos = 0;
	return spr;
}
