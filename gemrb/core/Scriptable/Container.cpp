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

#include "Scriptable/Container.h"

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
#include "Sprite2D.h"
#include "SpriteCover.h"
#include "TileMap.h"
#include "Video.h"
#include "GameScript/GSUtils.h"
#include "GUI/GameControl.h"

#include <cassert>
#include <cmath>

#define YESNO(x) ( (x)?"Yes":"No")

Container::Container(void)
	: Highlightable( ST_CONTAINER )
{
	Type = 0;
	LockDifficulty = 0;
	Flags = 0;
	TrapDetectionDiff = 0;
	TrapRemovalDiff = 0;
	Trapped = 0;
	TrapDetected = 0;
	inventory.SetInventoryType(INVENTORY_HEAP);
	// NULL should be 0 for this
	memset (groundicons, 0, sizeof(groundicons) );
	groundiconcover = 0;
}

void Container::FreeGroundIcons()
{
	Video* video = core->GetVideoDriver();

	for (int i = 0;i<MAX_GROUND_ICON_DRAWN;i++) {
		if (groundicons[i]) {
			video->FreeSprite( groundicons[i] );
			groundicons[i]=NULL;
		}
	}
	delete groundiconcover;
	groundiconcover = 0;
}

Container::~Container()
{
	FreeGroundIcons();
}

void Container::DrawPile(bool highlight, Region screen, Color tint)
{
	Video* video = core->GetVideoDriver();
	CreateGroundIconCover();
	for (int i = 0;i<MAX_GROUND_ICON_DRAWN;i++) {
		if (groundicons[i]) {
			//draw it with highlight
			video->BlitGameSprite(groundicons[i],
				screen.x + Pos.x, screen.y + Pos.y,
				BLIT_TINTED | (highlight ? 0:BLIT_NOSHADOW),
				tint, groundiconcover);
		}
	}
}

// create the SpriteCover for the groundicons
void Container::CreateGroundIconCover()
{
	int xpos = 0;
	int ypos = 0;
	int width = 0;
	int height = 0;

	int i; //msvc6.0
	for (i = 0;i<MAX_GROUND_ICON_DRAWN;i++) {
		if (groundicons[i]) {
			Sprite2D& spr = *groundicons[i];
			if (xpos < spr.XPos) {
				width += spr.XPos - xpos;
				xpos = spr.XPos;
			}
			if (ypos < spr.YPos) {
				height += spr.YPos - ypos;
				ypos = spr.YPos;
			}
			if (width-xpos < spr.Width-spr.XPos) {
				width = spr.Width-spr.XPos+xpos;
			}
			if (height-ypos < spr.Height-spr.YPos) {
				height = spr.Height-spr.YPos+ypos;
			}
		}
	}

	if (!groundiconcover ||
		!groundiconcover->Covers(Pos.x, Pos.y, xpos, ypos, width, height))
	{
		delete groundiconcover;
		groundiconcover = 0;
		if (width*height > 0) {
			groundiconcover = GetCurrentArea()->BuildSpriteCover
				(Pos.x, Pos.y, xpos, ypos, width, height, WantDither());
		}
	}

#ifndef NDEBUG
	// TODO: remove this checking code eventually
	for (i = 0;i<MAX_GROUND_ICON_DRAWN;i++) {
		if (groundicons[i]) {
			Sprite2D& spr = *groundicons[i];
			assert(groundiconcover->Covers(Pos.x, Pos.y, spr.XPos, spr.YPos, spr.Width, spr.Height));
		}
	}
#endif
}

void Container::SetContainerLocked(bool lock)
{
	if (lock) {
		Flags|=CONT_LOCKED;
	} else {
		Flags&=~CONT_LOCKED;
	}
}

//This function doesn't exist in the original IE, destroys a container
//turning it to a ground pile
void Container::DestroyContainer()
{
	//it is already a groundpile?
	if (Type == IE_CONTAINER_PILE)
		return;
	Type = IE_CONTAINER_PILE;
	RefreshGroundIcons();
	//probably we should stop the script or trigger it, whatever
}

//Takes an item from the container's inventory and returns its pointer
CREItem *Container::RemoveItem(unsigned int idx, unsigned int count)
{
	CREItem *ret = inventory.RemoveItem(idx, count);
	//we just took the 3. or less item, groundpile changed
	if ((Type == IE_CONTAINER_PILE) && (inventory.GetSlotCount()<3)) {
		RefreshGroundIcons();
	}
	return ret;
}

//Adds an item to the container's inventory
//containers always have enough capacity (so far), thus we always return 2
int Container::AddItem(CREItem *item)
{
	inventory.AddItem(item);
	//we just added a 3. or less item, groundpile changed
	if ((Type == IE_CONTAINER_PILE) && (inventory.GetSlotCount()<4)) {
		RefreshGroundIcons();
	}
	return 2;
}

void Container::RefreshGroundIcons()
{
	int i = inventory.GetSlotCount();
	if (i>MAX_GROUND_ICON_DRAWN)
		i = MAX_GROUND_ICON_DRAWN;
	FreeGroundIcons();
	while (i--) {
		CREItem *slot = inventory.GetSlotItem(i); //borrowed reference
		Item *itm = gamedata->GetItem( slot->ItemResRef ); //cached reference
		if (!itm) continue;
		//well, this is required in PST, needs more work if some other
		//game is broken by not using -1,0
		groundicons[i] = gamedata->GetBAMSprite( itm->GroundIcon, 0, 0 );
		gamedata->FreeItem( itm, slot->ItemResRef ); //decref
	}
}

//used for ground piles
int Container::WantDither()
{
	//if pile is highlighted, always dither it
	if (Highlight) {
		return 2; //dither me if you want
	}
	//if pile isn't highlighted, dither it if the polygon wants
	return 1;
}

int Container::IsOpen() const
{
	if (Flags&CONT_LOCKED) {
		return false;
	}
	return true;
}

void Container::TryPickLock(Actor *actor)
{
	core->PlaySound(DS_PICKLOCK);
	if (LockDifficulty == 100) {
		if (OpenFail != (ieDword)-1) {
			displaymsg->DisplayStringName(OpenFail, 0xbcefbc, actor, IE_STR_SOUND|IE_STR_SPEECH);
		} else {
			displaymsg->DisplayConstantStringName(STR_CONT_NOPICK, 0xbcefbc, actor);
		}
		return;
	}
	if (actor->GetStat(IE_LOCKPICKING)<LockDifficulty) {
		displaymsg->DisplayConstantStringName(STR_LOCKPICK_FAILED, 0xbcefbc, actor);
		AddTrigger(TriggerEntry(trigger_picklockfailed, actor->GetGlobalID()));
		return;
	}
	SetContainerLocked(false);
	displaymsg->DisplayConstantStringName(STR_LOCKPICK_DONE, 0xd7d7be, actor);
	AddTrigger(TriggerEntry(trigger_unlocked, actor->GetGlobalID()));
	ImmediateEvent();
	int xp = actor->CalculateExperience(XP_LOCKPICK, actor->GetXPLevel(1));
	Game *game = core->GetGame();
	game->ShareXP(xp, SX_DIVIDE);
}

void Container::TryBashLock(Actor *actor)
{
	//Get the strength bonus agains lock difficulty
	int str = actor->GetStat(IE_STR);
	int strEx = actor->GetStat(IE_STREXTRA);
	unsigned int bonus = core->GetStrengthBonus(2, str, strEx); //BEND_BARS_LIFT_GATES
	unsigned int roll = actor->LuckyRoll(1, 10, bonus, 0);

	if(roll < LockDifficulty || LockDifficulty == 100) {
		displaymsg->DisplayConstantStringName(STR_CONTBASH_FAIL, 0xbcefbc, actor);
		return;
	}

	displaymsg->DisplayConstantStringName(STR_CONTBASH_DONE, 0xd7d7be, actor);
	SetContainerLocked(false);
	//Is this really useful ?
	AddTrigger(TriggerEntry(trigger_unlocked, actor->GetGlobalID()));
	ImmediateEvent();
}

void Container::DebugDump() const
{
	print( "Debugdump of Container %s\n", GetScriptName() );
	print( "Container Global ID: %d\n", GetGlobalID());
	print( "Position: %d.%d\n", Pos.x, Pos.y);
	print( "Type: %d, Locked: %s, LockDifficulty: %d\n", Type, YESNO(Flags&CONT_LOCKED), LockDifficulty );
	print( "Flags: %d, Trapped: %s, Detected: %d\n", Flags, YESNO(Trapped), TrapDetected );
	print( "Trap detection: %d%%, Trap removal: %d%%\n", TrapDetectionDiff,
		TrapRemovalDiff );
	const char *name = "NONE";
	if (Scripts[0]) {
		name = Scripts[0]->GetName();
	}
	print( "Script: %s, Key: %s\n", name, KeyResRef );
	// FIXME: const_cast
	const_cast<Inventory&>(inventory).dump();
}

bool Container::TryUnlock(Actor *actor) {
	if (!(Flags&CONT_LOCKED)) return true;

	return Highlightable::TryUnlock(actor, false);
}

