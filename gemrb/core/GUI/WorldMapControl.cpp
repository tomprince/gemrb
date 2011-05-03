/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
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

#include "GUI/WorldMapControl.h"

#include "win32def.h"

#include "Font.h"
#include "Game.h"
#include "GameData.h"
#include "Interface.h"
#include "Video.h"
#include "WorldMap.h"
#include "GUI/EventMgr.h"
#include "GUI/Window.h"

#define MAP_TO_SCREENX(x) XWin + XPos - ScrollX + (x)
#define MAP_TO_SCREENY(y) YWin + YPos - ScrollY + (y)

WorldMapControl::WorldMapControl(const char *font, int direction)
{
	ScrollX = 0;
	ScrollY = 0;
	MouseIsDown = false;
	Changed = true;
	Area = NULL;
	Value = direction;
	Game* game = core->GetGame();
	WorldMap* worldmap = core->GetWorldMap();
	strncpy(currentArea, game->CurrentArea, 8);
	int entry = core->GetAreaAlias(currentArea);
	if (entry >= 0) {
		WMPAreaEntry *m = worldmap->GetEntry(entry);
		strncpy(currentArea, m->AreaResRef, 8);
	}

	//if there is no trivial area, look harder
	if (!worldmap->GetArea(currentArea, (unsigned int &) entry) && 
		core->HasFeature(GF_FLEXIBLE_WMAP) ) {
		WMPAreaEntry *m = worldmap->FindNearestEntry(currentArea, (unsigned int &) entry);
		if (m) {
			strncpy(currentArea, m->AreaResRef, 8);
		}
	}

	//this also updates visible locations
	worldmap->CalculateDistances(currentArea, Value);
	
	// alpha bit is unfortunately ignored
	if (font[0]) {
		ftext = core->GetFont(font);
	} else {
		ftext = NULL;
	}

	// initialize label colors
	// NOTE: it would be better to initialize these colors from
	//   some 2da file
	Color normal = { 0xf0, 0xf0, 0xf0, 0xff };
	Color selected = { 0xf0, 0x80, 0x80, 0xff };
	Color notvisited = { 0x80, 0x80, 0xf0, 0xff };
	Color black = { 0x00, 0x00, 0x00, 0x00 };

	pal_normal = core->CreatePalette ( normal, black );
	pal_selected = core->CreatePalette ( selected, black );
	pal_notvisited = core->CreatePalette ( notvisited, black );


	ResetEventHandler( WorldMapControlOnPress );
	ResetEventHandler( WorldMapControlOnEnter );
}

WorldMapControl::~WorldMapControl(void)
{
	//Video *video = core->GetVideoDriver();

	gamedata->FreePalette( pal_normal );
	gamedata->FreePalette( pal_selected );
	gamedata->FreePalette( pal_notvisited );
}

/** Draws the Control on the Output Display */
void WorldMapControl::Draw(unsigned short XWin, unsigned short YWin)
{
	WorldMap* worldmap = core->GetWorldMap();
	if (!Width || !Height) {
		return;
	}
	if(!Changed)
		return;
	Changed = false;
	Video* video = core->GetVideoDriver();
	Region r( XWin+XPos, YWin+YPos, Width, Height );
	Region clipbackup;
	video->GetClipRect(clipbackup);
	video->SetClipRect(&r);
	video->BlitSprite( worldmap->GetMapMOS(), MAP_TO_SCREENX(0), MAP_TO_SCREENY(0), true, &r );

	unsigned int i;
	unsigned int ec = worldmap->GetEntryCount();
	for(i=0;i<ec;i++) {
		WMPAreaEntry *m = worldmap->GetEntry(i);
		if (! (m->GetAreaStatus() & WMP_ENTRY_VISIBLE)) continue;

		int xOffs = MAP_TO_SCREENX(m->X);
		int yOffs = MAP_TO_SCREENY(m->Y);
		Sprite2D* icon = m->GetMapIcon(worldmap->bam);
		if( icon ) {
			video->BlitSprite( icon, xOffs, yOffs, true, &r );
			video->FreeSprite( icon );
		}

		if (AnimPicture && !strnicmp(m->AreaResRef, currentArea, 8) ) {
			core->GetVideoDriver()->BlitSprite( AnimPicture, xOffs, yOffs, true, &r );
		}
	}

	// Draw WMP entry labels
	if (ftext==NULL) {
		video->SetClipRect(&clipbackup);
		return;
	}
	for(i=0;i<ec;i++) {
		WMPAreaEntry *m = worldmap->GetEntry(i);
		if (! (m->GetAreaStatus() & WMP_ENTRY_VISIBLE)) continue;
		Sprite2D *icon=m->GetMapIcon(worldmap->bam);
		int h=0,w=0,xpos=0,ypos=0;
		if (icon) {
			h=icon->Height;
			w=icon->Width;
			xpos=icon->XPos;
			ypos=icon->YPos;
			video->FreeSprite( icon );
		}

		Region r2 = Region( MAP_TO_SCREENX(m->X-xpos), MAP_TO_SCREENY(m->Y-ypos), w, h );
		if (!m->GetCaption())
			continue;

		int tw = ftext->CalcStringWidth( m->GetCaption() ) + 5;
		int th = ftext->maxHeight;
		
		Palette* text_pal = pal_normal;
		
		if (Area == m) {
			text_pal = pal_selected;
		} else {
			if (! (m->GetAreaStatus() & WMP_ENTRY_VISITED)) {
				text_pal = pal_notvisited;
			}
		}

		ftext->Print( Region( r2.x + (r2.w - tw)/2, r2.y + r2.h, tw, th ),
				( const unsigned char * ) m->GetCaption(), text_pal, 0, true );
	}
	video->SetClipRect(&clipbackup);
}

/** Key Release Event */
void WorldMapControl::OnKeyRelease(unsigned char Key, unsigned short Mod)
{
	switch (Key) {
		case 'f':
			if (Mod & GEM_MOD_CTRL)
				core->GetVideoDriver()->ToggleFullscreenMode();
			break;
		default:
			break;
	}
}
void WorldMapControl::AdjustScrolling(short x, short y)
{
	WorldMap* worldmap = core->GetWorldMap();
	if (x || y) {
		ScrollX += x;
		ScrollY += y;
	} else {
		//center worldmap on current area
		unsigned entry;

		WMPAreaEntry *m = worldmap->GetArea(currentArea,entry);
		if (m) {
			ScrollX = m->X - Width/2;
			ScrollY = m->Y - Height/2;
		}
	}
	Sprite2D *MapMOS = worldmap->GetMapMOS();
	if (ScrollX > MapMOS->Width - Width)
		ScrollX = MapMOS->Width - Width;
	if (ScrollY > MapMOS->Height - Height)
		ScrollY = MapMOS->Height - Height;
	if (ScrollX < 0)
		ScrollX = 0;
	if (ScrollY < 0)
		ScrollY = 0;
	Changed = true;
	Area = NULL;
}

/** Mouse Over Event */
void WorldMapControl::OnMouseOver(unsigned short x, unsigned short y)
{
	WorldMap* worldmap = core->GetWorldMap();
	lastCursor = IE_CURSOR_GRAB;

	if (MouseIsDown) {
		AdjustScrolling(lastMouseX-x, lastMouseY-y);
	}

	lastMouseX = x;
	lastMouseY = y;

	if (Value!=(ieDword) -1) {
		x =(ieWord) (x + ScrollX);
		y =(ieWord) (y + ScrollY);

		WMPAreaEntry *oldArea = Area;
		Area = NULL;

		unsigned int i;
		unsigned int ec = worldmap->GetEntryCount();
		for (i=0;i<ec;i++) {
			WMPAreaEntry *ae = worldmap->GetEntry(i);

			if ( (ae->GetAreaStatus() & WMP_ENTRY_WALKABLE)!=WMP_ENTRY_WALKABLE) {
				continue; //invisible or inaccessible
			}
			if (!strnicmp(ae->AreaResRef, currentArea, 8) ) {
				continue; //current area
			}

			Sprite2D *icon=ae->GetMapIcon(worldmap->bam);
			int h=0,w=0;
			if (icon) {
				h=icon->Height;
				w=icon->Width;
				core->GetVideoDriver()->FreeSprite( icon );
			}
			if (ftext && ae->GetCaption()) {
				int tw = ftext->CalcStringWidth( ae->GetCaption() ) + 5;
				int th = ftext->maxHeight;
				if(h<th)
					h=th;
				if(w<tw)
					w=tw;
			}
			if (ae->X > x) continue;
			if (ae->X + w < x) continue;
			if (ae->Y > y) continue;
			if (ae->Y + h < y) continue;
			lastCursor = IE_CURSOR_NORMAL;
			Area=ae;
			if(oldArea!=ae) {
				RunEventHandler(WorldMapControlOnEnter);
			}
			break;
		}
	}

	Owner->Cursor = lastCursor;
}

/** Sets the tooltip to be displayed on the screen now */
void WorldMapControl::DisplayTooltip()
{
	if (Area) {
		int x = Owner->XPos+XPos+lastMouseX;
		int y = Owner->YPos+YPos+lastMouseY-50;
		core->DisplayTooltip( x, y, this );
	} else {
		core->DisplayTooltip( 0, 0, NULL );
	}
}

/** Mouse Leave Event */
void WorldMapControl::OnMouseLeave(unsigned short /*x*/, unsigned short /*y*/)
{
	Owner->Cursor = IE_CURSOR_NORMAL;
	Area = NULL;
}

/** Mouse Button Down */
void WorldMapControl::OnMouseDown(unsigned short x, unsigned short y,
	unsigned short Button, unsigned short /*Mod*/)
{
	switch(Button) {
	case GEM_MB_ACTION:
		MouseIsDown = true;
		lastMouseX = x;
		lastMouseY = y;
		break;
	case GEM_MB_SCRLUP:
		OnSpecialKeyPress(GEM_UP);
		break;
	case GEM_MB_SCRLDOWN:
		OnSpecialKeyPress(GEM_DOWN);
		break;
	}
}
/** Mouse Button Up */
void WorldMapControl::OnMouseUp(unsigned short /*x*/, unsigned short /*y*/,
	unsigned short Button, unsigned short /*Mod*/)
{
	if (Button != GEM_MB_ACTION) {
		return;
	}
	MouseIsDown = false;
	if (lastCursor==IE_CURSOR_NORMAL) {
		RunEventHandler( WorldMapControlOnPress );
	}
}

/** Special Key Press */
void WorldMapControl::OnSpecialKeyPress(unsigned char Key)
{
	WorldMap* worldmap = core->GetWorldMap();
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
			print( "ALT pressed\n" );
			break;
		case GEM_TAB:
			print( "TAB pressed\n" );
			break;
	}

	Sprite2D *MapMOS = worldmap->GetMapMOS();
	if (ScrollX > MapMOS->Width - Width)
		ScrollX = MapMOS->Width - Width;
	if (ScrollY > MapMOS->Height - Height)
		ScrollY = MapMOS->Height - Height;
	if (ScrollX < 0)
		ScrollX = 0;
	if (ScrollY < 0)
		ScrollY = 0;
}

bool WorldMapControl::SetEvent(int eventType, EventHandler handler)
{
	Changed = true;

	switch (eventType) {
	case IE_GUI_WORLDMAP_ON_PRESS:
		WorldMapControlOnPress = handler;
		break;
	case IE_GUI_MOUSE_ENTER_WORLDMAP:
		WorldMapControlOnEnter = handler;
		break;
	default:
		return false;
	}

	return true;
}

void WorldMapControl::SetColor(int which, Color color)
{
	Palette* pal;
	// FIXME: clearly it can cause palettes to be re-created several times,
	//   because setting background color creates all palettes anew.
	switch (which) {
	case IE_GUI_WMAP_COLOR_BACKGROUND:
		pal = core->CreatePalette( pal_normal->front, color );
		gamedata->FreePalette( pal_normal );
		pal_normal = pal;
		pal = core->CreatePalette( pal_selected->front, color );
		gamedata->FreePalette( pal_selected );
		pal_selected = pal;
		pal = core->CreatePalette( pal_notvisited->front, color );
		gamedata->FreePalette( pal_notvisited );
		pal_notvisited = pal;
		break;
	case IE_GUI_WMAP_COLOR_NORMAL:
		pal = core->CreatePalette( color, pal_normal->back );
		gamedata->FreePalette( pal_normal );
		pal_normal = pal;
		break;
	case IE_GUI_WMAP_COLOR_SELECTED:
		pal = core->CreatePalette( color, pal_selected->back );
		gamedata->FreePalette( pal_selected );
		pal_selected = pal;
		break;
	case IE_GUI_WMAP_COLOR_NOTVISITED:
		pal = core->CreatePalette( color, pal_notvisited->back );
		gamedata->FreePalette( pal_notvisited );
		pal_notvisited = pal;
		break;
	default:
		break;
	}

	Changed = true;
}
