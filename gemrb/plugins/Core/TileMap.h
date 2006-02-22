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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/TileMap.h,v 1.38 2006/02/22 18:38:20 avenger_teambg Exp $
 *
 */

#ifndef TILEMAP_H
#define TILEMAP_H

#include "TileOverlay.h"
#include "Polygon.h"
#include "GameScript.h"

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

//special container types
#define IE_CONTAINER_PILE   4

class GEM_EXPORT TileMap {
private:
	std::vector< TileOverlay*> overlays;
	std::vector< Door*> doors;
	std::vector< Container*> containers;
	std::vector< InfoPoint*> infoPoints;
	std::vector< TileObject*> tiles;
	bool LargeMap;
public:
	TileMap(void);
	~TileMap(void);

	Door* AddDoor(const char* ID, const char* Name, unsigned int Flags,
		int ClosedIndex, unsigned short* indices, int count,
		Gem_Polygon* open, Gem_Polygon* closed);
	Door* GetDoor(Point &position);
	Door* GetDoor(unsigned int idx);
	Door* GetDoor(const char* Name);
	unsigned int GetDoorCount() { return doors.size(); }

	/* type is an optional filter for container type*/
	void AddContainer(Container *c);
	Container* GetContainer(Point &position, int type=-1);
	Container* GetContainer(const char* Name);
	Container* GetContainer(unsigned int idx);
	/* cleans up empty heaps, returns 1 if container removed*/
	int CleanupContainer(Container *container);
	unsigned int GetContainerCount() { return containers.size(); }

	InfoPoint* AddInfoPoint(const char* Name, unsigned short Type,
		Gem_Polygon* outline);
	InfoPoint* GetInfoPoint(Point &position);
	InfoPoint* GetInfoPoint(const char* Name);
	InfoPoint* GetInfoPoint(unsigned int idx);
	unsigned int GetInfoPointCount() { return infoPoints.size(); }

	TileObject* AddTile(const char* ID, const char* Name, unsigned int Flags, 
		unsigned short* openindices, int opencount,unsigned short* closeindices, int closecount);
	TileObject* GetTile(unsigned int idx);
	TileObject* GetTile(const char* Name);
	unsigned int GetTileCount() { return tiles.size(); }

	void AddOverlay(TileOverlay* overlay);
	void DrawOverlays(Region screen);
	void DrawFogOfWar(ieByte* explored_mask, ieByte* visible_mask, Region viewport);
public:
	int XCellCount, YCellCount;
};

#endif
