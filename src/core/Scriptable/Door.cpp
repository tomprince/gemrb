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
 */

#include "Scriptable/Door.h"

#include "strrefs.h"
#include "win32def.h"

#include "Audio.h"
#include "DisplayMessage.h"
#include "Game.h"
#include "GameData.h"
#include "Interface.h"
#include "Item.h"
#include "Map.h"
#include "Projectile.h"
#include "Spell.h"
#include "SpriteCover.h"
#include "TileMap.h"
#include "Video.h"
#include "GameScript/GSUtils.h"
#include "GUI/GameControl.h"
#include "Scriptable/InfoPoint.h"

#include <cassert>
#include <cmath>

#define YESNO(x) ( (x)?"Yes":"No")

Door::Door(TileOverlay* Overlay)
	: Highlightable( ST_DOOR )
{
	tiles = NULL;
	tilecount = 0;
	Flags = 0;
	open = NULL;
	closed = NULL;
	open_ib = NULL;
	oibcount = 0;
	closed_ib = NULL;
	cibcount = 0;
	OpenSound[0] = 0;
	CloseSound[0] = 0;
	LockSound[0] = 0;
	UnLockSound[0] = 0;
	overlay = Overlay;
	LinkedInfo[0] = 0;
	OpenStrRef = (ieDword) -1;
}

Door::~Door(void)
{
	if (Flags&DOOR_OPEN) {
		if (closed) {
			delete( closed );
		}
	} else {
		if (open) {
			delete( open );
		}
	}
	if (tiles) {
		free( tiles );
	}
	if (open_ib) {
		free( open_ib );
	}
	if (closed_ib) {
		free( closed_ib );
	}
}

void Door::ImpedeBlocks(int count, Point *points, unsigned char value)
{
	for(int i = 0;i<count;i++) {
		unsigned char tmp = area->GetInternalSearchMap(points[i].x, points[i].y) & PATH_MAP_NOTDOOR;
		area->SetInternalSearchMap(points[i].x, points[i].y, tmp|value);
	}
}

void Door::UpdateDoor()
{
	if (Flags&DOOR_OPEN) {
		outline = open;
	} else {
		outline = closed;
	}
	// update the Scriptable position
	Pos.x = outline->BBox.x + outline->BBox.w/2;
	Pos.y = outline->BBox.y + outline->BBox.h/2;

	unsigned char oval, cval;
	oval = PATH_MAP_IMPASSABLE;
	if (Flags & DOOR_TRANSPARENT) {
		cval = PATH_MAP_DOOR_IMPASSABLE;
	}
	else {
		//both door flags are needed here, one for transparency the other
		//is for passability
		cval = PATH_MAP_DOOR_OPAQUE|PATH_MAP_DOOR_IMPASSABLE;
	}
	if (Flags &DOOR_OPEN) {
		ImpedeBlocks(cibcount, closed_ib, 0);
		ImpedeBlocks(oibcount, open_ib, cval);
	}
	else {
		ImpedeBlocks(oibcount, open_ib, 0);
		ImpedeBlocks(cibcount, closed_ib, cval);
	}

	InfoPoint *ip = area->TMap->GetInfoPoint(LinkedInfo);
	if (ip) {
		if (Flags&DOOR_OPEN) ip->Flags&=~INFO_DOOR;
		else ip->Flags|=INFO_DOOR;
	}
}

void Door::ToggleTiles(int State, int playsound)
{
	int i;
	int state;

	if (State) {
		state = !closedIndex;
		if (playsound && ( OpenSound[0] != '\0' ))
			core->GetAudioDrv()->Play( OpenSound );
	} else {
		state = closedIndex;
		if (playsound && ( CloseSound[0] != '\0' ))
			core->GetAudioDrv()->Play( CloseSound );
	}
	for (i = 0; i < tilecount; i++) {
		overlay->tiles[tiles[i]]->tileIndex = (ieByte) state;
	}

	//set door_open as state
	Flags = (Flags & ~DOOR_OPEN) | (State == !core->HasFeature(GF_REVERSE_DOOR) );
}

//this is the short name (not the scripting name)
void Door::SetName(const char* name)
{
	strnlwrcpy( ID, name, 8 );
}

void Door::SetTiles(unsigned short* Tiles, int cnt)
{
	if (tiles) {
		free( tiles );
	}
	tiles = Tiles;
	tilecount = cnt;
}

void Door::SetDoorLocked(int Locked, int playsound)
{
	if (Locked) {
		if (Flags & DOOR_LOCKED) return;
		Flags|=DOOR_LOCKED;
		if (playsound && ( LockSound[0] != '\0' ))
			core->GetAudioDrv()->Play( LockSound );
	}
	else {
		if (!(Flags & DOOR_LOCKED)) return;
		Flags&=~DOOR_LOCKED;
		if (playsound && ( UnLockSound[0] != '\0' ))
			core->GetAudioDrv()->Play( UnLockSound );
	}
}

int Door::IsOpen() const
{
	int ret = core->HasFeature(GF_REVERSE_DOOR);
	if (Flags&DOOR_OPEN) {
		ret = !ret;
	}
	return ret;
}

//also mark actors to fix position
bool Door::BlockedOpen(int Open, int ForceOpen)
{
	bool blocked;
	int count;
	Point *points;

	blocked = false;
	if (Open) {
		count = oibcount;
		points = open_ib;
	} else {
		count = cibcount;
		points = closed_ib;
	}
	//getting all impeded actors flagged for jump
	Region rgn;
	rgn.w = 16;
	rgn.h = 12;
	for(int i = 0;i<count;i++) {
		Actor** ab;
		rgn.x = points[i].x*16;
		rgn.y = points[i].y*12;
		unsigned char tmp = area->GetInternalSearchMap(points[i].x, points[i].y) & PATH_MAP_ACTOR;
		if (tmp) {
			int ac = area->GetActorInRect(ab, rgn, false);
			while(ac--) {
				if (ab[ac]->GetBase(IE_DONOTJUMP)) {
					continue;
				}
				ab[ac]->SetBase(IE_DONOTJUMP, DNJ_JUMP);
				blocked = true;
			}
			if (ab) {
				free(ab);
			}
		}
	}

	if ((Flags&DOOR_SLIDE) || ForceOpen) {
		return false;
	}
	return blocked;
}

void Door::SetDoorOpen(int Open, int playsound, ieDword ID)
{
	if (playsound) {
		//the door cannot be blocked when opening,
		//but the actors will be pushed
		//BlockedOpen will mark actors to be pushed
		if (BlockedOpen(Open,0) && !Open) {
			//clear up the blocking actors
			area->JumpActors(false);
			return;
		}
		area->JumpActors(true);
	}
	if (Open) {
		LastEntered = ID; //used as lastOpener

		// in PS:T, opening a door does not unlock it
		if (!core->HasFeature(GF_REVERSE_DOOR)) {
			SetDoorLocked(false,playsound);
		}
	} else {
		LastTriggerObject = LastTrigger = ID; //used as lastCloser
	}
	ToggleTiles(Open, playsound);
	//synchronising other data with the door state
	UpdateDoor();
	area->ActivateWallgroups(open_wg_index, open_wg_count, Flags&DOOR_OPEN);
	area->ActivateWallgroups(closed_wg_index, closed_wg_count, !(Flags&DOOR_OPEN));
	core->SetEventFlag(EF_TARGETMODE);
}

bool Door::TryUnlock(Actor *actor) {
	if (!(Flags&DOOR_LOCKED)) return true;

	// don't remove key in PS:T!
	bool removekey = !core->HasFeature(GF_REVERSE_DOOR) && Flags&DOOR_KEY;
	return Highlightable::TryUnlock(actor, removekey);
}

void Door::TryDetectSecret(int skill)
{
	if (Type != ST_DOOR) return;
	if (Visible()) return;
	if (skill > (signed)DiscoveryDiff) {
		Flags |= DOOR_FOUND;
		core->PlaySound(DS_FOUNDSECRET);
	}
}

// return true if the door isn't secret or if it is, but was already discovered
bool Door::Visible()
{
	return (!(Flags & DOOR_SECRET) || (Flags & DOOR_FOUND));
}

void Door::SetPolygon(bool Open, Gem_Polygon* poly)
{
	if (Open) {
		if (open)
			delete( open );
		open = poly;
	} else {
		if (closed)
			delete( closed );
		closed = poly;
	}
}

void Door::SetNewOverlay(TileOverlay *Overlay) {
	overlay = Overlay;
	ToggleTiles(IsOpen(), false);
}

void Highlightable::SetTrapDetected(int x)
{
	if(x == TrapDetected)
		return;
	TrapDetected = x;
	if(TrapDetected) {
		core->Autopause(AP_TRAP);
	}
}

void Highlightable::TryDisarm(Actor *actor)
{
	if (!Trapped || !TrapDetected) return;

	LastTriggerObject = LastTrigger = actor->GetGlobalID();
	int skill = actor->GetStat(IE_TRAPS);

	if (skill/2+core->Roll(1,skill/2,0)>TrapRemovalDiff) {
		LastDisarmed = actor->GetGlobalID();
		//trap removed
		Trapped = 0;
		displaymsg->DisplayConstantStringName(STR_DISARM_DONE, 0xd7d7be, actor);
		int xp = actor->CalculateExperience(XP_DISARM, actor->GetXPLevel(1));
		Game *game = core->GetGame();
		game->ShareXP(xp, SX_DIVIDE);
	} else {
		displaymsg->DisplayConstantStringName(STR_DISARM_FAIL, 0xd7d7be, actor);
		TriggerTrap(skill, LastTrigger);
	}
	ImmediateEvent();
}

void Door::TryPickLock(Actor *actor)
{
	if (LockDifficulty == 100) {
		if (OpenStrRef != (ieDword)-1) {
			displaymsg->DisplayStringName(OpenStrRef, 0xbcefbc, actor, IE_STR_SOUND|IE_STR_SPEECH);
		} else {
			displaymsg->DisplayConstantStringName(STR_DOOR_NOPICK, 0xbcefbc, actor);
		}
		return;
	}
	if (actor->GetStat(IE_LOCKPICKING)<LockDifficulty) {
		displaymsg->DisplayConstantStringName(STR_LOCKPICK_FAILED, 0xbcefbc, actor);
		LastPickLockFailed = actor->GetGlobalID();
		return;
	}
	SetDoorLocked( false, true);
	displaymsg->DisplayConstantStringName(STR_LOCKPICK_DONE, 0xd7d7be, actor);
	LastUnlocked = actor->GetGlobalID();
	ImmediateEvent();
	int xp = actor->CalculateExperience(XP_LOCKPICK, actor->GetXPLevel(1));
	Game *game = core->GetGame();
	game->ShareXP(xp, SX_DIVIDE);
}

void Door::TryBashLock(Actor *actor)
{
	//Get the strength bonus agains lock difficulty
	int str = actor->GetStat(IE_STR);
	int strEx = actor->GetStat(IE_STREXTRA);
	unsigned int bonus = core->GetStrengthBonus(2, str, strEx); //BEND_BARS_LIFT_GATES
	unsigned int roll = actor->LuckyRoll(1, 10, bonus, 0);

	if(roll < LockDifficulty || LockDifficulty == 100) {
		displaymsg->DisplayConstantStringName(STR_DOORBASH_FAIL, 0xbcefbc, actor);
		return;
	}

	displaymsg->DisplayConstantStringName(STR_DOORBASH_DONE, 0xd7d7be, actor);
	SetDoorLocked(false, true);
	//Is this really useful ?
	LastUnlocked = actor->GetGlobalID();
	ImmediateEvent();
}

void Door::DebugDump() const
{
	printf( "Debugdump of Door %s:\n", GetScriptName() );
	printf( "Door Global ID: %d\n", GetGlobalID());
	printf( "Position: %d.%d\n", Pos.x, Pos.y);
	printf( "Door Open: %s\n", YESNO(IsOpen()));
	printf( "Door Locked: %s\n", YESNO(Flags&DOOR_LOCKED));
	printf( "Door Trapped: %s\n", YESNO(Trapped));
	if (Trapped) {
		printf( "Trap Permanent: %s Detectable: %s\n", YESNO(Flags&DOOR_RESET), YESNO(Flags&DOOR_DETECTABLE) );
	}
	printf( "Secret door: %s (Found: %s)\n", YESNO(Flags&DOOR_SECRET),YESNO(Flags&DOOR_FOUND));
	const char *Key = GetKey();
	const char *name = "NONE";
	if (Scripts[0]) {
		name = Scripts[0]->GetName();
	}
	printf( "Script: %s, Key (%s) removed: %s, Dialog: %s\n", name, Key?Key:"NONE", YESNO(Flags&DOOR_KEY), Dialog );
}

