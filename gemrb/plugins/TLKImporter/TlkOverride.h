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
// TlkOverride.h: interface for the CTlkOverride class.
//
//////////////////////////////////////////////////////////////////////

#ifndef TLKOVERRIDE_H
#define TLKOVERRIDE_H

#include "globals.h"

#include "Interface.h"
#include "System/FileStream.h"

#ifdef CACHE_TLK_OVERRIDE
#include <map>

typedef std::map<ieStrRef, char *> StringMapType;
#endif

#define STRREF_START  300000
#define SEGMENT_SIZE  512
#define TOH_HEADER_SIZE 20

//the original games used these strings for custom biography (another quirk of the IE)
#define BIO_START 62016                 //first BIO string
#define BIO_END   (BIO_START+5)         //last BIO string

typedef struct
{
	ieDword strref;
	ieByte dummy[20];
	ieDword offset;
} EntryType;

class CTlkOverride  
{
private:
#ifdef CACHE_TLK_OVERRIDE
	StringMapType stringmap;
#endif
	DataStream *tot_str;
	DataStream *toh_str;
	ieDword AuxCount;
	ieDword FreeOffset;

	void UpdateFreeOffset(ieDword NewFree);
	void CloseResources();
	DataStream *GetAuxHdr(bool create);
	DataStream *GetAuxTlk(bool create);
	ieStrRef GetNewStrRef();
	ieDword LocateString(ieStrRef strref);
	char* LocateString2(ieDword offset);
	ieDword GetLength();
public:
	CTlkOverride();
	virtual ~CTlkOverride();

	bool Init();
	char *ResolveAuxString(ieStrRef strref, int &Length);
	ieStrRef UpdateString(ieStrRef strref, const char *newvalue);
	char *CS(const char *src);
};

#endif //TLKOVERRIDE_H

