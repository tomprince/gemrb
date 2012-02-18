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

#include "System/Logger.h"

#include "System/Logging.h"

#include <cstdio>

Logger::Logger()
{}

Logger::~Logger()
{}

void Logger::destroy()
{
	delete this;
}

const char* log_level_text[] = {
	"FATAL",
	"ERROR",
	"WARNING",
	"", // MESSAGE
	"COMBAT",
	"DEBUG"
};

#ifdef ANDROID

#include "System/Logger/Android.h"
Logger* (*createDefaultLogger)() = createAndroidLogger;

#elif defined(WIN32) && !defined(WIN32_USE_STDIO)

#include "System/Logger/Win32Console.h"
Logger* (*createDefaultLogger)() = createWin32ConsoleLogger;

#else

#include "System/Logger/Stdio.h"
Logger* (*createDefaultLogger)() = createStdioLogger;

#endif
