/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "ImageMgr.h"

#include "win32def.h"

#include "ImageFactory.h"
#include "Interface.h"
#include "Video.h"

const TypeID ImageMgr::ID = { "ImageMgr" };

ImageMgr::ImageMgr(void)
	: HasColorKey(false), pixels(), data(), Palette()
{
}

ImageMgr::~ImageMgr(void)
{
	delete[] data;
	delete[] Palette;
	delete[] pixels;
}

size_t ImageMgr::GetWidth()
{
	return Width;
}

size_t ImageMgr::GetHeight()
{
	return Height;
}

Sprite2D* ImageMgr::GetSprite2D()
{
	if (Palette) {
		void *p = malloc(Width * Height);
		memcpy(p, data, Width * Height);
		return core->GetVideoDriver()->CreateSprite8( Width, Height, 8,
				p, Palette, HasColorKey, ColorKeyIndex);
	} else {
		// FIXME: copy paste
		union {
			Color color;
			ieDword Mask;
		} r = {{ 0xFF, 0x00, 0x00, 0x00 }},
		      g = {{ 0x00, 0xFF, 0x00, 0x00 }},
		      b = {{ 0x00, 0x00, 0xFF, 0x00 }},
		      a = {{ 0x00, 0x00, 0x00, 0xFF }},
		      cK = { ColorKey };
		void *p = malloc(sizeof(Color) * Height*Width);
		memcpy(p, pixels, sizeof(Color)*Height*Width);
		return core->GetVideoDriver()->CreateSprite(Width, Height, 32,
				r.Mask, g.Mask, b.Mask, !HasColorKey ? a.Mask : 0, p, HasColorKey, cK.Mask);
	}
}

Bitmap* ImageMgr::GetBitmap()
{
	if (Palette) {
		Bitmap *bitmap = new Bitmap(Width, Height, data);
		data = NULL;
		return bitmap;
	} else {
		printMessage("ImageMgr", "Don't know how to handle 24bit bitmap from %s...", WHITE,
				str->filename );
		printStatus( "ERROR", LIGHT_RED );

		Bitmap *bitmap = new Bitmap(Width, Height);

		for (unsigned int y = 0; y < Height; y++) {
			for (unsigned int x = 0; x < Width; x++) {
				bitmap->SetAt(x,y, pixels[y*Width+x].r);
			}
		}
		return bitmap;
	}
}

Image* ImageMgr::GetImage()
{
	if (!Palette) {
		Image *image = new Image(Width, Height, pixels);
		pixels = NULL;
		return image;
	} else {
		Image *image = new Image(Width, Height);

		for (unsigned int y = 0; y < Height; y++) {
			for (unsigned int x = 0; x < Width; x++) {
				image->SetPixel(x,y, Palette[data[y*Width + x]]);
			}
		}

		return image;
	}
}

void ImageMgr::GetPalette(size_t colors, Color* pal)
{
	if (Palette) {
		if (colors > 256)
			colors = 256;
		memcpy(pal, Palette, sizeof(Color) * colors);
	} else {
		printMessage("ImageMgr", "Can't get non-existant palette from %s... ", WHITE,
			str->filename);
		printStatus("ERROR", LIGHT_RED);
	}
}

ImageFactory* ImageMgr::GetImageFactory(const char* ResRef)
{
	ImageFactory* fact = new ImageFactory( ResRef, GetSprite2D() );
	return fact;
}
