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
 * $Id$
 *
 */

#include "JPEGImporter.h"
#include "JPEGSourceManager.h"
#include "../../includes/globals.h"
#include "JPEGImporter.h"
#include "../../includes/RGBAColor.h"
#include "../Core/Interface.h"
#include "../Core/Video.h"
#include "../Core/ImageFactory.h"

#include <jpeglib.h>

static ieDword blue_mask = 0x00ff0000;
static ieDword green_mask = 0x0000ff00;
static ieDword red_mask = 0x000000ff;
//static ieDword alpha_mask = 0xff000000;

JPEGImporter::JPEGImporter(void)
	: cinfo()
{
}

JPEGImporter::~JPEGImporter(void)
{
	jpeg_destroy((j_common_ptr)&cinfo);
}

void JPEGImporter::Close()
{
}

void throw_error_exit (j_common_ptr /*cinfo*/)
{
	throw "JPEG Error: FIXME: report error";
}

bool JPEGImporter::Open(DataStream* stream, bool autoFree)
{
	if (!Resource::Open(stream, autoFree))
		return false;
	jpeg_destroy((j_common_ptr)&cinfo); // should this be jpeg_abort?

        // Allocate and initialize a JPEG decompression object
	cinfo.err = jpeg_std_error(&jerr);
	cinfo.err->error_exit = throw_error_exit;
	// ...
	try {
		jpeg_create_decompress(&cinfo);
		// Specify the source of the compressed data (eg, a file)
		cinfo.src = new JPEGSourceManager(str);

		// Call jpeg_read_header() to obtain image info
		jpeg_read_header(&cinfo, TRUE);
		//Set parameters for decompression
		cinfo.out_color_space = JCS_RGB;
	} catch (char *) {
		return false;
		//FIXME: do something
	}
	return true;
}

Sprite2D* JPEGImporter::GetSprite2D()
{
	JSAMPROW buffer = NULL;;
	JSAMPARRAY scanlines = NULL;
	try {
		jpeg_start_decompress(&cinfo);
		buffer = new JSAMPLE[cinfo.output_width * cinfo.output_height * cinfo.output_components];
		scanlines = new JSAMPROW[cinfo.output_height * cinfo.output_components];
		for (size_t line = 0; line < cinfo.output_height; line++)
			scanlines[line] = buffer + line * cinfo.output_width * cinfo.output_components;
		JSAMPARRAY p = scanlines;
		while (cinfo.output_scanline < cinfo.output_height) {
			int lines = jpeg_read_scanlines(&cinfo, p, cinfo.output_height - cinfo.output_scanline);
			p += lines;
		}
		jpeg_finish_decompress(&cinfo);
	} catch (char *) {
		delete [] scanlines;
		delete [] buffer;
		return NULL;
	}

	delete [] scanlines;
	return core->GetVideoDriver()->CreateSprite( cinfo.output_width, cinfo.output_height, 24,
			red_mask, green_mask, blue_mask, 0x00000000, buffer,
			true, green_mask );
}

#include "../../includes/plugindef.h"

GEMRB_PLUGIN(0x1B89400, "JPEG Image Importer")
PLUGIN_CLASS(PLUGIN_IMAGE_READER_JPEG, JPEGImporter)
PLUGIN_RESOURCE(JPEGImporter, ".jpeg")
PLUGIN_RESOURCE(JPEGImporter, ".jpg")
END_PLUGIN()
