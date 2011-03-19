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

#include "GameScript/GameScript.h"

#include "GameScript/GSUtils.h"
#include "GameScript/Matching.h"

#include "win32def.h"

#include "Game.h"
#include "GameData.h"
#include "Interface.h"
#include "Opcode.h"
#include "PluginMgr.h"

//debug flags
// 1 - cache
// 2 - cutscene ID
// 4 - globals
// 8 - action execution
//16 - trigger evaluation

OpcodeRegistry<TriggerFunction> TriggerRegistry;

//Make this an ordered list, so we could use bsearch!
static const ActionLink actionnames[] = {
	{"actionoverride",NULL, AF_INVALID}, //will this function ever be reached
	{"activate", GS::Activate, 0},
	{"activateportalcursor", GS::ActivatePortalCursor, 0},
	{"addareaflag", GS::AddAreaFlag, 0},
	{"addareatype", GS::AddAreaType, 0},
	{"addexperienceparty", GS::AddExperienceParty, 0},
	{"addexperiencepartycr", GS::AddExperiencePartyCR, 0},
	{"addexperiencepartyglobal", GS::AddExperiencePartyGlobal, 0},
	{"addfeat", GS::AddFeat, 0},
	{"addglobals", GS::AddGlobals, 0},
	{"addhp", GS::AddHP, 0},
	{"addjournalentry", GS::AddJournalEntry, 0},
	{"addkit", GS::AddKit, 0},
	{"addmapnote", GS::AddMapnote, 0},
	{"addpartyexperience", GS::AddExperienceParty, 0},
	{"addspecialability", GS::AddSpecialAbility, 0},
	{"addsuperkit", GS::AddSuperKit, 0},
	{"addwaypoint", GS::AddWayPoint,AF_BLOCKING},
	{"addxp2da", GS::AddXP2DA, 0},
	{"addxpobject", GS::AddXPObject, 0},
	{"addxpvar", GS::AddXP2DA, 0},
	{"advancetime", GS::AdvanceTime, 0},
	{"allowarearesting", GS::SetAreaRestFlag, 0},//iwd2
	{"ally", GS::Ally, 0},
	{"ambientactivate", GS::AmbientActivate, 0},
	{"ankhegemerge", GS::AnkhegEmerge, AF_ALIVE},
	{"ankheghide", GS::AnkhegHide, AF_ALIVE},
	{"applydamage", GS::ApplyDamage, 0},
	{"applydamagepercent", GS::ApplyDamagePercent, 0},
	{"applyspell", GS::ApplySpell, 0},
	{"applyspellpoint", GS::ApplySpellPoint, 0}, //gemrb extension
	{"attachtransitiontodoor", GS::AttachTransitionToDoor, 0},
	{"attack", GS::Attack,AF_BLOCKING|AF_ALIVE},
	{"attacknosound", GS::AttackNoSound,AF_BLOCKING|AF_ALIVE}, //no sound yet anyway
	{"attackoneround", GS::AttackOneRound,AF_BLOCKING|AF_ALIVE},
	{"attackreevaluate", GS::AttackReevaluate,AF_BLOCKING|AF_ALIVE},
	{"backstab", GS::Attack,AF_BLOCKING|AF_ALIVE},//actually hide+attack
	{"banterblockflag", GS::BanterBlockFlag,0},
	{"banterblocktime", GS::BanterBlockTime,0},
	{"bashdoor", GS::BashDoor,AF_BLOCKING|AF_ALIVE}, //the same until we know better
	{"battlesong", GS::BattleSong, AF_ALIVE},
	{"berserk", GS::Berserk, AF_ALIVE},
	{"bitclear", GS::BitClear,AF_MERGESTRINGS},
	{"bitglobal", GS::BitGlobal,AF_MERGESTRINGS},
	{"bitset", GS::GlobalBOr,AF_MERGESTRINGS}, //probably the same
	{"breakinstants", GS::BreakInstants, AF_BLOCKING},//delay execution of instants to the next AI cycle???
	{"calllightning", GS::Kill, 0}, //TODO: call lightning projectile
	{"calm", GS::Calm, 0},
	{"changeaiscript", GS::ChangeAIScript, 0},
	{"changeaitype", GS::ChangeAIType, 0},
	{"changealignment", GS::ChangeAlignment, 0},
	{"changeallegiance", GS::ChangeAllegiance, 0},
	{"changeanimation", GS::ChangeAnimation, 0},
	{"changeanimationnoeffect", GS::ChangeAnimationNoEffect, 0},
	{"changeclass", GS::ChangeClass, 0},
	{"changecolor", GS::ChangeColor, 0},
	{"changecurrentscript", GS::ChangeAIScript,AF_SCRIPTLEVEL},
	{"changedestination", GS::ChangeDestination,0}, //gemrb extension (iwd hack)
	{"changedialog", GS::ChangeDialogue, 0},
	{"changedialogue", GS::ChangeDialogue, 0},
	{"changegender", GS::ChangeGender, 0},
	{"changegeneral", GS::ChangeGeneral, 0},
	{"changeenemyally", GS::ChangeAllegiance, 0}, //this is the same
	{"changefaction", GS::SetFaction, 0}, //pst
	{"changerace", GS::ChangeRace, 0},
	{"changespecifics", GS::ChangeSpecifics, 0},
	{"changestat", GS::ChangeStat, 0},
	{"changestatglobal", GS::ChangeStatGlobal, 0},
	{"changestoremarkup", GS::ChangeStoreMarkup, 0},//iwd2
	{"changeteam", GS::SetTeam, 0}, //pst
	{"changetilestate", GS::ChangeTileState, 0}, //bg2
	{"chunkcreature", GS::Kill, 0}, //should be more graphical
	{"clearactions", GS::ClearActions, 0},
	{"clearallactions", GS::ClearAllActions, 0},
	{"clearpartyeffects", GS::ClearPartyEffects, 0},
	{"clearspriteeffects", GS::ClearSpriteEffects, 0},
	{"clicklbuttonobject", GS::ClickLButtonObject, AF_BLOCKING},
	{"clicklbuttonpoint", GS::ClickLButtonPoint, AF_BLOCKING},
	{"clickrbuttonobject", GS::ClickLButtonObject, AF_BLOCKING},
	{"clickrbuttonpoint", GS::ClickLButtonPoint, AF_BLOCKING},
	{"closedoor", GS::CloseDoor,0},
	{"containerenable", GS::ContainerEnable, 0},
	{"continue", GS::Continue,AF_IMMEDIATE | AF_CONTINUE},
	{"copygroundpilesto", GS::CopyGroundPilesTo, 0},
	{"createcreature", GS::CreateCreature, 0}, //point is relative to Sender
	{"createcreaturecopypoint", GS::CreateCreatureCopyPoint, 0}, //point is relative to Sender
	{"createcreaturedoor", GS::CreateCreatureDoor, 0},
	{"createcreatureatfeet", GS::CreateCreatureAtFeet, 0},
	{"createcreatureatlocation", GS::CreateCreatureAtLocation, 0},
	{"createcreatureimpassable", GS::CreateCreatureImpassable, 0},
	{"createcreatureimpassableallowoverlap", GS::CreateCreatureImpassableAllowOverlap, 0},
	{"createcreatureobject", GS::CreateCreatureObjectOffset, 0}, //the same
	{"createcreatureobjectcopy", GS::CreateCreatureObjectCopy, 0},
	{"createcreatureobjectcopyeffect", GS::CreateCreatureObjectCopy, 0}, //the same
	{"createcreatureobjectdoor", GS::CreateCreatureObjectDoor, 0},//same as createcreatureobject, but with dimension door animation
	{"createcreatureobjectoffscreen", GS::CreateCreatureObjectOffScreen, 0}, //same as createcreature object, but starts looking for a place far away from the player
	{"createcreatureobjectoffset", GS::CreateCreatureObjectOffset, 0}, //the same
	{"createcreatureoffscreen", GS::CreateCreatureOffScreen, 0},
	{"createitem", GS::CreateItem, 0},
	{"createitemglobal", GS::CreateItemNumGlobal, 0},
	{"createitemnumglobal", GS::CreateItemNumGlobal, 0},
	{"createpartygold", GS::CreatePartyGold, 0},
	{"createvisualeffect", GS::CreateVisualEffect, 0},
	{"createvisualeffectobject", GS::CreateVisualEffectObject, 0},
	{"createvisualeffectobjectSticky", GS::CreateVisualEffectObjectSticky, 0},
	{"cutsceneid", GS::CutSceneID,0},
	{"damage", GS::Damage, 0},
	{"daynight", GS::DayNight, 0},
	{"deactivate", GS::Deactivate, 0},
	{"debug", GS::Debug, 0},
	{"debugoutput", GS::Debug, 0},
	{"deletejournalentry", GS::RemoveJournalEntry, 0},
	{"demoend", GS::QuitGame, 0}, //same for now
	{"destroyalldestructableequipment", GS::DestroyAllDestructableEquipment, 0},
	{"destroyallequipment", GS::DestroyAllEquipment, 0},
	{"destroygold", GS::DestroyGold, 0},
	{"destroyitem", GS::DestroyItem, 0},
	{"destroypartygold", GS::DestroyPartyGold, 0},
	{"destroypartyitem", GS::DestroyPartyItem, 0},
	{"destroyself", GS::DestroySelf, 0},
	{"detectsecretdoor", GS::DetectSecretDoor, 0},
	{"dialog", GS::Dialogue,AF_BLOCKING},
	{"dialogforceinterrupt", GS::DialogueForceInterrupt,AF_BLOCKING},
	{"dialoginterrupt", GS::DialogueInterrupt,0},
	{"dialogue", GS::Dialogue,AF_BLOCKING},
	{"dialogueforceinterrupt", GS::DialogueForceInterrupt,AF_BLOCKING},
	{"dialogueinterrupt", GS::DialogueInterrupt,0},
	{"disablefogdither", GS::DisableFogDither, 0},
	{"disablespritedither", GS::DisableSpriteDither, 0},
	{"displaymessage", GS::DisplayMessage, 0},
	{"displaystring", GS::DisplayString, 0},
	{"displaystringhead", GS::DisplayStringHead, 0},
	{"displaystringheadowner", GS::DisplayStringHeadOwner, 0},
	{"displaystringheaddead", GS::DisplayStringHead, 0}, //same?
	{"displaystringnoname", GS::DisplayStringNoName, 0},
	{"displaystringnonamehead", GS::DisplayStringNoNameHead, 0},
	{"displaystringwait", GS::DisplayStringWait,AF_BLOCKING},
	{"doubleclicklbuttonobject", GS::DoubleClickLButtonObject, AF_BLOCKING},
	{"doubleclicklbuttonpoint", GS::DoubleClickLButtonPoint, AF_BLOCKING},
	{"doubleclickrbuttonobject", GS::DoubleClickLButtonObject, AF_BLOCKING},
	{"doubleclickrbuttonpoint", GS::DoubleClickLButtonPoint, AF_BLOCKING},
	{"dropinventory", GS::DropInventory, 0},
	{"dropinventoryex", GS::DropInventoryEX, 0},
	{"dropinventoryexexclude", GS::DropInventoryEX, 0}, //same
	{"dropitem", GS::DropItem, AF_BLOCKING},
	{"enablefogdither", GS::EnableFogDither, 0},
	{"enableportaltravel", GS::EnablePortalTravel, 0},
	{"enablespritedither", GS::EnableSpriteDither, 0},
	{"endcredits", GS::EndCredits, 0},//movie
	{"endcutscenemode", GS::EndCutSceneMode, 0},
	{"endgame", GS::QuitGame, 0}, //ending in iwd2
	{"enemy", GS::Enemy, 0},
	{"equipitem", GS::EquipItem, 0},
	{"equipmostdamagingmelee",GS::EquipMostDamagingMelee,0},
	{"equipranged", GS::EquipRanged,0},
	{"equipweapon", GS::EquipWeapon,0},
	{"erasejournalentry", GS::RemoveJournalEntry, 0},
	{"escapearea", GS::EscapeArea, AF_BLOCKING},
	{"escapeareadestroy", GS::EscapeAreaDestroy, AF_BLOCKING},
	{"escapeareanosee", GS::EscapeAreaNoSee, AF_BLOCKING},
	{"escapeareaobject", GS::EscapeAreaObject, AF_BLOCKING},
	{"escapeareaobjectnosee", GS::EscapeAreaObjectNoSee, AF_BLOCKING},
	{"exitpocketplane", GS::ExitPocketPlane, 0},
	{"expansionendcredits", GS::QuitGame, 0},//ends game too
	{"explore", GS::Explore, 0},
	{"exploremapchunk", GS::ExploreMapChunk, 0},
	{"exportparty", GS::ExportParty, 0},
	{"face", GS::Face,AF_BLOCKING},
	{"faceobject", GS::FaceObject, AF_BLOCKING},
	{"facesavedlocation", GS::FaceSavedLocation, AF_BLOCKING},
	{"fadefromblack", GS::FadeFromColor, AF_BLOCKING}, //probably the same
	{"fadefromcolor", GS::FadeFromColor, AF_BLOCKING},
	{"fadetoandfromcolor", GS::FadeToAndFromColor, AF_BLOCKING},
	{"fadetoblack", GS::FadeToColor, AF_BLOCKING}, //probably the same
	{"fadetocolor", GS::FadeToColor, AF_BLOCKING},
	{"fakeeffectexpirycheck", GS::FakeEffectExpiryCheck, 0},
	{"fillslot", GS::FillSlot, 0},
	{"finalsave", GS::SaveGame, 0}, //synonym
	{"findtraps", GS::FindTraps, 0},
	{"fixengineroom", GS::FixEngineRoom, 0},
	{"floatmessage", GS::DisplayStringHead, 0},
	{"floatmessagefixed", GS::FloatMessageFixed, 0},
	{"floatmessagefixedrnd", GS::FloatMessageFixedRnd, 0},
	{"floatmessagernd", GS::FloatMessageRnd, 0},
	{"floatrebus", GS::FloatRebus, 0},
	{"follow", GS::Follow, AF_ALIVE},
	{"followcreature", GS::FollowCreature, AF_BLOCKING|AF_ALIVE}, //pst
	{"followobjectformation", GS::FollowObjectFormation, AF_BLOCKING|AF_ALIVE},
	{"forceaiscript", GS::ForceAIScript, 0},
	{"forceattack", GS::ForceAttack, 0},
	{"forcefacing", GS::ForceFacing, 0},
	{"forcehide", GS::ForceHide, 0},
	{"forceleavearealua", GS::ForceLeaveAreaLUA, 0},
	{"forcemarkedspell", GS::ForceMarkedSpell, 0},
	{"forcespell", GS::ForceSpell, AF_BLOCKING},
	{"forcespellpoint", GS::ForceSpellPoint, AF_BLOCKING},
	{"forceusecontainer", GS::ForceUseContainer,AF_BLOCKING},
	{"formation", GS::Formation, AF_BLOCKING},
	{"fullheal", GS::FullHeal, 0},
	{"fullhealex", GS::FullHeal, 0}, //pst, not sure what's different
	{"generatemodronmaze", GS::GenerateMaze, 0},
	{"generatepartymember", GS::GeneratePartyMember, 0},
	{"getitem", GS::GetItem, 0},
	{"getstat", GS::GetStat, 0}, //gemrb specific
	{"giveexperience", GS::AddXPObject, 0},
	{"givegoldforce", GS::CreatePartyGold, 0}, //this is the same
	{"giveitem", GS::GiveItem, 0},
	{"giveitemcreate", GS::CreateItem, 0}, //actually this is a targeted createitem
	{"giveorder", GS::GiveOrder, 0},
	{"givepartyallequipment", GS::GivePartyAllEquipment, 0},
	{"givepartygold", GS::GivePartyGold, 0},
	{"givepartygoldglobal", GS::GivePartyGoldGlobal,0},//no mergestrings!
	{"globaladdglobal", GS::GlobalAddGlobal,AF_MERGESTRINGS},
	{"globalandglobal", GS::GlobalAndGlobal,AF_MERGESTRINGS},
	{"globalband", GS::GlobalBAnd,AF_MERGESTRINGS},
	{"globalbandglobal", GS::GlobalBAndGlobal,AF_MERGESTRINGS},
	{"globalbitglobal", GS::GlobalBitGlobal, AF_MERGESTRINGS},
	{"globalbor", GS::GlobalBOr,AF_MERGESTRINGS},
	{"globalborglobal", GS::GlobalBOrGlobal,AF_MERGESTRINGS},
	{"globalmax", GS::GlobalMax,AF_MERGESTRINGS},
	{"globalmaxglobal", GS::GlobalMaxGlobal,AF_MERGESTRINGS},
	{"globalmin", GS::GlobalMin,AF_MERGESTRINGS},
	{"globalminglobal", GS::GlobalMinGlobal,AF_MERGESTRINGS},
	{"globalorglobal", GS::GlobalOrGlobal,AF_MERGESTRINGS},
	{"globalset", GS::SetGlobal,AF_MERGESTRINGS},
	{"globalsetglobal", GS::GlobalSetGlobal,AF_MERGESTRINGS},
	{"globalshl", GS::GlobalShL,AF_MERGESTRINGS},
	{"globalshlglobal", GS::GlobalShLGlobal,AF_MERGESTRINGS},
	{"globalshout", GS::GlobalShout, 0},
	{"globalshr", GS::GlobalShR,AF_MERGESTRINGS},
	{"globalshrglobal", GS::GlobalShRGlobal,AF_MERGESTRINGS},
	{"globalsubglobal", GS::GlobalSubGlobal,AF_MERGESTRINGS},
	{"globalxor", GS::GlobalXor,AF_MERGESTRINGS},
	{"globalxorglobal", GS::GlobalXorGlobal,AF_MERGESTRINGS},
	{"gotostartscreen", GS::QuitGame, 0},//ending
	{"help", GS::Help, 0},
	{"hide", GS::Hide, 0},
	{"hideareaonmap", GS::HideAreaOnMap, 0},
	{"hidecreature", GS::HideCreature, 0},
	{"hidegui", GS::HideGUI, 0},
	{"incinternal", GS::IncInternal, 0}, //pst
	{"incrementinternal", GS::IncInternal, 0},//iwd
	{"incmoraleai", GS::IncMoraleAI, 0},
	{"incrementchapter", GS::IncrementChapter, AF_BLOCKING},
	{"incrementextraproficiency", GS::IncrementExtraProficiency, 0},
	{"incrementglobal", GS::IncrementGlobal,AF_MERGESTRINGS},
	{"incrementglobalonce", GS::IncrementGlobalOnce,AF_MERGESTRINGS},
	{"incrementkillstat", GS::IncrementKillStat, 0},
	{"incrementproficiency", GS::IncrementProficiency, 0},
	{"interact", GS::Interact, 0},
	{"joinparty", GS::JoinParty, 0}, //this action appears to be blocking in bg2
	{"journalentrydone", GS::SetQuestDone, 0},
	{"jumptoobject", GS::JumpToObject, 0},
	{"jumptopoint", GS::JumpToPoint, 0},
	{"jumptopointinstant", GS::JumpToPointInstant, 0},
	{"jumptosavedlocation", GS::JumpToSavedLocation, 0},
	{"kill", GS::Kill, 0},
	{"killfloatmessage", GS::KillFloatMessage, 0},
	{"leader", GS::Leader, AF_ALIVE},
	{"leavearea", GS::LeaveAreaLUA, 0}, //so far the same
	{"leavearealua", GS::LeaveAreaLUA, 0},
	{"leavearealuaentry", GS::LeaveAreaLUAEntry,AF_BLOCKING},
	{"leavearealuapanic", GS::LeaveAreaLUAPanic, 0},
	{"leavearealuapanicentry", GS::LeaveAreaLUAPanicEntry,AF_BLOCKING},
	{"leaveparty", GS::LeaveParty, 0},
	{"lock", GS::Lock, 0},//key not checked at this time!
	{"lockscroll", GS::LockScroll, 0},
	{"log", GS::Debug, 0}, //the same until we know better
	{"makeglobal", GS::MakeGlobal, 0},
	{"makeunselectable", GS::MakeUnselectable, 0},
	{"markobject", GS::MarkObject, 0},
	{"markspellandobject", GS::MarkSpellAndObject, 0},
	{"moraledec", GS::MoraleDec, 0},
	{"moraleinc", GS::MoraleInc, 0},
	{"moraleset", GS::MoraleSet, 0},
	{"matchhp", GS::MatchHP, 0},
	{"movebetweenareas", GS::MoveBetweenAreas, 0},
	{"movebetweenareaseffect", GS::MoveBetweenAreas, 0},
	{"movecursorpoint", GS::MoveCursorPoint, 0},//immediate move
	{"moveglobal", GS::MoveGlobal, 0},
	{"moveglobalobject", GS::MoveGlobalObject, 0},
	{"moveglobalobjectoffscreen", GS::MoveGlobalObjectOffScreen, 0},
	{"moveglobalsto", GS::MoveGlobalsTo, 0},
	{"transferinventory", GS::MoveInventory, 0},
	{"movetocenterofscreen", GS::MoveToCenterOfScreen,AF_BLOCKING},
	{"movetoexpansion", GS::MoveToExpansion,AF_BLOCKING},
	{"movetoobject", GS::MoveToObject,AF_BLOCKING|AF_ALIVE},
	{"movetoobjectfollow", GS::MoveToObjectFollow,AF_BLOCKING|AF_ALIVE},
	{"movetoobjectnointerrupt", GS::MoveToObjectNoInterrupt,AF_BLOCKING|AF_ALIVE},
	{"movetoobjectuntilsee", GS::MoveToObjectUntilSee,AF_BLOCKING|AF_ALIVE},
	{"movetooffset", GS::MoveToOffset,AF_BLOCKING|AF_ALIVE},
	{"movetopoint", GS::MoveToPoint,AF_BLOCKING|AF_ALIVE},
	{"movetopointnointerrupt", GS::MoveToPointNoInterrupt,AF_BLOCKING|AF_ALIVE},
	{"movetopointnorecticle", GS::MoveToPointNoRecticle,AF_BLOCKING|AF_ALIVE},//the same until we know better
	{"movetosavedlocation", GS::MoveToSavedLocation,AF_MERGESTRINGS|AF_BLOCKING},
	//take care of the typo in the original bg2 action.ids
	//FIXME: why doesn't this have MERGESTRINGS like the above entry?
	{"movetosavedlocationn", GS::MoveToSavedLocation,AF_BLOCKING},
	{"moveviewobject", GS::MoveViewObject, AF_BLOCKING},
	{"moveviewpoint", GS::MoveViewPoint, AF_BLOCKING},
	{"moveviewpointuntildone", GS::MoveViewPoint, 0},
	{"nidspecial1", GS::NIDSpecial1,AF_BLOCKING|AF_DIRECT|AF_ALIVE},//we use this for dialogs, hack
	{"nidspecial2", GS::NIDSpecial2,AF_BLOCKING},//we use this for worldmap, another hack
	{"nidspecial3", GS::Attack,AF_BLOCKING|AF_DIRECT|AF_ALIVE},//this hack is for attacking preset target
	{"nidspecial4", GS::ProtectObject,AF_BLOCKING|AF_DIRECT|AF_ALIVE},
	{"nidspecial5", GS::UseItem, AF_BLOCKING|AF_DIRECT|AF_ALIVE},
	{"nidspecial6", GS::Spell, AF_BLOCKING|AF_DIRECT|AF_ALIVE},
	{"nidspecial7", GS::SpellNoDec, AF_BLOCKING|AF_DIRECT|AF_ALIVE},
	//{"nidspecial8", GS::SpellPoint, AF_BLOCKING|AF_ALIVE}, //not needed
	{"nidspecial9", GS::ToggleDoor, AF_BLOCKING},//another internal hack, maybe we should use UseDoor instead
	{"noaction", GS::NoAction, 0},
	{"opendoor", GS::OpenDoor,0},
	{"panic", GS::Panic, AF_ALIVE},
	{"permanentstatchange", GS::PermanentStatChange, 0}, //pst
	{"pausegame", GS::PauseGame, AF_BLOCKING}, //this is almost surely blocking
	{"picklock", GS::PickLock,AF_BLOCKING},
	{"pickpockets", GS::PickPockets, AF_BLOCKING},
	{"pickupitem", GS::PickUpItem, 0},
	{"playbardsong", GS::PlayBardSong, AF_ALIVE},
	{"playdead", GS::PlayDead,AF_BLOCKING|AF_ALIVE},
	{"playdeadinterruptable", GS::PlayDeadInterruptable,AF_BLOCKING|AF_ALIVE},
	{"playerdialog", GS::PlayerDialogue,AF_BLOCKING},
	{"playerdialogue", GS::PlayerDialogue,AF_BLOCKING},
	{"playsequence", GS::PlaySequence, 0},
	{"playsequenceglobal", GS::PlaySequenceGlobal, 0}, //pst
	{"playsequencetimed", GS::PlaySequenceTimed, 0},//pst
	{"playsong", GS::StartSong, 0},
	{"playsound", GS::PlaySound, 0},
	{"playsoundnotranged", GS::PlaySoundNotRanged, 0},
	{"playsoundpoint", GS::PlaySoundPoint, 0},
	{"plunder", GS::Plunder,AF_BLOCKING|AF_ALIVE},
	{"polymorph", GS::Polymorph, 0},
	{"polymorphcopy", GS::PolymorphCopy, 0},
	{"polymorphcopybase", GS::PolymorphCopyBase, 0},
	{"protectobject", GS::ProtectObject, 0},
	{"protectpoint", GS::ProtectPoint, AF_BLOCKING},
	{"quitgame", GS::QuitGame, 0},
	{"randomfly", GS::RandomFly, AF_BLOCKING|AF_ALIVE},
	{"randomrun", GS::RandomRun, AF_BLOCKING|AF_ALIVE},
	{"randomturn", GS::RandomTurn, AF_BLOCKING},
	{"randomwalk", GS::RandomWalk, AF_BLOCKING|AF_ALIVE},
	{"randomwalkcontinuous", GS::RandomWalkContinuous, AF_BLOCKING|AF_ALIVE},
	{"realsetglobaltimer", GS::RealSetGlobalTimer,AF_MERGESTRINGS},
	{"reallyforcespell", GS::ReallyForceSpell, AF_BLOCKING},
	{"reallyforcespelldead", GS::ReallyForceSpellDead, AF_BLOCKING},
	{"reallyforcespelllevel", GS::ReallyForceSpell, AF_BLOCKING},//this is the same action
	{"reallyforcespellpoint", GS::ReallyForceSpellPoint, AF_BLOCKING},
	{"recoil", GS::Recoil, AF_ALIVE},
	{"regainpaladinhood", GS::RegainPaladinHood, 0},
	{"regainrangerhood", GS::RegainRangerHood, 0},
	{"removeareaflag", GS::RemoveAreaFlag, 0},
	{"removeareatype", GS::RemoveAreaType, 0},
	{"removejournalentry", GS::RemoveJournalEntry, 0},
	{"removemapnote", GS::RemoveMapnote, 0},
	{"removepaladinhood", GS::RemovePaladinHood, 0},
	{"removerangerhood", GS::RemoveRangerHood, 0},
	{"removespell", GS::RemoveSpell, 0},
	{"removetraps", GS::RemoveTraps, AF_BLOCKING},
	{"reputationinc", GS::ReputationInc, 0},
	{"reputationset", GS::ReputationSet, 0},
	{"resetfogofwar", GS::UndoExplore, 0}, //pst
	{"rest", GS::Rest, AF_ALIVE},
	{"restnospells", GS::RestNoSpells, 0},
	{"restorepartylocations", GS:: RestorePartyLocation, 0},
	{"restparty", GS::RestParty, 0},
	{"restuntilhealed", GS::RestUntilHealed, 0},
	//this is in iwd2, same as movetosavedlocation, but with stats
	{"returntosavedlocation", GS::ReturnToSavedLocation, AF_BLOCKING|AF_ALIVE},
	{"returntosavedlocationdelete", GS::ReturnToSavedLocationDelete, AF_BLOCKING|AF_ALIVE},
	{"returntosavedplace", GS::ReturnToSavedLocation, AF_BLOCKING|AF_ALIVE},
	{"revealareaonmap", GS::RevealAreaOnMap, 0},
	{"runawayfrom", GS::RunAwayFrom,AF_BLOCKING|AF_ALIVE},
	{"runawayfromnointerrupt", GS::RunAwayFromNoInterrupt,AF_BLOCKING|AF_ALIVE},
	{"runawayfromnoleavearea", GS::RunAwayFromNoLeaveArea,AF_BLOCKING|AF_ALIVE},
	{"runawayfrompoint", GS::RunAwayFromPoint,AF_BLOCKING|AF_ALIVE},
	{"runfollow", GS::RunAwayFrom,AF_BLOCKING|AF_ALIVE},
	{"runningattack", GS::RunningAttack,AF_BLOCKING|AF_ALIVE},
	{"runningattacknosound", GS::RunningAttackNoSound,AF_BLOCKING|AF_ALIVE},
	{"runtoobject", GS::RunToObject,AF_BLOCKING|AF_ALIVE},
	{"runtopoint", GS::RunToPoint,AF_BLOCKING},
	{"runtopointnorecticle", GS::RunToPointNoRecticle,AF_BLOCKING|AF_ALIVE},
	{"runtosavedlocation", GS::RunToSavedLocation,AF_BLOCKING|AF_ALIVE},
	{"savegame", GS::SaveGame, 0},
	{"savelocation", GS::SaveLocation, 0},
	{"saveplace", GS::SaveLocation, 0},
	{"saveobjectlocation", GS::SaveObjectLocation, 0},
	{"screenshake", GS::ScreenShake,AF_BLOCKING},
	{"selectweaponability", GS::SelectWeaponAbility, 0},
	{"sendtrigger", GS::SendTrigger, 0},
	{"setanimstate", GS::PlaySequence, AF_ALIVE},//pst
	{"setapparentnamestrref", GS::SetApparentName, 0},
	{"setareaflags", GS::SetAreaFlags, 0},
	{"setarearestflag", GS::SetAreaRestFlag, 0},
	{"setbeeninpartyflags", GS::SetBeenInPartyFlags, 0},
	{"setbestweapon", GS::SetBestWeapon, 0},
	{"setcorpseenabled", GS::AmbientActivate, 0},//another weird name
	{"setcutsceneline", GS::SetCursorState, 0}, //same as next
	{"setcursorstate", GS::SetCursorState, 0},
	{"setcreatureareaflag", GS::SetCreatureAreaFlag, 0},
	{"setcriticalpathobject", GS::SetCriticalPathObject, 0},
	{"setdialog", GS::SetDialogue,0},
	{"setdialogrange", GS::SetDialogueRange, 0},
	{"setdialogue", GS::SetDialogue,0},
	{"setdialoguerange", GS::SetDialogueRange, 0},
	{"setdoorflag", GS::SetDoorFlag,0},
	{"setdoorlocked", GS::SetDoorLocked,0},
	{"setencounterprobability", GS::SetEncounterProbability,0},
	{"setextendednight", GS::SetExtendedNight, 0},
	{"setfaction", GS::SetFaction, 0},
	{"setgabber", GS::SetGabber, 0},
	{"setglobal", GS::SetGlobal,AF_MERGESTRINGS},
	{"setglobalrandom", GS::SetGlobalRandom, AF_MERGESTRINGS},
	{"setglobaltimer", GS::SetGlobalTimer,AF_MERGESTRINGS},
	{"setglobaltimeronce", GS::SetGlobalTimerOnce,AF_MERGESTRINGS},
	{"setglobaltimerrandom", GS::SetGlobalTimerRandom,AF_MERGESTRINGS},
	{"setglobaltint", GS::SetGlobalTint, 0},
	{"sethomelocation", GS::SetSavedLocation, 0}, //bg2
	{"sethp", GS::SetHP, 0},
	{"sethppercent", GS::SetHPPercent, 0},
	{"setinternal", GS::SetInternal, 0},
	{"setinterrupt", GS::SetInterrupt, 0},
	{"setleavepartydialogfile", GS::SetLeavePartyDialogFile, 0},
	{"setleavepartydialoguefile", GS::SetLeavePartyDialogFile, 0},
	{"setmarkedspell", GS::SetMarkedSpell, 0},
	{"setmasterarea", GS::SetMasterArea, 0},
	{"setmazeeasier", GS::SetMazeEasier, 0}, //pst specific crap
	{"setmazeharder", GS::SetMazeHarder, 0}, //pst specific crap
	{"setmoraleai", GS::SetMoraleAI, 0},
	{"setmusic", GS::SetMusic, 0},
	{"setname", GS::SetApparentName, 0},
	{"setnamelessclass", GS::SetNamelessClass, 0},
	{"setnamelessdeath", GS::SetNamelessDeath, 0},
	{"setnamelessdisguise", GS::SetNamelessDisguise, 0},
	{"setnooneontrigger", GS::SetNoOneOnTrigger, 0},
	{"setnumtimestalkedto", GS::SetNumTimesTalkedTo, 0},
	{"setplayersound", GS::SetPlayerSound, 0},
	{"setquestdone", GS::SetQuestDone, 0},
	{"setregularnamestrref", GS::SetRegularName, 0},
	{"setrestencounterchance", GS::SetRestEncounterChance, 0},
	{"setrestencounterprobabilityday", GS::SetRestEncounterProbabilityDay, 0},
	{"setrestencounterprobabilitynight", GS::SetRestEncounterProbabilityNight, 0},
	{"setsavedlocation", GS::SetSavedLocation, 0},
	{"setsavedlocationpoint", GS::SetSavedLocationPoint, 0},
	{"setscriptname", GS::SetScriptName, 0},
	{"setselection", GS::SetSelection, 0},
	{"setsequence", GS::PlaySequence, 0}, //bg2 (only own)
	{"setstartpos", GS::SetStartPos, 0},
	{"setteam", GS::SetTeam, 0},
	{"setteambit", GS::SetTeamBit, 0},
	{"settextcolor", GS::SetTextColor, 0},
	{"settrackstring", GS::SetTrackString, 0},
	{"settoken", GS::SetToken, 0},
	{"settoken2da", GS::SetToken2DA, 0}, //GemRB specific
	{"settokenglobal", GS::SetTokenGlobal,AF_MERGESTRINGS},
	{"settokenobject", GS::SetTokenObject,0},
	{"setupwish", GS::SetupWish, 0},
	{"setupwishobject", GS::SetupWishObject, 0},
	{"setvisualrange", GS::SetVisualRange, 0},
	{"sg", GS::SG, 0},
	{"shout", GS::Shout, 0},
	{"sinisterpoof", GS::CreateVisualEffect, 0},
	{"smallwait", GS::SmallWait,AF_BLOCKING},
	{"smallwaitrandom", GS::SmallWaitRandom,AF_BLOCKING},
	{"soundactivate", GS::SoundActivate, 0},
	{"spawnptactivate", GS::SpawnPtActivate, 0},
	{"spawnptdeactivate", GS::SpawnPtDeactivate, 0},
	{"spawnptspawn", GS::SpawnPtSpawn, 0},
	{"spell", GS::Spell, AF_BLOCKING|AF_ALIVE},
	{"spellcasteffect", GS::SpellCastEffect, 0},
	{"spellhiteffectpoint", GS::SpellHitEffectPoint, 0},
	{"spellhiteffectsprite", GS::SpellHitEffectSprite, 0},
	{"spellnodec", GS::SpellNoDec, AF_BLOCKING|AF_ALIVE},
	{"spellpoint", GS::SpellPoint, AF_BLOCKING|AF_ALIVE},
	{"spellpointnodec", GS::SpellPointNoDec, AF_BLOCKING|AF_ALIVE},
	{"startcombatcounter", GS::StartCombatCounter, 0},
	{"startcutscene", GS::StartCutScene, 0},
	{"startcutsceneex", GS::StartCutScene, 0}, //pst (unknown)
	{"startcutscenemode", GS::StartCutSceneMode, 0},
	{"startdialog", GS::StartDialogue,AF_BLOCKING},
	{"startdialoginterrupt", GS::StartDialogueInterrupt,AF_BLOCKING},
	{"startdialogue", GS::StartDialogue,AF_BLOCKING},
	{"startdialogueinterrupt", GS::StartDialogueInterrupt,AF_BLOCKING},
	{"startdialognoname", GS::StartDialogue,AF_BLOCKING},
	{"startdialognoset", GS::StartDialogueNoSet,AF_BLOCKING},
	{"startdialognosetinterrupt", GS::StartDialogueNoSetInterrupt,AF_BLOCKING},
	{"startdialogoverride", GS::StartDialogueOverride,AF_BLOCKING},
	{"startdialogoverrideinterrupt", GS::StartDialogueOverrideInterrupt,AF_BLOCKING},
	{"startdialoguenoname", GS::StartDialogue,AF_BLOCKING},
	{"startdialoguenoset", GS::StartDialogueNoSet,AF_BLOCKING},
	{"startdialoguenosetinterrupt", GS::StartDialogueNoSetInterrupt,AF_BLOCKING},
	{"startdialogueoverride", GS::StartDialogueOverride,AF_BLOCKING},
	{"startdialogueoverrideinterrupt", GS::StartDialogueOverrideInterrupt,AF_BLOCKING},
	{"startmovie", GS::StartMovie,AF_BLOCKING},
	{"startmusic", GS::StartMusic, 0},
	{"startrainnow", GS::StartRainNow, 0},
	{"startrandomtimer", GS::StartRandomTimer, 0},
	{"startsong", GS::StartSong, 0},
	{"startstore", GS::StartStore, 0},
	{"starttimer", GS::StartTimer, 0},
	{"stateoverrideflag", GS::StateOverrideFlag, 0},
	{"stateoverridetime", GS::StateOverrideTime, 0},
	{"staticpalette", GS::StaticPalette, 0},
	{"staticsequence", GS::PlaySequence, 0},//bg2 animation sequence
	{"staticstart", GS::StaticStart, 0},
	{"staticstop", GS::StaticStop, 0},
	{"stickysinisterpoof", GS::CreateVisualEffectObjectSticky, 0},
	{"stopmoving", GS::StopMoving, 0},
	{"storepartylocations", GS::StorePartyLocation, 0},
	{"swing", GS::Swing, AF_ALIVE},
	{"swingonce", GS::SwingOnce, AF_ALIVE},
	{"takeitemlist", GS::TakeItemList, 0},
	{"takeitemlistparty", GS::TakeItemListParty, 0},
	{"takeitemlistpartynum", GS::TakeItemListPartyNum, 0},
	{"takeitemreplace", GS::TakeItemReplace, 0},
	{"takepartygold", GS::TakePartyGold, 0},
	{"takepartyitem", GS::TakePartyItem, 0},
	{"takepartyitemall", GS::TakePartyItemAll, 0},
	{"takepartyitemnum", GS::TakePartyItemNum, 0},
	{"takepartyitemrange", GS::TakePartyItemRange, 0},
	{"teleportparty", GS::TeleportParty, 0},
	{"textscreen", GS::TextScreen, AF_BLOCKING},
	{"timedmovetopoint", GS::TimedMoveToPoint,AF_BLOCKING|AF_ALIVE},
	{"tomsstringdisplayer", GS::DisplayMessage, 0},
	{"transformitem", GS::TransformItem, 0},
	{"transformitemall", GS::TransformItemAll, 0},
	{"transformpartyitem", GS::TransformPartyItem, 0},
	{"transformpartyitemall", GS::TransformPartyItemAll, 0},
	{"triggeractivation", GS::TriggerActivation, 0},
	{"triggerwalkto", GS::MoveToObject,AF_BLOCKING|AF_ALIVE}, //something like this
	{"turn", GS::Turn, 0},
	{"turnamt", GS::TurnAMT, AF_BLOCKING}, //relative Face()
	{"undoexplore", GS::UndoExplore, 0},
	{"unhidegui", GS::UnhideGUI, 0},
	{"unloadarea", GS::UnloadArea, 0},
	{"unlock", GS::Unlock, 0},
	{"unlockscroll", GS::UnlockScroll, 0},
	{"unmakeglobal", GS::UnMakeGlobal, 0}, //this is a GemRB extension
	{"usecontainer", GS::UseContainer,AF_BLOCKING},
	{"usedoor", GS::UseDoor,AF_BLOCKING},
	{"useitem", GS::UseItem,AF_BLOCKING},
	{"useitempoint", GS::UseItemPoint,AF_BLOCKING},
	{"useitempointslot", GS::UseItemPoint,AF_BLOCKING},
	{"useitemslot", GS::UseItem,AF_BLOCKING},
	{"vequip",GS::SetArmourLevel, 0},
	{"verbalconstant", GS::VerbalConstant, 0},
	{"verbalconstanthead", GS::VerbalConstantHead, 0},
	{"wait", GS::Wait, AF_BLOCKING},
	{"waitanimation", GS::WaitAnimation,AF_BLOCKING},//iwd2
	{"waitrandom", GS::WaitRandom, AF_BLOCKING},
	{"weather", GS::Weather, 0},
	{"xequipitem", GS::XEquipItem, 0},
	{ NULL,NULL, 0}
};

//Make this an ordered list, so we could use bsearch!
static const ObjectLink objectnames[] = {
	{"bestac", GS::BestAC},
	{"eighthnearest", GS::EighthNearest},
	{"eighthnearestdoor", GS::EighthNearestDoor},
	{"eighthnearestenemyof", GS::EighthNearestEnemyOf},
	{"eighthnearestenemyoftype", GS::EighthNearestEnemyOfType},
	{"eighthnearestmygroupoftype", GS::EighthNearestEnemyOfType},
	{"eigthnearestenemyof", GS::EighthNearestEnemyOf}, //typo in iwd
	{"eigthnearestenemyoftype", GS::EighthNearestEnemyOfType}, //bg2
	{"eigthnearestmygroupoftype", GS::EighthNearestEnemyOfType},//bg2
	{"farthest", GS::Farthest},
	{"farthestenemyof", GS::FarthestEnemyOf},
	{"fifthnearest", GS::FifthNearest},
	{"fifthnearestdoor", GS::FifthNearestDoor},
	{"fifthnearestenemyof", GS::FifthNearestEnemyOf},
	{"fifthnearestenemyoftype", GS::FifthNearestEnemyOfType},
	{"fifthnearestmygroupoftype", GS::FifthNearestEnemyOfType},
	{"fourthnearest", GS::FourthNearest},
	{"fourthnearestdoor", GS::FourthNearestDoor},
	{"fourthnearestenemyof", GS::FourthNearestEnemyOf},
	{"fourthnearestenemyoftype", GS::FourthNearestEnemyOfType},
	{"fourthnearestmygroupoftype", GS::FourthNearestEnemyOfType},
	{"gabber", GS::Gabber},
	{"groupof", GS::GroupOf},
	{"lastattackerof", GS::LastAttackerOf},
	{"lastcommandedby", GS::LastCommandedBy},
	{"lastheardby", GS::LastHeardBy},
	{"lasthelp", GS::LastHelp},
	{"lasthitter", GS::LastHitter},
	{"lastmarkedobject", GS::LastMarkedObject},
	{"lastseenby", GS::LastSeenBy},
	{"lastsummonerof", GS::LastSummonerOf},
	{"lasttalkedtoby", GS::LastTalkedToBy},
	{"lasttargetedby", GS::LastTargetedBy},
	{"lasttrigger", GS::LastTrigger},
	{"leaderof", GS::LeaderOf},
	{"leastdamagedof", GS::LeastDamagedOf},
	{"marked", GS::LastMarkedObject}, //pst
	{"mostdamagedof", GS::MostDamagedOf},
	{"myself", GS::Myself},
	{"mytarget", GS::MyTarget},//see lasttargetedby(myself)
	{"nearest", GS::Nearest}, //actually this seems broken in IE and resolve as Myself
	{"nearestdoor", GS::NearestDoor},
	{"nearestenemyof", GS::NearestEnemyOf},
	{"nearestenemyoftype", GS::NearestEnemyOfType},
	{"nearestenemysummoned", GS::NearestEnemySummoned},
	{"nearestmygroupoftype", GS::NearestMyGroupOfType},
	{"nearestpc", GS::NearestPC},
	{"ninthnearest", GS::NinthNearest},
	{"ninthnearestdoor", GS::NinthNearestDoor},
	{"ninthnearestenemyof", GS::NinthNearestEnemyOf},
	{"ninthnearestenemyoftype", GS::NinthNearestEnemyOfType},
	{"ninthnearestmygroupoftype", GS::NinthNearestMyGroupOfType},
	{"nothing", GS::Nothing},
	{"player1", GS::Player1},
	{"player1fill", GS::Player1Fill},
	{"player2", GS::Player2},
	{"player2fill", GS::Player2Fill},
	{"player3", GS::Player3},
	{"player3fill", GS::Player3Fill},
	{"player4", GS::Player4},
	{"player4fill", GS::Player4Fill},
	{"player5", GS::Player5},
	{"player5fill", GS::Player5Fill},
	{"player6", GS::Player6},
	{"player6fill", GS::Player6Fill},
	{"player7", GS::Player7},
	{"player7fill", GS::Player7Fill},
	{"player8", GS::Player8},
	{"player8fill", GS::Player8Fill},
	{"protectedby", GS::ProtectedBy},
	{"protectorof", GS::ProtectorOf},
	{"protagonist", GS::Protagonist},
	{"secondnearest", GS::SecondNearest},
	{"secondnearestdoor", GS::SecondNearestDoor},
	{"secondnearestenemyof", GS::SecondNearestEnemyOf},
	{"secondnearestenemyoftype", GS::SecondNearestEnemyOfType},
	{"secondnearestmygroupoftype", GS::SecondNearestMyGroupOfType},
	{"selectedcharacter", GS::SelectedCharacter},
	{"seventhnearest", GS::SeventhNearest},
	{"seventhnearestdoor", GS::SeventhNearestDoor},
	{"seventhnearestenemyof", GS::SeventhNearestEnemyOf},
	{"seventhnearestenemyoftype", GS::SeventhNearestEnemyOfType},
	{"seventhnearestmygroupoftype", GS::SeventhNearestMyGroupOfType},
	{"sixthnearest", GS::SixthNearest},
	{"sixthnearestdoor", GS::SixthNearestDoor},
	{"sixthnearestenemyof", GS::SixthNearestEnemyOf},
	{"sixthnearestenemyoftype", GS::SixthNearestEnemyOfType},
	{"sixthnearestmygroupoftype", GS::SixthNearestMyGroupOfType},
	{"strongestof", GS::StrongestOf},
	{"strongestofmale", GS::StrongestOfMale},
	{"tenthnearest", GS::TenthNearest},
	{"tenthnearestdoor", GS::TenthNearestDoor},
	{"tenthnearestenemyof", GS::TenthNearestEnemyOf},
	{"tenthnearestenemyoftype", GS::TenthNearestEnemyOfType},
	{"tenthnearestmygroupoftype", GS::TenthNearestMyGroupOfType},
	{"thirdnearest", GS::ThirdNearest},
	{"thirdnearestdoor", GS::ThirdNearestDoor},
	{"thirdnearestenemyof", GS::ThirdNearestEnemyOf},
	{"thirdnearestenemyoftype", GS::ThirdNearestEnemyOfType},
	{"thirdnearestmygroupoftype", GS::ThirdNearestMyGroupOfType},
	{"weakestof", GS::WeakestOf},
	{"worstac", GS::WorstAC},
	{ NULL,NULL}
};

static const IDSLink idsnames[] = {
	{"align", GS::ID_Alignment},
	{"alignmen", GS::ID_Alignment},
	{"alignmnt", GS::ID_Alignment},
	{"class20", GS::ID_AVClass},
	{"class", GS::ID_Class},
	{"classmsk", GS::ID_ClassMask},
	{"ea", GS::ID_Allegiance},
	{"faction", GS::ID_Faction},
	{"gender", GS::ID_Gender},
	{"general", GS::ID_General},
	{"race", GS::ID_Race},
	{"specific", GS::ID_Specific},
	{"subrace", GS::ID_Subrace},
	{"team", GS::ID_Team},
	{ NULL,NULL}
};

static const TriggerDesc* FindTrigger(const char* triggername)
{
	if (!triggername) {
		return NULL;
	}
	int len = strlench( triggername, '(' );
	char* name = strndup(triggername, len);
	TriggerDesc* desc = TriggerRegistry.Find(name);
	free(name);
	return desc;
}

static const ActionLink* FindAction(const char* actionname)
{
	if (!actionname) {
		return NULL;
	}
	int len = strlench( actionname, '(' );
	for (int i = 0; actionnames[i].Name; i++) {
		if (!strnicmp( actionnames[i].Name, actionname, len )) {
			if (!actionnames[i].Name[len]) {
				return actionnames + i;
			}
		}
	}
	return NULL;
}

static const ObjectLink* FindObject(const char* objectname)
{
	if (!objectname) {
		return NULL;
	}
	int len = strlench( objectname, '(' );
	for (int i = 0; objectnames[i].Name; i++) {
		if (!strnicmp( objectnames[i].Name, objectname, len )) {
			if (!objectnames[i].Name[len]) {
				return objectnames + i;
			}
		}
	}
	return NULL;
}

static const IDSLink* FindIdentifier(const char* idsname)
{
	if (!idsname) {
		return NULL;
	}
	int len = (int)strlen( idsname );
	for (int i = 0; idsnames[i].Name; i++) {
		if (!strnicmp( idsnames[i].Name, idsname, len )) {
			return idsnames + i;
		}
	}
	
	printMessage( "GameScript"," ", YELLOW );
	printf( "Couldn't assign ids target: %.*s\n", len, idsname );
	return NULL;
}

void SetScriptDebugMode(int arg)
{
	InDebug=arg;
}



/********************** Targets **********************************/

int Targets::Count() const
{
	return (int)objects.size();
}

targettype *Targets::RemoveTargetAt(targetlist::iterator &m)
{
	m=objects.erase(m);
	if (m!=objects.end() ) {
		return &(*m);
	}
	return NULL;
}

const targettype *Targets::GetLastTarget(int Type)
{
	targetlist::const_iterator m = objects.end();
	while (m--!=objects.begin() ) {
		if ( (Type==-1) || ((*m).actor->Type==Type) ) {
			return &(*(m));
		}
	}
	return NULL;
}

const targettype *Targets::GetFirstTarget(targetlist::iterator &m, int Type)
{
	m=objects.begin();
	while (m!=objects.end() ) {
		if ( (Type!=-1) && ( (*m).actor->Type!=Type)) {
			m++;
			continue;
		}
		return &(*m);
	}
	return NULL;
}

const targettype *Targets::GetNextTarget(targetlist::iterator &m, int Type)
{
	m++;
	while (m!=objects.end() ) {
		if ( (Type!=-1) && ( (*m).actor->Type!=Type)) {
			m++;
			continue;
		}
		return &(*m);
	}
	return NULL;
}

Scriptable *Targets::GetTarget(unsigned int index, int Type)
{
	targetlist::iterator m = objects.begin();
	while(m!=objects.end() ) {
		if ( (Type==-1) || ((*m).actor->Type==Type)) {
			if (!index) {
				return (*m).actor;
			}
			index--;
		}
		m++;
	}
	return NULL;
}

//this stuff should be refined, dead actors are sometimes targetable by script?
void Targets::AddTarget(Scriptable* target, unsigned int distance, int ga_flags)
{
	if (!target) {
		return;
	}

	switch (target->Type) {
	case ST_ACTOR:
		//i don't know if unselectable actors are targetable by script
		//if yes, then remove GA_SELECT
		if (ga_flags) {
			if (!((Actor *) target)->ValidTarget(ga_flags) ) {
				return;
			}
		}
		break;
	case ST_GLOBAL:
		// this doesn't seem a good idea to allow
		return;
	default:
		break;
	}
	targettype Target = {target, distance};
	targetlist::iterator m;
	for (m = objects.begin(); m != objects.end(); ++m) {
		if ( (*m).distance>distance) {
			objects.insert( m, Target);
			return;
		}
	}
	objects.push_back( Target );
}

void Targets::Clear()
{
	objects.clear();
}

/** releasing global memory */
static void CleanupIEScript()
{
	triggersTable.release();
	actionsTable.release();
	objectsTable.release();
	overrideActionsTable.release();
	if (ObjectIDSTableNames)
		free(ObjectIDSTableNames);
	ObjectIDSTableNames = NULL;
}

void printFunction(Holder<SymbolMgr> table, int index)
{
	const char *str = table->GetStringIndex(index);
	int value = table->GetValueIndex(index);

	int len = strchr(str,'(')-str;
	if (len<0) {
		printf("%d %s\n", value, str);
	} else {
		printf("%d %.*s\n", value, len, str);
	}
}

void InitializeIEScript()
{
	std::list<int> missing_triggers;
	std::list<int> missing_actions;
	std::list<int> missing_objects;
	std::list<int>::iterator l;

	PluginMgr::Get()->RegisterCleanup(CleanupIEScript);

	NoCreate = core->HasFeature(GF_NO_NEW_VARIABLES);
	HasKaputz = core->HasFeature(GF_HAS_KAPUTZ);

	InitScriptTables();
	int tT = core->LoadSymbol( "trigger" );
	int aT = core->LoadSymbol( "action" );
	int oT = core->LoadSymbol( "object" );
	int gaT = core->LoadSymbol( "gemact" );
	AutoTable objNameTable("script");
	if (tT < 0 || aT < 0 || oT < 0 || !objNameTable) {
		printMessage( "GameScript","A critical scripting file is missing!\n",LIGHT_RED );
		abort();
	}
	triggersTable = core->GetSymbol( tT );
	actionsTable = core->GetSymbol( aT );
	objectsTable = core->GetSymbol( oT );
	overrideActionsTable = core->GetSymbol( gaT );
	if (!triggersTable || !actionsTable || !objectsTable || !objNameTable) {
		printMessage( "GameScript","A critical scripting file is damaged!\n",LIGHT_RED );
		abort();
	}

	int i;

	/* Loading Script Configuration Parameters */

	ObjectIDSCount = atoi( objNameTable->QueryField() );
	if (ObjectIDSCount<0 || ObjectIDSCount>MAX_OBJECT_FIELDS) {
		printMessage("GameScript","The IDS Count shouldn't be more than 10!\n",LIGHT_RED);
		abort();
	}

	ObjectIDSTableNames = (ieResRef *) malloc( sizeof(ieResRef) * ObjectIDSCount );
	for (i = 0; i < ObjectIDSCount; i++) {
		const char *idsname;
		idsname=objNameTable->QueryField( 0, i + 1 );
		const IDSLink *poi=FindIdentifier( idsname );
		if (poi==NULL) {
			idtargets[i]=NULL;
		}
		else {
			idtargets[i]=poi->Function;
		}
		strnlwrcpy(ObjectIDSTableNames[i], idsname, 8 );
	}
	MaxObjectNesting = atoi( objNameTable->QueryField( 1 ) );
	if (MaxObjectNesting<0 || MaxObjectNesting>MAX_NESTING) {
		printMessage("GameScript","The Object Nesting Count shouldn't be more than 5!\n", LIGHT_RED);
		abort();
	}
	HasAdditionalRect = ( atoi( objNameTable->QueryField( 2 ) ) != 0 );
	ExtraParametersCount = atoi( objNameTable->QueryField( 3 ) );
	HasTriggerPoint = ( atoi( objNameTable->QueryField( 4 ) ) != 0 );
	ObjectFieldsCount = ObjectIDSCount - ExtraParametersCount;

	/* Initializing the Script Engine */

	memset( triggers, 0, sizeof( triggers ) );
	memset( triggerflags, 0, sizeof( triggerflags ) );
	memset( actions, 0, sizeof( actions ) );
	memset( actionflags, 0, sizeof( actionflags ) );
	memset( objects, 0, sizeof( objects ) );

	int j;

	j = triggersTable->GetSize();
	while (j--) {
		i = triggersTable->GetValueIndex( j );
		const TriggerDesc* poi = FindTrigger(triggersTable->GetStringIndex( j ));
		//maybe we should watch for this bit?
		//bool triggerflag = i & 0x4000;
		i &= 0x3fff;
		if (i >= MAX_TRIGGERS) {
			printMessage("GameScript"," ", RED);
			printf("trigger %d (%s) is too high, ignoring\n", i, triggersTable->GetStringIndex( j ) );
			continue;
		}
		if (triggers[i]) {
			if (poi && triggers[i]!=poi->Function) {
				printMessage("GameScript"," ", YELLOW);
				printf("%s is in collision with ", triggersTable->GetStringIndex( j ) );
				printFunction(triggersTable,triggersTable->FindValue(triggersTable->GetValueIndex( j )));
				//printFunction(triggersTable->GetStringIndex(triggersTable->FindValue(triggersTable->GetValueIndex( j )) ));
			} else {
				if (InDebug&ID_TRIGGERS) {
					printMessage("GameScript"," ", WHITE);
					printf("%s is a synonym of ", triggersTable->GetStringIndex( j ) );
					printFunction(triggersTable,triggersTable->FindValue(triggersTable->GetValueIndex( j )));
					//printFunction(triggersTable->GetStringIndex(triggersTable->FindValue(triggersTable->GetValueIndex( j ) ) ) );
				}
			}
			continue; //we already found an alternative
		}
		if (poi == NULL) {
			triggers[i] = NULL;
			triggerflags[i] = 0;
			missing_triggers.push_back(j);
			continue;
		}
		triggers[i] = poi->Function;
		triggerflags[i] = poi->Flags;
	}

	for (l = missing_triggers.begin(); l!=missing_triggers.end();l++) {
		j = *l;
		// found later as a different name
		int ii = triggersTable->GetValueIndex( j ) & 0x3fff;
		if (ii >= MAX_TRIGGERS) {
			continue;
		}
		
		if (InDebug&ID_TRIGGERS) {
			TriggerFunction f = triggers[ii];
			if (f) {
				// This only shows the first instance now.
				TriggerDesc *desc = TriggerRegistry.Find(f);
				if (desc) {
					printMessage("GameScript"," ", WHITE);
					printf("%s is a synonym of %s\n", triggersTable->GetStringIndex( j ), desc->Name );
					break;
				}
				continue;
			}
		}
		printMessage("GameScript","Couldn't assign function to trigger: ", YELLOW);
		printFunction(triggersTable,j);
//->GetStringIndex(j) );
	}

	j = actionsTable->GetSize();
	while (j--) {
		i = actionsTable->GetValueIndex( j );
		if (i >= MAX_ACTIONS) {
			printMessage("GameScript"," ", RED);
			printf("action %d (%s) is too high, ignoring\n", i, actionsTable->GetStringIndex( j ) );
			continue;
		}
		const ActionLink* poi = FindAction( actionsTable->GetStringIndex( j ));
		if (actions[i]) {
			if (poi && actions[i]!=poi->Function) {
				printMessage("GameScript"," ", YELLOW);
				printf("%s is in collision with ", actionsTable->GetStringIndex( j ) );
				printFunction(actionsTable, actionsTable->FindValue(actionsTable->GetValueIndex(j)));
//->GetStringIndex(actionsTable->FindValue(actionsTable->GetValueIndex( j )) ) );
			} else {
				if (InDebug&ID_ACTIONS) {
					printMessage("GameScript"," ", WHITE);
					printf("%s is a synonym of ", actionsTable->GetStringIndex( j ) );
					printFunction(actionsTable, actionsTable->FindValue(actionsTable->GetValueIndex( j )));
//actionsTable->GetStringIndex(actionsTable->FindValue(actionsTable->GetValueIndex( j )) ) );
				}
			}
			continue; //we already found an alternative
		}
		if (poi == NULL) {
			actions[i] = NULL;
			actionflags[i] = 0;
			missing_actions.push_back(j);
			continue;
		}
		actions[i] = poi->Function;
		actionflags[i] = poi->Flags;
	}

	if (overrideActionsTable) {
		/*
		 * we add/replace some actions from gemact.ids
		 * right now you can't print or generate these actions!
		 */
		j = overrideActionsTable->GetSize();
		while (j--) {
			i = overrideActionsTable->GetValueIndex( j );
			if (i >= MAX_ACTIONS) {
				printMessage("GameScript"," ", RED);
				printf("action %d (%s) is too high, ignoring\n", i, overrideActionsTable->GetStringIndex( j ) );
				continue;
			}
			const ActionLink *poi = FindAction( overrideActionsTable->GetStringIndex( j ));
			if (!poi) {
				continue;
			}
			if (actions[i]) {
				printMessage("GameScript"," ", WHITE);
				printf("%s overrides existing action ", overrideActionsTable->GetStringIndex( j ) );
				printFunction( actionsTable, actionsTable->FindValue(overrideActionsTable->GetValueIndex( j )));
				//printFunction( actionsTable->GetStringIndex(actionsTable->FindValue(overrideActionsTable->GetValueIndex( j )) ) );
			}
			actions[i] = poi->Function;
			actionflags[i] = poi->Flags;
		}
	}

	for (l = missing_actions.begin(); l!=missing_actions.end();l++) {
		j = *l;
		// found later as a different name
		int ii = actionsTable->GetValueIndex( j );
		if (ii>=MAX_ACTIONS) {
			continue;
		}

		ActionFunction f = actions[ii];
		if (f) {
			for (i = 0; actionnames[i].Name; i++) {
				if (f == actionnames[i].Function) {
					if (InDebug&ID_ACTIONS) {
						printMessage("GameScript"," ", WHITE);
						printf("%s is a synonym of %s\n", actionsTable->GetStringIndex( j ), actionnames[i].Name );
						break;
					}
				}
			}
			continue;
		}
		printMessage("GameScript","Couldn't assign function to action: ", YELLOW);
		printFunction(actionsTable,j);
		//printFunction(actionsTable->GetStringIndex(j) );
	}

	j = objectsTable->GetSize();
	while (j--) {
		i = objectsTable->GetValueIndex( j );
		if (i >= MAX_OBJECTS) {
			printMessage("GameScript"," ", RED);
			printf("object %d (%s) is too high, ignoring\n", i, objectsTable->GetStringIndex( j ) );
			continue;
		}
		const ObjectLink* poi = FindObject( objectsTable->GetStringIndex( j ));
		if (objects[i]) {
			if (poi && objects[i]!=poi->Function) {
				printMessage("GameScript"," ", YELLOW);
				printf("%s is in collision with ", objectsTable->GetStringIndex( j ) );
				printFunction(objectsTable,objectsTable->FindValue(objectsTable->GetValueIndex( j )));
				//printFunction(objectsTable->GetStringIndex(objectsTable->FindValue(objectsTable->GetValueIndex( j )) ) );
			} else {
				printMessage("GameScript"," ", WHITE);
				printf("%s is a synonym of ", objectsTable->GetStringIndex( j ) );
				printFunction(objectsTable, objectsTable->FindValue(objectsTable->GetValueIndex( j )));
				//printFunction(objectsTable->GetStringIndex(objectsTable->FindValue(objectsTable->GetValueIndex( j )) ) );
			}
			continue;
		}
		if (poi == NULL) {
			objects[i] = NULL;
			missing_objects.push_back(j);
		} else {
			objects[i] = poi->Function;
		}
	}

	for (l = missing_objects.begin(); l!=missing_objects.end();l++) {
		j = *l;
		// found later as a different name
		int ii = objectsTable->GetValueIndex( j );
		if (ii>=MAX_ACTIONS) {
			continue;
		}

		ObjectFunction f = objects[ii];
		if (f) {
			for (i = 0; objectnames[i].Name; i++) {
				if (f == objectnames[i].Function) {
					printMessage("GameScript"," ", WHITE);
					printf("%s is a synonym of %s\n", objectsTable->GetStringIndex( j ), objectnames[i].Name );
					break;
				}
			}
			continue;
		}
		printMessage("GameScript","Couldn't assign function to object: ", YELLOW);
		printFunction(objectsTable,j);
		//printFunction(objectsTable->GetStringIndex(j) );
	}

	int instantTableIndex = core->LoadSymbol("instant");
	if (instantTableIndex < 0) {
		printMessage("GameScript", "Couldn't find instant symbols!\n", LIGHT_RED);
		abort();
	}
	Holder<SymbolMgr> instantTable = core->GetSymbol(instantTableIndex);
	if (!instantTable) {
		printMessage("GameScript", "Couldn't load instant symbols!\n", LIGHT_RED);
		abort();
	}
	j = instantTable->GetSize();
	while (j--) {
		i = instantTable->GetValueIndex( j );
		if (i >= MAX_ACTIONS) {
			printMessage("GameScript"," ", RED);
			printf("instant action %d (%s) is too high, ignoring\n", i, instantTable->GetStringIndex( j ) );
			continue;
		}
		if (!actions[i]) {
			printMessage("GameScript"," ", YELLOW);
			printf("instant action %d (%s) doesn't exist, ignoring\n", i, instantTable->GetStringIndex( j ) );
			continue;
		}
		actionflags[i] |= AF_INSTANT;
	}
}

/********************** GameScript *******************************/
GameScript::GameScript()
{
	MySelf = NULL;
	scriptlevel = 0;
	Name[0] = 0;
}

void GameScript::SetStuff(const ieResRef Name, Scriptable* MySelf, int ScriptLevel)
{
	this->MySelf = MySelf;
	strnlwrcpy(this->Name, Name, 8);
	this->scriptlevel = ScriptLevel;
}

void GameScript::ExecuteString(Scriptable* Sender, char* String)
{
	if (String[0] == 0) {
		return;
	}
	Holder<Action> act = GenerateAction( String );
	if (!act) {
		return;
	}
	Sender->AddActionInFront(act);
}

//This must return integer because Or(3) returns 3
int GameScript::EvaluateString(Scriptable* Sender, char* String)
{
	if (String[0] == 0) {
		return 0;
	}
	Trigger* tri = GenerateTrigger( String );
	if (tri) {
		int ret = tri->Evaluate(Sender);
		delete tri;
		return ret;
	}
	return 0;
}

bool Condition::Evaluate(Scriptable* Sender)
{
	int ORcount = 0;
	unsigned int result = 0;
	bool subresult = true;

	for (size_t i = 0; i < triggers.size(); i++) {
		Trigger* tR = triggers[i];
		//do not evaluate triggers in an Or() block if one of them
		//was already True()
		if (!ORcount || !subresult) {
			result = tR->Evaluate(Sender);
		}
		if (result > 1) {
			//we started an Or() block
			if (ORcount) {
				printMessage( "GameScript","Unfinished OR block encountered!\n",YELLOW );
			}
			ORcount = result;
			subresult = false;
			continue;
		}
		if (ORcount) {
			subresult |= ( result != 0 );
			if (--ORcount) {
				continue;
			}
			result = subresult;
		}
		if (!result) {
			return 0;
		}
	}
	if (ORcount) {
		printMessage( "GameScript","Unfinished OR block encountered!\n",YELLOW );
	}
	return 1;
}

/* this may return more than a boolean, in case of Or(x) */
int Trigger::Evaluate(Scriptable* Sender)
{
	if (!this) {
		printMessage( "GameScript","Trigger evaluation fails due to NULL trigger.\n",LIGHT_RED );
		return 0;
	}
	TriggerFunction func = triggers[triggerID];
	const char *tmpstr=triggersTable->GetValue(triggerID);
	if (!tmpstr) {
		tmpstr=triggersTable->GetValue(triggerID|0x4000);
	}
	if (!func) {
		triggers[triggerID] = GS::False;
		printMessage("GameScript"," ",YELLOW);
		printf("Unhandled trigger code: 0x%04x %s\n",
			triggerID, tmpstr );
		return 0;
	}
	if (InDebug&ID_TRIGGERS) {
		printMessage("GameScript"," ",YELLOW);
		printf( "Executing trigger code: 0x%04x %s\n",
				triggerID, tmpstr );
	}
	int ret = func( Sender, this );
	if (flags & NEGATE_TRIGGER) {
		return !ret;
	}
	return ret;
}

void PrintAction(int actionID)
{
	printf("Action: %d %s\n", actionID , actionsTable->GetValue(actionID) );
}

void GameScript::ExecuteAction(Scriptable* Sender, Holder<Action> aC)
{
	int actionID = aC->actionID;

	if (aC->objects[0]) {
		Scriptable *scr = GetActorFromObject(Sender, aC->objects[0]);

		Sender->ReleaseCurrentAction();

		if (scr) {
			if (InDebug&ID_ACTIONS) {
				printMessage("GameScript"," ",YELLOW);
				printf("Sender: %s-->override: %s\n",Sender->GetScriptName(), scr->GetScriptName() );
			}
			scr->ReleaseCurrentAction();
			scr->AddAction(ParamCopyNoOverride(aC));
			if (!(actionflags[actionID] & AF_INSTANT)) {
				assert(scr->GetNextAction());
				// TODO: below was written before i added instants, this might be unnecessary now

				// there are plenty of places where it's vital that ActionOverride is not interrupted and if
				// there are actions left on the queue after the release above, we can't instant-execute,
				// so this is my best guess for now..
				scr->CurrentActionInterruptable = false;
			}
		} else {
			printMessage("GameScript","Actionoverride failed for object: \n",LIGHT_RED);
			aC->objects[0]->Dump();
		}
		return;
	}
	if (InDebug&ID_ACTIONS) {
		printMessage("GameScript"," ",YELLOW);
		PrintAction(actionID);
		printf("Sender: %s\n",Sender->GetScriptName() );
	}
	ActionFunction func = actions[actionID];
	if (func) {
		//turning off interruptable flag
		//uninterruptable actions will set it back
		if (Sender->Type==ST_ACTOR) {
			Sender->Activate();
			if (actionflags[actionID]&AF_ALIVE) {
				if (Sender->GetInternalFlag()&IF_STOPATTACK) {
					printMessage("GameScript", "Aborted action due to death\n", YELLOW);
					Sender->ReleaseCurrentAction();
					return;
				}
			}
		}
		func( Sender, aC.get() );
	} else {
		actions[actionID] = GS::NoActionAtAll;
		printMessage("GameScript", "Unknown ", YELLOW);
		textcolor(YELLOW);
		PrintAction(actionID);
		Sender->ReleaseCurrentAction();
		textcolor(WHITE);
		return;
	}

	//don't bother with special flow control actions
	if (actionflags[actionID] & AF_IMMEDIATE) {
		return;
	}

	//Releasing nonblocking actions, blocking actions will release themselves
	if (!( actionflags[actionID] & AF_BLOCKING )) {
		Sender->ReleaseCurrentAction();
		//aC is invalid beyond this point, so we return!
		return;
	}
}

Trigger* GenerateTrigger(char* String)
{
	strlwr( String );
	if (InDebug&ID_TRIGGERS) {
		printMessage("GameScript"," ",YELLOW);
		printf("Compiling:%s\n",String);
	}
	int negate = 0;
	if (*String == '!') {
		String++;
		negate = 1;
	}
	int len = strlench(String,'(')+1; //including (
	int i = triggersTable->FindString(String, len);
	if (i<0) {
		printMessage("GameScript"," ",LIGHT_RED);
		printf("Invalid scripting trigger: %s\n", String);
		return NULL;
	}
	char *src = String+len;
	char *str = triggersTable->GetStringIndex( i )+len;
	Trigger *trigger = GenerateTriggerCore(src, str, i, negate);
	if (!trigger) {
		printMessage("GameScript"," ",LIGHT_RED);
		printf("Malformed scripting trigger: %s\n", String);
		return NULL;
	}
	return trigger;
}

Holder<Action> GenerateAction(char* String)
{
	strlwr( String );
	if (InDebug&ID_ACTIONS) {
		printMessage("GameScript"," ",YELLOW);
		printf("Compiling:%s\n",String);
	}
	int len = strlench(String,'(')+1; //including (
	char *src = String+len;
	int i = -1;
	char *str;
	unsigned short actionID;
	if (overrideActionsTable) {
		i = overrideActionsTable->FindString(String, len);
		if (i >= 0) {
			str = overrideActionsTable->GetStringIndex( i )+len;
			actionID = overrideActionsTable->GetValueIndex(i);
		}
	}
	if (i<0) {
		i = actionsTable->FindString(String, len);
		if (i < 0) {
			printMessage("GameScript"," ",LIGHT_RED);
			printf("Invalid scripting action: %s\n", String);
			return NULL;
		}
		str = actionsTable->GetStringIndex( i )+len;
		actionID = actionsTable->GetValueIndex(i);
	}
	Holder<Action> action = GenerateActionCore( src, str, actionID);
	if (!action) {
		printMessage("GameScript"," ",LIGHT_RED);
		printf("Malformed scripting action: %s\n", String);
		return NULL;
	}
	return action;
}

Holder<Action> GenerateActionDirect(char *String, Scriptable *object)
{
	Holder<Action> action = GenerateAction(String);
	Object *tmp = action->objects[1];
	if (tmp && tmp->objectFields[0]==-1) {
		tmp->objectFields[1] = object->GetGlobalID();
	}
	action->pointParameter.empty();
	return action;
}

/** Return true if object is null */
bool Object::isNull()
{
	if (objectName[0]!=0) {
		return false;
	}
	if (objectFilters[0]) {
		return false;
	}
	for (int i=0;i<ObjectFieldsCount;i++) {
		if (objectFields[i]) {
			return false;
		}
	}
	return true;
}
