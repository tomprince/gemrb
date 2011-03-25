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

#include "GUI/MapControl.h"

#include "win32def.h"

#include "Game.h"
#include "Interface.h"
#include "Map.h"
#include "Video.h"

#define MAP_NO_NOTES   0
#define MAP_VIEW_NOTES 1
#define MAP_SET_NOTE   2
#define MAP_REVEAL     3

// Ratio between pixel sizes of an Area (Big map) and a Small map

static int MAP_DIV   = 3;
static int MAP_MULT  = 32;

typedef enum {black=0, gray, violet, green, orange, red, blue, darkblue, darkgreen} colorcode;

Color colors[]={
 { 0x00, 0x00, 0x00, 0xff }, //black
 { 0x60, 0x60, 0x60, 0xff }, //gray
 { 0xa0, 0x00, 0xa0, 0xff }, //violet
 { 0x00, 0xff, 0x00, 0xff }, //green
 { 0xff, 0xff, 0x00, 0xff }, //orange
 { 0xff, 0x00, 0x00, 0xff }, //red
 { 0x00, 0x00, 0xff, 0xff }, //blue
 { 0x00, 0x00, 0x80, 0xff }, //darkblue
 { 0x00, 0x80, 0x00, 0xff }  //darkgreen
};

#define MAP_TO_SCREENX(x) (XWin + XPos + XCenter - ScrollX + (x))
#define MAP_TO_SCREENY(y) (YWin + YPos + YCenter - ScrollY + (y))
// Omit [XY]Pos, since these macros are used in OnMouseDown(x, y), and x, y is 
//   already relative to control [XY]Pos there
#define SCREEN_TO_MAPX(x) ((x) - XCenter + ScrollX)
#define SCREEN_TO_MAPY(y) ((y) - YCenter + ScrollY)

#define GAME_TO_SCREENX(x) MAP_TO_SCREENX((int)((x) * MAP_DIV / MAP_MULT))
#define GAME_TO_SCREENY(y) MAP_TO_SCREENY((int)((y) * MAP_DIV / MAP_MULT))

#define SCREEN_TO_GAMEX(x) (SCREEN_TO_MAPX(x) * MAP_MULT / MAP_DIV)
#define SCREEN_TO_GAMEY(y) (SCREEN_TO_MAPY(y) * MAP_MULT / MAP_DIV)

MapControl::MapControl(void)
{
	if (core->HasFeature(GF_IWD_MAP_DIMENSIONS) ) {
		MAP_DIV=4;
		MAP_MULT=32;
	} else {
		MAP_DIV=3;
		MAP_MULT=32;
	}

	LinkedLabel = NULL;
	ScrollX = 0;
	ScrollY = 0;
	NotePosX = 0;
	NotePosY = 0;
	mouseIsDown = false;
	mouseIsDragging = false;
	Changed = true;
	convertToGame = true;
	memset(Flag,0,sizeof(Flag) );

	// initialize var and event callback to no-ops
	VarName[0] = 0;
	ResetEventHandler( MapControlOnPress );
	ResetEventHandler( MapControlOnRightPress );
	ResetEventHandler( MapControlOnDoublePress );

	MyMap = core->GetGame()->GetCurrentArea();
	if (MyMap->SmallMap) {
		MapMOS = MyMap->SmallMap;
		MapMOS->acquire();
	} else
		MapMOS = NULL;
}

MapControl::~MapControl(void)
{
	Video *video = core->GetVideoDriver();

	if (MapMOS) {
		video->FreeSprite(MapMOS);
	}
	for(int i=0;i<8;i++) {
		if (Flag[i]) {
			video->FreeSprite(Flag[i]);
		}
	}
}

// Draw fog on the small bitmap
void MapControl::DrawFog(unsigned short XWin, unsigned short YWin)
{
	Video *video = core->GetVideoDriver();

	Region old_clip;
	video->GetClipRect(old_clip);

	Region r( XWin + XPos, YWin + YPos, Width, Height );
	video->SetClipRect(&r);

	// FIXME: this is ugly, the knowledge of Map and ExploredMask
	//   sizes should be in Map.cpp
	int w = MyMap->GetWidth() / 2;
	int h = MyMap->GetHeight() / 2;

	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			Point p( (short) (MAP_MULT * x), (short) (MAP_MULT * y) );
			bool visible = MyMap->IsVisible( p, true );
			if (! visible) {
				Region rgn = Region ( MAP_TO_SCREENX(MAP_DIV * x), MAP_TO_SCREENY(MAP_DIV * y), MAP_DIV, MAP_DIV );
				video->DrawRect( rgn, colors[black] );
			}
		}
	}

	video->SetClipRect(&old_clip);
}

// To be called after changes in control's or screen geometry
void MapControl::Realize()
{
	// FIXME: ugly!! How to get area size in pixels?
	//Map *map = core->GetGame()->GetCurrentMap();
	//MapWidth = map->GetWidth();
	//MapHeight = map->GetHeight();

	if (MapMOS) {
		MapWidth = (short) MapMOS->Width;
		MapHeight = (short) MapMOS->Height;
	} else {
		MapWidth = 0;
		MapHeight = 0;
	}

	// FIXME: ugly hack! What is the actual viewport size?
	ViewWidth = (short) (core->Width * MAP_DIV / MAP_MULT);
	ViewHeight = (short) (core->Height * MAP_DIV / MAP_MULT);

	XCenter = (short) (Width - MapWidth ) / 2;
	YCenter = (short) (Height - MapHeight ) / 2;
	if (XCenter < 0) XCenter = 0;
	if (YCenter < 0) YCenter = 0;
}

void MapControl::RedrawMapControl(const char *VariableName, unsigned int Sum)
{
	if (strnicmp( VarName, VariableName, MAX_VARIABLE_LENGTH )) {
		return;
	}
	Value = Sum;
	Changed = true;
}

/** Draws the Control on the Output Display */
void MapControl::Draw(unsigned short XWin, unsigned short YWin)
{
	if (!Width || !Height) {
		return;
	}
	if (Owner->Visible!=WINDOW_VISIBLE) {
		return;
	}

	if (Changed) {
		Realize();
		Changed = false;
	}

	// we're going to paint over labels/etc, so they need to repaint!
	bool seen_this = false;
	unsigned int i;
	for (i = 0; i < Owner->GetControlCount(); i++) {
		Control *ctrl = Owner->GetControl(i);
		if (!ctrl) continue;

		// we could try working out which controls overlap,
		// but the later controls are cheap to paint..
		if (ctrl == this) { seen_this = true; continue; }
		if (!seen_this) continue;

		ctrl->Changed = true;
	}

	Video* video = core->GetVideoDriver();
	Region r( XWin + XPos, YWin + YPos, Width, Height );

	if (MapMOS) {
		video->BlitSprite( MapMOS, MAP_TO_SCREENX(0), MAP_TO_SCREENY(0), true, &r );
	}

	if (core->FogOfWar&FOG_DRAWFOG)
		DrawFog(XWin, YWin);

	Region vp = video->GetViewport();

	vp.x = GAME_TO_SCREENX(vp.x);
	vp.y = GAME_TO_SCREENY(vp.y);
	vp.w = ViewWidth;
	vp.h = ViewHeight;

	video->DrawRect( vp, colors[green], false, false );

	// Draw PCs' ellipses
	Game *game = core->GetGame();
	i = game->GetPartySize(true);
	while (i--) {
		Actor* actor = game->GetPC( i, true );
		if (MyMap->HasActor(actor) ) {
			video->DrawEllipse( (short) GAME_TO_SCREENX(actor->Pos.x), (short) GAME_TO_SCREENY(actor->Pos.y), 3, 2, actor->Selected ? colors[green] : colors[darkgreen], false );
		}
	}
	// Draw Map notes, could be turned off in bg2
	// we use the common control value to handle it, because then we
	// don't need another interface
	if (Value!=MAP_NO_NOTES) {
		i = MyMap -> GetMapNoteCount();
		while (i--) {
			MapNote * mn = MyMap -> GetMapNote(i);
			Sprite2D *anim = Flag[mn->color&7];
			Point pos = mn->Pos;
			if (convertToGame) {
				vp.x = GAME_TO_SCREENX(mn->Pos.x);
				vp.y = GAME_TO_SCREENY(mn->Pos.y);
			} else { //pst style
				vp.x = MAP_TO_SCREENX(mn->Pos.x);
				vp.y = MAP_TO_SCREENY(mn->Pos.y);
				pos.x = pos.x * MAP_MULT / MAP_DIV;
				pos.y = pos.y * MAP_MULT / MAP_DIV;
			}

			//Skip unexplored map notes
			bool visible = MyMap->IsVisible( pos, true );
			if (!visible)
				continue;

			if (anim) {
				video->BlitSprite( anim, vp.x - anim->Width/2, vp.y - anim->Height/2, true, &r );
			} else {
				video->DrawEllipse( (short) vp.x, (short) vp.y, 6, 5, colors[mn->color&7], false );
			}
		}
	}
}

/** Key Press Event */
void MapControl::OnKeyPress(unsigned char Key, unsigned short /*Mod*/)
{
#ifdef ANDROID
	switch(Key) {
		case 'o':
		case 'p':
			Control::OnKeyPress(Key, 0);
			break;
	}
#else
(void)Key; // unused, fool the compiler
#endif
}

/** Key Release Event */
void MapControl::OnKeyRelease(unsigned char Key, unsigned short Mod)
{
	switch (Key) {
		case '\t':
			//not GEM_TAB
			printf( "TAB released\n" );
			return;
		case 'f':
			if (Mod & GEM_MOD_CTRL)
				core->GetVideoDriver()->ToggleFullscreenMode();
			break;
		default:
			break;
	}
	if (!core->CheatEnabled()) {
		return;
	}
}
/** Mouse Over Event */
void MapControl::OnMouseOver(unsigned short x, unsigned short y)
{
	if (mouseIsDown) {
		ScrollX -= x - lastMouseX;
		ScrollY -= y - lastMouseY;

		if (ScrollX > MapWidth - Width)
			ScrollX = MapWidth - Width;
		if (ScrollY > MapHeight - Height)
			ScrollY = MapHeight - Height;
		if (ScrollX < 0)
			ScrollX = 0;
		if (ScrollY < 0)
			ScrollY = 0;
	}

	if (mouseIsDragging) {
		ViewHandle(x,y);
	}

	lastMouseX = x;
	lastMouseY = y;

	switch (Value) {
		case MAP_REVEAL: //for farsee effect
			Owner->Cursor = IE_CURSOR_CAST;
			break;
		case MAP_SET_NOTE:
			Owner->Cursor = IE_CURSOR_GRAB;
			break;
		default:
			Owner->Cursor = IE_CURSOR_NORMAL;
			break;
	}

	if (Value == MAP_VIEW_NOTES || Value == MAP_SET_NOTE || Value == MAP_REVEAL) {
		Point mp;
		unsigned int dist;

		if (convertToGame) {
			mp.x = (short) SCREEN_TO_GAMEX(x);
			mp.y = (short) SCREEN_TO_GAMEY(y);
			dist = 100;
		} else {
			mp.x = (short) SCREEN_TO_MAPX(x);
			mp.y = (short) SCREEN_TO_MAPY(y);
			dist = 16;
		}
		int i = MyMap -> GetMapNoteCount();
		while (i--) {
			MapNote * mn = MyMap -> GetMapNote(i);
			if (Distance(mp, mn->Pos)<dist) {
				if (LinkedLabel) {
					LinkedLabel->SetText( mn->text );
				}
				NotePosX = mn->Pos.x;
				NotePosY = mn->Pos.y;
				return;
			}
		}
		NotePosX = mp.x;
		NotePosY = mp.y;
	}
	if (LinkedLabel) {
		LinkedLabel->SetText( "" );
	}
}

/** Mouse Leave Event */
void MapControl::OnMouseLeave(unsigned short /*x*/, unsigned short /*y*/)
{
	Owner->Cursor = IE_CURSOR_NORMAL;
}

void MapControl::ClickHandle(unsigned short Button)
{
	core->GetDictionary()->SetAt( "MapControlX", NotePosX );
	core->GetDictionary()->SetAt( "MapControlY", NotePosY );
	switch(Button&GEM_MB_NORMAL) {
		case GEM_MB_ACTION:
			if (Button&GEM_MB_DOUBLECLICK) {
				RunEventHandler( MapControlOnDoublePress );
			} else {
				RunEventHandler( MapControlOnPress );
			}
			break;
		case GEM_MB_MENU:
			RunEventHandler( MapControlOnRightPress );
			break;
		default:
			break;
	}
}

void MapControl::ViewHandle(unsigned short x, unsigned short y)
{
	short xp = (short) (SCREEN_TO_MAPX(x) - ViewWidth / 2);
	short yp = (short) (SCREEN_TO_MAPY(y) - ViewHeight / 2);

	if (xp + ViewWidth > MapWidth) xp = MapWidth - ViewWidth;
	if (yp + ViewHeight > MapHeight) yp = MapHeight - ViewHeight;
	if (xp < 0) xp = 0;
	if (yp < 0) yp = 0;

	// clear any previously scheduled moves and then do it asap, so it works while paused
	unsigned int vpx = xp * MAP_MULT / MAP_DIV;
	unsigned int vpy = yp * MAP_MULT / MAP_DIV;
	core->timer->SetMoveViewPort( vpx, vpy, 0, false );
	core->GetVideoDriver()->MoveViewportTo( vpx, vpy );
}

/** Mouse Button Down */
void MapControl::OnMouseDown(unsigned short x, unsigned short y, unsigned short Button,
	unsigned short /*Mod*/)
{
	switch((unsigned char) Button) {
		case GEM_MB_SCRLUP:
			OnSpecialKeyPress(GEM_UP);
			return;
		case GEM_MB_SCRLDOWN:
			OnSpecialKeyPress(GEM_DOWN);
			return;
		case GEM_MB_ACTION:
			if (Button & GEM_MB_DOUBLECLICK) {
				ClickHandle(Button);
			}
			break;
		default:
			break;
	}

	mouseIsDown = true;
	short xp = (short) (SCREEN_TO_GAMEX(x));
	short yp = (short) (SCREEN_TO_GAMEY(y));
	Region vp = core->GetVideoDriver()->GetViewport();
	vp.w = vp.x+ViewWidth*MAP_MULT/MAP_DIV;
	vp.h = vp.y+ViewHeight*MAP_MULT/MAP_DIV;
	if ((xp>vp.x) && (xp<vp.w) && (yp>vp.y) && (yp<vp.h)) {
		mouseIsDragging = true;
	} else {
		mouseIsDragging = false;
	}
	lastMouseX = x;
	lastMouseY = y;
}

/** Mouse Button Up */
void MapControl::OnMouseUp(unsigned short x, unsigned short y, unsigned short Button,
	unsigned short /*Mod*/)
{
	if (!mouseIsDown) {
		return;
	}

	mouseIsDown = false;
	mouseIsDragging = false;
	switch(Value) {
		case MAP_REVEAL:
			ViewHandle(x,y);
			NotePosX = (short) SCREEN_TO_MAPX(x) * MAP_MULT / MAP_DIV;
			NotePosY = (short) SCREEN_TO_MAPY(y) * MAP_MULT / MAP_DIV;
			ClickHandle(Button);
			return;
		case MAP_NO_NOTES:
			ViewHandle(x,y);
			return;
		case MAP_VIEW_NOTES:
			//left click allows setting only when in MAP_SET_NOTE mode
			if ((Button == GEM_MB_ACTION) ) {
				ViewHandle(x,y);
			}
			ClickHandle(Button);
			return;
		default:
			ClickHandle(Button);
			return;
	}
}

/** Special Key Press */
void MapControl::OnSpecialKeyPress(unsigned char Key)
{
	switch (Key) {
		case GEM_LEFT:
			ScrollX -= 64;
			break;
		case GEM_UP:
			ScrollY -= 64;
			break;
		case GEM_RIGHT:
			ScrollX += 64;
			break;
		case GEM_DOWN:
			ScrollY += 64;
			break;
		case GEM_ALT:
			printf( "ALT pressed\n" );
			break;
		case GEM_TAB:
			printf( "TAB pressed\n" );
			break;
		default:
			break;
	}

	if (ScrollX > MapWidth - Width)
		ScrollX = MapWidth - Width;
	if (ScrollY > MapHeight - Height)
		ScrollY = MapHeight - Height;
	if (ScrollX < 0)
		ScrollX = 0;
	if (ScrollY < 0)
		ScrollY = 0;
}

bool MapControl::SetEvent(int eventType, EventHandler handler)
{
	Changed = true;

	switch (eventType) {
		case IE_GUI_MAP_ON_PRESS:
			MapControlOnPress = handler;
			break;
		case IE_GUI_MAP_ON_RIGHT_PRESS:
			MapControlOnRightPress = handler;
			break;
		case IE_GUI_MAP_ON_DOUBLE_PRESS:
			MapControlOnDoublePress = handler;
			break;
		default:
			return false;
	}

	return true;
}
