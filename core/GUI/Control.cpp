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

#include "GUI/Control.h"

#include "GUI/Window.h"

#include "win32def.h"

#include "ControlAnimation.h"
#include "Interface.h"
#include "ScriptEngine.h"
#include "Video.h"

#include <cstdio>
#include <cstring>

Control::Control()
{
	hasFocus = false;
	Changed = true;
	InHandler = false;
	VarName[0] = 0;
	Value = 0;
	Flags = 0;
	Tooltip = NULL;
	Owner = NULL;
	XPos = 0;
	YPos = 0;

	sb = NULL;
	animation = NULL;
	AnimPicture = NULL;
	ControlType = IE_GUI_INVALID;
}

Control::~Control()
{
	if (InHandler) {
		printMessage("Control","Destroying control inside event handler, crash may occur!\n", LIGHT_RED);
	}
	core->DisplayTooltip( 0, 0, NULL );
	free (Tooltip);

	delete animation;

	core->GetVideoDriver()->FreeSprite(AnimPicture);
}

/** Sets the Tooltip text of the current control */
int Control::SetTooltip(const char* string)
{
	free(Tooltip);

	if ((string == NULL) || (string[0] == 0)) {
		Tooltip = NULL;
	} else {
		Tooltip = strdup (string);
	}
	Changed = true;
	return 0;
}

/** Sets the tooltip to be displayed on the screen now */
void Control::DisplayTooltip()
{
	if (Tooltip)
		core->DisplayTooltip( Owner->XPos + XPos + Width / 2, Owner->YPos + YPos + Height / 2, this );
	else
		core->DisplayTooltip( 0, 0, NULL );
}

void Control::ResetEventHandler(EventHandler handler)
{
	handler = NULL;
}

int Control::RunEventHandler(EventHandler handler)
{
	if (InHandler) {
		printMessage("Control","Nested event handlers are not supported!\n", YELLOW);
		return -1;
	}
	if (handler) {
		Window *wnd = Owner;
		if (!wnd) {
			return -1;
		}
		unsigned short WID = wnd->WindowID;
		unsigned short ID = (unsigned short) ControlID;
		InHandler = true;
		handler->call();
		if (!core->IsValidWindow(WID,wnd) ) {
			printMessage ("Control","Owner window destructed!\n", LIGHT_RED);
			return -1;
		}
		if (!wnd->IsValidControl(ID,this) ) {
			printMessage ("Control","Control destructed!\n", LIGHT_RED);
			return -1;
		}
		InHandler = false;
	}
	return 0;
}

/** Key Press Event */
void Control::OnKeyPress(unsigned char Key, unsigned short /*Mod*/)
{
	//printf("OnKeyPress: CtrlID = 0x%08X, Key = %c (0x%02hX)\n", (unsigned int) ControlID, Key, Key);
#ifdef ANDROID // mapping volume control to volume control keys on device, these keys must be set up in AndroidAppSettings.cfg
	switch(Key)	{
		case 'o': // volume down
		case 'p': // volume up
			int Ambients, Movie, Music, SFX, Voices;
			core->GetDictionary()->Lookup( "Volume Ambients", (ieDword&)Ambients );
			core->GetDictionary()->Lookup( "Volume Movie", (ieDword&)Movie );
			core->GetDictionary()->Lookup( "Volume Music", (ieDword&)Music );
			core->GetDictionary()->Lookup( "Volume SFX", (ieDword&)SFX );
			core->GetDictionary()->Lookup( "Volume Voices", (ieDword&)Voices );
			if (Key=='o') {
				if(Ambients>0) Ambients-=10; if(Ambients<0) Ambients=0;
				if(Movie>0) Movie-=10; if(Movie<0) Movie=0;
				if(Music>0) Music-=10; if(Music<0) Music=0;
				if(SFX>0) SFX-=10; if(SFX<0) SFX=0;
				if(Voices>0) Voices-=10; if(Voices<0) Voices=0;
			} else {
				if(Ambients<100) Ambients+=10; if(Ambients>100) Ambients=100;
				if(Movie<100) Movie+=10; if(Movie>100) Movie=100;
				if(Music<100) Music+=10; if(Music>100) Music=100;
				if(SFX<100) SFX+=10; if(SFX>100) SFX=100;
				if(Voices<100) Voices+=10; if(Voices>100) Voices=100;
			}
			core->GetDictionary()->SetAt( "Volume Ambients", Ambients );
			core->GetDictionary()->SetAt( "Volume Movie", Movie );
			core->GetDictionary()->SetAt( "Volume Music", Music );
			core->GetDictionary()->SetAt( "Volume SFX", SFX );
			core->GetDictionary()->SetAt( "Volume Voices", Voices );
			core->GetAudioDrv()->UpdateVolume();
			break;
	}
#else
(void)Key; // unused, fool the compiler
#endif
}

/** Key Release Event */
void Control::OnKeyRelease(unsigned char /*Key*/, unsigned short /*Mod*/)
{
	//printf( "OnKeyRelease: CtrlID = 0x%08X, Key = %c (0x%02hX)\n", (unsigned int) ControlID, Key, Key );
}

/** Mouse Enter Event */
void Control::OnMouseEnter(unsigned short /*x*/, unsigned short /*y*/)
{
//	printf("OnMouseEnter: CtrlID = 0x%08X, x = %hd, y = %hd\n", (unsigned int) ControlID, x, y);
}

/** Mouse Leave Event */
void Control::OnMouseLeave(unsigned short /*x*/, unsigned short /*y*/)
{
//	printf("OnMouseLeave: CtrlID = 0x%08X, x = %hd, y = %hd\n", (unsigned int) ControlID, x, y);
}

/** Mouse Over Event */
void Control::OnMouseOver(unsigned short /*x*/, unsigned short /*y*/)
{
	//printf("OnMouseOver: CtrlID = 0x%08X, x = %hd, y = %hd\n", (unsigned int) ControlID, x, y);
}

/** Mouse Button Down */
void Control::OnMouseDown(unsigned short x, unsigned short y,
	unsigned short Button, unsigned short Mod)
{
	if (Button == GEM_MB_SCRLUP || Button == GEM_MB_SCRLDOWN) {
		Control *ctrl = Owner->GetScrollControl();
		if (ctrl && (ctrl!=this)) {
			ctrl->OnMouseDown(x,y,Button,Mod);
		}
	}
}

/** Mouse Button Up */
void Control::OnMouseUp(unsigned short /*x*/, unsigned short /*y*/,
	unsigned short /*Button*/, unsigned short /*Mod*/)
{
	//printf("OnMouseUp: CtrlID = 0x%08X, x = %hd, y = %hd, Button = %d, Mos = %hd\n", (unsigned int) ControlID, x, y, Button, Mod);
}

/** Special Key Press */
void Control::OnSpecialKeyPress(unsigned char Key)
{
	if (Key == GEM_UP || Key == GEM_DOWN) {
		Control *ctrl = Owner->GetScrollControl();
		if (ctrl && (ctrl!=this)) {
			ctrl->OnSpecialKeyPress(Key);
		}
	}
}

/** Sets the Display Flags */
int Control::SetFlags(int arg_flags, int opcode)
{
	if ((arg_flags >>24) != ControlType)
		return -2;
	switch (opcode) {
		case BM_SET:
			Flags = arg_flags;  //set
			break;
		case BM_AND:
			Flags &= arg_flags;
			break;
		case BM_OR:
			Flags |= arg_flags; //turn on
			break;
		case BM_XOR:
			Flags ^= arg_flags;
			break;
		case BM_NAND:
			Flags &= ~arg_flags;//turn off
			break;
		default:
			return -1;
	}
	Changed = true;
	Owner->Invalidate();
	return 0;
}

void Control::SetAnimPicture(Sprite2D* newpic)
{
	core->GetVideoDriver()->FreeSprite(AnimPicture);
	AnimPicture = newpic;
	//apparently this is needed too, so the artifacts are not visible
	if (Owner->Visible==WINDOW_VISIBLE) {
		Changed = true;
		Owner->InvalidateForControl(this);
	}
}

/** Sets the Scroll Bar Pointer. If 'ptr' is NULL no Scroll Bar will be linked
	to this Control. */
int Control::SetScrollBar(Control* ptr)
{
	if (ptr && (ptr->ControlType!=IE_GUI_SCROLLBAR)) {
		ptr = NULL;
		printMessage("Control","Attached control is not a ScrollBar!\n",YELLOW);
		return -1;
	}
	sb = ptr;
	Changed = true;
	if (ptr) return 1;
	return 0;
}
