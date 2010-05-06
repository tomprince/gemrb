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

#include "globals.h"
#include "PNGImporter.h"
#include "RGBAColor.h"
#include "Interface.h"
#include "Video.h"
#include "ImageFactory.h"

static void DataStream_png_read_data(png_structp png_ptr,
		 png_bytep data, png_size_t length)
{
	voidp read_io_ptr = png_get_io_ptr(png_ptr);
	DataStream* str = reinterpret_cast<DataStream*>(read_io_ptr);
	str->Read(data, length);
}

PNGImporter::PNGImporter(void)
	: png_ptr(), info_ptr(), end_info()
{
}

PNGImporter::~PNGImporter(void)
{
	if (png_ptr) {
		png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
	}
}

bool PNGImporter::Open(DataStream* stream)
{
	str = stream;

	png_byte header[8];
	if (stream->Read(header, 8) < 8) return false;
	if (png_sig_cmp(header, 0, 8)) {
		return false;
	}
	png_ptr = png_create_read_struct(
		PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr)
		return false;

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
	{
		png_destroy_read_struct(&png_ptr, (png_infopp)NULL,
			(png_infopp)NULL);
		return false;
	}

	end_info = png_create_info_struct(png_ptr);
	if (!end_info)
	{
		png_destroy_read_struct(&png_ptr, &info_ptr,
			(png_infopp)NULL);
		return false;
	}

	if (setjmp(png_jmpbuf(png_ptr))) {
		png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
		return false;
	}

	png_set_read_fn(png_ptr, stream, DataStream_png_read_data);
	png_set_sig_bytes(png_ptr, 8);

	png_read_info(png_ptr, info_ptr);

	png_uint_32 width, height;
	int bit_depth, color_type;
	int interlace_type, compression_type, filter_method;
	png_get_IHDR(png_ptr, info_ptr, &width, &height,
		 &bit_depth, &color_type,
		 &interlace_type, &compression_type, &filter_method);

	if (color_type != PNG_COLOR_TYPE_PALETTE &&
		png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
	{
		// if not indexed, turn transparency info into alpha
		// (if indexed, we use it directly for colorkeying)
		png_set_tRNS_to_alpha(png_ptr);
	}

	if (bit_depth == 16)
		png_set_strip_16(png_ptr);

	if (color_type == PNG_COLOR_TYPE_RGB)
		png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);

	png_read_update_info(png_ptr, info_ptr);


	Width = width;
	Height = height;

	hasPalette = (color_type == PNG_COLOR_TYPE_PALETTE);

	void *buffer;
	if (hasPalette)
		buffer = data = new unsigned char[Width * Height];
	else
		buffer = pixels = new Color[Width * Height];

	if (!ReadData((unsigned char*) buffer))
		return false;
	ReadPalette();

	return true;
}

bool PNGImporter::ReadData(unsigned char *buffer)
{
	png_bytep* row_pointers = new png_bytep[Height];
	for (unsigned int i = 0; i < Height; ++i)
		row_pointers[i] = reinterpret_cast<png_bytep>(&buffer[(hasPalette?1:4)*i*Width]);

	if (setjmp(png_jmpbuf(png_ptr))) {
		delete[] row_pointers;
		free( buffer );
		png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
		return false;
	}

	png_read_image(png_ptr, row_pointers);

	delete[] row_pointers;
	row_pointers = 0;

	// the end_info struct isn't used, but passing it anyway for now
	png_read_end(png_ptr, end_info);
	return true;
}

void PNGImporter::ReadPalette()
{
	if (hasPalette) {
		Palette = new Color[256];
		png_color* palette;
		int num_palette;
		png_get_PLTE(png_ptr, info_ptr, &palette, &num_palette);
		for (int i = 0; i < 256; i++) {
			Palette[i].r = palette[i%num_palette].red;
			Palette[i].g = palette[i%num_palette].green;
			Palette[i].b = palette[i%num_palette].blue;
			Palette[i].a = 0xff;
		}
	}
}

#include "plugindef.h"

GEMRB_PLUGIN(0x11C3EB12, "PNG File Importer")
PLUGIN_IE_RESOURCE(PNGImporter, ".png", (ieWord)IE_PNG_CLASS_ID)
END_PLUGIN()
