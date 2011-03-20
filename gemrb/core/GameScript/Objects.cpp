/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
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

#include "GameScript/GS.h"

#include "GameScript/GSUtils.h"
#include "GameScript/Matching.h"

#include "win32def.h"

#include "DialogHandler.h"
#include "Game.h"
#include "GUI/GameControl.h"

//-------------------------------------------------------------
// Object Functions
//-------------------------------------------------------------

//in this implementation, Myself will drop the parameter array
//i think all object filters could be expected to do so
//they should remove unnecessary elements from the parameters
Targets *Myself(Scriptable* Sender, Targets* parameters, int ga_flags)
{
	parameters->Clear();
	parameters->AddTarget(Sender, 0, ga_flags);
	return parameters;
}

Targets *NearestDoor(Scriptable* /*Sender*/, Targets *parameters, int /*ga_flags*/)
{
	return XthNearestDoor(parameters, 0);
}

Targets *SecondNearestDoor(Scriptable* /*Sender*/, Targets *parameters, int /*ga_flags*/)
{
	return XthNearestDoor(parameters, 1);
}

Targets *ThirdNearestDoor(Scriptable* /*Sender*/, Targets *parameters, int /*ga_flags*/)
{
	return XthNearestDoor(parameters, 2);
}

Targets *FourthNearestDoor(Scriptable* /*Sender*/, Targets *parameters, int /*ga_flags*/)
{
	return XthNearestDoor(parameters, 3);
}

Targets *FifthNearestDoor(Scriptable* /*Sender*/, Targets *parameters, int /*ga_flags*/)
{
	return XthNearestDoor(parameters, 4);
}

Targets *SixthNearestDoor(Scriptable* /*Sender*/, Targets *parameters, int /*ga_flags*/)
{
	return XthNearestDoor(parameters, 5);
}

Targets *SeventhNearestDoor(Scriptable* /*Sender*/, Targets *parameters, int /*ga_flags*/)
{
	return XthNearestDoor(parameters, 6);
}

Targets *EighthNearestDoor(Scriptable* /*Sender*/, Targets *parameters, int /*ga_flags*/)
{
	return XthNearestDoor(parameters, 7);
}

Targets *NinthNearestDoor(Scriptable* /*Sender*/, Targets *parameters, int /*ga_flags*/)
{
	return XthNearestDoor(parameters, 8);
}

Targets *TenthNearestDoor(Scriptable* /*Sender*/, Targets *parameters, int /*ga_flags*/)
{
	return XthNearestDoor(parameters, 9);
}

//in bg2 it is same as player1 so far
//in iwd2 this is the Gabber!!!
//but also, if there is no gabber, it is the first PC
//probably it is simply the nearest exportable character...
Targets *Protagonist(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	//this sucks but IWD2 is like that...
	static bool charnameisgabber = core->HasFeature(GF_CHARNAMEISGABBER);
	if (charnameisgabber) {
		GameControl* gc = core->GetGameControl();
		if (gc) {
			parameters->AddTarget(gc->dialoghandler->GetSpeaker(), 0, ga_flags);
		}
		if (parameters->Count()) {
			return parameters;
		}
		//ok, this will return the nearest PC in the first slot
		Game *game = core->GetGame();
		int i = game->GetPartySize(false);
		while(i--) {
			Actor *target = game->GetPC(i,false);
			parameters->AddTarget(target, Distance(Sender, target), ga_flags);
		}
		return parameters;
	}
	parameters->AddTarget(core->GetGame()->GetPC(0, false), 0, ga_flags);
	return parameters;
}

//last talker
Targets *Gabber(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	GameControl* gc = core->GetGameControl();
	if (gc) {
		parameters->AddTarget(gc->dialoghandler->GetSpeaker(), 0, ga_flags);
	}
	return parameters;
}

Targets *LastTrigger(Scriptable *Sender, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	if (Sender->LastTriggerObject) {
		Actor *target = Sender->GetCurrentArea()->GetActorByGlobalID(Sender->LastTriggerObject);
		parameters->AddTarget(target, 0, ga_flags);
	}
	return parameters;
}

Targets *LastMarkedObject(Scriptable *Sender, Targets *parameters, int ga_flags)
{
	Actor *actor = (Actor *) parameters->GetTarget(0, ST_ACTOR);
	if (!actor) {
		if (Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if (actor) {
		Actor *target = actor->GetCurrentArea()->GetActorByGlobalID(actor->LastMarked);
		if (target) {
			parameters->AddTarget(target, 0, ga_flags);
		}
	}
	return parameters;
}

//actions should always use LastMarkedObject, because LastSeen could be deleted
Targets *LastSeenBy(Scriptable *Sender, Targets *parameters, int ga_flags)
{
	Actor *actor = (Actor *) parameters->GetTarget(0, ST_ACTOR);
	if (!actor) {
		if (Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if (actor) {
		Actor *target = actor->GetCurrentArea()->GetActorByGlobalID(actor->LastSeen);
		if (target) {
			parameters->AddTarget(target, 0, ga_flags);
		}
	}
	return parameters;
}

Targets *LastHelp(Scriptable *Sender, Targets *parameters, int ga_flags)
{
	Actor *actor = (Actor *) parameters->GetTarget(0, ST_ACTOR);
	if (!actor) {
		if (Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if (actor) {
		Actor *target = actor->GetCurrentArea()->GetActorByGlobalID(actor->LastHelp);
		if (target) {
			parameters->AddTarget(target, 0, ga_flags);
		}
	}
	return parameters;
}

Targets *LastHeardBy(Scriptable *Sender, Targets *parameters, int ga_flags)
{
	Actor *actor = (Actor *) parameters->GetTarget(0, ST_ACTOR);
	if (!actor) {
		if (Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if (actor) {
		Actor *target = actor->GetCurrentArea()->GetActorByGlobalID(actor->LastHeard);
		if (target) {
			parameters->AddTarget(target, 0, ga_flags);
		}
	}
	return parameters;
}

//i was told that Group means the same specifics, so this is just an
//object selector for everyone with the same specifics as the current object
Targets *GroupOf(Scriptable *Sender, Targets *parameters, int ga_flags)
{
	Actor *actor = (Actor *) parameters->GetTarget(0, ST_ACTOR);
	if (!actor) {
		if (Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if (actor) {
		ieDword tmp = actor->GetStat(IE_SPECIFIC);
		Map *cm = Sender->GetCurrentArea();
		int i = cm->GetActorCount(true);
		while (i--) {
			Actor *target=cm->GetActor(i,true);
			if (target && (target->GetStat(IE_SPECIFIC)==tmp) ) {
				parameters->AddTarget(target, 0, ga_flags);
			}
		}
	}
	return parameters;
}

/*this one is tough, but done */
Targets *ProtectorOf(Scriptable *Sender, Targets *parameters, int ga_flags)
{
	Actor *actor = (Actor *) parameters->GetTarget(0, ST_ACTOR);
	if (!actor) {
		if (Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if (actor) {
		ieWord tmp = actor->LastProtected;
		Map *cm = Sender->GetCurrentArea();
		int i = cm->GetActorCount(true);
		while (i--) {
			Actor *target=cm->GetActor(i,true);
			if (target && (target->LastProtected ==tmp) ) {
				parameters->AddTarget(target, 0, ga_flags);
			}
		}
	}
	return parameters;
}

Targets *ProtectedBy(Scriptable *Sender, Targets *parameters, int ga_flags)
{
	Actor *actor = (Actor *) parameters->GetTarget(0, ST_ACTOR);
	if (!actor) {
		if (Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if (actor) {
		Actor *target = actor->GetCurrentArea()->GetActorByGlobalID(actor->LastProtected);
		if (target) {
			parameters->AddTarget(target, 0, ga_flags);
		}
	}
	return parameters;
}

Targets *LastCommandedBy(Scriptable *Sender, Targets *parameters, int ga_flags)
{
	Actor *actor = (Actor *) parameters->GetTarget(0, ST_ACTOR);
	if (!actor) {
		if (Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if (actor) {
		Actor *target = actor->GetCurrentArea()->GetActorByGlobalID(actor->LastCommander);
		if (target) {
			parameters->AddTarget(target, 0, ga_flags);
		}
	}
	return parameters;
}

// this is essentially a LastTargetedBy(0) - or MySelf
// but IWD2 defines it
Targets *MyTarget(Scriptable *Sender, Targets *parameters, int ga_flags)
{
	return GetMyTarget(Sender, NULL, parameters, ga_flags);
}

Targets *LastTargetedBy(Scriptable *Sender, Targets *parameters, int ga_flags)
{
	Actor *actor = (Actor *) parameters->GetTarget(0, ST_ACTOR);
	return GetMyTarget(Sender, actor, parameters, ga_flags);
}

Targets *LastAttackerOf(Scriptable *Sender, Targets *parameters, int ga_flags)
{
	Actor *actor = (Actor *) parameters->GetTarget(0, ST_ACTOR);
	if (!actor) {
		if (Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if (actor) {
		Actor *target = actor->GetCurrentArea()->GetActorByGlobalID(actor->LastHitter);
		if (target) {
			parameters->AddTarget(target, 0, ga_flags);
		}
	}
	return parameters;
}

Targets *LastHitter(Scriptable *Sender, Targets *parameters, int ga_flags)
{
	Actor *actor = (Actor *) parameters->GetTarget(0, ST_ACTOR);
	if (!actor) {
		if (Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if (actor) {
		Actor *target = actor->GetCurrentArea()->GetActorByGlobalID(actor->LastHitter);
		if (target) {
			parameters->AddTarget(target, 0, ga_flags);
		}
	}
	return parameters;
}

Targets *LeaderOf(Scriptable *Sender, Targets *parameters, int ga_flags)
{
	Actor *actor = (Actor *) parameters->GetTarget(0, ST_ACTOR);
	if (!actor) {
		if (Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if (actor) {
		Actor *target = actor->GetCurrentArea()->GetActorByGlobalID(actor->LastFollowed);
		if (target) {
			parameters->AddTarget(target, 0, ga_flags);
		}
	}
	return parameters;
}

Targets *LastTalkedToBy(Scriptable *Sender, Targets *parameters, int ga_flags)
{
	Actor *actor = (Actor *) parameters->GetTarget(0, ST_ACTOR);
	if (!actor) {
		if (Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if (actor) {
		Actor *target = actor->GetCurrentArea()->GetActorByGlobalID(actor->LastTalkedTo);
		if (target) {
			parameters->AddTarget(target, 0, ga_flags);
		}
	}
	return parameters;
}

Targets *LastSummonerOf(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	Actor *actor = (Actor *) parameters->GetTarget(0, ST_ACTOR);
	if (!actor) {
		if (Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if (actor) {
		Actor *target = actor->GetCurrentArea()->GetActorByGlobalID(actor->LastSummoner);
		if (target) {
			parameters->AddTarget(target, 0, ga_flags);
		}
	}
	return parameters;
}

Targets *Player1(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->GetPC(0,false), 0, ga_flags);
	return parameters;
}

Targets *Player1Fill(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->FindPC(1), 0, ga_flags);
	return parameters;
}

Targets *Player2(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->GetPC(1,false), 0, ga_flags);
	return parameters;
}

Targets *Player2Fill(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->FindPC(2), 0, ga_flags);
	return parameters;
}

Targets *Player3(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->GetPC(2,false), 0, ga_flags);
	return parameters;
}

Targets *Player3Fill(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->FindPC(3), 0, ga_flags);
	return parameters;
}

Targets *Player4(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->GetPC(3,false), 0, ga_flags);
	return parameters;
}

Targets *Player4Fill(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->FindPC(4), 0, ga_flags);
	return parameters;
}

Targets *Player5(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->GetPC(4,false), 0, ga_flags);
	return parameters;
}

Targets *Player5Fill(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->FindPC(5), 0, ga_flags);
	return parameters;
}

Targets *Player6(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->GetPC(5,false), 0, ga_flags);
	return parameters;
}

Targets *Player6Fill(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->FindPC(6), 0, ga_flags);
	return parameters;
}

Targets *Player7(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->GetPC(6,false), 0, ga_flags);
	return parameters;
}

Targets *Player7Fill(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->FindPC(7), 0, ga_flags);
	return parameters;
}

Targets *Player8(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->GetPC(7,false), 0, ga_flags);
	return parameters;
}

Targets *Player8Fill(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->FindPC(8), 0, ga_flags);
	return parameters;
}

Targets *BestAC(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	targetlist::iterator m;
	const targettype *t = parameters->GetFirstTarget(m, ST_ACTOR);
	if (!t) {
		return parameters;
	}
	Scriptable *scr=t->actor;
	Actor *actor=(Actor *) scr;
	int bestac=actor->GetStat(IE_ARMORCLASS);
	// assignment in while
	while ( (t = parameters->GetNextTarget(m, ST_ACTOR) ) ) {
		actor = (Actor *) t->actor;
		int ac=actor->GetStat(IE_ARMORCLASS);
		if (bestac<ac) {
			bestac=ac;
			scr=t->actor;
		}
	}

	parameters->Clear();
	parameters->AddTarget(scr, 0, ga_flags);
	return parameters;
}

/*no idea why this object exists since the gender could be filtered easier*/
Targets *StrongestOfMale(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	targetlist::iterator m;
	const targettype *t = parameters->GetFirstTarget(m, ST_ACTOR);
	if (!t) {
		return parameters;
	}
	int pos=-1;
	int worsthp=-1;
	Scriptable *scr = NULL;
	//assignment intentional
	while ( (t = parameters->GetNextTarget(m, ST_ACTOR) ) ) {
		Actor *actor = (Actor *) t->actor;
		if (actor->GetStat(IE_SEX)!=SEX_MALE) continue;
		int hp=actor->GetStat(IE_HITPOINTS);
		if ((pos==-1) || (worsthp<hp)) {
			worsthp=hp;
			scr=t->actor;
		}
	}
	parameters->Clear();
	if (scr) {
		parameters->AddTarget(scr, 0, ga_flags);
	}
	return parameters;
}

Targets *StrongestOf(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	targetlist::iterator m;
	const targettype *t = parameters->GetFirstTarget(m, ST_ACTOR);
	if (!t) {
		return parameters;
	}
	Scriptable *scr=t->actor;
	Actor *actor=(Actor *) scr;
	int besthp=actor->GetStat(IE_HITPOINTS);
	// assignment in while
	while ( (t = parameters->GetNextTarget(m, ST_ACTOR) ) ) {
		actor = (Actor *) t->actor;
		int hp=actor->GetStat(IE_HITPOINTS);
		if (besthp<hp) {
			besthp=hp;
			scr=t->actor;
		}
	}
	parameters->Clear();
	parameters->AddTarget(scr, 0, ga_flags);
	return parameters;
}

Targets *WeakestOf(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	targetlist::iterator m;
	const targettype *t = parameters->GetFirstTarget(m, ST_ACTOR);
	if (!t) {
		return parameters;
	}
	Scriptable *scr=t->actor;
	Actor *actor=(Actor *) scr;
	int worsthp=actor->GetStat(IE_HITPOINTS);
	// assignment in while
	while ( (t = parameters->GetNextTarget(m, ST_ACTOR) ) ) {
		actor = (Actor *) t->actor;
		int hp=actor->GetStat(IE_HITPOINTS);
		if (worsthp>hp) {
			worsthp=hp;
			scr=t->actor;
		}
	}
	parameters->Clear();
	parameters->AddTarget(scr, 0, ga_flags);
	return parameters;
}

Targets *WorstAC(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	targetlist::iterator m;
	const targettype *t = parameters->GetFirstTarget(m, ST_ACTOR);
	if (!t) {
		return parameters;
	}
	Scriptable *scr=t->actor;
	Actor *actor=(Actor *) scr;
	int worstac=actor->GetStat(IE_ARMORCLASS);
	// assignment in while
	while ( (t = parameters->GetNextTarget(m, ST_ACTOR) ) ) {
		actor = (Actor *) t->actor;
		int ac=actor->GetStat(IE_ARMORCLASS);
		if (worstac>ac) {
			worstac=ac;
			scr=t->actor;
		}
	}
	parameters->Clear();
	parameters->AddTarget(scr, 0, ga_flags);
	return parameters;
}

Targets *MostDamagedOf(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	//Original engines restrict this to the PCs...
	/*targetlist::iterator m;
	const targettype *t = parameters->GetFirstTarget(m, ST_ACTOR);
	if (!t) {
		return parameters;
	}
	Scriptable *scr = t->actor;
	Actor *actor=(Actor *) scr;
	int worsthp=actor->GetStat(IE_MAXHITPOINTS)-actor->GetBase(IE_HITPOINTS);
	// assignment in while
	while ( (t = parameters->GetNextTarget(m, ST_ACTOR) ) ) {
		actor = (Actor *) t->actor;
		int hp=actor->GetStat(IE_MAXHITPOINTS)-actor->GetBase(IE_HITPOINTS);
		if (worsthp>hp) {
			worsthp=hp;
			scr=t->actor;
		}
	}
	parameters->Clear();
	parameters->AddTarget(scr, 0, ga_flags);
	return parameters;*/
	Map* area = Sender->GetCurrentArea() ;
	Game *game = core->GetGame();
	Scriptable* scr = NULL ;
	int worsthp = 0xffff ;
	int i = game->GetPartySize(false);
	while (i--) {
		Actor *actor = game->GetPC(i, false);
		if(actor->GetCurrentArea() == area) {
			int hp=actor->GetStat(IE_MAXHITPOINTS)-actor->GetBase(IE_HITPOINTS);
			if (worsthp>hp) {
				worsthp=hp;
				scr=actor;
			}
		}
	}
	parameters->Clear();
	parameters->AddTarget(scr, 0, ga_flags);
	return parameters;
}
Targets *LeastDamagedOf(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	targetlist::iterator m;
	const targettype *t = parameters->GetFirstTarget(m, ST_ACTOR);
	if (!t) {
		return parameters;
	}
	Scriptable *scr = t->actor;
	Actor *actor = (Actor *) scr;
	int besthp=actor->GetStat(IE_MAXHITPOINTS)-actor->GetBase(IE_HITPOINTS);
	// assignment in while
	while ( (t = parameters->GetNextTarget(m, ST_ACTOR) ) ) {
		actor = (Actor *) t->actor;
		int hp=actor->GetStat(IE_MAXHITPOINTS)-actor->GetBase(IE_HITPOINTS);
		if (besthp<hp) {
			besthp=hp;
			scr=t->actor;
		}
	}
	parameters->Clear();
	parameters->AddTarget(scr, 0, ga_flags);
	return parameters;
}

Targets *Farthest(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	const targettype *t = parameters->GetLastTarget(ST_ACTOR);
	parameters->Clear();
	if (t) {
		parameters->AddTarget(t->actor, 0, ga_flags);
	}
	return parameters;
}

Targets *FarthestEnemyOf(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOf(parameters, -1, ga_flags);
}

Targets *NearestEnemyOf(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOf(parameters, 0, ga_flags);
}

Targets *SecondNearestEnemyOf(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOf(parameters, 1, ga_flags);
}

Targets *ThirdNearestEnemyOf(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOf(parameters, 2, ga_flags);
}

Targets *FourthNearestEnemyOf(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOf(parameters, 3, ga_flags);
}

Targets *FifthNearestEnemyOf(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOf(parameters, 4, ga_flags);
}

Targets *SixthNearestEnemyOf(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOf(parameters, 5, ga_flags);
}

Targets *SeventhNearestEnemyOf(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOf(parameters, 6, ga_flags);
}

Targets *EighthNearestEnemyOf(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOf(parameters, 7, ga_flags);
}

Targets *NinthNearestEnemyOf(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOf(parameters, 8, ga_flags);
}

Targets *TenthNearestEnemyOf(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOf(parameters, 9, ga_flags);
}

Targets *NearestEnemySummoned(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return ClosestEnemySummoned(Sender, parameters, ga_flags);
}

Targets *NearestEnemyOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOfType(Sender, parameters, 0, ga_flags);
}

Targets *SecondNearestEnemyOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOfType(Sender, parameters, 1, ga_flags);
}

Targets *ThirdNearestEnemyOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOfType(Sender, parameters, 2, ga_flags);
}

Targets *FourthNearestEnemyOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOfType(Sender, parameters, 3, ga_flags);
}

Targets *FifthNearestEnemyOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOfType(Sender, parameters, 4, ga_flags);
}

Targets *SixthNearestEnemyOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOfType(Sender, parameters, 5, ga_flags);
}

Targets *SeventhNearestEnemyOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOfType(Sender, parameters, 6, ga_flags);
}

Targets *EighthNearestEnemyOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOfType(Sender, parameters, 7, ga_flags);
}

Targets *NinthNearestEnemyOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOfType(Sender, parameters, 8, ga_flags);
}

Targets *TenthNearestEnemyOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOfType(Sender, parameters, 9, ga_flags);
}

Targets *NearestMyGroupOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestMyGroupOfType(Sender, parameters, 0, ga_flags);
}

Targets *SecondNearestMyGroupOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestMyGroupOfType(Sender, parameters, 1, ga_flags);
}

Targets *ThirdNearestMyGroupOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestMyGroupOfType(Sender, parameters, 2, ga_flags);
}

Targets *FourthNearestMyGroupOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestMyGroupOfType(Sender, parameters, 3, ga_flags);
}

Targets *FifthNearestMyGroupOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestMyGroupOfType(Sender, parameters, 4, ga_flags);
}

Targets *SixthNearestMyGroupOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestMyGroupOfType(Sender, parameters, 5, ga_flags);
}

Targets *SeventhNearestMyGroupOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestMyGroupOfType(Sender, parameters, 6, ga_flags);
}

Targets *EighthNearestMyGroupOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestMyGroupOfType(Sender, parameters, 7, ga_flags);
}

Targets *NinthNearestMyGroupOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestMyGroupOfType(Sender, parameters, 8, ga_flags);
}

Targets *TenthNearestMyGroupOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestMyGroupOfType(Sender, parameters, 9, ga_flags);
}

/* returns only living PC's? if not, alter getpartysize/getpc flag*/
Targets *NearestPC(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	Map *map = Sender->GetCurrentArea();
	Game *game = core->GetGame();
	int i = game->GetPartySize(true);
	int mindist = -1;
	Actor *ac = NULL;
	while (i--) {
		Actor *newactor=game->GetPC(i,true);
		//NearestPC for PC's will not give themselves as a result
		//this might be different from the original engine
		if ((Sender->Type==ST_ACTOR) && (newactor == (Actor *) Sender)) {
			continue;
		}
		if (newactor->GetCurrentArea()!=map) {
			continue;
		}
		int dist = Distance(Sender, newactor);
		if ( (mindist == -1) || (dist<mindist) ) {
			ac = newactor;
			mindist = dist;
		}
	}
	if (ac) {
		parameters->AddTarget(ac, 0, ga_flags);
	}
	return parameters;
}

Targets *Nearest(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestOf(parameters, 0, ga_flags);
}

Targets *SecondNearest(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestOf(parameters, 1, ga_flags);
}

Targets *ThirdNearest(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestOf(parameters, 2, ga_flags);
}

Targets *FourthNearest(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestOf(parameters, 3, ga_flags);
}

Targets *FifthNearest(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestOf(parameters, 4, ga_flags);
}

Targets *SixthNearest(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestOf(parameters, 5, ga_flags);
}

Targets *SeventhNearest(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestOf(parameters, 6, ga_flags);
}

Targets *EighthNearest(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestOf(parameters, 7, ga_flags);
}

Targets *NinthNearest(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestOf(parameters, 8, ga_flags);
}

Targets *TenthNearest(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestOf(parameters, 9, ga_flags);
}

Targets *SelectedCharacter(Scriptable* Sender, Targets* parameters, int ga_flags)
{
	Map *cm = Sender->GetCurrentArea();
	parameters->Clear();
	int i = cm->GetActorCount(true);
	while (i--) {
		Actor *ac=cm->GetActor(i,true);
		if (ac->GetCurrentArea()!=cm) {
			continue;
		}
		if (ac->IsSelected()) {
			parameters->AddTarget(ac, Distance(Sender, ac), ga_flags );
		}
	}
	return parameters;
}

Targets *Nothing(Scriptable* /*Sender*/, Targets* parameters, int /*ga_flags*/)
{
	parameters->Clear();
	return parameters;
}

//-------------------------------------------------------------
// IDS Functions
//-------------------------------------------------------------

int GS::ID_Alignment(Actor *actor, int parameter)
{
	int value = actor->GetStat( IE_ALIGNMENT );
	int a = parameter&15;
	if (a) {
		if (a != ( value & 15 )) {
			return 0;
		}
	}
	a = parameter & 240;
	if (a) {
		if (a != ( value & 240 )) {
			return 0;
		}
	}
	return 1;
}

int GS::ID_Allegiance(Actor *actor, int parameter)
{
	int value = actor->GetStat( IE_EA );
	switch (parameter) {
		case EA_GOODCUTOFF:
			return value <= EA_GOODCUTOFF;

		case EA_NOTGOOD:
			return value >= EA_NOTGOOD;

		case EA_NOTNEUTRAL:
			return value >=EA_EVILCUTOFF || value <= EA_GOODCUTOFF;

		case EA_NOTEVIL:
			return value <= EA_NOTEVIL;

		case EA_EVILCUTOFF:
			return value >= EA_EVILCUTOFF;

		case 0:
		case EA_ANYTHING:
			return true;

	}
	//default
	return parameter == value;
}

// *_ALL constants are different in iwd2 due to different classes (see note below)
// bard, cleric, druid, fighter, mage, paladin, ranger, thief
static const int all_bg_classes[] = { 206, 204, 208, 203, 202, 207, 209, 205 };
static const int all_iwd2_classes[] = { 202, 203, 204, 205, 209, 206, 207, 208 };

// Dual-classed characters will detect only as their new class until their
// original class is re-activated, when they will detect as a multi-class
// GetClassLevel takes care of this automatically!
inline bool idclass(Actor *actor, int parameter, bool iwd2) {
	int value = 0;
	if (parameter < 202 || parameter > 209) {
		value = actor->GetStat(IE_CLASS);
		return parameter==value;
	}

	const int *classes;
	if (iwd2) {
		classes = all_iwd2_classes;
	} else {
		classes = all_bg_classes;
	}

	// we got one of the *_ALL values
	if (parameter == classes[4]) {
		// MAGE_ALL (also sorcerers)
		value = actor->GetMageLevel() + actor->GetSorcererLevel();
	} else if (parameter == classes[3]) {
		// FIGHTER_ALL (also monks)
		value = actor->GetFighterLevel() + actor->GetMonkLevel();
	} else if (parameter == classes[1]) {
		// CLERIC_ALL
		value = actor->GetClericLevel();
	} else if (parameter == classes[7]) {
		// THIEF_ALL
		value = actor->GetThiefLevel();
	} else if (parameter == classes[0]) {
		// BARD_ALL
		value = actor->GetBardLevel();
	} else if (parameter == classes[5]) {
		// PALADIN_ALL
		value = actor->GetPaladinLevel();
	} else if (parameter == classes[2]) {
		// DRUID_ALL
		value = actor->GetDruidLevel();
	} else if (parameter == classes[6]) {
		// RANGER_ALL
		value = actor->GetRangerLevel();
	}
	return value > 0;
}

int GS::ID_Class(Actor *actor, int parameter)
{
	if (core->HasFeature(GF_3ED_RULES)) {
		//iwd2 has different values, see also the note for AVClass
		return idclass(actor, parameter, 1);
	}
	return idclass(actor, parameter, 0);
}

// IE_CLASS holds only one class, not a bitmask like with iwd2 kits. The ids values
// are friendly to binary comparison, so we just need to build such a class value
int GS::ID_ClassMask(Actor *actor, int parameter)
{
	// maybe we're lucky...
	int value = actor->GetStat(IE_CLASS);
	if (parameter&(1<<(value-1))) return 1;

	// otherwise iterate over all the classes
	value = actor->GetClassMask();

	if (parameter&value) return 1;
	return 0;
}

// this is only present in iwd2
// the function is identical to ID_Class, but uses the class20 IDS,
// iwd2's class.ids is different than the rest, while class20 is identical (remnant)
int GS::ID_AVClass(Actor *actor, int parameter)
{
	return idclass(actor, parameter, 0);
}

int GS::ID_Race(Actor *actor, int parameter)
{
	int value = actor->GetStat(IE_RACE);
	return parameter==value;
}

int GS::ID_Subrace(Actor *actor, int parameter)
{
	int value = actor->GetStat(IE_SUBRACE);
	return parameter==value;
}

int GS::ID_Faction(Actor *actor, int parameter)
{
	int value = actor->GetStat(IE_FACTION);
	return parameter==value;
}

int GS::ID_Team(Actor *actor, int parameter)
{
	int value = actor->GetStat(IE_TEAM);
	return parameter==value;
}

int GS::ID_Gender(Actor *actor, int parameter)
{
	int value = actor->GetStat(IE_SEX);
	return parameter==value;
}

int GS::ID_General(Actor *actor, int parameter)
{
	int value = actor->GetStat(IE_GENERAL);
	return parameter==value;
}

int GS::ID_Specific(Actor *actor, int parameter)
{
	int value = actor->GetStat(IE_SPECIFIC);
	return parameter==value;
}

static const ObjectDesc objectnames[] = {
	{"bestac", BestAC, 0, 0},
	{"eighthnearest", EighthNearest, 0, 0},
	{"eighthnearestdoor", EighthNearestDoor, 0, 0},
	{"eighthnearestenemyof", EighthNearestEnemyOf, 0, 0},
	{"eighthnearestenemyoftype", EighthNearestEnemyOfType, 0, 0},
	{"eighthnearestmygroupoftype", EighthNearestEnemyOfType, 0, 0},
	{"eigthnearestenemyof", EighthNearestEnemyOf, 0, 0}, //typo in iwd
	{"eigthnearestenemyoftype", EighthNearestEnemyOfType, 0, 0}, //bg2
	{"eigthnearestmygroupoftype", EighthNearestEnemyOfType, 0, 0},//bg2
	{"farthest", Farthest, 0, 0},
	{"farthestenemyof", FarthestEnemyOf, 0, 0},
	{"fifthnearest", FifthNearest, 0, 0},
	{"fifthnearestdoor", FifthNearestDoor, 0, 0},
	{"fifthnearestenemyof", FifthNearestEnemyOf, 0, 0},
	{"fifthnearestenemyoftype", FifthNearestEnemyOfType, 0, 0},
	{"fifthnearestmygroupoftype", FifthNearestEnemyOfType, 0, 0},
	{"fourthnearest", FourthNearest, 0, 0},
	{"fourthnearestdoor", FourthNearestDoor, 0, 0},
	{"fourthnearestenemyof", FourthNearestEnemyOf, 0, 0},
	{"fourthnearestenemyoftype", FourthNearestEnemyOfType, 0, 0},
	{"fourthnearestmygroupoftype", FourthNearestEnemyOfType, 0, 0},
	{"gabber", Gabber, 0, 0},
	{"groupof", GroupOf, 0, 0},
	{"lastattackerof", LastAttackerOf, 0, 0},
	{"lastcommandedby", LastCommandedBy, 0, 0},
	{"lastheardby", LastHeardBy, 0, 0},
	{"lasthelp", LastHelp, 0, 0},
	{"lasthitter", LastHitter, 0, 0},
	{"lastmarkedobject", LastMarkedObject, 0, 0},
	{"lastseenby", LastSeenBy, 0, 0},
	{"lastsummonerof", LastSummonerOf, 0, 0},
	{"lasttalkedtoby", LastTalkedToBy, 0, 0},
	{"lasttargetedby", LastTargetedBy, 0, 0},
	{"lasttrigger", LastTrigger, 0, 0},
	{"leaderof", LeaderOf, 0, 0},
	{"leastdamagedof", LeastDamagedOf, 0, 0},
	{"marked", LastMarkedObject, 0, 0}, //pst
	{"mostdamagedof", MostDamagedOf, 0, 0},
	{"myself", Myself, 0, 0},
	{"mytarget", MyTarget, 0, 0},//see lasttargetedby(myself)
	{"nearest", Nearest, 0, 0}, //actually this seems broken in IE and resolve as Myself
	{"nearestdoor", NearestDoor, 0, 0},
	{"nearestenemyof", NearestEnemyOf, 0, 0},
	{"nearestenemyoftype", NearestEnemyOfType, 0, 0},
	{"nearestenemysummoned", NearestEnemySummoned, 0, 0},
	{"nearestmygroupoftype", NearestMyGroupOfType, 0, 0},
	{"nearestpc", NearestPC, 0, 0},
	{"ninthnearest", NinthNearest, 0, 0},
	{"ninthnearestdoor", NinthNearestDoor, 0, 0},
	{"ninthnearestenemyof", NinthNearestEnemyOf, 0, 0},
	{"ninthnearestenemyoftype", NinthNearestEnemyOfType, 0, 0},
	{"ninthnearestmygroupoftype", NinthNearestMyGroupOfType, 0, 0},
	{"nothing", Nothing, 0, 0},
	{"player1", Player1, 0, 0},
	{"player1fill", Player1Fill, 0, 0},
	{"player2", Player2, 0, 0},
	{"player2fill", Player2Fill, 0, 0},
	{"player3", Player3, 0, 0},
	{"player3fill", Player3Fill, 0, 0},
	{"player4", Player4, 0, 0},
	{"player4fill", Player4Fill, 0, 0},
	{"player5", Player5, 0, 0},
	{"player5fill", Player5Fill, 0, 0},
	{"player6", Player6, 0, 0},
	{"player6fill", Player6Fill, 0, 0},
	{"player7", Player7, 0, 0},
	{"player7fill", Player7Fill, 0, 0},
	{"player8", Player8, 0, 0},
	{"player8fill", Player8Fill, 0, 0},
	{"protectedby", ProtectedBy, 0, 0},
	{"protectorof", ProtectorOf, 0, 0},
	{"protagonist", Protagonist, 0, 0},
	{"secondnearest", SecondNearest, 0, 0},
	{"secondnearestdoor", SecondNearestDoor, 0, 0},
	{"secondnearestenemyof", SecondNearestEnemyOf, 0, 0},
	{"secondnearestenemyoftype", SecondNearestEnemyOfType, 0, 0},
	{"secondnearestmygroupoftype", SecondNearestMyGroupOfType, 0, 0},
	{"selectedcharacter", SelectedCharacter, 0, 0},
	{"seventhnearest", SeventhNearest, 0, 0},
	{"seventhnearestdoor", SeventhNearestDoor, 0, 0},
	{"seventhnearestenemyof", SeventhNearestEnemyOf, 0, 0},
	{"seventhnearestenemyoftype", SeventhNearestEnemyOfType, 0, 0},
	{"seventhnearestmygroupoftype", SeventhNearestMyGroupOfType, 0, 0},
	{"sixthnearest", SixthNearest, 0, 0},
	{"sixthnearestdoor", SixthNearestDoor, 0, 0},
	{"sixthnearestenemyof", SixthNearestEnemyOf, 0, 0},
	{"sixthnearestenemyoftype", SixthNearestEnemyOfType, 0, 0},
	{"sixthnearestmygroupoftype", SixthNearestMyGroupOfType, 0, 0},
	{"strongestof", StrongestOf, 0, 0},
	{"strongestofmale", StrongestOfMale, 0, 0},
	{"tenthnearest", TenthNearest, 0, 0},
	{"tenthnearestdoor", TenthNearestDoor, 0, 0},
	{"tenthnearestenemyof", TenthNearestEnemyOf, 0, 0},
	{"tenthnearestenemyoftype", TenthNearestEnemyOfType, 0, 0},
	{"tenthnearestmygroupoftype", TenthNearestMyGroupOfType, 0, 0},
	{"thirdnearest", ThirdNearest, 0, 0},
	{"thirdnearestdoor", ThirdNearestDoor, 0, 0},
	{"thirdnearestenemyof", ThirdNearestEnemyOf, 0, 0},
	{"thirdnearestenemyoftype", ThirdNearestEnemyOfType, 0, 0},
	{"thirdnearestmygroupoftype", ThirdNearestMyGroupOfType, 0, 0},
	{"weakestof", WeakestOf, 0, 0},
	{"worstac", WorstAC, 0, 0},
};

void RegisterObjects()
{
	ObjectRegistry.Register(sizeof(objectnames)/sizeof(ObjectDesc), objectnames);
}

#define STATIC_LINK
#include "plugindef.h"

GEMRB_PLUGIN(_, _)
PLUGIN_INITIALIZER(RegisterObjects)
END_PLUGIN()
