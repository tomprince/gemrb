/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003-2005 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "GameScript/GSUtils.h"
#include "GameScript/Matching.h"

#include "strrefs.h"
#include "defsounds.h"

#include "Audio.h"
#include "DialogHandler.h"
#include "DisplayMessage.h"
#include "Game.h"
#include "GameData.h"
#include "Interface.h"
#include "Item.h"
#include "Map.h"
#include "Spell.h"
#include "StringMgr.h"
#include "TileMap.h"
#include "Video.h"
#include "GUI/GameControl.h"
#include "Scriptable/Container.h"
#include "Scriptable/Door.h"
#include "Scriptable/InfoPoint.h"

#include <cstdio>

//these tables will get freed by Core
Holder<SymbolMgr> triggersTable;
Holder<SymbolMgr> actionsTable;
Holder<SymbolMgr> overrideActionsTable;
Holder<SymbolMgr> objectsTable;
TriggerFunction triggers[MAX_TRIGGERS];
ActionFunction actions[MAX_ACTIONS];
short actionflags[MAX_ACTIONS];
short triggerflags[MAX_TRIGGERS];
ObjectFunction objects[MAX_OBJECTS];
IDSFunction idtargets[MAX_OBJECT_FIELDS];
Cache SrcCache; //cache for string resources (pst)
Cache BcsCache; //cache for scripts
int ObjectIDSCount = 7;
int MaxObjectNesting = 5;
bool HasAdditionalRect = false;
bool HasTriggerPoint = false;
//don't create new variables
bool NoCreate = false;
bool HasKaputz = false;
//released by ReleaseMemory
ieResRef *ObjectIDSTableNames;
int ObjectFieldsCount = 7;
int ExtraParametersCount = 0;
int InDebug = 0;
int happiness[3][20];
int RandomNumValue;
// reaction modifiers (by reputation and charisma)
int rmodrep[20];
int rmodchr[25];
Gem_Polygon **polygons;

void InitScriptTables()
{
	//initializing the happiness table
	{
	AutoTable tab("happy");
	if (tab) {
		for (int alignment=0;alignment<3;alignment++) {
			for (int reputation=0;reputation<20;reputation++) {
				happiness[alignment][reputation]=strtol(tab->QueryField(reputation,alignment), NULL, 0);
			}
		}
	}
	}

	//initializing the reaction mod. reputation table
	AutoTable rmr("rmodrep");
	if (rmr) {
		for (int reputation=0; reputation<20; reputation++) {
			rmodrep[reputation] = strtol(rmr->QueryField(0, reputation), NULL, 0);
		}
	}

	//initializing the reaction mod. charisma table
	AutoTable rmc("rmodchr");
	if (rmc) {
		for (int charisma=0; charisma<25; charisma++) {
			rmodchr[charisma] = strtol(rmc->QueryField(0, charisma), NULL, 0);
		}
	}
}

int GetReaction(Actor *target, Scriptable *Sender)
{
	int chr, rep, reaction;
	chr = target->GetStat(IE_CHR)-1;
	if (target->GetStat(IE_EA) == EA_PC) {
		rep = core->GetGame()->Reputation/10;
	} else {
		rep = target->GetStat(IE_REPUTATION);
	}
	reaction = 10 + rmodrep[rep] + rmodchr[chr];

	// add -4 penalty when dealing with racial enemies
	if (Sender && target->GetRangerLevel() && Sender->Type == ST_ACTOR && target->IsRacialEnemy((Actor *)Sender)) {
		reaction -= 4;
	}

	return reaction;
}

int GetHappiness(Scriptable* Sender, int reputation)
{
	if (Sender->Type != ST_ACTOR) {
		return 0;
	}
	Actor* ab = ( Actor* ) Sender;
	int alignment = ab->GetStat(IE_ALIGNMENT)&AL_GE_MASK; //good / evil
	if (reputation > 200) {
		reputation = 200;
	}
	return happiness[alignment][reputation/10-1];
}

int GetHPPercent(Scriptable* Sender)
{
	if (Sender->Type != ST_ACTOR) {
		return 0;
	}
	Actor* ab = ( Actor* ) Sender;
	int hp1 = ab->GetStat(IE_MAXHITPOINTS);
	if (hp1<1) {
		return 0;
	}
	int hp2 = ab->GetBase(IE_HITPOINTS);
	if (hp2<1) {
		return 0;
	}
	return hp2*100/hp1;
}

void HandleBitMod(ieDword &value1, ieDword value2, int opcode)
{
	switch(opcode) {
		case BM_AND:
			value1 = ( value1& value2 );
			break;
		case BM_OR:
			value1 = ( value1| value2 );
			break;
		case BM_XOR:
			value1 = ( value1^ value2 );
			break;
		case BM_NAND: //this is a GemRB extension
			value1 = ( value1& ~value2 );
			break;
		case BM_SET: //this is a GemRB extension
			value1 = value2;
			break;
	}
}

// SPIT is not in the original engine spec, it is reserved for the
// enchantable items feature
//					0      1       2     3      4
static const char *spell_suffices[]={"SPIT","SPPR","SPWI","SPIN","SPCL"};

//this function handles the polymorphism of Spell[RES] actions
//it returns spellres
bool ResolveSpellName(ieResRef spellres, Action *parameters)
{
	if (parameters->string0Parameter[0]) {
		strnlwrcpy(spellres, parameters->string0Parameter, 8);
	} else {
		//resolve spell
		int type = parameters->int0Parameter/1000;
		int spellid = parameters->int0Parameter%1000;
		if (type>4) {
			return false;
		}
		sprintf(spellres, "%s%03d", spell_suffices[type], spellid);
	}
	return gamedata->Exists(spellres, IE_SPL_CLASS_ID);
}

void ResolveSpellName(ieResRef spellres, ieDword number)
{
	//resolve spell
	unsigned int type = number/1000;
	int spellid = number%1000;
	if (type>4) {
		type=0;
	}
	sprintf(spellres, "%s%03d", spell_suffices[type], spellid);
}

ieDword ResolveSpellNumber(const ieResRef spellres)
{
	int i;

	for(i=0;i<5;i++) {
		if(!strnicmp(spellres, spell_suffices[i], 4)) {
			int n = -1;
			sscanf(spellres+4,"%d", &n);
			if (n<0) {
				return 0xffffffff;
			}
			return i*1000+n;
		}
	}
	return 0xffffffff;
}

bool ResolveItemName(ieResRef itemres, Actor *act, ieDword Slot)
{
	CREItem *itm = act->inventory.GetSlotItem(Slot);
	if(itm) {
		strnlwrcpy(itemres, itm->ItemResRef, 8);
		return gamedata->Exists(itemres, IE_ITM_CLASS_ID);
	}
	return false;
}

bool StoreHasItemCore(const ieResRef storename, const ieResRef itemname)
{
	bool had_nostore=false;
	bool has_current=false;
	ieResRef current;
	ieDword owner = 0;
	CREItem item;

	Store *store = core->GetCurrentStore();
	if (!store) {
		had_nostore = true;
		store = core->SetCurrentStore(storename, 0);
	} else {
		if (strnicmp(store->Name, storename, 8) ) {
			//not the current store, we need some dirty hack
			has_current = true;
			strnlwrcpy(current, store->Name, 8);
			owner = store->GetOwnerID();
		}
	}
	if (!store) {
		printMessage("GameScript","Store cannot be opened!\n", LIGHT_RED);
		return false;
	}

	bool ret = false;
	//don't use triggers (pst style), it would be possible to create infinite loops
	if (store->FindItem(itemname, false) != (unsigned int)-1) {
		ret=true;
	}
	if (has_current) {
		//setting back old store (this will save our current store)
		core->SetCurrentStore(current, owner);
	} else if (had_nostore) {
		core->CloseCurrentStore();
	}
	return ret;
}

//don't pass this point by reference, it is subject to change
void ClickCore(Scriptable *Sender, Point point, int type, int speed)
{
	Map *map = Sender->GetCurrentArea();
	if (!map) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Point p=map->TMap->GetMapSize();
	if (!p.PointInside(point)) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Video *video = core->GetVideoDriver();
	GlobalTimer *timer = core->timer;
	timer->SetMoveViewPort( point.x, point.y, speed, true );
	timer->DoStep(0);
	if (timer->ViewportIsMoving()) {
		Sender->AddActionInFront( Sender->GetCurrentAction() );
		Sender->SetWait(1);
		Sender->ReleaseCurrentAction();
		return;
	}

	video->ConvertToScreen(point.x, point.y);
	GameControl *win = core->GetGameControl();

	point.x+=win->XPos;
	point.y+=win->YPos;
	video->MoveMouse(point.x, point.y);
	video->ClickMouse(type);
	Sender->ReleaseCurrentAction();
}

void TransformItemCore(Actor *actor, Action *parameters, bool onlyone)
{
	int i = actor->inventory.GetSlotCount();
	while(i--) {
		CREItem *item = actor->inventory.GetSlotItem(i);
		if (!item) {
			continue;
		}
		if (strnicmp(item->ItemResRef, parameters->string0Parameter, 8) ) {
			continue;
		}
		actor->inventory.SetSlotItemRes(parameters->string1Parameter,i,parameters->int0Parameter,parameters->int1Parameter,parameters->int2Parameter);
		if (onlyone) {
			break;
		}
	}
}

//check if an inventory (container or actor) has item (could be recursive ?)
bool HasItemCore(Inventory *inventory, const ieResRef itemname, ieDword flags)
{
	if (inventory->HasItem(itemname, flags)) {
		return true;
	}
	int i=inventory->GetSlotCount();
	while (i--) {
		//maybe we could speed this up if we mark bag items with a flags bit
		CREItem *itemslot = inventory->GetSlotItem(i);
		if (!itemslot)
			continue;
		Item *item = gamedata->GetItem(itemslot->ItemResRef);
		if (!item)
			continue;
		bool ret = false;
		if (core->CanUseItemType(SLOT_BAG,item,NULL) ) {
			//the store is the same as the item's name
			ret = StoreHasItemCore(itemslot->ItemResRef, itemname);
		}
		gamedata->FreeItem(item, itemslot->ItemResRef);
		if (ret) {
			return true;
		}
	}
	return false;
}

void DisplayStringCore(Scriptable* Sender, int Strref, int flags)
{
	StringBlock sb;
	char Sound[_MAX_PATH];

	//no one hears you when you are in the Limbo!
	if (!Sender->GetCurrentArea()) {
		return;
	}

	memset(&sb,0,sizeof(sb));
	Sound[0]=0;
	printf( "Displaying string on: %s\n", Sender->GetScriptName() );
	if (flags & DS_CONST) {
		if (Sender->Type!=ST_ACTOR) {
			printMessage("GameScript","Verbal constant not supported for non actors!\n", LIGHT_RED);
			return;
		}
		Actor* actor = ( Actor* ) Sender;
		if ((ieDword) Strref>=VCONST_COUNT) {
			printMessage("GameScript","Invalid verbal constant!\n", LIGHT_RED);
			return;
		}

		int tmp=(int) actor->GetVerbalConstant(Strref);
		if (tmp <= 0 || (actor->GetStat(IE_MC_FLAGS) & MC_EXPORTABLE)) {
			//get soundset based string constant
			actor->ResolveStringConstant( sb.Sound, (unsigned int) Strref);
			if (actor->PCStats && actor->PCStats->SoundFolder[0]) {
				snprintf(Sound, _MAX_PATH, "%s/%s",
					actor->PCStats->SoundFolder, sb.Sound);
			} else {
				memcpy(Sound, sb.Sound, sizeof(ieResRef) );
			}
		}
		Strref = tmp;

		//display the verbal constants in the console
		ieDword charactersubtitles = 0;
		core->GetDictionary()->Lookup("Subtitles", charactersubtitles);
		if (charactersubtitles) {
			flags |= DS_CONSOLE;
		}
	}

	if ((Strref != -1) && !sb.Sound[0]) {
		sb = core->strings->GetStringBlock( Strref );
		memcpy(Sound, sb.Sound, sizeof(ieResRef) );
		if (sb.text[0] && strcmp(sb.text," ") && (flags & DS_CONSOLE)) {
			//can't play the sound here, we have to delay action
			//and for that, we have to know how long the text takes
			if(flags&DS_NONAME) {
				displaymsg->DisplayString( sb.text );
			} else {
				displaymsg->DisplayStringName( Strref, 0xf0f0f0, Sender, 0);
			}
		}
		if (sb.text[0] && strcmp(sb.text," ") && (flags & (DS_HEAD | DS_AREA))) {
			Sender->DisplayHeadText( sb.text );
			//don't free sb.text, it is residing in Sender
			if (flags & DS_AREA) {
				Sender->FixHeadTextPos();
			}
		} else {
			core->FreeString( sb.text );
		}
	}
	if (Sound[0] && !(flags&DS_SILENT) ) {
		ieDword speech = GEM_SND_RELATIVE; //disable position
		if (flags&DS_SPEECH) speech|=GEM_SND_SPEECH;
		unsigned int len = 0;
		core->GetAudioDrv()->Play( Sound,0,0,speech,&len );
		ieDword counter = ( AI_UPDATE_TIME * len ) / 1000;
		if ((counter != 0) && (flags &DS_WAIT) )
			Sender->SetWait( counter );
	}
}

int CanSee(Scriptable* Sender, Scriptable* target, bool range, int seeflag)
{
	Map *map;

	if (target->Type==ST_ACTOR) {
		Actor *tar = (Actor *) target;

		if (!tar->ValidTarget(seeflag)) {
			return 0;
		}
	}

	map = target->GetCurrentArea();
	//if (!(seeflag&GA_GLOBAL)) {
		if (!map || map!=Sender->GetCurrentArea() ) {
			return 0;
		}
	//}

	if (range) {
		unsigned int dist;

		if (Sender->Type == ST_ACTOR) {
			Actor* snd = ( Actor* ) Sender;
			dist = snd->Modified[IE_VISUALRANGE];
		} else {
			dist = 30;
		}

		if (Distance(target->Pos, Sender->Pos) > dist * 15) {
			return 0;
		}
	}

	return map->IsVisible(target->Pos, Sender->Pos);
}

//non actors can see too (reducing function to LOS)
//non actors can be seen too (reducing function to LOS)
int SeeCore(Scriptable* Sender, Trigger* parameters, int justlos)
{
	//see dead
	int flags;

	if (parameters->int0Parameter) {
		flags = GA_DETECT;
	} else {
		flags = GA_NO_DEAD;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter, flags );
	/* don't set LastSeen if this isn't an actor */
	if (!tar) {
		return 0;
	}
	//both are actors
	if (CanSee(Sender, tar, true, flags) ) {
		if (justlos) {
			return 1;
		}
		if (Sender->Type==ST_ACTOR && tar->Type==ST_ACTOR) {
			Actor* snd = ( Actor* ) Sender;
			//additional checks for invisibility?
			snd->LastSeen = tar->GetGlobalID();
		}
		return 1;
	}
	return 0;
}

//transfering item from Sender to target
//if target has no inventory, the item will be destructed
//if target can't get it, it will be dropped at its feet
int MoveItemCore(Scriptable *Sender, Scriptable *target, const char *resref, int flags, int setflag)
{
	Inventory *myinv;
	Map *map;
	// track whether we are dealing with our party and need to display feedback
	bool lostitem = false;
	bool gotitem = false;

	if (!target) {
		return MIC_INVALID;
	}
	map=Sender->GetCurrentArea();
	switch(Sender->Type) {
		case ST_ACTOR:
			myinv=&((Actor *) Sender)->inventory;
			if (((Actor *)Sender)->InParty) lostitem = true;
			break;
		case ST_CONTAINER:
			myinv=&((Container *) Sender)->inventory;
			break;
		default:
			return MIC_INVALID;
	}
	CREItem *item;
	myinv->RemoveItem(resref, flags, &item);
	if (!item) {
		// nothing was removed
		return MIC_NOITEM;
	}

	item->Flags|=setflag;

	switch(target->Type) {
		case ST_ACTOR:
			myinv=&((Actor *) target)->inventory;
			if (((Actor *) target)->InParty) gotitem = true;
			break;
		case ST_CONTAINER:
			myinv=&((Container *) target)->inventory;
			break;
		default:
			myinv = NULL;
			break;
	}
	if (!myinv) {
		delete item;
		if (lostitem) displaymsg->DisplayConstantString(STR_LOSTITEM, 0xbcefbc);
		return MIC_GOTITEM; // actually it was lost, not gained
	}
	if ( myinv->AddSlotItem(item, SLOT_ONLYINVENTORY) !=ASI_SUCCESS) {
		// drop it at my feet
		map->AddItemToLocation(target->Pos, item);
		if (gotitem) displaymsg->DisplayConstantString(STR_INVFULL_ITEMDROP, 0xbcefbc);
		return MIC_FULL;
	}
	if (gotitem) displaymsg->DisplayConstantString(STR_GOTITEM, 0xbcefbc);
	return MIC_GOTITEM;
}

/*FIXME: what is 'base'*/
void PolymorphCopyCore(Actor *src, Actor *tar, bool base)
{
	tar->SetBase(IE_ANIMATION_ID, src->GetStat(IE_ANIMATION_ID) );
	if (!base) {
		tar->SetBase(IE_ARMOR_TYPE, src->GetStat(IE_ARMOR_TYPE) );
		for (int i=0;i<7;i++) {
			tar->SetBase(IE_COLORS+i, src->GetStat(IE_COLORS+i) );
		}
	}
	tar->SetName(src->GetName(0),0);
	tar->SetName(src->GetName(1),1);
	//add more attribute copying
}

void CreateCreatureCore(Scriptable* Sender, Action* parameters, int flags)
{
	Scriptable *tmp = GetActorFromObject( Sender, parameters->objects[1] );
	//if there is nothing to copy, don't spawn anything
	if (flags & CC_COPY) {
		if (!tmp || tmp->Type != ST_ACTOR) {
			return;
		}
	}

	Actor* ab;
	if (flags & CC_STRING1) {
		ab = gamedata->GetCreature(parameters->string1Parameter);
	}
	else {
		ab = gamedata->GetCreature(parameters->string0Parameter);
	}

	if (!ab) {
		printMessage("GameScript","Failed to create creature! ",LIGHT_RED);
		printf("(missing creature file %s?)\n", parameters->string0Parameter);
		// maybe this should abort()?
		return;
	}

	//iwd2 allows an optional scriptname to be set
	//but bg2 doesn't have this feature
	//this way it works for both games
	if ((flags & CC_SCRIPTNAME) && parameters->string1Parameter[0]) {
		ab->SetScriptName(parameters->string1Parameter);
	}

	int radius;
	Point pnt;

	radius=0;
	switch (flags & CC_MASK) {
		//creates creature just off the screen
		case CC_OFFSCREEN:
			{
			Region vp = core->GetVideoDriver()->GetViewport();
			radius=vp.w/2; //actually it must be further divided by the tile size, hmm 16?
			}
			//falling through
		case CC_OBJECT://use object + offset
			if (tmp) Sender=tmp;
			//falling through
		case CC_OFFSET://use sender + offset
			pnt.x = parameters->pointParameter.x+Sender->Pos.x;
			pnt.y = parameters->pointParameter.y+Sender->Pos.y;
			break;
		default: //absolute point, but -1,-1 means AtFeet
			pnt.x = parameters->pointParameter.x;
			pnt.y = parameters->pointParameter.y;
			if (pnt.isempty()) {
				pnt.x = Sender->Pos.x;
				pnt.y = Sender->Pos.y;
			}
			break;
	}

	Map *map = Sender->GetCurrentArea();
	map->AddActor( ab );
	ab->SetPosition( pnt, flags&CC_CHECK_IMPASSABLE, radius );
	ab->SetOrientation(parameters->int0Parameter, false );

	//if string1 is animation, then we can't use it for a DV too
	if (flags & CC_PLAY_ANIM) {
		CreateVisualEffectCore( ab, ab->Pos, parameters->string1Parameter, 1);
	} else {
		//setting the deathvariable if it exists (iwd2)
		if (parameters->string1Parameter[0]) {
			ab->SetScriptName(parameters->string1Parameter);
		}
	}

	if (flags & CC_COPY) {
		PolymorphCopyCore ( (Actor *) tmp, ab, false);
	}
}

static ScriptedAnimation *GetVVCEffect(const char *effect, int iterations)
{
	if (effect[0]) {
		ScriptedAnimation* vvc = gamedata->GetScriptedAnimation(effect, false);
		if (!vvc) {
			printMessage("GameScript","Failed to create effect.",LIGHT_RED);
			return NULL;
		}
		if (iterations) {
			vvc->SetDefaultDuration( vvc->GetSequenceDuration(AI_UPDATE_TIME * iterations));
		} else {
			vvc->PlayOnce();
		}
		return vvc;
	}
	return NULL;
}

void CreateVisualEffectCore(Actor *target, const char *effect, int iterations)
{
	ScriptedAnimation *vvc = GetVVCEffect(effect, iterations);
	if (vvc) {
		target->AddVVCell( vvc );
	}
}

void CreateVisualEffectCore(Scriptable *Sender, const Point &position, const char *effect, int iterations)
{
	ScriptedAnimation *vvc = GetVVCEffect(effect, iterations);
	if (vvc) {
		vvc->XPos +=position.x;
		vvc->YPos +=position.y;
		Sender->GetCurrentArea( )->AddVVCell( vvc );
	}
}

//this destroys the current actor and replaces it with another
void ChangeAnimationCore(Actor *src, const char *resref, bool effect)
{
	Actor *tar = gamedata->GetCreature(resref);
	if (tar) {
		Map *map = src->GetCurrentArea();
		map->AddActor( tar );
		Point pos = src->Pos;
		tar->SetOrientation(src->GetOrientation(), false );
		// make sure to copy the HP, to avoid things like magically-healing trolls
		tar->BaseStats[IE_HITPOINTS]=src->BaseStats[IE_HITPOINTS];
		src->DestroySelf();
		// can't SetPosition while the old actor is taking the spot
		tar->SetPosition(pos, 1);
		if (effect) {
			CreateVisualEffectCore(tar, tar->Pos,"smokepuffeffect",1);
		}
	}
}

void EscapeAreaCore(Scriptable* Sender, const Point &p, const char* area, const Point &enter, int flags, int wait)
{
	char Tmp[256];

	if ( !p.isempty() && PersonalDistance(p, Sender)>MAX_OPERATING_DISTANCE) {
		//MoveNearerTo will return 0, if the actor is in move
		//it will return 1 (the fourth parameter) if the target is unreachable
		if (!MoveNearerTo(Sender, p, MAX_OPERATING_DISTANCE,1) ) {
			if (!Sender->InMove()) printf("At least it said so...\n");
			return;
		}
	}

	if (flags &EA_DESTROY) {
		//this must be put into a non-const variable
		sprintf( Tmp, "DestroySelf()" );
	} else {
		// last parameter is 'face', which should be passed from relevant action parameter..
		sprintf( Tmp, "MoveBetweenAreas(\"%s\",[%hd.%hd],%d)", area, enter.x, enter.y, 0 );
	}
	printMessage("GSUtils"," ", WHITE);
	printf("Executing %s in EscapeAreaCore\n", Tmp);
	//drop this action, but add another (destroyself or movebetweenareas)
	//between the arrival and the final escape, there should be a wait time
	//that wait time could be handled here
	if (wait) {
		printf("But wait a bit... (%d)\n", wait);
		Sender->SetWait(wait);
	}
	Sender->ReleaseCurrentAction();
	Action * action = GenerateAction( Tmp);
	Sender->AddActionInFront( action );
}

void GetTalkPositionFromScriptable(Scriptable* scr, Point &position)
{
	switch (scr->Type) {
		case ST_AREA: case ST_GLOBAL:
			position = scr->Pos; //fake
			break;
		case ST_ACTOR:
			//if there are other moveables, put them here
			position = ((Movable *) scr)->GetMostLikelyPosition();
			break;
		case ST_TRIGGER: case ST_PROXIMITY: case ST_TRAVEL:
			if (((InfoPoint *) scr)->Flags & TRAP_USEPOINT) {
				position=((InfoPoint *) scr)->UsePoint;
				break;
			}
			position=((InfoPoint *) scr)->TrapLaunch;
			break;
		case ST_DOOR: case ST_CONTAINER:
			position=((Highlightable *) scr)->TrapLaunch;
			break;
	}
}

void GetPositionFromScriptable(Scriptable* scr, Point &position, bool dest)
{
	if (!dest) {
		position = scr->Pos;
		return;
	}
	switch (scr->Type) {
		case ST_AREA: case ST_GLOBAL:
			position = scr->Pos; //fake
			break;
		case ST_ACTOR:
		//if there are other moveables, put them here
			position = ((Movable *) scr)->GetMostLikelyPosition();
			break;
		case ST_TRIGGER: case ST_PROXIMITY: case ST_TRAVEL:
			if (((InfoPoint *) scr)->Flags & TRAP_USEPOINT) {
				position=((InfoPoint *) scr)->UsePoint;
				break;
			}
		case ST_DOOR: case ST_CONTAINER:
			position=((Highlightable *) scr)->TrapLaunch;
	}
}

int CheckInteract(const char *talker, const char *target)
{
	AutoTable interact("interact");
	if(!interact)
		return 0;
	const char *value = interact->QueryField(talker, target);
	if(!value)
		return 0;
	switch(value[0]) {
		case 's':
			return I_SPECIAL;
		case 'c':
			return I_COMPLIMENT;
		case 'i':
			return I_INSULT;
	}
	return 0;
}

static ieResRef PlayerDialogRes = "PLAYERx\0";

void BeginDialog(Scriptable* Sender, Action* parameters, int Flags)
{
	Scriptable* tar, *scr;
	int seeflag = GA_NO_DEAD;

	if (InDebug&ID_VARIABLES) {
		printf("BeginDialog core\n");
	}
	if (Flags & BD_OWN) {
		tar = GetStoredActorFromObject( Sender, parameters->objects[1], seeflag);
		scr = tar;
	} else {
		tar = GetStoredActorFromObject( Sender, parameters->objects[1], seeflag);
		scr = Sender;
	}
	if (!scr) {
		printMessage("GameScript"," ",LIGHT_RED);
		printf("Speaker for dialog couldn't be found (Sender: %s, Type: %d) Flags:%d.\n", Sender->GetScriptName(), Sender->Type, Flags);
		Sender->ReleaseCurrentAction();
		return;
	}

	if (!tar || tar->Type!=ST_ACTOR) {
		printMessage("GameScript"," ",LIGHT_RED);
		printf("Target for dialog couldn't be found (Sender: %s, Type: %d).\n", Sender->GetScriptName(), Sender->Type);
		if (Sender->Type == ST_ACTOR) {
			((Actor *) Sender)->DebugDump();
		}
		printf ("Target object: ");
		if (parameters->objects[1]) {
			parameters->objects[1]->Dump();
		} else {
			printf("<NULL>\n");
		}
		Sender->ReleaseCurrentAction();
		return;
	}

	Actor *speaker, *target;

	speaker = NULL;
	target = (Actor *) tar;
	if ((Flags & BD_CHECKDIST) && !CanSee(scr, target, false, seeflag) ) {
		printMessage("GameScript"," ",LIGHT_RED);
		printf("CanSee returned false! Speaker (%s, type %d) and target are:\n", scr->GetScriptName(), scr->Type);
		if (scr->Type == ST_ACTOR) {
			((Actor *) scr)->DebugDump();
		}
		((Actor *) tar)->DebugDump();
		Sender->ReleaseCurrentAction();
		return;
	}
	bool swap = false;
	if (scr->Type==ST_ACTOR) {
		speaker = (Actor *) scr;
		if (speaker->GetStat(IE_STATE_ID)&STATE_DEAD) {
			printMessage("GameScript"," ",LIGHT_RED);
			printf("Speaker is dead, cannot start dialogue. Speaker and target are:\n");
			speaker->DebugDump();
			target->DebugDump();
			Sender->ReleaseCurrentAction();
			return;
		}
		//DialogueRange is set in IWD
		ieDword range = MAX_OPERATING_DISTANCE + speaker->GetBase(IE_DIALOGRANGE);
		//making sure speaker is the protagonist, player, actor
		if ( target->InParty == 1) swap = true;
		else if ( speaker->InParty !=1 && target->InParty) swap = true;
		//CHECKDIST works only for mobile scriptables
		if (Flags&BD_CHECKDIST) {
			if ( scr->GetCurrentArea()!=target->GetCurrentArea() ||
				PersonalDistance(scr, target)>range) {
				MoveNearerTo(Sender, target, MAX_OPERATING_DISTANCE);
				return;
			}
		}
	} else {
		//pst style dialog with trigger points
		swap=true;
		if (Flags&BD_CHECKDIST) {
			Point TalkPos;

			if (target->InMove()) {
				//waiting for target
				Sender->AddActionInFront( Sender->GetCurrentAction() );
				Sender->ReleaseCurrentAction();
				Sender->SetWait(1);
				return;
			}
			GetTalkPositionFromScriptable(scr, TalkPos);
			if (PersonalDistance(TalkPos, target)>MAX_OPERATING_DISTANCE ) {
				//try to force the target to come closer???
				GoNear(target, TalkPos);
				Sender->AddActionInFront( Sender->GetCurrentAction() );
				Sender->ReleaseCurrentAction();
				Sender->SetWait(1);
				return;
			}
		}
	}

	GameControl* gc = core->GetGameControl();
	if (!gc) {
		printMessage( "GameScript","Dialog cannot be initiated because there is no GameControl.", YELLOW );
		Sender->ReleaseCurrentAction();
		return;
	}
	//can't initiate dialog, because it is already there
	if (gc->GetDialogueFlags()&DF_IN_DIALOG) {
		if (Flags & BD_INTERRUPT) {
			//break the current dialog if possible
			gc->dialoghandler->EndDialog(true);
		}
		//check if we could manage to break it, not all dialogs are breakable!
		if (gc->GetDialogueFlags()&DF_IN_DIALOG) {
			printMessage( "GameScript","Dialog cannot be initiated because there is already one.", YELLOW );
			Sender->ReleaseCurrentAction();
			return;
		}
	}

	// starting a dialog ends cutscenes!
	core->SetCutSceneMode(false);

	const char* Dialog = NULL;
	AutoTable pdtable;

	switch (Flags & BD_LOCMASK) {
		case BD_STRING0:
			Dialog = parameters->string0Parameter;
			if (Flags & BD_SETDIALOG) {
				scr->SetDialog( Dialog );
			}
			break;
		case BD_SOURCE:
		case BD_TARGET:
			if (swap) Dialog = scr->GetDialog();
			else Dialog = target->GetDialog(GD_FEEDBACK);
			break;
		case BD_RESERVED:
			//what if playerdialog was initiated from Player2?
			PlayerDialogRes[5] = '1';
			Dialog = ( const char * ) PlayerDialogRes;
			break;
		case BD_INTERACT: //using the source for the dialog
			const char* scriptingname = scr->GetScriptName();

			/* use interact.2da for short, inlined dialogue */
			int type = CheckInteract(scriptingname, target->GetScriptName());
			if(type) {
				//TODO increase interact counter in scr
				speaker->Interact(type);
				target->Response(type);
				Sender->ReleaseCurrentAction();
				return;
			}
			/* banter dialogue */
			pdtable.load("interdia");
			//Dialog is a borrowed reference, we cannot free pdtable while it is being used
			if (pdtable) {
				Dialog = pdtable->QueryField( scriptingname, "FILE" );
			}
			break;
	}


	//dialog is not meaningful
	if (!Dialog || Dialog[0]=='*') {
		Sender->ReleaseCurrentAction();
		return;
	}

	//maybe we should remove the action queue, but i'm unsure
	//no, we shouldn't even call this!
	//Sender->ReleaseCurrentAction();

	// moved this here from InitDialog, because InitDialog doesn't know which side is which
	// post-swap (and non-actors always have IF_NOINT set) .. also added a check that it's
	// actually busy doing something, for the same reason
	if (target->GetInternalFlag()&IF_NOINT && (target->GetCurrentAction() || target->GetNextAction())) {
		displaymsg->DisplayConstantString(STR_TARGETBUSY,0xff0000);
		Sender->ReleaseCurrentAction();
		return;
	}

	if (speaker!=target) {
		if (swap) {
			Scriptable *tmp = tar;
			tar = scr;
			scr = tmp;
		} else {
			if (!(Flags & BD_INTERRUPT)) {
				// added CurrentAction as part of blocking action fixes
				if (tar->GetCurrentAction() || tar->GetNextAction()) {
					displaymsg->DisplayConstantString(STR_TARGETBUSY,0xff0000);
					Sender->ReleaseCurrentAction();
					return;
				}
			}
		}
	}

	//don't clear target's actions, because a sequence like this will be broken:
	//StartDialog([PC]); SetGlobal("Talked","LOCALS",1);
	if (scr!=tar) {
		if (scr->Type==ST_ACTOR) {
			((Actor *) scr)->SetOrientation(GetOrient( tar->Pos, scr->Pos), true);
		}
		if (tar->Type==ST_ACTOR) {
			((Actor *) tar)->SetOrientation(GetOrient( scr->Pos, tar->Pos), true);
		}
	}

	int ret;

	if (Dialog[0]) {
		//increasing NumTimesTalkedTo or NumTimesInteracted
		if (Flags & BD_TALKCOUNT) {
			gc->SetDialogueFlags(DF_TALKCOUNT, BM_OR);
		} else if ((Flags & BD_LOCMASK) == BD_INTERACT) {
			gc->SetDialogueFlags(DF_INTERACT, BM_OR);
		}

		core->GetDictionary()->SetAt("DialogChoose",(ieDword) -1);
		ret = gc->dialoghandler->InitDialog( scr, tar, Dialog);
	}
	else {
		ret = -1;
	}

	if (ret<0) {
		Sender->ReleaseCurrentAction();
		if (Flags & BD_NOEMPTY) {
			return;
		}
		displaymsg->DisplayConstantStringName(STR_NOTHINGTOSAY,0xff0000,tar);
		return;
	}

	//this is a bit fishy
	Sender->SetWait(1);
	Sender->ReleaseCurrentAction();

}

void MoveBetweenAreasCore(Actor* actor, const char *area, const Point &position, int face, bool adjust)
{
	printMessage("GameScript", " ", WHITE);
	printf("MoveBetweenAreas: %s to %s [%d.%d] face: %d\n", actor->GetName(0), area,position.x,position.y, face);
	Map* map2;
	Game* game = core->GetGame();
	if (area[0]) { //do we need to switch area?
		Map* map1 = actor->GetCurrentArea();
		//we have to change the pathfinder
		//to the target area if adjust==true
		map2 = game->GetMap(area, false);
		if ( map1!=map2 ) {
			if (map1) {
				map1->RemoveActor( actor );
			}
			map2->AddActor( actor );
		}
	}
	actor->SetPosition(position, adjust);
	if (face !=-1) {
		actor->SetOrientation( face, false );
	}
	// should this perhaps be a 'selected' check or similar instead?
	if (actor->InParty) {
		GameControl *gc=core->GetGameControl();
		gc->SetScreenFlags(SF_CENTERONACTOR,BM_OR);
		game->ChangeSong(false, true);
	}
}

//repeat movement, until goal isn't reached
//if int0parameter is !=0, then it will try only x times
void MoveToObjectCore(Scriptable *Sender, Action *parameters, ieDword flags, bool untilsee)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Scriptable* target = GetStoredActorFromObject( Sender, parameters->objects[1] );
	if (!target) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	if (untilsee && CanSee(actor, target, true, 0) ) {
		Sender->ReleaseCurrentAction();
		return;
	} else {
		if (PersonalDistance(actor, target)<MAX_OPERATING_DISTANCE) {
			Sender->ReleaseCurrentAction();
			return;
		}
	}
	if (!actor->InMove() || actor->Destination != target->Pos) {
		actor->WalkTo( target->Pos, flags, 0 );
	}
	//hopefully this hack will prevent lockups
	if (!actor->InMove()) {
		Sender->ReleaseCurrentAction();
		return;
	}

	//repeat movement...
	Action *newaction = ParamCopyNoOverride(parameters);
	if (newaction->int0Parameter!=1) {
		if (newaction->int0Parameter) {
			newaction->int0Parameter--;
		}
		actor->AddActionInFront(newaction);
		actor->SetWait(1);
	}

	Sender->ReleaseCurrentAction();
}

void CreateItemCore(CREItem *item, const char *resref, int a, int b, int c)
{
	//copy the whole resref, including the terminating zero
	strnuprcpy(item->ItemResRef, resref, 8);
	core->ResolveRandomItem(item);
	if (a==-1) {
		//use the default charge counts of the item
		Item *origitem = gamedata->GetItem(item->ItemResRef);
		if (origitem) {
			for(int i=0;i<3;i++) {
				ITMExtHeader *e = origitem->GetExtHeader(i);
				item->Usages[i]=e?e->Charges:0;
			}
			gamedata->FreeItem(origitem, item->ItemResRef, false);
		}
	} else {
		item->Usages[0]=(ieWord) a;
		item->Usages[1]=(ieWord) b;
		item->Usages[2]=(ieWord) c;
	}
	item->Flags=0;
}

//It is possible to attack CONTAINERS/DOORS as well!!!
void AttackCore(Scriptable *Sender, Scriptable *target, int flags)
{
	//this is a dangerous cast, make sure actor is Actor * !!!
	Actor *actor = (Actor *) Sender;

	WeaponInfo wi;
	ITMExtHeader *header = NULL;
	ITMExtHeader *hittingheader = NULL;
	int tohit;
	ieDword Flags;
	int DamageBonus, CriticalBonus;
	int speed, style;

	//bool leftorright = (bool) ((attacksperround-attackcount)&1);
	bool leftorright = false;
	Actor *tar = NULL;
	ieDword targetID = 0;
	if (target->Type==ST_ACTOR) {
		tar = (Actor *) target;
		targetID = tar->GetGlobalID();
	}
	if (actor == tar) {
		Sender->ReleaseCurrentAction();
		return;
	}

	//will return false on any errors (eg, unusable weapon)
	if (!actor->GetCombatDetails(tohit, leftorright, wi, header, hittingheader, Flags, DamageBonus, speed, CriticalBonus, style, tar)) {
		actor->SetStance(IE_ANI_READY);
		Sender->ReleaseCurrentAction();
		return;
	}

	if (header) wi.range *= 10;
	else wi.range = 0;

	if ( target->Type == ST_DOOR || target->Type == ST_CONTAINER) {
		wi.range += 10;
	}
	if (!(flags&AC_NO_SOUND) ) {
		if (actor->LastTarget != targetID) {
			//play attack sound for party members
			if (actor->InParty) {
				//pick from all 5 possible verbal constants
				actor->VerbalConstant(VB_ATTACK, 5);
				//DisplayStringCore(Sender, VB_ATTACK, DS_CONSOLE|DS_CONST );
			}
			//display attack message
			displaymsg->DisplayConstantStringAction(STR_ACTION_ATTACK,0xf0f0f0, Sender, target);
		}
	}
	//action performed
	if(target->Type == ST_ACTOR) {
		actor->SetTarget( target );
	}
	if ( Sender->GetCurrentArea()!=target->GetCurrentArea() ||
		(PersonalDistance(Sender, target) > wi.range) ||
		(!Sender->GetCurrentArea()->IsVisible(Sender->Pos, target->Pos))) {
		MoveNearerTo(Sender, target, wi.range);
		return;
	} else if (target->Type == ST_DOOR) {
		//Forcing a lock does not launch the trap...
		Door* door = (Door*) target;
		if(door->Flags & DOOR_LOCKED) {
			door->TryBashLock(actor);
		}
		Sender->ReleaseCurrentAction();
		return;
	} else if (target->Type == ST_CONTAINER) {
		Container* cont = (Container*) target;
		if(cont->Flags & CONT_LOCKED) {
			cont->TryBashLock(actor);
		}
		Sender->ReleaseCurrentAction();
		return;
	}

	actor->PerformAttack(core->GetGame()->GameTime);
}

//we need this because some special characters like _ or * are also accepted
inline bool ismysymbol(const char letter)
{
	if (letter==']') return false;
	if (letter=='[') return false;
	if (letter==')') return false;
	if (letter=='(') return false;
	if (letter=='.') return false;
	if (letter==',') return false;
	return true;
}

//this function returns a value, symbol could be a numeric string or
//a symbol from idsname
static int GetIdsValue(const char *&symbol, const char *idsname)
{
	int idsfile=core->LoadSymbol(idsname);
	Holder<SymbolMgr> valHook = core->GetSymbol(idsfile);
	if (!valHook) {
		//FIXME:missing ids file!!!
		if (InDebug&ID_TRIGGERS) {
			printMessage("GameScript"," ",LIGHT_RED);
			printf("Missing IDS file %s for symbol %s!\n",idsname, symbol);
		}
		return -1;
	}
	char *newsymbol;
	int value=strtol(symbol, &newsymbol, 0);
	if (symbol!=newsymbol) {
		symbol=newsymbol;
		return value;
	}
	char symbolname[64];
	int x;
	for (x=0;ismysymbol(*symbol) && x<(int) sizeof(symbolname)-1;x++) {
		symbolname[x]=*symbol;
		symbol++;
	}
	symbolname[x]=0;
	return valHook->GetValue(symbolname);
}

static void ParseIdsTarget(const char *&src, Object *&object)
{
	for (int i=0;i<ObjectFieldsCount;i++) {
		object->objectFields[i]=GetIdsValue(src, ObjectIDSTableNames[i]);
		if (*src!='.') {
			break;
		}
		src++;
	}
	src++; //skipping ]
}

//this will skip to the next element in the prototype of an action/trigger
#define SKIP_ARGUMENT() while (*str && ( *str != ',' ) && ( *str != ')' )) str++

static void ParseObject(const char *&str,const char *&src, Object *&object)
{
	SKIP_ARGUMENT();
	object = new Object();
	switch (*src) {
	case '"':
		//Scriptable Name
		src++;
		int i;
		for (i=0;i<(int) sizeof(object->objectName)-1 && *src && *src!='"';i++)
		{
			object->objectName[i] = *src;
			src++;
		}
		object->objectName[i] = 0;
		src++;
		break;
	case '[':
		src++; //skipping [
		ParseIdsTarget(src, object);
		break;
	default: //nested object filters
		int Nesting=0;

		while (Nesting<MaxObjectNesting) {
			memmove(object->objectFilters+1, object->objectFilters, (int) sizeof(int) *(MaxObjectNesting-1) );
			object->objectFilters[0]=GetIdsValue(src,"object");
			if (*src!='(') {
				break;
			}
			src++; //skipping (
			if (*src==')') {
				break;
			}
			Nesting++;
		}
		if (*src=='[') {
			ParseIdsTarget(src, object);
		}
		src+=Nesting; //skipping )
	}
}

/* this function was lifted from GenerateAction, to make it clearer */
Action* GenerateActionCore(const char *src, const char *str, unsigned short actionID)
{
	Action *newAction = new Action(true);
	newAction->actionID = actionID;
	//this flag tells us to merge 2 consecutive strings together to get
	//a variable (context+variablename)
	int mergestrings = actionflags[newAction->actionID]&AF_MERGESTRINGS;
	int objectCount = ( newAction->actionID == 1 ) ? 0 : 1;
	int stringsCount = 0;
	int intCount = 0;
	if (actionflags[newAction->actionID]&AF_DIRECT) {
		Object *tmp = new Object();
		tmp->objectFields[0] = -1;
		//tmp->objectFields[1] = core->GetGameControl()->targetID;
		newAction->objects[objectCount++] = tmp;
	}
	//Here is the Action; Now we need to evaluate the parameters, if any
	if (*str!=')') while (*str) {
		if (*(str+1)!=':') {
			printf("Warning, parser was sidetracked: %s\n",str);
		}
		switch (*str) {
			default:
				printf("Invalid type: %s\n",str);
				//str++;
				delete newAction;
				return NULL;
				break;

			case 'p': //Point
				SKIP_ARGUMENT();
				src++; //Skip [
				newAction->pointParameter.x = (short) strtol( src, (char **) &src, 10 );
				src++; //Skip .
				newAction->pointParameter.y = (short) strtol( src, (char **) &src, 10 );
				src++; //Skip ]
				break;

			case 'i': //Integer
			{
				//going to the variable name
				while (*str != '*' && *str !=',' && *str != ')' ) {
					str++;
				}
				int value;
				if (*str=='*') { //there may be an IDS table
					str++;
					ieVariable idsTabName;
					char* tmp = idsTabName;
					while (( *str != ',' ) && ( *str != ')' )) {
						*tmp = *str;
						tmp++;
						str++;
					}
					*tmp = 0;
					if (idsTabName[0]) {
						value = GetIdsValue(src, idsTabName);
					}
					else {
						value = strtol( src, (char **) &src, 0);
					}
				}
				else { //no IDS table
					value = strtol( src, (char **) &src, 0);
				}
				if (!intCount) {
					newAction->int0Parameter = value;
				} else if (intCount == 1) {
					newAction->int1Parameter = value;
				} else {
					newAction->int2Parameter = value;
				}
				intCount++;
			}
			break;

			case 'a':
			//Action
			{
				SKIP_ARGUMENT();
				char action[257];
				int i = 0;
				int openParenthesisCount = 0;
				while (true) {
					if (*src == ')') {
						if (!openParenthesisCount)
							break;
						openParenthesisCount--;
					} else {
						if (*src == '(') {
							openParenthesisCount++;
						} else {
							if (( *src == ',' ) &&
								!openParenthesisCount)
								break;
						}
					}
					action[i] = *src;
					i++;
					src++;
				}
				action[i] = 0;
				Action* act = GenerateAction( action);
				if (!act) {
					delete newAction;
					return NULL;
				}
				act->objects[0] = newAction->objects[0];
				newAction->objects[0] = NULL; //avoid freeing of object
				delete newAction; //freeing action
				newAction = act;
			}
			break;

			case 'o': //Object
				if (objectCount==3) {
					printf("Invalid object count!\n");
					//abort();
					delete newAction;
					return NULL;
				}
				ParseObject(str, src, newAction->objects[objectCount++]);
				break;

			case 's': //String
			{
				SKIP_ARGUMENT();
				src++;
				int i;
				char* dst;
				if (!stringsCount) {
					dst = newAction->string0Parameter;
				} else {
					dst = newAction->string1Parameter;
				}
				//if there are 3 strings, the first 2 will be merged,
				//the last one will be left alone
				if (*str==')') {
					mergestrings = 0;
				}
				//skipping the context part, which
				//is to be readed later
				if (mergestrings) {
					for (i=0;i<6;i++) {
						*dst++='*';
					}
				}
				else {
					i=0;
				}
				//breaking on ',' in case of a monkey attack
				//fixes bg1:melicamp.dlg, bg1:sharte.dlg
				//if strings ever need a , inside, this is a FIXME
				while (*src != '"' && *src !=',') {
					if (*src == 0) {
						delete newAction;
						return NULL;
					}
					//sizeof(context+name) = 40
					if (i<40) {
						*dst++ = (char) tolower(*src);
						i++;
					}
					src++;
				}
				*dst = 0;
				//reading the context part
				if (mergestrings) {
					str++;
					if (*str!='s') {
						printf("Invalid mergestrings:%s\n",str);
						//abort();
						delete newAction;
						return NULL;
					}
					SKIP_ARGUMENT();
					if (!stringsCount) {
						dst = newAction->string0Parameter;
					} else {
						dst = newAction->string1Parameter;
					}

					//this works only if there are no spaces
					if (*src++!='"' || *src++!=',' || *src++!='"') {
						break;
					}
					//reading the context string
					i=0;
					while (*src != '"') {
						if (*src == 0) {
							delete newAction;
							return NULL;
						}
						if (i++<6) {
							*dst++ = (char) tolower(*src);
						}
						src++;
					}
				}
				src++; //skipping "
				stringsCount++;
			}
			break;
		}
		str++;
		if (*src == ',' || *src==')')
			src++;
	}
	return newAction;
}

void GoNear(Scriptable *Sender, const Point &p)
{
	if (Sender->GetCurrentAction()) {
		printMessage("GameScript","Target busy???\n",LIGHT_RED);
		return;
	}
	char Tmp[256];
	sprintf( Tmp, "MoveToPoint([%hd.%hd])", p.x, p.y );
	Action * action = GenerateAction( Tmp);
	Sender->AddActionInFront( action );
}

void MoveNearerTo(Scriptable *Sender, Scriptable *target, int distance)
{
	Point p;
	Map *myarea, *hisarea;

	if (Sender->Type != ST_ACTOR) {
		printMessage("GameScript","MoveNearerTo only works with actors\n",LIGHT_RED);
		Sender->ReleaseCurrentAction();
		return;
	}

	myarea = Sender->GetCurrentArea();
	hisarea = target->GetCurrentArea();
	if (hisarea && hisarea!=myarea) {
		target = myarea->GetTileMap()->GetTravelTo(hisarea->GetScriptName());

		if (!target) {
			printMessage("GameScript", "MoveNearerTo failed to find an exit\n", YELLOW);
			Sender->ReleaseCurrentAction();
			return;
		}
		((Actor *) Sender)->UseExit(target->GetGlobalID());
	} else {
		((Actor *) Sender)->UseExit(0);
	}
	// we deliberately don't try GetLikelyPosition here for now,
	// maybe a future idea if we have a better implementation
	// (the old code used it - by passing true not 0 below - when target was a movable)
	GetPositionFromScriptable(target, p, 0);

	// account for PersonalDistance (which caller uses, but pathfinder doesn't)
	if (distance && Sender->Type == ST_ACTOR) {
		distance += ((Actor *)Sender)->size*10;
	}
	if (distance && target->Type == ST_ACTOR) {
		distance += ((Actor *)target)->size*10;
	}

	MoveNearerTo(Sender, p, distance, 0);
}

//It is not always good to release the current action if target is unreachable
//we should also raise the trigger TargetUnreachable (if this is an Attack, at least)
//i hacked only this low level function, didn't need the higher ones so far
int MoveNearerTo(Scriptable *Sender, const Point &p, int distance, int dont_release)
{
	if (Sender->Type != ST_ACTOR) {
		printMessage("GameScript","MoveNearerTo only works with actors\n",LIGHT_RED);
		Sender->ReleaseCurrentAction();
		return 0;
	}

	//chasing is unbreakable
	//TODO: is this true?
	Sender->CurrentActionInterruptable = false;

	Actor *actor = (Actor *)Sender;

	if (!actor->InMove() || actor->Destination != p) {
		actor->WalkTo(p, 0, distance);
	}

	if (!actor->InMove()) {
		//didn't release
		if (dont_release) {
			return dont_release;
		}
		// we can't walk any nearer to destination, give up
		Sender->ReleaseCurrentAction();
	}
	return 0;
}

void FreeSrc(SrcVector *poi, const ieResRef key)
{
	int res = SrcCache.DecRef((void *) poi, key, true);
	if (res<0) {
		printMessage( "GameScript", "Corrupted Src cache encountered (reference count went below zero), ", LIGHT_RED );
		printf( "Src name is: %.8s\n", key);
		abort();
	}
	if (!res) {
		delete poi;
	}
}

SrcVector *LoadSrc(const ieResRef resname)
{
	SrcVector *src = (SrcVector *) SrcCache.GetResource(resname);
	if (src) {
		return src;
	}
	DataStream* str = gamedata->GetResource( resname, IE_SRC_CLASS_ID );
	if ( !str) {
		return NULL;
	}
	ieDword size=0;
	str->ReadDword(&size);
	src = new SrcVector(size);
	SrcCache.SetAt( resname, (void *) src );
	while (size--) {
		ieDword tmp;
		str->ReadDword(&tmp);
		src->at(size)=tmp;
		str->ReadDword(&tmp);
	}
	delete ( str );
	return src;
}

#define MEMCPY(a,b) memcpy((a),(b),sizeof(a) )

static Object *ObjectCopy(Object *object)
{
	if (!object) return NULL;
	Object *newObject = new Object();
	MEMCPY( newObject->objectFields, object->objectFields );
	MEMCPY( newObject->objectFilters, object->objectFilters );
	MEMCPY( newObject->objectRect, object->objectRect );
	MEMCPY( newObject->objectName, object->objectName );
	return newObject;
}

Action *ParamCopy(Action *parameters)
{
	Action *newAction = new Action(true);
	newAction->actionID = parameters->actionID;
	newAction->int0Parameter = parameters->int0Parameter;
	newAction->int1Parameter = parameters->int1Parameter;
	newAction->int2Parameter = parameters->int2Parameter;
	newAction->pointParameter = parameters->pointParameter;
	MEMCPY( newAction->string0Parameter, parameters->string0Parameter );
	MEMCPY( newAction->string1Parameter, parameters->string1Parameter );
	for (int c=0;c<3;c++) {
		newAction->objects[c]= ObjectCopy( parameters->objects[c] );
	}
	return newAction;
}

Action *ParamCopyNoOverride(Action *parameters)
{
	Action *newAction = new Action(true);
	newAction->actionID = parameters->actionID;
	newAction->int0Parameter = parameters->int0Parameter;
	newAction->int1Parameter = parameters->int1Parameter;
	newAction->int2Parameter = parameters->int2Parameter;
	newAction->pointParameter = parameters->pointParameter;
	MEMCPY( newAction->string0Parameter, parameters->string0Parameter );
	MEMCPY( newAction->string1Parameter, parameters->string1Parameter );
	newAction->objects[0]= NULL;
	newAction->objects[1]= ObjectCopy( parameters->objects[1] );
	newAction->objects[2]= ObjectCopy( parameters->objects[2] );
	return newAction;
}

Trigger *GenerateTriggerCore(const char *src, const char *str, int trIndex, int negate)
{
	Trigger *newTrigger = new Trigger();
	newTrigger->triggerID = (unsigned short) triggersTable->GetValueIndex( trIndex )&0x3fff;
	newTrigger->flags = (unsigned short) negate;
	int mergestrings = triggerflags[newTrigger->triggerID]&TF_MERGESTRINGS;
	int stringsCount = 0;
	int intCount = 0;
	//Here is the Trigger; Now we need to evaluate the parameters
	if (*str!=')') while (*str) {
		if (*(str+1)!=':') {
			printf("Warning, parser was sidetracked: %s\n",str);
		}
		switch (*str) {
			default:
				printf("Invalid type: %s\n",str);
				//str++;
				delete newTrigger;
				return NULL;
				break;

			case 'p': //Point
				SKIP_ARGUMENT();
				src++; //Skip [
				newTrigger->pointParameter.x = (short) strtol( src, (char **) &src, 10 );
				src++; //Skip .
				newTrigger->pointParameter.y = (short) strtol( src, (char **) &src, 10 );
				src++; //Skip ]
				break;

			case 'i': //Integer
			{
				//going to the variable name
				while (*str != '*' && *str !=',' && *str != ')' ) {
					str++;
				}
				int value;
				if (*str=='*') { //there may be an IDS table
					str++;
					ieVariable idsTabName;
					char* tmp = idsTabName;
					while (( *str != ',' ) && ( *str != ')' )) {
						*tmp = *str;
						tmp++;
						str++;
					}
					*tmp = 0;
					if (idsTabName[0]) {
						value = GetIdsValue(src, idsTabName);
					}
					else {
						value = strtol( src, (char **) &src, 0);
					}
				}
				else { //no IDS table
					value = strtol( src, (char **) &src, 0);
				}
				if (!intCount) {
					newTrigger->int0Parameter = value;
				} else if (intCount == 1) {
					newTrigger->int1Parameter = value;
				} else {
					newTrigger->int2Parameter = value;
				}
				intCount++;
			}
			break;

			case 'o': //Object
				ParseObject(str, src, newTrigger->objectParameter);
				break;

			case 's': //String
			{
				SKIP_ARGUMENT();
				src++;
				int i;
				char* dst;
				if (!stringsCount) {
					dst = newTrigger->string0Parameter;
				} else {
					dst = newTrigger->string1Parameter;
				}
				//skipping the context part, which
				//is to be readed later
				if (mergestrings) {
					for (i=0;i<6;i++) {
						*dst++='*';
					}
				}
				else {
					i=0;
				}
				while (*src != '"') {
					if (*src == 0) {
						delete newTrigger;
						return NULL;
					}

					//sizeof(context+name) = 40
					if (i<40) {
						*dst++ = (char) tolower(*src);
						i++;
					}
					src++;
				}
				*dst = 0;
				//reading the context part
				if (mergestrings) {
					str++;
					if (*str!='s') {
						printf("Invalid mergestrings:%s\n",str);
						//abort();
						delete newTrigger;
						return NULL;
					}
					SKIP_ARGUMENT();
					if (!stringsCount) {
						dst = newTrigger->string0Parameter;
					} else {
						dst = newTrigger->string1Parameter;
					}

					//this works only if there are no spaces
					if (*src++!='"' || *src++!=',' || *src++!='"') {
						break;
					}
					//reading the context string
					i=0;
					while (*src != '"') {
						if (*src == 0) {
							delete newTrigger;
							return NULL;
						}

						if (i++<6) {
							*dst++ = (char) tolower(*src);
						}
						src++;
					}
				}
				src++; //skipping "
				stringsCount++;
			}
			break;
		}
		str++;
		if (*src == ',' || *src==')')
			src++;
	}
	return newTrigger;
}

void SetVariable(Scriptable* Sender, const char* VarName, const char* Context, ieDword value)
{
	char newVarName[8+33];

	if (InDebug&ID_VARIABLES) {
		printf( "Setting variable(\"%s%s\", %d)\n", Context,
			VarName, value );
	}

	strncpy( newVarName, Context, 6 );
	newVarName[6]=0;
	if (strnicmp( newVarName, "MYAREA", 6 ) == 0) {
		Sender->GetCurrentArea()->locals->SetAt( VarName, value, NoCreate );
		return;
	}
	if (strnicmp( newVarName, "LOCALS", 6 ) == 0) {
		Sender->locals->SetAt( VarName, value, NoCreate );
		return;
	}
	Game *game = core->GetGame();
	if (HasKaputz && !strnicmp(newVarName,"KAPUTZ",6) ) {
		game->kaputz->SetAt( VarName, value );
		return;
	}

	if (strnicmp(newVarName,"GLOBAL",6) ) {
		Map *map=game->GetMap(game->FindMap(newVarName));
		if (map) {
			map->locals->SetAt( VarName, value, NoCreate);
		}
		else if (InDebug&ID_VARIABLES) {
			printMessage("GameScript"," ",YELLOW);
			printf("Invalid variable %s %s in setvariable\n",Context, VarName);
		}
	}
	else {
		game->locals->SetAt( VarName, ( ieDword ) value, NoCreate );
	}
}

void SetVariable(Scriptable* Sender, const char* VarName, ieDword value)
{
	char newVarName[8];
	const char *poi;

	poi = &VarName[6];
	//some HoW triggers use a : to separate the scope from the variable name
	if (*poi==':') {
		poi++;
	}

	if (InDebug&ID_VARIABLES) {
		printf( "Setting variable(\"%s\", %d)\n", VarName, value );
	}
	strncpy( newVarName, VarName, 6 );
	newVarName[6]=0;
	if (strnicmp( newVarName, "MYAREA", 6 ) == 0) {
		Sender->GetCurrentArea()->locals->SetAt( poi, value, NoCreate );
		return;
	}
	if (strnicmp( newVarName, "LOCALS", 6 ) == 0) {
		Sender->locals->SetAt( poi, value, NoCreate );
		return;
	}
	Game *game = core->GetGame();
	if (HasKaputz && !strnicmp(newVarName,"KAPUTZ",6) ) {
		game->kaputz->SetAt( poi, value, NoCreate );
		return;
	}
	if (strnicmp(newVarName,"GLOBAL",6) ) {
		Map *map=game->GetMap(game->FindMap(newVarName));
		if (map) {
			map->locals->SetAt( poi, value, NoCreate);
		}
		else if (InDebug&ID_VARIABLES) {
			printMessage("GameScript"," ",YELLOW);
			printf("Invalid variable %s in setvariable\n",VarName);
		}
	}
	else {
		game->locals->SetAt( poi, ( ieDword ) value, NoCreate );
	}
}

ieDword CheckVariable(Scriptable* Sender, const char* VarName, bool *valid)
{
	char newVarName[8];
	const char *poi;
	ieDword value = 0;

	strncpy( newVarName, VarName, 6 );
	newVarName[6]=0;
	poi = &VarName[6];
	//some HoW triggers use a : to separate the scope from the variable name
	if (*poi==':') {
		poi++;
	}

	if (strnicmp( newVarName, "MYAREA", 6 ) == 0) {
		Sender->GetCurrentArea()->locals->Lookup( poi, value );
		if (InDebug&ID_VARIABLES) {
			printf("CheckVariable %s: %d\n",VarName, value);
		}
		return value;
	}
	if (strnicmp( newVarName, "LOCALS", 6 ) == 0) {
		Sender->locals->Lookup( poi, value );
		if (InDebug&ID_VARIABLES) {
			printf("CheckVariable %s: %d\n",VarName, value);
		}
		return value;
	}
	Game *game = core->GetGame();
	if (HasKaputz && !strnicmp(newVarName,"KAPUTZ",6) ) {
		game->kaputz->Lookup( poi, value );
		if (InDebug&ID_VARIABLES) {
			printf("CheckVariable %s: %d\n",VarName, value);
		}
		return value;
	}
	if (strnicmp(newVarName,"GLOBAL",6) ) {
		Map *map=game->GetMap(game->FindMap(newVarName));
		if (map) {
			map->locals->Lookup( poi, value);
		} else {
			if (valid) {
				*valid=false;
			}
			if (InDebug&ID_VARIABLES) {
				printMessage("GameScript"," ",YELLOW);
				printf("Invalid variable %s in checkvariable\n",VarName);
			}
		}
	} else {
		game->locals->Lookup( poi, value );
	}
	if (InDebug&ID_VARIABLES) {
		printf("CheckVariable %s: %d\n",VarName, value);
	}
	return value;
}

ieDword CheckVariable(Scriptable* Sender, const char* VarName, const char* Context, bool *valid)
{
	char newVarName[8];
	ieDword value = 0;

	strncpy(newVarName, Context, 6);
	newVarName[6]=0;
	if (strnicmp( newVarName, "MYAREA", 6 ) == 0) {
		Sender->GetCurrentArea()->locals->Lookup( VarName, value );
		if (InDebug&ID_VARIABLES) {
			printf("CheckVariable %s%s: %d\n",Context, VarName, value);
		}
		return value;
	}
	if (strnicmp( newVarName, "LOCALS", 6 ) == 0) {
		Sender->locals->Lookup( VarName, value );
		if (InDebug&ID_VARIABLES) {
			printf("CheckVariable %s%s: %d\n",Context, VarName, value);
		}
		return value;
	}
	Game *game = core->GetGame();
	if (HasKaputz && !strnicmp(newVarName,"KAPUTZ",6) ) {
		game->kaputz->Lookup( VarName, value );
		if (InDebug&ID_VARIABLES) {
			printf("CheckVariable %s%s: %d\n",Context, VarName, value);
		}
		return value;
	}
	if (strnicmp(newVarName,"GLOBAL",6) ) {
		Map *map=game->GetMap(game->FindMap(newVarName));
		if (map) {
			map->locals->Lookup( VarName, value);
		} else {
			if (valid) {
				*valid=false;
			}
			if (InDebug&ID_VARIABLES) {
				printMessage("GameScript"," ",YELLOW);
				printf("Invalid variable %s %s in checkvariable\n",Context, VarName);
			}
		}
	} else {
		game->locals->Lookup( VarName, value );
	}
	if (InDebug&ID_VARIABLES) {
		printf("CheckVariable %s%s: %d\n",Context, VarName, value);
	}
	return value;
}

int DiffCore(ieDword a, ieDword b, int diffmode)
{
	switch (diffmode) {
		case LESS_THAN:
			if (a<b) {
				return 1;
			}
			break;
		case EQUALS:
			if (a==b) {
				return 1;
			}
			break;
		case GREATER_THAN:
			if (a>b) {
				return 1;
			}
			break;
		case GREATER_OR_EQUALS:
			if (a>=b) {
				return 1;
			}
			break;
		case NOT_EQUALS:
			if (a!=b) {
				return 1;
			}
			break;
		case BINARY_LESS_OR_EQUALS:
			if ((a&b) == a) {
				return 1;
			}
			break;
		case BINARY_MORE:
			if ((a&b) != a) {
				return 1;
			}
			break;
		case BINARY_MORE_OR_EQUALS:
			if ((a&b) == b) {
				return 1;
			}
			break;
		case BINARY_LESS:
			if ((a&b) != b) {
				return 1;
			}
			break;
		case BINARY_INTERSECT:
			if (a&b) {
				return 1;
			}
			break;
		case BINARY_NOT_INTERSECT:
			if (!(a&b)) {
				return 1;
			}
			break;
		default: //less or equals
			if (a<=b) {
				return 1;
			}
			break;
	}
	return 0;
}

int GetGroup(Actor *actor)
{
	int type = 2; //neutral, has no enemies
	if (actor->GetStat(IE_EA) <= EA_GOODCUTOFF) {
		type = 1; //PC
	}
	if (actor->GetStat(IE_EA) >= EA_EVILCUTOFF) {
		type = 0;
	}
	return type;
}

Point GetEntryPoint(const char *areaname, const char *entryname)
{
	Point p;

	AutoTable tab("entries");
	if (!tab) {
		return p;
	}
	const char *tmpstr = tab->QueryField(areaname, entryname);
	int x=-1;
	int y=-1;
	sscanf(tmpstr, "%d.%d", &x, &y);
	p.x=(short) x;
	p.y=(short) y;
	return p;
}

/* returns a spell's casting distance, it depends on the caster (level), and targeting mode too
 the used header is calculated from the caster level */
unsigned int GetSpellDistance(const ieResRef spellres, Scriptable *Sender)
{
	unsigned int dist;

	Spell* spl = gamedata->GetSpell( spellres );
	if (!spl) {
		printMessage("GameScript"," ",LIGHT_RED);
		printf("Spell couldn't be found:%.8s.\n", spellres);
		return 0;
	}
	dist = spl->GetCastingDistance(Sender);
	//make possible special return values (like 0xffffffff means the spell doesn't need distance)
	//this is used with special targeting mode (3)
	if (dist>0xff000000) {
		return dist;
	}

	gamedata->FreeSpell(spl, spellres, false);
	return dist*5; //FIXME: this empirical constant shouldn't be needed!
}

/* returns an item's casting distance, it depends on the used header, and targeting mode too
 the used header is explictly given */
unsigned int GetItemDistance(const ieResRef itemres, int header)
{
	unsigned int dist;

	Item* itm = gamedata->GetItem( itemres );
	if (!itm) {
		printMessage("GameScript"," ",LIGHT_RED);
		printf("Item couldn't be found:%.8s.\n", itemres);
		return 0;
	}
	dist=itm->GetCastingDistance(header);
	//make possible special return values (like 0xffffffff means the item doesn't need distance)
	//this is used with special targeting mode (3)
	if (dist>0xff000000) {
		return dist;
	}

	gamedata->FreeItem(itm, itemres, false);
	return dist*15;
}

//read the wish 2da
void SetupWishCore(Scriptable *Sender, int column, int picks)
{
	int count;
	ieVariable varname;
	int *selects;
	int i,j;

	//FIXME: find out what the original really used the picks parameter for
	if (picks == 1) picks = 5;

	AutoTable tm("wish");
	if (!tm) {
		printStatus( "ERROR", LIGHT_RED );
		printf( "Cannot find wish.2da.\n");
		return;
	}

	selects = (int *) malloc(picks*sizeof(int));
	count = tm->GetRowCount();

	for(i=0;i<99;i++) {
		snprintf(varname,32, "wishpower%02d", i);
		if(CheckVariable(Sender, varname, "GLOBAL") ) {
			SetVariable(Sender, varname, "GLOBAL", 0);
		}
	}

	if (count<picks) {
		for(i=0;i<count;i++) {
			selects[i]=i;
		}
		while(i++<picks) {
			selects[i]=-1;
		}
	} else {
		for(i=0;i<picks;i++) {
			selects[i]=rand()%count;
retry:
			for(j=0;j<i;j++) {
				if(selects[i]==selects[j]) {
					selects[i]++;
					goto retry;
				}
			}
		}
	}

	for (i = 0; i < picks; i++) {
		if (selects[i]<0)
			continue;
		int spnum = atoi( tm->QueryField( selects[i], column-1 ) );
		snprintf(varname,32,"wishpower%02d", spnum);
		SetVariable(Sender, varname, "GLOBAL",1);
	}
	free(selects);
}

#define MAX_ISLAND_POLYGONS  10

//read a polygon 2da
Gem_Polygon *GetPolygon2DA(ieDword index)
{
	ieResRef resref;

	if (index>=MAX_ISLAND_POLYGONS) {
		return NULL;
	}

	if (!polygons) {
		polygons = (Gem_Polygon **) calloc(MAX_ISLAND_POLYGONS, sizeof(Gem_Polygon *) );
	}
	if (polygons[index]) {
		return polygons[index];
	}
	snprintf(resref, sizeof(ieResRef), "ISLAND%02d", index);
	AutoTable tm(resref);
	if (!tm) {
		return NULL;
	}
	int cnt = tm->GetRowCount();
	if (!cnt) {
		return NULL;
	}
	Point *p = new Point[cnt];

	int i = cnt;
	while(i--) {
		p[i].x = atoi(tm->QueryField(i, 0));
		p[i].y = atoi(tm->QueryField(i, 1));
	}

	polygons[index] = new Gem_Polygon(p, cnt, NULL);
	delete [] p;
	return polygons[index];
}

