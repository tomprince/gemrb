#include "Script.h"

#include "Actor.h"
#include "ActorBlock.h"

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
