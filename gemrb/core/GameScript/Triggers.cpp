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

#include "GameScript/GameScript.h"

#include "GameScript/GSUtils.h"
#include "GameScript/Matching.h"

#include "win32def.h"

#include "Calendar.h"
#include "DialogHandler.h"
#include "Game.h"
#include "GameData.h"
#include "Video.h"
#include "GUI/GameControl.h"
#include "math.h" //needs for acos

//-------------------------------------------------------------
// Trigger Functions
//-------------------------------------------------------------
int BreakingPoint(Scriptable* Sender, Trigger* /*parameters*/)
{
	int value=GetHappiness(Sender, core->GetGame()->Reputation );
	return value < -300;
}

int Reaction(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr || scr->Type != ST_ACTOR) {
		parameters->Dump();
		return 0;
	}
	int value = GetReaction(((Actor*) scr), Sender);
	return value == parameters->int0Parameter;
}

int ReactionGT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr || scr->Type != ST_ACTOR) {
		parameters->Dump();
		return 0;
	}
	int value = GetReaction(((Actor*) scr), Sender);
	return value > parameters->int0Parameter;
}

int ReactionLT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr || scr->Type != ST_ACTOR) {
		parameters->Dump();
		return 0;
	}
	int value = GetReaction(((Actor*) scr), Sender);
	return value < parameters->int0Parameter;
}

int Happiness(Scriptable* Sender, Trigger* parameters)
{
	int value=GetHappiness(Sender, core->GetGame()->Reputation );
	return value == parameters->int0Parameter;
}

int HappinessGT(Scriptable* Sender, Trigger* parameters)
{
	int value=GetHappiness(Sender, core->GetGame()->Reputation );
	return value > parameters->int0Parameter;
}

int HappinessLT(Scriptable* Sender, Trigger* parameters)
{
	int value=GetHappiness(Sender, core->GetGame()->Reputation );
	return value < parameters->int0Parameter;
}

int Reputation(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->Reputation/10 == (ieDword) parameters->int0Parameter;
}

int ReputationGT(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->Reputation/10 > (ieDword) parameters->int0Parameter;
}

int ReputationLT(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->Reputation/10 < (ieDword) parameters->int0Parameter;
}

int Alignment(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr || scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	return GS::ID_Alignment( actor, parameters->int0Parameter);
}

int Allegiance(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr || scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	return GS::ID_Allegiance( actor, parameters->int0Parameter);
}

//should return *_ALL stuff
int Class(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr || scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = (Actor*)scr;
	return GS::ID_Class( actor, parameters->int0Parameter);
}

int ClassEx(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr || scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = (Actor*)scr;
	return GS::ID_AVClass( actor, parameters->int0Parameter);
}

int Faction(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr || scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = (Actor*)scr;
	return GS::ID_Faction( actor, parameters->int0Parameter);
}

int Team(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr || scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = (Actor*)scr;
	return GS::ID_Team( actor, parameters->int0Parameter);
}

int SubRace(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr || scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = (Actor*)scr;
	//subrace trigger uses a weird system, cannot use ID_*
	//return ID_Subrace( actor, parameters->int0Parameter);
	int value = actor->GetStat(IE_SUBRACE);
	if (value) {
		value |= actor->GetStat(IE_RACE)<<16;
	}
	if (value == parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

//if object parameter is given (gemrb) it is used
//otherwise it works on the current object (iwd2)
int IsTeamBitOn(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = Sender;
	if (parameters->objectParameter) {
		scr = GetActorFromObject( Sender, parameters->objectParameter );
	}
	if (!scr || scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = (Actor*)scr;
	if (actor->GetStat(IE_TEAM) & parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int NearbyDialog(Scriptable* Sender, Trigger* parameters)
{
	Actor *target = Sender->GetCurrentArea()->GetActorByDialog(parameters->string0Parameter);
	if ( !target ) {
		return 0;
	}
	return Sender->CanSee(  target, true, GA_NO_DEAD | GA_NO_HIDDEN );
}

//atm this checks for InParty and See, it is unsure what is required
int IsValidForPartyDialog(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		scr = Sender;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor *target = (Actor *) scr;
	//inparty returns -1 if not in party
	if (core->GetGame()->InParty( target )<0) {
		return 0;
	}
	//don't accept parties currently in dialog!
	//this might disturb some modders, but this is the correct behaviour
	//for example the aaquatah dialog in irenicus dungeon depends on it
	GameControl *gc = core->GetGameControl();
	Actor *pc = (Actor *) scr;
	if (pc->GetGlobalID() == gc->dialoghandler->targetID || pc->GetGlobalID()==gc->dialoghandler->speakerID) {
		return 0;
	}

	//don't accept parties with the no interrupt flag
	//this fixes bug #2573808 on gamescript level
	//(still someone has to turn the no interrupt flag off)
	if(!pc->GetDialog(GD_CHECK)) {
		return 0;
	}
	return Sender->CanSee(  target, false, GA_NO_DEAD );
}

int InParty(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr;

	if (parameters->objectParameter) {
		scr = GetActorFromObject( Sender, parameters->objectParameter );
	} else {
		scr = Sender;
	}
	if (!scr || scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor *tar = (Actor *) scr;
	if (core->GetGame()->InParty( tar ) <0) {
		return 0;
	}
	//don't allow dead, don't allow maze and similar effects
	if (tar->ValidTarget(GA_NO_DEAD|GA_NO_HIDDEN)) {
		return 1;
	}
	return 0;
}

int InPartyAllowDead(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr;

	if (parameters->objectParameter) {
		scr = GetActorFromObject( Sender, parameters->objectParameter );
	} else {
		scr = Sender;
	}
	if (!scr || scr->Type != ST_ACTOR) {
		return 0;
	}
	return core->GetGame()->InParty( ( Actor * ) scr ) >= 0 ? 1 : 0;
}

int InPartySlot(Scriptable* Sender, Trigger* parameters)
{
	Actor *actor = core->GetGame()->GetPC(parameters->int0Parameter, false);
	return MatchActor(Sender, actor->GetGlobalID(), parameters->objectParameter);
}

int Exists(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	return 1;
}

int IsAClown(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr || scr->Type!=ST_ACTOR) {
		return 0;
	}
	return 1;
}

int IsGabber(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr || scr->Type!=ST_ACTOR) {
		return 0;
	}
	if (scr->GetGlobalID() == core->GetGameControl()->dialoghandler->speakerID)
		return 1;
	return 0;
}

int IsActive(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (scr->GetInternalFlag()&IF_ACTIVE) {
		return 1;
	}
	return 0;
}

int InTrap(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (scr->GetInternalFlag()&IF_INTRAP) {
		return 1;
	}
	return 0;
}

/* checks if targeted actor is in the specified region
   GemRB allows different regions, referenced by int0Parameter
   The polygons are stored in island<nn>.2da files */
int OnIsland(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	Gem_Polygon *p = GetPolygon2DA(parameters->int0Parameter);
	if (!p) {
		return 0;
	}
	return p->PointIn(scr->Pos);
}

int School(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr || scr->Type!=ST_ACTOR) {
		return 0;
	}
	Actor* actor = (Actor *) scr;
	//only the low 2 bytes count
	//the School values start from 1 to 9 and the first school value is 0x40
	//so this mild hack will do
	if ( actor->GetStat(IE_KIT) == (ieDword) (0x20<<parameters->int0Parameter)) {
		return 1;
	}
	return 0;
}

int Kit(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr || scr->Type!=ST_ACTOR) {
		return 0;
	}
	Actor* actor = (Actor *) scr;

	ieDword kit = actor->GetStat(IE_KIT);
	//TODO: fix baseclass / barbarian confusion

	//IWD2 style kit matching (also used for mage schools)
	if (kit == (ieDword) (parameters->int0Parameter)) {
		return 1;
	}
	//BG2 style kit matching (not needed anymore?), we do it on load
	//kit = (kit>>16)|(kit<<16);
	if ( kit == (ieDword) (parameters->int0Parameter)) {
		return 1;
	}
	return 0;
}

int General(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		scr = Sender;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	if (actor == NULL) {
		return 0;
	}
	return GS::ID_General(actor, parameters->int0Parameter);
}

int Specifics(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		scr = Sender;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	if (actor == NULL) {
		return 0;
	}
	return GS::ID_Specific(actor, parameters->int0Parameter);
}

int BitCheck(Scriptable* Sender, Trigger* parameters)
{
	bool valid=true;

	ieDword value = CheckVariable(Sender, parameters->string0Parameter, &valid );
	if (valid) {
		if ( value & parameters->int0Parameter ) return 1;
	}
	return 0;
}

int BitCheckExact(Scriptable* Sender, Trigger* parameters)
{
	bool valid=true;

	ieDword value = CheckVariable(Sender, parameters->string0Parameter, &valid );
	if (valid) {
		ieDword tmp = (ieDword) parameters->int0Parameter ;
		if ((value & tmp) == tmp) return 1;
	}
	return 0;
}

//BM_OR would make sense only if this trigger changes the value of the variable
//should I do that???
int BitGlobal_Trigger(Scriptable* Sender, Trigger* parameters)
{
	bool valid=true;

	ieDword value = CheckVariable(Sender, parameters->string0Parameter, &valid );
	if (valid) {
		HandleBitMod(value, parameters->int0Parameter, parameters->int1Parameter);
		if (value!=0) return 1;
	}
	return 0;
}

int GlobalOrGlobal_Trigger(Scriptable* Sender, Trigger* parameters)
{
	bool valid=true;

	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, &valid );
	if (valid) {
		if ( value1 ) return 1;
		ieDword value2 = CheckVariable(Sender, parameters->string1Parameter, &valid );
		if (valid) {
			if ( value2 ) return 1;
		}
	}
	return 0;
}

int GlobalAndGlobal_Trigger(Scriptable* Sender, Trigger* parameters)
{
	bool valid=true;

	ieDword value1 = CheckVariable( Sender, parameters->string0Parameter, &valid );
	if (valid && value1) {
		ieDword value2 = CheckVariable( Sender, parameters->string1Parameter, &valid );
		if (valid && value2) return 1;
	}
	return 0;
}

int GlobalBAndGlobal_Trigger(Scriptable* Sender, Trigger* parameters)
{
	bool valid=true;

	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, &valid );
	if (valid) {
		ieDword value2 = CheckVariable(Sender, parameters->string1Parameter, &valid );
		if (valid) {
			if ((value1& value2 ) != 0) return 1;
		}
	}
	return 0;
}

int GlobalBAndGlobalExact(Scriptable* Sender, Trigger* parameters)
{
	bool valid=true;

	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, &valid );
	if (valid) {
		ieDword value2 = CheckVariable(Sender, parameters->string1Parameter, &valid );
		if (valid) {
			if (( value1& value2 ) == value2) return 1;
		}
	}
	return 0;
}

int GlobalBitGlobal_Trigger(Scriptable* Sender, Trigger* parameters)
{
	bool valid=true;

	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, &valid );
	if (valid) {
		ieDword value2 = CheckVariable(Sender, parameters->string1Parameter, &valid );
		if (valid) {
			HandleBitMod( value1, value2, parameters->int1Parameter);
			if (value1!=0) return 1;
		}
	}
	return 0;
}

//no what exactly this trigger would do, defined in iwd2, but never used
//i just assume it sets a global in the trigger block
int TriggerSetGlobal(Scriptable* Sender, Trigger* parameters)
{
	SetVariable( Sender, parameters->string0Parameter, parameters->int0Parameter );
	return 1;
}

//would this function also alter the variable?
int Xor(Scriptable* Sender, Trigger* parameters)
{
	bool valid=true;

	ieDword value = CheckVariable(Sender, parameters->string0Parameter, &valid );
	if (valid) {
		if (( value ^ parameters->int0Parameter ) != 0) return 1;
	}
	return 0;
}

//TODO:
//no sprite is dead for iwd, they use KILL_<name>_CNT
int NumDead(Scriptable* Sender, Trigger* parameters)
{
	ieDword value;

	if (core->HasFeature(GF_HAS_KAPUTZ) ) {
		value = CheckVariable(Sender, parameters->string0Parameter, "KAPUTZ");
	} else {
		ieVariable VariableName;
		snprintf(VariableName, 32, core->GetDeathVarFormat(), parameters->string0Parameter);
		value = CheckVariable(Sender, VariableName, "GLOBAL" );
	}
	return ( value == (ieDword) parameters->int0Parameter );
}

int NumDeadGT(Scriptable* Sender, Trigger* parameters)
{
	ieDword value;

	if (core->HasFeature(GF_HAS_KAPUTZ) ) {
		value = CheckVariable(Sender, parameters->string0Parameter, "KAPUTZ");
	} else {
		ieVariable VariableName;
		snprintf(VariableName, 32, core->GetDeathVarFormat(), parameters->string0Parameter);
		value = CheckVariable(Sender, VariableName, "GLOBAL" );
	}
	return ( value > (ieDword) parameters->int0Parameter );
}

int NumDeadLT(Scriptable* Sender, Trigger* parameters)
{
	ieDword value;

	if (core->HasFeature(GF_HAS_KAPUTZ) ) {
		value = CheckVariable(Sender, parameters->string0Parameter, "KAPUTZ");
	} else {
		ieVariable VariableName;

		snprintf(VariableName, 32, core->GetDeathVarFormat(), parameters->string0Parameter);
		value = CheckVariable(Sender, VariableName, "GLOBAL" );
	}
	return ( value < (ieDword) parameters->int0Parameter );
}

int G_Trigger(Scriptable* Sender, Trigger* parameters)
{
	ieDwordSigned value = CheckVariable(Sender, parameters->string0Parameter, "GLOBAL" );
	return ( value == parameters->int0Parameter );
}

int Global(Scriptable* Sender, Trigger* parameters)
{
	bool valid=true;

	ieDwordSigned value = CheckVariable(Sender, parameters->string0Parameter, &valid );
	if (valid) {
		if ( value == parameters->int0Parameter ) return 1;
	}
	return 0;
}

int GLT_Trigger(Scriptable* Sender, Trigger* parameters)
{
	ieDwordSigned value = CheckVariable(Sender, parameters->string0Parameter,"GLOBAL" );
	return ( value < parameters->int0Parameter );
}

int GlobalLT(Scriptable* Sender, Trigger* parameters)
{
	bool valid=true;

	ieDwordSigned value = CheckVariable(Sender, parameters->string0Parameter, &valid );
	if (valid) {
		if ( value < parameters->int0Parameter ) return 1;
	}
	return 0;
}

int GGT_Trigger(Scriptable* Sender, Trigger* parameters)
{
	ieDwordSigned value = CheckVariable(Sender, parameters->string0Parameter, "GLOBAL" );
	return ( value > parameters->int0Parameter );
}

int GlobalGT(Scriptable* Sender, Trigger* parameters)
{
	bool valid=true;

	ieDwordSigned value = CheckVariable(Sender, parameters->string0Parameter, &valid );
	if (valid) {
		if ( value > parameters->int0Parameter ) return 1;
	}
	return 0;
}

int GlobalLTGlobal(Scriptable* Sender, Trigger* parameters)
{
	bool valid=true;

	ieDwordSigned value1 = CheckVariable(Sender, parameters->string0Parameter, &valid );
	if (valid) {
		ieDwordSigned value2 = CheckVariable(Sender, parameters->string1Parameter, &valid );
		if (valid) {
			if ( value1 < value2 ) return 1;
		}
	}
	return 0;
}

int GlobalGTGlobal(Scriptable* Sender, Trigger* parameters)
{
	bool valid=true;

	ieDwordSigned value1 = CheckVariable(Sender, parameters->string0Parameter, &valid );
	if (valid) {
		ieDwordSigned value2 = CheckVariable(Sender, parameters->string1Parameter, &valid );
		if (valid) {
			if ( value1 > value2 ) return 1;
		}
	}
	return 0;
}

int GlobalsEqual(Scriptable* Sender, Trigger* parameters)
{
	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, "GLOBAL" );
	ieDword value2 = CheckVariable(Sender, parameters->string1Parameter, "GLOBAL" );
	return ( value1 == value2 );
}

int GlobalsGT(Scriptable* Sender, Trigger* parameters)
{
	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, "GLOBAL" );
	ieDword value2 = CheckVariable(Sender, parameters->string1Parameter, "GLOBAL" );
	return ( value1 > value2 );
}

int GlobalsLT(Scriptable* Sender, Trigger* parameters)
{
	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, "GLOBAL" );
	ieDword value2 = CheckVariable(Sender, parameters->string1Parameter, "GLOBAL" );
	return ( value1 < value2 );
}

int LocalsEqual(Scriptable* Sender, Trigger* parameters)
{
	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, "LOCALS" );
	ieDword value2 = CheckVariable(Sender, parameters->string1Parameter, "LOCALS" );
	return ( value1 == value2 );
}

int LocalsGT(Scriptable* Sender, Trigger* parameters)
{
	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, "LOCALS" );
	ieDword value2 = CheckVariable(Sender, parameters->string1Parameter, "LOCALS" );
	return ( value1 > value2 );
}

int LocalsLT(Scriptable* Sender, Trigger* parameters)
{
	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, "LOCALS" );
	ieDword value2 = CheckVariable(Sender, parameters->string1Parameter, "LOCALS" );
	return ( value1 < value2 );
}

int RealGlobalTimerExact(Scriptable* Sender, Trigger* parameters)
{
	bool valid=true;

	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, parameters->string1Parameter, &valid );
	if (valid && value1) {
		ieDword value2 = core->GetGame()->RealTime;
		if ( value1 == value2 ) return 1;
	}
	return 0;
}

int RealGlobalTimerExpired(Scriptable* Sender, Trigger* parameters)
{
	bool valid=true;

	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, parameters->string1Parameter, &valid );
	if (valid && value1) {
		if ( value1 < core->GetGame()->RealTime ) return 1;
	}
	return 0;
}

int RealGlobalTimerNotExpired(Scriptable* Sender, Trigger* parameters)
{
	bool valid=true;

	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, parameters->string1Parameter, &valid );
	if (valid && value1) {
		if ( value1 > core->GetGame()->RealTime ) return 1;
	}
	return 0;
}

int GlobalTimerExact(Scriptable* Sender, Trigger* parameters)
{
	bool valid=true;

	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, parameters->string1Parameter, &valid );
	if (valid) {
		if ( value1 == core->GetGame()->GameTime ) return 1;
	}
	return 0;
}

int GlobalTimerExpired(Scriptable* Sender, Trigger* parameters)
{
	bool valid=true;

	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, parameters->string1Parameter, &valid );
	if (valid && value1) {
		if ( value1 < core->GetGame()->GameTime ) return 1;
	}
	return 0;
}

//globaltimernotexpired returns false if the timer doesn't exist
int GlobalTimerNotExpired(Scriptable* Sender, Trigger* parameters)
{
	bool valid=true;

	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, parameters->string1Parameter, &valid );
	if (valid && value1) {
	 	if ( value1 > core->GetGame()->GameTime ) return 1;
	}
	return 0;
}

//globaltimerstarted returns false if the timer doesn't exist
//is it the same as globaltimernotexpired?
int GlobalTimerStarted(Scriptable* Sender, Trigger* parameters)
{
	bool valid=true;

	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, parameters->string1Parameter, &valid );
	if (valid && value1) {
		if ( value1 > core->GetGame()->GameTime ) return 1;
	}
	return 0;
}

int WasInDialog(Scriptable* Sender, Trigger* /*parameters*/)
{
	if (Sender->GetInternalFlag()&IF_WASINDIALOG) {
		Sender->SetBitTrigger(BT_WASINDIALOG);
		return 1;
	}
	return 0;
}

int OnCreation(Scriptable* Sender, Trigger* /*parameters*/)
{
	if (Sender->GetInternalFlag()&IF_ONCREATION) {
		Sender->SetBitTrigger(BT_ONCREATION);
		return 1;
	}
	return 0;
}

int NumItemsParty(Scriptable* /*Sender*/, Trigger* parameters)
{
	int cnt = 0;
	Game *game=core->GetGame();

	int i = game->GetPartySize(true);
	while(i--) {
		Actor *actor = game->GetPC(i, true);
		cnt+=actor->inventory.CountItems(parameters->string0Parameter,1);
	}
	return cnt==parameters->int0Parameter;
}

int NumItemsPartyGT(Scriptable* /*Sender*/, Trigger* parameters)
{
	int cnt = 0;
	Game *game=core->GetGame();

	int i = game->GetPartySize(true);
	while(i--) {
		Actor *actor = game->GetPC(i, true);
		cnt+=actor->inventory.CountItems(parameters->string0Parameter,1);
	}
	return cnt>parameters->int0Parameter;
}

int NumItemsPartyLT(Scriptable* /*Sender*/, Trigger* parameters)
{
	int cnt = 0;
	Game *game=core->GetGame();

	int i = game->GetPartySize(true);
	while(i--) {
		Actor *actor = game->GetPC(i, true);
		cnt+=actor->inventory.CountItems(parameters->string0Parameter,1);
	}
	return cnt<parameters->int0Parameter;
}

int NumItems(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}

	Inventory *inv = NULL;
	switch (tar->Type) {
		case ST_ACTOR:
			inv = &(((Actor *) tar)->inventory);
			break;
		case ST_CONTAINER:
			inv = &(((Container *) tar)->inventory);
			break;
		default:;
	}
	if (!inv) {
		return 0;
	}

	int cnt = inv->CountItems(parameters->string0Parameter,1);
	return cnt==parameters->int0Parameter;
}

int TotalItemCnt(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if ( !tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) tar;
	int cnt = actor->inventory.CountItems("",1); //shall we count heaps or not?
	return cnt==parameters->int0Parameter;
}

int TotalItemCntExclude(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if ( !tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) tar;
	int cnt = actor->inventory.CountItems("",1)-actor->inventory.CountItems(parameters->string0Parameter,1); //shall we count heaps or not?
	return cnt==parameters->int0Parameter;
}

int NumItemsGT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}

	Inventory *inv = NULL;
	switch (tar->Type) {
		case ST_ACTOR:
			inv = &(((Actor *) tar)->inventory);
			break;
		case ST_CONTAINER:
			inv = &(((Container *) tar)->inventory);
			break;
		default:;
	}
	if (!inv) {
		return 0;
	}

	int cnt = inv->CountItems(parameters->string0Parameter,1);
	return cnt>parameters->int0Parameter;
}

int TotalItemCntGT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if ( !tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) tar;
	int cnt = actor->inventory.CountItems("",1); //shall we count heaps or not?
	return cnt>parameters->int0Parameter;
}

int TotalItemCntExcludeGT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if ( !tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) tar;
	int cnt = actor->inventory.CountItems("",1)-actor->inventory.CountItems(parameters->string0Parameter,1); //shall we count heaps or not?
	return cnt>parameters->int0Parameter;
}

int NumItemsLT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}

	Inventory *inv = NULL;
	switch (tar->Type) {
		case ST_ACTOR:
			inv = &(((Actor *) tar)->inventory);
			break;
		case ST_CONTAINER:
			inv = &(((Container *) tar)->inventory);
			break;
		default:;
	}
	if (!inv) {
		return 0;
	}

	int cnt = inv->CountItems(parameters->string0Parameter,1);
	return cnt<parameters->int0Parameter;
}

int TotalItemCntLT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if ( !tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) tar;
	int cnt = actor->inventory.CountItems("",1); //shall we count heaps or not?
	return cnt<parameters->int0Parameter;
}

int TotalItemCntExcludeLT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if ( !tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) tar;
	int cnt = actor->inventory.CountItems("",1)-actor->inventory.CountItems(parameters->string0Parameter,1); //shall we count heaps or not?
	return cnt<parameters->int0Parameter;
}

//the int0 parameter is an addition, normally it is 0
int Contains(Scriptable* Sender, Trigger* parameters)
{
//actually this should be a container
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if ( !tar || tar->Type!=ST_CONTAINER) {
		return 0;
	}
	Container *cnt = (Container *) tar;
	if (HasItemCore(&cnt->inventory, parameters->string0Parameter, parameters->int0Parameter) ) {
		return 1;
	}
	return 0;
}

int StoreHasItem(Scriptable* /*Sender*/, Trigger* parameters)
{
	return StoreHasItemCore(parameters->string0Parameter, parameters->string1Parameter);
}

//the int0 parameter is an addition, normally it is 0
int HasItem(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if ( !scr ) {
		return 0;
	}
	Inventory *inventory;
	switch (scr->Type) {
		case ST_ACTOR:
			inventory = &( (Actor *) scr)->inventory;
			break;
		case ST_CONTAINER:
			inventory = &( (Container *) scr)->inventory;
			break;
		default:
			inventory = NULL;
			break;
	}
	if (inventory && HasItemCore(inventory, parameters->string0Parameter, parameters->int0Parameter) ) {
		return 1;
	}
	return 0;
}

int ItemIsIdentified(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if ( !scr || scr->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) scr;
	if (HasItemCore(&actor->inventory, parameters->string0Parameter, IE_INV_ITEM_IDENTIFIED) ) {
		return 1;
	}
	return 0;
}

/** if the string is zero, then it will return true if there is any item in the slot (BG2)*/
/** if the string is non-zero, it will return true, if the given item was in the slot (IWD2)*/
int HasItemSlot(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if ( !scr || scr->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) scr;
	//this might require a conversion of the slots
	if (actor->inventory.HasItemInSlot(parameters->string0Parameter, parameters->int0Parameter) ) {
		return 1;
	}
	return 0;
}

//this is a GemRB extension
//HasItemTypeSlot(Object, SLOT, ItemType)
//returns true if the item in SLOT is of ItemType
int HasItemTypeSlot(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if ( !scr || scr->Type!=ST_ACTOR) {
		return 0;
	}
	Inventory *inv = &((Actor *) scr)->inventory;
	if (parameters->int0Parameter>=inv->GetSlotCount()) {
		return 0;
	}
	CREItem *slot = inv->GetSlotItem(parameters->int0Parameter);
	if (!slot) {
		return 0;
	}
	Item *itm = gamedata->GetItem(slot->ItemResRef);
	int itemtype = itm->ItemType;
	gamedata->FreeItem(itm, slot->ItemResRef, 0);
	if (itemtype==parameters->int1Parameter) {
		return 1;
	}
	return 0;
}

int HasItemEquipped(Scriptable * Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if ( !scr || scr->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) scr;
	if (actor->inventory.HasItem(parameters->string0Parameter, IE_INV_ITEM_EQUIPPED) ) {
		return 1;
	}
	return 0;
}

int Acquired(Scriptable * Sender, Trigger* parameters)
{
	if ( Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) Sender;
	if (actor->inventory.HasItem(parameters->string0Parameter, IE_INV_ITEM_ACQUIRED) ) {
		return 1;
	}
	return 0;
}

/** this trigger accepts a numeric parameter, this number is the same as inventory flags
    like: 1 - identified, 2 - unstealable, 4 - stolen, 8 - undroppable, etc. */
/** this is a GemRB extension */
int PartyHasItem(Scriptable * /*Sender*/, Trigger* parameters)
{
	Game *game=core->GetGame();

	int i = game->GetPartySize(true);
	while(i--) {
		Actor *actor = game->GetPC(i, true);
		if (HasItemCore(&actor->inventory, parameters->string0Parameter, parameters->int0Parameter) ) {
			return 1;
		}
	}
	return 0;
}

int PartyHasItemIdentified(Scriptable * /*Sender*/, Trigger* parameters)
{
	Game *game=core->GetGame();

	int i = game->GetPartySize(true);
	while(i--) {
		Actor *actor = game->GetPC(i, true);
		if (HasItemCore(&actor->inventory, parameters->string0Parameter, IE_INV_ITEM_IDENTIFIED) ) {
			return 1;
		}
	}
	return 0;
}

int InventoryFull( Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) tar;
	if (actor->inventory.FindCandidateSlot( SLOT_INVENTORY, 0 )==-1) {
		return 1;
	}
	return 0;
}

int HasInnateAbility(Scriptable *Sender, Trigger *parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) tar;
	if (parameters->string0Parameter[0]) {
		return actor->spellbook.HaveSpell(parameters->string0Parameter, 0);
	}
	return actor->spellbook.HaveSpell(parameters->int0Parameter, 0);
}

int HaveSpell(Scriptable *Sender, Trigger *parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) Sender;
	if (parameters->string0Parameter[0]) {
		return actor->spellbook.HaveSpell(parameters->string0Parameter, 0);
	}
	return actor->spellbook.HaveSpell(parameters->int0Parameter, 0);
}

int HaveAnySpells(Scriptable* Sender, Trigger* /*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) Sender;
	return actor->spellbook.HaveSpell("", 0);
}

int HaveSpellParty(Scriptable* /*Sender*/, Trigger *parameters)
{
	Game *game=core->GetGame();

	int i = game->GetPartySize(true);

	if (parameters->string0Parameter[0]) {
		while(i--) {
			Actor *actor = game->GetPC(i, true);
			if (actor->spellbook.HaveSpell(parameters->string0Parameter, 0) ) {
				return 1;
			}
		}
	} else {
		while(i--) {
			Actor *actor = game->GetPC(i, true);
			if (actor->spellbook.HaveSpell(parameters->int0Parameter, 0) ) {
				return 1;
			}
		}
	}
	return 0;
}

int KnowSpell(Scriptable *Sender, Trigger *parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) Sender;
	if (parameters->string0Parameter[0]) {
		return actor->spellbook.KnowSpell(parameters->string0Parameter);
	}
	return actor->spellbook.KnowSpell(parameters->int0Parameter);
}

int True(Scriptable * /* Sender*/, Trigger * /*parameters*/)
{
	return 1;
}

//in fact this could be used only on Sender, but we want to enhance these
//triggers and actions to accept an object argument whenever possible.
//0 defaults to Myself (Sender)
int NumTimesTalkedTo(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		scr = Sender;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	return actor->TalkCount == (ieDword) parameters->int0Parameter ? 1 : 0;
}

int NumTimesTalkedToGT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		scr = Sender;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	return actor->TalkCount > (ieDword) parameters->int0Parameter ? 1 : 0;
}

int NumTimesTalkedToLT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		scr = Sender;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	return actor->TalkCount < (ieDword) parameters->int0Parameter ? 1 : 0;
}

int NumTimesInteracted(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		scr = Sender;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	ieDword npcid = parameters->int0Parameter;
	if (npcid>=MAX_INTERACT) return 0;
	if (!actor->PCStats) return 0;
	return actor->PCStats->Interact[npcid] == (ieDword) parameters->int1Parameter ? 1 : 0;
}

int NumTimesInteractedGT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		scr = Sender;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	ieDword npcid = parameters->int0Parameter;
	if (npcid>=MAX_INTERACT) return 0;
	if (!actor->PCStats) return 0;
	return actor->PCStats->Interact[npcid] > (ieDword) parameters->int1Parameter ? 1 : 0;
}

int NumTimesInteractedLT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		scr = Sender;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	ieDword npcid = parameters->int0Parameter;
	if (npcid>=MAX_INTERACT) return 0;
	if (!actor->PCStats) return 0;
	return actor->PCStats->Interact[npcid] < (ieDword) parameters->int1Parameter ? 1 : 0;
}

//GemRB specific
//interacting npc counts were restricted to 24
//gemrb will increase a local variable in the interacting npc, with the scriptname of the
//target npc
int NumTimesInteractedObject(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return 0;
	}

	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* tar = ( Actor* ) scr;
	return CheckVariable(Sender, tar->GetScriptName(), "LOCALS") == (ieDword) parameters->int0Parameter ? 1 : 0;
}

int NumTimesInteractedObjectGT(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return 0;
	}

	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* tar = ( Actor* ) scr;
	return CheckVariable(Sender, tar->GetScriptName(), "LOCALS") > (ieDword) parameters->int0Parameter ? 1 : 0;
}

int NumTimesInteractedObjectLT(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return 0;
	}

	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* tar = ( Actor* ) scr;
	return CheckVariable(Sender, tar->GetScriptName(), "LOCALS") < (ieDword) parameters->int0Parameter ? 1 : 0;
}

int ObjectActionListEmpty(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}

	// added CurrentAction as part of blocking action fixes
	if (scr->GetCurrentAction() || scr->GetNextAction()) {
		return 0;
	}
	return 1;
}

int ActionListEmpty(Scriptable* Sender, Trigger* /*parameters*/)
{
	// added CurrentAction as part of blocking action fixes
	if (Sender->GetCurrentAction() || Sender->GetNextAction()) {
		return 0;
	}
	return 1;
}

int GS::False(Scriptable* /*Sender*/, Trigger* /*parameters*/)
{
	return 0;
}

/* i guess this is a range of circle edges (instead of centers) */
int PersonalSpaceDistance(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	int range = parameters->int0Parameter;

	int distance = PersonalDistance(Sender, scr);
	if (distance <= ( range * 10 )) {
		return 1;
	}
	return 0;
}

int Range(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	int distance = SquaredMapDistance(Sender, scr);
	return DiffCore(distance, (parameters->int0Parameter+1)*(parameters->int0Parameter+1), parameters->int1Parameter);
}

int InLine(Scriptable* Sender, Trigger* parameters)
{
	Map *map = Sender->GetCurrentArea();
	if (!map) {
		return 0;
	}

	Scriptable* scr1 = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr1) {
		return 0;
	}

	//looking for a scriptable by scriptname only
	Scriptable* scr2 = map->GetActor( parameters->string0Parameter, 0 );
	if (!scr2) {
		scr2 = GetActorObject(map->GetTileMap(), parameters->string0Parameter);
	}  
	if (!scr2) {
		return 0;
	}

	double fdm1 = SquaredDistance(Sender, scr1);
	double fdm2 = SquaredDistance(Sender, scr2);
	double fd12 = SquaredDistance(scr1, scr2);
	double dm1 = sqrt(fdm1);
	double dm2 = sqrt(fdm2);

	if (fdm1>fdm2 || fd12>fdm2) {
		return 0;
	}
	double angle = acos(( fdm2 + fdm1 - fd12 ) / (2*dm1*dm2));
	if (angle*180.0*M_PI<30.0) return 1;
	return 0;
}

//PST
int AtLocation( Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if ( (tar->Pos.x==parameters->pointParameter.x) &&
		(tar->Pos.y==parameters->pointParameter.y) ) {
		return 1;
	}
	return 0;
}

//in pst this is a point
//in iwd2 this is not a point
int NearLocation(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (parameters->pointParameter.isnull()) {
		Point p((short) parameters->int0Parameter, (short) parameters->int1Parameter);
		int distance = PersonalDistance(p, scr);
		if (distance <= ( parameters->int2Parameter * 10 )) {
			return 1;
		}
		return 0;
	}
	//personaldistance is needed for modron constructs in PST maze
	int distance = PersonalDistance(parameters->pointParameter, scr);
	if (distance <= ( parameters->int0Parameter * 10 )) {
		return 1;
	}
	return 0;
}

int NearSavedLocation(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	if (core->HasFeature(GF_HAS_KAPUTZ)) {
		// we don't understand how this works in pst yet
		return 1;
	}
	Actor *actor = (Actor *) Sender;
	Point p( (short) actor->GetStat(IE_SAVEDXPOS), (short) actor->GetStat(IE_SAVEDYPOS) );
	// should this be PersonalDistance?
	int distance = Distance(p, Sender);
	if (distance <= ( parameters->int0Parameter * 10 )) {
		return 1;
	}
	return 0;
}

int Or(Scriptable* /*Sender*/, Trigger* parameters)
{
	return parameters->int0Parameter;
}

int TriggerTrigger(Scriptable* Sender, Trigger* parameters)
{
	if(Sender->TriggerID==(ieDword) parameters->int0Parameter) {
		Sender->AddTrigger (&Sender->TriggerID);
		return 1;
	}
	return 0;
}

int WalkedToTrigger(Scriptable* Sender, Trigger* parameters)
{
	Actor *target = Sender->GetCurrentArea()->GetActorByGlobalID(Sender->LastTrigger);
	if (!target) {
		return 0;
	}
	if (PersonalDistance(target, Sender) > 3*MAX_OPERATING_DISTANCE ) {
		return 0;
	}
	//now objects suicide themselves if they are empty objects
	//so checking an empty object is easier
	if (parameters->objectParameter == NULL) {
		Sender->AddTrigger (&Sender->LastTrigger);
		return 1;
	}
	if (MatchActor(Sender, Sender->LastTrigger, parameters->objectParameter)) {
		Sender->AddTrigger (&Sender->LastTrigger);
		return 1;
	}
	return 0;
}

int Clicked(Scriptable* Sender, Trigger* parameters)
{
	//now objects suicide themselves if they are empty objects
	//so checking an empty object is easier
	if (parameters->objectParameter == NULL) {
		if (Sender->LastTrigger) {
			Sender->AddTrigger (&Sender->LastTrigger);
			return 1;
		}
		return 0;
	}
	if (MatchActor(Sender, Sender->LastTrigger, parameters->objectParameter)) {
		Sender->AddTrigger (&Sender->LastTrigger);
		return 1;
	}
	return 0;
}

int Disarmed(Scriptable* Sender, Trigger* parameters)
{
	switch(Sender->Type) {
		case ST_DOOR: case ST_CONTAINER: case ST_PROXIMITY:
			break;
		default:
			return 0;
	}
	if (parameters->objectParameter == NULL) {
		if (Sender->LastDisarmed) {
			Sender->AddTrigger (&Sender->LastDisarmed);
			return 1;
		}
		return 0;
	}
	if (MatchActor(Sender, Sender->LastDisarmed, parameters->objectParameter)) {
		Sender->AddTrigger (&Sender->LastDisarmed);
		return 1;
	}
	return 0;
}

//stealing from a store failed, owner triggered
int StealFailed(Scriptable* Sender, Trigger* parameters)
{
	switch(Sender->Type) {
		case ST_ACTOR:
			break;
		default:
			return 0;
	}
	// maybe check if Sender is a shopkeeper???

	if (parameters->objectParameter == NULL) {
		if (Sender->LastDisarmFailed) {
			Sender->AddTrigger (&Sender->LastDisarmFailed);
			return 1;
		}
		return 0;
	}
	if (MatchActor(Sender, Sender->LastDisarmFailed, parameters->objectParameter)) {
		Sender->AddTrigger (&Sender->LastDisarmFailed);
		return 1;
	}
	return 0;
}

int PickpocketFailed(Scriptable* Sender, Trigger* parameters)
{
	switch(Sender->Type) {
		case ST_ACTOR:
			break;
		default:
			return 0;
	}
	if (parameters->objectParameter == NULL) {
		if (Sender->LastOpenFailed) {
			Sender->AddTrigger (&Sender->LastOpenFailed);
			return 1;
		}
		return 0;
	}
	if (MatchActor(Sender, Sender->LastOpenFailed, parameters->objectParameter)) {
		Sender->AddTrigger (&Sender->LastOpenFailed);
		return 1;
	}
	return 0;
}

int PickLockFailed(Scriptable* Sender, Trigger* parameters)
{
	switch(Sender->Type) {
		case ST_DOOR: case ST_CONTAINER:
			break;
		default:
			return 0;
	}
	if (parameters->objectParameter == NULL) {
		if (Sender->LastPickLockFailed) {
			Sender->AddTrigger (&Sender->LastPickLockFailed);
			return 1;
		}
		return 0;
	}
	if (MatchActor(Sender, Sender->LastPickLockFailed, parameters->objectParameter)) {
		Sender->AddTrigger (&Sender->LastPickLockFailed);
		return 1;
	}
	return 0;
}

int OpenFailed(Scriptable* Sender, Trigger* parameters)
{
	switch(Sender->Type) {
		case ST_DOOR: case ST_CONTAINER:
			break;
		default:
			return 0;
	}
	if (parameters->objectParameter == NULL) {
		if (Sender->LastOpenFailed) {
			Sender->AddTrigger (&Sender->LastOpenFailed);
			return 1;
		}
		return 0;
	}
	if (MatchActor(Sender, Sender->LastOpenFailed, parameters->objectParameter
)) {
		Sender->AddTrigger (&Sender->LastOpenFailed);
		return 1;
	}
	return 0;
}

int DisarmFailed(Scriptable* Sender, Trigger* parameters)
{
	switch(Sender->Type) {
		case ST_DOOR: case ST_CONTAINER: case ST_PROXIMITY:
			break;
		default:
			return 0;
	}
	if (parameters->objectParameter == NULL) {
		if (Sender->LastDisarmFailed) {
			Sender->AddTrigger (&Sender->LastDisarmFailed);
			return 1;
		}
		return 0;
	}
	if (MatchActor(Sender, Sender->LastDisarmFailed, parameters->objectParameter)) {
		Sender->AddTrigger (&Sender->LastDisarmFailed);
		return 1;
	}
	return 0;
}

//opened for doors/containers (using lastEntered)
int Opened(Scriptable* Sender, Trigger* parameters)
{
	Door *door;

	switch (Sender->Type) {
		case ST_DOOR:
			door = (Door *) Sender;
			if (!door->IsOpen()) {
				return 0;
			}
			break;
		case ST_CONTAINER:
			break;
		default:
			return 0;
	}

	if (parameters->objectParameter == NULL) {
		if (Sender->LastEntered) {
			Sender->AddTrigger (&Sender->LastEntered);
			return 1;
		}
		return 0;
	}
	if (MatchActor(Sender, Sender->LastEntered, parameters->objectParameter)) {
		Sender->AddTrigger (&Sender->LastEntered);
		return 1;
	}
	return 0;
}

//closed for doors (using lastTrigger)
int Closed(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type != ST_DOOR) {
		return 0;
	}
	Door *door = (Door *) Sender;
	if (door->IsOpen()) {
		return 0;
	}

	if (parameters->objectParameter == NULL) {
		if (Sender->LastTrigger) {
			Sender->AddTrigger (&Sender->LastTrigger);
			return 1;
		}
		return 0;
	}
	if (MatchActor(Sender, Sender->LastTrigger, parameters->objectParameter)) {
		Sender->AddTrigger (&Sender->LastTrigger);
		return 1;
	}
	return 0;
}

//unlocked for doors/containers (using lastUnlocked)
int Unlocked(Scriptable* Sender, Trigger* parameters)
{
	Door *door;

	switch (Sender->Type) {
		case ST_DOOR:
			door = (Door *) Sender;
			if ((door->Flags&DOOR_LOCKED) ) {
				return 0;
			}
			break;
		case ST_CONTAINER:
			break;
		default:
			return 0;
	}

	if (parameters->objectParameter == NULL) {
		if (Sender->LastUnlocked) {
			Sender->AddTrigger (&Sender->LastUnlocked);
			return 1;
		}
		return 0;
	}
	if (MatchActor(Sender, Sender->LastUnlocked, parameters->objectParameter)) {
		Sender->AddTrigger (&Sender->LastUnlocked);
		return 1;
	}
	return 0;
}

int Entered(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type != ST_PROXIMITY) {
		return 0;
	}
	InfoPoint *ip = (InfoPoint *) Sender;
	if (!ip->Trapped) {
		return 0;
	}

	if (parameters->objectParameter == NULL) {
		if (Sender->LastEntered) {
			Sender->AddTrigger (&Sender->LastEntered);
			return 1;
		}
		return 0;
	}
	if (MatchActor(Sender, Sender->LastEntered, parameters->objectParameter)) {
		Sender->AddTrigger (&Sender->LastEntered);
		return 1;
	}
	return 0;
}

int HarmlessEntered(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type != ST_PROXIMITY) {
		return 0;
	}
	if (parameters->objectParameter == NULL) {
		if (Sender->LastEntered) {
			Sender->AddTrigger (&Sender->LastEntered);
			return 1;
		}
		return 0;
	}
	if (MatchActor(Sender, Sender->LastEntered, parameters->objectParameter)) {
		Sender->AddTrigger (&Sender->LastEntered);
		return 1;
	}
	return 0;
}

int IsOverMe(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type != ST_PROXIMITY) {
		return 0;
	}
	Highlightable *trap = (Highlightable *)Sender;

	Targets *tgts = GetAllObjects(Sender->GetCurrentArea(), Sender, parameters->objectParameter, GA_NO_DEAD);
	int ret = 0;
	if (tgts) {
		targetlist::iterator m;
		const targettype *tt = tgts->GetFirstTarget(m, ST_ACTOR);
		while (tt) {
			Actor *actor = (Actor *) tt->actor;
			if (trap->IsOver(actor->Pos)) {
				ret = 1;
				break;
			}
			tt = tgts->GetNextTarget(m, ST_ACTOR);
		}
	}
	delete tgts;
	return ret;
}

//this function is different in every engines, if you use a string0parameter
//then it will be considered as a variable check
//you can also use an object parameter (like in iwd)
int Dead(Scriptable* Sender, Trigger* parameters)
{
	if (parameters->string0Parameter[0]) {
		ieDword value;
		ieVariable Variable;

		if (core->HasFeature( GF_HAS_KAPUTZ )) {
			value = CheckVariable( Sender, parameters->string0Parameter, "KAPUTZ");
		} else {
			snprintf( Variable, 32, core->GetDeathVarFormat(), parameters->string0Parameter );
		}
		value = CheckVariable( Sender, Variable, "GLOBAL" );
		if (value>0) {
			return 1;
		}
		return 0;
	}
	Scriptable* target = GetActorFromObject( Sender, parameters->objectParameter );
	if (!target) {
		return 1;
	}
	if (target->Type != ST_ACTOR) {
		return 1;
	}
	Actor* actor = ( Actor* ) target;
	if (actor->GetStat( IE_STATE_ID ) & STATE_DEAD) {
		return 1;
	}
	return 0;
}

int CreatureHidden(Scriptable* Sender, Trigger* /*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *act=(Actor *) Sender;

	//this stuff is not completely clear, but HoW has a flag for this
	//and GemRB uses the avatarremoval stat for it.
	//HideCreature also sets this stat, so...
	if (act->GetStat(IE_AVATARREMOVAL)) {
		return 1;
	}

	if (act->GetInternalFlag()&IF_VISIBLE) {
		return 0;
	}
	return 1;
}
int BecameVisible(Scriptable* Sender, Trigger* /*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *act=(Actor *) Sender;
	if (act->GetInternalFlag()&IF_BECAMEVISIBLE) {
		//set trigger to erase
		act->SetBitTrigger(BT_BECAMEVISIBLE);
		return 1;
	}
	return 0;
}

int Die(Scriptable* Sender, Trigger* /*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *act=(Actor *) Sender;
	if (act->GetInternalFlag()&IF_JUSTDIED) {
		//set trigger to erase
		act->SetBitTrigger(BT_DIE);
		return 1;
	}
	return 0;
}

int Died(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *act=(Actor *) tar;
	if (act->GetInternalFlag()&IF_JUSTDIED) {
		//set trigger to erase
		act->SetBitTrigger(BT_DIE);
		return 1;
	}
	return 0;
}

int PartyMemberDied(Scriptable* /*Sender*/, Trigger* /*parameters*/)
{
	Game *game = core->GetGame();
	int i = game->PartyMemberDied();
	if (i==-1) {
		return 0;
	}
	//set trigger to erase
	game->GetPC(i,false)->SetBitTrigger(BT_DIE);
	return 1;
}

int NamelessBitTheDust(Scriptable* /*Sender*/, Trigger* /*parameters*/)
{
	Actor* actor = core->GetGame()->GetPC(0, false);
	if (actor->GetInternalFlag()&IF_JUSTDIED) {
		//set trigger to clear
		actor->SetBitTrigger(BT_DIE);
		return 1;
	}
	return 0;
}

int Race(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	return GS::ID_Race(actor, parameters->int0Parameter);
}

int Gender(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	return GS::ID_Gender(actor, parameters->int0Parameter);
}

int HP(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	if ((signed) actor->GetBase( IE_HITPOINTS ) == parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int HPGT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	if ( (signed) actor->GetBase( IE_HITPOINTS ) > parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int HPLT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	if ( (signed) actor->GetBase( IE_HITPOINTS ) < parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

//these triggers work on the current damage (not the last damage)
/* they are identical to HPLost
int DamageTaken(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) Sender;
	int damage = actor->GetStat(IE_MAXHITPOINTS)-actor->GetBase(IE_HITPOINTS);
	if (damage==(int) parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int DamageTakenGT(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) Sender;
	int damage = actor->GetStat(IE_MAXHITPOINTS)-actor->GetBase(IE_HITPOINTS);
	if (damage>(int) parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int DamageTakenLT(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) Sender;
	int damage = actor->GetStat(IE_MAXHITPOINTS)-actor->GetBase(IE_HITPOINTS);
	if (damage<(int) parameters->int0Parameter) {
		return 1;
	}
	return 0;
}
*/

int HPLost(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	//max-current
	if ( (signed) actor->GetStat(IE_MAXHITPOINTS)-(signed) actor->GetBase( IE_HITPOINTS ) == (signed) parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int HPLostGT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	//max-current
	if ( (signed) actor->GetStat(IE_MAXHITPOINTS)-(signed) actor->GetBase( IE_HITPOINTS ) > (signed) parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int HPLostLT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	//max-current
	if ( (signed) actor->GetStat(IE_MAXHITPOINTS)-(signed) actor->GetBase( IE_HITPOINTS ) < (signed) parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int HPPercent(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (GetHPPercent( scr ) == parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int HPPercentGT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (GetHPPercent( scr ) > parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int HPPercentLT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (GetHPPercent( scr ) < parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int XP(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	if (actor->GetStat( IE_XP ) == (unsigned) parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int XPGT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	if (actor->GetStat( IE_XP ) > (unsigned) parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int XPLT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	if (actor->GetStat( IE_XP ) < (unsigned) parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int CheckSkill(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* target = GetActorFromObject( Sender, parameters->objectParameter );
	if (!target) {
		return 0;
	}
	if (target->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) target;
	int sk = actor->GetSkill( parameters->int1Parameter );
	if (sk<0) return 0;
	if ( sk == parameters->int0Parameter) {
		return 1;
	}
	return 0;
}
int CheckStat(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* target = GetActorFromObject( Sender, parameters->objectParameter );
	if (!target) {
		return 0;
	}
	if (target->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) target;
	if ( (signed) actor->GetStat( parameters->int1Parameter ) == parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int CheckSkillGT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	int sk = actor->GetSkill( parameters->int1Parameter );
	if (sk<0) return 0;
	if ( sk > parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int CheckStatGT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	if ( (signed) actor->GetStat( parameters->int1Parameter ) > parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int CheckSkillLT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	int sk = actor->GetSkill( parameters->int1Parameter );
	if (sk<0) return 0;
	if ( sk < parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int CheckStatLT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	if ( (signed) actor->GetStat( parameters->int1Parameter ) < parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

/* i believe this trigger is the same as 'MarkObject' action
 except that if it cannot set the marked object, it returns false */
int SetLastMarkedObject(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return 0;
	}
	Actor *scr = (Actor *) Sender;

	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	scr->LastMarked = tar->GetGlobalID();
	return 1;
}

int IsSpellTargetValid(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return 0;
	}
	Actor *scr = (Actor *) Sender;

	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	Actor *actor = NULL;
	if (tar->Type == ST_ACTOR) {
		actor = (Actor *) tar;
	}

	int flags = parameters->int1Parameter;
	if (!(flags & MSO_IGNORE_NULL) && !actor) {
		return 0;
	}
	if (!(flags & MSO_IGNORE_INVALID) && actor && actor->InvalidSpellTarget() ) {
		return 0;
	}
	int splnum = parameters->int0Parameter;
	if (!(flags & MSO_IGNORE_HAVE) && !scr->spellbook.HaveSpell(splnum, 0) ) {
		return 0;
	}
	int range;
	if ((flags & MSO_IGNORE_RANGE) || !actor) {
		range = 0;
	} else {
		range = Distance(scr, actor);
	}
	if (!(flags & MSO_IGNORE_INVALID) && actor->InvalidSpellTarget(splnum, scr, range)) {
		return 0;
	}
	return 1;
}

//This trigger seems to always return true for actors...
//Always manages to set spell to 0, otherwise it sets if there was nothing set earlier
int SetMarkedSpell_Trigger(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return 0;
	}
	Actor *scr = (Actor *) Sender;
	if (parameters->int0Parameter) {
		if (scr->LastMarkedSpell) {
			return 1;
		}
		if (!scr->spellbook.HaveSpell(parameters->int0Parameter, 0) ) {
			return 1;
		}
	}

	//TODO: check if spell exists (not really important)
	scr->LastMarkedSpell = parameters->int0Parameter;
	return 1;
}

int ForceMarkedSpell_Trigger(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return 0;
	}
	Actor *scr = (Actor *) Sender;
	scr->LastMarkedSpell = parameters->int0Parameter;
	return 1;
}

int IsMarkedSpell(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return 0;
	}
	Actor *scr = (Actor *) Sender;
	return scr->LastMarkedSpell == parameters->int0Parameter;
}


int See(Scriptable* Sender, Trigger* parameters)
{
	int see = SeeCore(Sender, parameters, 0);
	//don't mark LastSeen for clear!!!
	if (Sender->Type==ST_ACTOR && see) {
		Actor *act = (Actor *) Sender;
		//save lastseen as lastmarked
		act->LastMarked = act->LastSeen;
		//Sender->AddTrigger (&act->LastSeen);
	}
	return see;
}

int Detect(Scriptable* Sender, Trigger* parameters)
{
	parameters->int0Parameter=1; //seedead/invis
	int see = SeeCore(Sender, parameters, 0);
	if (!see) {
		return 0;
	}
	return 1;
}

int LOS(Scriptable* Sender, Trigger* parameters)
{
	int see=SeeCore(Sender, parameters, 1);
	if (!see) {
		return 0;
	}
	return Range(Sender, parameters); //same as range
}

int NumCreatures(Scriptable* Sender, Trigger* parameters)
{
	int value = GetObjectCount(Sender, parameters->objectParameter);
	return value == parameters->int0Parameter;
}

int NumCreaturesAtMyLevel(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return 0;
	}
	int level = ((Actor *) Sender)->GetXPLevel(true);
	int value;

	if (parameters->int0Parameter) {
		value = GetObjectLevelCount(Sender, parameters->objectParameter);
	} else {
		value = GetObjectCount(Sender, parameters->objectParameter);
	}
	return value == level;
}

int NumCreaturesLT(Scriptable* Sender, Trigger* parameters)
{
	int value = GetObjectCount(Sender, parameters->objectParameter);
	return value < parameters->int0Parameter;
}

int NumCreaturesLTMyLevel(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return 0;
	}
	int level = ((Actor *) Sender)->GetXPLevel(true);
	int value;

	if (parameters->int0Parameter) {
		value = GetObjectLevelCount(Sender, parameters->objectParameter);
	} else {
		value = GetObjectCount(Sender, parameters->objectParameter);
	}
	return value < level;
}

int NumCreaturesGT(Scriptable* Sender, Trigger* parameters)
{
	int value = GetObjectCount(Sender, parameters->objectParameter);
	return value > parameters->int0Parameter;
}

int NumCreaturesGTMyLevel(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return 0;
	}
	int level = ((Actor *) Sender)->GetXPLevel(true);
	int value;

	if (parameters->int0Parameter) {
		value = GetObjectLevelCount(Sender, parameters->objectParameter);
	} else {
		value = GetObjectCount(Sender, parameters->objectParameter);
	}
	return value > level;
}

int NumCreatureVsParty(Scriptable* Sender, Trigger* parameters)
{
	//creating object on the spot
	if (!parameters->objectParameter) {
		parameters->objectParameter = new Object();
	}
	int value = GetObjectCount(Sender, parameters->objectParameter);
	value -= core->GetGame()->GetPartySize(true);
	return value == parameters->int0Parameter;
}

int NumCreatureVsPartyGT(Scriptable* Sender, Trigger* parameters)
{
	if (!parameters->objectParameter) {
		parameters->objectParameter = new Object();
	}
	int value = GetObjectCount(Sender, parameters->objectParameter);
	value -= core->GetGame()->GetPartySize(true);
	return value > parameters->int0Parameter;
}

int NumCreatureVsPartyLT(Scriptable* Sender, Trigger* parameters)
{
	if (!parameters->objectParameter) {
		parameters->objectParameter = new Object();
	}
	int value = GetObjectCount(Sender, parameters->objectParameter);
	value -= core->GetGame()->GetPartySize(true);
	return value < parameters->int0Parameter;
}

int Morale(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return (signed) actor->GetStat(IE_MORALEBREAK) == parameters->int0Parameter;
}

int MoraleGT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return (signed) actor->GetStat(IE_MORALEBREAK) > parameters->int0Parameter;
}

int MoraleLT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return (signed) actor->GetStat(IE_MORALEBREAK) < parameters->int0Parameter;
}

int CheckSpellState(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	if (parameters->int0Parameter>255) {
		return 0;
	}
	unsigned int position = parameters->int0Parameter>>5;
	unsigned int bit = 1<<(parameters->int0Parameter&31);
	if (actor->GetStat(IE_SPLSTATE_ID1+position) & bit) {
		return 1;
	}
	return 0;
}

int StateCheck(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	if (actor->GetStat(IE_STATE_ID) & parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int ExtendedStateCheck(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	if (actor->GetStat(IE_EXTSTATE_ID) & parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int NotStateCheck(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	if (actor->GetStat(IE_STATE_ID) & ~parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int RandomNum(Scriptable* /*Sender*/, Trigger* parameters)
{
	if (parameters->int0Parameter<0) {
		return 0;
	}
	if (parameters->int1Parameter<0) {
		return 0;
	}
	return parameters->int1Parameter-1 == RandomNumValue%parameters->int0Parameter;
}

int RandomNumGT(Scriptable* /*Sender*/, Trigger* parameters)
{
	if (parameters->int0Parameter<0) {
		return 0;
	}
	if (parameters->int1Parameter<0) {
		return 0;
	}
	return parameters->int1Parameter-1 < RandomNumValue%parameters->int0Parameter;
}

int RandomNumLT(Scriptable* /*Sender*/, Trigger* parameters)
{
	if (parameters->int0Parameter<0) {
		return 0;
	}
	if (parameters->int1Parameter<0) {
		return 0;
	}
	return parameters->int1Parameter-1 > RandomNumValue%parameters->int0Parameter;
}

int OpenState(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		if (InDebug&ID_TRIGGERS) {
			printMessage("GameScript"," ",LIGHT_RED);
			printf("Couldn't find door/container:%s\n", parameters->objectParameter? parameters->objectParameter->objectName:"<NULL>");
			printf("Sender: %s\n", Sender->GetScriptName() );
		}
		return 0;
	}
	switch(tar->Type) {
		case ST_DOOR:
		{
			Door *door =(Door *) tar;
			return !door->IsOpen() == !parameters->int0Parameter;
		}
		case ST_CONTAINER:
		{
			Container *cont = (Container *) tar;
			return !(cont->Flags&CONT_LOCKED) == !parameters->int0Parameter;
		}
		default:; //to remove a warning
	}
	printMessage("GameScript"," ",LIGHT_RED);
	printf("Not a door/container:%s\n", tar->GetScriptName());
	return 0;
}

int IsLocked(Scriptable * Sender, Trigger *parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		printMessage("GameScript"," ",LIGHT_RED);
		printf("Couldn't find door/container:%s\n", parameters->objectParameter? parameters->objectParameter->objectName:"<NULL>");
		printf("Sender: %s\n", Sender->GetScriptName() );
		return 0;
	}
	switch(tar->Type) {
		case ST_DOOR:
		{
			Door *door =(Door *) tar;
			return !!(door->Flags&DOOR_LOCKED);
		}
		case ST_CONTAINER:
		{
			Container *cont = (Container *) tar;
			return !!(cont->Flags&CONT_LOCKED);
		}
		default:; //to remove a warning
	}
	printMessage("GameScript"," ",LIGHT_RED);
	printf("Not a door/container:%s\n", tar->GetScriptName());
	return 0;
}

int Level(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	// FIXME: what about multiclasses or dualclasses?
	return actor->GetStat(IE_LEVEL) == (unsigned) parameters->int0Parameter;
}

//this is just a hack, actually multiclass should be available
int ClassLevel(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;

	if (!GS::ID_Class( actor, parameters->int0Parameter) )
		return 0;
	// FIXME: compare the requested level
	return actor->GetStat(IE_LEVEL) == (unsigned) parameters->int1Parameter;
}

// iwd2 and pst have different order of parameters:
// ClassLevelGT(Protagonist,MAGE,89)
// LevelInClass(Myself,10,CLERIC)
int LevelInClass(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;

	if (!GS::ID_ClassMask( actor, parameters->int1Parameter) )
		return 0;
	// FIXME: compare the requested level
	return actor->GetStat(IE_LEVEL) == (unsigned) parameters->int0Parameter;
}

int LevelGT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return actor->GetStat(IE_LEVEL) > (unsigned) parameters->int0Parameter;
}

//this is just a hack, actually multiclass should be available
int ClassLevelGT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	if (!GS::ID_Class( actor, parameters->int0Parameter) )
		return 0;
	return actor->GetStat(IE_LEVEL) > (unsigned) parameters->int1Parameter;
}

int LevelInClassGT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;

	if (!GS::ID_ClassMask( actor, parameters->int1Parameter) )
		return 0;
	return actor->GetStat(IE_LEVEL) > (unsigned) parameters->int0Parameter;
}

int LevelLT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return actor->GetStat(IE_LEVEL) < (unsigned) parameters->int0Parameter;
}

int ClassLevelLT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	if (!GS::ID_Class( actor, parameters->int0Parameter) )
		return 0;
	return actor->GetStat(IE_LEVEL) < (unsigned) parameters->int1Parameter;
}

int LevelInClassLT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;

	if (!GS::ID_ClassMask( actor, parameters->int1Parameter) )
		return 0;
	return actor->GetStat(IE_LEVEL) < (unsigned) parameters->int0Parameter;
}

int UnselectableVariable(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	return tar->UnselectableTimer == (unsigned) parameters->int0Parameter;
}

int UnselectableVariableGT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	return tar->UnselectableTimer > (unsigned) parameters->int0Parameter;
}

int UnselectableVariableLT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	return tar->UnselectableTimer < (unsigned) parameters->int0Parameter;
}

int AreaCheck(Scriptable* Sender, Trigger* parameters)
{
	if (!strnicmp(Sender->GetCurrentArea()->GetScriptName(), parameters->string0Parameter, 8)) {
		return 1;
	}
	return 0;
}

int AreaCheckObject(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );

	if (!tar) {
		return 0;
	}
	if (!strnicmp(tar->GetCurrentArea()->GetScriptName(), parameters->string0Parameter, 8)) {
		return 1;
	}
	return 0;
}

//lame iwd2 uses a numeric area identifier, this reduces its usability
int CurrentAreaIs(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );

	if (!tar) {
		return 0;
	}
	ieResRef arearesref;
	snprintf(arearesref, 8, "AR%04d", parameters->int0Parameter);
	if (!strnicmp(tar->GetCurrentArea()->GetScriptName(), arearesref, 8)) {
		return 1;
	}
	return 0;
}

//lame bg2 uses a constant areaname prefix, this reduces its usability
//but in the spirit of flexibility, gemrb extension allows arbitrary prefixes
int AreaStartsWith(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );

	if (!tar) {
		return 0;
	}
	ieResRef arearesref;
	if (parameters->string0Parameter[0]) {
		strnlwrcpy(arearesref, parameters->string0Parameter, 8);
	} else {
		strnlwrcpy(arearesref, "AR30", 8); //InWatchersKeep
	}
	int i = strlen(arearesref);
	if (!strnicmp(tar->GetCurrentArea()->GetScriptName(), arearesref, i)) {
		return 1;
	}
	return 0;
}

int EntirePartyOnMap(Scriptable* Sender, Trigger* /*parameters*/)
{
	Map *map = Sender->GetCurrentArea();
	Game *game=core->GetGame();
	int i=game->GetPartySize(true);
	while (i--) {
		Actor *actor=game->GetPC(i,true);
		if (actor->GetCurrentArea()!=map) {
			return 0;
		}
	}
	return 1;
}

int AnyPCOnMap(Scriptable* Sender, Trigger* /*parameters*/)
{
	Map *map = Sender->GetCurrentArea();
	Game *game=core->GetGame();
	int i=game->GetPartySize(true);
	while (i--) {
		Actor *actor=game->GetPC(i,true);
		if (actor->GetCurrentArea()==map) {
			return 1;
		}
	}
	return 0;
}

int InActiveArea(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (core->GetGame()->GetCurrentArea() == tar->GetCurrentArea()) {
		return 1;
	}
	return 0;
}

int InMyArea(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (Sender->GetCurrentArea() == tar->GetCurrentArea()) {
		return 1;
	}
	return 0;
}

int AreaType(Scriptable* Sender, Trigger* parameters)
{
	Map *map=Sender->GetCurrentArea();
	return (map->AreaType&parameters->int0Parameter)>0;
}

int IsExtendedNight( Scriptable* Sender, Trigger* /*parameters*/)
{
	Map *map=Sender->GetCurrentArea();
	if (map->AreaType&AT_EXTENDED_NIGHT) {
		return 1;
	}
	return 0;
}

int AreaFlag(Scriptable* Sender, Trigger* parameters)
{
	Map *map=Sender->GetCurrentArea();
	return (map->AreaFlags&parameters->int0Parameter)>0;
}

int AreaRestDisabled(Scriptable* Sender, Trigger* /*parameters*/)
{
	Map *map=Sender->GetCurrentArea();
	if (map->AreaFlags&2) {
		return 1;
	}
	return 0;
}

//new optional parameter: size of actor (to reach target)
int TargetUnreachable(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 1; //well, if it doesn't exist it is unreachable
	}
	Map *map=Sender->GetCurrentArea();
	if (!map) {
		return 1;
	}
	unsigned int size = parameters->int0Parameter;

	if (!size) {
		if (Sender->Type==ST_ACTOR) {
			size = ((Movable *) Sender)->size;
		}
		else {
			size = 1;
		}
	}
	return map->TargetUnreachable( Sender->Pos, tar->Pos, size);
}

int PartyCountEQ(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->GetPartySize(0)==parameters->int0Parameter;
}

int PartyCountLT(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->GetPartySize(0)<parameters->int0Parameter;
}

int PartyCountGT(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->GetPartySize(0)>parameters->int0Parameter;
}

int PartyCountAliveEQ(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->GetPartySize(1)==parameters->int0Parameter;
}

int PartyCountAliveLT(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->GetPartySize(1)<parameters->int0Parameter;
}

int PartyCountAliveGT(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->GetPartySize(1)>parameters->int0Parameter;
}

int LevelParty(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->GetPartyLevel(1)==parameters->int0Parameter;
}

int LevelPartyLT(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->GetPartyLevel(1)<parameters->int0Parameter;
}

int LevelPartyGT(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->GetPartyLevel(1)>parameters->int0Parameter;
}

int PartyGold(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->PartyGold == (ieDword) parameters->int0Parameter;
}

int PartyGoldGT(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->PartyGold > (ieDword) parameters->int0Parameter;
}

int PartyGoldLT(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->PartyGold < (ieDword) parameters->int0Parameter;
}

int OwnsFloaterMessage(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	return tar->textDisplaying;
}

int InCutSceneMode(Scriptable* /*Sender*/, Trigger* /*parameters*/)
{
	return core->InCutSceneMode();
}

int Proficiency(Scriptable* Sender, Trigger* parameters)
{
	unsigned int idx = parameters->int0Parameter;
	if (idx>31) {
		return 0;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return (signed) actor->GetStat(IE_PROFICIENCYBASTARDSWORD+idx) == parameters->int1Parameter;
}

int ProficiencyGT(Scriptable* Sender, Trigger* parameters)
{
	unsigned int idx = parameters->int0Parameter;
	if (idx>31) {
		return 0;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return (signed) actor->GetStat(IE_PROFICIENCYBASTARDSWORD+idx) > parameters->int1Parameter;
}

int ProficiencyLT(Scriptable* Sender, Trigger* parameters)
{
	unsigned int idx = parameters->int0Parameter;
	if (idx>31) {
		return 0;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return (signed) actor->GetStat(IE_PROFICIENCYBASTARDSWORD+idx) < parameters->int1Parameter;
}

//this is a PST specific stat, shows how many free proficiency slots we got
//we use an unused stat for it
int ExtraProficiency(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return (signed) actor->GetStat(IE_FREESLOTS) == parameters->int0Parameter;
}

int ExtraProficiencyGT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return (signed) actor->GetStat(IE_FREESLOTS) > parameters->int0Parameter;
}

int ExtraProficiencyLT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return (signed) actor->GetStat(IE_FREESLOTS) < parameters->int0Parameter;
}

int Internal(Scriptable* Sender, Trigger* parameters)
{
	unsigned int idx = parameters->int0Parameter;
	if (idx>15) {
		return 0;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return (signed) actor->GetStat(IE_INTERNAL_0+idx) == parameters->int1Parameter;
}

int InternalGT(Scriptable* Sender, Trigger* parameters)
{
	unsigned int idx = parameters->int0Parameter;
	if (idx>15) {
		return 0;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return (signed) actor->GetStat(IE_INTERNAL_0+idx) > parameters->int1Parameter;
}

int InternalLT(Scriptable* Sender, Trigger* parameters)
{
	unsigned int idx = parameters->int0Parameter;
	if (idx>15) {
		return 0;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return (signed) actor->GetStat(IE_INTERNAL_0+idx) < parameters->int1Parameter;
}

//we check if target is currently in dialog or not
int NullDialog(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	GameControl *gc = core->GetGameControl();
	if ( (tar->GetGlobalID() != gc->dialoghandler->targetID) && (tar->GetGlobalID() != gc->dialoghandler->speakerID) ) {
		return 1;
	}
	return 0;
}

//this one checks scriptname (deathvar), i hope it is right
//IsScriptName depends on this too
//Name is another (similar function)
int CalledByName(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) tar;
	if (stricmp(actor->GetScriptName(), parameters->string0Parameter) ) {
		return 0;
	}
	return 1;
}

//This is checking on the character's name as it was typed in
int CharName(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr || scr->Type!=ST_ACTOR) {
		return 0;
	}
	Actor* actor = (Actor *) scr;
	if (!strnicmp(actor->ShortName, parameters->string0Parameter, 32) ) {
		return 1;
	}
	return 0;
}

int AnimationID(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) tar;
	if ((ieWord) actor->GetStat(IE_ANIMATION_ID) == (ieWord) parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int AnimState(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) tar;
	return actor->GetStance() == parameters->int0Parameter;
}

//this trigger uses hours
int Time(Scriptable* /*Sender*/, Trigger* parameters)
{
	return (core->GetGame()->GameTime/AI_UPDATE_TIME)%7200/300 == (ieDword) parameters->int0Parameter;
}

//this trigger uses hours
int TimeGT(Scriptable* /*Sender*/, Trigger* parameters)
{
	return (core->GetGame()->GameTime/AI_UPDATE_TIME)%7200/300 > (ieDword) parameters->int0Parameter;
}

//this trigger uses hours
int TimeLT(Scriptable* /*Sender*/, Trigger* parameters)
{
	return (core->GetGame()->GameTime/AI_UPDATE_TIME)%7200/300 < (ieDword) parameters->int0Parameter;
}

int HotKey(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return 0;
	}
	Actor *scr = (Actor *) Sender;
				// FIXME: this is never going to work on 64 bit archs ...
	int ret = (unsigned long) scr->HotKey == (unsigned long) parameters->int0Parameter;
	//probably we need to implement a trigger mechanism, clear
	//the hotkey only when the triggerblock was evaluated as true
	if (ret) {
		Sender->AddTrigger (&scr->HotKey);
	}
	return ret;
}

int CombatCounter(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->CombatCounter == (ieDword) parameters->int0Parameter;
}

int CombatCounterGT(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->CombatCounter > (ieDword) parameters->int0Parameter;
}

int CombatCounterLT(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->CombatCounter < (ieDword) parameters->int0Parameter;
}

int TrapTriggered(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type != ST_TRIGGER) {
		return 0;
	}
/* matchactor would do this, hmm
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
*/
	if (MatchActor(Sender, Sender->LastTrigger, parameters->objectParameter)) {
		Sender->AddTrigger (&Sender->LastTrigger);
		return 1;
	}
	return 0;
}

int InteractingWith(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	GameControl *gc = core->GetGameControl();
	if (Sender->GetGlobalID() != gc->dialoghandler->targetID && Sender->GetGlobalID() != gc->dialoghandler->speakerID) {
		return 0;
	}
	if (tar->GetGlobalID() != gc->dialoghandler->targetID && tar->GetGlobalID() != gc->dialoghandler->speakerID) {
		return 0;
	}
	return 1;
}

int LastPersonTalkedTo(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor *scr = (Actor *) Sender;
	if (MatchActor(Sender, scr->LastTalkedTo, parameters->objectParameter)) {
		return 1;
	}
	return 0;
}

int IsRotation(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	if ( actor->GetOrientation() == parameters->int0Parameter ) {
		return 1;
	}
	return 0;
}

//GemRB currently stores the saved location in a local variable, but it is
//actually stored in the .gam structure (only for PCs)
int IsFacingSavedRotation(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	if (actor->GetOrientation() == actor->GetStat(IE_SAVEDFACE) ) {
		return 1;
	}
	return 0;
}

int IsFacingObject(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return 0;
	}
	Scriptable* target = GetActorFromObject( Sender, parameters->objectParameter );
	if (!target) {
		return 0;
	}
	Actor* actor = ( Actor* ) Sender;
	if (actor->GetOrientation()==GetOrient( target->Pos, actor->Pos ) ) {
		return 1;
	}
	return 0;
}

int AttackedBy(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *scr = (Actor *) Sender;
	Targets *tgts = GetAllObjects(Sender->GetCurrentArea(), Sender, parameters->objectParameter, GA_NO_DEAD);
	int ret = 0;
	int AStyle = parameters->int0Parameter;
	//iterate through targets to get the actor
	if (tgts) {
		targetlist::iterator m;
		const targettype *tt = tgts->GetFirstTarget(m, ST_ACTOR);
		while (tt) {
			Actor *actor = (Actor *) tt->actor;
			//if (actor->LastTarget == scr->GetID()) {
			if (scr->LastAttacker == actor->GetGlobalID()) {
				if (!AStyle || (AStyle==actor->GetAttackStyle()) ) {
					scr->AddTrigger(&scr->LastAttacker);
					ret = 1;
					break;
				}
			}
			tt = tgts->GetNextTarget(m, ST_ACTOR);
		}
	}
	delete tgts;
	return ret;
}

int TookDamage(Scriptable* Sender, Trigger* /*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) Sender;
	//zero damage doesn't count?
	if (actor->LastHitter && actor->LastDamage) {
		Sender->AddTrigger(&actor->LastHitter);
		return 1;
	}
	return 0;
}

int HitBy(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) Sender;
	if (parameters->int0Parameter) {
		if (!(parameters->int0Parameter&actor->LastDamageType) ) {
			return 0;
		}
	}
	if (MatchActor(Sender, actor->LastHitter, parameters->objectParameter)) {
		Sender->AddTrigger(&actor->LastHitter);
		return 1;
	}
	return 0;
}

int Heard(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) Sender;
	if (parameters->int0Parameter) {
		if (parameters->int0Parameter!=actor->LastShout) {
			return 0;
		}
	}
	if (MatchActor(Sender, actor->LastHeard, parameters->objectParameter)) {
		Sender->AddTrigger(&actor->LastHeard);
		return 1;
	}
	return 0;
}

int LastMarkedObject_Trigger(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) Sender;
	if (MatchActor(Sender, actor->LastMarked, parameters->objectParameter)) {
		//don't mark this object for clear
		//Sender->AddTrigger(&actor->LastSeen);
		return 1;
	}
	return 0;
}

int HelpEX(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	int stat;
	switch (parameters->int0Parameter) {
		case 1: stat = IE_EA; break;
		case 2: stat = IE_GENERAL; break;
		case 3: stat = IE_RACE; break;
		case 4: stat = IE_CLASS; break;
		case 5: stat = IE_SPECIFIC; break;
		case 6: stat = IE_SEX; break;
		case 7: stat = IE_ALIGNMENT; break;
		default: return 0;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type!=ST_ACTOR) {
		//a non actor checking for help?
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	Actor* help = Sender->GetCurrentArea()->GetActorByGlobalID(actor->LastHelp);
	if (!help) {
		//no help required
		return 0;
	}
	if (actor->GetStat(stat)==help->GetStat(stat) ) {
		Sender->AddTrigger(&actor->LastHelp);
		return 1;
	}
	return 0;
}

int Help_Trigger(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) Sender;
	if (MatchActor(Sender, actor->LastHelp, parameters->objectParameter)) {
		Sender->AddTrigger(&actor->LastHelp);
		return 1;
	}
	return 0;
}

int ReceivedOrder(Scriptable* Sender, Trigger* parameters)
{
	if (MatchActor(Sender, Sender->LastOrderer, parameters->objectParameter) &&
		parameters->int0Parameter==Sender->LastOrder) {
		Sender->AddTrigger(&Sender->LastOrderer);
		return 1;
	}
	return 0;
}

int Joins(Scriptable* Sender, Trigger* parameters)
{
	if(Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor * actor = ( Actor* ) Sender;
	//this trigger is sent only to PCs in a party
	if(!actor->PCStats) {
		return 0;
	}
	if (MatchActor(Sender, actor->PCStats->LastJoined, parameters->objectParameter)) {
		Sender->AddTrigger(&actor->PCStats->LastJoined);
		return 1;
	}
	return 0;
}

int Leaves(Scriptable* Sender, Trigger* parameters)
{
	if(Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor * actor = ( Actor* ) Sender;
	//this trigger is sent only to PCs in a party
	if(!actor->PCStats) {
		return 0;
	}
	if (MatchActor(Sender, actor->PCStats->LastLeft, parameters->objectParameter)) {
		Sender->AddTrigger(&actor->PCStats->LastLeft);
		return 1;
	}
	return 0;
}

int FallenPaladin(Scriptable* Sender, Trigger* /*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor* act = ( Actor* ) Sender;
	return (act->GetStat(IE_MC_FLAGS) & MC_FALLEN_PALADIN)!=0;
}

int FallenRanger(Scriptable* Sender, Trigger* /*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor* act = ( Actor* ) Sender;
	return (act->GetStat(IE_MC_FLAGS) & MC_FALLEN_RANGER)!=0;
}

int NightmareModeOn(Scriptable* /*Sender*/, Trigger* /*parameters*/)
{
	ieDword diff;

	core->GetDictionary()->Lookup("Nightmare Mode", diff);
	if (diff) {
		return 1;
	}
	return 0;
}

int Difficulty(Scriptable* /*Sender*/, Trigger* parameters)
{
	ieDword diff;

	core->GetDictionary()->Lookup("Difficulty Level", diff);
	int mode = parameters->int1Parameter;
	//hack for compatibility
	if (!mode) {
		mode = EQUALS;
	}
	return DiffCore(diff, (ieDword) parameters->int0Parameter, mode);
}

int DifficultyGT(Scriptable* /*Sender*/, Trigger* parameters)
{
	ieDword diff;

	core->GetDictionary()->Lookup("Difficulty Level", diff);
	return diff>(ieDword) parameters->int0Parameter;
}

int DifficultyLT(Scriptable* /*Sender*/, Trigger* parameters)
{
	ieDword diff;

	core->GetDictionary()->Lookup("Difficulty Level", diff);
	return diff<(ieDword) parameters->int0Parameter;
}

int Vacant(Scriptable* Sender, Trigger* /*parameters*/)
{
	if (Sender->Type!=ST_AREA) {
		return 0;
	}
	Map *map = (Map *) Sender;
	if ( map->CanFree() ) {
		return 1;
	}
	return 0;
}

//this trigger always checks the right hand weapon?
int InWeaponRange(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	Actor *actor = (Actor *) Sender;
	WeaponInfo wi;
	unsigned int wrange = 0;
	ITMExtHeader *header = actor->GetWeapon(wi, false);
	if (header) {
		wrange = wi.range;
	}
	header = actor->GetWeapon(wi, true);
	if (header && (wi.range>wrange)) {
		wrange = wi.range;
	}
	if ( PersonalDistance( Sender, tar ) <= wrange * 10 ) {
		return 1;
	}
	return 0;
}

//this implementation returns only true if there is a bow wielded
//but there is no ammo for it
//if the implementation should sign 'no ranged attack possible'
//then change some return values
//in bg2/iwd2 it doesn't accept an object (the object parameter is gemrb ext.)
int OutOfAmmo(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = Sender;
	if (parameters->objectParameter) {
		scr = GetActorFromObject( Sender, parameters->objectParameter );
	}
	if ( !scr || scr->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) scr;
	WeaponInfo wi;
	ITMExtHeader *header = actor->GetWeapon(wi, false);
	//no bow wielded?
	if (!header || header->AttackType!=ITEM_AT_BOW) {
		return 0;
	}
	//we either have a projectile (negative) or an empty bow (positive)
	//so we should find a negative slot, positive slot means: OutOfAmmo
	if (actor->inventory.GetEquipped()<0) {
		return 0;
	}
	//out of ammo
	return 1;
}

//returns true if a weapon is equipped (with more than 0 range)
//if a bow is equipped without projectile, it is useless!
//please notice how similar is this to OutOfAmmo
int HaveUsableWeaponEquipped(Scriptable* Sender, Trigger* /*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) Sender;
	WeaponInfo wi;
	ITMExtHeader *header = actor->GetWeapon(wi, false);

	//bows are not usable (because if they are loaded, the equipped
	//weapon is the projectile)
	if (!header || header->AttackType==ITEM_AT_BOW) {
		return 0;
	}
	//only fist we have, it is not qualified as weapon?
	if (actor->inventory.GetEquippedSlot() == actor->inventory.GetFistSlot()) {
		return 0;
	}
	return 1;
}

int HasWeaponEquipped(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	if (actor->inventory.GetEquippedSlot() == IW_NO_EQUIPPED) {
		return 0;
	}
	return 1;
}

int PCInStore( Scriptable* /*Sender*/, Trigger* /*parameters*/)
{
	if (core->GetCurrentStore()) {
		return 1;
	}
	return 0;
}

//this checks if the launch point is onscreen, a more elaborate check
//would see if any piece of the Scriptable is onscreen, what is the original
//behaviour?
int OnScreen( Scriptable* Sender, Trigger* /*parameters*/)
{
	Region vp = core->GetVideoDriver()->GetViewport();
	if (vp.PointInside(Sender->Pos) ) {
		return 1;
	}
	return 0;
}

int IsPlayerNumber( Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	if (actor->InParty == parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int PCCanSeePoint( Scriptable* /*Sender*/, Trigger* parameters)
{
	Map* map = core->GetGame()->GetCurrentArea();
	if (map->IsVisible(parameters->pointParameter, false) ) {
		return 1;
	}
	return 0;
}

//i'm clueless about this trigger
int StuffGlobalRandom( Scriptable* Sender, Trigger* parameters)
{
	unsigned int max=parameters->int0Parameter+1;
	ieDword Value;
	if (max) {
		Value = RandomNumValue%max;
	} else {
		Value = RandomNumValue;
	}
	SetVariable( Sender, parameters->string0Parameter, Value );
	if (Value) {
		return 1;
	}
	return 0;
}

int IsCreatureAreaFlag( Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	if (actor->GetStat(IE_MC_FLAGS) & parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int IsPathCriticalObject( Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	if (actor->GetStat(IE_MC_FLAGS) & MC_PLOT_CRITICAL) {
		return 1;
	}
	return 0;
}

// 0 - ability, 1 - number, 2 - mode
int ChargeCount( Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	int Slot = actor->inventory.FindItem(parameters->string0Parameter,0);
	if (Slot<0) {
		return 0;
	}
	CREItem *item = actor->inventory.GetSlotItem (Slot);
	if (!item) {//bah
		return 0;
	}
	if (parameters->int0Parameter>2) {
		return 0;
	}
	int charge = item->Usages[parameters->int0Parameter];
	switch (parameters->int2Parameter) {
		case DM_EQUAL:
			if (charge == parameters->int1Parameter)
				return 1;
			break;
		case DM_LESS:
			if (charge < parameters->int1Parameter)
				return 1;
			break;
		case DM_GREATER:
			if (charge > parameters->int1Parameter)
				return 1;
			break;
		default:
			return 0;
	}
	return 0;
}

// no idea if it checks only alive partymembers
int CheckPartyLevel( Scriptable* /*Sender*/, Trigger* parameters)
{
	if (core->GetGame()->GetPartyLevel(false)<parameters->int0Parameter) {
		return 0;
	}
	return 1;
}

// no idea if it checks only alive partymembers
int CheckPartyAverageLevel( Scriptable* /*Sender*/, Trigger* parameters)
{
	int level = core->GetGame()->GetPartyLevel(false);
	switch (parameters->int1Parameter) {
		case DM_EQUAL:
			if (level ==parameters->int0Parameter) {
				return 1;
			}
			break;
		case DM_LESS:
			if (level < parameters->int0Parameter) {
				return 1;
			}
			break;
		case DM_GREATER:
			if (level > parameters->int0Parameter) {
				return 1;
			}
			break;
		default:
			return 0;
	}
	return 1;
}

int CheckDoorFlags( Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type!=ST_DOOR) {
		return 0;
	}
	Door* door = ( Door* ) tar;
	if (door->Flags&parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

// works only on animations?
// Be careful when converting to GetActorFromObject, it won't return animations (those are not scriptable)
int Frame( Scriptable* Sender, Trigger* parameters)
{
	//to avoid a crash
	if (!parameters->objectParameter) {
		return 0;
	}
	AreaAnimation* anim = Sender->GetCurrentArea()->GetAnimation(parameters->objectParameter->objectName);
	if (!anim) {
		return 0;
	}
	int frame = anim->frame;
	if ((frame>=parameters->int0Parameter) &&
	(frame<=parameters->int1Parameter) ) {
		return 1;
	}
	return 0;
}

//Modalstate in IWD2 allows specifying an object
int ModalState( Scriptable* Sender, Trigger* parameters)
{
	Scriptable *scr;

	if (parameters->objectParameter) {
		scr = GetActorFromObject( Sender, parameters->objectParameter );
	} else {
		scr = Sender;
	}
	if (scr->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) scr;

	if (actor->ModalState==(ieDword) parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

/* a special redundant trigger for iwd2 - could do something extra */
int IsCreatureHiddenInShadows( Scriptable* Sender, Trigger* /*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) Sender;

	if (actor->ModalState==MS_STEALTH) {
		return 1;
	}
	return 0;
}

int IsWeather( Scriptable* /*Sender*/, Trigger* parameters)
{
	Game *game = core->GetGame();
	ieDword weather = game->WeatherBits & parameters->int0Parameter;
	if (weather == (ieDword) parameters->int1Parameter) {
		return 1;
	}
	return 0;
}

int Delay( Scriptable* Sender, Trigger* parameters)
{
	ieDword delay = (ieDword) parameters->int0Parameter;
	if (delay<=1) {
		return 1;
	}
	ieDword time1=Sender->lastDelay/1000/delay;
	ieDword time2=Sender->lastRunTime/1000/delay;

	if (time1!=time2) {
		return 1;
	}
	return 0;
}

int TimeOfDay(Scriptable* /*Sender*/, Trigger* parameters)
{
	ieDword timeofday = (core->GetGame()->GameTime/AI_UPDATE_TIME)%7200/1800;

	if (timeofday==(ieDword) parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

//this is a PST action, it's using delta, not diffmode
int RandomStatCheck(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;

	ieDword stat = actor->GetStat(parameters->int0Parameter);
	ieDword value = Bones(parameters->int2Parameter);
	switch(parameters->int1Parameter) {
		case DM_SET:
			if (stat==value)
				return 1;
			break;
		case DM_LOWER:
			if (stat<value)
				return 1;
			break;
		case DM_RAISE:
			if (stat>value)
				return 1;
			break;
	}
	return 0;
}

int PartyRested(Scriptable* Sender, Trigger* /*parameters*/)
{
	if (Sender->GetInternalFlag()&IF_PARTYRESTED) {
		Sender->SetBitTrigger(BT_PARTYRESTED);
		return 1;
	}
	return 0;
}

int IsWeaponRanged(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	if (actor->inventory.GetEquipped()<0) {
		return 1;
	}
	return 0;
}

//HoW applies sequence on area animations
int Sequence(Scriptable* Sender, Trigger* parameters)
{
	//to avoid a crash, check if object is NULL
	if (parameters->objectParameter) {
		AreaAnimation *anim = Sender->GetCurrentArea()->GetAnimation(parameters->objectParameter->objectName);
		if (anim) {
			//this is the cycle count for the area animation
			//very much like stance for avatar anims
			if (anim->sequence==parameters->int0Parameter) {
				return 1;
			}
			return 0;
		}
	}

	Scriptable *tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	if (actor->GetStance()==parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int TimerExpired(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->TimerExpired(parameters->int0Parameter) ) {
		return 1;
	}
	return 0;
}

int TimerActive(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->TimerActive(parameters->int0Parameter) ) {
		return 1;
	}
	return 0;
}

int ActuallyInCombat(Scriptable* /*Sender*/, Trigger* /*parameters*/)
{
	Game *game=core->GetGame();
	if (game->AnyPCInCombat()) return 1;
	return 0;
}

int InMyGroup(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}

	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
/* IESDP SUCKS
	if (GetGroup( (Actor *) tar)==GetGroup( (Actor *) Sender) ) {
		return 1;
	}
*/
	if ( ((Actor *) tar)->GetStat(IE_SPECIFIC)==((Actor *) tar)->GetStat(IE_SPECIFIC) ) {
		return 1;
	}
	return 0;
}

int AnyPCSeesEnemy(Scriptable* /*Sender*/, Trigger* /*parameters*/)
{
	Game *game = core->GetGame();
	unsigned int i = (unsigned int) game->GetLoadedMapCount();
	while(i--) {
		Map *map = game->GetMap(i);
		if (map->AnyPCSeesEnemy()) {
			return 1;
		}
	}
	return 0;
}

int Unusable(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) Sender;

	Item *item = gamedata->GetItem(parameters->string0Parameter);
	int ret;
	if (actor->Unusable(item)) {
		ret = 0;
	} else {
		ret = 1;
	}
	gamedata->FreeItem(item, parameters->string0Parameter, true);
	return ret;
}

int HasBounceEffects(Scriptable* Sender, Trigger* parameters)
{
	Scriptable *tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	if (actor->GetStat(IE_BOUNCE)) return 1;
	return 0;
}

int HasImmunityEffects(Scriptable* Sender, Trigger* parameters)
{
	Scriptable *tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	if (actor->GetStat(IE_IMMUNITY)) return 1;
	return 0;
}

// this is a GemRB specific trigger, to transfer some system variables
// to a global (game variable), it will always return true, and the
// variable could be checked in a subsequent trigger (like triggersetglobal)

#define SYSV_SCREENFLAGS    0
#define SYSV_CONTROLSTATUS  1
#define SYSV_REPUTATION     2
#define SYSV_PARTYGOLD      3

int SystemVariable_Trigger(Scriptable* Sender, Trigger* parameters)
{
	ieDword value;

	switch (parameters->int0Parameter) {
	case SYSV_SCREENFLAGS:
		value = core->GetGameControl()->GetScreenFlags();
		break;
	case SYSV_CONTROLSTATUS:
		value = core->GetGame()->ControlStatus;
		break;
	case SYSV_REPUTATION:
		value = core->GetGame()->Reputation;
		break;
	case SYSV_PARTYGOLD:
		value = core->GetGame()->PartyGold;
		break;
	default:
		return 0;
	}

	SetVariable(Sender, parameters->string0Parameter, value);
	return 1;
}

int SpellCast(Scriptable* Sender, Trigger* parameters)
{
	if(parameters->int0Parameter) {
		unsigned int param = 2000+parameters->int0Parameter%1000;
		if (param!=Sender->LastSpellSeen) {
			return 0;
		}
	}
	if(MatchActor(Sender, Sender->LastCasterSeen, parameters->objectParameter)) {
		Sender->AddTrigger(&Sender->LastCasterSeen);
		return 1;
	}
	return 0;
}

int SpellCastPriest(Scriptable* Sender, Trigger* parameters)
{
	if(parameters->int0Parameter) {
		unsigned int param = 1000+parameters->int0Parameter%1000;
		if (param!=Sender->LastSpellSeen) {
			return 0;
		}
	}
	if(MatchActor(Sender, Sender->LastCasterSeen, parameters->objectParameter)) {
		Sender->AddTrigger(&Sender->LastCasterSeen);
		return 1;
	}
	return 0;
}

int SpellCastInnate(Scriptable* Sender, Trigger* parameters)
{
	if(parameters->int0Parameter) {
		unsigned int param = 3000+parameters->int0Parameter%1000;
		if (param!=Sender->LastSpellSeen) {
			return 0;
		}
	}
	if(MatchActor(Sender, Sender->LastCasterSeen, parameters->objectParameter)) {
		Sender->AddTrigger(&Sender->LastCasterSeen);
		return 1;
	}
	return 0;
}

int SpellCastOnMe(Scriptable* Sender, Trigger* parameters)
{
	if(parameters->int0Parameter) {
		if ((ieDword) parameters->int0Parameter!=Sender->LastSpellOnMe) {
			return 0;
		}
	}
	if(MatchActor(Sender, Sender->LastCasterOnMe, parameters->objectParameter)) {
		Sender->AddTrigger(&Sender->LastCasterOnMe);
		return 1;
	}
	return 0;
}

int CalendarDay(Scriptable* /*Sender*/, Trigger* parameters)
{
	int day = core->GetCalendar()->GetCalendarDay(core->GetGame()->GameTime/AI_UPDATE_TIME/7200);
	if(day == parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int CalendarDayGT(Scriptable* /*Sender*/, Trigger* parameters)
{
	int day = core->GetCalendar()->GetCalendarDay(core->GetGame()->GameTime/AI_UPDATE_TIME/7200);
	if(day > parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int CalendarDayLT(Scriptable* /*Sender*/, Trigger* parameters)
{
	int day = core->GetCalendar()->GetCalendarDay(core->GetGame()->GameTime/AI_UPDATE_TIME/7200);
	if(day < parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

//NT Returns true only if the active CRE was turned by the specified priest or paladin.
int TurnedBy(Scriptable* Sender, Trigger* /*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) Sender;
	if (MatchActor(Sender, actor->LastTurner, NULL)) {
		Sender->AddTrigger(&actor->LastTurner);
		return 1;
	}
	return 0;
}

static const TriggerDesc triggernames[] = {
	{"actionlistempty", ActionListEmpty, 0, 0},
	{"actuallyincombat", ActuallyInCombat, 0, 0},
	{"acquired", Acquired, 0, 0},
	{"alignment", Alignment, 0, 0},
	{"allegiance", Allegiance, 0, 0},
	{"animstate", AnimState, 0, 0},
	{"anypconmap", AnyPCOnMap, 0, 0},
	{"anypcseesenemy", AnyPCSeesEnemy, 0, 0},
	{"areacheck", AreaCheck, 0, 0},
	{"areacheckobject", AreaCheckObject, 0, 0},
	{"areaflag", AreaFlag, 0, 0},
	{"arearestdisabled", AreaRestDisabled, 0, 0},
	{"areatype", AreaType, 0, 0},
	{"atlocation", AtLocation, 0, 0},
	{"assaltedby", AttackedBy, 0, 0},//pst
	{"attackedby", AttackedBy, 0, 0},
	{"becamevisible", BecameVisible, 0, 0},
	{"bitcheck", BitCheck,TF_MERGESTRINGS, 0},
	{"bitcheckexact", BitCheckExact,TF_MERGESTRINGS, 0},
	{"bitglobal", BitGlobal_Trigger,TF_MERGESTRINGS, 0},
	{"breakingpoint", BreakingPoint, 0, 0},
	{"calanderday", CalendarDay, 0, 0}, //illiterate developers O_o
	{"calendarday", CalendarDay, 0, 0},
	{"calanderdaygt", CalendarDayGT, 0, 0},
	{"calendardaygt", CalendarDayGT, 0, 0},
	{"calanderdaylt", CalendarDayLT, 0, 0},
	{"calendardaylt", CalendarDayLT, 0, 0},
	{"calledbyname", CalledByName, 0, 0}, //this is still a question
	{"chargecount", ChargeCount, 0, 0},
	{"charname", CharName, 0, 0}, //not scripting name
	{"checkareadifflevel", DifficultyLT, 0, 0},//iwd2 guess
	{"checkdoorflags", CheckDoorFlags, 0, 0},
	{"checkpartyaveragelevel", CheckPartyAverageLevel, 0, 0},
	{"checkpartylevel", CheckPartyLevel, 0, 0},
	{"checkskill", CheckSkill, 0, 0},
	{"checkskillgt", CheckSkillGT, 0, 0},
	{"checkskilllt", CheckSkillLT, 0, 0},
	{"checkspellstate", CheckSpellState, 0, 0},
	{"checkstat", CheckStat, 0, 0},
	{"checkstatgt", CheckStatGT, 0, 0},
	{"checkstatlt", CheckStatLT, 0, 0},
	{"class", Class, 0, 0},
	{"classex", ClassEx, 0, 0}, //will return true for multis
	{"classlevel", ClassLevel, 0, 0}, //pst
	{"classlevelgt", ClassLevelGT, 0, 0},
	{"classlevellt", ClassLevelLT, 0, 0},
	{"clicked", Clicked, 0, 0},
	{"closed", Closed, 0, 0},
	{"combatcounter", CombatCounter, 0, 0},
	{"combatcountergt", CombatCounterGT, 0, 0},
	{"combatcounterlt", CombatCounterLT, 0, 0},
	{"contains", Contains, 0, 0},
	{"currentareais", CurrentAreaIs, 0, 0},//checks object
	{"creaturehidden", CreatureHidden, 0, 0},//this is the engine level hiding feature, not the skill
	{"creatureinarea", AreaCheck, 0, 0}, //pst, checks this object
	{"damagetaken", HPLost, 0, 0},
	{"damagetakengt", HPLostGT, 0, 0},
	{"damagetakenlt", HPLostLT, 0, 0},
	{"dead", Dead, 0, 0},
	{"delay", Delay, 0, 0},
	{"detect", Detect, 0, 0}, //so far i see no difference
	{"die", Die, 0, 0},
	{"died", Died, 0, 0},
	{"difficulty", Difficulty, 0, 0},
	{"difficultygt", DifficultyGT, 0, 0},
	{"difficultylt", DifficultyLT, 0, 0},
	{"disarmed", Disarmed, 0, 0},
	{"disarmfailed", DisarmFailed, 0, 0},
	{"entered", Entered, 0, 0},
	{"entirepartyonmap", EntirePartyOnMap, 0, 0},
	{"exists", Exists, 0, 0},
	{"extendedstatecheck", ExtendedStateCheck, 0, 0},
	{"extraproficiency", ExtraProficiency, 0, 0},
	{"extraproficiencygt", ExtraProficiencyGT, 0, 0},
	{"extraproficiencylt", ExtraProficiencyLT, 0, 0},
	{"faction", Faction, 0, 0},
	{"failedtoopen", OpenFailed, 0, 0},
	{"fallenpaladin", FallenPaladin, 0, 0},
	{"fallenranger", FallenRanger, 0, 0},
	{"false", GS::False, 0, 0},
	{"forcemarkedspell", ForceMarkedSpell_Trigger, 0, 0},
	{"frame", Frame, 0, 0},
	{"g", G_Trigger, 0, 0},
	{"gender", Gender, 0, 0},
	{"general", General, 0, 0},
	{"ggt", GGT_Trigger, 0, 0},
	{"glt", GLT_Trigger, 0, 0},
	{"global", Global,TF_MERGESTRINGS, 0},
	{"globalandglobal", GlobalAndGlobal_Trigger,TF_MERGESTRINGS, 0},
	{"globalband", BitCheck,TF_MERGESTRINGS, 0},
	{"globalbandglobal", GlobalBAndGlobal_Trigger,TF_MERGESTRINGS, 0},
	{"globalbandglobalexact", GlobalBAndGlobalExact,TF_MERGESTRINGS, 0},
	{"globalbitglobal", GlobalBitGlobal_Trigger,TF_MERGESTRINGS, 0},
	{"globalequalsglobal", GlobalsEqual,TF_MERGESTRINGS, 0}, //this is the same
	{"globalgt", GlobalGT,TF_MERGESTRINGS, 0},
	{"globalgtglobal", GlobalGTGlobal,TF_MERGESTRINGS, 0},
	{"globallt", GlobalLT,TF_MERGESTRINGS, 0},
	{"globalltglobal", GlobalLTGlobal,TF_MERGESTRINGS, 0},
	{"globalorglobal", GlobalOrGlobal_Trigger,TF_MERGESTRINGS, 0},
	{"globalsequal", GlobalsEqual, 0, 0},
	{"globalsgt", GlobalsGT, 0, 0},
	{"globalslt", GlobalsLT, 0, 0},
	{"globaltimerexact", GlobalTimerExact, 0, 0},
	{"globaltimerexpired", GlobalTimerExpired, 0, 0},
	{"globaltimernotexpired", GlobalTimerNotExpired, 0, 0},
	{"globaltimerstarted", GlobalTimerStarted, 0, 0},
	{"happiness", Happiness, 0, 0},
	{"happinessgt", HappinessGT, 0, 0},
	{"happinesslt", HappinessLT, 0, 0},
	{"harmlessclosed", Closed, 0, 0}, //pst, not sure
	{"harmlessentered", HarmlessEntered, 0, 0}, //???
	{"harmlessopened", Opened, 0, 0}, //pst, not sure
	{"hasbounceeffects", HasBounceEffects, 0, 0},
	{"hasimmunityeffects", HasImmunityEffects, 0, 0},
	{"hasinnateability", HaveSpell, 0, 0}, //these must be the same
	{"hasitem", HasItem, 0, 0},
	{"hasitemequiped", HasItemEquipped, 0, 0}, //typo in bg2
	{"hasitemequipedreal", HasItemEquipped, 0, 0}, //not sure
	{"hasitemequipped", HasItemEquipped, 0, 0},
	{"hasitemequippedreal", HasItemEquipped, 0, 0}, //not sure
	{"hasiteminslot", HasItemSlot, 0, 0},
	{"hasitemslot", HasItemSlot, 0, 0},
	{"hasitemtypeslot", HasItemTypeSlot, 0, 0},//gemrb extension
	{"hasweaponequiped", HasWeaponEquipped, 0, 0},//a typo again
	{"hasweaponequipped", HasWeaponEquipped, 0, 0},
	{"haveanyspells", HaveAnySpells, 0, 0},
	{"havespell", HaveSpell, 0, 0}, //these must be the same
	{"havespellparty", HaveSpellParty, 0, 0},
	{"havespellres", HaveSpell, 0, 0}, //they share the same ID
	{"haveusableweaponequipped", HaveUsableWeaponEquipped, 0, 0},
	{"heard", Heard, 0, 0},
	{"help", Help_Trigger, 0, 0},
	{"helpex", HelpEX, 0, 0},
	{"hitby", HitBy, 0, 0},
	{"hotkey", HotKey, 0, 0},
	{"hp", HP, 0, 0},
	{"hpgt", HPGT, 0, 0},
	{"hplost", HPLost, 0, 0},
	{"hplostgt", HPLostGT, 0, 0},
	{"hplostlt", HPLostLT, 0, 0},
	{"hplt", HPLT, 0, 0},
	{"hppercent", HPPercent, 0, 0},
	{"hppercentgt", HPPercentGT, 0, 0},
	{"hppercentlt", HPPercentLT, 0, 0},
	{"inactivearea", InActiveArea, 0, 0},
	{"incutscenemode", InCutSceneMode, 0, 0},
	{"inline", InLine, 0, 0},
	{"inmyarea", InMyArea, 0, 0},
	{"inmygroup", InMyGroup, 0, 0},
	{"inparty", InParty, 0, 0},
	{"inpartyallowdead", InPartyAllowDead, 0, 0},
	{"inpartyslot", InPartySlot, 0, 0},
	{"internal", Internal, 0, 0},
	{"internalgt", InternalGT, 0, 0},
	{"internallt", InternalLT, 0, 0},
	{"interactingwith", InteractingWith, 0, 0},
	{"intrap", InTrap, 0, 0},
	{"inventoryfull", InventoryFull, 0, 0},
	{"inview", LOS, 0, 0}, //it seems the same, needs research
	{"inwatcherskeep", AreaStartsWith, 0, 0},
	{"inweaponrange", InWeaponRange, 0, 0},
	{"isaclown", IsAClown, 0, 0},
	{"isactive", IsActive, 0, 0},
	{"isanimationid", AnimationID, 0, 0},
	{"iscreatureareaflag", IsCreatureAreaFlag, 0, 0},
	{"iscreaturehiddeninshadows", IsCreatureHiddenInShadows, 0, 0},
	{"isfacingobject", IsFacingObject, 0, 0},
	{"isfacingsavedrotation", IsFacingSavedRotation, 0, 0},
	{"isgabber", IsGabber, 0, 0},
	{"isheartoffurymodeon", NightmareModeOn, 0, 0},
	{"islocked", IsLocked, 0, 0},
	{"isextendednight", IsExtendedNight, 0, 0},
	{"ismarkedspell", IsMarkedSpell, 0, 0},
	{"isoverme", IsOverMe, 0, 0},
	{"ispathcriticalobject", IsPathCriticalObject, 0, 0},
	{"isplayernumber", IsPlayerNumber, 0, 0},
	{"isrotation", IsRotation, 0, 0},
	{"isscriptname", CalledByName, 0, 0}, //seems the same
	{"isspelltargetvalid", IsSpellTargetValid, 0, 0},
	{"isteambiton", IsTeamBitOn, 0, 0},
	{"isvalidforpartydialog", IsValidForPartyDialog, 0, 0},
	{"isvalidforpartydialogue", IsValidForPartyDialog, 0, 0},
	{"isweaponranged", IsWeaponRanged, 0, 0},
	{"isweather", IsWeather, 0, 0}, //gemrb extension
	{"itemisidentified", ItemIsIdentified, 0, 0},
	{"joins", Joins, 0, 0},
	{"kit", Kit, 0, 0},
	{"knowspell", KnowSpell, 0, 0}, //gemrb specific
	{"lastmarkedobject", LastMarkedObject_Trigger, 0, 0},
	{"lastpersontalkedto", LastPersonTalkedTo, 0, 0}, //pst
	{"leaves", Leaves, 0, 0},
	{"level", Level, 0, 0},
	{"levelgt", LevelGT, 0, 0},
	{"levelinclass", LevelInClass, 0, 0}, //iwd2
	{"levelinclassgt", LevelInClassGT, 0, 0},
	{"levelinclasslt", LevelInClassLT, 0, 0},
	{"levellt", LevelLT, 0, 0},
	{"levelparty", LevelParty, 0, 0},
	{"levelpartygt", LevelPartyGT, 0, 0},
	{"levelpartylt", LevelPartyLT, 0, 0},
	{"localsequal", LocalsEqual, 0, 0},
	{"localsgt", LocalsGT, 0, 0},
	{"localslt", LocalsLT, 0, 0},
	{"los", LOS, 0, 0},
	{"modalstate", ModalState, 0, 0},
	{"morale", Morale, 0, 0},
	{"moralegt", MoraleGT, 0, 0},
	{"moralelt", MoraleLT, 0, 0},
	{"name", CalledByName, 0, 0}, //this is the same too?
	{"namelessbitthedust", NamelessBitTheDust, 0, 0},
	{"nearbydialog", NearbyDialog, 0, 0},
	{"nearbydialogue", NearbyDialog, 0, 0},
	{"nearlocation", NearLocation, 0, 0},
	{"nearsavedlocation", NearSavedLocation, 0, 0},
	{"nightmaremodeon", NightmareModeOn, 0, 0},
	{"notstatecheck", NotStateCheck, 0, 0},
	{"nulldialog", NullDialog, 0, 0},
	{"nulldialogue", NullDialog, 0, 0},
	{"numcreature", NumCreatures, 0, 0},
	{"numcreaturegt", NumCreaturesGT, 0, 0},
	{"numcreaturelt", NumCreaturesLT, 0, 0},
	{"numcreaturesatmylevel", NumCreaturesAtMyLevel, 0, 0},
	{"numcreaturesgtmylevel", NumCreaturesGTMyLevel, 0, 0},
	{"numcreaturesltmylevel", NumCreaturesLTMyLevel, 0, 0},
	{"numcreaturevsparty", NumCreatureVsParty, 0, 0},
	{"numcreaturevspartygt", NumCreatureVsPartyGT, 0, 0},
	{"numcreaturevspartylt", NumCreatureVsPartyLT, 0, 0},
	{"numdead", NumDead, 0, 0},
	{"numdeadgt", NumDeadGT, 0, 0},
	{"numdeadlt", NumDeadLT, 0, 0},
	{"numinparty", PartyCountEQ, 0, 0},
	{"numinpartyalive", PartyCountAliveEQ, 0, 0},
	{"numinpartyalivegt", PartyCountAliveGT, 0, 0},
	{"numinpartyalivelt", PartyCountAliveLT, 0, 0},
	{"numinpartygt", PartyCountGT, 0, 0},
	{"numinpartylt", PartyCountLT, 0, 0},
	{"numitems", NumItems, 0, 0},
	{"numitemsgt", NumItemsGT, 0, 0},
	{"numitemslt", NumItemsLT, 0, 0},
	{"numitemsparty", NumItemsParty, 0, 0},
	{"numitemspartygt", NumItemsPartyGT, 0, 0},
	{"numitemspartylt", NumItemsPartyLT, 0, 0},
	{"numtimesinteracted", NumTimesInteracted, 0, 0},
	{"numtimesinteractedgt", NumTimesInteractedGT, 0, 0},
	{"numtimesinteractedlt", NumTimesInteractedLT, 0, 0},
	{"numtimesinteractedobject", NumTimesInteractedObject, 0, 0},//gemrb
	{"numtimesinteractedobjectgt", NumTimesInteractedObjectGT, 0, 0},//gemrb
	{"numtimesinteractedobjectlt", NumTimesInteractedObjectLT, 0, 0},//gemrb
	{"numtimestalkedto", NumTimesTalkedTo, 0, 0},
	{"numtimestalkedtogt", NumTimesTalkedToGT, 0, 0},
	{"numtimestalkedtolt", NumTimesTalkedToLT, 0, 0},
	{"objectactionlistempty", ObjectActionListEmpty, 0, 0}, //same function
	{"objitemcounteq", NumItems, 0, 0},
	{"objitemcountgt", NumItemsGT, 0, 0},
	{"objitemcountlt", NumItemsLT, 0, 0},
	{"oncreation", OnCreation, 0, 0},
	{"onisland", OnIsland, 0, 0},
	{"onscreen", OnScreen, 0, 0},
	{"opened", Opened, 0, 0},
	{"openfailed", OpenFailed, 0, 0},
	{"openstate", OpenState, 0, 0},
	{"or", Or, 0, 0},
	{"outofammo", OutOfAmmo, 0, 0},
	{"ownsfloatermessage", OwnsFloaterMessage, 0, 0},
	{"partycounteq", PartyCountEQ, 0, 0},
	{"partycountgt", PartyCountGT, 0, 0},
	{"partycountlt", PartyCountLT, 0, 0},
	{"partygold", PartyGold, 0, 0},
	{"partygoldgt", PartyGoldGT, 0, 0},
	{"partygoldlt", PartyGoldLT, 0, 0},
	{"partyhasitem", PartyHasItem, 0, 0},
	{"partyhasitemidentified", PartyHasItemIdentified, 0, 0},
	{"partyitemcounteq", NumItemsParty, 0, 0},
	{"partyitemcountgt", NumItemsPartyGT, 0, 0},
	{"partyitemcountlt", NumItemsPartyLT, 0, 0},
	{"partymemberdied", PartyMemberDied, 0, 0},
	{"partyrested", PartyRested, 0, 0},
	{"pccanseepoint", PCCanSeePoint, 0, 0},
	{"pcinstore", PCInStore, 0, 0},
	{"personalspacedistance", PersonalSpaceDistance, 0, 0},
	{"picklockfailed", PickLockFailed, 0, 0},
	{"pickpocketfailed", PickpocketFailed, 0, 0},
	{"proficiency", Proficiency, 0, 0},
	{"proficiencygt", ProficiencyGT, 0, 0},
	{"proficiencylt", ProficiencyLT, 0, 0},
	{"race", Race, 0, 0},
	{"randomnum", RandomNum, 0, 0},
	{"randomnumgt", RandomNumGT, 0, 0},
	{"randomnumlt", RandomNumLT, 0, 0},
	{"randomstatcheck", RandomStatCheck, 0, 0},
	{"range", Range, 0, 0},
	{"reaction", Reaction, 0, 0},
	{"reactiongt", ReactionGT, 0, 0},
	{"reactionlt", ReactionLT, 0, 0},
	{"realglobaltimerexact", RealGlobalTimerExact, 0, 0},
	{"realglobaltimerexpired", RealGlobalTimerExpired, 0, 0},
	{"realglobaltimernotexpired", RealGlobalTimerNotExpired, 0, 0},
	{"receivedorder", ReceivedOrder, 0, 0},
	{"reputation", Reputation, 0, 0},
	{"reputationgt", ReputationGT, 0, 0},
	{"reputationlt", ReputationLT, 0, 0},
	{"school", School, 0, 0}, //similar to kit
	{"see", See, 0, 0},
	{"sequence", Sequence, 0, 0},
	{"setlastmarkedobject", SetLastMarkedObject, 0, 0},
	{"setmarkedspell", SetMarkedSpell_Trigger, 0, 0},
	{"specifics", Specifics, 0, 0},
	{"spellcast", SpellCast, 0, 0},
	{"spellcastinnate", SpellCastInnate, 0, 0},
	{"spellcastonme", SpellCastOnMe, 0, 0},
	{"spellcastpriest", SpellCastPriest, 0, 0},
	{"statecheck", StateCheck, 0, 0},
	{"stealfailed", StealFailed, 0, 0},
	{"storehasitem", StoreHasItem, 0, 0},
	{"stuffglobalrandom", StuffGlobalRandom, 0, 0},//hm, this is a trigger
	{"subrace", SubRace, 0, 0},
	{"systemvariable", SystemVariable_Trigger, 0, 0}, //gemrb
	{"targetunreachable", TargetUnreachable, 0, 0},
	{"team", Team, 0, 0},
	{"time", Time, 0, 0},
	{"timegt", TimeGT, 0, 0},
	{"timelt", TimeLT, 0, 0},
	{"timeofday", TimeOfDay, 0, 0},
	{"timeractive", TimerActive, 0, 0},
	{"timerexpired", TimerExpired, 0, 0},
	{"tookdamage", TookDamage, 0, 0},
	{"totalitemcnt", TotalItemCnt, 0, 0}, //iwd2
	{"totalitemcntexclude", TotalItemCntExclude, 0, 0}, //iwd2
	{"totalitemcntexcludegt", TotalItemCntExcludeGT, 0, 0}, //iwd2
	{"totalitemcntexcludelt", TotalItemCntExcludeLT, 0, 0}, //iwd2
	{"totalitemcntgt", TotalItemCntGT, 0, 0}, //iwd2
	{"totalitemcntlt", TotalItemCntLT, 0, 0}, //iwd2
	{"traptriggered", TrapTriggered, 0, 0},
	{"trigger", TriggerTrigger, 0, 0},
	{"triggerclick", Clicked, 0, 0}, //not sure
	{"triggersetglobal", TriggerSetGlobal,0, 0}, //iwd2, but never used
	{"true", True, 0, 0},
	{"turnedby", TurnedBy, 0, 0},
	{"unlocked", Unlocked, 0, 0},
	{"unselectablevariable", UnselectableVariable, 0, 0},
	{"unselectablevariablegt", UnselectableVariableGT, 0, 0},
	{"unselectablevariablelt", UnselectableVariableLT, 0, 0},
	{"unusable",Unusable, 0, 0},
	{"vacant",Vacant, 0, 0},
	{"walkedtotrigger", WalkedToTrigger, 0, 0},
	{"wasindialog", WasInDialog, 0, 0},
	{"xor", Xor,TF_MERGESTRINGS, 0},
	{"xp", XP, 0, 0},
	{"xpgt", XPGT, 0, 0},
	{"xplt", XPLT, 0, 0},
};

void RegisterTriggers()
{
	TriggerRegistry.Register(sizeof(triggernames)/sizeof(TriggerDesc), triggernames);
}

#define STATIC_LINK
#include "plugindef.h"

GEMRB_PLUGIN(_, _)
PLUGIN_INITIALIZER(RegisterTriggers)
END_PLUGIN()
