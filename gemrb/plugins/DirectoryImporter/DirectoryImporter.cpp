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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "DirectoryImporter.h"

#include "globals.h"
#include "win32def.h"

#include "Interface.h"
#include "ResourceDesc.h"
#include "System/FileStream.h"

DirectoryImporter::DirectoryImporter(void)
{
	description = NULL;
}

DirectoryImporter::~DirectoryImporter(void)
{
	free(description);
}

bool DirectoryImporter::Open(const char *dir, const char *desc)
{
	if (!dir_exists(dir))
		return false;

	free(description);
	description = strdup(desc);
	strcpy(path, dir);
	return true;
}

static bool FindIn(const char *Path, const char *ResRef, const char *Type)
{
	char p[_MAX_PATH], f[_MAX_PATH] = {0};
	strcpy(f, ResRef);
	strlwr(f);

	return PathJoinExt(p, Path, f, Type);
}

static FileStream *SearchIn(const char * Path,const char * ResRef, const char *Type)
{
	char p[_MAX_PATH], f[_MAX_PATH] = {0};
	strcpy(f, ResRef);
	strlwr(f);

	if (!PathJoinExt(p, Path, f, Type))
		return NULL;

	return FileStream::OpenFile(p);
}

bool DirectoryImporter::HasResource(const char* resname, SClass_ID type)
{
	return FindIn( path, resname, core->TypeExt(type) );
}

bool DirectoryImporter::HasResource(const char* resname, const ResourceDesc &type)
{
	return FindIn( path, resname, type.GetExt() );
}

DataStream* DirectoryImporter::GetResource(const char* resname, SClass_ID type)
{
	return SearchIn( path, resname, core->TypeExt(type) );
}

DataStream* DirectoryImporter::GetResource(const char* resname, const ResourceDesc &type)
{
	return SearchIn( path, resname, type.GetExt() );
}

CachedDirectoryImporter::CachedDirectoryImporter()
{
}

CachedDirectoryImporter::~CachedDirectoryImporter()
{
}

bool CachedDirectoryImporter::Open(const char *dir, const char *desc)
{
	if (!DirectoryImporter::Open(dir, desc))
		return false;

	Refresh();

	return true;
}

void CachedDirectoryImporter::Refresh()
{
	cache.clear();

	DirectoryIterator it(path);
	if (!it)
		return;

	char buf[_MAX_PATH];
	do {
		if (it.IsDirectory())
			continue;
		const char *name = it.GetName();
		strnlwrcpy(buf, name, _MAX_PATH, false);
		if (cache.find(buf) != cache.end()) {
			printMessage("CachedDirectoryImporter", "Duplicate '%s' files in '%s' directory", LIGHT_RED, buf, path);
		}
		cache[buf] = name;
	} while (++it);
}

static const char *ConstructFilename(const char* resname, const char* ext)
{
	static char buf[_MAX_PATH];
	strnlwrcpy(buf, resname, _MAX_PATH-4, false);
	strcat(buf, ".");
	strcat(buf, ext);
	return buf;
}

bool CachedDirectoryImporter::HasResource(const char* resname, SClass_ID type)
{
	const char* filename = ConstructFilename(resname, core->TypeExt(type));
	return (cache.find(filename) != cache.end());
}

bool CachedDirectoryImporter::HasResource(const char* resname, const ResourceDesc &type)
{
	const char* filename = ConstructFilename(resname, type.GetExt());
	return (cache.find(filename) != cache.end());
}

DataStream* CachedDirectoryImporter::GetResource(const char* resname, SClass_ID type)
{
	const char* filename = ConstructFilename(resname, core->TypeExt(type));
	std::map<std::string, std::string>::const_iterator it = cache.find(filename);
	if (it == cache.end())
		return NULL;
	char buf[_MAX_PATH];
	strcpy(buf, path);
	PathAppend(buf, it->second.c_str());
	return FileStream::OpenFile(buf);
}

DataStream* CachedDirectoryImporter::GetResource(const char* resname, const ResourceDesc &type)
{
	const char* filename = ConstructFilename(resname, type.GetExt());
	std::map<std::string, std::string>::const_iterator it = cache.find(filename);
	if (it == cache.end())
		return NULL;
	char buf[_MAX_PATH];
	strcpy(buf, path);
	PathAppend(buf, it->second.c_str());
	return FileStream::OpenFile(buf);
}

#include "plugindef.h"

GEMRB_PLUGIN(0xAB4534, "Directory Importer")
PLUGIN_CLASS(PLUGIN_RESOURCE_DIRECTORY, DirectoryImporter)
PLUGIN_CLASS(PLUGIN_RESOURCE_CACHEDDIRECTORY, CachedDirectoryImporter)
END_PLUGIN()
