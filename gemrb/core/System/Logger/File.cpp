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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "System/Logger/File.h"

#include "System/DataStream.h"
#include "System/Logging.h"

#include <cstdio>

FileLogger::FileLogger(DataStream* log_file)
	: StdioLogger(false), log_file(log_file)
{}

FileLogger::~FileLogger()
{
	delete log_file;
}

void FileLogger::vprint(const char *message, va_list ap)
{
	// Don't try to be smart.
	// Assume this is long enough. If not, message will be truncated.
	// MSVC6 has old vsnprintf that doesn't give length
	char buff[_MAX_PATH];
	vsnprintf(buff, _MAX_PATH, message, ap);
	log_file->Write(buff, strlen(buff));
}

Logger* createFileLogger(DataStream* log_file)
{
	return new FileLogger(log_file);
}

#include "System/FileStream.h"
#ifndef STATIC_LINK
# define STATIC_LINK
#endif
#include "plugindef.h"

static void addLogger()
{
	FileStream* log_file = new FileStream();
	if (log_file->Create("/tmp/gemrb.log"))
		AddLogger(createFileLogger(log_file));
}

GEMRB_PLUGIN(unused, "tmp/file logger")
PLUGIN_INITIALIZER(addLogger)
END_PLUGIN()
