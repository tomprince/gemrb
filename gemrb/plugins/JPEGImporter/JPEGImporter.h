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

#ifndef JPEGIMP_H
#define JPEGIMP_H

#include "ImageMgr.h"

#include <jpeglib.h>

class JPEGImporter : public ImageMgr {
private:
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
public:
	JPEGImporter(void);
	~JPEGImporter(void);
	void Close();
	bool Open(DataStream* stream);
	Sprite2D* GetSprite2D();
	int GetWidth() { return (int) cinfo.output_width; }
	int GetHeight() { return (int) cinfo.output_height; }
public:
	void release(void)
	{
		delete this;
	}
};

#endif
