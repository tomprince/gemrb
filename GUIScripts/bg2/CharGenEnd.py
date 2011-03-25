# -*-python-*-
# GemRB - Infinity Engine Emulator
# Copyright (C) 2003-2004 The GemRB Project
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# character generation end
import GemRB
import GUICommon
import CommonTables
import LUCommon
from GUIDefines import *
from ie_slots import *
from ie_stats import *
from ie_spells import *
from ie_restype import RES_2DA

def OnLoad():
	# Lay on hands, turn undead and backstab multiplier get set by the core
	# set my character up
	MyChar = GemRB.GetVar ("Slot")
	Class = GemRB.GetPlayerStat (MyChar, IE_CLASS)
	ClassIndex = CommonTables.Classes.FindValue (5, Class)
	ClassName = CommonTables.Classes.GetRowName (ClassIndex)
	IsMulti = GUICommon.IsMultiClassed (MyChar, 1)
	Levels = [GemRB.GetPlayerStat (MyChar, IE_LEVEL), GemRB.GetPlayerStat (MyChar, IE_LEVEL2), \
			GemRB.GetPlayerStat (MyChar, IE_LEVEL3)]

	# weapon proficiencies
	# set the base number of attacks; effects will add the proficiency bonus
	# 2 means 1 attack, because this is the number of attacks in 2 rounds
	GemRB.SetPlayerStat (MyChar, IE_NUMBEROFATTACKS, 2)

	#lore, thac0, hp, and saves
	GemRB.SetPlayerStat (MyChar, IE_MAXHITPOINTS, 0)
	GemRB.SetPlayerStat (MyChar, IE_HITPOINTS, 0)
	LUCommon.SetupSavingThrows (MyChar)
	LUCommon.SetupThaco (MyChar)
	LUCommon.SetupLore (MyChar)
	LUCommon.SetupHP (MyChar)

	# mage spells
	TableName = CommonTables.ClassSkills.GetValue (Class, 2, 0)
	if TableName != "*":
		index = 0
		if IsMulti[0]>1:
			#find out which class gets mage spells
			for i in range (IsMulti[0]):
				if CommonTables.ClassSkills.GetValue (IsMulti[i+1], 2, 0) != "*":
					index = i
					break
		GUICommon.SetupSpellLevels(MyChar, TableName, IE_SPELL_TYPE_WIZARD, Levels[index])

	# apply class/kit abilities
	KitIndex = GUICommon.GetKitIndex (MyChar)
	if IsMulti[0]>1:
		#get the class abilites for each class
		for i in range (IsMulti[0]):
			TmpClassName = CommonTables.Classes.GetRowName (CommonTables.Classes.FindValue (5, IsMulti[i+1]) )
			ABTable = CommonTables.ClassSkills.GetValue (TmpClassName, "ABILITIES")
			if ABTable != "*" and GemRB.HasResource (ABTable, RES_2DA, 1):
				GUICommon.AddClassAbilities (MyChar, ABTable, Levels[i], Levels[i])
	else:
		if KitIndex:
			ABTable = CommonTables.KitList.GetValue (str(KitIndex), "ABILITIES")
		else:
			ABTable = CommonTables.ClassSkills.GetValue (ClassName, "ABILITIES")
		if ABTable != "*" and GemRB.HasResource (ABTable, RES_2DA, 1):
			GUICommon.AddClassAbilities (MyChar, ABTable, Levels[0], Levels[0])

	# apply starting (alignment dictated) abilities
	# pc, table, new level, level diff, alignment
	AlignmentTable = GemRB.LoadTable ("aligns")
	AlignmentAbbrev = AlignmentTable.FindValue (3, GemRB.GetPlayerStat (MyChar, IE_ALIGNMENT))
	GUICommon.AddClassAbilities (MyChar, "abstart", 6,6, AlignmentAbbrev)

	# setup starting gold (uses a roll dictated by class
	TmpTable = GemRB.LoadTable ("strtgold")
	temp = GemRB.Roll (TmpTable.GetValue (Class, 1),TmpTable.GetValue (Class, 0), TmpTable.GetValue (Class, 2))
	GemRB.SetPlayerStat (MyChar, IE_GOLD, temp * TmpTable.GetValue (Class, 3))

	# save the appearance
	GUICommon.SetColorStat (MyChar, IE_HAIR_COLOR, GemRB.GetVar ("HairColor") )
	GUICommon.SetColorStat (MyChar, IE_SKIN_COLOR, GemRB.GetVar ("SkinColor") )
	GUICommon.SetColorStat (MyChar, IE_MAJOR_COLOR, GemRB.GetVar ("MajorColor") )
	GUICommon.SetColorStat (MyChar, IE_MINOR_COLOR, GemRB.GetVar ("MinorColor") )
	#GUICommon.SetColorStat (MyChar, IE_METAL_COLOR, 0x1B )
	#GUICommon.SetColorStat (MyChar, IE_LEATHER_COLOR, 0x16 )
	#GUICommon.SetColorStat (MyChar, IE_ARMOR_COLOR, 0x17 )
	GemRB.SetPlayerStat (MyChar, IE_EA, 2 )

	# save the name and starting xp (can level right away in game)
	GemRB.SetPlayerName (MyChar, GemRB.GetToken ("CHARNAME"), 0)

	# does all the rest
	LargePortrait = GemRB.GetToken ("LargePortrait")
	SmallPortrait = GemRB.GetToken ("SmallPortrait")
	GemRB.FillPlayerInfo (MyChar, LargePortrait, SmallPortrait)

	if GUICommon.GameIsTOB():
		# add the starting inventory for tob
		GiveEquipment(MyChar, ClassName, KitIndex)

	playmode = GemRB.GetVar ("PlayMode")
	if playmode >=0:
		if GemRB.GetVar("GUIEnhancements"):
			GemRB.SaveCharacter ( MyChar, "gembak" )
		#LETS PLAY!!
		GemRB.EnterGame()
		GemRB.ExecuteString ("EquipMostDamagingMelee()", MyChar)
	else:
		#leaving multi player pregen
		if CharGenWindow:
			CharGenWindow.Unload ()
		#when export is done, go to start
		if GUICommon.HasTOB():
			GemRB.SetToken ("NextScript","Start2")
		else:
			GemRB.SetToken ("NextScript","Start")
		GemRB.SetNextScript ("ExportFile") #export
	return

def GiveEquipment(MyChar, ClassName, KitIndex):
		# get the kit (or use class if no kit) to load the start table
		if KitIndex == 0:
			EquipmentColName = ClassName
			# sorcerers are missing from the table, use the mage equipment instead
			if EquipmentColName == "SORCERER":
				EquipmentColName = "MAGE"
		else:
			EquipmentColName = CommonTables.KitList.GetValue (KitIndex, 0)

		EquipmentTable = GemRB.LoadTable ("25stweap")

		# a map of slots in the table to the real slots
		# SLOT_BAG is invalid, so use the inventory (first occurence)
		# SLOT_INVENTORY: use -1 instead, that's what CreateItem expects
		RealSlots = [ SLOT_ARMOUR, SLOT_SHIELD, SLOT_HELM, -1, SLOT_RING, \
					SLOT_RING, SLOT_CLOAK, SLOT_BOOT, SLOT_AMULET, SLOT_GLOVE, \
					SLOT_BELT, SLOT_QUIVER, SLOT_QUIVER, SLOT_QUIVER, \
					SLOT_ITEM, SLOT_ITEM, SLOT_ITEM, SLOT_WEAPON, SLOT_WEAPON, SLOT_WEAPON ]
		inventory_exclusion = 0

		#loop over rows - item slots
		for slot in range(0, EquipmentTable.GetRowCount ()):
			slotname = EquipmentTable.GetRowName (slot)
			item_resref = EquipmentTable.GetValue (slotname, EquipmentColName)

			# no item - go to next
			if item_resref == "*":
				continue

			# the table has typos for kitted bard's armor
			if item_resref == "LEATH14":
				item_resref = "LEAT14"

			# get empty slots of the requested type
			realslot = GemRB.GetSlots (MyChar, RealSlots[slot], -1)
			if RealSlots[slot] == SLOT_WEAPON:
				# exclude the shield slot, so the offhand stays empty
				realslot = realslot[1:]

			if realslot == (): # fallback to the inventory
				realslot = GemRB.GetSlots (MyChar, -1, -1)

			if realslot == (): # this shouldn't happen!
				print "Eeek! No free slots for", item_resref
				continue

			# if an item contains a comma, the rest of the value is the stack
			if "," in item_resref:
				item_resref = item_resref.split(",")
				count = int(item_resref[1])
				item_resref = item_resref[0]
			else:
				count = 0

			targetslot = realslot[0]
			SlotType = GemRB.GetSlotType (targetslot, MyChar)
			i = 1
			item = GemRB.GetItem (item_resref)

			if inventory_exclusion & item['Exclusion']:
				# oops, too many magic items to equip, so just dump it to the inventory
				targetslot = GemRB.GetSlots (MyChar, -1, -1)[0]
				SlotType = -1
			else:
				# if there are no free slots, CreateItem will create the item on the ground
				while not GemRB.CanUseItemType (SlotType["Type"], item_resref, MyChar) \
				and i < len(realslot):
					targetslot = realslot[i]
					SlotType = GemRB.GetSlotType (targetslot, MyChar)
					i = i + 1

			GemRB.CreateItem(MyChar, item_resref, targetslot, count, 0, 0)
 			GemRB.ChangeItemFlag (MyChar, targetslot, IE_INV_ITEM_IDENTIFIED, OP_OR)
			inventory_exclusion |= item['Exclusion']

		# grant the slayer change ability to the protagonist
		if MyChar == 1:
			GemRB.LearnSpell (MyChar, "SPIN822", LS_MEMO)
		return
		
