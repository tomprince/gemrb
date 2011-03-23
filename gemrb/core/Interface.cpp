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
*
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "Interface.h"

#include "exports.h"
#include "globals.h"
#include "strrefs.h"
#include "win32def.h"

#include "ActorMgr.h"
#include "AmbientMgr.h"
#include "AnimationMgr.h"
#include "ArchiveImporter.h"
#include "Audio.h"
#include "Calendar.h"
#include "DataFileMgr.h"
#include "DialogHandler.h"
#include "DialogMgr.h"
#include "DisplayMessage.h"
#include "EffectMgr.h"
#include "EffectQueue.h"
#include "Factory.h"
#include "Game.h"
#include "GameData.h"
#include "ImageMgr.h"
#include "ItemMgr.h"
#include "MapMgr.h"
#include "MoviePlayer.h"
#include "MusicMgr.h"
#include "Palette.h"
#include "PluginMgr.h"
#include "PluginMgr.h"
#include "ProjectileServer.h"
#include "SaveGameIterator.h"
#include "SaveGameMgr.h"
#include "ScriptEngine.h"
#include "ScriptedAnimation.h"
#include "SoundMgr.h"
#include "SpellMgr.h"
#include "StoreMgr.h"
#include "StringMgr.h"
#include "SymbolMgr.h"
#include "TileMap.h"
#include "Video.h"
#include "WorldMapMgr.h"
#include "GameScript/GameScript.h"
#include "GUI/Button.h"
#include "GUI/Console.h"
#include "GUI/GameControl.h"
#include "GUI/Label.h"
#include "GUI/MapControl.h"
#include "GUI/WorldMapControl.h"
#include "System/FileStream.h"
#include "System/VFS.h"

#if defined(__HAIKU__)
#include <unistd.h>
#endif

#include <cstdlib>
#include <time.h>
#include <vector>

GEM_EXPORT Interface* core;

#ifdef WIN32
GEM_EXPORT HANDLE hConsole;
#endif

//use DialogF.tlk if the protagonist is female, that's why we leave space
static const char dialogtlk[] = "dialog.tlk\0";

static int MaximumAbility = 25;
static ieWordSigned *strmod = NULL;
static ieWordSigned *strmodex = NULL;
static ieWordSigned *intmod = NULL;
static ieWordSigned *dexmod = NULL;
static ieWordSigned *conmod = NULL;
static ieWordSigned *chrmod = NULL;
static ieWordSigned *lorebon = NULL;
static ieWordSigned *wisbon = NULL;
static int **reputationmod = NULL;
static ieVariable IWD2DeathVarFormat = "_DEAD%s";
static ieVariable DeathVarFormat = "SPRITE_IS_DEAD%s";

Interface::Interface(int iargc, char* iargv[])
{
	argc = iargc;
	argv = iargv;
#ifdef WIN32
	hConsole = GetStdHandle( STD_OUTPUT_HANDLE );
#endif
	textcolor( LIGHT_WHITE );
	printf( "GemRB Core Version v%s Loading...\n", VERSION_GEMRB );

	// default to the correct endianswitch
	ieWord endiantest = 1;
	if (((char *)&endiantest)[1] == 1) {
		// big-endian
		DataStream::SetEndianSwitch(true);
	}

	unsigned int i;
	for(i=0;i<256;i++) {
		pl_uppercase[i]=(ieByte) toupper(i);
		pl_lowercase[i]=(ieByte) tolower(i);
	}

	projserv = NULL;
	VideoDriverName = "sdl";
	AudioDriverName = "openal";
	vars = NULL;
	tokens = NULL;
	lists = NULL;
	RtRows = NULL;
	sgiterator = NULL;
	game = NULL;
	calendar = NULL;
	worldmap = NULL;
	CurrentStore = NULL;
	CurrentContainer = NULL;
	UseContainer = false;
	InfoTextPalette = NULL;
	timer = NULL;
	evntmgr = NULL;
	console = NULL;
	slottypes = NULL;
	slotmatrix = NULL;

	ModalWindow = NULL;
	tooltip_x = 0;
	tooltip_y = 0;
	tooltip_currtextw = 0;
	tooltip_ctrl = NULL;
	plugin_flags = NULL;

	pal16 = NULL;
	pal32 = NULL;
	pal256 = NULL;

	GUIEnhancements = 0;

	CursorCount = 0;
	Cursors = NULL;

	mousescrollspd = 10;

	strncpy( GameType, "auto", sizeof( GameType )-1);
	ConsolePopped = false;
	CheatFlag = false;
	FogOfWar = 1;
	QuitFlag = QF_NORMAL;
	EventFlag = EF_CONTROL;
#ifndef WIN32
	CaseSensitive = true; //this is the default value, so CD1/CD2 will be resolved
#else
	CaseSensitive = false;
#endif
	GameOnCD = false;
	SkipIntroVideos = false;
	DrawFPS = false;
	KeepCache = false;
	TooltipDelay = 100;
	IgnoreOriginalINI = 0;
	FullScreen = 0;
	GUIScriptsPath[0] = 0;
	GamePath[0] = 0;
	SavePath[0] = 0;
	GemRBPath[0] = 0;
	PluginsPath[0] = 0;
	CachePath[0] = 0;
	GemRBOverridePath[0] = 0;
	GameName[0] = 0;

	strncpy( GameOverridePath, "override", sizeof(GameOverridePath) );
	strncpy( GameSoundsPath, "sounds", sizeof(GameSoundsPath) );
	strncpy( GameScriptsPath, "scripts", sizeof(GameScriptsPath) );
	strncpy( GamePortraitsPath, "portraits", sizeof(GamePortraitsPath) );
	strncpy( GameCharactersPath, "characters", sizeof(GameCharactersPath) );
	strncpy( GameDataPath, "data", sizeof(GameDataPath) );
	strncpy( INIConfig, "baldur.ini", sizeof(INIConfig) );
	strncpy( ButtonFont, "STONESML", sizeof(ButtonFont) );
	strncpy( TooltipFont, "STONESML", sizeof(TooltipFont) );
	strncpy( MovieFont, "STONESML", sizeof(MovieFont) );
	strncpy( ScrollCursorBam, "CURSARW", sizeof(ScrollCursorBam) );
	strncpy( GlobalScript, "BALDUR", sizeof(GlobalScript) );
	strncpy( WorldMapName[0], "WORLDMAP", sizeof(ieResRef) );
	memset( WorldMapName[1], 0, sizeof(ieResRef) );
	strncpy( Palette16, "MPALETTE", sizeof(Palette16) );
	strncpy( Palette32, "PAL32", sizeof(Palette32) );
	strncpy( Palette256, "MPAL256", sizeof(Palette256) );
	strcpy( TooltipBackResRef, "\0" );
	for (int size = 0; size < MAX_CIRCLE_SIZE; size++) {
		strcpy( GroundCircleBam[size], "\0" );
		GroundCircleScale[size] = 0;
	}
	TooltipColor.r = 0;
	TooltipColor.g = 255;
	TooltipColor.b = 0;
	TooltipColor.a = 255;
	TooltipMargin = 10;

	TooltipBack = NULL;
	DraggedItem = NULL;
	DraggedPortrait = 0;
	DefSound = NULL;
	DSCount = -1;
	memset(GameFeatures, 0, sizeof( GameFeatures ));
	//GameFeatures = 0;
	//GameFeatures2 = 0;
	memset( WindowFrames, 0, sizeof( WindowFrames ));
	memset( GroundCircles, 0, sizeof( GroundCircles ));
	memset(FogSprites, 0, sizeof( FogSprites ));
	AreaAliasTable = NULL;
	ItemExclTable = NULL;
	ItemDialTable = NULL;
	ItemDial2Table = NULL;
	ItemTooltipTable = NULL;
	update_scripts = false;
	SpecialSpellsCount = -1;
	SpecialSpells = NULL;

	gamedata = new GameData();
}

#define FreeResourceVector(type, variable) \
{ \
	size_t i=variable.size(); \
	while(i--) { \
		if (variable[i]) { \
		delete variable[i]; \
		} \
	} \
	variable.clear(); \
}

//2da lists are ieDword lists allocated by malloc
static void Release2daList(void *poi)
{
	free( (ieDword *) poi);
}

static void ReleaseItemList(void *poi)
{
	delete ((ItemList *) poi);
}

void FreeAbilityTables()
{
	if (strmod) {
		free(strmod);
	}
	strmod = NULL;
	if (strmodex) {
		free(strmodex);
	}
	strmodex = NULL;
	if (intmod) {
		free(intmod);
	}
	intmod = NULL;
	if (dexmod) {
		free(dexmod);
	}
	dexmod = NULL;
	if (conmod) {
		free(conmod);
	}
	conmod = NULL;
	if (chrmod) {
		free(chrmod);
	}
	chrmod = NULL;
	if (lorebon) {
		free(lorebon);
	}
	lorebon = NULL;
	if (wisbon) {
		free(wisbon);
	}
	wisbon = NULL;
}

void Interface::FreeResRefTable(ieResRef *&table, int &count)
{
	if (table) {
		free( table );
		count = -1;
	}
}

static void ReleaseItemTooltip(void *poi)
{
	free(poi);
}

Interface::~Interface(void)
{
	DragItem(NULL,NULL);
	delete AreaAliasTable;

	if (music) {
		music->HardEnd();
	}
	// stop any ambients which are still enqueued
	if (AudioDriver) {
		AmbientMgr *ambim = AudioDriver->GetAmbientMgr();
		if (ambim) ambim->deactivate();
	}
	//destroy the highest objects in the hierarchy first!
	delete game;
	delete calendar;
	delete worldmap;

	FreeAbilityTables();

	if (reputationmod) {
		for (unsigned int i=0; i<20; i++) {
			if (reputationmod[i]) {
				free(reputationmod[i]);
			}
		}
		free(reputationmod);
		reputationmod=NULL;
	}

	if (SpecialSpells) {
		free(SpecialSpells);
	}
	SurgeSpells.clear();

	PluginMgr::Get()->RunCleanup();

	ReleaseMemoryActor();
	CharAnimations::ReleaseMemory();
	delete CurrentStore;

	FreeResRefTable(DefSound, DSCount);

	free( slottypes );
	free( slotmatrix );

	delete sgiterator;

	if (Cursors) {
		for (int i = 0; i < CursorCount; i++) {
			video->FreeSprite( Cursors[i] );
		}
		delete[] Cursors;
	}

	FreeResourceVector( Font, fonts );
	FreeResourceVector( Window, windows );

	size_t i;
	for (i = 0; i < musiclist.size(); i++) {
		free((void *)musiclist[i]);
	}

	DamageInfoMap.clear();

	ModalStates.clear();

	delete plugin_flags;

	delete projserv;

	delete console;

	delete pal256;
	delete pal32;
	delete pal16;

	delete timer;

	if (video) {

		for(i=0;i<sizeof(FogSprites)/sizeof(Sprite2D *);i++ ) {
			video->FreeSprite(FogSprites[i]);
		}

		for(i=0;i<4;i++) {
			video->FreeSprite(WindowFrames[i]);
		}

		for (int size = 0; size < MAX_CIRCLE_SIZE; size++) {
			for(i=0;i<6;i++) {
				video->FreeSprite(GroundCircles[size][i]);
			}
		}

		if (TooltipBack) {
			for(i=0;i<3;i++) {
				//freesprite checks for null pointer
				video->FreeSprite(TooltipBack[i]);
			}
			delete[] TooltipBack;
		}
		if (InfoTextPalette) {
			gamedata->FreePalette(InfoTextPalette);
		}

		video->SetDragCursor(NULL);
	}

	delete evntmgr;

	delete vars;
	delete tokens;
	if (lists) {
		lists->RemoveAll(Release2daList);
		delete lists;
	}

	if (RtRows) {
		RtRows->RemoveAll(ReleaseItemList);
		delete RtRows;
	}
	if (ItemExclTable) {
		ItemExclTable->RemoveAll(NULL);
		delete ItemExclTable;
	}
	if (ItemDialTable) {
		ItemDialTable->RemoveAll(NULL);
		delete ItemDialTable;
	}
	if (ItemDial2Table) {
		ItemDial2Table->RemoveAll(NULL);
		delete ItemDial2Table;
	}
	if (ItemTooltipTable) {
		ItemTooltipTable->RemoveAll(ReleaseItemTooltip);
		delete ItemTooltipTable;
	}

	Map::ReleaseMemory();
	Actor::ReleaseMemory();

	gamedata->ClearCaches();
	delete gamedata;
	gamedata = NULL;

	// Removing all stuff from Cache, except bifs
	if (!KeepCache) DelTree((const char *) CachePath, true);
}

void Interface::SetWindowFrame(int i, Sprite2D *Picture)
{
	video->FreeSprite(WindowFrames[i]);
	WindowFrames[i]=Picture;
}

GameControl* Interface::StartGameControl()
{
	//making sure that our window is the first one
	if (ConsolePopped) {
		PopupConsole();
	}
	DelAllWindows();//deleting ALL windows
	gamedata->DelTable(0xffffu); //dropping ALL tables
	Window* gamewin = new Window( 0xffff, 0, 0, (ieWord) Width, (ieWord) Height );
	gamewin->WindowPack[0]=0;
	GameControl* gc = new GameControl();
	gc->XPos = 0;
	gc->YPos = 0;
	gc->Width = (ieWord) Width;
	gc->Height = (ieWord) Height;
	gc->Owner = gamewin;
	gc->ControlID = 0x00000000;
	gc->ControlType = IE_GUI_GAMECONTROL;
	gamewin->AddControl( gc );
	AddWindow( gamewin );
	SetVisible( 0, WINDOW_VISIBLE );
	//setting the focus to the game control
	evntmgr->SetFocused(gamewin, gc);
	if (guiscript->LoadScript( "MessageWindow" )) {
		guiscript->RunFunction( "MessageWindow", "OnLoad" );
		gc->UnhideGUI();
	}

	return gc;
}

/* handle main loop events that might destroy or create windows
thus cannot be called from DrawWindows directly
these events are pending until conditions are right
*/
void Interface::HandleEvents()
{
	GameControl *gc = GetGameControl();
	if (gc && (!gc->Owner || !gc->Owner->Visible)) {
		gc=NULL;
	}

	if (EventFlag&EF_SELECTION) {
		EventFlag&=~EF_SELECTION;
		guiscript->RunFunction( "GUICommonWindows", "SelectionChanged", false);
	}

	if (EventFlag&EF_UPDATEANIM) {
		EventFlag&=~EF_UPDATEANIM;
		guiscript->RunFunction( "GUICommonWindows", "UpdateAnimation", false);
	}

	if (EventFlag&EF_PORTRAIT) {
		ieDword tmp = (ieDword) ~0;
		vars->Lookup( "PortraitWindow", tmp );
		if (tmp != (ieDword) ~0) {
			EventFlag&=~EF_PORTRAIT;
			guiscript->RunFunction( "GUICommonWindows", "UpdatePortraitWindow" );
		}
	}

	if (EventFlag&EF_ACTION) {
		ieDword tmp = (ieDword) ~0;
		vars->Lookup( "ActionsWindow", tmp );
		if (tmp != (ieDword) ~0) {
			EventFlag&=~EF_ACTION;
			guiscript->RunFunction( "GUICommonWindows", "UpdateActionsWindow" );
		}
	}

	if ((EventFlag&EF_CONTROL) && gc) {
		EventFlag&=~EF_CONTROL;
		guiscript->RunFunction( "MessageWindow", "UpdateControlStatus" );
		//this is the only value we can use here
		if (game->ControlStatus & CS_HIDEGUI)
			gc->HideGUI();
		else
			gc->UnhideGUI();
		return;
	}
	if ((EventFlag&EF_SHOWMAP) && gc) {
		ieDword tmp = (ieDword) ~0;
		vars->Lookup( "OtherWindow", tmp );
		if (tmp == (ieDword) ~0) {
			EventFlag &= ~EF_SHOWMAP;
			guiscript->RunFunction( "GUIMA", "ShowMap" );
		}
		return;
	}

	if (EventFlag&EF_SEQUENCER) {
		EventFlag&=~EF_SEQUENCER;
		guiscript->RunFunction( "GUIMG", "OpenSequencerWindow" );
		return;
	}

	if (EventFlag&EF_IDENTIFY) {
		EventFlag&=~EF_IDENTIFY;
		// FIXME: Implement this.
		guiscript->RunFunction( "GUICommonWindows", "OpenIdentifyWindow" );
		return;
	}
	if (EventFlag&EF_OPENSTORE) {
		EventFlag&=~EF_OPENSTORE;
		guiscript->RunFunction( "GUISTORE", "OpenStoreWindow" );
		return;
	}

	if (EventFlag&EF_EXPANSION) {
		EventFlag&=~EF_EXPANSION;
		guiscript->RunFunction( "MessageWindow", "GameExpansion", false );
		return;
	}

	if (EventFlag&EF_CREATEMAZE) {
		EventFlag&=~EF_CREATEMAZE;
		guiscript->RunFunction( "Maze", "CreateMaze", false );
		return;
	}

	if ((EventFlag&EF_RESETTARGET) && gc) {
		EventFlag&=~EF_RESETTARGET;
		EventFlag|=EF_TARGETMODE;
		gc->ResetTargetMode();
		return;
	}

	if ((EventFlag&EF_TARGETMODE) && gc) {
		EventFlag&=~EF_TARGETMODE;
		gc->UpdateTargetMode();
		return;
	}
}

/* handle main loop events that might destroy or create windows
thus cannot be called from DrawWindows directly
*/
void Interface::HandleFlags()
{
	//clear events because the context changed
	EventFlag = EF_CONTROL;

	if (QuitFlag&(QF_QUITGAME|QF_EXITGAME) ) {
		// when reaching this, quitflag should be 1 or 2
		// if Exitgame was set, we'll set Start.py too
		QuitGame (QuitFlag&QF_EXITGAME);
		QuitFlag &= ~(QF_QUITGAME|QF_EXITGAME);
	}

	if (QuitFlag&QF_LOADGAME) {
		QuitFlag &= ~QF_LOADGAME;
		LoadGame(LoadGameIndex.get(), VersionOverride );
		LoadGameIndex.release();
		//after loading a game, always check if the game needs to be upgraded
	}

	if (QuitFlag&QF_ENTERGAME) {
		QuitFlag &= ~QF_ENTERGAME;
		if (game) {
			EventFlag|=EF_EXPANSION;
			timer->Init();

			//rearrange party slots
			game->ConsolidateParty();
			GameControl* gc = StartGameControl();
			//switch map to protagonist
			Actor* actor = GetFirstSelectedPC(true);
			if (actor) {
				gc->ChangeMap(actor, true);
			}
		} else {
			printMessage("Core", "No game to enter...\n", LIGHT_RED);
			QuitFlag = QF_QUITGAME;
		}
	}

	if (QuitFlag&QF_CHANGESCRIPT) {
		QuitFlag &= ~QF_CHANGESCRIPT;
		guiscript->LoadScript( NextScript );
		guiscript->RunFunction( NextScript, "OnLoad" );
	}
}

bool GenerateAbilityTables()
{
	FreeAbilityTables();

	//range is: 0 - maximumability
	int tablesize = MaximumAbility+1;
	strmod = (ieWordSigned *) malloc (tablesize * 4 * sizeof(ieWordSigned) );
	if (!strmod)
		return false;
	strmodex = (ieWordSigned *) malloc (101 * 4 * sizeof(ieWordSigned) );
	if (!strmodex)
		return false;
	intmod = (ieWordSigned *) malloc (tablesize * 5 * sizeof(ieWordSigned) );
	if (!intmod)
		return false;
	dexmod = (ieWordSigned *) malloc (tablesize * 3 * sizeof(ieWordSigned) );
	if (!dexmod)
		return false;
	conmod = (ieWordSigned *) malloc (tablesize * 5 * sizeof(ieWordSigned) );
	if (!conmod)
		return false;
	chrmod = (ieWordSigned *) malloc (tablesize * 1 * sizeof(ieWordSigned) );
	if (!chrmod)
		return false;
	lorebon = (ieWordSigned *) malloc (tablesize * 1 * sizeof(ieWordSigned) );
	if (!lorebon)
		return false;
	wisbon = (ieWordSigned *) malloc (tablesize * 1 * sizeof(ieWordSigned) );
	if (!wisbon)
		return false;
	return true;
}

bool Interface::ReadAbilityTable(const ieResRef tablename, ieWordSigned *mem, int columns, int rows)
{
	AutoTable tab(tablename);
	if (!tab) {
		return false;
	}
	//this is a hack for rows not starting at 0 in some cases
	int fix = 0;
	const char * tmp = tab->GetRowName(0);
	if (tmp && (tmp[0]!='0')) {
		fix = atoi(tmp);
		for (int i=0;i<fix;i++) {
			for (int j=0;j<columns;j++) {
				mem[rows*j+i]=(ieWordSigned) strtol(tab->QueryField(0,j),NULL,0 );
			}
		}
	}
	for (int j=0;j<columns;j++) {
		for( int i=0;i<rows-fix;i++) {
			mem[rows*j+i+fix] = (ieWordSigned) strtol(tab->QueryField(i,j),NULL,0 );
		}
	}
	return true;
}

bool Interface::ReadAbilityTables()
{
	bool ret = GenerateAbilityTables();
	if (!ret)
		return ret;
	ret = ReadAbilityTable("strmod", strmod, 4, MaximumAbility + 1);
	if (!ret)
		return ret;
	ret = ReadAbilityTable("strmodex", strmodex, 4, 101);
	//3rd ed doesn't have strmodex, but has a maximum of 40
	if (!ret && (MaximumAbility<=25) )
		return ret;
	ret = ReadAbilityTable("intmod", intmod, 5, MaximumAbility + 1);
	if (!ret)
		return ret;
	ret = ReadAbilityTable("hpconbon", conmod, 5, MaximumAbility + 1);
	if (!ret)
		return ret;
	if (!HasFeature(GF_3ED_RULES)) {
		//no lorebon in iwd2???
		ret = ReadAbilityTable("lorebon", lorebon, 1, MaximumAbility + 1);
		if (!ret)
			return ret;
		//no dexmod in iwd2???
		ret = ReadAbilityTable("dexmod", dexmod, 3, MaximumAbility + 1);
		if (!ret)
			return ret;
	}
	//this table is a single row (not a single column)
	ret = ReadAbilityTable("chrmodst", chrmod, MaximumAbility + 1, 1);
	if (!ret)
		return ret;
	if (HasFeature(GF_WISDOM_BONUS)) {
		ret = ReadAbilityTable("wisxpbon", wisbon, 1, MaximumAbility + 1);
		if (!ret)
			return ret;
	}
	return true;
}

bool Interface::ReadGameTimeTable()
{
	AutoTable table("gametime");
	if (!table) {
		return false;
	}

	Time.round_sec = atoi(table->QueryField("ROUND_SECONDS", "DURATION"));
	Time.turn_sec = atoi(table->QueryField("TURN_SECONDS", "DURATION"));
	Time.round_size = Time.round_sec * AI_UPDATE_TIME;
	Time.rounds_per_turn = Time.turn_sec / Time.round_sec;

	return true;
}

bool Interface::ReadSpecialSpells()
{
	int i;
	bool result = true;

	AutoTable table("splspec");
	if (table) {
		SpecialSpellsCount = table->GetRowCount();
		SpecialSpells = (SpellDescType *) malloc( sizeof(SpellDescType) * SpecialSpellsCount);
		for (i=0;i<SpecialSpellsCount;i++) {
			strnlwrcpy(SpecialSpells[i].resref, table->GetRowName(i),8 );
			//if there are more flags, compose this value into a bitfield
			SpecialSpells[i].value = atoi(table->QueryField(i,0) );
		}
	} else {
		result = false;
	}

	table.load("wildmag");
	if (table) {
		SurgeSpell ss;
		for (i = 0; (unsigned)i < table->GetRowCount(); i++) {
			strncpy(ss.spell, table->QueryField(i, 0), 8);
			ss.message = strtol(table->QueryField(i, 1), NULL, 0);
			// comment ignored
			SurgeSpells.push_back(ss);
		}
	} else {
		result = false;
	}

	return result;
}

int Interface::GetSpecialSpell(ieResRef resref)
{
	for (int i=0;i<SpecialSpellsCount;i++) {
		if (!strnicmp(resref, SpecialSpells[i].resref, sizeof(ieResRef))) {
			return SpecialSpells[i].value;
		}
	}
	return 0;
}

//disable spells based on some circumstances
int Interface::CheckSpecialSpell(ieResRef resref, Actor *actor)
{
	int sp = GetSpecialSpell(resref);

	//the identify spell is always disabled on the menu
	if (sp&SP_IDENTIFY) {
		return 1;
	}

	//if actor is silenced, and spell cannot be cast in silence, disable it
	if (actor->GetStat(IE_STATE_ID) & STATE_SILENCED ) {
		if (!(sp&SP_SILENCE)) {
			return 1;
		}
	}

	return 0;
}

bool Interface::ReadAuxItemTables()
{
	int idx;
	bool flag = true;

	if (ItemExclTable) {
		ItemExclTable->RemoveAll(NULL);
	} else {
		ItemExclTable = new Variables();
		ItemExclTable->SetType(GEM_VARIABLES_INT);
	}

	AutoTable aa;

	//don't report error when the file doesn't exist
	if (aa.load("itemexcl")) {
		idx = aa->GetRowCount();
		while (idx--) {
			ieResRef key;

			strnlwrcpy(key,aa->GetRowName(idx),8);
			ieDword value = strtol(aa->QueryField(idx,0),NULL,0);
			ItemExclTable->SetAt(key, value);
		}
	}
	if (ItemDialTable) {
		ItemDialTable->RemoveAll(NULL);
	} else {
		ItemDialTable = new Variables();
		ItemDialTable->SetType(GEM_VARIABLES_INT);
	}
	if (ItemDial2Table) {
		ItemDial2Table->RemoveAll(NULL);
	} else {
		ItemDial2Table = new Variables();
		ItemDial2Table->SetType(GEM_VARIABLES_STRING);
	}

	//don't report error when the file doesn't exist
	if (aa.load("itemdial")) {
		idx = aa->GetRowCount();
		while (idx--) {
			ieResRef key, dlgres;

			strnlwrcpy(key,aa->GetRowName(idx),8);
			ieDword value = strtol(aa->QueryField(idx,0),NULL,0);
			ItemDialTable->SetAt(key, value);
			strnlwrcpy(dlgres,aa->QueryField(idx,1),8);
			ItemDial2Table->SetAtCopy(key, dlgres);
		}
	}

	if (ItemTooltipTable) {
		ItemTooltipTable->RemoveAll(ReleaseItemTooltip);
	} else {
		ItemTooltipTable = new Variables();
		ItemTooltipTable->SetType(GEM_VARIABLES_POINTER);
	}

	//don't report error when the file doesn't exist
	if (aa.load("tooltip")) {
		idx = aa->GetRowCount();
		while (idx--) {
			ieResRef key;
			int *tmppoi = (int *) malloc(sizeof(int)*3);

			strnlwrcpy(key,aa->GetRowName(idx),8);
			for (int i=0;i<3;i++) {
				tmppoi[i] = atoi(aa->QueryField(idx,i));
			}
			ItemTooltipTable->SetAt(key, (void*)tmppoi);
		}
	}
	return flag;
}

//Static
const char *Interface::GetDeathVarFormat()
{
	return DeathVarFormat;
}

int Interface::GetItemExcl(const ieResRef itemname) const
{
	ieDword value;

	if (ItemExclTable && ItemExclTable->Lookup(itemname, value)) {
		return (int) value;
	}
	return 0;
}

int Interface::GetItemTooltip(const ieResRef itemname, int header, int identified)
{
	int *value = NULL;

	if (ItemTooltipTable) {
		void* lookup = NULL;
		ItemTooltipTable->Lookup(itemname, lookup);
		value = (int*)lookup;
	}
	if (value && (value[header]>=0)) {
		return value[header];
	}
	Item *item = gamedata->GetItem(itemname);
	if (!item) {
		return -1;
	}
	int ret = identified?item->ItemNameIdentified:item->ItemName;
	gamedata->FreeItem(item, itemname, 0);
	return ret;
}

int Interface::GetItemDialStr(const ieResRef itemname) const
{
	ieDword value;

	if (ItemDialTable && ItemDialTable->Lookup(itemname, value)) {
		return (int) value;
	}
	return -1;
}

//second value is the item dialog resource returned by this method
int Interface::GetItemDialRes(const ieResRef itemname, ieResRef retval) const
{
	if (ItemDial2Table && ItemDial2Table->Lookup(itemname, retval, sizeof(ieResRef))) {
		return 1;
	}
	return 0;
}

bool Interface::ReadAreaAliasTable(const ieResRef tablename)
{
	if (AreaAliasTable) {
		AreaAliasTable->RemoveAll(NULL);
	} else {
		AreaAliasTable = new Variables();
		AreaAliasTable->SetType(GEM_VARIABLES_INT);
	}

	AutoTable aa(tablename);
	if (!aa) {
		//don't report error when the file doesn't exist
		return true;
	}

	int idx = aa->GetRowCount();
	while (idx--) {
		ieResRef key;

		strnlwrcpy(key,aa->GetRowName(idx),8);
		ieDword value = atoi(aa->QueryField(idx,0));
		AreaAliasTable->SetAt(key, value);
	}
	return true;
}

//this isn't const
int Interface::GetAreaAlias(const ieResRef areaname) const
{
	ieDword value;

	if (AreaAliasTable && AreaAliasTable->Lookup(areaname, value)) {
		return (int) value;
	}
	return -1;
}

bool Interface::ReadMusicTable(const ieResRef tablename, int col) {
	AutoTable tm(tablename);
	if (!tm)
		return false;

	for (unsigned int i = 0; i < tm->GetRowCount(); i++) {
		musiclist.push_back(strdup(tm->QueryField(i, col)));
	}

	return true;
}

bool Interface::ReadDamageTypeTable() {
	AutoTable tm("dmgtypes");
	if (!tm)
		return false;

	DamageInfoStruct di;
	for (ieDword i = 0; i < tm->GetRowCount(); i++) {
		di.strref = GetStringReference(atoi(tm->QueryField(i, 0)));
		di.resist_stat = TranslateStat(tm->QueryField(i, 1));
		di.value = strtol(tm->QueryField(i, 2), (char **) NULL, 16);
		di.iwd_mod_type = atoi(tm->QueryField(i, 3));
		DamageInfoMap.insert(std::make_pair <ieDword, DamageInfoStruct> ((ieDword)di.value, di));
	}

	return true;
}

bool Interface::ReadReputationModTable() {
	AutoTable tm("reputati");
	if (!tm)
		return false;

	reputationmod = (int **) calloc(21, sizeof(int *));
	int cols = tm->GetColumnCount();
	for (unsigned int i=0; i<20; i++) {
		reputationmod[i] = (int *) calloc(cols, sizeof(int));
		for (int j=0; j<cols; j++) {
			reputationmod[i][j] = atoi(tm->QueryField(i, j));
		}
	}

	return true;
}

bool Interface::ReadModalStates()
{
	AutoTable table("modal");
	if (!table)
		return false;

	ModalStatesStruct ms;
	for (unsigned short i = 0; i < table->GetRowCount(); i++) {
		strncpy(ms.spell, table->QueryField(i, 0), 8);
		strncpy(ms.action, table->QueryField(i, 1), 16);
		ms.entering_str = atoi(table->QueryField(i, 2));
		ms.leaving_str = atoi(table->QueryField(i, 3));
		ms.failed_str = atoi(table->QueryField(i, 4));
		ms.aoe_spell = atoi(table->QueryField(i, 5));
		ModalStates.push_back(ms);
	}

	return true;
}

//Not a constant anymore, we let the caller set the entry to zero
char *Interface::GetMusicPlaylist(int SongType) const {
	if (SongType < 0 || (unsigned int)SongType >= musiclist.size())
		return NULL;

	return musiclist[SongType];
}

static const Color white = {0xff,0xff,0xff,0xff};
static const Color black = {0x00,0x00,0x00,0xff};
static const Region bg( 0, 0, 100, 30 );

/** this is the main loop */
void Interface::Main()
{
	ieDword brightness = 10;
	ieDword contrast = 5;
	ieDword speed = 10;

	vars->Lookup("Full Screen", FullScreen);
	video->CreateDisplay( Width, Height, Bpp, FullScreen);
	video->SetDisplayTitle( GameName, GameType );
	vars->Lookup("Brightness Correction", brightness);
	vars->Lookup("Gamma Correction", contrast);
	vars->Lookup("Mouse Scroll Speed", speed);
	video->SetGamma(brightness, contrast);
	SetMouseScrollSpeed((int) speed);
	if (vars->Lookup("Tooltips", TooltipDelay)) {
		// the games store the slider position*10, not the actual delay
		TooltipDelay *= TOOLTIP_DELAY_FACTOR/10;
	}

	Font* fps = GetFont( ( unsigned int ) 0 );
	char fpsstring[40]={"???.??? fps"};
	unsigned long frame = 0, time, timebase;
	GetTime(timebase);
	double frames = 0.0;
	Palette* palette = CreatePalette( white, black );
	do {
		//don't change script when quitting is pending

		while (QuitFlag) {
			HandleFlags();
		}
		//eventflags are processed only when there is a game
		if (EventFlag && game) {
			HandleEvents();
		}
		HandleGUIBehaviour();

		GameLoop();
		DrawWindows();
		if (DrawFPS) {
			frame++;
			GetTime( time );
			if (time - timebase > 1000) {
				frames = ( frame * 1000.0 / ( time - timebase ) );
				timebase = time;
				frame = 0;
				sprintf( fpsstring, "%.3f fps", frames );
			}
			video->DrawRect( bg, black );
			fps->Print( bg,
				( unsigned char * ) fpsstring, palette,
				IE_FONT_ALIGN_LEFT | IE_FONT_ALIGN_MIDDLE, true );
		}
		if (TickHook)
			TickHook->call();
	} while (video->SwapBuffers() == GEM_OK);
	gamedata->FreePalette( palette );
}

int Interface::ReadResRefTable(const ieResRef tablename, ieResRef *&data)
{
	int count = 0;

	if (data) {
		free(data);
		data = NULL;
	}
	AutoTable tm(tablename);
	if (!tm) {
		printStatus( "ERROR", LIGHT_RED );
		printf( "Cannot find %s.2da.\n",tablename );
		return 0;
	}
	count = tm->GetRowCount();
	data = (ieResRef *) calloc( count, sizeof(ieResRef) );
	for (int i = 0; i < count; i++) {
		strnlwrcpy( data[i], tm->QueryField( i, 0 ), 8 );
		//* marks an empty resource
		if (data[i][0]=='*') {
			data[i][0]=0;
		}
	}
	return count;
}

int Interface::LoadSprites()
{
	ieDword i;
	int size;
	if (!IsAvailable( IE_2DA_CLASS_ID )) {
		printf( "No 2DA Importer Available.\nTermination in Progress...\n" );
		return GEM_ERROR;
	}

	//loading cursors
	AnimationFactory* anim;
	anim = (AnimationFactory*) gamedata->GetFactoryResource("cursors", IE_BAM_CLASS_ID);
	if (anim)
	{
		CursorCount = anim->GetCycleCount();
		Cursors = new Sprite2D * [CursorCount];
		for (int i = 0; i < CursorCount; i++) {
			Cursors[i] = anim->GetFrame( 0, (ieByte) i );
		}
	}
	printMessage( "Core", "Loading Cursors...", WHITE );

	// this is the last existing cursor type
	if (CursorCount<IE_CURSOR_WAY) {
		printStatus( "ERROR", LIGHT_RED );
		return GEM_ERROR;
	}
	video->SetCursor( Cursors[0], Cursors[1] );
	printStatus( "OK", LIGHT_GREEN );

	// Load fog-of-war bitmaps
	anim = (AnimationFactory*) gamedata->GetFactoryResource("fogowar", IE_BAM_CLASS_ID);
	printMessage( "Core", "Loading Fog-Of-War bitmaps...", WHITE );
	if (!anim || anim->GetCycleSize( 0 ) != 8) {
		// unknown type of fog anim
		printStatus( "ERROR", LIGHT_RED );
		return GEM_ERROR;
	}

	FogSprites[0] = NULL;
	FogSprites[1] = anim->GetFrame( 0, 0 );
	FogSprites[2] = anim->GetFrame( 1, 0 );
	FogSprites[3] = anim->GetFrame( 2, 0 );

	FogSprites[4] = video->MirrorSpriteVertical( FogSprites[1], false );

	FogSprites[5] = NULL;

	FogSprites[6] = video->MirrorSpriteVertical( FogSprites[3], false );

	FogSprites[7] = NULL;

	FogSprites[8] = video->MirrorSpriteHorizontal( FogSprites[2], false );

	FogSprites[9] = video->MirrorSpriteHorizontal( FogSprites[3], false );

	FogSprites[10] = NULL;
	FogSprites[11] = NULL;

	FogSprites[12] = video->MirrorSpriteHorizontal( FogSprites[6], false );

	FogSprites[16] = anim->GetFrame( 3, 0 );
	FogSprites[17] = anim->GetFrame( 4, 0 );
	FogSprites[18] = anim->GetFrame( 5, 0 );
	FogSprites[19] = anim->GetFrame( 6, 0 );

	FogSprites[20] = video->MirrorSpriteVertical( FogSprites[17], false );

	FogSprites[21] = NULL;

	FogSprites[23] = NULL;

	FogSprites[24] = video->MirrorSpriteHorizontal( FogSprites[18], false );

	FogSprites[25] = anim->GetFrame( 7, 0 );

	{
		Sprite2D *tmpsprite = video->MirrorSpriteVertical( FogSprites[25], false );
		FogSprites[22] = video->MirrorSpriteHorizontal( tmpsprite, false );
		video->FreeSprite( tmpsprite );
	}

	FogSprites[26] = NULL;
	FogSprites[27] = NULL;

	{
		Sprite2D *tmpsprite = video->MirrorSpriteVertical( FogSprites[19], false );
		FogSprites[28] = video->MirrorSpriteHorizontal( tmpsprite, false );
		video->FreeSprite( tmpsprite );
	}

	i = 0;
	vars->Lookup("3D Acceleration", i);
	if (i) {
		for(i=0;i<sizeof(FogSprites)/sizeof(Sprite2D *);i++ ) {
			if (FogSprites[i]) {
				Sprite2D* alphasprite = video->CreateAlpha( FogSprites[i] );
				video->FreeSprite ( FogSprites[i] );
				FogSprites[i] = alphasprite;
			}
		}
	}

	printStatus( "OK", LIGHT_GREEN );

	// Load ground circle bitmaps (PST only)
	//block required due to msvc6.0 incompatibility
	for (size = 0; size < MAX_CIRCLE_SIZE; size++) {
		if (GroundCircleBam[size][0]) {
			anim = (AnimationFactory*) gamedata->GetFactoryResource(GroundCircleBam[size], IE_BAM_CLASS_ID);
			if (!anim || anim->GetCycleCount() != 6) {
				// unknown type of circle anim
				printMessage( "Core", "Loading Ground circle bitmaps...", WHITE );
				printStatus( "ERROR", LIGHT_RED );
				return GEM_ERROR;
			}

			for (int i = 0; i < 6; i++) {
				Sprite2D* sprite = anim->GetFrame( 0, (ieByte) i );
				if (GroundCircleScale[size]) {
					GroundCircles[size][i] = video->SpriteScaleDown( sprite, GroundCircleScale[size] );
					video->FreeSprite( sprite );
				} else {
					GroundCircles[size][i] = sprite;
				}
			}
		}
	}

	printMessage( "Core", "Loading Ground circle bitmaps...", WHITE );
	printStatus( "OK", LIGHT_GREEN );

	printMessage( "Core", "Loading Fonts...\n", WHITE );
	AutoTable tab("fonts");
	if (!tab) {
		printStatus( "ERROR", LIGHT_RED );
		printf( "Cannot find fonts.2da.\nTermination in Progress...\n" );
		return GEM_ERROR;
	} else {
		PluginHolder<AnimationMgr> bamint(IE_BAM_CLASS_ID);
		if (!bamint) {
			printStatus( "ERROR", LIGHT_RED );
			printf( "No BAM Importer Available.\nTermination in Progress...\n" );
			return GEM_ERROR;
		}
		DataStream* str = NULL;

		int count = tab->GetRowCount();
		for (int i = 0; i < count; i++) {
			const char* ResRef = tab->QueryField( i, 0 );
			int needpalette = atoi( tab->QueryField( i, 1 ) );
			int first_char = atoi( tab->QueryField( i, 2 ) );
			str = gamedata->GetResource( ResRef, IE_BAM_CLASS_ID );
			if (!bamint->Open( str, true )) {
				continue;
			}
			Font* fnt = bamint->GetFont();
			if (!fnt) {
				continue;
			}
			strnlwrcpy( fnt->ResRef, ResRef, 8 );
			if (needpalette) {

				Color fore = {0xff, 0xff, 0xff, 0};
				Color back = {0x00, 0x00, 0x00, 0};
				if (!strnicmp( TooltipFont, ResRef, 8) ) {
					if (TooltipColor.a==0xff) {
						fore = TooltipColor;
					} else {
						fore = back;
						back = TooltipColor;
					}
				}
				Palette* pal = CreatePalette( fore, back );
				pal->CreateShadedAlphaChannel();
				fnt->SetPalette(pal);
				gamedata->FreePalette( pal );
			}
			fnt->SetFirstChar( (ieByte) first_char );
			fonts.push_back( fnt );
		}

		if (fonts.size() == 0) {
			printMessage( "Core", "No default font loaded! ", WHITE );
			printStatus( "ERROR", LIGHT_RED );
			return GEM_ERROR;
		}
		if (GetFont( ButtonFont ) == NULL) {
			printMessage( "Core", "ButtonFont not loaded: ", WHITE );
			printf("%s ", ButtonFont);
			printStatus( "WARNING", YELLOW );
		}
		if (GetFont( MovieFont ) == NULL) {
			printMessage( "Core", "MovieFont not loaded: ", WHITE );
			printf("%s ", MovieFont);
			printStatus( "WARNING", YELLOW );
		}
		if (GetFont( TooltipFont ) == NULL) {
			printMessage( "Core", "TooltipFont not loaded: ", WHITE );
			printf("%s ", TooltipFont);
			printStatus( "WARNING", YELLOW );
		}
	}
	printMessage( "Core", "Fonts Loaded...", WHITE );
	printStatus( "OK", LIGHT_GREEN );

	if (TooltipBackResRef[0]) {
		anim = (AnimationFactory*) gamedata->GetFactoryResource(TooltipBackResRef, IE_BAM_CLASS_ID);
		printMessage( "Core", "Initializing Tooltips...", WHITE );
		if (!anim) {
			printStatus( "ERROR", LIGHT_RED );
			return GEM_ERROR;
		}
		TooltipBack = new Sprite2D * [3];
		for (int i = 0; i < 3; i++) {
			TooltipBack[i] = anim->GetFrame( 0, (ieByte) i );
			TooltipBack[i]->XPos = 0;
			TooltipBack[i]->YPos = 0;
		}
		printStatus( "OK", LIGHT_GREEN );
	}

	return GEM_OK;
}

int Interface::Init()
{
	plugin_flags = new Variables();
	plugin_flags->SetType( GEM_VARIABLES_INT );

	printMessage( "Core", "Initializing the Event Manager...", WHITE );
	evntmgr = new EventMgr();

	printMessage( "Core", "Initializing Lists Dictionary...", WHITE );
	lists = new Variables();
	if (!lists) {
		printStatus( "ERROR", LIGHT_RED );
		return GEM_ERROR;
	}
	lists->SetType( GEM_VARIABLES_POINTER );

	printMessage( "Core", "Initializing Variables Dictionary...", WHITE );
	vars = new Variables();
	if (!vars) {
		printStatus( "ERROR", LIGHT_RED );
		return GEM_ERROR;
	}
	vars->SetType( GEM_VARIABLES_INT );
	vars->ParseKey(true);

	vars->SetAt( "Volume Ambients", 100 );
	vars->SetAt( "Volume Movie", 100 );
	vars->SetAt( "Volume Music", 100 );
	vars->SetAt( "Volume SFX", 100 );
	vars->SetAt( "Volume Voices", 100 );
	printStatus( "OK", LIGHT_GREEN );

	if (!LoadConfig()) {
		return GEM_ERROR;
	}
	printMessage( "Core", "Starting Plugin Manager...\n", WHITE );
	PluginMgr *plugin = PluginMgr::Get();
	plugin->LoadPlugins(PluginsPath);
	if (plugin && plugin->GetPluginCount()) {
		printMessage( "Core", "Plugin Loading Complete...", WHITE );
		printStatus( "OK", LIGHT_GREEN );
	} else {
		printMessage( "Core", "Plugin Loading Failed, check path...", YELLOW);
		printStatus( "ERROR", LIGHT_RED );
		return GEM_ERROR;
	}
	plugin->RunInitializers();

	time_t t;
	t = time( NULL );
	srand( ( unsigned int ) t );
#ifdef _DEBUG
	FileStreamPtrCount = 0;
	CachedFileStreamPtrCount = 0;
#endif
	printMessage( "Core", "GemRB Core Initialization...\n", WHITE );
	printStatus( "OK", LIGHT_GREEN );
	printMessage( "Core", "Initializing Video Driver...", WHITE );
	video = ( Video * ) PluginMgr::Get()->GetDriver(&Video::ID, VideoDriverName.c_str());
	if (!video) {
		printStatus( "ERROR", LIGHT_RED );
		printf( "No Video Driver Available.\nTermination in Progress...\n" );
		return GEM_ERROR;
	}
	if (video->Init() == GEM_ERROR) {
		printStatus( "ERROR", LIGHT_RED );
		printf( "Cannot Initialize Video Driver.\nTermination in Progress...\n" );
		return GEM_ERROR;
	}
	Color defcolor={255,255,255,200};
	SetInfoTextColor(defcolor);
	printStatus( "OK", LIGHT_GREEN );

	{
		printMessage( "Core", "Initializing Search Path...", WHITE );
		if (!IsAvailable( PLUGIN_RESOURCE_DIRECTORY )) {
			printf( "no DirectoryImporter! " );
			printStatus( "ERROR", LIGHT_RED );
			return GEM_ERROR;
		}

		char path[_MAX_PATH];

		PathJoin( path, CachePath, NULL);
		gamedata->AddSource(path, "Cache", PLUGIN_RESOURCE_DIRECTORY);

		size_t i;
		for (i = 0; i < ModPath.size(); ++i)
			gamedata->AddSource(ModPath[i].c_str(), "Mod paths", PLUGIN_RESOURCE_DIRECTORY);

		PathJoin( path, GemRBOverridePath, "override", "shared", NULL);
		gamedata->AddSource(path, "shared GemRB Override", PLUGIN_RESOURCE_DIRECTORY);

		PathJoin( path, GamePath, GameOverridePath, NULL);
		gamedata->AddSource(path, "Override", PLUGIN_RESOURCE_DIRECTORY);

		PathJoin( path, GamePath, GameSoundsPath, NULL);
		gamedata->AddSource(path, "Sounds", PLUGIN_RESOURCE_DIRECTORY);

		PathJoin( path, GamePath, GamePortraitsPath, NULL);
		gamedata->AddSource(path, "Portraits", PLUGIN_RESOURCE_DIRECTORY);

		PathJoin( path, GamePath, GameDataPath, NULL);
		gamedata->AddSource(path, "Data", PLUGIN_RESOURCE_DIRECTORY);

		//IWD2 movies are on the CD but not in the BIF
		char *description = strdup("CD1/data");
		for (i = 0; i < MAX_CD; i++) {
			for (size_t j=0;j<CD[i].size();j++) {
				description[2]='1'+i;				
				PathJoin( path, CD[i][j].c_str(), GameDataPath, NULL);
				gamedata->AddSource(path, description, PLUGIN_RESOURCE_DIRECTORY);
			}
		}
		free(description);

		printStatus( "OK", LIGHT_GREEN );
	}

	{
		printMessage( "Core", "Initializing KEY Importer...", WHITE );
		char ChitinPath[_MAX_PATH];
		PathJoin( ChitinPath, GamePath, "chitin.key", NULL );
		if (!gamedata->AddSource(ChitinPath, "chitin.key", PLUGIN_RESOURCE_KEY)) {
			printStatus( "ERROR", LIGHT_RED );
			return GEM_ERROR;
		}
		printStatus( "OK", LIGHT_GREEN );
	}

	printMessage( "Core", "Initializing GUI Script Engine...", WHITE );
	guiscript = PluginHolder<ScriptEngine>(IE_GUI_SCRIPT_CLASS_ID);
	if (guiscript == NULL) {
		printStatus( "ERROR", LIGHT_RED );
		return GEM_ERROR;
	}
	if (!guiscript->Init()) {
		printStatus( "ERROR", LIGHT_RED );
		return GEM_ERROR;
	}
	printStatus( "OK", LIGHT_GREEN );
	strcpy( NextScript, "Start" );


	char path[_MAX_PATH];
	PathJoin( path, GemRBOverridePath, "override", GameType, NULL);
	gamedata->AddSource(path, "GemRB Override", PLUGIN_RESOURCE_DIRECTORY);


	printMessage( "Core", "Reading Game Options...\n", WHITE );
	if (!LoadGemRBINI()) {
		printf( "Cannot Load INI\nTermination in Progress...\n" );
		return GEM_ERROR;
	}

	//loading baldur.ini
	if (!IgnoreOriginalINI) {
		char ini_path[_MAX_PATH];
		PathJoin( ini_path, GamePath, INIConfig, NULL );
		LoadINI( ini_path );
	}

	int i;
	for (i = 0; i < 8; i++) {
		if (INIConfig[i] == '.')
			break;
		GameNameResRef[i] = INIConfig[i];
	}
	GameNameResRef[i] = 0;

	printMessage( "Core", "Creating Projectile Server...\n", WHITE );
	projserv = new ProjectileServer();
	if (!projserv->GetHighestProjectileNumber()) {
		printStatus( "ERROR", LIGHT_RED );
		printf( "No projectiles are available...\n" );
	}

	printMessage( "Core", "Checking for Dialogue Manager...", WHITE );
	if (!IsAvailable( IE_TLK_CLASS_ID )) {
		printStatus( "ERROR", LIGHT_RED );
		printf( "No TLK Importer Available.\nTermination in Progress...\n" );
		return GEM_ERROR;
	}
	printStatus( "OK", LIGHT_GREEN );
	strings = PluginHolder<StringMgr>(IE_TLK_CLASS_ID);
	printMessage( "Core", "Loading Dialog.tlk file...", WHITE );
	char strpath[_MAX_PATH];
	PathJoin( strpath, GamePath, dialogtlk, NULL );
	FileStream* fs = new FileStream();
	if (!fs->Open( strpath, true )) {
		printStatus( "ERROR", LIGHT_RED );
		printf( "Cannot find Dialog.tlk.\nTermination in Progress...\n" );
		delete fs;
		return GEM_ERROR;
	}
	printStatus( "OK", LIGHT_GREEN );
	strings->Open( fs, true );

	{
		printMessage( "Core", "Loading Palettes...\n", WHITE );
		ResourceHolder<ImageMgr> pal16im(Palette16);
		if (pal16im)
			pal16 = pal16im->GetImage();
		ResourceHolder<ImageMgr> pal32im(Palette32);
		if (pal32im)
			pal32 = pal32im->GetImage();
		ResourceHolder<ImageMgr> pal256im(Palette256);
		if (pal256im)
			pal256 = pal256im->GetImage();
		if (!pal16 || !pal32 || !pal256) {
			printStatus( "ERROR", LIGHT_RED );
			return GEM_ERROR;
		}
		printMessage( "Core", "Palettes Loaded\n", WHITE );
	}

	if (!IsAvailable( IE_BAM_CLASS_ID )) {
		printStatus( "ERROR", LIGHT_RED );
		printf( "No BAM Importer Available.\nTermination in Progress...\n" );
		return GEM_ERROR;
	}

	printMessage( "Core", "Initializing stock sounds...\n", WHITE );
	DSCount = ReadResRefTable ("defsound", DefSound);
	if (DSCount == 0) {
		printStatus( "ERROR", LIGHT_RED );
		printf( "Cannot find defsound.2da.\nTermination in Progress...\n" );
		return GEM_ERROR;
	}

	printStatus( "OK", LIGHT_GREEN );
	printMessage( "Core", "Broadcasting Event Manager...", WHITE );
	video->SetEventMgr( evntmgr );
	printStatus( "OK", LIGHT_GREEN );
	printMessage( "Core", "Initializing Window Manager...", WHITE );
	windowmgr = PluginHolder<WindowMgr>(IE_CHU_CLASS_ID);
	if (windowmgr == NULL) {
		printStatus( "ERROR", LIGHT_RED );
		return GEM_ERROR;
	}
	printStatus( "OK", LIGHT_GREEN );

	int ret = LoadSprites();
	if (ret) return ret;

	printMessage( "Core", "Setting up the Console...", WHITE );
	QuitFlag = QF_CHANGESCRIPT;
	console = new Console();
	console->XPos = 0;
	console->YPos = (ieWord) (Height - 25);
	console->Width = (ieWord) Width;
	console->Height = 25;
	if (fonts.size() > 0) {
		console->SetFont( fonts[0] );
	}

	Sprite2D *tmpsprite = GetCursorSprite();
	if (!tmpsprite) {
		printStatus( "ERROR", LIGHT_RED );
		return GEM_ERROR;
	}
	console->SetCursor (tmpsprite);
	printStatus( "OK", LIGHT_GREEN );

	printMessage( "Core", "Starting up the Sound Driver...", WHITE );
	AudioDriver = ( Audio * ) PluginMgr::Get()->GetDriver(&Audio::ID, AudioDriverName.c_str());
	if (AudioDriver == NULL) {
		printStatus( "ERROR", LIGHT_RED );
		return GEM_ERROR;
	}
	if (!AudioDriver->Init()) {
		printStatus( "ERROR", LIGHT_RED );
		return GEM_ERROR;
	}
	printStatus( "OK", LIGHT_GREEN );

	printMessage( "Core", "Allocating SaveGameIterator...", WHITE );
	sgiterator = new SaveGameIterator();
	if (sgiterator == NULL) {
		printStatus( "ERROR", LIGHT_RED );
		return GEM_ERROR;
	}
	printStatus( "OK", LIGHT_GREEN );

	//no need of strdup, variables do copy the key!
	vars->SetAt( "SkipIntroVideos", (unsigned long)SkipIntroVideos );
	vars->SetAt( "GUIEnhancements", (unsigned long)GUIEnhancements );

	printMessage( "Core", "Initializing Token Dictionary...", WHITE );
	tokens = new Variables();
	if (!tokens) {
		printStatus( "ERROR", LIGHT_RED );
		return GEM_ERROR;
	}
	tokens->SetType( GEM_VARIABLES_STRING );
	printStatus( "OK", LIGHT_GREEN );

	printMessage( "Core", "Initializing Music Manager...", WHITE );
	music = PluginHolder<MusicMgr>(IE_MUS_CLASS_ID);
	if (!music) {
		printStatus( "ERROR", LIGHT_RED );
		return GEM_ERROR;
	}
	printStatus( "OK", LIGHT_GREEN );

	printMessage("Core", "Loading music list...\n", WHITE );
	if (HasFeature( GF_HAS_SONGLIST )) {
		ret = ReadMusicTable("songlist", 1);
	} else {
		/*since bg1 and pst has no .2da for songlist,
		we must supply one in the gemrb/override folder.
		It should be: music.2da, first column is a .mus filename*/
		ret = ReadMusicTable("music", 0);
	}
	if (ret) {
		printStatus( "OK", LIGHT_GREEN );
	} else {
		printStatus( "NOT FOUND", YELLOW );
	}

	if (HasFeature( GF_RESDATA_INI )) {
		printMessage( "Core", "Loading resource data File...", WHITE );
		INIresdata = PluginHolder<DataFileMgr>(IE_INI_CLASS_ID);
		DataStream* ds = gamedata->GetResource("resdata", IE_INI_CLASS_ID);
		if (!INIresdata->Open( ds, true )) {
			printStatus( "ERROR", LIGHT_RED );
		} else {
			printStatus( "OK", LIGHT_GREEN );
		}
	}

	if (HasFeature( GF_HAS_PARTY_INI )) {
		printMessage( "Core", "Loading precreated teams setup...\n",
			WHITE );
		INIparty = PluginHolder<DataFileMgr>(IE_INI_CLASS_ID);
		FileStream* fs = new FileStream();
		char tINIparty[_MAX_PATH];
		PathJoin( tINIparty, GamePath, "Party.ini", NULL );
		fs->Open( tINIparty, true );
		if (!INIparty->Open( fs, true )) {
			printStatus( "ERROR", LIGHT_RED );
		} else {
			printStatus( "OK", LIGHT_GREEN );
		}
	}

	if (HasFeature(GF_IWD2_DEATHVARFORMAT)) {
		memcpy(DeathVarFormat, IWD2DeathVarFormat, sizeof(ieVariable));
	}

	if (HasFeature( GF_HAS_BEASTS_INI )) {
		printMessage( "Core", "Loading beasts definition File...\n",
			WHITE );
		INIbeasts = PluginHolder<DataFileMgr>(IE_INI_CLASS_ID);
		FileStream* fs = new FileStream();
		char tINIbeasts[_MAX_PATH];
		PathJoin( tINIbeasts, GamePath, "beast.ini", NULL );
		// FIXME: crashes if file does not open
		fs->Open( tINIbeasts, true );
		if (!INIbeasts->Open( fs, true )) {
			printStatus( "ERROR", LIGHT_RED );
		} else {
			printStatus( "OK", LIGHT_GREEN );
		}

		printMessage( "Core", "Loading quests definition File...\n",
			WHITE );
		INIquests = PluginHolder<DataFileMgr>(IE_INI_CLASS_ID);
		FileStream* fs2 = new FileStream();
		char tINIquests[_MAX_PATH];
		PathJoin( tINIquests, GamePath, "quests.ini", NULL );
		// FIXME: crashes if file does not open
		fs2->Open( tINIquests, true );
		if (!INIquests->Open( fs2, true )) {
			printStatus( "ERROR", LIGHT_RED );
		} else {
			printStatus( "OK", LIGHT_GREEN );
		}
	}
	game = NULL;
	calendar = NULL;

	timer = new GlobalTimer();
	printMessage( "Core", "Bringing up the Global Timer...", WHITE );
	if (!timer) {
		printStatus( "ERROR", LIGHT_RED );
		return GEM_ERROR;
	}
	printStatus( "OK", LIGHT_GREEN );

	ret = Init_EffectQueue();
	printMessage( "Core", "Initializing effects...", WHITE );
	if (!ret) {
		printStatus( "ERROR", LIGHT_RED );
		return GEM_ERROR;
	}
	printStatus( "OK", LIGHT_GREEN );

	ret = InitItemTypes();
	printMessage( "Core", "Initializing Inventory Management...", WHITE );
	if (!ret) {
		printStatus( "ERROR", LIGHT_RED );
		return GEM_ERROR;
	}
	printStatus( "OK", LIGHT_GREEN );

	printMessage( "Core", "Initializing string constants...", WHITE );
	if (!Init_DisplayString()) {
		printStatus( "ERROR", LIGHT_RED );
		return GEM_ERROR;
	}
	printStatus( "OK", LIGHT_GREEN );

	ret = ReadRandomItems();
	printMessage( "Core", "Initializing random treasure...", WHITE );
	if (ret) {
		printStatus( "OK", LIGHT_GREEN );
	}
	else {
		printStatus( "ERROR", LIGHT_RED );
	}

	ret = ReadAbilityTables();
	printMessage( "Core", "Initializing ability tables...", WHITE );
	if (!ret) {
		printStatus( "ERROR", LIGHT_RED );
		return GEM_ERROR;
	}
	printStatus( "OK", LIGHT_GREEN );

	ret = ReadReputationModTable();
	printMessage( "Core", "Reading reputation mod table...", WHITE);
	if (ret) {
		printStatus( "OK", LIGHT_GREEN );
	} else {
		printStatus( "NOT FOUND", LIGHT_RED );
	}

	if ( gamedata->Exists("WMAPLAY", IE_2DA_CLASS_ID) ) {
		ret = ReadAreaAliasTable( "WMAPLAY" );
		printMessage( "Core", "Initializing area aliases...", WHITE );
		if (ret) {
			printStatus( "OK", LIGHT_GREEN );
		}
		else {
			printStatus( "NOT FOUND", YELLOW );
		}
	}

	ret = ReadGameTimeTable();
	printMessage( "Core", "Reading game time table...", WHITE);
	if (!ret) {
		printStatus( "ERROR", LIGHT_RED );
		return GEM_ERROR;
	}
	printStatus( "OK", LIGHT_GREEN );

	ret = ReadSpecialSpells();
	printMessage( "Core", "Reading special spells table...", WHITE);
	if (ret) {
		printStatus( "OK", LIGHT_GREEN );
	} else {
		printStatus( "NOT FOUND", YELLOW );
	}

	ret = ReadAuxItemTables();
	printMessage( "Core", "Reading item tables...", WHITE);
	if (!ret) {
		printStatus( "ERROR", LIGHT_RED );
		return GEM_ERROR;
	}
	printStatus( "OK", LIGHT_GREEN );

	ret = ReadDamageTypeTable();
	printMessage( "Core", "Reading damage type table...", WHITE);
	if (!ret) {
		printStatus( "ERROR", LIGHT_RED );
	} else {
		printStatus( "OK", LIGHT_GREEN );
	}

	ret = ReadModalStates();
	printMessage( "Core", "Reading modal states table...", WHITE);
	if (!ret) {
		printStatus( "ERROR", LIGHT_RED );
		return GEM_ERROR;
	} else {
		printStatus( "OK", LIGHT_GREEN );
	}

	printMessage( "Core", "Reading game script tables...", WHITE);
	InitializeIEScript();
	printStatus( "OK", LIGHT_GREEN );

	printMessage( "Core", "Core Initialization Complete!\n", WHITE );

	return GEM_OK;
}

bool Interface::IsAvailable(SClass_ID filetype) const
{
	return PluginMgr::Get()->IsAvailable( filetype );
}

WorldMap *Interface::GetWorldMap(const char *map)
{
	int index = worldmap->FindAndSetCurrentMap(map?map:game->CurrentArea);
	return worldmap->GetWorldMap(index);
}

ProjectileServer* Interface::GetProjectileServer() const
{
	return projserv;
}

Video* Interface::GetVideoDriver() const
{
	return video.get();
}

Audio* Interface::GetAudioDrv(void) const {
	return AudioDriver.get();
}

const char* Interface::TypeExt(SClass_ID type) const
{
	switch (type) {
		case IE_2DA_CLASS_ID:
			return ".2da";

		case IE_ACM_CLASS_ID:
			return ".acm";

		case IE_ARE_CLASS_ID:
			return ".are";

		case IE_BAM_CLASS_ID:
			return ".bam";

		case IE_BCS_CLASS_ID:
			return ".bcs";

		case IE_BS_CLASS_ID:
			return ".bs";

		case IE_BIF_CLASS_ID:
			return ".bif";

		case IE_BIO_CLASS_ID:
			if (HasFeature(GF_BIOGRAPHY_RES)) {
				return ".res";
			}
			return ".bio";

		case IE_BMP_CLASS_ID:
			return ".bmp";

		case IE_PNG_CLASS_ID:
			return ".png";

		case IE_CHR_CLASS_ID:
			return ".chr";

		case IE_CHU_CLASS_ID:
			return ".chu";

		case IE_CRE_CLASS_ID:
			return ".cre";

		case IE_DLG_CLASS_ID:
			return ".dlg";

		case IE_EFF_CLASS_ID:
			return ".eff";

		case IE_GAM_CLASS_ID:
			return ".gam";

		case IE_IDS_CLASS_ID:
			return ".ids";

		case IE_INI_CLASS_ID:
			return ".ini";

		case IE_ITM_CLASS_ID:
			return ".itm";

		case IE_MOS_CLASS_ID:
			return ".mos";

		case IE_MUS_CLASS_ID:
			return ".mus";

		case IE_MVE_CLASS_ID:
			return ".mve";

		case IE_OGG_CLASS_ID:
			return ".ogg";

		case IE_PLT_CLASS_ID:
			return ".plt";

		case IE_PRO_CLASS_ID:
			return ".pro";

		case IE_SAV_CLASS_ID:
			return ".sav";

		case IE_SPL_CLASS_ID:
			return ".spl";

		case IE_SRC_CLASS_ID:
			return ".src";

		case IE_STO_CLASS_ID:
			return ".sto";

		case IE_TIS_CLASS_ID:
			return ".tis";

		case IE_TLK_CLASS_ID:
			return ".tlk";

		case IE_TOH_CLASS_ID:
			return ".toh";

		case IE_TOT_CLASS_ID:
			return ".tot";

		case IE_VAR_CLASS_ID:
			return ".var";

		case IE_VVC_CLASS_ID:
			return ".vvc";

		case IE_WAV_CLASS_ID:
			return ".wav";

		case IE_WED_CLASS_ID:
			return ".wed";

		case IE_WFX_CLASS_ID:
			return ".wfx";

		case IE_WMP_CLASS_ID:
			return ".wmp";
	}
	return NULL;
}

void Interface::FreeString(char *&str) const
{
	if (str) {
		strings->FreeString(str);
	}
	str = NULL;
}

ieStrRef Interface::UpdateString(ieStrRef strref, const char *text) const
{
	return strings->UpdateString( strref, text );
}

char* Interface::GetString(ieStrRef strref, ieDword options) const
{
	ieDword flags = 0;

	if (!(options & IE_STR_STRREFOFF)) {
		vars->Lookup( "Strref On", flags );
	}
	return strings->GetString( strref, flags | options );
}

void Interface::SetFeature(int flag, int position)
{
	if (flag) {
		GameFeatures[position>>5] |= 1<<(position&31);
	} else {
		GameFeatures[position>>5] &= ~(1<<(position&31) );
	}
}

ieDword Interface::HasFeature(int position) const
{
	return GameFeatures[position>>5] & (1<<(position&31));
}

/** Search directories and load a config file */
bool Interface::LoadConfig(void)
{
#ifndef WIN32
	char path[_MAX_PATH];
	char name[_MAX_PATH];

	// Find directory where user stores GemRB configurations (~/.gemrb).
	// FIXME: Create it if it does not exist
	// Use current dir if $HOME is not defined (or bomb out??)

	char* s = getenv( "HOME" );
	if (s) {
		strcpy( UserDir, s );
		strcat( UserDir, "/."PACKAGE"/" );
	} else {
		strcpy( UserDir, "./" );
	}

	// Find basename of this program. It does the same as basename (3),
	// but that's probably missing on some archs
	s = strrchr( argv[0], PathDelimiter );
	if (s) {
		s++;
	} else {
		s = argv[0];
	}

	strcpy( name, s );
	//if (!name[0])		// FIXME: could this happen?
	//	strcpy (name, PACKAGE); // ugly hack

	// If we were called as $0 -c <filename>, load config from filename
	if (argc > 2 && ! strcmp("-c", argv[1])) {
		if (LoadConfig( argv[2] )) {
			return true;
		} else {
			// Explicitly specified cfg file HAS to be present
			return false;
		}
	}

	// FIXME: temporary hack, to be deleted??
	if (LoadConfig( "GemRB.cfg" )) {
		return true;
	}

	PathJoin( path, UserDir, name, NULL );
	strcat( path, ".cfg" );

	if (LoadConfig( path )) {
		return true;
	}

#ifdef SYSCONFDIR
	PathJoin( path, SYSCONFDIR, name, NULL );
	strcat( path, ".cfg" );

	if (LoadConfig( path )) {
		return true;
	}
#endif

	// Don't try with default binary name if we have tried it already
	if (!strcmp( name, PACKAGE )) {
		return false;
	}

	PathJoin( path, UserDir, PACKAGE, NULL );
	strcat( path, ".cfg" );

	if (LoadConfig( path )) {
		return true;
	}

#ifdef SYSCONFDIR
	PathJoin( path, SYSCONFDIR, PACKAGE, NULL );
	strcat( path, ".cfg" );

	if (LoadConfig( path )) {
		return true;
	}
#endif

	return false;
#else // WIN32
	// If we were called as $0 -c <filename>, load config from filename
	if (argc > 2 && ! strcmp("-c", argv[1])) {
		return LoadConfig( argv[2] );
		// Explicitly specified cfg file HAS to be present
	}
	strcpy( UserDir, ".\\" );
	return LoadConfig( "GemRB.cfg" );
#endif// WIN32
}

bool Interface::LoadConfig(const char* filename)
{
	FILE* config;
	size_t i;

	printMessage("Config","Trying to open ", WHITE);
	textcolor(LIGHT_WHITE);
	printf("%s ", filename);
	config = fopen( filename, "rb" );
	if (config == NULL) {
		printStatus("NOT FOUND", LIGHT_RED);
		return false;
	}
	char name[65], value[_MAX_PATH + 3];

	//once GemRB own format is working well, this might be set to 0
	SaveAsOriginal = 1;

	while (!feof( config )) {
		char rem;

		if (fread( &rem, 1, 1, config ) != 1)
			break;

		if (rem == '#') {
			//it should always return 0
			if (fscanf( config, "%*[^\r\n]%*[\r\n]" )!=0)
				break;
			continue;
		}
		fseek( config, -1, SEEK_CUR );
		memset(value,'\0',_MAX_PATH + 3);
		//the * element is not counted
		if (fscanf( config, "%64[^= ] = %[^\r\n]%*[\r\n]", name, value )!=2)
			continue;
		for (i=_MAX_PATH + 2; i > 0; i--) {
			if (value[i] == '\0') continue;
			if (value[i] == ' ') {
				value[i] = '\0';
			} else {
				break;
			}
		}

		if (false) {
#define CONFIG_INT(str, var) \
		} else if (stricmp(name, str) == 0) { \
			var ( atoi(value) )
		CONFIG_INT("Bpp", Bpp = );
		CONFIG_INT("CaseSensitive", CaseSensitive = );
		CONFIG_INT("DoubleClickDelay", evntmgr->SetDCDelay);
		CONFIG_INT("DrawFPS", DrawFPS = );
		CONFIG_INT("EnableCheatKeys", EnableCheatKeys);
		CONFIG_INT("EndianSwitch", DataStream::SetEndianSwitch);
		CONFIG_INT("FogOfWar", FogOfWar = );
		CONFIG_INT("FullScreen", FullScreen = );
		CONFIG_INT("GUIEnhancements", GUIEnhancements = );
		CONFIG_INT("GameOnCD", GameOnCD = );
		CONFIG_INT("Height", Height = );
		CONFIG_INT("KeepCache", KeepCache = );
		CONFIG_INT("MultipleQuickSaves", GameControl::MultipleQuickSaves);
		CONFIG_INT("RepeatKeyDelay", evntmgr->SetRKDelay);
		CONFIG_INT("SaveAsOriginal", SaveAsOriginal = );
		CONFIG_INT("ScriptDebugMode", SetScriptDebugMode);
		CONFIG_INT("SkipIntroVideos", SkipIntroVideos = );
		CONFIG_INT("TooltipDelay", TooltipDelay = );
		CONFIG_INT("Width", Width = );
		CONFIG_INT("IgnoreOriginalINI", IgnoreOriginalINI = );
#undef CONFIG_INT
#define CONFIG_STRING(str, var) \
		} else if (stricmp(name, str) == 0) { \
			strncpy(var, value, sizeof(var))
		CONFIG_STRING("GameCharactersPath", GameCharactersPath);
		CONFIG_STRING("GameDataPath", GameDataPath);
		CONFIG_STRING("GameName", GameName);
		CONFIG_STRING("GameOverridePath", GameOverridePath);
		CONFIG_STRING("GamePortraitsPath", GamePortraitsPath);
		CONFIG_STRING("GameScriptsPath", GameScriptsPath);
		CONFIG_STRING("GameSoundsPath", GameSoundsPath);
		CONFIG_STRING("GameType", GameType);
#undef CONFIG_STRING
#define CONFIG_STRING(str, var) \
		} else if (stricmp(name, str) == 0) { \
			var = value
		CONFIG_STRING("AudioDriver", AudioDriverName);
		CONFIG_STRING("VideoDriver", VideoDriverName);
#undef CONFIG_STRING
#define CONFIG_PATH(str, var) \
		} else if (stricmp(name, str) == 0) { \
			strncpy(var, value, sizeof(var));
		CONFIG_PATH("CachePath", CachePath);
		CONFIG_PATH("GUIScriptsPath", GUIScriptsPath);
		CONFIG_PATH("GamePath", GamePath);
		CONFIG_PATH("GemRBOverridePath", GemRBOverridePath);
		CONFIG_PATH("GemRBPath", GemRBPath);
		CONFIG_PATH("PluginsPath", PluginsPath);
		CONFIG_PATH("SavePath", SavePath);
#undef CONFIG_PATH
		} else if (stricmp( name, "ModPath" ) == 0) {
			for (char *path = strtok(value,SPathListSeparator);
					path;
					path = strtok(NULL,SPathListSeparator)) {
				ModPath.push_back(path);
			}
		} else if (stricmp( name, "SkipPlugin" ) == 0) {
			plugin_flags->SetAt( value, PLF_SKIP );
		} else if (stricmp( name, "DelayPlugin" ) == 0) {
			plugin_flags->SetAt( value, PLF_DELAY );
		} else {
			for(i=0;i<MAX_CD;i++) {
				char keyname[] = { 'C', 'D', '1'+i, '\0' };
				if (stricmp(name, keyname) == 0) {
					for(char *path = strtok(value, SPathListSeparator);
							path;
							path = strtok(NULL,SPathListSeparator)) {
						CD[i].push_back(path);
					}
				}
			}
		}
	}
	fclose( config );

	// WARNING: Don't move ResolveFilePath into the loop
	// Otherwise, it won't obey CaseSensitive set at the end
	// of the config file.

	if (stricmp( GameType, "tob" ) == 0) {
		strncpy( GameType, "bg2", sizeof(GameType) );
	}

#ifdef DATADIR
	if (!GemRBPath[0]) {
		strcpy( GemRBPath, DATADIR );
	}
#endif
	ResolveFilePath(GemRBPath);

	if (!GemRBOverridePath[0]) {
		strcpy( GemRBOverridePath, GemRBPath );
	} else {
		ResolveFilePath(GemRBOverridePath);
	}

	if (!PluginsPath[0]) {
#ifdef PLUGINDIR
		strcpy( PluginsPath, PLUGINDIR );
#else
		PathJoin( PluginsPath, GemRBPath, "plugins", NULL );
#endif
	} else {
		ResolveFilePath( PluginsPath );
	}

	if (!GUIScriptsPath[0]) {
		strcpy( GUIScriptsPath, GemRBPath );
	} else {
		ResolveFilePath( GUIScriptsPath );
	}

	if (!GameName[0]) {
		strcpy( GameName, GEMRB_STRING );
	}

	ResolveFilePath( GamePath );

	if (!SavePath[0]) {
		// FIXME: maybe should use UserDir instead of GamePath
		strcpy( SavePath, GamePath );
	} else {
		ResolveFilePath( SavePath );
	}

	if (! CachePath[0]) {
		PathJoin( CachePath, UserDir, "Cache", NULL );
	} else {
		ResolveFilePath(CachePath);
	}

	for (i = 0; i < MAX_CD; ++i) {
		if (!CD[i].size()) {
			char cd[] = { 'C', 'D', '1'+i, '\0' };
			char name[_MAX_PATH];

			PathJoin(name, GamePath, cd, NULL);
			CD[i].push_back(name);
		} else {
			size_t cnt = CD[i].size();
			while(cnt--) {
				ResolveFilePath( CD[i][cnt] );
			}
		}
	}

	for (i = 0; i < ModPath.size(); ++i) {
		ResolveFilePath(ModPath[i]);
	}

	FixPath( GUIScriptsPath, true );
	FixPath( PluginsPath, true );
	FixPath( GemRBPath, true );
	FixPath( GemRBOverridePath, true );

	if (GamePath[0]) {
		FixPath( GamePath, true );
	}

	//FixPath( SavePath, false );
	//mkdir( SavePath, S_IREAD|S_IWRITE|S_IEXEC );
	//chmod( SavePath, S_IREAD|S_IWRITE|S_IEXEC );
	FixPath( SavePath, true );

	FixPath( CachePath, false );
	mkdir( CachePath, S_IREAD|S_IWRITE|S_IEXEC );
	chmod( CachePath, S_IREAD|S_IWRITE|S_IEXEC );

	printStatus( "OK", LIGHT_GREEN );

	// Missing GameType is a common users' error
	if (!GameType[0]) {
		printMessage("Config","GameType was not set in your config file.\n", LIGHT_RED);
		return false;
	}

	if ( StupidityDetector( CachePath )) {
		printMessage("Core"," ",LIGHT_RED);
		printf( "Cache path %s doesn't exist, not a folder or contains alien files!\n", CachePath );
		return false;
	}
	if (!KeepCache) DelTree((const char *) CachePath, false);
	FixPath( CachePath, true );

	return true;
}

static void upperlower(int upper, int lower)
{
	pl_uppercase[lower]=(ieByte) upper;
	pl_lowercase[upper]=(ieByte) lower;
}

static const char *game_flags[GF_COUNT+1]={
		"HasKaputz",          //0 GF_HAS_KAPUTZ
		"AllStringsTagged",   //1 GF_ALL_STRINGS_TAGGED
		"HasSongList",        //2 GF_HAS_SONGLIST
		"TeamMovement",       //3 GF_TEAM_MOVEMENT
		"UpperButtonText",    //4 GF_UPPER_BUTTON_TEXT
		"LowerLabelText",     //5 GF_LOWER_LABEL_TEXT
		"HasPartyIni",        //6 GF_HAS_PARTY_INI
		"SoundFolders",       //7 GF_SOUNDFOLDERS
		"IgnoreButtonFrames", //8 GF_IGNORE_BUTTON_FRAMES
		"OneByteAnimationID", //9 GF_ONE_BYTE_ANIMID
		"HasDPLAYER",         //10GF_HAS_DPLAYER
		"HasEXPTABLE",        //11GF_HAS_EXPTABLE
		"HasBeastsIni",       //12GF_HAS_BEASTS_INI
		"HasDescIcon",        //13GF_HAS_DESC_ICON
		"HasPickSound",       //14GF_HAS_PICK_SOUND
		"IWDMapDimensions",   //15GF_IWD_MAP_DIMENSIONS
		"AutomapIni",         //16GF_AUTOMAP_INI
		"SmallFog",           //17GF_SMALL_FOG
		"ReverseDoor",        //18GF_REVERSE_DOOR
		"ProtagonistTalks",   //19GF_PROTAGONIST_TALKS
		"HasSpellList",       //20GF_HAS_SPELLLIST
		"IWD2ScriptName",     //21GF_IWD2_SCRIPTNAME
		"DialogueScrolls",    //22GF_DIALOGUE_SCROLLS
		"KnowWorld",          //23GF_KNOW_WORLD
		"ReverseToHit",       //24GF_REVERSE_TOHIT
		"SaveForHalfDamage",  //25GF_SAVE_FOR_HALF
		"CharNameIsGabber",   //26GF_CHARNAMEISGABBER
		"MagicBit",           //27GF_MAGICBIT
		"CheckAbilities",     //28GF_CHECK_ABILITIES
		"ChallengeRating",    //29GF_CHALLENGERATING
		"SpellBookIconHack",  //30GF_SPELLBOOKICONHACK
		"EnhancedEffects",    //31GF_ENHANCED_EFFECTS
		"DeathOnZeroStat",    //32GF_DEATH_ON_ZERO_STAT
		"SpawnIni",           //33GF_SPAWN_INI
		"IWD2DeathVarFormat", //34GF_IWD2_DEATHVARFORMAT
		"HasResDataIni",      //35GF_RESDATA_INI
		"OverrideCursorPos",  //36GF_OVERRIDE_CURSORPOS
		"BreakableWeapons",   //37GF_BREAKABLE_WEAPONS
		"3EdRules",           //38GF_3ED_RULES
		"LevelslotPerClass",  //39GF_LEVELSLOT_PER_CLASS
		"SelectiveMagicRes",  //40GF_SELECTIVE_MAGIC_RES
		"HasHideInShadows",   //41GF_HAS_HIDE_IN_SHADOWS
		"AreaVisitedVar",     //42GF_AREA_VISITED_VAR
		"ProperBackstab",     //43GF_PROPER_BACKSTAB
		"OnScreenText",       //44GF_ONSCREEN_TEXT
		"HasSpecificDamageBonus", //45GF_SPECIFIC_DMG_BONUS
		"StrrefSaveGame",     //46GF_STRREF_SAVEGAME
		"HasWisdomBonusTable",//47GF_WISDOM_BONUS
		"BiographyIsRes",     //48GF_BIOGRAPHY_RES
		"NoBiography",        //49GF_NO_BIOGRAPHY
		"StealIsAttack",      //50GF_STEAL_IS_ATTACK
		"CutsceneAreascripts",//51GF_CUTSCENE_AREASCRIPTS
		"FlexibleWorldmap",   //52GF_FLEXIBLE_WMAP
		"AutoSearchHidden",   //53GF_AUTOSEARCH_HIDDEN
		"PSTStateFlags",      //54GF_PST_STATE_FLAGS
		"NoDropCanMove",      //55GF_NO_DROP_CAN_MOVE
		"JournalHasSections", //56GF_JOURNAL_HAS_SECTIONS
		"CastingSounds",      //57GF_CASTING_SOUNDS
		"EnhancedCastingSounds", //58GF_CASTING_SOUNDS2
		"ForceAreaScript",    //59GF_FORCE_AREA_SCRIPT
		"AreaOverride",       //60GF_AREA_OVERRIDE
		"NoNewVariables",     //61GF_NO_NEW_VARIABLES
		NULL                  //for our own safety, this marks the end of the pole
};

/** Loads gemrb.ini */
bool Interface::LoadGemRBINI()
{
	DataStream* inifile = gamedata->GetResource( "gemrb", IE_INI_CLASS_ID );
	if (! inifile) {
		printStatus( "ERROR", LIGHT_RED );
		return false;
	}

	printMessage( "Core", "Loading game type-specific GemRB setup...\n", WHITE );
	printf( "%s",inifile->originalfile);

	if (!IsAvailable( IE_INI_CLASS_ID )) {
		printStatus( "ERROR", LIGHT_RED );
		printf( "[Core]: No INI Importer Available.\n" );
		return false;
	}
	PluginHolder<DataFileMgr> ini(IE_INI_CLASS_ID);
	ini->Open( inifile, true ); //autofree

	printStatus( "OK", LIGHT_GREEN );

	const char *s;

	// Resrefs are already initialized in Interface::Interface()
	s = ini->GetKeyAsString( "resources", "CursorBAM", NULL );
	if (s)
		strnlwrcpy( CursorBam, s, 8 ); //console cursor

	s = ini->GetKeyAsString( "resources", "ScrollCursorBAM", NULL );
	if (s)
		strnlwrcpy( ScrollCursorBam, s, 8 );

	s = ini->GetKeyAsString( "resources", "ButtonFont", NULL );
	if (s)
		strnlwrcpy( ButtonFont, s, 8 );

	s = ini->GetKeyAsString( "resources", "TooltipFont", NULL );
	if (s)
		strnlwrcpy( TooltipFont, s, 8 );

	s = ini->GetKeyAsString( "resources", "MovieFont", NULL );
	if (s)
		strnlwrcpy( MovieFont, s, 8 );

	s = ini->GetKeyAsString( "resources", "TooltipBack", NULL );
	if (s)
		strnlwrcpy( TooltipBackResRef, s, 8 );

	s = ini->GetKeyAsString( "resources", "TooltipColor", NULL );
	if (s) {
		if (s[0] == '#') {
			unsigned long c = strtoul (s + 1, NULL, 16);
			// FIXME: check errno
			TooltipColor.r = (unsigned char) (c >> 24);
			TooltipColor.g = (unsigned char) (c >> 16);
			TooltipColor.b = (unsigned char) (c >> 8);
			TooltipColor.a = (unsigned char) (c);
		}
	}

	//which stat determines the fist weapon (defaults to class)
	Actor::SetFistStat(ini->GetKeyAsInt( "resources", "FistStat", IE_CLASS));

	TooltipMargin = ini->GetKeyAsInt( "resources", "TooltipMargin", TooltipMargin );

	// The format of GroundCircle can be:
	// GroundCircleBAM1 = wmpickl/3
	// to denote that the bitmap should be scaled down 3x
	for (int size = 0; size < MAX_CIRCLE_SIZE; size++) {
		char name[30];
		sprintf( name, "GroundCircleBAM%d", size+1 );
		s = ini->GetKeyAsString( "resources", name, NULL );
		if (s) {
			const char *pos = strchr( s, '/' );
			if (pos) {
				GroundCircleScale[size] = atoi( pos+1 );
				strncpy( GroundCircleBam[size], s, pos - s );
				GroundCircleBam[size][pos - s] = '\0';
			} else {
				strcpy( GroundCircleBam[size], s );
			}
		}
	}

	s = ini->GetKeyAsString( "resources", "NoteString", NULL );
	TextArea::SetNoteString(s);

	s = ini->GetKeyAsString( "resources", "INIConfig", NULL );
	if (s)
		strcpy( INIConfig, s );

	s = ini->GetKeyAsString( "resources", "Palette16", NULL );
	if (s)
		strcpy( Palette16, s );

	s = ini->GetKeyAsString( "resources", "Palette32", NULL );
	if (s)
		strcpy( Palette32, s );

	s = ini->GetKeyAsString( "resources", "Palette256", NULL );
	if (s)
		strcpy( Palette256, s );

	unsigned int i = (unsigned int) ini->GetKeyAsInt ("charset", "CharCount", 0);
	if (i>99) i=99;
	while(i--) {
		char key[10];
		snprintf(key,9,"Letter%d", i+1);
		s = ini->GetKeyAsString( "charset", key, NULL );
		if (s) {
			const char *s2 = strchr(s,',');
			if (s2) {
				upperlower(atoi(s), atoi(s2+1) );
			}
		}
	}

	MaximumAbility = ini->GetKeyAsInt ("resources", "MaximumAbility", 25 );

	RedrawTile = ini->GetKeyAsInt( "resources", "RedrawTile", 0 )!=0;

	for (i=0;i<GF_COUNT;i++) {
		if (!game_flags[i]) {
			printf("Fix the game flags!\n");
			abort();
		}
		SetFeature( ini->GetKeyAsInt( "resources", game_flags[i], 0 ), i );
		//printMessage("Option", "", GREEN);
		//printf("%s = %s\n", game_flags[i], HasFeature(i)?"yes":"no");
	}

	ForceStereo = ini->GetKeyAsInt( "resources", "ForceStereo", 0 );

	return true;
}

Palette* Interface::CreatePalette(const Color &color, const Color &back)
{
	Palette* pal = new Palette();
	pal->col[0].r = 0;
	pal->col[0].g = 0xff;
	pal->col[0].b = 0;
	pal->col[0].a = 0;
	for (int i = 1; i < 256; i++) {
		pal->col[i].r = back.r +
			( unsigned char ) ( ( ( color.r - back.r ) * ( i ) ) / 255 );
		pal->col[i].g = back.g +
			( unsigned char ) ( ( ( color.g - back.g ) * ( i ) ) / 255 );
		pal->col[i].b = back.b +
			( unsigned char ) ( ( ( color.b - back.b ) * ( i ) ) / 255 );
		pal->col[i].a = back.a +
			( unsigned char ) ( ( ( color.a - back.a ) * ( i ) ) / 255 );
	}
	return pal;
}

/** No descriptions */
Color* Interface::GetPalette(unsigned index, int colors, Color *pal) const
{
	Image *img;
	if (colors == 32) {
		img = pal32;
	} else if (colors <= 32) {
		img = pal16;
	} else if (colors == 256) {
		img = pal256;
	} else {
		return pal;
	}
	if (index >= img->GetHeight()) {
		index = 0;
	}
	for (int i = 0; i < colors; i++) {
		pal[i] = img->GetPixel(i, index);
	}
	return pal;
}
/** Returns a preloaded Font */
Font* Interface::GetFont(const char *ResRef) const
{
	for (unsigned int i = 0; i < fonts.size(); i++) {
		if (strnicmp( fonts[i]->ResRef, ResRef, 8 ) == 0) {
			return fonts[i];
		}
	}
	return NULL;
}

Font* Interface::GetFont(unsigned int index) const
{
	if (index >= fonts.size()) {
		return NULL;
	}
	return fonts[index];
}

Font* Interface::GetButtonFont() const
{
	return GetFont( ButtonFont );
}

/** Returns the Event Manager */
EventMgr* Interface::GetEventMgr() const
{
	return evntmgr;
}

/** Returns the Window Manager */
WindowMgr* Interface::GetWindowMgr() const
{
	return windowmgr.get();
}

/** Get GUI Script Manager */
ScriptEngine* Interface::GetGUIScriptEngine() const
{
	return guiscript.get();
}

static EffectRef fx_summon_disable_ref = { "AvatarRemovalModifier", -1 };

//NOTE: if there were more summoned creatures, it will return only the last
Actor *Interface::SummonCreature(const ieResRef resource, const ieResRef vvcres, Scriptable *owner, Actor *target, const Point &position, int eamod, int level, Effect *fx, bool sexmod)
{
	//maximum number of monsters summoned
	int cnt=10;
	Actor * ab = NULL;

	//TODO:
	//decrease the number of summoned creatures with the number of already summoned creatures here
	//the summoned creatures have a special IE_SPECIFIC

	while(cnt--) {
		ab = gamedata->GetCreature(resource);
		if (!ab) {
			return NULL;
		}

		if (owner && owner->Type==ST_ACTOR) {
			ab->LastSummoner = owner->GetGlobalID();
		}
		//Always use Base stats for the recently summoned creature

		int enemyally;

		if (eamod==EAM_SOURCEALLY || eamod==EAM_SOURCEENEMY) {
			if (owner && owner->Type==ST_ACTOR) {
				enemyally = ((Actor *) owner)->GetStat(IE_EA)>EA_GOODCUTOFF;
			} else {
				enemyally = true;
			}
		} else {
			if (target) {
				enemyally = target->GetBase(IE_EA)>EA_GOODCUTOFF;
			} else {
				enemyally = true;
			}
		}

		switch (eamod) {
		case EAM_SOURCEALLY:
		case EAM_ALLY:
			if (enemyally) {
				ab->SetBase(IE_EA, EA_ENEMY); //is this the summoned EA?
			} else {
				ab->SetBase(IE_EA, EA_CONTROLLED); //is this the summoned EA?
			}
			break;
		case EAM_SOURCEENEMY:
		case EAM_ENEMY:
			if (enemyally) {
				ab->SetBase(IE_EA, EA_CONTROLLED); //is this the summoned EA?
			} else {
				ab->SetBase(IE_EA, EA_ENEMY); //is this the summoned EA?
			}
			break;
		case EAM_NEUTRAL:
			ab->SetBase(IE_EA, EA_NEUTRAL);
			break;
		default:
			break;
		}

		// mark the summon, but only if they don't have a special sex already
		if (sexmod && ab->BaseStats[IE_SEX] < SEX_EXTRA) {
			ab->SetBase(IE_SEX, SEX_SUMMON);
		}

		Map *map;
		if (target) {
			map = target->GetCurrentArea();
		} else {
			map = owner->GetCurrentArea();
		}
		map->AddActor(ab);
		ab->SetPosition(position, true, 0);
		ab->RefreshEffects(NULL);

		if (vvcres[0]) {
			ScriptedAnimation* vvc = gamedata->GetScriptedAnimation(vvcres, false);
			if (vvc) {
				//This is the final position of the summoned creature
				//not the original target point
				vvc->XPos=ab->Pos.x;
				vvc->YPos=ab->Pos.y;
				//force vvc to play only once
				vvc->PlayOnce();
				map->AddVVCell( vvc );

				//set up the summon disable effect
				Effect *newfx = EffectQueue::CreateEffect(fx_summon_disable_ref, 0, 1, FX_DURATION_ABSOLUTE);
				if (newfx) {
					newfx->Duration = vvc->GetSequenceDuration(AI_UPDATE_TIME)*9/10 + core->GetGame()->GameTime;
					ApplyEffect(newfx, ab, ab);
				}
			}
		}

		//remove the xp value of friendly summons
		if (ab->BaseStats[IE_EA]<EA_GOODCUTOFF) {
			ab->SetBase(IE_XPVALUE, 0);
		}
		if (fx) {
			ApplyEffect(fx, ab, owner);
		}

		//this check should happen after the fact
		level -= ab->GetBase(IE_XP);
		if(level<0 || ab->GetBase(IE_XP) == 0) {
			break;
		}

	}
	return ab;
}

void Interface::RedrawControls(const char *varname, unsigned int value)
{
	for (unsigned int i = 0; i < windows.size(); i++) {
		Window *win = windows[i];
		if (win != NULL && win->Visible!=WINDOW_INVALID) {
			win->RedrawControls(varname, value);
		}
	}
}

void Interface::RedrawAll()
{
	for (unsigned int i = 0; i < windows.size(); i++) {
		Window *win = windows[i];
		if (win != NULL && win->Visible!=WINDOW_INVALID) {
			win->Invalidate();
		}
	}
}

/** Loads a WindowPack (CHUI file) in the Window Manager */
bool Interface::LoadWindowPack(const char* name)
{
	DataStream* stream = gamedata->GetResource( name, IE_CHU_CLASS_ID );
	if (stream == NULL) {
		printMessage( "Interface", "Error: Cannot find ", LIGHT_RED );
		printf( "%s.chu\n", name );
		return false;
	}
	if (!GetWindowMgr()->Open( stream, true )) {
		printMessage( "Interface", "Error: Cannot Load ", LIGHT_RED );
		printf( "%s.chu\n", name );
		return false;
	}

	strncpy( WindowPack, name, sizeof( WindowPack ) );
	WindowPack[sizeof( WindowPack ) - 1] = '\0';

	return true;
}

/** Loads a Window in the Window Manager */
int Interface::LoadWindow(unsigned short WindowID)
{
	unsigned int i;

	for (i = 0; i < windows.size(); i++) {
		Window *win = windows[i];
		if (win == NULL)
			continue;
		if (win->Visible==WINDOW_INVALID) {
			continue;
		}
		if (win->WindowID == WindowID &&
			!strnicmp( WindowPack, win->WindowPack, sizeof(WindowPack) )) {
			SetOnTop( i );
			win->Invalidate();
			return i;
		}
	}
	Window* win = windowmgr->GetWindow( WindowID );
	if (win == NULL) {
		return -1;
	}
	memcpy( win->WindowPack, WindowPack, sizeof(WindowPack) );

	int slot = -1;
	for (i = 0; i < windows.size(); i++) {
		if (windows[i] == NULL) {
			slot = i;
			break;
		}
	}
	if (slot == -1) {
		windows.push_back( win );
		slot = ( int ) windows.size() - 1;
	} else {
		windows[slot] = win;
	}
	win->Invalidate();
	return slot;
}
// FIXME: it's a clone of LoadWindow
/** Creates a Window in the Window Manager */
int Interface::CreateWindow(unsigned short WindowID, int XPos, int YPos, unsigned int Width, unsigned int Height, char* Background)
{
	unsigned int i;

	for (i = 0; i < windows.size(); i++) {
		if (windows[i] == NULL)
			continue;
		if (windows[i]->WindowID == WindowID && !stricmp( WindowPack,
			windows[i]->WindowPack )) {
			SetOnTop( i );
			windows[i]->Invalidate();
			return i;
		}
	}

	Window* win = new Window( WindowID, (ieWord) XPos, (ieWord) YPos, (ieWord) Width, (ieWord) Height );
	if (Background[0]) {
		ResourceHolder<ImageMgr> mos(Background);
		if (mos != NULL) {
			win->SetBackGround( mos->GetSprite2D(), true );
		} else {
			printf( "[Core]: Cannot Load BackGround, skipping\n" );
		}
	}

	strcpy( win->WindowPack, WindowPack );

	int slot = -1;
	for (i = 0; i < windows.size(); i++) {
		if (windows[i] == NULL) {
			slot = i;
			break;
		}
	}
	if (slot == -1) {
		windows.push_back( win );
		slot = ( int ) windows.size() - 1;
	} else {
		windows[slot] = win;
	}
	win->Invalidate();
	return slot;
}

/** Sets a Window on the Top */
void Interface::SetOnTop(int Index)
{
	std::vector<int>::iterator t;
	for(t = topwin.begin(); t != topwin.end(); ++t) {
		if((*t) == Index) {
			topwin.erase(t);
			break;
		}
	}
	if(topwin.size() != 0)
		topwin.insert(topwin.begin(), Index);
	else
		topwin.push_back(Index);
}
/** Add a window to the Window List */
void Interface::AddWindow(Window * win)
{
	int slot = -1;
	for(unsigned int i = 0; i < windows.size(); i++) {
		Window *w = windows[i];

		if(w==NULL) {
			slot = i;
			break;
		}
	}
	if(slot == -1) {
		windows.push_back(win);
		slot=(int)windows.size()-1;
	}
	else
		windows[slot] = win;
	win->Invalidate();
}

/** Get a Control on a Window */
int Interface::GetControl(unsigned short WindowIndex, unsigned long ControlID) const
{
	if (WindowIndex >= windows.size()) {
		return -1;
	}
	Window* win = windows[WindowIndex];
	if (win == NULL) {
		return -1;
	}
	int i = 0;
	while (true) {
		Control* ctrl = win->GetControl( (unsigned short) i );
		if (ctrl == NULL)
			return -1;
		if (ctrl->ControlID == ControlID)
			return i;
		i++;
	}
}
/** Adjust the Scrolling factor of a control (worldmap atm) */
int Interface::AdjustScrolling(unsigned short WindowIndex,
		unsigned short ControlIndex, short x, short y)
{
	if (WindowIndex >= windows.size()) {
		return -1;
	}
	Window* win = windows[WindowIndex];
	if (win == NULL) {
		return -1;
	}
	Control* ctrl = win->GetControl( ControlIndex );
	if (ctrl == NULL) {
		return -1;
	}
	switch(ctrl->ControlType) {
		case IE_GUI_WORLDMAP:
			((WorldMapControl *) ctrl)->AdjustScrolling(x,y);
			break;
		default: //doesn't work for these
			return -1;
	}
	return 0;
}

/** Set the Text of a Control */
int Interface::SetText(unsigned short WindowIndex,
		unsigned short ControlIndex, const char* string)
{
	if (WindowIndex >= windows.size()) {
		return -1;
	}
	Window* win = windows[WindowIndex];
	if (win == NULL) {
		return -1;
	}
	Control* ctrl = win->GetControl( ControlIndex );
	if (ctrl == NULL) {
		return -1;
	}
	return ctrl->SetText( string );
}
/** Set the Tooltip text of a Control */
int Interface::SetTooltip(unsigned short WindowIndex,
		unsigned short ControlIndex, const char* string)
{
	if (WindowIndex >= windows.size()) {
		return -1;
	}
	Window* win = windows[WindowIndex];
	if (win == NULL) {
		return -1;
	}
	Control* ctrl = win->GetControl( ControlIndex );
	if (ctrl == NULL) {
		return -1;
	}
	return ctrl->SetTooltip( string );
}

void Interface::DisplayTooltip(int x, int y, Control *ctrl)
{
	if (tooltip_ctrl && tooltip_ctrl == ctrl && tooltip_x == x && tooltip_y == y)
		return;
	tooltip_x = x;
	tooltip_y = y;
	tooltip_currtextw = 0;
	tooltip_ctrl = ctrl;
}

int Interface::GetVisible(unsigned short WindowIndex) const
{
	if (WindowIndex >= windows.size()) {
		return -1;
	}
	Window* win = windows[WindowIndex];
	if (win == NULL) {
		return -1;
	}
	return win->Visible;
}
/** Set a Window Visible Flag */
int Interface::SetVisible(unsigned short WindowIndex, int visible)
{
	if (WindowIndex >= windows.size()) {
		return -1;
	}
	Window* win = windows[WindowIndex];
	if (win == NULL) {
		return -1;
	}
	if (visible!=WINDOW_FRONT) {
		win->Visible = (char) visible;
	}
	switch (visible) {
		case WINDOW_GRAYED:
			win->Invalidate();
			//here is a fallthrough
		case WINDOW_INVISIBLE:
			//hiding the viewport if the gamecontrol window was made invisible
			if (win->WindowID==65535) {
				video->SetViewport( 0,0,0,0 );
			}
			evntmgr->DelWindow( win );
			break;

		case WINDOW_VISIBLE:
			if (win->WindowID==65535) {
				video->SetViewport( win->XPos, win->YPos, win->Width, win->Height);
			}
			//here is a fallthrough
		case WINDOW_FRONT:
			if (win->Visible==WINDOW_VISIBLE) {
				evntmgr->AddWindow( win );
			}
			win->Invalidate();
			SetOnTop( WindowIndex );
			break;
	}
	return 0;
}


/** Set the Status of a Control in a Window */
int Interface::SetControlStatus(unsigned short WindowIndex,
		unsigned short ControlIndex, unsigned long Status)
{
	//don't set the status of an already invalidated window
	Window* win = GetWindow(WindowIndex);
	if (win == NULL) {
		return -1;
	}
	Control* ctrl = win->GetControl( ControlIndex );
	if (ctrl == NULL) {
		return -1;
	}
	if (Status&IE_GUI_CONTROL_FOCUSED) {
		evntmgr->SetFocused( win, ctrl);
	}
	if (ctrl->ControlType != ((Status >> 24) & 0xff) ) {
		return -2;
	}
	switch (ctrl->ControlType) {
		case IE_GUI_BUTTON:
			//Button
			{
				Button* btn = ( Button* ) ctrl;
				btn->SetState( ( unsigned char ) ( Status & 0x7f ) );
			}
			break;
		default:
			ctrl->Value = Status & 0x7f;
			break;
	}
	return 0;
}

/** Show a Window in Modal Mode */
int Interface::ShowModal(unsigned short WindowIndex, int Shadow)
{
	if (WindowIndex >= windows.size()) {
		printMessage( "Core", "Window not found", LIGHT_RED );
		return -1;
	}
	Window* win = windows[WindowIndex];
	if (win == NULL) {
		printMessage( "Core", "Window already freed", LIGHT_RED );
		return -1;
	}
	win->Visible = WINDOW_FRONT;
	//don't destroy the other window handlers
	//evntmgr->Clear();
	SetOnTop( WindowIndex );
	evntmgr->AddWindow( win );
	evntmgr->SetFocused( win, NULL );

	ModalWindow = NULL;
	DrawWindows();
	win->Invalidate();

	Color gray = {
		0, 0, 0, 128
	};
	Color black = {
		0, 0, 0, 255
	};

	Region r( 0, 0, Width, Height );

	if (Shadow == MODAL_SHADOW_GRAY) {
		video->DrawRect( r, gray );
	} else if (Shadow == MODAL_SHADOW_BLACK) {
		video->DrawRect( r, black );
	}

	ModalWindow = win;
	return 0;
}

bool Interface::IsFreezed()
{
	return !update_scripts;
}

void Interface::GameLoop(void)
{
	update_scripts = false;
	GameControl *gc = GetGameControl();
	if (gc) {
		update_scripts = !(gc->GetDialogueFlags() & DF_FREEZE_SCRIPTS);
	}

	GSUpdate(update_scripts);

	//i'm not sure if this should be here

	//in multi player (if we ever get to it), only the server must call this
	if (update_scripts) {
		if ( game->selected.size() > 0 ) {
			gc->ChangeMap(GetFirstSelectedPC(true), false);
		}
		// the game object will run the area scripts as well
		game->UpdateScripts();
	}
}

/** handles hardcoded gui behaviour */
void Interface::HandleGUIBehaviour(void)
{
	GameControl *gc = GetGameControl();
	if (gc) {
		//this variable is used all over in the following hacks
		int flg = gc->GetDialogueFlags();

		//the following part is a series of hardcoded gui behaviour

		//initiating dialog
		if (flg & DF_IN_DIALOG) {
			// -3 noaction
			// -2 close
			// -1 open
			// choose option
			ieDword var = (ieDword) -3;
			vars->Lookup("DialogChoose", var);
			if ((int) var == -2) {
				gc->dialoghandler->EndDialog();
			} else if ( (int)var !=-3) {
				gc->dialoghandler->DialogChoose(var);
				if (!(gc->GetDialogueFlags() & (DF_OPENCONTINUEWINDOW | DF_OPENENDWINDOW)))
					guiscript->RunFunction( "GUIWORLD", "NextDialogState" );

				// the last node of a dialog can have a new-dialog action! don't interfere in that case
				ieDword newvar = 0; vars->Lookup("DialogChoose", newvar);
				if (var == (ieDword) -1 || newvar != (ieDword) -1) {
					vars->SetAt("DialogChoose", (ieDword) -3);
				}
			}
			if (flg & DF_OPENCONTINUEWINDOW) {
				guiscript->RunFunction( "GUIWORLD", "OpenContinueMessageWindow" );
				gc->SetDialogueFlags(DF_OPENCONTINUEWINDOW|DF_OPENENDWINDOW, BM_NAND);
			} else if (flg & DF_OPENENDWINDOW) {
				guiscript->RunFunction( "GUIWORLD", "OpenEndMessageWindow" );
				gc->SetDialogueFlags(DF_OPENCONTINUEWINDOW|DF_OPENENDWINDOW, BM_NAND);
			} else {
				guiscript->RunFunction( "GUICommonWindows", "EmptyControls" );
			}
		}

		//handling container
		if (CurrentContainer && UseContainer) {
			if (!(flg & DF_IN_CONTAINER) ) {
				gc->SetDialogueFlags(DF_IN_CONTAINER, BM_OR);
				guiscript->RunFunction( "CommonWindow", "OpenContainerWindow" );
			}
		} else {
			if (flg & DF_IN_CONTAINER) {
				gc->SetDialogueFlags(DF_IN_CONTAINER, BM_NAND);
				guiscript->RunFunction( "CommonWindow", "CloseContainerWindow" );
			}
		}
		//end of gui hacks
	}
}

void Interface::DrawWindows(void)
{
	//here comes the REAL drawing of windows
	if (ModalWindow) {
		ModalWindow->DrawWindow();
		return;
	}
	size_t i = topwin.size();
	while(i--) {
		unsigned int t = topwin[i];

		if ( t >=windows.size() )
			continue;

		//visible ==1 or 2 will be drawn
		Window* win = windows[t];
		if (win != NULL) {
			if (win->Visible == WINDOW_INVALID) {
				topwin.erase(topwin.begin()+i);
				evntmgr->DelWindow( win );
				delete win;
				windows[t]=NULL;
			} else if (win->Visible) {
				win->DrawWindow();
			}
		}
	}
}

void Interface::DrawTooltip ()
{
	if (! tooltip_ctrl || !tooltip_ctrl->Tooltip)
		return;

	Font* fnt = GetFont( TooltipFont );
	char *tooltip_text = tooltip_ctrl->Tooltip;

	int w1 = 0;
	int w2 = 0;
	int strw = fnt->CalcStringWidth( tooltip_text ) + 8;
	int w = strw;
	int h = fnt->maxHeight;

	if (TooltipBack) {
		// animate BG tooltips
		// TODO: make tooltip animation an option instead
		// of following hard-coded check!
		if (TooltipMargin == 5) {
			// TODO: make speed an option
			int tooltip_anim_speed = 15;
			if (tooltip_currtextw < strw) {
				tooltip_currtextw += tooltip_anim_speed;
			}
			if (tooltip_currtextw > strw) {
				tooltip_currtextw = strw;
			}
			w = tooltip_currtextw;
		}

		h = TooltipBack[0]->Height;
		w1 = TooltipBack[1]->Width;
		w2 = TooltipBack[2]->Width;
		w += TooltipMargin*2;
		strw += TooltipMargin*2;
		//multiline in case of too much text
		if (w>TooltipBack[0]->Width)
			strw=w=TooltipBack[0]->Width;
		else if (strw>TooltipBack[0]->Width)
			strw=TooltipBack[0]->Width;
	}

	int strx = tooltip_x - strw / 2;
	int y = tooltip_y - h / 2;
	// Ensure placement within the screen
	if (strx < 0) strx = 0;
	else if (strx + strw + w1 + w2 > Width)
		strx = Width - strw - w1 - w2;
	if (y < 0) y = 0;
	else if (y + h > Height)
		y = Height - h;

	int x = strx + ((strw - w) / 2);

	// FIXME: take back[0] from center, not from left end
	Region r2 = Region( x, y, w, h );
	if (TooltipBack) {
		video->BlitSprite( TooltipBack[0], x + TooltipMargin, y, true, &r2 );
		video->BlitSprite( TooltipBack[1], x, y, true );
		video->BlitSprite( TooltipBack[2], x + w, y, true );
	}

	if (TooltipBack) {
		r2.x+=TooltipMargin;
		strx+=TooltipMargin;
	}
	Region textr = Region( strx, y, strw, h );
	fnt->Print( r2, textr, (ieByte *) tooltip_text, NULL,
		IE_FONT_ALIGN_CENTER | IE_FONT_ALIGN_MIDDLE, true );
}

//interface for higher level functions, if the window was
//marked for deletion it is not returned
Window* Interface::GetWindow(unsigned short WindowIndex) const
{
	if (WindowIndex < windows.size()) {
		Window *win = windows[WindowIndex];
		if (win && (win->Visible!=WINDOW_INVALID) ) {
			return win;
		}
	}
	return NULL;
}

// this function will determine if wnd is a valid window pointer
// by checking if its WindowID is the same as the reference
bool Interface::IsValidWindow(unsigned short WindowID, Window *wnd) const
{
	size_t WindowIndex = windows.size();
	while (WindowIndex--) {
		if (windows[WindowIndex] == wnd) {
			return wnd->WindowID == WindowID;
		}
	}
	return false;
}

//this function won't delete the window, just mark it for deletion
//it will be deleted in the next DrawWindows cycle
//regardless, the window deleted is inaccessible for gui scripts and
//other high level functions from now
int Interface::DelWindow(unsigned short WindowIndex)
{
	if (WindowIndex >= windows.size()) {
		return -1;
	}
	Window* win = windows[WindowIndex];
	if ((win == NULL) || (win->Visible==WINDOW_INVALID) ) {
		printMessage( "Core", "Window deleted again", LIGHT_RED );
		return -1;
	}
	if (win == ModalWindow) {
		ModalWindow = NULL;
		RedrawAll(); //marking windows for redraw
	}
	evntmgr->DelWindow( win );
	win->release();
	//re-capturing new (old) modal window if any
	size_t tw = topwin.size();
	for(size_t i=0;i<tw;i++) {
		Window *tmp = windows[topwin[i]];
		if (tmp->Visible==WINDOW_FRONT) {
			ModalWindow = tmp;
			break;
		}
	}
	return 0;
}

void Interface::DelAllWindows()
{
	vars->SetAt("MessageWindow", (ieDword) ~0);
	vars->SetAt("OptionsWindow", (ieDword) ~0);
	vars->SetAt("PortraitWindow", (ieDword) ~0);
	vars->SetAt("ActionsWindow", (ieDword) ~0);
	vars->SetAt("TopWindow", (ieDword) ~0);
	vars->SetAt("OtherWindow", (ieDword) ~0);
	vars->SetAt("FloatWindow", (ieDword) ~0);
	for(unsigned int WindowIndex=0; WindowIndex<windows.size();WindowIndex++) {
		Window* win = windows[WindowIndex];
		delete win;
	}
	windows.clear();
	topwin.clear();
	evntmgr->Clear();
	ModalWindow = NULL;
}

/** Popup the Console */
void Interface::PopupConsole()
{
	ConsolePopped = !ConsolePopped;
	RedrawAll();
	console->Changed = true;
}

/** Draws the Console */
void Interface::DrawConsole()
{
	console->Draw( 0, 0 );
}

/** Get the Sound Manager */
SaveGameIterator* Interface::GetSaveGameIterator() const
{
	return sgiterator;
}
/** Sends a termination signal to the Video Driver */
bool Interface::Quit(void)
{
	return video->Quit();
}
/** Returns the variables dictionary */
Variables* Interface::GetDictionary() const
{
	return vars;
}
/** Returns the token dictionary */
Variables* Interface::GetTokenDictionary() const
{
	return tokens;
}
/** Get the Music Manager */
MusicMgr* Interface::GetMusicMgr() const
{
	return music.get();
}
/** Loads an IDS Table, returns -1 on error or the Symbol Table Index on success */
int Interface::LoadSymbol(const char* ResRef)
{
	int ind = GetSymbolIndex( ResRef );
	if (ind != -1) {
		return ind;
	}
	DataStream* str = gamedata->GetResource( ResRef, IE_IDS_CLASS_ID );
	if (!str) {
		return -1;
	}
	PluginHolder<SymbolMgr> sm(IE_IDS_CLASS_ID);
	if (!sm) {
		delete str;
		return -1;
	}
	if (!sm->Open( str, true )) {
		return -1;
	}
	Symbol s;
	strncpy( s.ResRef, ResRef, 8 );
	s.sm = sm;
	ind = -1;
	for (size_t i = 0; i < symbols.size(); i++) {
		if (!symbols[i].sm) {
			ind = ( int ) i;
			break;
		}
	}
	if (ind != -1) {
		symbols[ind] = s;
		return ind;
	}
	symbols.push_back( s );
	return ( int ) symbols.size() - 1;
}
/** Gets the index of a loaded Symbol Table, returns -1 on error */
int Interface::GetSymbolIndex(const char* ResRef) const
{
	for (size_t i = 0; i < symbols.size(); i++) {
		if (!symbols[i].sm)
			continue;
		if (strnicmp( symbols[i].ResRef, ResRef, 8 ) == 0)
			return ( int ) i;
	}
	return -1;
}
/** Gets a Loaded Symbol Table by its index, returns NULL on error */
Holder<SymbolMgr> Interface::GetSymbol(unsigned int index) const
{
	if (index >= symbols.size()) {
		return Holder<SymbolMgr>();
	}
	if (!symbols[index].sm) {
		return Holder<SymbolMgr>();
	}
	return symbols[index].sm;
}
/** Frees a Loaded Symbol Table, returns false on error, true on success */
bool Interface::DelSymbol(unsigned int index)
{
	if (index >= symbols.size()) {
		return false;
	}
	if (!symbols[index].sm) {
		return false;
	}
	symbols[index].sm.release();
	return true;
}
/** Plays a Movie */
int Interface::PlayMovie(const char* ResRef)
{
	ResourceHolder<MoviePlayer> mp(ResRef);
	if (!mp) {
		return -1;
	}

	ieDword subtitles = 0;
	Font *SubtitleFont = NULL;
	Palette *palette = NULL;
	ieDword *frames = NULL;
	ieDword *strrefs = NULL;
	int cnt = 0;
	int offset = 0;

	//one of these two should exist (they both mean the same thing)
	vars->Lookup("Display Movie Subtitles", subtitles);
	if (subtitles) {
		//HoW flag
		cnt=-3;
		offset = 3;
	} else {
		//ToB flag
		vars->Lookup("Display Subtitles", subtitles);
	}
	AutoTable sttable;
	if (subtitles && sttable.load(ResRef)) {
		cnt += sttable->GetRowCount();
		if (cnt>0) {
			frames = (ieDword *) malloc(cnt * sizeof(ieDword) );
			strrefs = (ieDword *) malloc(cnt * sizeof(ieDword) );
		} else {
			cnt = 0;
		}
		if (frames && strrefs) {
			for (int i=0;i<cnt;i++) {
				frames[i] = atoi (sttable->QueryField(i+offset, 0) );
				strrefs[i] = atoi (sttable->QueryField(i+offset, 1) );
			}
		}
		int r = atoi(sttable->QueryField("red", "frame"));
		int g = atoi(sttable->QueryField("green", "frame"));
		int b = atoi(sttable->QueryField("blue", "frame"));
		SubtitleFont = GetFont (MovieFont); //will change
		if (r || g || b) {
			if (SubtitleFont) {
				Color fore = {(unsigned char) r,(unsigned char) g,(unsigned char) b, 0x00};
				Color back = {0x00, 0x00, 0x00, 0x00};
				palette = CreatePalette( fore, back );
			}
		}
	}

	//shutting down music and ambients before movie
	if (music)
		music->HardEnd();
	AmbientMgr *ambim = AudioDriver->GetAmbientMgr();
	if (ambim) ambim->deactivate();
	video->SetMovieFont(SubtitleFont, palette );
	mp->CallBackAtFrames(cnt, frames, strrefs);
	mp->Play();
	gamedata->FreePalette( palette );
	if (frames)
		free(frames);
	if (strrefs)
		free(strrefs);
	//restarting music
	if (music)
		music->Start();
	if (ambim) ambim->activate();
	//this will fix redraw all windows as they looked like
	//before the movie
	RedrawAll();

	//Setting the movie name to 1
	vars->SetAt( ResRef, 1 );
	return 0;
}

int Interface::Roll(int dice, int size, int add) const
{
	if (dice < 1) {
		return add;
	}
	if (size < 1) {
		return add;
	}
	if (dice > 100) {
		return add + dice * size / 2;
	}
	for (int i = 0; i < dice; i++) {
		add += rand() % size + 1;
	}
	return add;
}

static char bmp_suffix[6]="M.BMP";
static char png_suffix[6]="M.PNG";

int Interface::GetPortraits(TextArea* ta, bool smallorlarge)
{
	int count = 0;
	char Path[_MAX_PATH];

	if (smallorlarge) {
		bmp_suffix[0]='S';
		png_suffix[0]='S';
	} else {
		bmp_suffix[0]='M';
		png_suffix[0]='M';
	}
	PathJoin( Path, GamePath, GamePortraitsPath, NULL );
	DirectoryIterator dir(Path);
	if (!dir) {
		return -1;
	}
	printf( "Looking in %s\n", Path );
	do {
		char *name = dir.GetName();
		if (name[0] == '.')
			continue;
		if (dir.IsDirectory())
			continue;
		strupr(name);
		char *pos = strstr(name,bmp_suffix);
		if (!pos && IsAvailable(IE_PNG_CLASS_ID) ) {
			pos = strstr(name,png_suffix);
		}
		if (!pos) continue;
		pos[1]=0;
		count++;
		ta->AppendText( name, -1 );
	} while (++dir);
	return count;
}

int Interface::GetCharSounds(TextArea* ta)
{
	bool hasfolders;
	int count = 0;
	char Path[_MAX_PATH];

	PathJoin( Path, GamePath, GameSoundsPath, NULL );
	hasfolders = ( HasFeature( GF_SOUNDFOLDERS ) != 0 );
	DirectoryIterator dir(Path);
	if (!dir) {
		return -1;
	}
	printf( "Looking in %s\n", Path );
	do {
		char *name = dir.GetName();
		if (name[0] == '.')
			continue;
		if (hasfolders == !dir.IsDirectory())
			continue;
		if (!hasfolders) {
			strupr(name);
			char *pos = strstr(name,"A.WAV");
			if (!pos) continue;
			*pos=0;
		}
		count++;
		ta->AppendText( name, -1 );
	} while (++dir);
	return count;
}

int Interface::GetCharacters(TextArea* ta)
{
	int count = 0;
	char Path[_MAX_PATH];

	PathJoin( Path, GamePath, GameCharactersPath, NULL );
	DirectoryIterator dir(Path);
	if (!dir) {
		return -1;
	}
	printf( "Looking in %s\n", Path );
	do {
		char *name = dir.GetName();
		if (name[0] == '.')
			continue;
		if (dir.IsDirectory())
			continue;
		strupr(name);
		char *pos = strstr(name,".CHR");
		if (!pos) continue;
		*pos=0;
		count++;
		ta->AppendText( name, -1 );
	} while (++dir);
	return count;
}

bool Interface::LoadINI(const char* filename)
{
	FILE* config;
	config = fopen( filename, "rb" );
	if (config == NULL) {
		return false;
	}
	char name[65], value[_MAX_PATH + 3];
	while (!feof( config )) {
		name[0] = 0;
		value[0] = 0;
		char rem;

		if (fread( &rem, 1, 1, config ) != 1)
			break;

		if (( rem == '#' ) ||
			( rem == '[' ) ||
			( rem == '\r' ) ||
			( rem == '\n' ) ||
			( rem == ';' )) {
			if (rem == '\r') {
				fgetc( config );
				continue;
			} else if (rem == '\n')
				continue;

			//it should always return zero
			if (fscanf( config, "%*[^\r\n]%*[\r\n]" )!=0)
				break;
			continue;
		}
		fseek( config, -1, SEEK_CUR );
		//the * element is not counted
		if (fscanf( config, "%[^=]=%[^\r\n]%*[\r\n]", name, value )!=2)
			continue;
		if (( value[0] >= '0' ) && ( value[0] <= '9' )) {
			vars->SetAt( name, atoi( value ) );
		}
	}
	fclose( config );
	return true;
}

/** Enables/Disables the Cut Scene Mode */
void Interface::SetCutSceneMode(bool active)
{
	GameControl *gc = GetGameControl();
	if (gc) {
		gc->SetCutSceneMode( active );
	}
	if (game) {
		if (active) {
			game->ControlStatus |= CS_HIDEGUI;
		} else {
			game->ControlStatus &= ~CS_HIDEGUI;
		}
		SetEventFlag(EF_CONTROL);
	}
	video->SetMouseEnabled(!active);
}

/** returns true if in dialogue or cutscene */
bool Interface::InCutSceneMode() const
{
	GameControl *gc = GetGameControl();
	if (!gc || (gc->GetDialogueFlags()&DF_IN_DIALOG) || (gc->GetScreenFlags()&SF_DISABLEMOUSE) ) {
		return true;
	}
	return false;
}

void Interface::QuitGame(int BackToMain)
{
	SetCutSceneMode(false);
	if (timer) {
		//clear cutscenes
		//clear fade/screenshake effects
		timer->Init();
		timer->SetFadeFromColor(0);
	}

	DelAllWindows(); //delete all windows, including GameControl

	//shutting down ingame music
	//(do it before deleting the game)
	if (music) {
		music->HardEnd();
	}
	// stop any ambients which are still enqueued
	if (AudioDriver) {
		AmbientMgr *ambim = AudioDriver->GetAmbientMgr();
		if (ambim) ambim->deactivate();
	}
	//delete game, worldmap
	if (game) {
		delete game;
		game=NULL;
	}
	if (worldmap) {
		delete worldmap;
		worldmap=NULL;
	}
	if (BackToMain) {
		strcpy(NextScript, "Start");
		QuitFlag |= QF_CHANGESCRIPT;
	}
	GSUpdate(true);
}

void Interface::SetupLoadGame(Holder<SaveGame> sg, int ver_override)
{
	LoadGameIndex = sg;
	VersionOverride = ver_override;
	QuitFlag |= QF_LOADGAME;
}

void Interface::LoadGame(SaveGame *sg, int ver_override)
{
	// This function has rather painful error handling,
	// as it should swap all the objects or none at all
	// and the loading can fail for various reasons

	// Yes, it uses goto. Other ways seemed too awkward for me.

	strings->CloseAux();
	tokens->RemoveAll(NULL); //clearing the token dictionary

	if(calendar) delete calendar;
	calendar = new Calendar;

	DataStream* gam_str = NULL;
	DataStream* sav_str = NULL;
	DataStream* wmp_str1 = NULL;
	DataStream* wmp_str2 = NULL;

	Game* new_game = NULL;
	WorldMapArray* new_worldmap = NULL;

	LoadProgress(10);
	if (!KeepCache) DelTree((const char *) CachePath, true);
	LoadProgress(15);

	if (sg == NULL) {
		//Load the Default Game
		gam_str = gamedata->GetResource( GameNameResRef, IE_GAM_CLASS_ID );
		sav_str = NULL;
		wmp_str1 = gamedata->GetResource( WorldMapName[0], IE_WMP_CLASS_ID );
		if (WorldMapName[1][0]) {
			wmp_str2 = gamedata->GetResource( WorldMapName[1], IE_WMP_CLASS_ID );
		}
	} else {
		gam_str = sg->GetGame();
		sav_str = sg->GetSave();
		wmp_str1 = sg->GetWmap(0);
		if (WorldMapName[1][0]) {
			wmp_str2 = sg->GetWmap(1);
			if (!wmp_str2) {
				//upgrade an IWD game to HOW
				wmp_str2 = gamedata->GetResource( WorldMapName[1], IE_WMP_CLASS_ID );
			}
		}
	}

	// These are here because of the goto
	PluginHolder<SaveGameMgr> gam_mgr(IE_GAM_CLASS_ID);
	PluginHolder<WorldMapMgr> wmp_mgr(IE_WMP_CLASS_ID);

	if (!gam_str || !(wmp_str1 || wmp_str2) )
		goto cleanup;

	// Load GAM file
	if (!gam_mgr)
		goto cleanup;

	if (!gam_mgr->Open( gam_str, true ))
		goto cleanup;

	new_game = gam_mgr->LoadGame(new Game(), ver_override);
	if (!new_game)
		goto cleanup;

	gam_str = NULL;

	// Load WMP (WorldMap) file
	if (!wmp_mgr)
		goto cleanup;

	if (!wmp_mgr->Open( wmp_str1, wmp_str2, true ))
		goto cleanup;

	new_worldmap = wmp_mgr->GetWorldMapArray( );

	wmp_str1 = NULL;
	wmp_str2 = NULL;

	LoadProgress(20);
	// Unpack SAV (archive) file to Cache dir
	if (sav_str) {
		PluginHolder<ArchiveImporter> ai(IE_BIF_CLASS_ID);
		if (ai) {
			if (ai->DecompressSaveGame(sav_str) != GEM_OK) {
				goto cleanup;
			}
		}
		delete sav_str;
		sav_str = NULL;
	}

	// Let's assume that now is everything loaded OK and swap the objects

	delete game;
	delete worldmap;

	game = new_game;
	worldmap = new_worldmap;

	strings->OpenAux();
	LoadProgress(70);
	return;
cleanup:
	// Something went wrong, so try to clean after itself

	delete new_game;
	delete new_worldmap;

	delete gam_str;
	delete wmp_str1;
	delete wmp_str2;
	delete sav_str;
}

/* swapping out old resources */
void Interface::UpdateMasterScript()
{
	if (game) {
		game->SetScript( GlobalScript, 0 );
	}

	PluginHolder<WorldMapMgr> wmp_mgr(IE_WMP_CLASS_ID);
	if (! wmp_mgr)
		return;

	if (worldmap) {
		DataStream *wmp_str1 = gamedata->GetResource( WorldMapName[0], IE_WMP_CLASS_ID );
		DataStream *wmp_str2 = gamedata->GetResource( WorldMapName[1], IE_WMP_CLASS_ID );

		if (!wmp_mgr->Open( wmp_str1, wmp_str2, true )) {
			delete wmp_str1;
			delete wmp_str2;
		}

		delete worldmap;
		worldmap = wmp_mgr->GetWorldMapArray();
	}
}

bool Interface::HideGCWindow()
{
	Window *window = GetWindow( 0 );
	// in the beginning, there's no window at all
	if (! window)
		return false;

	Control* gc = window->GetControl(0);
	if (gc->ControlType!=IE_GUI_GAMECONTROL) {
		return false;
	}
	SetVisible(0, WINDOW_INVISIBLE);
	return true;
}

void Interface::UnhideGCWindow()
{
	Window *window = GetWindow( 0 );
	if (!window)
		return;
	Control* gc = window->GetControl(0);
	if (gc->ControlType!=IE_GUI_GAMECONTROL)
		return;
	SetVisible(0, WINDOW_VISIBLE);
}

GameControl *Interface::GetGameControl() const
{
	Window *window = GetWindow( 0 );
	// in the beginning, there's no window at all
	if (! window)
		return NULL;

	Control* gc = window->GetControl(0);
	if (gc == NULL) {
		return NULL;
	}
	if (gc->ControlType!=IE_GUI_GAMECONTROL) {
		return NULL;
	}
	return (GameControl *) gc;
}

bool Interface::InitItemTypes()
{
	if (slotmatrix) {
		free(slotmatrix);
	}
	AutoTable it("itemtype");
	ItemTypes = 0;
	if (it) {
		ItemTypes = it->GetRowCount(); //number of itemtypes
		if (ItemTypes<0) {
			ItemTypes = 0;
		}
		int InvSlotTypes = it->GetColumnCount();
		if (InvSlotTypes > 32) { //bit count limit
			InvSlotTypes = 32;
		}
		//make sure unsigned int is 32 bits
		slotmatrix = (ieDword *) malloc(ItemTypes * sizeof(ieDword) );
		for (int i=0;i<ItemTypes;i++) {
			unsigned int value = 0;
			unsigned int k = 1;
			for (int j=0;j<InvSlotTypes;j++) {
				if (strtol(it->QueryField(i,j),NULL,0) ) {
					value |= k;
				}
				k <<= 1;
			}
			//we let any items in the inventory
			slotmatrix[i] = (ieDword) value | SLOT_INVENTORY;
		}
	}

	//slottype describes the inventory structure
	Inventory::Init(HasFeature(GF_MAGICBIT));
	AutoTable st("slottype");
	if (slottypes) {
		free(slottypes);
		slottypes = NULL;
	}
	SlotTypes = 0;
	if (st) {
		SlotTypes = st->GetRowCount();
		//make sure unsigned int is 32 bits
		slottypes = (SlotType *) malloc(SlotTypes * sizeof(SlotType) );
		memset(slottypes, -1, SlotTypes * sizeof(SlotType) );
		for (unsigned int row = 0; row < SlotTypes; row++) {
			bool alias;
			unsigned int i = (ieDword) strtol(st->GetRowName(row),NULL,0 );
			if (i>=SlotTypes) continue;
			if (slottypes[i].sloteffects!=0xffffffffu) {
				slottypes[row].slot = i;
				i=row;
				alias = true;
			} else {
				slottypes[row].slot = i;
				alias = false;
			}
			slottypes[i].slottype = (ieDword) strtol(st->QueryField(row,0),NULL,0 );
			slottypes[i].slotid = (ieDword) strtol(st->QueryField(row,1),NULL,0 );
			strnlwrcpy( slottypes[i].slotresref, st->QueryField(row,2), 8 );
			slottypes[i].slottip = (ieDword) strtol(st->QueryField(row,3),NULL,0 );
			slottypes[i].slotflags = (ieDword) strtol(st->QueryField(row,5),NULL,0 );
			//don't fill sloteffects for aliased slots (pst)
			if (alias) {
				continue;
			}
			slottypes[i].sloteffects = (ieDword) strtol(st->QueryField(row,4),NULL,0 );
			//setting special slots
			if (slottypes[i].slottype&SLOT_ITEM) {
				if (slottypes[i].slottype&SLOT_INVENTORY) {
					Inventory::SetInventorySlot(i);
				} else {
					Inventory::SetQuickSlot(i);
				}
			}
			switch (slottypes[i].sloteffects) {
				//fist slot, not saved, default weapon
			case SLOT_EFFECT_FIST: Inventory::SetFistSlot(i); break;
				//magic weapon slot, overrides all weapons
			case SLOT_EFFECT_MAGIC: Inventory::SetMagicSlot(i); break;
				//weapon slot, Equipping marker is relative to it
			case SLOT_EFFECT_MELEE: Inventory::SetWeaponSlot(i); break;
				//ranged slot
			case SLOT_EFFECT_MISSILE: Inventory::SetRangedSlot(i); break;
				//right hand
			case SLOT_EFFECT_LEFT: Inventory::SetShieldSlot(i); break;
				//head (for averting critical hit)
			case SLOT_EFFECT_HEAD: Inventory::SetHeadSlot(i); break;
			default:;
			}
		}
	}
	return (it && st);
}

ieDword Interface::FindSlot(unsigned int idx) const
{
	ieDword i;

	for (i=0;i<SlotTypes;i++) {
		if (idx==slottypes[i].slot) {
			break;
		}
	}
	return i;
}

ieDword Interface::QuerySlot(unsigned int idx) const
{
	if (idx>=SlotTypes) {
		return 0;
	}
	return slottypes[idx].slot;
}

ieDword Interface::QuerySlotType(unsigned int idx) const
{
	if (idx>=SlotTypes) {
		return 0;
	}
	return slottypes[idx].slottype;
}

ieDword Interface::QuerySlotID(unsigned int idx) const
{
	if (idx>=SlotTypes) {
		return 0;
	}
	return slottypes[idx].slotid;
}

ieDword Interface::QuerySlottip(unsigned int idx) const
{
	if (idx>=SlotTypes) {
		return 0;
	}
	return slottypes[idx].slottip;
}

ieDword Interface::QuerySlotEffects(unsigned int idx) const
{
	if (idx>=SlotTypes) {
		return 0;
	}
	return slottypes[idx].sloteffects;
}

ieDword Interface::QuerySlotFlags(unsigned int idx) const
{
	if (idx>=SlotTypes) {
		return 0;
	}
	return slottypes[idx].slotflags;
}

const char *Interface::QuerySlotResRef(unsigned int idx) const
{
	if (idx>=SlotTypes) {
		return "";
	}
	return slottypes[idx].slotresref;
}

// checks the itemtype vs. slottype, and also checks the usability flags
// vs. Actor's stats (alignment, class, race, kit etc.)
int Interface::CanUseItemType(int slottype, Item *item, Actor *actor, bool feedback, bool equipped) const
{
	//inventory is a special case, we allow any items to enter it
	if ( slottype==-1 ) {
		return SLOT_INVENTORY;
	}
	//if we look for ALL slot types, then SLOT_SHIELD shouldn't interfere
	//with twohandedness
	//As long as this is an Item, use the ITEM constant
	//switch for IE_INV_ITEM_* if it is a CREItem
	if (item->Flags&IE_ITEM_TWO_HANDED) {
		//if the item is twohanded and there are more slots, drop the shield slot
		if (slottype&~SLOT_SHIELD) {
			slottype&=~SLOT_SHIELD;
		}
		if (slottype&SLOT_SHIELD) {
			//cannot equip twohanded in offhand
			if (feedback) DisplayConstantString(STR_NOT_IN_OFFHAND, 0xf0f0f0);
			return 0;
		}
	}

	if ( (unsigned int) item->ItemType>=(unsigned int) ItemTypes) {
		//invalid itemtype
		if (feedback) DisplayConstantString(STR_WRONGITEMTYPE, 0xf0f0f0);
		return 0;
	}

	//if actor is supplied, check its usability fields
	if (actor) {
		//constant strings
		int idx = actor->Unusable(item);
		if (idx) {
			if (feedback) DisplayConstantString(idx, 0xf0f0f0);
			return 0;
		}
		//custom strings
		ieStrRef str = actor->Disabled(item->Name, item->ItemType);
		if (str && !equipped) {
			if (feedback) DisplayString(str, 0xf0f0f0, 0);
			return 0;
		}
	}

	//if any bit is true, the answer counts as true
	int ret = (slotmatrix[item->ItemType]&slottype);

	if (!ret) {
		if (feedback) DisplayConstantString(STR_WRONGITEMTYPE, 0xf0f0f0);
		return 0;
	}

	//this warning comes only when feedback is enabled
	if (feedback) {
		//this was, but that disabled equipping of amber earrings in PST
		//if (slotmatrix[item->ItemType]&(SLOT_QUIVER|SLOT_WEAPON|SLOT_ITEM)) {
		if (ret&(SLOT_QUIVER|SLOT_WEAPON|SLOT_ITEM)) {
			//don't ruin the return variable, it contains the usable slot bits
			int flg = 0;
			if (ret&SLOT_QUIVER) {
				if (item->GetWeaponHeader(true)) flg = 1;
			}

			if (ret&SLOT_WEAPON) {
				//melee
				if (item->GetWeaponHeader(false)) flg = 1;
				//ranged
				if (item->GetWeaponHeader(true)) flg = 1;
			}

			if (ret&SLOT_ITEM) {
				if (item->GetEquipmentHeaderNumber(0)!=0xffff) flg = 1;
			}

			if (!flg) {
				DisplayConstantString(STR_UNUSABLEITEM, 0xf0f0f0);
				return 0;
			}
		}
	}

	return ret;
}

Label *Interface::GetMessageLabel() const
{
	ieDword WinIndex = (ieDword) -1;
	ieDword TAIndex = (ieDword) -1;

	vars->Lookup( "OtherWindow", WinIndex );
	if (( WinIndex != (ieDword) -1 ) &&
		( vars->Lookup( "MessageLabel", TAIndex ) )) {
		Window* win = GetWindow( (unsigned short) WinIndex );
		if (win) {
			Control *ctrl = win->GetControl( (unsigned short) TAIndex );
			if (ctrl && ctrl->ControlType==IE_GUI_LABEL)
				return (Label *) ctrl;
		}
	}
	return NULL;
}

TextArea *Interface::GetMessageTextArea() const
{
	ieDword WinIndex = (ieDword) -1;
	ieDword TAIndex = (ieDword) -1;

	vars->Lookup( "MessageWindow", WinIndex );
	if (( WinIndex != (ieDword) -1 ) &&
		( vars->Lookup( "MessageTextArea", TAIndex ) )) {
		Window* win = GetWindow( (unsigned short) WinIndex );
		if (win) {
			Control *ctrl = win->GetControl( (unsigned short) TAIndex );
			if (ctrl && ctrl->ControlType==IE_GUI_TEXTAREA)
				return (TextArea *) ctrl;
		}
	}
	return NULL;
}

static const char *saved_extensions[]={".are",".sto",0};
static const char *saved_extensions_last[]={".tot",".toh",0};

//returns the priority of the file to be saved
//2 - save
//1 - save last
//0 - don't save
int Interface::SavedExtension(const char *filename)
{
	const char *str=strchr(filename,'.');
	if (!str) return 0;
	int i=0;
	while(saved_extensions[i]) {
		if (!stricmp(saved_extensions[i], str) ) return 2;
		i++;
	}
	i=0;
	while(saved_extensions_last[i]) {
		if (!stricmp(saved_extensions_last[i], str) ) return 1;
		i++;
	}
	return 0;
}

static const char *protected_extensions[]={".exe",".dll",".so",0};

//returns true if file should be saved
bool Interface::ProtectedExtension(const char *filename)
{
	const char *str=strchr(filename,'.');
	if (!str) return false;
	int i=0;
	while(protected_extensions[i]) {
		if (!stricmp(protected_extensions[i], str) ) return true;
		i++;
	}
	return false;
}

void Interface::RemoveFromCache(const ieResRef resref, SClass_ID ClassID)
{
	char filename[_MAX_PATH];

	snprintf(filename, _MAX_PATH, "%s%.8s%s", CachePath, resref, TypeExt( ClassID ) );
	unlink ( filename);
}

//this function checks if the path is eligible as a cache
//if it contains a directory, or suspicious file extensions
//we bail out, because the cache will be purged regularly.
bool Interface::StupidityDetector(const char* Pt)
{
	char Path[_MAX_PATH];
	strcpy( Path, Pt );
	DirectoryIterator dir(Path);
	if (!dir) {
		printf("\n**cannot open**\n");
		return true;
	}
	do {
		const char *name = dir.GetName();
		if (dir.IsDirectory()) {
			if (name[0] == '.') {
				if (name[1] == '\0')
					continue;
				if (name[1] == '.' && name[2] == '\0')
					continue;
			}
			printf("\n**contains another dir**\n");
			return true; //a directory in there???
		}
		if (ProtectedExtension(name) ) {
			printf("\n**contains alien files**\n");
			return true; //an executable file in there???
		}
	} while (++dir);
	//ok, we got a good conscience
	return false;
}

void Interface::DelTree(const char* Pt, bool onlysave)
{
	char Path[_MAX_PATH];

	if (!Pt[0]) return; //Don't delete the root filesystem :)
	strcpy( Path, Pt );
	DirectoryIterator dir(Path);
	if (!dir) {
		return;
	}
	do {
		char *name = dir.GetName();
		if (dir.IsDirectory())
			continue;
		if (name[0] == '.')
			continue;
		if (!onlysave || SavedExtension(name) ) {
			char dtmp[_MAX_PATH];
			dir.GetFullPath(dtmp);
			unlink( dtmp );
		}
	} while (++dir);
}

void Interface::LoadProgress(int percent)
{
	vars->SetAt("Progress", percent);
	RedrawControls("Progress", percent);
	RedrawAll();
	DrawWindows();
	video->SwapBuffers();
}

void Interface::ReleaseDraggedItem()
{
	DraggedItem=NULL; //shouldn't free this
	video->SetDragCursor (NULL);
}

void Interface::DragItem(CREItem *item, const ieResRef Picture)
{
	//We should drop the dragged item and pick this up,
	//we shouldn't have a valid DraggedItem at this point.
	//Anyway, if there is still a dragged item, it will be destroyed.
	if (DraggedItem) {
		printMessage("Core","Forgot to call ReleaseDraggedItem when leaving inventory (item destroyed)!\n",YELLOW);
		delete DraggedItem;
	}
	DraggedItem = item;
	if (video) {
		Sprite2D* DraggedCursor = NULL;
		if (item) {
			DraggedCursor = gamedata->GetBAMSprite( Picture, 0, 0 );
		}
		video->SetDragCursor (DraggedCursor);
	}
}

void Interface::SetDraggedPortrait(int dp, int idx)
{
	if (idx<0) idx=14;
	DraggedPortrait = dp;
	if (dp) {
		//hmm this might work?
		Cursors[idx]->acquire();
		video->SetDragCursor(Cursors[idx]);
	} else {
		video->SetDragCursor(NULL);
	}
}

bool Interface::ReadItemTable(const ieResRef TableName, const char * Prefix)
{
	ieResRef ItemName;
	int i,j;

	AutoTable tab(TableName);
	if (!tab) {
		return false;
	}
	i=tab->GetRowCount();
	for(j=0;j<i;j++) {
		if (Prefix) {
			snprintf(ItemName,sizeof(ItemName),"%s%02d",Prefix, j+1);
		} else {
			strnlwrcpy(ItemName,tab->GetRowName(j), 8);
		}
		//Variable elements are free'd, so we have to use malloc
		//well, not anymore, we can use ReleaseFunction
		int l=tab->GetColumnCount(j);
		if (l<1) continue;
		int cl = atoi(tab->GetColumnName(0));
		ItemList *itemlist = new ItemList(l, cl);
		for(int k=0;k<l;k++) {
			strnlwrcpy(itemlist->ResRefs[k],tab->QueryField(j,k), 8);
		}
		RtRows->SetAt(ItemName, (void*)itemlist);
	}
	return true;
}

bool Interface::ReadRandomItems()
{
	ieResRef RtResRef;
	int i;

	ieDword difflev=0; //rt norm or rt fury
	vars->Lookup("Nightmare Mode", difflev);
	if (RtRows) {
		RtRows->RemoveAll(ReleaseItemList);
	}
	else {
		RtRows=new Variables(10, 17); //block size, hash table size
		if (!RtRows) {
			return false;
		}
		RtRows->SetType( GEM_VARIABLES_POINTER );
	}
	AutoTable tab("randitem");
	if (!tab) {
		return false;
	}
	if (difflev>=tab->GetColumnCount()) {
		difflev = tab->GetColumnCount()-1;
	}

	//the gold item
	strnlwrcpy( GoldResRef, tab->QueryField((unsigned int) 0,(unsigned int) 0), 8);
	if ( GoldResRef[0]=='*' ) {
		return false;
	}
	strnlwrcpy( RtResRef, tab->QueryField( 1, difflev ), 8);
	i=atoi( RtResRef );
	if (i<1) {
		ReadItemTable( RtResRef, 0 ); //reading the table itself
		return true;
	}
	if (i>5) {
		i=5;
	}
	while(i--) {
		strnlwrcpy( RtResRef, tab->QueryField(2+i,difflev), 8);
		ReadItemTable( RtResRef,tab->GetRowName(2+i) );
	}
	return true;
}

CREItem *Interface::ReadItem(DataStream *str)
{
	CREItem *itm = new CREItem();
	if (ReadItem(str, itm)) return itm;
	delete itm;
	return NULL;
}

CREItem *Interface::ReadItem(DataStream *str, CREItem *itm)
{
	str->ReadResRef( itm->ItemResRef );
	str->ReadWord( &itm->Expired );
	str->ReadWord( &itm->Usages[0] );
	str->ReadWord( &itm->Usages[1] );
	str->ReadWord( &itm->Usages[2] );
	str->ReadDword( &itm->Flags );
	if (ResolveRandomItem(itm) ) {
		return itm;
	}
	return NULL;
}

#define MAX_LOOP 10

//This function generates random items based on the randitem.2da file
//there could be a loop, but we don't want to freeze, so there is a limit
bool Interface::ResolveRandomItem(CREItem *itm)
{
	if (!RtRows) return true;
	for(int loop=0;loop<MAX_LOOP;loop++) {
		int i,j,k;
		char *endptr;
		ieResRef NewItem;

		void* lookup;
		if ( !RtRows->Lookup( itm->ItemResRef, lookup ) ) {
			return true;
		}
		ItemList *itemlist = (ItemList*)lookup;
		if (itemlist->WeightOdds) {
			//instead of 1d19 we calculate with 2d10 (which also has 19 possible values)
			i=Roll(2,(itemlist->Count+1)/2,-2);
		} else {
			i=Roll(1,itemlist->Count,-1);
		}
		strnlwrcpy( NewItem, itemlist->ResRefs[i], 8);
		char *p=(char *) strchr(NewItem,'*');
		if (p) {
			*p=0; //doing this so endptr is ok
			k=strtol(p+1,NULL,10);
		} else {
			k=1;
		}
		j=strtol(NewItem,&endptr,10);
		if (j<1) {
			j=1;
		}
		if (*endptr) {
			strnlwrcpy(itm->ItemResRef, NewItem, 8);
		} else {
			strnlwrcpy(itm->ItemResRef, GoldResRef, 8);
		}
		if ( !memcmp( itm->ItemResRef,"no_drop",8 ) ) {
			itm->ItemResRef[0]=0;
		}
		if (!itm->ItemResRef[0]) {
			return false;
		}
		itm->Usages[0]=(ieWord) Roll(j,k,0);
	}
	printMessage("Interface"," ",LIGHT_RED);
	printf("Loop detected while generating random item:%s\n",itm->ItemResRef);
	return false;
}

//now that we store spell name in spl, i guess, we shouldn't pass 'ieResRef name'
//these functions are needed because Win32 doesn't allow freeing memory from
//another dll. So we allocate all commonly used memories from core
ITMExtHeader *Interface::GetITMExt(int count)
{
	return new ITMExtHeader[count];
}

SPLExtHeader *Interface::GetSPLExt(int count)
{
	return new SPLExtHeader[count];
}

Effect *Interface::GetEffect(ieDword opcode)
{
	if (opcode==0xffffffff) {
		return NULL;
	}
	Effect *fx = new Effect();
	if (!fx) {
		return NULL;
	}
	memset(fx,0,sizeof(Effect));
	fx->Opcode=opcode;
	return fx;
}

Effect *Interface::GetFeatures(int count)
{
	return new Effect[count];
}

/*
void Interface::FreeITMExt(ITMExtHeader *p, Effect *e)
{
	delete [] p;
	delete [] e;
}

void Interface::FreeSPLExt(SPLExtHeader *p, Effect *e)
{
	delete [] p;
	delete [] e;
}
*/

WorldMapArray *Interface::NewWorldMapArray(int count)
{
	return new WorldMapArray(count);
}

Container *Interface::GetCurrentContainer()
{
	return CurrentContainer;
}

int Interface::CloseCurrentContainer()
{
	UseContainer = false;
	if ( !CurrentContainer) {
		return -1;
	}
	//remove empty ground piles on closeup
	CurrentContainer->GetCurrentArea()->TMap->CleanupContainer(CurrentContainer);
	CurrentContainer = NULL;
	return 0;
}

void Interface::SetCurrentContainer(Actor *actor, Container *arg, bool flag)
{
	//abort action if the first selected PC isn't the original actor
	if (actor!=GetFirstSelectedPC(false)) {
		CurrentContainer = NULL;
		return;
	}
	CurrentContainer = arg;
	UseContainer = flag;
}

Store *Interface::GetCurrentStore()
{
	return CurrentStore;
}

int Interface::CloseCurrentStore()
{
	if ( !CurrentStore ) {
		return -1;
	}
	PluginHolder<StoreMgr> sm(IE_STO_CLASS_ID);
	if (sm == NULL) {
		return -1;
	}
	int size = sm->GetStoredFileSize (CurrentStore);
	if (size > 0) {
		//created streams are always autofree (close file on destruct)
		//this one will be destructed when we return from here
		FileStream str;

		str.Create( CurrentStore->Name, IE_STO_CLASS_ID );
		int ret = sm->PutStore (&str, CurrentStore);
		if (ret <0) {
			printMessage("Core"," ", YELLOW);
			printf("Store removed: %s\n", CurrentStore->Name);
			RemoveFromCache(CurrentStore->Name, IE_STO_CLASS_ID);
		}
	} else {
		printMessage("Core"," ", YELLOW);
		printf("Store removed: %s\n", CurrentStore->Name);
		RemoveFromCache(CurrentStore->Name, IE_STO_CLASS_ID);
	}
	//make sure the stream isn't connected to sm, or it will be double freed
	delete CurrentStore;
	CurrentStore = NULL;
	return 0;
}

Store *Interface::SetCurrentStore(const ieResRef resname, ieDword owner)
{
	if ( CurrentStore ) {
		if ( !strnicmp(CurrentStore->Name, resname, 8) ) {
			return CurrentStore;
		}

		//not simply delete the old store, but save it
		CloseCurrentStore();
	}

	DataStream* str = gamedata->GetResource( resname, IE_STO_CLASS_ID );
	PluginHolder<StoreMgr> sm(IE_STO_CLASS_ID);
	if (sm == NULL) {
		delete ( str );
		return NULL;
	}
	if (!sm->Open( str, true )) {
		return NULL;
	}

	// FIXME - should use some already allocated in core
	// not really, only one store is open at a time, then it is
	// unloaded, we don't really have to cache it, it will be saved in
	// Cache anyway!
	CurrentStore = sm->GetStore( new Store() );
	if (CurrentStore == NULL) {
		return NULL;
	}
	strnlwrcpy(CurrentStore->Name, resname, 8);
	if (owner) {
		CurrentStore->SetOwnerID(owner);
	}
	return CurrentStore;
}

void Interface::SetMouseScrollSpeed(int speed) {
	mousescrollspd = (speed+1)*2;
}

int Interface::GetMouseScrollSpeed() {
	return mousescrollspd;
}

ieStrRef Interface::GetRumour(const ieResRef dlgref)
{
	PluginHolder<DialogMgr> dm(IE_DLG_CLASS_ID);
	dm->Open( gamedata->GetResource( dlgref, IE_DLG_CLASS_ID ), true );
	Dialog *dlg = dm->GetDialog();

	if (!dlg) {
		printMessage("Interface"," ", LIGHT_RED);
		printf( "Cannot load dialog: %s\n", dlgref );
		return (ieStrRef) -1;
	}
	Scriptable *pc=game->GetPC( game->GetSelectedPCSingle(), false );

	ieStrRef ret = (ieStrRef) -1;
	int i = dlg->FindRandomState( pc );
	if (i>=0 ) {
		ret = dlg->GetState( i )->StrRef;
	}
	delete dlg;
	return ret;
}

void Interface::DoTheStoreHack(Store *s)
{
	size_t size = s->PurchasedCategoriesCount * sizeof( ieDword );
	s->purchased_categories=(ieDword *) malloc(size);

	size = s->CuresCount * sizeof( STOCure );
	s->cures=(STOCure *) malloc(size);

	size = s->DrinksCount * sizeof( STODrink );
	s->drinks=(STODrink *) malloc(size);

	for(size=0;size<s->ItemsCount;size++) {
		STOItem *si = new STOItem();
		memset(si, 0, sizeof(STOItem) );
		s->items.push_back( si );
	}
}

//plays stock sound listed in defsound.2da
void Interface::PlaySound(int index)
{
	if (index<=DSCount) {
		AudioDriver->Play(DefSound[index]);
	}
}

Actor *Interface::GetFirstSelectedPC(bool forced)
{
	Actor *ret = NULL;
	int slot = 0;
	int partySize = game->GetPartySize( false );
	if (!partySize) return NULL;
	for (int i = 0; i < partySize; i++) {
		Actor* actor = game->GetPC( i,false );
		if (actor->IsSelected()) {
			if (actor->InParty<slot || !ret) {
				ret = actor;
				slot = actor->InParty;
			}
		}
	}

	if (forced && !ret) {
		return game->FindPC((unsigned int) 0);
	}
	return ret;
}

Actor *Interface::GetFirstSelectedActor()
{
	if (game->selected.size()) {
		return game->selected[0];
	}
	return NULL;
}

//this is used only for the console
Sprite2D *Interface::GetCursorSprite()
{
	Sprite2D *spr = gamedata->GetBAMSprite(CursorBam, 0, 0);
	if (spr)
	{
		if(HasFeature(GF_OVERRIDE_CURSORPOS))
		{
			spr->XPos=1;
			spr->YPos=spr->Height-1;
		}
	}
	return spr;
}

Sprite2D *Interface::GetScrollCursorSprite(int frameNum, int spriteNum)
{
	return gamedata->GetBAMSprite(ScrollCursorBam, frameNum, spriteNum);
}

/* we should return -1 if it isn't gold, otherwise return the gold value */
int Interface::CanMoveItem(const CREItem *item) const
{
	//This is an inventory slot, switch to IE_ITEM_* if you use Item
	if (!HasFeature(GF_NO_DROP_CAN_MOVE) ) {
		if (item->Flags & IE_INV_ITEM_UNDROPPABLE)
			return 0;
	}
	//not gold, we allow only one single coin ResRef, this is good
	//for all of the original games
	if (strnicmp(item->ItemResRef, GoldResRef, 8 ) )
		return -1;
	//gold, returns the gold value (stack size)
	return item->Usages[0];
}

// dealing with applying effects
void Interface::ApplySpell(const ieResRef resname, Actor *actor, Scriptable *caster, int level)
{
	Spell *spell = gamedata->GetSpell(resname);
	if (!spell) {
		return;
	}

	int header = spell->GetHeaderIndexFromLevel(level);
	EffectQueue *fxqueue = spell->GetEffectBlock(caster, actor->Pos, header, level);

	ApplyEffectQueue(fxqueue, actor, caster, actor->Pos);
	delete fxqueue;
}

void Interface::ApplySpellPoint(const ieResRef resname, Map* area, const Point &pos, Scriptable *caster, int level)
{
	Spell *spell = gamedata->GetSpell(resname);
	if (!spell) {
		return;
	}
	int header = spell->GetHeaderIndexFromLevel(level);
	Projectile *pro = spell->GetProjectile(caster, header, pos);
	pro->SetCaster(caster->GetGlobalID(), level);
	area->AddProjectile(pro, caster->Pos, pos);
}

//-1 means the effect was reflected back to the caster
//0 means the effect was resisted and should be removed
//1 means the effect was applied
int Interface::ApplyEffect(Effect *effect, Actor *actor, Scriptable *caster)
{
	if (!effect) {
		return 0;
	}

	EffectQueue *fxqueue = new EffectQueue();
	//AddEffect now copies the fx data, please delete your effect reference
	//if you created it. (Don't delete cached references)
	fxqueue->AddEffect( effect );
	int res = ApplyEffectQueue(fxqueue, actor, caster);
	delete fxqueue;
	return res;
}

int Interface::ApplyEffectQueue(EffectQueue *fxqueue, Actor *actor, Scriptable *caster)
{
	Point p;
	p.empty(); //the effect should have all its coordinates already set
	return ApplyEffectQueue(fxqueue, actor, caster, p);
}

int Interface::ApplyEffectQueue(EffectQueue *fxqueue, Actor *actor, Scriptable *caster, Point p)
{
	int res = fxqueue->CheckImmunity ( actor );
	if (res) {
		if (res == -1 ) {
			//bounced back at a nonliving caster
			if (caster->Type!=ST_ACTOR) {
				return 0;
			}
			actor = (Actor *) caster;
		}
		fxqueue->SetOwner( caster );

		if (fxqueue->AddAllEffects( actor, p)==FX_NOT_APPLIED) {
			res=0;
		}
	}
	return res;
}

Effect *Interface::GetEffect(const ieResRef resname, int level, const Point &p)
{
	//Don't free this reference, it is cached!
	Effect *effect = gamedata->GetEffect(resname);
	if (!effect) {
		return NULL;
	}
	if (!level) {
		level = 1;
	}
	effect->Power = level;
	effect->PosX=p.x;
	effect->PosY=p.y;
	return effect;
}

// dealing with saved games
int Interface::SwapoutArea(Map *map)
{
	PluginHolder<MapMgr> mm(IE_ARE_CLASS_ID);
	if (mm == NULL) {
		return -1;
	}
	int size = mm->GetStoredFileSize (map);
	if (size > 0) {
		//created streams are always autofree (close file on destruct)
		//this one will be destructed when we return from here
		FileStream str;

		str.Create( map->GetScriptName(), IE_ARE_CLASS_ID );
		int ret = mm->PutArea (&str, map);
		if (ret <0) {
			printMessage("Core"," ", YELLOW);
			printf("Area removed: %s\n", map->GetScriptName());
			RemoveFromCache(map->GetScriptName(), IE_ARE_CLASS_ID);
		}
	} else {
		printMessage("Core"," ", YELLOW);
		printf("Area removed: %s\n", map->GetScriptName());
		RemoveFromCache(map->GetScriptName(), IE_ARE_CLASS_ID);
	}
	//make sure the stream isn't connected to sm, or it will be double freed
	return 0;
}

int Interface::WriteCharacter(const char *name, Actor *actor)
{
	char Path[_MAX_PATH];

	PathJoin( Path, GamePath, GameCharactersPath, NULL );
	if (!actor) {
		return -1;
	}
	PluginHolder<ActorMgr> gm(IE_CRE_CLASS_ID);
	if (gm == NULL) {
		return -1;
	}

	//str is freed
	{
		FileStream str;

		if (!str.Create( Path, name, IE_CHR_CLASS_ID ))
			return -1;

		int ret = gm->PutActor(&str, actor, true);
		if (ret <0) {
			printMessage("Core"," ", YELLOW);
			printf("Character cannot be saved: %s\n", name);
			return -1;
		}
	}

	//write the BIO string
	if (!HasFeature(GF_NO_BIOGRAPHY)) {
		FileStream str;

		str.Create( Path, name, IE_BIO_CLASS_ID );
		//never write the string reference into this string
		char *tmp = GetString(actor->GetVerbalConstant(VB_BIO),IE_STR_STRREFOFF);
		str.Write (tmp, strlen(tmp));
		free(tmp);
	}
	return 0;
}

int Interface::WriteGame(const char *folder)
{
	PluginHolder<SaveGameMgr> gm(IE_GAM_CLASS_ID);
	if (gm == NULL) {
		return -1;
	}

	int size = gm->GetStoredFileSize (game);
	if (size > 0) {
		//created streams are always autofree (close file on destruct)
		//this one will be destructed when we return from here
		FileStream str;

		str.Create( folder, GameNameResRef, IE_GAM_CLASS_ID );
		int ret = gm->PutGame (&str, game);
		if (ret <0) {
			printMessage("Core"," ", YELLOW);
			printf("Game cannot be saved: %s\n", folder);
			return -1;
		}
	} else {
		printMessage("Core"," ", YELLOW);
		printf("Internal error, game cannot be saved: %s\n", folder);
		return -1;
	}
	return 0;
}

int Interface::WriteWorldMap(const char *folder)
{
	PluginHolder<WorldMapMgr> wmm(IE_WMP_CLASS_ID);
	if (wmm == NULL) {
		return -1;
	}

	if (WorldMapName[1][0]) {
		worldmap->SetSingle(false);
	}

	int size1 = wmm->GetStoredFileSize (worldmap, 0);
	int size2 = 1; //just a dummy value

	//if size is 0 for the first worldmap, then there is a problem
	if (!worldmap->IsSingle() && (size1>0) ) {
		size2=wmm->GetStoredFileSize (worldmap, 1);
	}

	int ret = 0;
	if ((size1 < 0) || (size2<0) ) {
		ret=-1;
	} else {
		//created streams are always autofree (close file on destruct)
		//this one will be destructed when we return from here
		FileStream str1;
		FileStream str2;

		str1.Create( folder, WorldMapName[0], IE_WMP_CLASS_ID );
		if (!worldmap->IsSingle()) {
			str2.Create( folder, WorldMapName[1], IE_WMP_CLASS_ID );
		}
		ret = wmm->PutWorldMap (&str1, &str2, worldmap);
	}
	if (ret <0) {
		printMessage("Core"," ", YELLOW);
		printf("Internal error, worldmap cannot be saved: %s\n", folder);
		return -1;
	}
	return 0;
}

int Interface::CompressSave(const char *folder)
{
	FileStream str;

	str.Create( folder, GameNameResRef, IE_SAV_CLASS_ID );
	DirectoryIterator dir(CachePath);
	if (!dir) {
		return -1;
	}
	//BIF and SAV are the same
	PluginHolder<ArchiveImporter> ai(IE_BIF_CLASS_ID);
	ai->CreateArchive( &str);

	//.tot and .toh should be saved last, because they are updated when an .are is saved
	int priority=2;
	while(priority) {
		do {
			const char *name = dir.GetName();
			if (dir.IsDirectory())
				continue;
			if (name[0] == '.')
				continue;
			if (SavedExtension(name)==priority) {
				char dtmp[_MAX_PATH];
				dir.GetFullPath(dtmp);
				FileStream fs;
				fs.Open(dtmp, true);
				ai->AddToSaveGame(&str, &fs);
			}
		} while (++dir);
		//reopen list for the second round
		priority--;
		if (priority>0) {
			dir.Rewind();
		}
	}
	return 0;
}

int Interface::GetMaximumAbility() const { return MaximumAbility; }

int Interface::GetStrengthBonus(int column, int value, int ex) const
{
	//to hit, damage, open doors, weight allowance
	if (column<0 || column>3)
		return -9999;

	if (value<0)
		value = 0;
	else if (value>25)
		value = 25;

	if (ex<0)
		ex=0;
	else if (ex>100)
		ex=100;

	return strmod[column*(MaximumAbility+1)+value]+strmodex[column*101+ex];
}

//The maze columns are used only in the maze spell, no need to restrict them further
int Interface::GetIntelligenceBonus(int column, int value) const
{
	//learn spell, max spell level, max spell number on level, maze duration dice, maze duration dice size
	if (column<0 || column>4) return -9999;

	return intmod[column*(MaximumAbility+1)+value];
}

int Interface::GetDexterityBonus(int column, int value) const
{
	//no dexmod in iwd2 and only one type of modifier
	if (HasFeature(GF_3ED_RULES)) {
		return (value-10)/2;
	}

	//reaction, missile, ac
	if (column<0 || column>2)
		return -9999;

	return dexmod[column*(MaximumAbility+1)+value];
}

int Interface::GetConstitutionBonus(int column, int value) const
{
	//no conmod in iwd2
	if (HasFeature(GF_3ED_RULES)) {
		return (value-10)/2;
	}

	//normal, warrior, minimum, regen hp, regen fatigue
	if (column<0 || column>4)
		return -9999;

	return conmod[column*(MaximumAbility+1)+value];
}

int Interface::GetCharismaBonus(int column, int /*value*/) const
{
	// store price reduction
	if (column<0 || column>(MaximumAbility-1))
		return -9999;

	return chrmod[column];
}

int Interface::GetLoreBonus(int column, int value) const
{
	//no lorebon in iwd2 - lore is a skill
	if (HasFeature(GF_3ED_RULES)) return 0;

	if (column<0 || column>0)
		return -9999;

	return lorebon[value];
}

int Interface::GetWisdomBonus(int column, int value) const
{
	//no wismod in iwd2
	if (HasFeature(GF_3ED_RULES)) {
		return (value-10)/2;
	}

	if (!HasFeature(GF_WISDOM_BONUS)) return 0;

	// xp bonus
	if (column<0 || column>0)
		return -9999;

	return wisbon[value];
}

int Interface::GetReputationMod(int column) const
{
	int reputation = game->Reputation / 10 - 1;

	if (column<0 || column>8) {
		return -9999;
	}
	if (reputation > 19) {
		reputation = 19;
	}
	if (reputation < 0) {
		reputation = 0;
	}

	return reputationmod[reputation][column];
}

// -3, -2 if request is illegal or in cutscene
// -1 if pause is already active
// 0 if pause was not allowed
// 1 if autopause happened
int Interface::Autopause(ieDword flag)
{
	GameControl *gc = GetGameControl();
	if (!gc) {
		return -3;
	}
	if (InCutSceneMode()) {
		return -2;
	}
	if (gc->GetDialogueFlags()&DF_FREEZE_SCRIPTS) {
		return -1;
	}
	ieDword autopause_flags = 0;

	vars->Lookup("Auto Pause State", autopause_flags);
	if (autopause_flags & (1<<flag)) {
		DisplayConstantString(STR_AP_UNUSABLE+flag, 0xff0000);
		gc->SetDialogueFlags(DF_FREEZE_SCRIPTS, BM_OR);
		return 1;
	}
	return 0;
}

void Interface::SetInfoTextColor(const Color &color)
{
	if (InfoTextPalette) {
		gamedata->FreePalette(InfoTextPalette);
	}
	InfoTextPalette = CreatePalette(color, black);
}

//todo row?
void Interface::GetResRefFrom2DA(const ieResRef resref, ieResRef resource1, ieResRef resource2, ieResRef resource3)
{
	if (!resource1) {
		return;
	}
	resource1[0]=0;
	if (resource2) {
		resource2[0]=0;
	}
	if (resource3) {
		resource3[0]=0;
	}
	AutoTable tab(resref);
	if (tab) {
		unsigned int cols = tab->GetColumnCount();
		unsigned int row = (unsigned int) Roll(1,tab->GetRowCount(),-1);
		strnuprcpy(resource1, tab->QueryField(row,0), 8);
		if (resource2 && cols>1)
			strnuprcpy(resource2, tab->QueryField(row,1), 8);
		if (resource3 && cols>2)
			strnuprcpy(resource3, tab->QueryField(row,2), 8);
	}
}

ieDword *Interface::GetListFrom2DAInternal(const ieResRef resref)
{
	ieDword *ret;

	AutoTable tab(resref);
	if (tab) {
		ieDword cnt = tab->GetRowCount();
		ret = (ieDword *) malloc((1+cnt)*sizeof(ieDword));
		ret[0]=cnt;
		while(cnt) {
			ret[cnt]=strtol(tab->QueryField(cnt-1, 0),NULL, 0);
			cnt--;
		}
		return ret;
	}
	ret = (ieDword *) malloc(sizeof(ieDword));
	ret[0]=0;
	return ret;
}

ieDword* Interface::GetListFrom2DA(const ieResRef tablename)
{
	ieDword *list;

	if (!lists->Lookup(tablename, (void *&) list)) {
		list = GetListFrom2DAInternal(tablename);
		lists->SetAt(tablename, list);
	}

	return list;
}

//returns a numeric value associated with a stat name (symbol) from stats.ids
ieDword Interface::TranslateStat(const char *stat_name)
{
	long tmp;

	if (valid_number(stat_name, tmp)) {
		return (ieDword) tmp;
	}

	int symbol = LoadSymbol( "stats" );
	Holder<SymbolMgr> sym = GetSymbol( symbol );
	ieDword stat = (ieDword) sym->GetValue( stat_name );
	if (stat==(ieDword) ~0) {
		printMessage("Core"," ",YELLOW);
		printf("Cannot translate symbol: %s\n", stat_name);
	}
	return stat;
}

void Interface::WaitForDisc(int disc_number, const char* path)
{
	GetDictionary()->SetAt( "WaitForDisc", (ieDword) disc_number );

	GetGUIScriptEngine()->RunFunction( "GUICommonWindows", "OpenWaitForDiscWindow" );
	do {
		DrawWindows();
		for (size_t i=0;i<CD[disc_number-1].size();i++) {
			char name[_MAX_PATH];

			PathJoin(name, CD[disc_number-1][i].c_str(),path,NULL);
			if (file_exists (name)) {
				GetGUIScriptEngine()->RunFunction( "GUICommonWindows", "OpenWaitForDiscWindow" );
				return;
			}
		}

	} while (video->SwapBuffers() == GEM_OK);
}

// remove the extraneus EOL newline and carriage return
void Interface::StripLine(char * string, size_t size) {
	if (size >= 2 && string[size-2] == '\n') {
		string[size-2] = '\0';
	}
	if (size >= 3 && string[size-3] == '\r') {
		string[size-3] = '\0'; // remove the carriage return too
	}
}

void Interface::SetTickHook(EventHandler hook)
{
	TickHook = hook;
}

void Interface::SetNextScript(const char *script)
{
	strncpy( NextScript, script, sizeof(NextScript) );
	QuitFlag |= QF_CHANGESCRIPT;
}

void Interface::SanityCheck(const char *ver) {
	if (strcmp(ver, VERSION_GEMRB)) {
		printf("version check failed: core version %s doesn't match caller's version %s\n", VERSION_GEMRB, ver);
		abort();
	}
}
