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
 * $Id$
 *
 */

#include <cstddef>
#include <cstdio>
#include <jpeglib.h>
#include "../Core/DataStream.h"

class JPEGSourceManager : public jpeg_source_mgr
{
public:
	JPEGSourceManager(DataStream *stream);
private:
	DataStream *stream;
	unsigned char buffer[4096];
	static void init_source(j_decompress_ptr cinfo);
	static boolean fill_input_buffer(j_decompress_ptr cinfo);
	static void skip_input_data (j_decompress_ptr cinfo, long num_bytes);
	static void term_source(j_decompress_ptr cinfo);
};
