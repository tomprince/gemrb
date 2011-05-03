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

#include "Sprite2D.h"

#include "win32def.h"

#include "Interface.h"
#include "Video.h"

const TypeID Sprite2D::ID = { "Sprite2D" };

Sprite2D::Sprite2D()
{
	BAM = false;
	vptr = NULL;
	pixels = NULL;
	XPos = 0;
	YPos = 0;
	RefCount = 1;
}

Sprite2D::~Sprite2D(void)
{
}

bool Sprite2D::IsPixelTransparent(unsigned short x, unsigned short y) const
{
	if (x >= Width || y >= Height) return true;

	if (!BAM) {
		return core->GetVideoDriver()->GetPixel(vptr, x, y)==0;
	}

	Sprite2D_BAM_Internal* data = (Sprite2D_BAM_Internal*)vptr;

	if (data->flip_ver)
		y = Height - y - 1;
	if (data->flip_hor)
		x = Width - x - 1;

	int skipcount = y * Width + x;

	const ieByte* rle = (const ieByte*)pixels;
	if (data->RLE) {
		while (skipcount > 0) {
			if (*rle++ == data->transindex)
				skipcount -= (*rle++)+1;
			else
				skipcount--;
		}
	} else {
		// uncompressed
		rle += skipcount;
		skipcount = 0;
	}
	if (skipcount < 0 || *rle == data->transindex)
		return true;

	return false;
}

/** Get the Palette of a Sprite */
Palette* Sprite2D::GetPalette() const
{
	if (!vptr) return NULL;
	if (!BAM) {
		return core->GetVideoDriver()->GetPalette(vptr);
	}

	Sprite2D_BAM_Internal* data = (Sprite2D_BAM_Internal*)vptr;
	data->pal->IncRef();
	return data->pal;
}

void Sprite2D::SetPalette(Palette* pal)
{
	if (!vptr) return;
	if (!BAM) {
		core->GetVideoDriver()->SetPalette(vptr, pal);
	} else {
		Sprite2D_BAM_Internal* data = (Sprite2D_BAM_Internal*)vptr;
		data->pal->Release();
		pal->IncRef();
		data->pal = pal;
	}
}

Color Sprite2D::GetPixel(unsigned short x, unsigned short y) const
{
	Color c = { 0, 0, 0, 0 };

	if (x >= Width || y >= Height) return c;

	if (!BAM) {
		core->GetVideoDriver()->GetPixel(vptr, x, y, c);
		return c;
	}

	Sprite2D_BAM_Internal* data = (Sprite2D_BAM_Internal*)vptr;

	if (data->flip_ver)
		y = Height - y - 1;
	if (data->flip_hor)
		x = Width - x - 1;

	int skipcount = y * Width + x;

	const ieByte *rle = (const ieByte*)pixels;
	if (data->RLE) {
		while (skipcount > 0) {
			if (*rle++ == data->transindex)
				skipcount -= (*rle++)+1;
			else
				skipcount--;
		}
	} else {
		// uncompressed
		rle += skipcount;
		skipcount = 0;
	}

	if (skipcount >= 0 && *rle != data->transindex) {
		c = data->pal->col[*rle];
		c.a = 0xff;
	}
	return c;
}

void Sprite2D::release()
{
	Sprite2D *that = this;
	core->GetVideoDriver()->FreeSprite(that);
}
