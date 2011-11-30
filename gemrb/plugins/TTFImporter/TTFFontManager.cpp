/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2011 The GemRB Project
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
 *
 *
 */

#include "TTFFontManager.h"

#include "win32def.h"

#include "FileCache.h"
#include "GameData.h"
#include "Interface.h"
#include "Palette.h"
#include "Sprite2D.h"
#include "Video.h"
#include "System/FileStream.h"

/*
TODO: ditch SDL_TTF and use FreeType directly
*/

TTFFontManager::~TTFFontManager(void)
{
	if (TTF_WasInit()) TTF_Quit();
}

TTFFontManager::TTFFontManager(void)
{
	FontPath[0] = 0;
	if (!TTF_WasInit()) TTF_Init();
}

bool TTFFontManager::Open(DataStream* stream)
{
	if (stream) {
		strncpy(FontPath, stream->originalfile, sizeof(FontPath));
		// we don't actually need anything from the stream.
		return true;
	}
	return false;
}

Font* TTFFontManager::GetFont(uint16_t FirstChar,
							  uint16_t LastChar,
							  unsigned short ptSize,
							  FontStyle style, Palette* pal)
{
	printMessage( "TTF", "Constructing TTF font.", WHITE );
	print("Creating font of size %i with %i characters...", ptSize, LastChar - FirstChar);

	TTF_Font* ttf = TTF_OpenFont(FontPath, ptSize);

	if (!ttf){
		printMessage( "TTF", "Unable to initialize font: ", WHITE );
		print("%s; TTFError:%s.", FontPath, TTF_GetError());
		printStatus( "ERROR", LIGHT_RED );
		return NULL;
	}
	if (!ptSize) {
		printMessage( "TTF", "Unable to initialize font with size 0.", WHITE );
		printStatus( "ERROR", LIGHT_RED );
		return NULL;
	}

	TTF_SetFontStyle(ttf, style);

	if (!pal) {
		Color fore = {0xFF, 0xFF, 0xFF, 0}; //white
		Color back = {0x00, 0x00, 0x00, 0}; //black
		pal = core->CreatePalette( fore, back );
		pal->CreateShadedAlphaChannel();
	}

	Sprite2D** glyphs = (Sprite2D**)malloc((LastChar - FirstChar + 1) * sizeof(Sprite2D*));

	Uint16 i; // for double byte character suport
	Uint16 chr[3];
	chr[0] = UNICODE_BOM_NATIVE;
	chr[2] = '\0';// is this needed?

	for (i = FirstChar; i <= LastChar; i++) { //printable ASCII range minus space
		chr[1] = i;

		SDL_Surface* glyph = TTF_RenderUNICODE_Shaded(ttf, chr, *(SDL_Color*)(&pal->front), *(SDL_Color*)(&pal->back));
		if (glyph){
			void* px = malloc(glyph->w * glyph->h);

			//need to convert pitch to glyph width here. video driver assumes this.
			unsigned char * dstPtr = (unsigned char*)px;
			unsigned char * srcPtr = (unsigned char*)glyph->pixels;
			for (int glyphY = 0; glyphY < glyph->h; glyphY++) {
				memcpy( dstPtr, srcPtr, glyph->w);
				srcPtr += glyph->pitch;
				dstPtr += glyph->w;
			}
			glyphs[i - FirstChar] = core->GetVideoDriver()->CreateSprite8(glyph->w, glyph->h, 8, px, pal->col, true, 0);
			glyphs[i - FirstChar]->XPos = 0;
			glyphs[i - FirstChar]->YPos = 20; //FIXME: figure out why this is required and find true value
		}
		else glyphs[i - FirstChar] = NULL;
	}
	Font* font = new Font(glyphs, FirstChar, LastChar, pal);
	font->ptSize = ptSize;
	font->style = style;
	free(glyphs);
	printStatus( "OK", LIGHT_GREEN );
	return font;
}

#include "plugindef.h"

GEMRB_PLUGIN(0x3AD6427C, "TTF Font Importer")
PLUGIN_RESOURCE(TTFFontManager, "ttf")
END_PLUGIN()
