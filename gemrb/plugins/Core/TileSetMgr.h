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

#ifndef TILESETMGR_H
#define TILESETMGR_H

#include "Plugin.h"
#include "Resource.h"
#include "Tile.h"

/**
 * Base clase for tile set plugins
 */
class GEM_EXPORT TileSetMgr : public Resource {
public:
	const static TypeID ID;
	TileSetMgr(void);
	virtual ~TileSetMgr(void);
	/**
	 * Returns a @ref Tile from the paramaters
	 *
	 * This function can't be overridden and is implemented
	 * in terms of the private virtual function.
	 */
	Tile* GetTile(unsigned short* indexes, int count,
		unsigned short* secondary = NULL);
private:
	/**
	 * Returns a sprite representing a tile.
	 *
	 * @param[in] index Index of the tile to return.
	 */
	virtual Sprite2D* GetTile(int index) = 0;
};

#endif
