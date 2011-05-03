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
 */

/**
 * @file GameControl.h
 * Declares GameControl widget which is responsible for displaying areas,
 * interacting with PCs, NPCs and the rest of the game world.
 * @author The GemRB Project
 */

#ifndef GAMECONTROL_H
#define GAMECONTROL_H

#include "GUI/Control.h"

#include "exports.h"

#include "Dialog.h"
#include "Interface.h"
#include "Map.h"

class GameControl;
class Window;
class DialogHandler;

//dialog flags
#define DF_IN_DIALOG      1
#define DF_TALKCOUNT      2
#define DF_UNBREAKABLE    4
#define DF_FREEZE_SCRIPTS 8
#define DF_INTERACT       16
#define DF_IN_CONTAINER   32
#define DF_OPENCONTINUEWINDOW 64
#define DF_OPENENDWINDOW 128

//screen flags
// !!! Keep these synchronized with GUIDefines.py !!!
#define SF_DISABLEMOUSE  1  //no mouse cursor
#define SF_CENTERONACTOR 2  //
#define SF_ALWAYSCENTER  4
#define SF_GUIENABLED    8  //
#define SF_LOCKSCROLL    16 //don't scroll
#define SF_CUTSCENE      32 //don't push new actions onto the action queue
#define SF_TRACKING      64 //draw blue arrows on the edge for creatures

// target modes and types
// !!! Keep these synchronized with GUIDefines.py !!!
#define TARGET_MODE_NONE    0
#define TARGET_MODE_TALK    1
#define TARGET_MODE_ATTACK  2
#define TARGET_MODE_CAST    3
#define TARGET_MODE_DEFEND  4
#define TARGET_MODE_PICK    5

/*
#define TARGET_SELECT       16
#define TARGET_NO_DEAD      32
#define TARGET_POINT        64
#define TARGET_NO_HIDDEN    128
#define TARGET_TYPE_NONE    0x000
#define TARGET_NO_ALLY      0x100 //0x100
#define TARGET_NO_ENEMY     0x200 //0x200
#define TARGET_NO_NEUTRAL   0x400
#define TARGET_NO_SELF      0x800
#define TARGET_TYPE_ALL      0 //(TARGET_TYPE_ALLY | TARGET_TYPE_ENEMY | TARGET_TYPE_NEUTRAL)
*/

/**
 * @class GameControl
 * Widget displaying areas, where most of the game 'happens'.
 * It allows for interacting with PCs, NPCs and the rest of the world.
 * It's also a very core part of GemRB, as some processes are driven from it.
 * It's always assigned Control index 0.
 */

class GEM_EXPORT GameControl : public Control {
public:
	GameControl(void);
	~GameControl(void);
public:
	/** Draws the Control on the Output Display */
	void Draw(unsigned short x, unsigned short y);
	/** Sets the Text of the current control */
	int SetText(const char* string, int pos = 0);
	/** Sets multiple quicksaves flag*/
	static void MultipleQuickSaves(int arg);
	void SetTracker(Actor *actor, ieDword dist);
private:
	ieDword lastActorID;
	ieDword trackerID;
	ieDword distance;  //tracking distance
	std::vector< Actor*> highlighted;
	bool DrawSelectionRect;
	bool MouseIsDown;
	bool DoubleClick;
	Region SelectionRect;
	short StartX, StartY;
	//int action;
#ifdef TOUCHSCREEN
	bool touched; // true, if player touched screen (left button down and hold)
#endif
public:
	Door* overDoor;
	Container* overContainer;
	InfoPoint* overInfoPoint;

	// allow targetting allies, enemies and/or neutrals (bitmask)
	int target_types;
private:
	// currently selected targetting type, such as talk, attack, cast, ...
	// private to enforce proper cursor changes
	int target_mode;
	unsigned char lastCursor;
	short moveX, moveY;
	int numScrollCursor;
	bool scrolling;
	unsigned short lastMouseX, lastMouseY;
	int DebugFlags;
	Point pfs;
	PathNode* drawPath;
	unsigned long AIUpdateCounter;
	int ScreenFlags;
	int DialogueFlags;
	char *DisplayText;
	unsigned int DisplayTextTime;
	bool EnableRunning;
public: //Events
	/** Key Press Event */
	void OnKeyPress(unsigned char Key, unsigned short Mod);
	/** Key Release Event */
	void OnKeyRelease(unsigned char Key, unsigned short Mod);
	/** Mouse Over Event */
	void OnMouseOver(unsigned short x, unsigned short y);
	/** Global Mouse Move Event */
	void OnGlobalMouseMove(unsigned short x, unsigned short y);
	/** Mouse Button Down */
	void OnMouseDown(unsigned short x, unsigned short y, unsigned short Button,
		unsigned short Mod);
	/** Mouse Button Up */
	void OnMouseUp(unsigned short x, unsigned short y, unsigned short Button,
		unsigned short Mod);
	/** Special Key Press */
	void OnSpecialKeyPress(unsigned char Key);
	void DisplayTooltip();
	void UpdateScrolling();
	void SetTargetMode(int mode);
	int GetTargetMode() { return target_mode; }
	void SetScreenFlags(int value, int mode);
	void SetDialogueFlags(int value, int mode);
	int GetScreenFlags() { return ScreenFlags; }
	int GetDialogueFlags() { return DialogueFlags; }
	/** this function is called from the area when autosave is needed */
	void AutoSave();
	void SetDisplayText(char *text, unsigned int time);
	void SetDisplayText(ieStrRef text, unsigned int time);
	/* centers viewport to the points specified */
	void Center(unsigned short x, unsigned short y);
private:
	/** this function is called when the user presses 'q' (or equivalent) */
	void QuickSave();
	/** this function safely retrieves an Actor by ID */
	Actor *GetActorByGlobalID(ieDword ID);
	void CalculateSelection(const Point &p);
	void ResizeDel(Window* win, int type);
	void ResizeAdd(Window* win, int type);
	void HandleWindowHide(const char *WindowName, const char *WindowPosition);
	void HandleWindowReveal(const char *WindowName, const char *WindowPosition);
	void ReadFormations();
	/** Draws an arrow on the edge of the screen based on the point (points at offscreen actors) */
	void DrawArrowMarker(const Region &screen, Point p, const Region &viewport);

private:
	unsigned char LeftCount, BottomCount, RightCount, TopCount;
	Actor *user;     //the user of item or spell
public:
	DialogHandler *dialoghandler;
	//the name of the spell to cast
	ieResRef spellName;
	//using spell or item
	int spellOrItem; // -1 = item, otherwise the spell type
	//the user of spell or item
	Actor *spellUser;
	int spellSlot, spellIndex; //or inventorySlot/itemHeader
	int spellCount; //multiple targeting
public:
	/** Selects one or all PC */
	void SelectActor(int whom, int type = -1);
	void SetLastActor(Actor *actor, Actor *prevActor);
	void SetCutSceneMode(bool active);
	int HideGUI();
	int UnhideGUI();
	void TryToAttack(Actor *source, Actor *target);
	void TryToCast(Actor *source, const Point &p);
	void TryToCast(Actor *source, Actor *target);
	void TryToDefend(Actor *source, Actor *target);
	void TryToTalk(Actor *source, Actor *target);
	void TryToPick(Actor *source, Actor *tgt);
	void TryToPick(Actor *source, Door *tgt);
	void TryToPick(Actor *source, Container *tgt);
	void TryToDisarm(Actor *source, InfoPoint *tgt);
	void PerformActionOn(Actor *actor);
	void ResetTargetMode();
	void UpdateTargetMode();

	// returns the default cursor fitting the targeting mode 
	int GetDefaultCursor() const;
	//containers
	int GetCursorOverContainer(Container *overContainer) const;
	void HandleContainer(Container *container, Actor *actor);
	//doors
	int GetCursorOverDoor(Door *overDoor) const;
	void HandleDoor(Door *door, Actor *actor);
	//infopoints
	int GetCursorOverInfoPoint(InfoPoint *overInfoPoint) const;
	bool HandleActiveRegion(InfoPoint *trap, Actor *actor, Point &p);

	Point GetFormationOffset(ieDword formation, ieDword pos);
	void MoveToPointFormation(Actor *actor, unsigned int pos, Point src, Point p);
	/** calls MoveToPoint or RunToPoint */
	void CreateMovement(Actor *actor, const Point &p);
	/** Displays a string over an object */
	void DisplayString(Scriptable* target);
	/** Displays a string on screen */
	void DisplayString(const Point &p, const char *Text);
	Actor *GetLastActor();
	/** changes map to the current PC */
	void ChangeMap(Actor *pc, bool forced);
	/** Returns game screenshot, with or without GUI controls */
	Sprite2D* GetScreenshot( bool show_gui = 0 );
	/** Returns current area preview for saving a game */
	Sprite2D* GetPreview();
	/** Returns PC portrait for a currently running game */
	Sprite2D* GetPortraitPreview(int pcslot);
	/** Sets up targeting with spells or items */
	void SetupItemUse(int slot, int header, Actor *actor, int targettype, int cnt);
	/** Page is the spell type + spell level info */
	void SetupCasting(ieResRef spellname, int type, int level, int slot, Actor *actor, int targettype, int cnt);
	bool SetEvent(int eventType, EventHandler handler);
};

#endif
