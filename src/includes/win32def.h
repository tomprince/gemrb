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

/**
 * @file win32def.h
 * Some global definitions, mostly for Un*x vs. MS Windows compatibility
 * @author The GemRB Project
 */


#ifndef WIN32DEF_H
#define WIN32DEF_H

#include "exports.h"

#ifdef WIN32
# define WIN32_LEAN_AND_MEAN
# include <windows.h>

# if _MSC_VER >= 1000
// 4251 disables the annoying warning about missing dll interface in templates
#  pragma warning( disable: 4251 521 )
#  pragma warning( disable: 4275 )
//disables annoying warning caused by STL:Map in msvc 6.0
#  if _MSC_VER < 7000
#    pragma warning(disable:4786)
#  endif
# endif

# if defined(__MINGW32__) && ! defined(HAVE_SNPRINTF)
#  define HAVE_SNPRINTF 1
# endif

#else //WIN32
# ifndef ANDROID
#  include <config.h>
# endif
# include <cstdio>
# include <cstdlib>
# include <cstring>

# define stricmp strcasecmp
# define strnicmp strncasecmp
#endif //WIN32

#ifndef HAVE_SNPRINTF
# ifdef WIN32
#  define snprintf _snprintf
#  define HAVE_SNPRINTF 1
# else
#  include "System/snprintf.h"
# endif
#endif

#include "System/VFS.h"

#ifdef _MSC_VER
# ifndef round
#  define round(x) ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))
# endif
#endif

#ifndef M_PI
#define M_PI    3.14159265358979323846 // pi
#endif
#ifndef M_PI_2
#define M_PI_2  1.57079632679489661923 // pi/2
#endif

#include "logging.h"
#endif  //! WIN32DEF_H
