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
#include "System/String.h"

CacheImporter::CacheImporter(void)
{
	description = NULL;
}

CacheImporter::~CacheImporter(void)
{
	free(description);
}

DirectoryImporter::DirectoryImporter()
{
}

DirectoryImporter::~DirectoryImporter()
{
}

bool CacheImporter::Open(const char *dir, const char *desc)
{
	if (!dir_exists(dir))
		return false;

	free(description);
	description = strdup(desc);
	strcpy(path, dir);

	return true;
}

CacheDirectory::CacheDirectory()
{
}

CacheDirectory::~CacheDirectory()
{
}

bool CacheDirectory::Open(const char *dir, const char *desc)
{
	source = new CacheImporter();
	source->cache.init(4 * 1024, 256);
	return source->Open(dir, desc);
}

ResourceSource* CacheDirectory::GetSource()
{
	return source.get();
}

DataStream* CacheDirectory::CreateFile(const char* resname, bool force)
{
	if (!force && source->cache.has(resname))
		return NULL;
	char path[_MAX_PATH];
	char filename[_MAX_PATH];
	strnlwrcpy(filename, resname, _MAX_PATH);
	PathJoin(path, source->path, resname, NULL);

	FileStream* file = new FileStream();
	if (!file->Create(path)) {
		delete file;
		return NULL;
	}

	source->cache.set(filename, filename);
	return file;
}

DataStream* CacheDirectory::OpenFile(const char* resname)
{
	char filename[_MAX_PATH];
	strnlwrcpy(filename, resname, _MAX_PATH, false);
	const std::string *s = source->cache.get(filename);
	if (!s)
		return NULL;
	char buf[_MAX_PATH];
	strcpy(buf, source->path);
	PathAppend(buf, s->c_str());
	return FileStream::OpenFile(buf);
}

class Unlinker {
public:
	Unlinker(const char* path) : path(path) {}
	void operator() (const std::string& filename)
	{
		char file[_MAX_PATH];
		PathJoin(file, path, filename.c_str(), NULL);
		//unlink(file);
		printMessage("UNLIKER", "%s", RED, file);
	}
private:
	const char* path;
};

void CacheDirectory::EmptyCache()
{
	source->cache.for_each(Unlinker(source->path));
	source->cache.clear();
	source->cache.init(4 * 1024, 256);
}

bool DirectoryImporter::Open(const char *dir, const char *desc)
{
	if (!CacheImporter::Open(dir, desc))
		return false;

	Refresh();

	return true;
}

void DirectoryImporter::Refresh()
{
	cache.clear();

	DirectoryIterator it(path);
	if (!it)
		return;

	unsigned int count = 0;
	do {
		if (it.IsDirectory())
			continue;
		count++;
	} while (++it);

	// limit to 4k buckets
	// less than 1% of the bg2+fixpack override are of bucket length >4
	cache.init(count > 4 * 1024 ? 4 * 1024 : count, count);

	it.Rewind();

	char buf[_MAX_PATH];
	do {
		if (it.IsDirectory())
			continue;
		const char *name = it.GetName();
		strnlwrcpy(buf, name, _MAX_PATH, false);
		if (cache.set(buf, name)) {
			printMessage("CachedDirectoryImporter", "Duplicate '%s' files in '%s' directory", LIGHT_RED, buf, path);
		}
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

bool CacheImporter::HasResource(const char* resname, SClass_ID type)
{
	const char* filename = ConstructFilename(resname, core->TypeExt(type));
	return cache.has(filename);
}

bool CacheImporter::HasResource(const char* resname, const ResourceDesc &type)
{
	const char* filename = ConstructFilename(resname, type.GetExt());
	return cache.has(filename);
}

DataStream* CacheImporter::GetResource(const char* resname, SClass_ID type)
{
	const char* filename = ConstructFilename(resname, core->TypeExt(type));
	const std::string *s = cache.get(filename);
	if (!s)
		return NULL;
	char buf[_MAX_PATH];
	strcpy(buf, path);
	PathAppend(buf, s->c_str());
	return FileStream::OpenFile(buf);
}

DataStream* CacheImporter::GetResource(const char* resname, const ResourceDesc &type)
{
	const char* filename = ConstructFilename(resname, type.GetExt());
	const std::string *s = cache.get(filename);
	if (!s)
		return NULL;
	char buf[_MAX_PATH];
	strcpy(buf, path);
	PathAppend(buf, s->c_str());
	return FileStream::OpenFile(buf);
}

#include "plugindef.h"

GEMRB_PLUGIN(0xAB4534, "Directory Importer")
PLUGIN_CLASS(PLUGIN_RESOURCE_DIRECTORY, DirectoryImporter)
PLUGIN_CLASS(PLUGIN_CACHE_DIRECTORY, CacheDirectory)
END_PLUGIN()
