/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2004 The GemRB Project
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

// This class represents the .gam (savegame) file in the engine

#include "Game.h"

#include "defsounds.h"
#include "strrefs.h"
#include "win32def.h"

#include "DisplayMessage.h"
#include "GameData.h"
#include "Interface.h"
#include "IniSpawn.h"
#include "MapMgr.h"
#include "MusicMgr.h"
#include "Particles.h"
#include "PluginMgr.h"
#include "ScriptEngine.h"
#include "TableMgr.h"
#include "GameScript/GameScript.h"
#include "GUI/GameControl.h"
#include "System/DataStream.h"

#define MAX_MAPS_LOADED 1

Game::Game(void) : Scriptable( ST_GLOBAL )
{
	protagonist = PM_YES; //set it to 2 for iwd/iwd2 and 0 for pst
	partysize = 6;
	Ticks = 0;
	version = 0;
	Expansion = 0;
	LoadMos[0] = 0;
	SelectedSingle = 1; //the PC we are looking at (inventory, shop)
	PartyGold = 0;
	SetScript( core->GlobalScript, 0 );
	MapIndex = -1;
	Reputation = 0;
	ControlStatus = 0;
	CombatCounter = 0; //stored here until we know better
	StateOverrideTime = 0;
	StateOverrideFlag = 0;
	BanterBlockTime = 0;
	BanterBlockFlag = 0;
	WeatherBits = 0;
	crtable = NULL;
	kaputz = NULL;
	beasts = NULL;
	mazedata = NULL;
	timestop_owner = NULL;
	timestop_end = 0;
	event_timer = 0;
	event_handler = NULL;
	weather = new Particles(200);
	weather->SetRegion(0, 0, core->Width, core->Height);
	LastScriptUpdate = 0;

	//loading master areas
	AutoTable table;
	if (table.load("mastarea")) {
		int i = table->GetRowCount();
		mastarea.reserve(i);
		while(i--) {
			char *tmp = (char *) malloc(9);
			strnuprcpy (tmp,table->QueryField(i,0),8);
			mastarea.push_back( tmp );
		}
	}

	//loading rest/daylight switching movies (only bg2 has them)
	memset(restmovies,'*',sizeof(restmovies));
	memset(daymovies,'*',sizeof(restmovies));
	memset(nightmovies,'*',sizeof(restmovies));
	if (table.load("restmov")) {
		for(int i=0;i<8;i++) {
			strnuprcpy(restmovies[i],table->QueryField(i,0),8);
			strnuprcpy(daymovies[i],table->QueryField(i,1),8);
			strnuprcpy(nightmovies[i],table->QueryField(i,2),8);
		}
	}

	//loading npc starting levels
	ieResRef tn;
	if (Expansion == 5) { // tob is special
		strncpy(tn, "npclvl25", sizeof(ieResRef));
	} else {
		strncpy(tn, "npclevel", sizeof(ieResRef));
	}
	if (table.load(tn)) {
		int cols = table->GetColumnCount();
		int rows = table->GetRowCount();
		int i, j;
		npclevels.reserve(rows);
		for (i = 0; i < rows; i++) {
			npclevels.push_back (std::vector<char *>(cols+1));
			for(j = -1; j < cols; j++) {
				char *ref = new char[9];
				if (j == -1) {
					strncpy(ref, table->GetRowName(i), sizeof(ieResRef));
					npclevels[i][j+1] = ref;
				} else {
					strncpy(ref, table->QueryField(i, j), sizeof(ieResRef));
					npclevels[i][j+1] = ref;
				}
			}
		}
	}

	interval = 1000/AI_UPDATE_TIME;
	hasInfra = false;
	familiarBlock = false;
	//FIXME:i'm not sure in this...
	NoInterrupt();
	bntchnc = NULL;
	bntrows = -1;
}

Game::~Game(void)
{
	size_t i;

	delete weather;
	for (i = 0; i < Maps.size(); i++) {
		delete( Maps[i] );
	}
	for (i = 0; i < PCs.size(); i++) {
		delete ( PCs[i] );
	}
	for (i = 0; i < NPCs.size(); i++) {
		delete ( NPCs[i] );
	}
	for (i = 0; i < mastarea.size(); i++) {
		free ( mastarea[i] );
	}

	if (crtable) {
		delete[] crtable;
	}

	if (mazedata) {
		free (mazedata);
	}
	if (kaputz) {
		delete kaputz;
	}
	if (beasts) {
		free (beasts);
	}
	i=Journals.size();
	while(i--) {
		delete Journals[i];
	}

	i=savedpositions.size();
	while(i--) {
		free (savedpositions[i]);
	}

	i=planepositions.size();
	while(i--) {
		free (planepositions[i]);
	}

	i = npclevels.size();
	while (i--) {
		size_t j = npclevels[i].size();
		while(j--) {
			delete [] npclevels[i][j];
		}
	}
}

static bool IsAlive(Actor *pc)
{
	if (pc->GetStat(IE_STATE_ID)&STATE_DEAD) {
		return false;
	}
	return true;
}

int Game::FindPlayer(unsigned int partyID)
{
	for (unsigned int slot=0; slot<PCs.size(); slot++) {
		if (PCs[slot]->InParty==partyID) {
			return slot;
		}
	}
	return -1;
}

Actor* Game::FindPC(unsigned int partyID)
{
	for (unsigned int slot=0; slot<PCs.size(); slot++) {
		if (PCs[slot]->InParty==partyID) return PCs[slot];
	}
	return NULL;
}

Actor* Game::FindPC(const char *scriptingname)
{
	for (unsigned int slot=0; slot<PCs.size(); slot++) {
		if (strnicmp(PCs[slot]->GetScriptName(),scriptingname,32)==0 ) {
			return PCs[slot];
		}
	}
	return NULL;
}

Actor* Game::FindNPC(unsigned int partyID)
{
	for (unsigned int slot=0; slot<NPCs.size(); slot++) {
		if (NPCs[slot]->InParty==partyID) return NPCs[slot];
	}
	return NULL;
}

Actor* Game::FindNPC(const char *scriptingname)
{
	for (unsigned int slot=0; slot<NPCs.size(); slot++) {
		if (strnicmp(NPCs[slot]->GetScriptName(),scriptingname,32)==0 )
		{
			return NPCs[slot];
		}
	}
	return NULL;
}

Actor *Game::GetGlobalActorByGlobalID(ieDword globalID)
{
	unsigned int slot;

	for (slot=0; slot<PCs.size(); slot++) {
		if (PCs[slot]->GetGlobalID()==globalID ) {
			return PCs[slot];
		}
	}
	for (slot=0; slot<NPCs.size(); slot++) {
		if (NPCs[slot]->GetGlobalID()==globalID ) {
			return NPCs[slot];
		}
	}
	return NULL;
}

Actor* Game::GetPC(unsigned int slot, bool onlyalive)
{
	if (slot >= PCs.size()) {
		return NULL;
	}
	if (onlyalive) {
		unsigned int i=0;
		while(i<PCs.size() ) {
			Actor *ac = PCs[i++];

			if (IsAlive(ac) ) {
				if (!slot--) {
					return ac;
				}
			}
		}
		return NULL;
	}
	return PCs[slot];
}

int Game::InStore(Actor* pc) const
{
	for (unsigned int i = 0; i < NPCs.size(); i++) {
		if (NPCs[i] == pc) {
			return i;
		}
	}
	return -1;
}

int Game::InParty(Actor* pc) const
{
	for (unsigned int i = 0; i < PCs.size(); i++) {
		if (PCs[i] == pc) {
			return i;
		}
	}
	return -1;
}

int Game::DelPC(unsigned int slot, bool autoFree)
{
	if (slot >= PCs.size()) {
		return -1;
	}
	if (!PCs[slot]) {
		return -1;
	}
	SelectActor(PCs[slot], false, SELECT_NORMAL);
	if (autoFree) {
		delete( PCs[slot] );
	}
	std::vector< Actor*>::iterator m = PCs.begin() + slot;
	PCs.erase( m );
	return 0;
}

int Game::DelNPC(unsigned int slot, bool autoFree)
{
	if (slot >= NPCs.size()) {
		return -1;
	}
	if (!NPCs[slot]) {
		return -1;
	}
	if (autoFree) {
		delete( NPCs[slot] );
	}
	std::vector< Actor*>::iterator m = NPCs.begin() + slot;
	NPCs.erase( m );
	return 0;
}

//i'm sure this could be faster
void Game::ConsolidateParty()
{
	int max = (int) PCs.size();
	std::vector< Actor*>::const_iterator m;
	for (int i=1;i<=max;) {
		if (FindPlayer(i)==-1) {

			for ( m = PCs.begin(); m != PCs.end(); ++m) {
				if ( (*m)->InParty>i) {
					(*m)->InParty--;
				}
			}
		} else i++;
	}
	for ( m = PCs.begin(); m != PCs.end(); ++m) {
		(*m)->RefreshEffects(NULL);
		//TODO: how to set up bardsongs
		(*m)->SetModalSpell((*m)->ModalState, 0);
	}
}

int Game::LeaveParty (Actor* actor)
{
	core->SetEventFlag(EF_PORTRAIT);
	actor->CreateStats(); //create or update stats for leaving
	actor->SetBase(IE_EXPLORE, 0);

	SelectActor(actor, false, SELECT_NORMAL);
	int slot = InParty( actor );
	if (slot < 0) {
		return slot;
	}
	std::vector< Actor*>::iterator m = PCs.begin() + slot;
	PCs.erase( m );

	ieDword id = actor->GetGlobalID();
	for ( m = PCs.begin(); m != PCs.end(); ++m) {
		(*m)->PCStats->LastLeft = id;
		if ( (*m)->InParty>actor->InParty) {
			(*m)->InParty--;
		}
	}
	//removing from party, but actor remains in 'game'
	actor->SetPersistent(0);
	NPCs.push_back( actor );

	if (core->HasFeature( GF_HAS_DPLAYER )) {
		actor->SetScript( "", SCR_DEFAULT );
	}
	actor->SetBase( IE_EA, EA_NEUTRAL );
	return ( int ) NPCs.size() - 1;
}

//determines if startpos.2da has rotation rows (it cannot have tutorial line)
bool Game::DetermineStartPosType(const TableMgr *strta)
{
	if ((strta->GetRowCount()>=6) && !stricmp(strta->GetRowName(4),"START_ROT" ) )
	{
		return true;
	}
	return false;
}

#define PMODE_COUNT 3

void Game::InitActorPos(Actor *actor)
{
	//start.2da row labels
	const char *mode[PMODE_COUNT] = { "NORMAL", "TUTORIAL", "EXPANSION" };

	unsigned int ip = (unsigned int) (actor->InParty-1);
	AutoTable start("start");
	AutoTable strta("startpos");

	if (!start || !strta) {
		error("Game", "Game is missing character start data.\n");
	}
	// 0 - single player, 1 - tutorial, 2 - expansion
	ieDword playmode = 0;
	core->GetDictionary()->Lookup( "PlayMode", playmode );

	//Sometimes playmode is set to -1 (in pregenerate)
	//normally execution shouldn't ever come here, but it actually does
	//preventing problems by defaulting to the regular entry points
	if (playmode>PMODE_COUNT) {
		playmode = 0;
	}
	const char *xpos = start->QueryField(mode[playmode],"XPOS");
	const char *ypos = start->QueryField(mode[playmode],"YPOS");
	const char *area = start->QueryField(mode[playmode],"AREA");
	const char *rot = start->QueryField(mode[playmode],"ROT");

	actor->Pos.x = actor->Destination.x = (short) atoi( strta->QueryField( strta->GetRowIndex(xpos), ip ) );
	actor->Pos.y = actor->Destination.y = (short) atoi( strta->QueryField( strta->GetRowIndex(ypos), ip ) );
	actor->SetOrientation( atoi( strta->QueryField( strta->GetRowIndex(rot), ip) ), false );

	if (strta.load("startare")) {
		strnlwrcpy(actor->Area, strta->QueryField( strta->GetRowIndex(area), 0 ), 8 );
	} else {
		strnlwrcpy(actor->Area, CurrentArea, 8 );
	}
}

int Game::JoinParty(Actor* actor, int join)
{
	core->SetEventFlag(EF_PORTRAIT);
	actor->CreateStats(); //create stats if they didn't exist yet
	actor->InitButtons(actor->GetStat(IE_CLASS), false); //init actor's buttons
	actor->SetBase(IE_EXPLORE, 1);
	if (join&JP_INITPOS) {
		InitActorPos(actor);
	}
	int slot = InParty( actor );
	if (slot != -1) {
		return slot;
	}
	size_t size = PCs.size();
	//set the lastjoined trigger

	if (join&JP_JOIN) {
		//update kit abilities of actor
		actor->ApplyKit(false);
		//update the quickslots
		actor->ReinitQuickSlots();
		//set the joining date
		actor->PCStats->JoinDate = GameTime;
		if (size) {
			ieDword id = actor->GetGlobalID();
			for (size_t i=0;i<size; i++) {
				Actor *a = GetPC(i, false);
				a->PCStats->LastJoined = id;
			}
		} else {
			Reputation = actor->GetStat(IE_REPUTATION);
		}
	}
	slot = InStore( actor );
	if (slot >= 0) {
		std::vector< Actor*>::iterator m = NPCs.begin() + slot;
		NPCs.erase( m );
	}


	PCs.push_back( actor );
	if (!actor->InParty) {
		actor->InParty = (ieByte) (size+1);
	}

	if (join&(JP_INITPOS|JP_SELECT)) {
		actor->Selected = 0; // don't confuse SelectActor!
		SelectActor(actor,true, SELECT_NORMAL);
	}

	return ( int ) size;
}

int Game::GetPartySize(bool onlyalive) const
{
	if (onlyalive) {
		int count = 0;
		for (unsigned int i = 0; i < PCs.size(); i++) {
			if (!IsAlive(PCs[i])) {
				continue;
			}
			count++;
		}
		return count;
	}
	return (int) PCs.size();
}

/* sends the hotkey trigger to all selected actors */
void Game::SetHotKey(unsigned long Key)
{
	std::vector< Actor*>::const_iterator m;

	for ( m = selected.begin(); m != selected.end(); ++m) {
		Actor *actor = *m;

		if (actor->IsSelected()) {
			actor->AddTrigger(TriggerEntry(trigger_hotkey, (ieDword) Key));
		}
	}
}

bool Game::SelectPCSingle(int index)
{
	Actor* actor = FindPC( index );
	if (!actor || ! actor->ValidTarget( GA_NO_HIDDEN ))
		return false;

	SelectedSingle = index;
	return true;
}

int Game::GetSelectedPCSingle() const
{
	return SelectedSingle;
}

/*
 * SelectActor() - handle (de)selecting actors.
 * If selection was changed, runs "SelectionChanged" handler
 *
 * actor - either specific actor, or NULL for all
 * select - whether actor(s) should be selected or deselected
 * flags:
 * SELECT_REPLACE - if true, deselect all other actors when selecting one
 * SELECT_QUIET   - do not run handler if selection was changed. Used for
 * nested calls to SelectActor()
 */

bool Game::SelectActor(Actor* actor, bool select, unsigned flags)
{
	std::vector< Actor*>::iterator m;

	// actor was not specified, which means all selectables should be (de)selected
	if (! actor) {
		for ( m = selected.begin(); m != selected.end(); ++m) {
			(*m)->Select( false );
			(*m)->SetOver( false );
		}
		selected.clear();

		if (select) {
			area->SelectActors();
/*
			for ( m = PCs.begin(); m != PCs.end(); ++m) {
				if (! *m) {
					continue;
				}
				SelectActor( *m, true, SELECT_QUIET );
			}
*/
		}

		if (! (flags & SELECT_QUIET)) {
			core->SetEventFlag(EF_SELECTION);
		}
		Infravision();
		return true;
	}

	// actor was specified, so we will work with him
	if (select) {
		if (! actor->ValidTarget( GA_SELECT | GA_NO_DEAD ))
			return false;

		// deselect all actors first when exclusive
		if (flags & SELECT_REPLACE) {
			if (selected.size() == 1 && actor->IsSelected()) {
				assert(selected[0] == actor);
				// already the only selected actor
				return true;
			}
			SelectActor( NULL, false, SELECT_QUIET );
		} else if (actor->IsSelected()) {
			// already selected
			return true;
		}

		actor->Select( true );
		assert(actor->IsSelected());
		selected.push_back( actor );
	} else {
		if (!actor->IsSelected()) {
			// already not selected
			return true;

			/*for ( m = selected.begin(); m != selected.end(); ++m) {
				assert((*m) != actor);
			}
			return true;*/
		}
		for ( m = selected.begin(); m != selected.end(); ++m) {
			if ((*m) == actor) {
				selected.erase( m );
				break;
			}
		}
		actor->Select( false );
		assert(!actor->IsSelected());
	}

	if (! (flags & SELECT_QUIET)) {
		core->SetEventFlag(EF_SELECTION);
	}
	Infravision();
	return true;
}

// Gets average party level, of onlyalive is true, then counts only living PCs
int Game::GetPartyLevel(bool onlyalive) const
{
	int count = 0;
	for (unsigned int i = 0; i<PCs.size(); i++) {
			if (onlyalive) {
				if (PCs[i]->GetStat(IE_STATE_ID)&STATE_DEAD) {
					continue;
				}
			}
			count += PCs[i]->GetXPLevel(0);
	}
	return count;
}

// Returns map structure (ARE) if it is already loaded in memory
int Game::FindMap(const char *ResRef)
{
	int index = (int) Maps.size();
	while (index--) {
		Map *map=Maps[index];
		if (strnicmp(ResRef, map->GetScriptName(), 8) == 0) {
			return index;
		}
	}
	return -1;
}

Map* Game::GetMap(unsigned int index) const
{
	if (index >= Maps.size()) {
		return NULL;
	}
	return Maps[index];
}

Map *Game::GetMap(const char *areaname, bool change)
{
	int index = LoadMap(areaname, change);
	if (index >= 0) {
		if (change) {
			MapIndex = index;
			area = GetMap(index);
			memcpy (CurrentArea, areaname, 8);
			area->SetupAmbients();
			//change the tileset if needed
			area->ChangeMap(IsDay());
			ChangeSong(false, true);
			Infravision();

			//call area customization script for PST
			//moved here because the current area is set here
			ScriptEngine *sE = core->GetGUIScriptEngine();
			if (core->HasFeature(GF_AREA_OVERRIDE) && sE) {
				//area ResRef is accessible by GemRB.GetGameString (STR_AREANAME)
				sE->RunFunction("Maze", "CustomizeArea");
			}

			return area;
		}
		return GetMap(index);
	}
	return NULL;
}

bool Game::MasterArea(const char *area)
{
	unsigned int i=(int) mastarea.size();
	while(i--) {
		if (strnicmp(mastarea[i], area, 8) ) {
			return true;
		}
	}
	return false;
}

void Game::SetMasterArea(const char *area)
{
	if (MasterArea(area) ) return;
	char *tmp = (char *) malloc(9);
	strnlwrcpy (tmp,area,8);
	mastarea.push_back(tmp);
}

int Game::AddMap(Map* map)
{
	if (MasterArea(map->GetScriptName()) ) {
		Maps.insert(Maps.begin(), 1, map);
		MapIndex++;
		return 0;
	}
	unsigned int i = (unsigned int) Maps.size();
	Maps.push_back( map );
	return i;
}

int Game::DelMap(unsigned int index, int forced)
{
//this function should archive the area, and remove it only if the area
//contains no active actors (combat, partymembers, etc)
	if (index >= Maps.size()) {
		return -1;
	}
	Map *map = Maps[index];

	if (MapIndex==(int) index) { //can't remove current map in any case
		const char *name = map->GetScriptName();
		memcpy(AnotherArea, name, sizeof(AnotherArea) );
		return -1;
	}


	if (!map) { //this shouldn't happen, i guess
		printMessage("Game","Erased NULL Map\n",YELLOW);
		Maps.erase( Maps.begin()+index);
		if (MapIndex>(int) index) {
			MapIndex--;
		}
		return 1;
	}

	if (forced || (Maps.size()>MAX_MAPS_LOADED) )
	{
		//keep at least one master
		const char *name = map->GetScriptName();
		if (MasterArea(name)) {
			if(!AnotherArea[0]) {
				memcpy(AnotherArea, name, sizeof(AnotherArea));
				if (!forced) {
					return -1;
				}
			}
		}
		//this check must be the last, because
		//after PurgeActors you cannot keep the
		//area in memory
		//Or the queues should be regenerated!
		if (!map->CanFree())
		{
			return 1;
		}
		//remove map from memory
		core->SwapoutArea(Maps[index]);
		delete( Maps[index] );
		Maps.erase( Maps.begin()+index);
		//current map will be decreased
		if (MapIndex>(int) index) {
			MapIndex--;
		}
		return 1;
	}
	//didn't remove the map
	return 0;
}

/* Loads an area */
int Game::LoadMap(const char* ResRef, bool loadscreen)
{
	unsigned int i, last;
	Map *newMap;
	PluginHolder<MapMgr> mM(IE_ARE_CLASS_ID);
	ScriptEngine *sE = core->GetGUIScriptEngine();

	//this shouldn't happen
	if (!mM) {
		return -1;
	}

	int index = FindMap(ResRef);
	if (index>=0) {
		return index;
	}

	bool hide = false;
	if (loadscreen && sE) {
		hide = core->HideGCWindow();
		sE->RunFunction("LoadScreen", "StartLoadScreen");
		sE->RunFunction("LoadScreen", "SetLoadScreen");
	}
	DataStream* ds = gamedata->GetResource( ResRef, IE_ARE_CLASS_ID );
	if (!ds) {
		goto failedload;
	}
	if(!mM->Open(ds)) {
		goto failedload;
	}
	newMap = mM->GetMap(ResRef, IsDay());
	if (!newMap) {
		goto failedload;
	}

	core->LoadProgress(100);

	for (i = 0; i < PCs.size(); i++) {
		if (stricmp( PCs[i]->Area, ResRef ) == 0) {
			newMap->AddActor( PCs[i] );
		}
	}
	// count the number of replaced actors, so we don't need to recheck them
	// if their max level is still lower than ours, each check would also result in a substitution
	last = NPCs.size()-1;
	for (i = 0; i < NPCs.size(); i++) {
		if (stricmp( NPCs[i]->Area, ResRef ) == 0) {
			if (i < last && CheckForReplacementActor(i)) {
				i--;
				last--;
				continue;
			}
			newMap->AddActor( NPCs[i] );
		}
	}
	if (hide) {
		core->UnhideGCWindow();
	}
	return AddMap( newMap );
failedload:
	if (hide) 
		core->UnhideGCWindow();
	core->LoadProgress(100);
	return -1;
}

// check if the actor is in npclevel.2da and replace accordingly
bool Game::CheckForReplacementActor(int i) {
	if (core->InCutSceneMode() || npclevels.empty()) {
		return false;
	}

	Actor* act = NPCs[i];
	ieDword level = GetPartyLevel(false) / GetPartySize(false);
	if (!(act->Modified[IE_MC_FLAGS]&MC_BEENINPARTY) && !(act->Modified[IE_STATE_ID]&STATE_DEAD) && act->GetXPLevel(false) < level) {
		ieResRef newcre = "****"; // default table value
		std::vector<std::vector<char *> >::iterator it;
		for (it = npclevels.begin(); it != npclevels.end(); it++) {
			if (!stricmp((*it)[0], act->GetScriptName()) && (level > 2)) {
				strncpy(newcre, (*it)[level-2], sizeof(ieResRef));
				break;
			}
		}

		if (stricmp(newcre, "****")) {
			int pos = gamedata->LoadCreature(newcre, 0, false, act->version);
			if (pos < 0) {
				error("Game::CheckForReplacementActor", "LoadCreature failed: pos is negative!\n");
			} else {
				Actor *newact = GetNPC(pos);
				if (!newact) {
					error("Game::CheckForReplacementActor", "GetNPC failed: cannot find act!\n");
				} else {
					newact->Pos = act->Pos; // the map is not loaded yet, so no SetPosition
					strncpy(newact->Area, act->Area, sizeof(ieResRef));
					DelNPC(InStore(act));
					return true;
				}
			}
		}
	}
	return false;
}

int Game::AddNPC(Actor* npc)
{
	int slot = InStore( npc ); //already an npc
	if (slot != -1) {
		return slot;
	}
	slot = InParty( npc );
	if (slot != -1) {
		return -1;
	} //can't add as npc already in party
	npc->SetPersistent(0);
	NPCs.push_back( npc );

	return (int) NPCs.size() - 1;
}

Actor* Game::GetNPC(unsigned int Index)
{
	if (Index >= NPCs.size()) {
		return NULL;
	}
	return NPCs[Index];
}

void Game::SwapPCs(unsigned int Index1, unsigned int Index2)
{
	if (Index1 >= PCs.size()) {
		return;
	}

	if (Index2 >= PCs.size()) {
		return;
	}
	int tmp = PCs[Index1]->InParty;
	PCs[Index1]->InParty = PCs[Index2]->InParty;
	PCs[Index2]->InParty = tmp;
	//signal a change of the portrait window
	core->SetEventFlag(EF_PORTRAIT | EF_SELECTION);
}

void Game::DeleteJournalEntry(ieStrRef strref)
{
	size_t i=Journals.size();
	while(i--) {
		if ((Journals[i]->Text==strref) || (strref==(ieStrRef) -1) ) {
			delete Journals[i];
			Journals.erase(Journals.begin()+i);
		}
	}
}

void Game::DeleteJournalGroup(int Group)
{
	size_t i=Journals.size();
	while(i--) {
		if (Journals[i]->Group==(ieByte) Group) {
			delete Journals[i];
			Journals.erase(Journals.begin()+i);
		}
	}
}
/* returns true if it modified or added a journal entry */
bool Game::AddJournalEntry(ieStrRef strref, int Section, int Group)
{
	GAMJournalEntry *je = FindJournalEntry(strref);
	if (je) {
		//don't set this entry again in the same section
		if (je->Section==Section) {
			return false;
		}
		if ((Section == IE_GAM_QUEST_DONE) && Group) {
			//removing all of this group and adding a new entry
			DeleteJournalGroup(Group);
		} else {
			//modifying existing entry
			je->Section = (ieByte) Section;
			je->Group = (ieByte) Group;
			ieDword chapter = 0;
			if (!core->HasFeature(GF_NO_NEW_VARIABLES)) {
				locals->Lookup("CHAPTER", chapter);
			}
			je->Chapter = (ieByte) chapter;
			je->GameTime = GameTime;
			return true;
		}
	}
	je = new GAMJournalEntry;
	je->GameTime = GameTime;
	ieDword chapter = 0;
	if (!core->HasFeature(GF_NO_NEW_VARIABLES)) {
		locals->Lookup("CHAPTER", chapter);
	}
	je->Chapter = (ieByte) chapter;
	je->unknown09 = 0;
	je->Section = (ieByte) Section;
	je->Group = (ieByte) Group;
	je->Text = strref;

	Journals.push_back( je );
	return true;
}

void Game::AddJournalEntry(GAMJournalEntry* entry)
{
	Journals.push_back( entry );
}

unsigned int Game::GetJournalCount() const
{
	return (unsigned int) Journals.size();
}

GAMJournalEntry* Game::FindJournalEntry(ieStrRef strref)
{
	unsigned int Index = (unsigned int) Journals.size();
	while(Index--) {
		GAMJournalEntry *ret = Journals[Index];

		if (ret->Text==strref) {
			return ret;
		}
	}

	return NULL;
}

GAMJournalEntry* Game::GetJournalEntry(unsigned int Index)
{
	if (Index >= Journals.size()) {
		return NULL;
	}
	return Journals[Index];
}

unsigned int Game::GetSavedLocationCount() const
{
	return (unsigned int) savedpositions.size();
}

void Game::ClearSavedLocations()
{
	size_t i=savedpositions.size();
	while(i--) {
		delete savedpositions[i];
	}
	savedpositions.clear();
}

GAMLocationEntry* Game::GetSavedLocationEntry(unsigned int i)
{
	size_t current = savedpositions.size();
	if (i>=current) {
		if (i>PCs.size()) {
			return NULL;
		}
		savedpositions.resize(i+1);
		while(current<=i) {
			savedpositions[current++]=(GAMLocationEntry *) calloc(1, sizeof(GAMLocationEntry) );
		}
	}
	return savedpositions[i];
}

unsigned int Game::GetPlaneLocationCount() const
{
	return (unsigned int) planepositions.size();
}

void Game::ClearPlaneLocations()
{
	size_t i=planepositions.size();
	while(i--) {
		delete planepositions[i];
	}
	planepositions.clear();
}

GAMLocationEntry* Game::GetPlaneLocationEntry(unsigned int i)
{
	size_t current = planepositions.size();
	if (i>=current) {
		if (i>PCs.size()) {
			return NULL;
		}
		planepositions.resize(i+1);
		while(current<=i) {
			planepositions[current++]=(GAMLocationEntry *) calloc(1, sizeof(GAMLocationEntry) );
		}
	}
	return planepositions[i];
}

char *Game::GetFamiliar(unsigned int Index)
{
	return Familiars[Index];
}

//reading the challenge rating table for iwd2 (only when needed)
void Game::LoadCRTable()
{
	AutoTable table("moncrate");
	if (table.ok()) {
		int maxrow = table->GetRowCount()-1;
		crtable = new CRRow[MAX_LEVEL];
		for(int i=0;i<MAX_LEVEL;i++) {
			//row shouldn't be larger than maxrow
			int row = i<maxrow?i:maxrow;
			int maxcol = table->GetColumnCount(row)-1;
			for(int j=0;j<MAX_CRLEVEL;j++) {
				//col shouldn't be larger than maxcol
				int col = j<maxcol?j:maxcol;
				crtable[i][j]=atoi(table->QueryField(row,col) );
			}
		}
	}
}

int Game::GetXPFromCR(int cr)
{
	if (!crtable) LoadCRTable();
	if (crtable) {
		int level = GetPartyLevel(true);
		if (cr>=MAX_CRLEVEL) {
			cr=MAX_CRLEVEL-1;
		}
		printMessage("Game", "Challenge Rating: %d, party level: %d\n", LIGHT_WHITE, cr, level);
		return crtable[level][cr];
	}
	printMessage("Game","Cannot find moncrate.2da!\n", LIGHT_RED);
	return 0;
}

void Game::ShareXP(int xp, int flags)
{
	int individual;

	if (flags&SX_CR) {
		xp = GetXPFromCR(xp);
	}

	if (flags&SX_DIVIDE) {
		int PartySize = GetPartySize(true); //party size, only alive
		if (PartySize<1) {
			return;
		}
		individual = xp / PartySize;
	} else {
		individual = xp;
	}

	if (!individual) {
		return;
	}

	if (xp>0) {
		displaymsg->DisplayConstantStringValue( STR_GOTXP, DMC_BG2XPGREEN, (ieDword) xp); //you have gained ... xp
	} else {
		displaymsg->DisplayConstantStringValue( STR_LOSTXP, DMC_BG2XPGREEN, (ieDword) -xp); //you have lost ... xp
	}
	for (unsigned int i=0; i<PCs.size(); i++) {
		if (PCs[i]->GetStat(IE_STATE_ID)&STATE_DEAD) {
			continue;
		}
		PCs[i]->AddExperience(individual);
	}
}

bool Game::EveryoneStopped() const
{
	for (unsigned int i=0; i<PCs.size(); i++) {
		if (PCs[i]->GetNextStep() ) return false;
	}
	return true;
}

//canmove=true: if some PC can't move (or hostile), then this returns false
bool Game::EveryoneNearPoint(Map *area, const Point &p, int flags) const
{
	for (unsigned int i=0; i<PCs.size(); i++) {
		if (flags&ENP_ONLYSELECT) {
			if(!PCs[i]->Selected) {
				continue;
			}
		}
		if (PCs[i]->GetStat(IE_STATE_ID)&STATE_DEAD) {
			continue;
		}
		if (flags&ENP_CANMOVE) {
			//someone is uncontrollable, can't move
			if (PCs[i]->GetStat(IE_EA)>EA_GOODCUTOFF) {
				return false;
			}

			if (PCs[i]->GetStat(IE_STATE_ID)&STATE_CANTMOVE) {
				return false;
			}
		}
		if (PCs[i]->GetCurrentArea()!=area) {
			return false;
		}
		if (Distance(p,PCs[i])>MAX_TRAVELING_DISTANCE) {
			printMessage("Game", "Actor %s is not near!\n", LIGHT_WHITE, PCs[i]->LongName);
			return false;
		}
	}
	return true;
}

//called when someone died
void Game::PartyMemberDied(Actor *actor)
{
	//this could be null, in some extreme cases...
	Map *area = actor->GetCurrentArea();

	for (unsigned int i=0; i<PCs.size(); i++) {
		if (PCs[i]==actor) {
			continue;
		}
		if (PCs[i]->GetStat(IE_STATE_ID)&STATE_DEAD) {
			continue;
		}
		if (PCs[i]->GetCurrentArea()!=area) {
			continue;
		}
		PCs[i]->ReactToDeath(actor->GetScriptName());
	}
}

void Game::IncrementChapter()
{
	//chapter first set to 0 (prologue)
	ieDword chapter = (ieDword) -1;
	locals->Lookup("CHAPTER",chapter);
	//increment chapter only if it exists
	locals->SetAt("CHAPTER", chapter+1, core->HasFeature(GF_NO_NEW_VARIABLES) );
	//clear statistics
	for (unsigned int i=0; i<PCs.size(); i++) {
		//all PCs must have this!
		PCs[i]->PCStats->IncrementChapter();
	}
}

void Game::SetReputation(ieDword r)
{
	if (r<10) r=10;
	else if (r>200) r=200;
	if (Reputation>r) {
		displaymsg->DisplayConstantStringValue(STR_LOSTREP, DMC_GOLD, (Reputation-r)/10);
	} else if (Reputation<r) {
		displaymsg->DisplayConstantStringValue(STR_GOTREP, DMC_GOLD, (r-Reputation)/10);
	}
	Reputation = r;
	for (unsigned int i=0; i<PCs.size(); i++) {
		PCs[i]->SetBase(IE_REPUTATION, Reputation);
	}
}

void Game::SetControlStatus(int value, int mode)
{
	switch(mode) {
		case BM_OR: ControlStatus|=value; break;
		case BM_NAND: ControlStatus&=~value; break;
		case BM_SET: ControlStatus=value; break;
		case BM_AND: ControlStatus&=value; break;
		case BM_XOR: ControlStatus^=value; break;
	}
	core->SetEventFlag(EF_CONTROL);
}

void Game::AddGold(ieDword add)
{
	ieDword old;

	if (!add) {
		return;
	}
	old = PartyGold;
	PartyGold += add;
	if (old<PartyGold) {
		displaymsg->DisplayConstantStringValue( STR_GOTGOLD, DMC_GOLD, PartyGold-old);
	} else {
		displaymsg->DisplayConstantStringValue( STR_LOSTGOLD, DMC_GOLD, old-PartyGold);
	}
}

//later this could be more complicated
void Game::AdvanceTime(ieDword add)
{
	ieDword h = GameTime/(300*AI_UPDATE_TIME);
	GameTime+=add;
	if (h!=GameTime/(300*AI_UPDATE_TIME)) {
		//asking for a new weather when the hour changes
		WeatherBits&=~WB_HASWEATHER;
	}
	Ticks+=add*interval;
	//change the tileset if needed
	Map *map = GetCurrentArea();
	if (map && map->ChangeMap(IsDay())) {
		//play the daylight transition movie appropriate for the area
		//it is needed to play only when the area truly changed its tileset
		//this is signalled by ChangeMap
		int areatype = (area->AreaType&(AT_FOREST|AT_CITY|AT_DUNGEON))>>3;
		ieResRef *res;

		printMessage("Game","Switching DayLight\n",GREEN);
		if (IsDay()) {
			res=&nightmovies[areatype];
		} else {
			res=&daymovies[areatype];
		}
		if (*res[0]!='*') {
			core->PlayMovie(*res);
		}
	}
}

//returns true if there are excess players in the team
bool Game::PartyOverflow() const
{
	GameControl *gc = core->GetGameControl();
	if (!gc) {
		return false;
	}
	//don't start this screen when the gui is busy
	if (gc->GetDialogueFlags() & (DF_IN_DIALOG|DF_IN_CONTAINER|DF_FREEZE_SCRIPTS) ) {
		return false;
	}
	if (!partysize) {
		return false;
	}
	return (PCs.size()>partysize);
}

bool Game::AnyPCInCombat() const
{
	if (!CombatCounter) {
		return false;
	}

	return true;
}

//returns true if the protagonist (or the whole party died)
bool Game::EveryoneDead() const
{
	//if there are no PCs, then we assume everyone dead
	if (!PCs.size() ) {
		return true;
	}
	if (protagonist==PM_NO) {
		Actor *nameless = PCs[0];
		if (nameless->GetStat(IE_STATE_ID)&STATE_NOSAVE) {
			if (area->INISpawn) {
				area->INISpawn->RespawnNameless();
			}
		}
		return false;
	}
	// if protagonist died
	if (protagonist==PM_YES) {
		if (PCs[0]->GetStat(IE_STATE_ID)&STATE_NOSAVE) {
			return true;
		}
		return false;
	}
	//protagonist == 2
	for (unsigned int i=0; i<PCs.size(); i++) {
		if (!(PCs[i]->GetStat(IE_STATE_ID)&STATE_NOSAVE) ) {
			return false;
		}
	}
	return true;
}

//runs all area scripts

void Game::UpdateScripts()
{
	Update();
	size_t idx;

	PartyAttack = false;

	for (idx=0;idx<Maps.size();idx++) {
		Maps[idx]->UpdateScripts();
	}

	if (PartyAttack) {
		//ChangeSong will set the battlesong only if CombatCounter is nonzero
		CombatCounter=150;
		ChangeSong(false, true);
	} else {
		if (CombatCounter) {
			CombatCounter--;
			//Change song if combatcounter went down to 0
			if (!CombatCounter) {
				ChangeSong(false, false);
			}
		}
	}

	if (StateOverrideTime)
		StateOverrideTime--;
	if (BanterBlockTime)
		BanterBlockTime--;

	if (Maps.size()>MAX_MAPS_LOADED) {
		idx = Maps.size();

		//starting from 0, so we see the most recent master area first
		for(unsigned int i=0;i<idx;i++) {
			DelMap( (unsigned int) i, false );
		}
	}

	// perhaps a StartMusic action stopped the area music?
	// (we should probably find a less silly way to handle this,
	// because nothing can ever stop area music now..)
	if (!core->GetMusicMgr()->IsPlaying()) {
		ChangeSong(false,false);
	}

	//this is used only for the death delay so far
	if (event_handler) {
		if (!event_timer) {
			event_handler->call();
			event_handler = NULL;
		}
		event_timer--;
	}

	if (EveryoneDead()) {
		//don't check it any more
		protagonist = PM_NO;
		core->GetGUIScriptEngine()->RunFunction("GUIWORLD", "DeathWindow");
		return;
	}

	if (PartyOverflow()) {
		partysize = 0;
		core->GetGUIScriptEngine()->RunFunction("GUIWORLD", "OpenReformPartyWindow");
		return;
	}
}

void Game::SetTimedEvent(EventHandler func, int count)
{
	event_timer = count;
	event_handler = func;
}

void Game::SetProtagonistMode(int mode)
{
	protagonist = mode;
}

void Game::SetPartySize(int size)
{
	// 0 size means no party size control
	if (size<0) {
		return;
	}
	partysize = (size_t) size;
}

//Get the area dependent rest movie
ieResRef *Game::GetDream(Map *area)
{
	//select dream based on area
	int daynight = IsDay();
	if (area->Dream[daynight][0]) {
		return area->Dream+daynight;
	}
	int dream = (area->AreaType&(AT_FOREST|AT_CITY|AT_DUNGEON))>>3;
	return restmovies+dream;
}

//Start dream cutscenes for player1
void Game::PlayerDream()
{
	Scriptable *Sender = GetPC(0,true);
	if (!Sender) return;

	GameScript* gs = new GameScript( "player1d", Sender,0,0 );
	gs->Update();
	delete( gs );
}

//noareacheck = no random encounters
//dream = 0 - based on area non-0 - select from list
//-1 no dream
//hp is how much hp the rest will heal
void Game::RestParty(int checks, int dream, int hp)
{
	if (!(checks&REST_NOMOVE) ) {
		if (!EveryoneStopped()) {
			return;
		}
	}
	Actor *leader = GetPC(0, true);
	if (!leader) {
		return;
	}

	Map *area = leader->GetCurrentArea();
	//we let them rest if someone is paralyzed, but the others gather around
	if (!(checks&REST_NOSCATTER) ) {
		if (!EveryoneNearPoint( area, leader->Pos, 0 ) ) {
			//party too scattered
			displaymsg->DisplayConstantString( STR_SCATTERED, DMC_RED );
			return;
		}
	}

	if (!(checks&REST_NOCRITTER) ) {
		//don't allow resting while in combat
		if (AnyPCInCombat()) {
			displaymsg->DisplayConstantString( STR_CANTRESTMONS, DMC_RED );
			return;
		}
		//don't allow resting if hostiles are nearby
		if (area->AnyEnemyNearPoint(leader->Pos)) {
			displaymsg->DisplayConstantString( STR_CANTRESTMONS, DMC_RED );
			return;
		}
	}

	//rest check, if PartyRested should be set, area should return true
	int hours = 8;
	int hoursLeft = 0;
	if (!(checks&REST_NOAREA) ) {
		//you cannot rest here
		if (area->AreaFlags&1) {
			displaymsg->DisplayConstantString( STR_MAYNOTREST, DMC_RED );
			return;
		}
		//you may not rest here, find an inn
		if (!(area->AreaType&(AT_OUTDOOR|AT_FOREST|AT_DUNGEON|AT_CAN_REST) ))
		{
			displaymsg->DisplayConstantString( STR_MAYNOTREST, DMC_RED );
			return;
		}
		//area encounters
		// also advances gametime (so partial rest is possible)
		hoursLeft = area->Rest( leader->Pos, hours, (GameTime/AI_UPDATE_TIME)%7200/3600);
		if (hoursLeft) {
			// partial rest only, so adjust the parameters for the loop below
			if (hp) {
				hp = hp * (hours - hoursLeft) / hours;
				// 0 means full heal, so we need to cancel it if we rounded to 0
				if (!hp) {
					hp = 1;
				}
			}
			hours -= hoursLeft;
			// the interruption occured before any resting could be done, so just bail out
			if (!hours) {
				return;
			}
		}
	} else {
		AdvanceTime(hours*300*AI_UPDATE_TIME);
	}

	int i = GetPartySize(true); // party size, only alive

	while (i--) {
		Actor *tar = GetPC(i, true);
		tar->ClearPath();
		tar->ClearActions();
		tar->SetModal(MS_NONE, 0);
		//if hp = 0, then healing will be complete
		tar->Heal(hp);
		//removes fatigue, recharges spells
		tar->Rest(hours);
		if (!hoursLeft)
			tar->PartyRested();
	}

	// abort the partial rest; we got what we wanted
	if (hoursLeft) {
		return;
	}

	//movie and cutscene dreams
	if (dream>=0) {
		//cutscene dreams
		if (gamedata->Exists("player1d",IE_BCS_CLASS_ID, true))
			PlayerDream();

		//select dream based on area
		ieResRef *movie;
		if (dream==0 || dream>7) {
			movie = GetDream(area);
		} else {
			movie = restmovies+dream;
		}
		if (*movie[0]!='*') {
			core->PlayMovie(*movie);
		}
	}

	//set partyrested flags
	PartyRested();
	area->PartyRested();
	core->SetEventFlag(EF_ACTION);

	//restindex will be -1 in the case of PST
	//FIXME: I don't quite see why we can't sumply use the same strings.2da entry
	//It seems we could reduce complexity here, and free up 2-3 string slots too
	int restindex = displaymsg->GetStringReference(STR_REST);
	int strindex;
	char* tmpstr = NULL;

	core->GetTokenDictionary()->SetAtCopy("HOUR", hours);
	if (restindex != -1) {
		strindex = displaymsg->GetStringReference(STR_HOURS);
	} else {
		strindex = displaymsg->GetStringReference(STR_PST_HOURS);
		restindex = displaymsg->GetStringReference(STR_PST_REST);
	}

	//this would be bad
	if (strindex == -1 || restindex == -1) return;
	tmpstr = core->GetString(strindex, 0);
	//as would this
	if (!tmpstr) return;

	core->GetTokenDictionary()->SetAtCopy("DURATION", tmpstr);
	core->FreeString(tmpstr);
	displaymsg->DisplayString(restindex, DMC_WHITE, 0);
}

//timestop effect
void Game::TimeStop(Actor* owner, ieDword end)
{
	timestop_owner=owner;
	timestop_end=end;
}

// check if the passed actor is a victim of timestop
bool Game::TimeStoppedFor(const Actor* target)
{
	if (!timestop_owner) {
		return false;
	}
	if (target == timestop_owner || target->GetStat(IE_DISABLETIMESTOP)) {
		return false;
	}
	return true;
}

//recalculate the party's infravision state
void Game::Infravision()
{
	hasInfra = false;
	Map *map = GetCurrentArea();
	if (!map) return;
	for(size_t i=0;i<PCs.size();i++) {
		Actor *actor = PCs[i];
		if (!IsAlive(actor)) continue;
		if (actor->GetCurrentArea()!=map) continue;
		//Group infravision overrides this???
		if (!actor->Selected) continue;
		if (actor->GetStat(IE_STATE_ID) & STATE_INFRA) {
			hasInfra = true;
			return;
		}
	}
}

//returns the colour which should be applied onto the whole game area viewport
//this is based on timestop, dream area, weather, daytime

static const Color DreamTint={0xf0,0xe0,0xd0,0x10};    //light brown scale
static const Color NightTint={0x80,0x80,0xe0,0x40};    //dark, bluish
static const Color DuskTint={0xe0,0x80,0x80,0x40};     //dark, reddish
static const Color FogTint={0xff,0xff,0xff,0x40};      //whitish
static const Color DarkTint={0x80,0x80,0xe0,0x10};     //slightly dark bluish

const Color *Game::GetGlobalTint() const
{
	Map *map = GetCurrentArea();
	if (!map) return NULL;
	if (map->AreaFlags&AF_DREAM) {
		return &DreamTint;
	}
	if ((map->AreaType&(AT_OUTDOOR|AT_DAYNIGHT|AT_EXTENDED_NIGHT)) == (AT_OUTDOOR|AT_DAYNIGHT) ) {
		//get daytime colour
		ieDword daynight = ((GameTime/AI_UPDATE_TIME)%7200/300);
		if (daynight<2 || daynight>22) {
			return &NightTint;
		}
		if (daynight>20 || daynight<4) {
			return &DuskTint;
		}
	}
	if ((map->AreaType&(AT_OUTDOOR|AT_WEATHER)) == (AT_OUTDOOR|AT_WEATHER)) {
		//get weather tint
		if (WeatherBits&WB_RAIN) {
			return &DarkTint;
		}
		if (WeatherBits&WB_FOG) {
			return &FogTint;
		}
	}

	return NULL;
}

bool Game::IsDay()
{
	ieDword daynight = ((GameTime/AI_UPDATE_TIME)%7200/300);
	if(daynight<4 || daynight>20) {
		return false;
	}
	return true;
}

void Game::ChangeSong(bool always, bool force)
{
	int Song;

	if (CombatCounter) {
		//battlesong
		Song = SONG_BATTLE;
	} else {
		//will select SONG_DAY or SONG_NIGHT
		Song = (GameTime/AI_UPDATE_TIME)%7200/3600;
	}
	//area may override the song played (stick in battlemusic)
	//always transition gracefully with ChangeSong
	//force just means, we schedule the song for later, if currently
	//is playing
	area->PlayAreaSong( Song, always, force );
}

/* this method redraws weather. If update is false,
   then the weather particles won't change (game paused)
*/
void Game::DrawWeather(const Region &screen, bool update)
{
	if (!weather) {
		return;
	}
	if (!area->HasWeather()) {
		return;
	}

	weather->Draw( screen );
	if (!update) {
		return;
	}

	if (!(WeatherBits & (WB_RAIN|WB_SNOW)) ) {
		if (weather->GetPhase() == P_GROW) {
			weather->SetPhase(P_FADE);
		}
	}
	//if (GameTime&1) {
		int drawn = weather->Update();
		if (drawn) {
			WeatherBits &= ~WB_START;
		}
	//}

	if (WeatherBits&WB_HASWEATHER) {
		return;
	}
	StartRainOrSnow(true, area->GetWeather());
}

/* sets the weather type */
void Game::StartRainOrSnow(bool conditional, int w)
{
	if (conditional && (w & (WB_RAIN|WB_SNOW)) ) {
		if (WeatherBits & (WB_RAIN | WB_SNOW) )
			return;
	}
	// whatever was responsible for calling this, we now have some set weather
	WeatherBits = w | WB_HASWEATHER;
	if (w & WB_LIGHTNING) {
		if (WeatherBits&WB_START) {
			//already raining
			if (GameTime&1) {
				core->PlaySound(DS_LIGHTNING1);
			} else {
				core->PlaySound(DS_LIGHTNING2);
			}
		} else {
			//start raining (far)
			core->PlaySound(DS_LIGHTNING3);
		}
	}
	if (w&WB_SNOW) {
		core->PlaySound(DS_SNOW);
		weather->SetType(SP_TYPE_POINT, SP_PATH_FLIT, SP_SPAWN_SOME);
		weather->SetPhase(P_GROW);
		weather->SetColor(SPARK_COLOR_WHITE);
		return;
	}
	if (w&WB_RAIN) {
		core->PlaySound(DS_RAIN);
		weather->SetType(SP_TYPE_LINE, SP_PATH_RAIN, SP_SPAWN_SOME);
		weather->SetPhase(P_GROW);
		weather->SetColor(SPARK_COLOR_STONE);
		return;
	}
	weather->SetPhase(P_FADE);
}

void Game::SetExpansion(ieDword value)
{
	if (value) {
		if (Expansion>=value) {
			return;
		}
		Expansion = value;
	}

	core->SetEventFlag(EF_EXPANSION);
	switch(value) {
	default:
		break;
	//TODO: move this hardcoded hack to the scripts
	case 0:
		core->GetDictionary()->SetAt( "PlayMode", 2 );

		int i = GetPartySize(false);
		while(i--) {
			Actor *actor = GetPC(i, false);
			InitActorPos(actor);
		}
	}
}

void Game::DebugDump()
{
	size_t idx;

	print("Currently loaded areas:\n");
	for(idx=0;idx<Maps.size();idx++) {
		Map *map = Maps[idx];

		print("%s\n",map->GetScriptName());
	}
	print("Current area: %s   Previous area: %s\n", CurrentArea, PreviousArea);
	print("Global script: %s\n", Scripts[0]->GetName());
	print("CombatCounter: %d\n", (int) CombatCounter);

	print("Party size: %d\n", (int) PCs.size());
	for(idx=0;idx<PCs.size();idx++) {
		Actor *actor = PCs[idx];

		print("Name: %s Order %d %s\n",actor->ShortName, actor->InParty, actor->Selected?"x":"-");
	}
}

Actor *Game::GetActorByGlobalID(ieDword globalID)
{
	size_t mc = GetLoadedMapCount();
	while(mc--) {
		Map *map = GetMap(mc);
		Actor *actor = map->GetActorByGlobalID(globalID);
		if (actor) return actor;
	}
	return GetGlobalActorByGlobalID(globalID);
}

ieByte *Game::AllocateMazeData()
{
	if (mazedata) {
		free(mazedata);
	}
	mazedata = (ieByte*)malloc(MAZE_DATA_SIZE);
	return mazedata;
}

bool Game::IsTimestopActive() const
{
	return timestop_end > GameTime;
}

bool Game::RandomEncounter(ieResRef &BaseArea)
{
	displaymsg->DisplayConstantString(STR_AMBUSH, DMC_BG2XPGREEN);

	if (bntrows<0) {
		AutoTable table;

	        if (table.load("bntychnc")) {
			bntrows = table->GetRowCount();
			bntchnc = (int *) calloc(sizeof(int),bntrows);
			for(int i = 0; i<bntrows; i++) {
				bntchnc[i] = atoi(table->QueryField(i, 0));
			}
		} else {
			bntrows = 0;
		}
	}

	int rep = Reputation/10;
	if (rep>=bntrows) return false;
	if (core->Roll(1, 100, 0)>bntchnc[rep]) return false;
	//TODO: unhardcode this
	memcpy(BaseArea+4,"10",3);
	return gamedata->Exists(BaseArea, IE_ARE_CLASS_ID);
}
