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

#include "JPEGSourceManager.h"

JPEGSourceManager::JPEGSourceManager(DataStream *stream)
	: stream(stream)
{
	jpeg_source_mgr::init_source = init_source;
	jpeg_source_mgr::fill_input_buffer = fill_input_buffer;
	jpeg_source_mgr::skip_input_data = skip_input_data;
	jpeg_source_mgr::resync_to_restart = jpeg_resync_to_restart;
	jpeg_source_mgr::term_source = term_source;
	bytes_in_buffer = 0;
	next_input_byte = NULL;
}

void JPEGSourceManager::init_source(j_decompress_ptr cinfo)
{
	JPEGSourceManager *mgr = static_cast<JPEGSourceManager*>(cinfo->src);
	mgr->stream->Seek(0, GEM_STREAM_START);
	fill_input_buffer(cinfo);
}

boolean JPEGSourceManager::fill_input_buffer(j_decompress_ptr cinfo)
{
	JPEGSourceManager *mgr = static_cast<JPEGSourceManager*>(cinfo->src);
	unsigned long remaining = mgr->stream->Remains();
	if (remaining >= sizeof(mgr->buffer)) {
		mgr->stream->Read(mgr->buffer, sizeof(mgr->buffer));
		mgr->bytes_in_buffer = sizeof(mgr->buffer);
		mgr->next_input_byte = mgr->buffer;
	} else if (remaining > 0) {
		mgr->stream->Read(mgr->buffer, remaining);
		mgr->bytes_in_buffer = remaining;
		mgr->next_input_byte = mgr->buffer;
	} else {
		static unsigned char eoibuf[] = {
			0xff, JPEG_EOI
		};
		mgr->next_input_byte = eoibuf;
		mgr->bytes_in_buffer = 2;
	}
	return true;
}

void JPEGSourceManager::skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
	JPEGSourceManager *mgr = static_cast<JPEGSourceManager*>(cinfo->src);
	if (mgr->bytes_in_buffer >= (size_t)num_bytes) {
		mgr->next_input_byte += num_bytes;
		mgr->bytes_in_buffer -= num_bytes;
	} else {
		num_bytes -= mgr->bytes_in_buffer;
		mgr->stream->Seek(num_bytes, GEM_CURRENT_POS);
		fill_input_buffer(cinfo);
	}
}
void JPEGSourceManager::term_source(j_decompress_ptr)
{
}
