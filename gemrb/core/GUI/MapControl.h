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

/**
 * @file MapControl.h
 * Declares MapControl, widget for displaying current area map
 */

class MapControl;

#ifndef MAPCONTROL_H
#define MAPCONTROL_H

#include "GUI/Control.h"

#include "exports.h"
#include "Interface.h"

// !!! Keep these synchronized with GUIDefines.py !!!
#define IE_GUI_MAP_ON_PRESS     	0x09000000
#define IE_GUI_MAP_ON_RIGHT_PRESS	0x09000005
#define IE_GUI_MAP_ON_DOUBLE_PRESS	0x09000008


/**
 * @class MapControl
 * Widget displaying current area map, with a viewport rectangle
 * and PCs' ground circles
 */

class GEM_EXPORT MapControl : public Control {
public:
	int ScrollX, ScrollY;
	int NotePosX, NotePosY;
	unsigned short lastMouseX, lastMouseY;
	bool mouseIsDown;
	bool mouseIsDragging;
	bool convertToGame;
	// Small map bitmap
	Sprite2D* MapMOS;
	// current map
	Map *MyMap;
	// map flags
	Sprite2D *Flag[8];
	// The MapControl can set the text of this label directly
	Control *LinkedLabel;
	// Size of big map (area) in pixels
	short MapWidth, MapHeight;
	// Size of area viewport. FIXME: hack!
	short ViewWidth, ViewHeight;
	short XCenter, YCenter;
	EventHandler MapControlOnPress;
	EventHandler MapControlOnRightPress;
	EventHandler MapControlOnDoublePress;

	MapControl(void);
	~MapControl(void);
	/** redraws the control after its associated variable has changed */
	void RedrawMapControl(const char *VariableName, unsigned int Sum);
	/** Draws the Control on the Output Display */
	void Draw(unsigned short XWin, unsigned short YWin);
	void DrawFog(unsigned short XWin, unsigned short YWin);
	/** Compute parameters after changes in control's or screen geometry */
	void Realize();
	/** Sets the Text of the current control */
	int SetText(const char* /*string*/, int /*pos*/) { return 0; }

	/** Key Press Event */
	void OnKeyPress(unsigned char Key, unsigned short Mod);
	/** Mouse Over Event */
	void OnMouseOver(unsigned short x, unsigned short y);
	/** Mouse Leave Event */
	void OnMouseLeave(unsigned short x, unsigned short y);
	/** Mouse Button Down */
	void OnMouseDown(unsigned short x, unsigned short y, unsigned short Button,
		unsigned short Mod);
	/** Mouse Button Up */
	void OnMouseUp(unsigned short x, unsigned short y, unsigned short Button,
		unsigned short Mod);
	/** Key Release Event */
	void OnKeyRelease(unsigned char Key, unsigned short Mod);
	/** Special Key Press */
	void OnSpecialKeyPress(unsigned char Key);
	/** Set handler for specified event */
	bool SetEvent(int eventType, EventHandler handler);
private:
	/** Call event handler on click */
	void ClickHandle(unsigned short Button);
	/** Move viewport */
	void ViewHandle(unsigned short x, unsigned short y);
};

#endif
