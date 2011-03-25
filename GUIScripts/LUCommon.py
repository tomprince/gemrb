# -*-python-*-
# GemRB - Infinity Engine Emulator
# Copyright (C) 2009 The GemRB Project
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
#
# LUCommon.py - common functions related to leveling up

import GemRB
import GUICommon
import CommonTables
from ie_stats import *

def GetNextLevelExp (Level, Class):
	"""Returns the amount of XP required to gain the next level."""
	Row = CommonTables.NextLevel.GetRowIndex (Class)
	if Level < CommonTables.NextLevel.GetColumnCount (Row):
		return str (CommonTables.NextLevel.GetValue (Row, Level) )

	return 0

def CanLevelUp(actor):
	"""Returns true if the actor can level up."""

	# get our class and placements for Multi'd and Dual'd characters
	Class = GemRB.GetPlayerStat (actor, IE_CLASS)
	Class = CommonTables.Classes.FindValue (5, Class)
	Class = CommonTables.Classes.GetRowName (Class)
	Multi = GUICommon.IsMultiClassed (actor, 1)
	Dual = GUICommon.IsDualClassed (actor, 1)

	# get all the levels and overall xp here
	xp = GemRB.GetPlayerStat (actor, IE_XP)
	Levels = [GemRB.GetPlayerStat (actor, IE_LEVEL), GemRB.GetPlayerStat (actor, IE_LEVEL2),\
		GemRB.GetPlayerStat (actor, IE_LEVEL3)]

	#TODO: double-check this
	if GemRB.GetPlayerStat(actor, IE_LEVELDRAIN)>0:
		return 0

	if Multi[0] > 1: # multiclassed
		xp = xp/Multi[0] # divide the xp evenly between the classes
		for i in range (Multi[0]):
			# if any class can level, return 1
			ClassIndex = CommonTables.Classes.FindValue (5, Multi[i+1])
			tmpNext = int(GetNextLevelExp (Levels[i], CommonTables.Classes.GetRowName (ClassIndex) ) )
			if tmpNext != 0 and tmpNext <= xp:
				return 1

		# didn't find a class that could level
		return 0
	elif Dual[0] > 0: # dual classed
		# get the class we can level
		Class = CommonTables.Classes.GetRowName (Dual[2])
		if GUICommon.IsDualSwap(actor):
			Levels = [Levels[1], Levels[0], Levels[2]]

	# check the class that can be level (single or dual)
	tmpNext = int(GetNextLevelExp (Levels[0], Class) )
	return (tmpNext != 0 and tmpNext <= xp)

def SetupSavingThrows (pc, Level=None):
	"""Updates an actors saving throws based upon level.

	Level should contain the actors current level.
	If Level is None, it is filled with the actors current level."""

	#storing levels as an array makes them easier to deal with
	if not Level:
		Levels = [GemRB.GetPlayerStat (pc, IE_LEVEL)-1, \
			GemRB.GetPlayerStat (pc, IE_LEVEL2)-1, \
			GemRB.GetPlayerStat (pc, IE_LEVEL3)-1]
	else:
		Levels = []
		for level in Level:
			Levels.append (level-1)

	#get some basic values
	Class = [GemRB.GetPlayerStat (pc, IE_CLASS)]
	Race = GemRB.GetPlayerStat (pc, IE_RACE)

	#adjust the class for multi/dual chars
	Multi = GUICommon.IsMultiClassed (pc, 1)
	Dual = GUICommon.IsDualClassed (pc, 1)
	NumClasses = 1
	if Multi[0]>1: #get each of the multi-classes
		NumClasses = Multi[0]
		Class = [Multi[1], Multi[2], Multi[3]]
	elif Dual[0]: #only worry about the newer class
		Class = [CommonTables.Classes.GetValue (Dual[2], 5)]
		#assume Level is correct if passed
		if GUICommon.IsDualSwap(pc) and not Level:
			Levels = [Levels[1], Levels[0], Levels[2]]
	if NumClasses>len(Levels):
		return

	#see if we can add racial bonuses to saves
	Race = CommonTables.Races.GetRowName (CommonTables.Races.FindValue (3, Race) )
	RaceSaveTableName = CommonTables.Races.GetValue (Race, "SAVE", 0)
	RaceSaveTable = None
	if RaceSaveTableName != "-1" and RaceSaveTableName != "*":
		Con = GemRB.GetPlayerStat (pc, IE_CON, 1)-1
		RaceSaveTable = GemRB.LoadTable (RaceSaveTableName)
		if Con >= RaceSaveTable.GetRowCount ():
			Con = RaceSaveTable.GetRowCount ()-1

	#preload our tables to limit multi-classed lookups
	SaveTables = []
	ClassBonus = 0
	for i in range (NumClasses):
		Row = CommonTables.Classes.FindValue (5, Class[i])
		RowName = CommonTables.Classes.GetRowName (Row)
		SaveName = CommonTables.Classes.GetValue (RowName, "SAVE", 0)
		SaveTables.append (GemRB.LoadTable (SaveName) )
		#use numeric value
		ClassBonus += CommonTables.ClassSkills.GetValue (RowName, "SAVEBONUS", 1)

	if not len (SaveTables):
		return

	#make sure to limit the levels to the table allowable
	MaxLevel = SaveTables[0].GetColumnCount ()-1
	for i in range (len(Levels)):
		if Levels[i] > MaxLevel:
			Levels[i] = MaxLevel

	#save the saves
	for row in range (5):
		CurrentSave = GemRB.GetPlayerStat(pc, IE_SAVEVSDEATH+i, 1)
		for i in range (NumClasses):
			#loop through each class and update the save value if we have
			#a better save
			TmpSave = SaveTables[i].GetValue (row, Levels[i])
			if TmpSave and (TmpSave < CurrentSave or i == 0):
				CurrentSave = TmpSave

		#add racial bonuses if applicable (small pc's)
		if RaceSaveTable:
			CurrentSave -= RaceSaveTable.GetValue (row, Con)

		#add class bonuses if applicable (paladin)
		CurrentSave -= ClassBonus
		GemRB.SetPlayerStat (pc, IE_SAVEVSDEATH+row, CurrentSave)
	return

def GetNextLevelFromExp (XP, Class):
	"""Gets the next level based on current experience."""

	ClassIndex = CommonTables.Classes.FindValue (5, Class)
	ClassName = CommonTables.Classes.GetRowName (ClassIndex)
	Row = CommonTables.NextLevel.GetRowIndex (ClassName)
	for i in range(1, CommonTables.NextLevel.GetColumnCount()-1):
		if XP < CommonTables.NextLevel.GetValue (Row, i):
			return i
	# fix hacked characters that have more xp than the xp cap
	return 40

def SetupThaco (pc, Level=None):
	"""Updates an actors THAC0 based upon level.

	Level should contain the actors current level.
	If Level is None it is filled with the actors current level."""

	#storing levels as an array makes them easier to deal with
	if not Level:
		Levels = [GemRB.GetPlayerStat (pc, IE_LEVEL)-1, \
			GemRB.GetPlayerStat (pc, IE_LEVEL2)-1, \
			GemRB.GetPlayerStat (pc, IE_LEVEL3)-1]
	else:
		Levels = []
		for level in Level:
			Levels.append (level-1)

	#get some basic values
	Class = [GemRB.GetPlayerStat (pc, IE_CLASS)]
	ThacoTable = GemRB.LoadTable ("THAC0")

	#adjust the class for multi/dual chars
	Multi = GUICommon.IsMultiClassed (pc, 1)
	Dual = GUICommon.IsDualClassed (pc, 1)
	NumClasses = 1
	if Multi[0]>1: #get each of the multi-classes
		NumClasses = Multi[0]
		Class = [Multi[1], Multi[2], Multi[3]]
	elif Dual[0]: #only worry about the newer class
		Class = [CommonTables.Classes.GetValue (Dual[2], 5)]
		#assume Level is correct if passed
		if GUICommon.IsDualSwap(pc) and not Level:
			Levels = [Levels[1], Levels[0], Levels[2]]
	if NumClasses>len(Levels):
		return

	#make sure to limit the levels to the table allowable
	MaxLevel = ThacoTable.GetColumnCount ()-1
	for i in range (len(Levels)):
		if Levels[i] > MaxLevel:
			Levels[i] = MaxLevel

	CurrentThaco = GemRB.GetPlayerStat (pc, IE_TOHIT, 1)
	NewThaco = 0
	for i in range (NumClasses):
		#loop through each class and update the save value if we have
		#a better thac0
		ClassName = CommonTables.Classes.GetRowName (CommonTables.Classes.FindValue (5, Class[i]))
		TmpThaco = ThacoTable.GetValue (ClassName, str(Levels[i]+1))
		if TmpThaco < CurrentThaco:
			NewThaco = 1
			CurrentThaco = TmpThaco

	#only update if we have a better thac0
	if NewThaco:
		GemRB.SetPlayerStat (pc, IE_TOHIT, CurrentThaco)
	return

def SetupLore (pc, LevelDiff=None):
	"""Updates an actors lore based upon level.

	Level should contain the actors current level.
	LevelDiff should contain the change in levels.
	Level and LevelDiff must be of the same length.
	If either are None, they are filled with the actors current level."""

	#storing levels as an array makes them easier to deal with
	if not LevelDiff:
		LevelDiffs = [GemRB.GetPlayerStat (pc, IE_LEVEL), \
			GemRB.GetPlayerStat (pc, IE_LEVEL2), \
			GemRB.GetPlayerStat (pc, IE_LEVEL3)]
	else:
		LevelDiffs = []
		for diff in LevelDiff:
			LevelDiffs.append (diff)

	#get some basic values
	Class = [GemRB.GetPlayerStat (pc, IE_CLASS)]
	LoreTable = GemRB.LoadTable ("lore")

	#adjust the class for multi/dual chars
	Multi = GUICommon.IsMultiClassed (pc, 1)
	Dual = GUICommon.IsDualClassed (pc, 1)
	NumClasses = 1
	if Multi[0]>1: #get each of the multi-classes
		NumClasses = Multi[0]
		Class = [Multi[1], Multi[2], Multi[3]]
	elif Dual[0]: #only worry about the newer class
		Class = [CommonTables.Classes.GetValue (Dual[2], 5)]
		#if LevelDiff is passed, we assume it is correct
		if GUICommon.IsDualSwap(pc) and not LevelDiff:
			LevelDiffs = [LevelDiffs[1], LevelDiffs[0], LevelDiffs[2]]
	if NumClasses>len(LevelDiffs):
		return

	#loop through each class and update the lore value if we have
	CurrentLore = GemRB.GetPlayerStat (pc, IE_LORE, 1)
	for i in range (NumClasses):
		#correct unlisted progressions
		ClassName = CommonTables.Classes.GetRowName (CommonTables.Classes.FindValue (5, Class[i]) )
		if ClassName == "SORCERER":
			ClassName = "MAGE"
		elif ClassName == "MONK": #monks have a rate of 1, so this is arbitrary
			ClassName = "CLERIC"

		#add the lore from this class to the total lore
		TmpLore = LevelDiffs[i] * LoreTable.GetValue (ClassName, "RATE", 1)
		if TmpLore:
			CurrentLore += TmpLore

	#update our lore value
	GemRB.SetPlayerStat (pc, IE_LORE, CurrentLore)
	return

def SetupHP (pc, Level=None, LevelDiff=None):
	"""Updates an actors hp based upon level.

	Level should contain the actors current level.
	LevelDiff should contain the change in levels.
	Level and LevelDiff must be of the same length.
	If either are None, they are filled with the actors current level."""

	#storing levels as an array makes them easier to deal with
	if not Level:
		Levels = [GemRB.GetPlayerStat (pc, IE_LEVEL), \
			GemRB.GetPlayerStat (pc, IE_LEVEL2), \
			GemRB.GetPlayerStat (pc, IE_LEVEL3)]
	else:
		Levels = []
		for level in Level:
			Levels.append (level)
	if not LevelDiff:
		LevelDiffs = [GemRB.GetPlayerStat (pc, IE_LEVEL), \
			GemRB.GetPlayerStat (pc, IE_LEVEL2), \
			GemRB.GetPlayerStat (pc, IE_LEVEL3)]
	else:
		LevelDiffs = []
		for diff in LevelDiff:
			LevelDiffs.append (diff)
	if len (Levels) != len (LevelDiffs):
		return

	#get some basic values
	Class = [GemRB.GetPlayerStat (pc, IE_CLASS)]
		
	#adjust the class for multi/dual chars
	Multi = GUICommon.IsMultiClassed (pc, 1)
	Dual = GUICommon.IsDualClassed (pc, 1)
	NumClasses = 1
	if Multi[0]>1: #get each of the multi-classes
		NumClasses = Multi[0]
		Class = [Multi[1], Multi[2], Multi[3]]
	elif Dual[0]: #only worry about the newer class
		#we only get the hp bonus if the old class is reactivated
		if (Levels[0]<=Levels[1]):
			return
		Class = [CommonTables.Classes.GetValue (Dual[2], 5)]
		#if Level and LevelDiff are passed, we assume it is correct
		if GUICommon.IsDualSwap(pc) and not Level and not LevelDiff:
			LevelDiffs = [LevelDiffs[1], LevelDiffs[0], LevelDiffs[2]]
	if NumClasses>len(Levels):
		return

	#get the correct hp for barbarians
	Kit = GUICommon.GetKitIndex (pc)
	ClassName = None
	if Kit and not Dual[0] and Multi[0]<2:
		KitName = CommonTables.KitList.GetValue (Kit, 0, 0)
		if CommonTables.Classes.GetRowIndex (KitName) >= 0:
			ClassName = KitName

	#loop through each class and update the hp
	OldHP = GemRB.GetPlayerStat (pc, IE_MAXHITPOINTS, 1)
	CurrentHP = 0
	Divisor = float (NumClasses)
	for i in range (NumClasses):
		#check this classes hp table for any gain
		if not ClassName or NumClasses > 1:
			ClassName = CommonTables.Classes.GetRowName (CommonTables.Classes.FindValue (5, Class[i]) )
		HPTable = CommonTables.Classes.GetValue (ClassName, "HP")
		HPTable = GemRB.LoadTable (HPTable)

		#make sure we are within table ranges
		MaxLevel = HPTable.GetRowCount()-1
		LowLevel = Levels[i]-LevelDiffs[i]
		HiLevel = Levels[i]
		if LowLevel >= HiLevel:
			continue
		if LowLevel < 0:
			LowLevel = 0
		elif LowLevel > MaxLevel:
			LowLevel = MaxLevel
		if HiLevel < 0:
			HiLevel = 0
		elif HiLevel > MaxLevel:
			HiLevel = MaxLevel

		#add all the hp for the given level
		#we use ceil to ensure each class gets hp
		for level in range(LowLevel, HiLevel):
			rolls = HPTable.GetValue (level, 1)
			bonus = HPTable.GetValue (level, 2)

			# we only do a roll if core diff or higher, or uncheck max
			if rolls:
				if GemRB.GetVar ("Difficulty Level") >= 3 and not GemRB.GetVar ("Maximum HP"):
					CurrentHP += int (GemRB.Roll (rolls, HPTable.GetValue (level, 0), bonus) / Divisor + 0.5)
				else:
					CurrentHP += int ((rolls * HPTable.GetValue (level, 0) + bonus) / Divisor + 0.5)
			else:
				CurrentHP += int (bonus / Divisor + 0.5)
			CurrentHP = int (CurrentHP)

	#update our hp values
	GemRB.SetPlayerStat (pc, IE_MAXHITPOINTS, CurrentHP+OldHP)
	GemRB.SetPlayerStat (pc, IE_HITPOINTS, GemRB.GetPlayerStat (pc, IE_HITPOINTS, 1)+CurrentHP)
	return
