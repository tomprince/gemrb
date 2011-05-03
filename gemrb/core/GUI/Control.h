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
 * @file Control.h
 * Declares Control, root class for all widgets except of windows
 */

#ifndef CONTROL_H
#define CONTROL_H

#define IE_GUI_BUTTON		0
#define IE_GUI_PROGRESSBAR	1 //gemrb extension
#define IE_GUI_SLIDER		2
#define IE_GUI_EDIT		3
#define IE_GUI_TEXTAREA		5
#define IE_GUI_LABEL		6
#define IE_GUI_SCROLLBAR	7
#define IE_GUI_WORLDMAP         8 // gemrb extension
#define IE_GUI_MAP              9 // gemrb extension
#define IE_GUI_GAMECONTROL	128
#define IE_GUI_INVALID          255

#define IE_GUI_CONTROL_FOCUSED  0x80

//this is in the control ID
#define IGNORE_CONTROL 0x10000000

#include "RGBAColor.h"
#include "exports.h"
#include "ie_types.h"
#include "win32def.h"

#include "Callback.h"

class ControlAnimation;
class Sprite2D;
class Window;

/**
 * @class Control
 * Basic Control Object, also called widget or GUI element. Parent class for Labels, Buttons, etc.
 * Every GUI element except of a Window is a descendant of this class.
 */

class GEM_EXPORT Control {
public:
	Control();
	virtual ~Control();
	/** Draws the Control on the Output Display */
	virtual void Draw(unsigned short x, unsigned short y) = 0;
	/** Sets the Text of the current control */
	virtual int SetText(const char* string, int pos = 0) = 0;
	/** Sets the Tooltip text of the current control */
	int SetTooltip(const char* string);
	/** Displays the tooltip text, Worldmap handles this differently */
	virtual void DisplayTooltip();
	/** Variable length is 40-1 (zero terminator) */
	char VarName[MAX_VARIABLE_LENGTH];
	/** the value of the control to add to the variable */
	ieDword Value;
	/** various flags based on the control type */
	ieDword Flags;
	ControlAnimation* animation;
	Sprite2D* AnimPicture;

public: // Public attributes
	/** Defines the Control ID Number used for GUI Scripting */
	ieDword ControlID;
	/** X position of control relative to containing window */
	ieWord XPos;
	/** Y position of control relative to containing window */
	ieWord YPos;
	/** Width of control */
	ieWord Width;
	/** Height of control */
	ieWord Height;
	/** Type of control */
	ieByte ControlType;
	/** Text to display as a tooltip when the mouse cursor hovers
	 * for some time over the control */
	char* Tooltip;
	/** Focused Control */
	bool hasFocus;
	/** If true, control is redrawn during next call to gc->DrawWindows.
	 * Then it's set back to false. */
	bool Changed;
	/** True if we are currently in an event handler */
	bool InHandler;
	/** Owner Window */
	Window* Owner;
	/** Attached Scroll Bar Pointer*/
	Control* sb;
public: //Events
	/** Reset/init event handler */
	void ResetEventHandler(EventHandler handler);
	/** Returns the Owner */
	Window *GetOwner() const { return Owner; }
	/** Set the Flags */
	int SetFlags(int arg_flags, int opcode);
	/** Set handler for specified event. Override in child classes */
	virtual bool SetEvent(int eventType, EventHandler handler) = 0;
	/** Run specified handler, it may return error code */
	int RunEventHandler(EventHandler handler);
	/** Key Press Event */
	virtual void OnKeyPress(unsigned char Key, unsigned short Mod);
	/** Key Release Event */
	virtual void OnKeyRelease(unsigned char Key, unsigned short Mod);
	/** Mouse Enter Event */
	virtual void OnMouseEnter(unsigned short x, unsigned short y);
	/** Mouse Leave Event */
	virtual void OnMouseLeave(unsigned short x, unsigned short y);
	/** Mouse Over Event */
	virtual void OnMouseOver(unsigned short x, unsigned short y);
	/** Mouse Button Down */
	virtual void OnMouseDown(unsigned short x, unsigned short y,
		unsigned short Button, unsigned short Mod);
	/** Mouse Button Up */
	virtual void OnMouseUp(unsigned short x, unsigned short y,
		unsigned short Button, unsigned short Mod);
	/** Special Key Press */
	virtual void OnSpecialKeyPress(unsigned char Key);
	virtual bool IsPixelTransparent(unsigned short /*x*/, unsigned short /*y*/) {
		return false;
	}
	/** Sets the animation picture ref */
	void SetAnimPicture(Sprite2D* Picture);
	/** Sets the Scroll Bar Pointer */
	int SetScrollBar(Control* ptr);
};

#endif
