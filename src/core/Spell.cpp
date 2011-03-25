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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

//This class represents the .spl (spell) files of the game.

#include "Spell.h"

#include "win32def.h"

#include "Audio.h"
#include "Game.h"
#include "Interface.h"
#include "Projectile.h"
#include "ProjectileServer.h"
#include "Scriptable/Actor.h"

SPLExtHeader::SPLExtHeader(void)
{
	features = NULL;
}

SPLExtHeader::~SPLExtHeader(void)
{
	delete [] features;
}

Spell::Spell(void)
{
	ext_headers = NULL;
	casting_features = NULL;
}

Spell::~Spell(void)
{
	//Spell is in the core, so this is not needed, i guess (Avenger)
	//core->FreeSPLExt(ext_headers, casting_features);
	delete [] ext_headers;
	delete [] casting_features;
}

int Spell::GetHeaderIndexFromLevel(int level) const
{
	if (level<0) return -1;
	if (Flags & SF_SIMPLIFIED_DURATION) {
		return level;
	}
	int block_index;
	for(block_index=0;block_index<ExtHeaderCount-1;block_index++) {
		if (ext_headers[block_index+1].RequiredLevel>level) {
			return block_index;
		}
	}
	return ExtHeaderCount-1;
}

//-1 will return cfb
//0 will always return first spell block
//otherwise set to caster level
static EffectRef fx_casting_glow_ref = { "CastingGlow", -1 };

void Spell::AddCastingGlow(EffectQueue *fxqueue, ieDword duration, int gender)
{
	char g, t;
	Effect *fx;
	ieResRef Resource;
	
	int cgsound = CastingSound;
	if (cgsound>=0 && duration > 1) {
		//bg2 style
		if(cgsound&0x100) {    
			switch(gender) {
			default: g = 'm'; break;
			case SEX_FEMALE: g = 'f'; break;
			case SEX_OTHER: case SEX_NEITHER: g = 's'; break;
			}
		} else {
			//how style
			
			switch(gender) {
			default: g = 'm'; break;
			case SEX_FEMALE: g = 'f'; break;
			}         
		}
		if (SpellType==IE_SPL_PRIEST) {
			t = 'p';
		} else {
			t = 'm';
		}
		snprintf(Resource, 9,"CHA_%c%c%02d", g, t, cgsound&0xff);
		// only actors have fxqueue's and also the parent function checks for that
		Actor *caster = (Actor *) fxqueue->GetOwner();
		caster->casting_sound = core->GetAudioDrv()->Play(Resource, caster->Pos.x, caster->Pos.y);
	}

	fx = EffectQueue::CreateEffect(fx_casting_glow_ref, 0, CastingGraphics, FX_DURATION_ABSOLUTE);
	fx->Duration = core->GetGame()->GameTime + duration;
	fx->InventorySlot = 0xffff;
	fx->Projectile = 0;
	fxqueue->AddEffect(fx);
	//AddEffect creates a copy, we need to destroy the original
	delete fx;
}

EffectQueue *Spell::GetEffectBlock(Scriptable *self, const Point &pos, int block_index, int level, ieDword pro) const
{
	Effect *features;
	int count;

	//iwd2 has this hack
	if (block_index>=0) {
		if (Flags & SF_SIMPLIFIED_DURATION) {
			features = ext_headers[0].features;
			count = ext_headers[0].FeatureCount;
		} else {
			features = ext_headers[block_index].features;
			count = ext_headers[block_index].FeatureCount;
		}
	} else {
		features = casting_features;
		count = CastingFeatureCount;
	}
	EffectQueue *fxqueue = new EffectQueue();
	EffectQueue *selfqueue = NULL;

	for (int i=0;i<count;i++) {
		Effect *fx = features+i;

		if ((Flags & SF_SIMPLIFIED_DURATION) && (block_index>=0)) {
			//hack the effect according to Level
			//fxqueue->AddEffect will copy the effect,
			//so we don't risk any overwriting
			if (EffectQueue::HasDuration(features+i)) {
				fx->Duration = (TimePerLevel*block_index+TimeConstant)*core->Time.round_sec;
			}
		}
		//fill these for completeness, inventoryslot is a good way
		//to discern a spell from an item effect

		fx->InventorySlot = 0xffff;
		//the hostile flag is used to determine if this was an attack
		fx->SourceFlags = Flags;
		fx->CasterLevel = level;

		// apply the stat-based spell duration modifier
		if (self->Type == ST_ACTOR) {
			Actor *caster = (Actor *) self;
			if (caster->Modified[IE_SPELLDURATIONMODMAGE] && SpellType == IE_SPL_WIZARD) {
				fx->Duration = (fx->Duration * caster->Modified[IE_SPELLDURATIONMODMAGE]) / 100;
			} else if (caster->Modified[IE_SPELLDURATIONMODPRIEST] && SpellType == IE_SPL_PRIEST) {
				fx->Duration = (fx->Duration * caster->Modified[IE_SPELLDURATIONMODPRIEST]) / 100;
			}
		}

		if (fx->Target != FX_TARGET_SELF) {
			fx->Projectile = pro;
			fxqueue->AddEffect( fx );
		} else {
			fx->Projectile = 0;
			fx->PosX=pos.x;
			fx->PosY=pos.y;
			if (!selfqueue) {
				selfqueue = new EffectQueue();
			}
			// effects should be able to affect non living targets
			//This is done by NULL target, the position should be enough
			//to tell which non-actor object is affected
			selfqueue->AddEffect( fx );
		}
	}
	if (selfqueue) {
		Actor *target = (self->Type==ST_ACTOR)?(Actor *) self:NULL;
		core->ApplyEffectQueue(selfqueue, target, self);
		delete selfqueue;
	}
	return fxqueue;
}

Projectile *Spell::GetProjectile(Scriptable *self, int header, const Point &target) const
{
	SPLExtHeader *seh = GetExtHeader(header);
	if (!seh) {
		printMessage("Spell", "Cannot retrieve spell header!!! ",RED);
		printf("required header: %d, maximum: %d\n", header, (int) ExtHeaderCount);
		return NULL;
	}
	Projectile *pro = core->GetProjectileServer()->GetProjectileByIndex(seh->ProjectileAnimation);
	if (seh->FeatureCount) {
		pro->SetEffects(GetEffectBlock(self, target, header, seh->ProjectileAnimation));
	}
	return pro;
}

//get the casting distance of the spell
//it depends on the casting level of the actor
//if actor isn't given, then the first header is used
unsigned int Spell::GetCastingDistance(Scriptable *Sender) const
{
	int level = 0;
	Actor *actor = NULL;
	if (Sender && Sender->Type==ST_ACTOR) {
		actor = (Actor *) Sender;
		level = actor->GetCasterLevel(SpellType);
	}

	if (level<1) {
		level = 1;
	}
	int idx = GetHeaderIndexFromLevel(level);
	SPLExtHeader *seh = GetExtHeader(idx);
	if (!seh) {
		printMessage("Spell", "Cannot retrieve spell header!!! ",RED);
		printf("required header: %d, maximum: %d\n", idx, (int) ExtHeaderCount);
		return 0;
	}

	if (seh->Target==TARGET_DEAD) {
		return 0xffffffff;
	}
	return (unsigned int) seh->Range;
}
