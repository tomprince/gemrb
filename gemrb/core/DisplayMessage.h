/* GemRB - Infinity Engine Emulator
* Copyright (C) 2003-2005 The GemRB Project
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

/**
 * @file DisplayMessage.h
 * Declaration of the DisplayMessage class used for displaying messages in
 * game message window
 */

#ifndef DISPLAYMESSAGE_H
#define DISPLAYMESSAGE_H

#include "exports.h"
#include "ie_types.h"

#include <cstddef>

class Scriptable;

bool Init_DisplayString();

/** returns a string reference from a string reference index constant */
GEM_EXPORT ieStrRef GetStringReference(int stridx);
/** returns true if a string reference for a string reference index constant exists */
GEM_EXPORT bool HasStringReference(int stridx);
/** returns the speaker's color and name */
GEM_EXPORT unsigned int GetSpeakerColor(const char *&name, const Scriptable *&speaker);
/** displays any string in the textarea */
GEM_EXPORT void DisplayString(const char *txt, Scriptable *speaker=NULL);
/** displays a string constant in the textarea */
GEM_EXPORT void DisplayConstantString(int stridx, unsigned int color, Scriptable *speaker=NULL);
/** displays actor name - action : parameter */
GEM_EXPORT void DisplayConstantStringNameString(int stridx, unsigned int color, int stridx2, const Scriptable *actor);
/** displays a string constant followed by a number in the textarea */
GEM_EXPORT void DisplayConstantStringValue(int stridx, unsigned int color, ieDword value);
/** displays a string constant in the textarea, starting with speaker's name */
GEM_EXPORT void DisplayConstantStringName(int stridx, unsigned int color, const Scriptable *speaker);
/** displays a string constant in the textarea, starting with actor, and ending with target */
GEM_EXPORT void DisplayConstantStringAction(int stridx, unsigned int color, const Scriptable *actor, const Scriptable *target);
/** displays a string in the textarea */
GEM_EXPORT void DisplayString(int stridx, unsigned int color, ieDword flags);
GEM_EXPORT void DisplayString(const char *text, unsigned int color, Scriptable *target);
/** displays a string in the textarea, starting with speaker's name */
GEM_EXPORT void DisplayStringName(int stridx, unsigned int color, const Scriptable *speaker, ieDword flags);
GEM_EXPORT void DisplayStringName(const char *text, unsigned int color, const Scriptable *speaker);

#endif
