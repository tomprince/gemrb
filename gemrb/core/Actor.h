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
 *
 */

#ifndef ACTOR_H
#define ACTOR_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <vector>
#include <cstring>
#include "exports.h"
#include "ie_types.h"
#include "Animation.h"
#include "CharAnimations.h"
#include "ScriptedAnimation.h"
#include "ActorBlock.h"
#include "EffectQueue.h"
#include "PCStatStruct.h"

class Map;
class ScriptedAnimation;

/** USING DEFINITIONS AS DESCRIBED IN STATS.IDS */
#include "ie_stats.h"

#include "Inventory.h"
#include "Spellbook.h"

#define MAX_STATS 256
#define MAX_LEVEL 128
#define MAX_FEATS 96   //3*sizeof(ieDword)

//modal states
#define MS_NONE        0
#define MS_BATTLESONG  1
#define MS_DETECTTRAPS 2
#define MS_STEALTH     3
#define MS_TURNUNDEAD  4

//stat modifier type
#define MOD_ADDITIVE  0
#define MOD_ABSOLUTE  1
#define MOD_PERCENT   2

//'do not jump' flags
#define DNJ_FIT        1
#define DNJ_UNHINDERED 2
#define DNJ_JUMP       4
#define DNJ_BIRD       (DNJ_FIT|DNJ_UNHINDERED)

//add_animation flags (override vvc)
#define AA_PLAYONCE    1
#define AA_BLEND       2

//GetDialog flags
#define GD_NORMAL      0
#define GD_CHECK       1
#define GD_FEEDBACK    2 //(also check)

/** flags for GetActor */
//default action
#define GA_DEFAULT  0
//actor selected for talk
#define GA_TALK     1
//actor selected for attack
#define GA_ATTACK   2
//actor selected for spell target
#define GA_SPELL    3
//actor selected for defend
#define GA_DEFEND   4
//actor selected for pick pockets
#define GA_PICK     5
//action mask
#define GA_ACTION   15
//unselectable actor may not be selected (can still block)
#define GA_SELECT   16
//dead actor may not be selected
#define GA_NO_DEAD  32
//any point could be selected (area effect)
#define GA_POINT    64
//hidden actor may not be selected
#define GA_NO_HIDDEN 128
//party members cannot be selected
#define GA_NO_ALLY  256
//only party members could be selected
#define GA_NO_ENEMY 512
//
#define GA_NO_NEUTRAL 1024
//cannot target self
#define GA_NO_SELF    2048
//try other areas too
#define GA_GLOBAL     4096

#define GUIBT_COUNT  12

#define VCONST_COUNT 100

//interact types
#define I_INSULT     1
#define I_COMPLIMENT 2
#define I_SPECIAL    3

// 3 for blur, 8 for mirror images
#define EXTRA_ACTORCOVERS 11

//flags for UseItem
#define UI_SILENT    1       //no sound when used up
#define UI_MISS      2       //ranged miss (projectile has no effects)

//used to mask off current profs
#define PROFS_MASK  0x07

//locations of classes in the isclass/levelslots arrays
#define ISFIGHTER   0
#define ISMAGE      1
#define ISTHIEF     2
#define ISBARBARIAN 3
#define ISBARD      4
#define ISCLERIC    5
#define ISDRUID     6
#define ISMONK      7
#define ISPALADIN   8
#define ISRANGER    9
#define ISSORCERER  10
#define ISCLASSES   11

//appearance flags

#define APP_HALFTRANS    2           //half transparent
#define APP_DEATHVAR     16          //set death variable
#define APP_DEATHTYPE    32          //count creature type deaths
//64
#define APP_FACTION      128         //count killed faction
#define APP_TEAM         0x100       //count killed team
#define APP_INVULNERABLE 0x200       //invulnerable
#define APP_GOOD         0x400       //good count
#define APP_LAW          0x800       //law count
#define APP_LADY         0x1000      //lady count
#define APP_MURDER       0x2000      //murder count
#define APP_NOTURN       0x4000      //doesn't face gabber in dialogue
#define APP_BUDDY        0x8000      //npcs will turn hostile if this one dies
#define APP_DEAD         0x40000000  //used by the engine to prevent dying twice

#define DC_GOOD   0
#define DC_LAW    1
#define DC_LADY   2
#define DC_MURDER 3

// used for distinguishing damage immunity from high damage resistance
#define DR_IMMUNE 999999

typedef ieByte ActionButtonRow[GUIBT_COUNT];

typedef std::vector< ScriptedAnimation*> vvcVector;
typedef std::list<ieResRef*> resourceList;

struct WeaponInfo {
	int slot;
	int enchantment;
	unsigned int range;
	ieDword itemflags;
	ieDword prof;
	bool backstabbing;
};

extern void ReleaseMemoryActor();

class GEM_EXPORT Actor : public Movable {
public:
	//CRE DATA FIELDS
	ieDword BaseStats[MAX_STATS];
	ieDword Modified[MAX_STATS];
	ieByteSigned DeathCounters[4];   //PST specific (good, law, lady, murder)
	
	ieResRef applyWhenHittingMelee;  //set melee effect
	ieResRef applyWhenHittingRanged; //set ranged effect
	ieResRef applyWhenNearLiving;    //cast spell on condition
	ieResRef applyWhen50Damage;      //cast spell on condition
	ieResRef applyWhen90Damage;      //cast spell on condition
	ieResRef applyWhenEnemySighted;  //cast spell on condition
	ieResRef applyWhenPoisoned;      //cast spell on condition
	ieResRef applyWhenHelpless;      //cast spell on condition
	ieResRef applyWhenAttacked;      //cast spell on condition
	ieResRef applyWhenBeingHit;      //cast spell on condition
	ieResRef ModalSpell;             //apply this spell once per round

	PCStatsStruct*  PCStats;
	ieResRef SmallPortrait;
	ieResRef LargePortrait;
	/** 0: NPC, 1-8 party slot */
	ieByte InParty;
	char* LongName, * ShortName;
	ieStrRef ShortStrRef, LongStrRef;
	ieStrRef StrRefs[VCONST_COUNT];

	ieDword AppearanceFlags;

	ieVariable KillVar; //this second field is present in pst, iwd1 and iwd2
	ieVariable IncKillVar; // iwd1, iwd2

	ieByte SetDeathVar, IncKillCount, UnknownField; // boolean fields from iwd1 and iwd2

	Inventory inventory;
	ieWordSigned Equipped;         //the equipped weapon slot
	ieWord EquippedHeader;         //the used extended header
	Spellbook spellbook;
	//savefile version (creatures embedded in area)
	int version;
	//in game or area actor header
	ieDword TalkCount;
	ieDword RemovalTime;
	ieDword InteractCount; //this is accessible in iwd2, probably exists in other games too
	ieDword appearance;
	ieDword ModalState;
	ieWord globalID;
	ieWord localID;
	int PathTries; //the # of previous tries to pick up a new walkpath
public:
	#define LastTarget LastDisarmFailed
	//ieDword LastTarget; use lastdisarmfailed
	#define LastAttacker LastDisarmed
	//ieDword LastAttacker; use lastdisarmed
	#define LastHitter LastEntered
	//ieDword LastHitter; use lastentered
	#define LastSummoner LastTrigger
	//ieDword LastSummoner; use lasttrigger
	#define LastTalkedTo LastUnlocked
	//ieDword LastTalkedTo; use lastunlocked
	ieDword LastProtected;
	ieDword LastFollowed;
	ieDword LastCommander;
	ieDword LastHelp;
	ieDword LastSeen;
	ieDword LastMarked; // no idea if non-actors could mark objects
	ieDword LastHeard;
	ieDword HotKey;
	char ShieldRef[2];
	char HelmetRef[2];
	char WeaponRef[2];
	int WeaponType;
	ieDword multiclass;
	bool GotLUFeedback;

	int LastCommand;   //lastcommander
	int LastShout;     //lastheard
	int LastDamage;    //lasthitter
	int LastDamageType;//lasthitter
	Point FollowOffset;//follow lastfollowed at this offset

	class Door *TargetDoor;

	EffectQueue fxqueue;
	vvcVector vvcOverlays;
	vvcVector vvcShields;
	ieDword *projectileImmunity; //classic bitfield
	ieDword roundTime;           //these are timers for attack rounds
	ieDword lastInit;
	bool no_more_steps;
	int speed;
private:
	//this stuff doesn't get saved
	CharAnimations* anims;
	SpriteCover* extraCovers[EXTRA_ACTORCOVERS];
	ieByte SavingThrow[5];
	//how many attacks in this round
	int attackcount;
	int attacksperround;
	//time of our next attack
	ieDword nextattack;
	ieDword lastattack;
	ieDword InTrap;
	char AttackStance ;
	/*The projectile bringing the current attack*/
	Projectile* attackProjectile ;
	/** paint the actor itself. Called internally by Draw() */
	void DrawActorSprite(Region &screen, int cx, int cy, Region& bbox,
				 SpriteCover*& sc, Animation** anims,
				 unsigned char Face, Color& tint);

	/** fixes the palette */
	void SetupColors();
	/** debugging function, gets the scripting name of an actor referenced by a global ID */
	const char* GetActorNameByID(ieDword ID) const;
	/* if Lasttarget is gone, call this */
	void StopAttack();
	/* checks a weapon quick slot and resets it to fist if it is empty */
	void CheckWeaponQuickSlot(unsigned int which);
	/* helper for usability checks */
	int CheckUsability(Item *item) const;
	/* Set up all the missing stats on load time, or after level up */
	void CreateDerivedStatsBG();
	/* Set up all the missing stats on load time, or after level up */
	void CreateDerivedStatsIWD2();
	/* Gets the given ISCLASS level */
	ieDword GetClassLevel (const ieDword id) const;
	/* Returns true if the dual class is backwards */
	bool IsDualSwap() const;
	/** Re/Inits the Modified vector for PCs/NPCs */
	void RefreshPCStats();
	bool ShouldHibernate();
public:
	Actor(void);
	~Actor(void);
	/** releases memory */
	static void ReleaseMemory();
	/** sets game specific parameter (which stat should determine the fist weapon type */
	static void SetFistStat(ieDword stat);
	/** sets game specific default data about action buttons */
	static void SetDefaultActions(int qslot, ieByte slot1, ieByte slot2, ieByte slot3);
	/** prints useful information on console */
	void DumpMaxValues();
	void DebugDump();
	/** fixes the feet circle */
	void SetCircleSize();
	/** places the actor on the map with a unique object ID */
	void SetMap(Map *map, ieWord LID, ieWord GID);
	/** sets the actor's position, calculating with the nojump flag*/
	void SetPosition(const Point &position, int jump, int radius=0);
	/** you better use SetStat, this stuff is only for special cases*/
	void SetAnimationID(unsigned int AnimID);
	/** returns the animations */
	CharAnimations* GetAnims();
	/** Re/Inits the Modified vector */
	void RefreshEffects(EffectQueue *eqfx);
	/** gets saving throws */
	void RollSaves();
	/** returns a saving throw */
	bool GetSavingThrow(ieDword type, int modifier);
	/** Returns true if the actor is targetable */
	bool ValidTarget(int ga_flags) const;
	/** Returns a Stat value */
	ieDword GetStat(unsigned int StatIndex) const;
	/** Sets a Stat Value (unsaved) */
	bool SetStat(unsigned int StatIndex, ieDword Value, int pcf);
	/** Returns the difference */
	int GetMod(unsigned int StatIndex);
	/** Returns a Stat Base Value */
	ieDword GetBase(unsigned int StatIndex);
	/** Sets a Base Stat Value */
	bool SetBase(unsigned int StatIndex, ieDword Value);
	bool SetBaseNoPCF(unsigned int StatIndex, ieDword Value);
	/** set/resets a Base Stat bit */
	bool SetBaseBit(unsigned int StatIndex, ieDword Value, bool setreset);
	/** Sets the modified value in different ways, returns difference */
	int NewStat(unsigned int StatIndex, ieDword ModifierValue, ieDword ModifierType);
	/** Modifies the base stat value in different ways, returns difference */
	int NewBase(unsigned int StatIndex, ieDword ModifierValue, ieDword ModifierType);
	void SetLeader(Actor *actor, int xoffset=0, int yoffset=0);
	ieDword GetID()
	{
		return (localID<<16) | globalID;
	}
	/** Sets the Icon ResRef */
	//Which - 0 both, 1 Large, 2 Small
	void SetPortrait(const char* ResRef, int Which=0);
	void SetSoundFolder(const char *soundset);
	/** Gets the Character Long Name/Short Name */
	char* GetName(int which) const
	{
		if(which==-1) which=TalkCount;
		if (which) {
			return LongName;
		}
		return ShortName;
	}
	/** Gets the DeathVariable */
	const char* GetScriptName(void) const
	{
		return scriptName;
	}
	/** Gets a Script ResRef */
	const char* GetScript(int ScriptIndex) const;
	/** Gets the Character's level for XP calculations */
	ieDword GetXPLevel(int modified) const;
	/** Returns the wild mage casting level modifier */
	int GetWildMod(int level) const;
	/** Returns any casting level modifier */
	int CastingLevelBonus(int level, int type) const;

	/** Gets the Dialog ResRef */
	const char* GetDialog(int flags=GD_NORMAL) const;
	/** Gets the Portrait ResRef */
	const char* GetPortrait(int which) const
	{
		return which ? SmallPortrait : LargePortrait;
	}

	void SetName(const char* ptr, unsigned char type);
	void SetName(int strref, unsigned char type);
	/* returns carried weight atm, could calculate with strength*/
	int GetEncumbrance();
	/* checks on death of actor, returns true if it should be removed*/
	bool CheckOnDeath();
	/* receives undead turning message */
	void Turn(Scriptable *cleric, ieDword turnlevel);
	/* call this on gui selects */
	void SelectActor();
	/* sets the actor in panic (turn/morale break) */
	void Panic();
	/* sets a multi class flag (actually this is a lot of else too) */
	void SetMCFlag(ieDword bitmask, int op);
	/* inlined dialogue start */
	void Interact(int type);
	/* displaying a random verbal constant */
	void VerbalConstant(int start, int count);
	/* inlined dialogue response */
	void Response(int type);
	/* called when someone died in the party */
	void ReactToDeath(const char *deadname);
	/* called when someone talks to Actor */
	void DialogInterrupt();
	/* called when actor was hit */
	void GetHit();
	/* called when actor starts to cast a spell*/
	bool HandleCastingStance(const ieResRef SpellResRef, bool deplete);
	/* deals damage to this actor */
	int Damage(int damage, int damagetype, Scriptable *hitter, int modtype=MOD_ADDITIVE);
	/* displays the damage taken and other details (depends on the game type) */
	void DisplayCombatFeedback (unsigned int damage, int resisted, int damagetype, Actor *hitter);
	/* play the proper hit sound (in pst) */
	void PlayHitSound(DataFileMgr *resdata, int damagetype, bool suffix);
	/* drops items from inventory to current spot */
	void DropItem(const ieResRef resref, unsigned int flags);
	void DropItem(int slot, unsigned int flags);
	/* returns item information in quickitem slot */
	void GetItemSlotInfo(ItemExtHeader *item, int which, int header);
	/* returns spell information in quickspell slot */
	void GetSpellSlotInfo(SpellExtHeader *spell, int which);
	/* updates quickslots */
	void ReinitQuickSlots();
	/* actor is in trap */
	void SetInTrap(ieDword tmp);
	/* sets some of the internal flags */
	void SetRunFlags(ieDword flags);
	/* applies the kit abilities, returns false if kit is not applicable */
	bool ApplyKit(ieDword Value);
	/* calls InitQuickSlot in PCStatStruct */
	void SetupQuickSlot(unsigned int which, int slot, int headerindex);
	/* returns true if the actor is PC/joinable*/
	bool Persistent() const;
	/* assigns actor to party slot, 0 = NPC, areas won't remove it */
	void SetPersistent(int partyslot);
	/* resurrects actor */
	void Resurrect();
	/* removes actor in the next update cycle */
	void DestroySelf();
	/* schedules actor to die */
	void Die(Scriptable *killer);
	/* debug function */
	void GetNextAnimation();
	/* debug function */
	void GetPrevAnimation();
	/* debug function */
	void GetNextStance();
	/* learns the given spell, possibly receive XP */
	int LearnSpell(const ieResRef resref, ieDword flags);
	/* returns the ranged weapon header associated with the currently equipped projectile */
	ITMExtHeader *GetRangedWeapon(WeaponInfo &wi) const;
	/* Returns current weapon range and extended header
	 if range is nonzero, then which is valid */
	ITMExtHeader* GetWeapon(WeaponInfo &wi, bool leftorright=false);
	/* Creates player statistics */
	void CreateStats();
	/* Heals actor by days */
	void Heal(int days);
	/* Receive experience (handle dual/multi class) */
	void AddExperience(int exp);
	/* Receive experience bonus */
	void AddExperience(int type, int level);
	/* Sets the modal state after checks */
	void SetModal(ieDword newstate);
	/* returns current attack style */
	int GetAttackStyle();
	/* sets target for immediate attack */
	void SetTarget( Scriptable *actor);
	/* starts combat round, possibly one more attacks in every second round*/
	void InitRound(ieDword gameTime, bool secondround);
	/* gets the to hit value */
	int GetToHit(int bonus, ieDword Flags);
	/* gets the defense against an attack */
	int GetDefense(int DamageType) ;
	/* get the current hit bonus */
	bool GetCombatDetails(int &tohit, bool leftorright, WeaponInfo &wi, ITMExtHeader *&header, ITMExtHeader *&hittingheader,\
		ieDword &Flags, int &DamageBonus, int &speed, int &CriticalBonus, int &style);
	/* performs attack against target */
	void PerformAttack(ieDword gameTime);
	/* ensures we can deal damage to a target */
	void ModifyDamage(Actor *target, int &damage, int &resisted, int damagetype, WeaponInfo *wi, bool critical);
	/* applies modal spell etc, if needed */
	void UpdateActorState(ieDword gameTime);
	/* sets a colour gradient stat, handles location */
	void SetColor( ieDword idx, ieDword grd);
	/* sets an RGB colour modification effect; location 0xff for global */
	void SetColorMod( ieDword location, RGBModifier::Type type, int speed,
		unsigned char r, unsigned char g, unsigned char b,
		int phase=-1 );
	bool Schedule(ieDword gametime, bool checkhide);
	/* call this when path needs to be changed */
	void NewPath();
	/* overridden method, won't walk if dead */
	void WalkTo(Point &Des, ieDword flags, int MinDistance = 0);
	/* resolve string constant (sound will be altered) */
	void ResolveStringConstant(ieResRef sound, unsigned int index);
	void GetSoundFromINI(ieResRef Sound, unsigned int index);
	void GetSoundFrom2DA(ieResRef Sound, unsigned int index);
	/* sets the quick slots */
	void SetActionButtonRow(ActionButtonRow &ar);
	/* updates the quick slots */
	void GetActionButtonRow(ActionButtonRow &qs);

	/* Handling automatic stance changes */
	bool HandleActorStance();

	/* if necessary, advance animation and draw actor */
	void Draw(Region &screen);

	/* add mobile vvc (spell effects) to actor's list */
	void AddVVCell(ScriptedAnimation* vvc);
	/* remove a vvc from the list, graceful means animated removal */
	void RemoveVVCell(const ieResRef vvcname, bool graceful);
	/* returns true if actor already has the overlay (slow) */
	bool HasVVCCell(const ieResRef resource);
	/* returns the vvc pointer to a hardcoded overlay */
	/* if it exists (faster than hasvvccell) */
	ScriptedAnimation *FindOverlay(int index);
	/* draw videocells */
	void DrawVideocells(Region &screen, vvcVector &vvcCells, Color &tint);

	void SetLockedPalette(const ieDword *gradients);
	void UnlockPalette();
	void AddAnimation(const ieResRef resource, int gradient, int height, int flags);
	/* plays damage animation, if hit is not set, then plays only the splash part */
	void PlayDamageAnimation(int x, bool hit=true);
	/* restores a spell of maximum maxlevel level, type is a mask of disabled spells */
	int RestoreSpellLevel(ieDword maxlevel, ieDword typemask);
	/* rememorizes spells, cures fatigue, etc */
	void Rest(int hours);
	/* returns the portrait icons list */
	const unsigned char *GetStateString();
	/* adds a state icon to the list */
	void AddPortraitIcon(ieByte icon);
	/* disables a state icon in the list, doesn't remove it! */
	void DisablePortraitIcon(ieByte icon);
	/* returns which slot belongs to the quickweapon slot */
	int GetQuickSlot(int slot);
	/* Sets equipped Quick slot, if header is -1, then use the current one */
	int SetEquippedQuickSlot(int slot, int header);
	/* Uses an item on the target or point */
	bool UseItemPoint(ieDword slot, ieDword header, Point &point, ieDword flags);
	bool UseItem(ieDword slot, ieDword header, Scriptable *target, ieDword flags, int damage = 0);
	/* Deducts a charge from an item */
	void ChargeItem(ieDword slot, ieDword header, CREItem *item, Item *itm, bool silent);
	/* If it returns true, then default AC=10 and the lesser the better */
	int IsReverseToHit();
	/* initialize the action buttons based on class. If forced, it will override 
		previously customized or set buttons. */
	void InitButtons(ieDword cls, bool forced);
	void SetFeat(unsigned int feat, int mode);
	int GetFeat(unsigned int feat) const;
	void SetUsedWeapon(const char *AnimationType, ieWord *MeleeAnimation,
		int WeaponType=-1);
	void SetUsedShield(const char *AnimationType, int WeaponType=-1);
	void SetUsedHelmet(const char *AnimationType);
	void SetupFist();
	/* Returns nonzero if the caster is held */
	int Immobile() const;
	/* Returns negative error code if the item is unusable */
	int Unusable(Item *item) const;
	/* Sets all clown colour to the given gradient */
	void SetGradient(ieDword gradient);
	/* Enables an overlay */
	void SetOverlay(unsigned int overlay);
	/* Checks and sets a spellstate if it wasn't set yet */
	bool SetSpellState(unsigned int spellstate);
	/* Checks a spellstate */
	bool HasSpellState(unsigned int spellstate);
	/* Checks a feat */
	bool HasFeat(unsigned int featindex) const;
	/* Reports projectile immunity, nonzero if immune */
	ieDword ImmuneToProjectile(ieDword projectile) const;
	/* Sets projectile immunity */
	void AddProjectileImmunity(ieDword projectile);
	/* Set up all the missing stats on load time, chargen, or after level up */
	void CreateDerivedStats();
	/* Checks if the actor is multiclassed (excluding dualclassed actors)) */
	bool IsMultiClassed() const;
	/* Checks if the actor is dualclassed */
	bool IsDualClassed() const;
	/* Returns an exact copy of this actor */
	Actor *CopySelf() const;
	/* Returns the actor's level of the given class */
	ieDword GetFighterLevel() const { return GetClassLevel(ISFIGHTER); }
	ieDword GetMageLevel() const { return GetClassLevel(ISMAGE); }
	ieDword GetThiefLevel() const { return GetClassLevel(ISTHIEF); }
	ieDword GetBarbarianLevel() const { return GetClassLevel(ISBARBARIAN); }
	ieDword GetBardLevel() const { return GetClassLevel(ISBARD); }
	ieDword GetClericLevel() const { return GetClassLevel(ISCLERIC); }
	ieDword GetDruidLevel() const { return GetClassLevel(ISDRUID); }
	ieDword GetMonkLevel() const { return GetClassLevel(ISMONK); }
	ieDword GetPaladinLevel() const { return GetClassLevel(ISPALADIN); }
	ieDword GetRangerLevel() const { return GetClassLevel(ISRANGER); }
	ieDword GetSorcererLevel() const { return GetClassLevel(ISSORCERER); }
	/* Returns true if the character is a warrior */
	ieDword GetWarriorLevel() const;
	bool IsWarrior() const { return (GetFighterLevel()||GetBarbarianLevel()||GetRangerLevel()||GetPaladinLevel()); }
	/* Returns true if the old class is inactive */
	bool IsDualInactive() const;
	/* true if we are dual-wielding */
	int IsDualWielding() const;
	bool BlocksSearchMap() const;
	bool CannotPassEntrance() const;
	void UseExit(int flag);
	int GetReaction();
	/* Similar to Roll, but takes luck into account */
	int LuckyRoll(int dice, int size, int add, bool critical=1, bool only_damage=0, Actor* opponent=NULL) const;
	/* removes normal invisibility (type 0) */
	void CureInvisibility();
	/* removes sanctuary */
	void CureSanctuary();
	/* resets the invisibility, sanctuary and modal states */
	void ResetState();
	/* checks whether the actor is behind the target */
	bool IsBehind(Actor* target);
};
#endif